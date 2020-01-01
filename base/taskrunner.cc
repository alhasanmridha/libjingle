/*
 * libjingle
 * Copyright 2004--2006, Google Inc.
 *
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions are met:
 *
 *  1. Redistributions of source code must retain the above copyright notice, 
 *     this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 *  3. The name of the author may not be used to endorse or promote products 
 *     derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR 
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <algorithm>

#include "base/taskrunner.h"

#include "base/common.h"
#include "base/scoped_ptr.h"
#include "base/task.h"
#include "base/logging.h"

namespace talk_base {

TaskRunner::TaskRunner()
  : TaskParent(this),
    next_timeout_task_(NULL),
    tasks_running_(false)
#ifdef _DEBUG
    , abort_count_(0),
    deleting_task_(NULL)
#endif
{
}

TaskRunner::~TaskRunner() {
  // this kills and deletes children silently!
  AbortAllChildren();
  InternalRunTasks(true);
}

void TaskRunner::StartTask(Task * task) {
  tasks_.push_back(task);

  // the task we just started could be about to timeout --
  // make sure our "next timeout task" is correct
  UpdateTaskTimeout(task, 0);

  WakeTasks();
}

void TaskRunner::RunTasks() {
  InternalRunTasks(false);
}

void TaskRunner::InternalRunTasks(bool in_destructor) {
  // This shouldn't run while an abort is happening.
  // If that occurs, then tasks may be deleted in this method,
  // but pointers to them will still be in the
  // "ChildSet copy" in TaskParent::AbortAllChildren.
  // Subsequent use of those task may cause data corruption or crashes.  
  ASSERT(!abort_count_);
  // Running continues until all tasks are Blocked (ok for a small # of tasks)
  if (tasks_running_) {
    return;  // don't reenter
  }

  tasks_running_ = true;

  int64 previous_timeout_time = next_task_timeout();

  int did_run = true;
  while (did_run) {
    did_run = false;
    // use indexing instead of iterators because tasks_ may grow
    for (size_t i = 0; i < tasks_.size(); ++i) {
      while (!tasks_[i]->Blocked()) {
        tasks_[i]->Step();
        did_run = true;
      }
    }
  }
  // Tasks are deleted when running has paused
  bool need_timeout_recalc = false;
  for (size_t i = 0; i < tasks_.size(); ++i) {
    if (tasks_[i]->IsDone()) {
      Task* task = tasks_[i];
      if (next_timeout_task_ &&
          task->unique_id() == next_timeout_task_->unique_id()) {
        next_timeout_task_ = NULL;
        need_timeout_recalc = true;
      }

#ifdef _DEBUG
      deleting_task_ = task;
#endif
      delete task;
#ifdef _DEBUG
      deleting_task_ = NULL;
#endif
      tasks_[i] = NULL;
    }
  }
  // Finally, remove nulls
  std::vector<Task *>::iterator it;
  it = std::remove(tasks_.begin(),
                   tasks_.end(),
                   reinterpret_cast<Task *>(NULL));

  tasks_.erase(it, tasks_.end());

  if (need_timeout_recalc)
    RecalcNextTimeout(NULL);

  // Make sure that adjustments are done to account
  // for any timeout changes (but don't call this
  // while being destroyed since it calls a pure virtual function).
  if (!in_destructor)
    CheckForTimeoutChange(previous_timeout_time);

  tasks_running_ = false;
}

void TaskRunner::PollTasks() {
  // see if our "next potentially timed-out task" has indeed timed out.
  // If it has, wake it up, then queue up the next task in line
  // Repeat while we have new timed-out tasks.
  // TODO: We need to guard against WakeTasks not updating
  // next_timeout_task_. Maybe also add documentation in the header file once
  // we understand this code better.
  Task* old_timeout_task = NULL;
  while (next_timeout_task_ &&
      old_timeout_task != next_timeout_task_ &&
      next_timeout_task_->TimedOut()) {
    old_timeout_task = next_timeout_task_;
    next_timeout_task_->Wake();
    WakeTasks();
  }
}

int64 TaskRunner::next_task_timeout() const {
  if (next_timeout_task_) {
    return next_timeout_task_->timeout_time();
  }
  return 0;
}

// this function gets called frequently -- when each task changes
// state to something other than DONE, ERROR or BLOCKED, it calls
// ResetTimeout(), which will call this function to make sure that
// the next timeout-able task hasn't changed.  The logic in this function
// prevents RecalcNextTimeout() from getting called in most cases,
// effectively making the task scheduler O-1 instead of O-N

void TaskRunner::UpdateTaskTimeout(Task* task,
                                   int64 previous_task_timeout_time) {
  ASSERT(task != NULL);
  int64 previous_timeout_time = next_task_timeout();
  bool task_is_timeout_task = next_timeout_task_ != NULL &&
      task->unique_id() == next_timeout_task_->unique_id();
  if (task_is_timeout_task) {
    previous_timeout_time = previous_task_timeout_time;
  }

  // if the relevant task has a timeout, then
  // check to see if it's closer than the current
  // "about to timeout" task
  if (task->timeout_time()) {
    if (next_timeout_task_ == NULL ||
        (task->timeout_time() <= next_timeout_task_->timeout_time())) {
      next_timeout_task_ = task;
    }
  } else if (task_is_timeout_task) {
    // otherwise, if the task doesn't have a timeout,
    // and it used to be our "about to timeout" task,
    // walk through all the tasks looking for the real
    // "about to timeout" task
    RecalcNextTimeout(task);
  }

  // Note when task_running_, then the running routine
  // (TaskRunner::InternalRunTasks) is responsible for calling
  // CheckForTimeoutChange.
  if (!tasks_running_) {
    CheckForTimeoutChange(previous_timeout_time);
  }
}

void TaskRunner::RecalcNextTimeout(Task *exclude_task) {
  // walk through all the tasks looking for the one
  // which satisfies the following:
  //   it's not finished already
  //   we're not excluding it
  //   it has the closest timeout time

  int64 next_timeout_time = 0;
  next_timeout_task_ = NULL;

  for (size_t i = 0; i < tasks_.size(); ++i) {
    Task *task = tasks_[i];
    // if the task isn't complete, and it actually has a timeout time
    if (!task->IsDone() && (task->timeout_time() > 0))
      // if it doesn't match our "exclude" task
      if (exclude_task == NULL ||
          exclude_task->unique_id() != task->unique_id())
        // if its timeout time is sooner than our current timeout time
        if (next_timeout_time == 0 ||
            task->timeout_time() <= next_timeout_time) {
          // set this task as our next-to-timeout
          next_timeout_time = task->timeout_time();
          next_timeout_task_ = task;
        }
  }
}

void TaskRunner::CheckForTimeoutChange(int64 previous_timeout_time) {
  int64 next_timeout = next_task_timeout();
  bool timeout_change = (previous_timeout_time == 0 && next_timeout != 0) ||
      next_timeout < previous_timeout_time ||
      (previous_timeout_time <= CurrentTime() &&
       previous_timeout_time != next_timeout);
  if (timeout_change) {
    OnTimeoutChange();
  }
}

} // namespace talk_base
