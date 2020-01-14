/*
 * libjingle
 * Copyright 2004--2005, Google Inc.
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

#ifndef _MUC_H_
#define _MUC_H_

#include <map>
#include "engine/xmpp/jid.h"
#include "engine/xmpp/presencestatus.h"

namespace buzz {

class Muc {
 public:
   Muc(const Jid& jid, const std::string& nick) : state_(MUC_JOINING),
       jid_(jid), local_jid_(Jid(jid.Str() + "/" + nick)) {}
  ~Muc() {};

  enum State { MUC_JOINING, MUC_JOINED, MUC_LEAVING };
  State state() const { return state_; }
  void set_state(State state) { state_ = state; }
  const Jid & jid() const { return jid_; }
  const Jid & local_jid() const { return local_jid_; }

  typedef std::map<std::string, MucPresenceStatus> MemberMap;

  // All the intelligence about how to manage the members is in
  // CallClient, so we completely expose the map.
  MemberMap& members() {
    return members_;
  }

private:
  State state_;
  Jid jid_;
  Jid local_jid_;
  MemberMap members_;
};

}

#endif
