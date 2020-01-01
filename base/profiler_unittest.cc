/*
 * libjingle
 * Copyright 2004--2013, Google Inc.
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

#include "base/gunit.h"
#include "base/profiler.h"
#include "base/thread.h"

namespace {

const int kWaitMs = 250;
const double kWaitSec = 0.250;
const double kTolerance = 0.1;

const char* TestFunc() {
  PROFILE_F();
  talk_base::Thread::SleepMs(kWaitMs);
  return __FUNCTION__;
}

}  // namespace

namespace talk_base {

TEST(ProfilerTest, TestFunction) {
  ASSERT_TRUE(Profiler::Instance()->Clear());
  // Profile a long-running function.
  const char* function_name = TestFunc();
  const ProfilerEvent* event = Profiler::Instance()->GetEvent(function_name);
  ASSERT_TRUE(event != NULL);
  EXPECT_FALSE(event->is_started());
  EXPECT_EQ(1, event->event_count());
  EXPECT_NEAR(kWaitSec, event->mean(), kTolerance);
  // Run it a second time.
  TestFunc();
  EXPECT_FALSE(event->is_started());
  EXPECT_EQ(2, event->event_count());
  EXPECT_NEAR(kWaitSec, event->mean(), kTolerance);
  EXPECT_NEAR(kWaitSec * 2, event->total_time(), kTolerance * 2);
  EXPECT_DOUBLE_EQ(event->mean(), event->total_time() / event->event_count());
}

TEST(ProfilerTest, TestScopedEvents) {
  const std::string kEvent1Name = "Event 1";
  const std::string kEvent2Name = "Event 2";
  const int kEvent2WaitMs = 150;
  const double kEvent2WaitSec = 0.150;
  const ProfilerEvent* event1;
  const ProfilerEvent* event2;
  ASSERT_TRUE(Profiler::Instance()->Clear());
  {  // Profile a scope.
    PROFILE(kEvent1Name);
    event1 = Profiler::Instance()->GetEvent(kEvent1Name);
    ASSERT_TRUE(event1 != NULL);
    EXPECT_TRUE(event1->is_started());
    EXPECT_EQ(0, event1->event_count());
    talk_base::Thread::SleepMs(kWaitMs);
    EXPECT_TRUE(event1->is_started());
  }
  // Check the result.
  EXPECT_FALSE(event1->is_started());
  EXPECT_EQ(1, event1->event_count());
  EXPECT_NEAR(kWaitSec, event1->mean(), kTolerance);
  {  // Profile a second event.
    PROFILE(kEvent2Name);
    event2 = Profiler::Instance()->GetEvent(kEvent2Name);
    ASSERT_TRUE(event2 != NULL);
    EXPECT_FALSE(event1->is_started());
    EXPECT_TRUE(event2->is_started());
    talk_base::Thread::SleepMs(kEvent2WaitMs);
  }
  // Check the result.
  EXPECT_FALSE(event2->is_started());
  EXPECT_EQ(1, event2->event_count());
  EXPECT_NEAR(kEvent2WaitSec, event2->mean(), kTolerance);
  // Make sure event1 is unchanged.
  EXPECT_FALSE(event1->is_started());
  EXPECT_EQ(1, event1->event_count());
  {  // Run another event 1.
    PROFILE(kEvent1Name);
    EXPECT_TRUE(event1->is_started());
    talk_base::Thread::SleepMs(kWaitMs);
  }
  // Check the result.
  EXPECT_FALSE(event1->is_started());
  EXPECT_EQ(2, event1->event_count());
  EXPECT_NEAR(kWaitSec, event1->mean(), kTolerance);
  EXPECT_NEAR(kWaitSec * 2, event1->total_time(), kTolerance * 2);
  EXPECT_DOUBLE_EQ(event1->mean(),
                   event1->total_time() / event1->event_count());
}

TEST(ProfilerTest, Clear) {
  ASSERT_TRUE(Profiler::Instance()->Clear());
  PROFILE_START("event");
  EXPECT_FALSE(Profiler::Instance()->Clear());
  EXPECT_TRUE(Profiler::Instance()->GetEvent("event") != NULL);
  PROFILE_STOP("event");
  EXPECT_TRUE(Profiler::Instance()->Clear());
  EXPECT_EQ(NULL, Profiler::Instance()->GetEvent("event"));
}

}  // namespace talk_base
