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

#include "pseudotcp.h"

#include <cstdio>
#include <cstdlib>
#include <set>

#include "basictypes.h"
#include "bytebuffer.h"
#include "byteorder.h"
#include "common.h"
#include "logging.h"
#include "socket.h"
#include "stringutils.h"
#include "timeutils.h"

// The following logging is for detailed (packet-level) analysis only.
#define _DBG_NONE     0
#define _DBG_NORMAL   1
#define _DBG_VERBOSE  2
#define _DEBUGMSG _DBG_NONE

namespace cricket {

//////////////////////////////////////////////////////////////////////
// Network Constants
//////////////////////////////////////////////////////////////////////

// Standard MTUs
const uint16 PACKET_MAXIMUMS[] = {
  65535,    // Theoretical maximum, Hyperchannel
  32000,    // Nothing
  17914,    // 16Mb IBM Token Ring
  8166,   // IEEE 802.4
  //4464,   // IEEE 802.5 (4Mb max)
  4352,   // FDDI
  //2048,   // Wideband Network
  2002,   // IEEE 802.5 (4Mb recommended)
  //1536,   // Expermental Ethernet Networks
  //1500,   // Ethernet, Point-to-Point (default)
  1492,   // IEEE 802.3
  1006,   // SLIP, ARPANET
  //576,    // X.25 Networks
  //544,    // DEC IP Portal
  //512,    // NETBIOS
  508,    // IEEE 802/Source-Rt Bridge, ARCNET
  296,    // Point-to-Point (low delay)
  //68,     // Official minimum
  0,      // End of list marker
};

const uint32 MAX_PACKET = 65535;
// Note: we removed lowest level because packet overhead was larger!
const uint32 MIN_PACKET = 296;

const uint32 IP_HEADER_SIZE = 20; // (+ up to 40 bytes of options?)
const uint32 ICMP_HEADER_SIZE = 8;
const uint32 UDP_HEADER_SIZE = 8;
// TODO: Make JINGLE_HEADER_SIZE transparent to this code?
const uint32 JINGLE_HEADER_SIZE = 64; // when relay framing is in use

// Default size for receive and send buffer.
const uint32 DEFAULT_RCV_BUF_SIZE = 60 * 1024;
const uint32 DEFAULT_SND_BUF_SIZE = 90 * 1024;

//////////////////////////////////////////////////////////////////////
// Global Constants and Functions
//////////////////////////////////////////////////////////////////////
//
//    0                   1                   2                   3
//    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//  0 |                      Conversation Number                      |
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//  4 |                        Sequence Number                        |
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//  8 |                     Acknowledgment Number                     |
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//    |               |   |U|A|P|R|S|F|                               |
// 12 |    Control    |   |R|C|S|S|Y|I|            Window             |
//    |               |   |G|K|H|T|N|N|                               |
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// 16 |                       Timestamp sending                       |
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// 20 |                      Timestamp receiving                      |
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// 24 |                             data                              |
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
//////////////////////////////////////////////////////////////////////

#define PSEUDO_KEEPALIVE 0

const uint32 MAX_SEQ = 0xFFFFFFFF;
const uint32 HEADER_SIZE = 24;
const uint32 PACKET_OVERHEAD = HEADER_SIZE + UDP_HEADER_SIZE + IP_HEADER_SIZE + JINGLE_HEADER_SIZE;

const uint32 MIN_RTO   =   250; // 250 ms (RFC1122, Sec 4.2.3.1 "fractions of a second")
const uint32 DEF_RTO   =  3000; // 3 seconds (RFC1122, Sec 4.2.3.1)
const uint32 MAX_RTO   = 60000; // 60 seconds
const uint32 DEF_ACK_DELAY = 100; // 100 milliseconds

const uint8 FLAG_CTL = 0x02;
const uint8 FLAG_RST = 0x04;

const uint8 CTL_CONNECT = 0;
//const uint8 CTL_REDIRECT = 1;
const uint8 CTL_EXTRA = 255;

// TCP options.
const uint8 TCP_OPT_EOL = 0;  // End of list.
const uint8 TCP_OPT_NOOP = 1;  // No-op.
const uint8 TCP_OPT_MSS = 2;  // Maximum segment size.
const uint8 TCP_OPT_WND_SCALE = 3;  // Window scale factor.

/*
const uint8 FLAG_FIN = 0x01;
const uint8 FLAG_SYN = 0x02;
const uint8 FLAG_ACK = 0x10;
*/

const uint32 CTRL_BOUND = 0x80000000;

const long DEFAULT_TIMEOUT = 4000; // If there are no pending clocks, wake up every 4 seconds
const long CLOSED_TIMEOUT = 60 * 1000; // If the connection is closed, once per minute

#if PSEUDO_KEEPALIVE
// !?! Rethink these times
const uint32 IDLE_PING = 20 * 1000; // 20 seconds (note: WinXP SP2 firewall udp timeout is 90 seconds)
const uint32 IDLE_TIMEOUT = 90 * 1000; // 90 seconds;
#endif // PSEUDO_KEEPALIVE

//////////////////////////////////////////////////////////////////////
// Helper Functions
//////////////////////////////////////////////////////////////////////

inline void long_to_bytes(uint32 val, void* buf) {
  *static_cast<uint32*>(buf) = talk_base::HostToNetwork32(val);
}

inline void short_to_bytes(uint16 val, void* buf) {
  *static_cast<uint16*>(buf) = talk_base::HostToNetwork16(val);
}

inline uint32 bytes_to_long(const void* buf) {
  return talk_base::NetworkToHost32(*static_cast<const uint32*>(buf));
}

inline uint16 bytes_to_short(const void* buf) {
  return talk_base::NetworkToHost16(*static_cast<const uint16*>(buf));
}

uint32 bound(uint32 lower, uint32 middle, uint32 upper) {
  return talk_base::_min(talk_base::_max(lower, middle), upper);
}

//////////////////////////////////////////////////////////////////////
// Debugging Statistics
//////////////////////////////////////////////////////////////////////

#if 0  // Not used yet

enum Stat {
  S_SENT_PACKET,   // All packet sends
  S_RESENT_PACKET, // All packet sends that are retransmits
  S_RECV_PACKET,   // All packet receives
  S_RECV_NEW,      // All packet receives that are too new
  S_RECV_OLD,      // All packet receives that are too old
  S_NUM_STATS
};

const char* const STAT_NAMES[S_NUM_STATS] = {
  "snt",
  "snt-r",
  "rcv"
  "rcv-n",
  "rcv-o"
};

int g_stats[S_NUM_STATS];
inline void Incr(Stat s) { ++g_stats[s]; }
void ReportStats() {
  char buffer[256];
  size_t len = 0;
  for (int i = 0; i < S_NUM_STATS; ++i) {
    len += talk_base::sprintfn(buffer, ARRAY_SIZE(buffer), "%s%s:%d",
                               (i == 0) ? "" : ",", STAT_NAMES[i], g_stats[i]);
    g_stats[i] = 0;
  }
  LOG(LS_INFO) << "Stats[" << buffer << "]";
}

#endif

//////////////////////////////////////////////////////////////////////
// PseudoTcp
//////////////////////////////////////////////////////////////////////

uint32 PseudoTcp::Now() {
#if 0  // Use this to synchronize timers with logging timestamps (easier debug)
  return talk_base::TimeSince(StartTime());
#else
  return talk_base::Time();
#endif
}

PseudoTcp::PseudoTcp(IPseudoTcpNotify* notify, uint32 conv)
    : m_notify(notify),
      m_shutdown(SD_NONE),
      m_error(0),
      m_rbuf_len(DEFAULT_RCV_BUF_SIZE),
      m_rbuf(m_rbuf_len),
      m_sbuf_len(DEFAULT_SND_BUF_SIZE),
      m_sbuf(m_sbuf_len) {

  // Sanity check on buffer sizes (needed for OnTcpWriteable notification logic)
  ASSERT(m_rbuf_len + MIN_PACKET < m_sbuf_len);

  uint32 now = Now();

  m_state = TCP_LISTEN;
  m_conv = conv;
  m_rcv_wnd = m_rbuf_len;
  m_rwnd_scale = m_swnd_scale = 0;
  m_snd_nxt = 0;
  m_snd_wnd = 1;
  m_snd_una = m_rcv_nxt = 0;
  m_bReadEnable = true;
  m_bWriteEnable = false;
  m_t_ack = 0;

  m_msslevel = 0;
  m_largest = 0;
  ASSERT(MIN_PACKET > PACKET_OVERHEAD);
  m_mss = MIN_PACKET - PACKET_OVERHEAD;
  m_mtu_advise = MAX_PACKET;

  m_rto_base = 0;

  m_cwnd = 2 * m_mss;
  m_ssthresh = m_rbuf_len;
  m_lastrecv = m_lastsend = m_lasttraffic = now;
  m_bOutgoing = false;

  m_dup_acks = 0;
  m_recover = 0;

  m_ts_recent = m_ts_lastack = 0;

  m_rx_rto = DEF_RTO;
  m_rx_srtt = m_rx_rttvar = 0;

  m_use_nagling = true;
  m_ack_delay = DEF_ACK_DELAY;
  m_support_wnd_scale = true;
}

PseudoTcp::~PseudoTcp() {
}

int PseudoTcp::Connect() {
  if (m_state != TCP_LISTEN) {
    m_error = EINVAL;
    return -1;
  }

  m_state = TCP_SYN_SENT;
  LOG(LS_INFO) << "State: TCP_SYN_SENT";

  queueConnectMessage();
  attemptSend();

  return 0;
}

void PseudoTcp::NotifyMTU(uint16 mtu) {
  m_mtu_advise = mtu;
  if (m_state == TCP_ESTABLISHED) {
    adjustMTU();
  }
}

void PseudoTcp::NotifyClock(uint32 now) {
  if (m_state == TCP_CLOSED)
    return;

    // Check if it's time to retransmit a segment
  if (m_rto_base && (talk_base::TimeDiff(m_rto_base + m_rx_rto, now) <= 0)) {
    if (m_slist.empty()) {
      ASSERT(false);
    } else {
      // Note: (m_slist.front().xmit == 0)) {
      // retransmit segments
#if _DEBUGMSG >= _DBG_NORMAL
      LOG(LS_INFO) << "timeout retransmit (rto: " << m_rx_rto
                   << ") (rto_base: " << m_rto_base
                   << ") (now: " << now
                   << ") (dup_acks: " << static_cast<unsigned>(m_dup_acks)
                   << ")";
#endif // _DEBUGMSG
      if (!transmit(m_slist.begin(), now)) {
        closedown(ECONNABORTED);
        return;
      }

      uint32 nInFlight = m_snd_nxt - m_snd_una;
      m_ssthresh = talk_base::_max(nInFlight / 2, 2 * m_mss);
      //LOG(LS_INFO) << "m_ssthresh: " << m_ssthresh << "  nInFlight: " << nInFlight << "  m_mss: " << m_mss;
      m_cwnd = m_mss;

      // Back off retransmit timer.  Note: the limit is lower when connecting.
      uint32 rto_limit = (m_state < TCP_ESTABLISHED) ? DEF_RTO : MAX_RTO;
      m_rx_rto = talk_base::_min(rto_limit, m_rx_rto * 2);
      m_rto_base = now;
    }
  }

  // Check if it's time to probe closed windows
  if ((m_snd_wnd == 0)
        && (talk_base::TimeDiff(m_lastsend + m_rx_rto, now) <= 0)) {
    if (talk_base::TimeDiff(now, m_lastrecv) >= 15000) {
      closedown(ECONNABORTED);
      return;
    }

    // probe the window
    packet(m_snd_nxt - 1, 0, 0, 0);
    m_lastsend = now;

    // back off retransmit timer
    m_rx_rto = talk_base::_min(MAX_RTO, m_rx_rto * 2);
  }

  // Check if it's time to send delayed acks
  if (m_t_ack && (talk_base::TimeDiff(m_t_ack + m_ack_delay, now) <= 0)) {
    packet(m_snd_nxt, 0, 0, 0);
  }

#if PSEUDO_KEEPALIVE
  // Check for idle timeout
  if ((m_state == TCP_ESTABLISHED) && (TimeDiff(m_lastrecv + IDLE_TIMEOUT, now) <= 0)) {
    closedown(ECONNABORTED);
    return;
  }

  // Check for ping timeout (to keep udp mapping open)
  if ((m_state == TCP_ESTABLISHED) && (TimeDiff(m_lasttraffic + (m_bOutgoing ? IDLE_PING * 3/2 : IDLE_PING), now) <= 0)) {
    packet(m_snd_nxt, 0, 0, 0);
  }
#endif // PSEUDO_KEEPALIVE
}

bool PseudoTcp::NotifyPacket(const char* buffer, size_t len) {
  if (len > MAX_PACKET) {
    LOG_F(WARNING) << "packet too large";
    return false;
  }
  return parse(reinterpret_cast<const uint8 *>(buffer), uint32(len));
}

bool PseudoTcp::GetNextClock(uint32 now, long& timeout) {
  return clock_check(now, timeout);
}

void PseudoTcp::GetOption(Option opt, int* value) {
  if (opt == OPT_NODELAY) {
    *value = m_use_nagling ? 0 : 1;
  } else if (opt == OPT_ACKDELAY) {
    *value = m_ack_delay;
  } else if (opt == OPT_SNDBUF) {
    *value = m_sbuf_len;
  } else if (opt == OPT_RCVBUF) {
    *value = m_rbuf_len;
  } else {
    ASSERT(false);
  }
}
void PseudoTcp::SetOption(Option opt, int value) {
  if (opt == OPT_NODELAY) {
    m_use_nagling = value == 0;
  } else if (opt == OPT_ACKDELAY) {
    m_ack_delay = value;
  } else if (opt == OPT_SNDBUF) {
    ASSERT(m_state == TCP_LISTEN);
    resizeSendBuffer(value);
  } else if (opt == OPT_RCVBUF) {
    ASSERT(m_state == TCP_LISTEN);
    resizeReceiveBuffer(value);
  } else {
    ASSERT(false);
  }
}

uint32 PseudoTcp::GetCongestionWindow() const {
  return m_cwnd;
}

uint32 PseudoTcp::GetBytesInFlight() const {
  return m_snd_nxt - m_snd_una;
}

uint32 PseudoTcp::GetBytesBufferedNotSent() const {
  size_t buffered_bytes = 0;
  m_sbuf.GetBuffered(&buffered_bytes);
  return static_cast<uint32>(m_snd_una + buffered_bytes - m_snd_nxt);
}

uint32 PseudoTcp::GetRoundTripTimeEstimateMs() const {
  return m_rx_srtt;
}

//
// IPStream Implementation
//

int PseudoTcp::Recv(char* buffer, size_t len) {
  if (m_state != TCP_ESTABLISHED) {
    m_error = ENOTCONN;
    return SOCKET_ERROR;
  }

  size_t read = 0;
  talk_base::StreamResult result = m_rbuf.Read(buffer, len, &read, NULL);

  // If there's no data in |m_rbuf|.
  if (result == talk_base::SR_BLOCK) {
    m_bReadEnable = true;
    m_error = EWOULDBLOCK;
    return SOCKET_ERROR;
  }
  ASSERT(result == talk_base::SR_SUCCESS);

  size_t available_space = 0;
  m_rbuf.GetWriteRemaining(&available_space);

  if (uint32(available_space) - m_rcv_wnd >=
      talk_base::_min<uint32>(m_rbuf_len / 2, m_mss)) {
    // TODO(jbeda): !?! Not sure about this was closed business
    bool bWasClosed = (m_rcv_wnd == 0);
    m_rcv_wnd = static_cast<uint32>(available_space);

    if (bWasClosed) {
      attemptSend(sfImmediateAck);
    }
  }

  return static_cast<int>(read);
}

int PseudoTcp::Send(const char* buffer, size_t len) {
  if (m_state != TCP_ESTABLISHED) {
    m_error = ENOTCONN;
    return SOCKET_ERROR;
  }

  size_t available_space = 0;
  m_sbuf.GetWriteRemaining(&available_space);

  if (!available_space) {
    m_bWriteEnable = true;
    m_error = EWOULDBLOCK;
    return SOCKET_ERROR;
  }

  int written = queue(buffer, uint32(len), false);
  attemptSend();
  return written;
}

void PseudoTcp::Close(bool force) {
  LOG_F(LS_VERBOSE) << "(" << (force ? "true" : "false") << ")";
  m_shutdown = force ? SD_FORCEFUL : SD_GRACEFUL;
}

int PseudoTcp::GetError() {
  return m_error;
}

//
// Internal Implementation
//

uint32 PseudoTcp::queue(const char* data, uint32 len, bool bCtrl) {
  size_t available_space = 0;
  m_sbuf.GetWriteRemaining(&available_space);

  if (len > static_cast<uint32>(available_space)) {
    ASSERT(!bCtrl);
    len = static_cast<uint32>(available_space);
  }

  // We can concatenate data if the last segment is the same type
  // (control v. regular data), and has not been transmitted yet
  if (!m_slist.empty() && (m_slist.back().bCtrl == bCtrl) &&
      (m_slist.back().xmit == 0)) {
    m_slist.back().len += len;
  } else {
    size_t snd_buffered = 0;
    m_sbuf.GetBuffered(&snd_buffered);
    SSegment sseg(static_cast<uint32>(m_snd_una + snd_buffered), len, bCtrl);
    m_slist.push_back(sseg);
  }

  size_t written = 0;
  m_sbuf.Write(data, len, &written, NULL);
  return static_cast<uint32>(written);
}

IPseudoTcpNotify::WriteResult PseudoTcp::packet(uint32 seq, uint8 flags,
                                                uint32 offset, uint32 len) {
  ASSERT(HEADER_SIZE + len <= MAX_PACKET);

  uint32 now = Now();

  uint8 buffer[MAX_PACKET];
  long_to_bytes(m_conv, buffer);
  long_to_bytes(seq, buffer + 4);
  long_to_bytes(m_rcv_nxt, buffer + 8);
  buffer[12] = 0;
  buffer[13] = flags;
  short_to_bytes(static_cast<uint16>(m_rcv_wnd >> m_rwnd_scale), buffer + 14);

  // Timestamp computations
  long_to_bytes(now, buffer + 16);
  long_to_bytes(m_ts_recent, buffer + 20);
  m_ts_lastack = m_rcv_nxt;

  if (len) {
    size_t bytes_read = 0;
    talk_base::StreamResult result = m_sbuf.ReadOffset(buffer + HEADER_SIZE,
                                                       len,
                                                       offset,
                                                       &bytes_read);
    UNUSED(result);
    ASSERT(result == talk_base::SR_SUCCESS);
    ASSERT(static_cast<uint32>(bytes_read) == len);
  }

#if _DEBUGMSG >= _DBG_VERBOSE
  LOG(LS_INFO) << "<-- <CONV=" << m_conv
               << "><FLG=" << static_cast<unsigned>(flags)
               << "><SEQ=" << seq << ":" << seq + len
               << "><ACK=" << m_rcv_nxt
               << "><WND=" << m_rcv_wnd
               << "><TS="  << (now % 10000)
               << "><TSR=" << (m_ts_recent % 10000)
               << "><LEN=" << len << ">";
#endif // _DEBUGMSG

  IPseudoTcpNotify::WriteResult wres = m_notify->TcpWritePacket(this, reinterpret_cast<char *>(buffer), len + HEADER_SIZE);
  // Note: When len is 0, this is an ACK packet.  We don't read the return value for those,
  // and thus we won't retry.  So go ahead and treat the packet as a success (basically simulate
  // as if it were dropped), which will prevent our timers from being messed up.
  if ((wres != IPseudoTcpNotify::WR_SUCCESS) && (0 != len))
    return wres;

  m_t_ack = 0;
  if (len > 0) {
    m_lastsend = now;
  }
  m_lasttraffic = now;
  m_bOutgoing = true;

  return IPseudoTcpNotify::WR_SUCCESS;
}

bool PseudoTcp::parse(const uint8* buffer, uint32 size) {
  if (size < 12)
    return false;

  Segment seg;
  seg.conv = bytes_to_long(buffer);
  seg.seq = bytes_to_long(buffer + 4);
  seg.ack = bytes_to_long(buffer + 8);
  seg.flags = buffer[13];
  seg.wnd = bytes_to_short(buffer + 14);

  seg.tsval = bytes_to_long(buffer + 16);
  seg.tsecr = bytes_to_long(buffer + 20);

  seg.data = reinterpret_cast<const char *>(buffer) + HEADER_SIZE;
  seg.len = size - HEADER_SIZE;

#if _DEBUGMSG >= _DBG_VERBOSE
  LOG(LS_INFO) << "--> <CONV=" << seg.conv
               << "><FLG=" << static_cast<unsigned>(seg.flags)
               << "><SEQ=" << seg.seq << ":" << seg.seq + seg.len
               << "><ACK=" << seg.ack
               << "><WND=" << seg.wnd
               << "><TS="  << (seg.tsval % 10000)
               << "><TSR=" << (seg.tsecr % 10000)
               << "><LEN=" << seg.len << ">";
#endif // _DEBUGMSG

  return process(seg);
}

bool PseudoTcp::clock_check(uint32 now, long& nTimeout) {
  if (m_shutdown == SD_FORCEFUL)
    return false;

  size_t snd_buffered = 0;
  m_sbuf.GetBuffered(&snd_buffered);
  if ((m_shutdown == SD_GRACEFUL)
      && ((m_state != TCP_ESTABLISHED)
          || ((snd_buffered == 0) && (m_t_ack == 0)))) {
    return false;
  }

  if (m_state == TCP_CLOSED) {
    nTimeout = CLOSED_TIMEOUT;
    return true;
  }

  nTimeout = DEFAULT_TIMEOUT;

  if (m_t_ack) {
    nTimeout = talk_base::_min<int32>(nTimeout,
      talk_base::TimeDiff(m_t_ack + m_ack_delay, now));
  }
  if (m_rto_base) {
    nTimeout = talk_base::_min<int32>(nTimeout,
      talk_base::TimeDiff(m_rto_base + m_rx_rto, now));
  }
  if (m_snd_wnd == 0) {
    nTimeout = talk_base::_min<int32>(nTimeout, talk_base::TimeDiff(m_lastsend + m_rx_rto, now));
  }
#if PSEUDO_KEEPALIVE
  if (m_state == TCP_ESTABLISHED) {
    nTimeout = talk_base::_min<int32>(nTimeout,
      talk_base::TimeDiff(m_lasttraffic + (m_bOutgoing ? IDLE_PING * 3/2 : IDLE_PING), now));
  }
#endif // PSEUDO_KEEPALIVE
  return true;
}

bool PseudoTcp::process(Segment& seg) {
  // If this is the wrong conversation, send a reset!?! (with the correct conversation?)
  if (seg.conv != m_conv) {
    //if ((seg.flags & FLAG_RST) == 0) {
    //  packet(tcb, seg.ack, 0, FLAG_RST, 0, 0);
    //}
    LOG_F(LS_ERROR) << "wrong conversation";
    return false;
  }

  uint32 now = Now();
  m_lasttraffic = m_lastrecv = now;
  m_bOutgoing = false;

  if (m_state == TCP_CLOSED) {
    // !?! send reset?
    LOG_F(LS_ERROR) << "closed";
    return false;
  }

  // Check if this is a reset segment
  if (seg.flags & FLAG_RST) {
    closedown(ECONNRESET);
    return false;
  }

  // Check for control data
  bool bConnect = false;
  if (seg.flags & FLAG_CTL) {
    if (seg.len == 0) {
      LOG_F(LS_ERROR) << "Missing control code";
      return false;
    } else if (seg.data[0] == CTL_CONNECT) {
      bConnect = true;

      // TCP options are in the remainder of the payload after CTL_CONNECT.
      parseOptions(&seg.data[1], seg.len - 1);

      if (m_state == TCP_LISTEN) {
        m_state = TCP_SYN_RECEIVED;
        LOG(LS_INFO) << "State: TCP_SYN_RECEIVED";
        //m_notify->associate(addr);
        queueConnectMessage();
      } else if (m_state == TCP_SYN_SENT) {
        m_state = TCP_ESTABLISHED;
        LOG(LS_INFO) << "State: TCP_ESTABLISHED";
        adjustMTU();
        if (m_notify) {
          m_notify->OnTcpOpen(this);
        }
        //notify(evOpen);
      }
    } else {
      LOG_F(LS_WARNING) << "Unknown control code: " << seg.data[0];
      return false;
    }
  }

  // Update timestamp
  if ((seg.seq <= m_ts_lastack) && (m_ts_lastack < seg.seq + seg.len)) {
    m_ts_recent = seg.tsval;
  }

  // Check if this is a valuable ack
  if ((seg.ack > m_snd_una) && (seg.ack <= m_snd_nxt)) {
    // Calculate round-trip time
    if (seg.tsecr) {
      long rtt = talk_base::TimeDiff(now, seg.tsecr);
      if (rtt >= 0) {
        if (m_rx_srtt == 0) {
          m_rx_srtt = rtt;
          m_rx_rttvar = rtt / 2;
        } else {
          m_rx_rttvar = (3 * m_rx_rttvar + abs(long(rtt - m_rx_srtt))) / 4;
          m_rx_srtt = (7 * m_rx_srtt + rtt) / 8;
        }
        m_rx_rto = bound(MIN_RTO, m_rx_srtt +
            talk_base::_max<uint32>(1, 4 * m_rx_rttvar), MAX_RTO);
#if _DEBUGMSG >= _DBG_VERBOSE
        LOG(LS_INFO) << "rtt: " << rtt
                     << "  srtt: " << m_rx_srtt
                     << "  rto: " << m_rx_rto;
#endif // _DEBUGMSG
      } else {
        ASSERT(false);
      }
    }

    m_snd_wnd = static_cast<uint32>(seg.wnd) << m_swnd_scale;

    uint32 nAcked = seg.ack - m_snd_una;
    m_snd_una = seg.ack;

    m_rto_base = (m_snd_una == m_snd_nxt) ? 0 : now;

    m_sbuf.ConsumeReadData(nAcked);

    for (uint32 nFree = nAcked; nFree > 0; ) {
      ASSERT(!m_slist.empty());
      if (nFree < m_slist.front().len) {
        m_slist.front().len -= nFree;
        nFree = 0;
      } else {
        if (m_slist.front().len > m_largest) {
          m_largest = m_slist.front().len;
        }
        nFree -= m_slist.front().len;
        m_slist.pop_front();
      }
    }

    if (m_dup_acks >= 3) {
      if (m_snd_una >= m_recover) { // NewReno
        uint32 nInFlight = m_snd_nxt - m_snd_una;
        m_cwnd = talk_base::_min(m_ssthresh, nInFlight + m_mss); // (Fast Retransmit)
#if _DEBUGMSG >= _DBG_NORMAL
        LOG(LS_INFO) << "exit recovery";
#endif // _DEBUGMSG
        m_dup_acks = 0;
      } else {
#if _DEBUGMSG >= _DBG_NORMAL
        LOG(LS_INFO) << "recovery retransmit";
#endif // _DEBUGMSG
        if (!transmit(m_slist.begin(), now)) {
          closedown(ECONNABORTED);
          return false;
        }
        m_cwnd += m_mss - talk_base::_min(nAcked, m_cwnd);
      }
    } else {
      m_dup_acks = 0;
      // Slow start, congestion avoidance
      if (m_cwnd < m_ssthresh) {
        m_cwnd += m_mss;
      } else {
        m_cwnd += talk_base::_max<uint32>(1, m_mss * m_mss / m_cwnd);
      }
    }
  } else if (seg.ack == m_snd_una) {
    // !?! Note, tcp says don't do this... but otherwise how does a closed window become open?
    m_snd_wnd = static_cast<uint32>(seg.wnd) << m_swnd_scale;

    // Check duplicate acks
    if (seg.len > 0) {
      // it's a dup ack, but with a data payload, so don't modify m_dup_acks
    } else if (m_snd_una != m_snd_nxt) {
      m_dup_acks += 1;
      if (m_dup_acks == 3) { // (Fast Retransmit)
#if _DEBUGMSG >= _DBG_NORMAL
        LOG(LS_INFO) << "enter recovery";
        LOG(LS_INFO) << "recovery retransmit";
#endif // _DEBUGMSG
        if (!transmit(m_slist.begin(), now)) {
          closedown(ECONNABORTED);
          return false;
        }
        m_recover = m_snd_nxt;
        uint32 nInFlight = m_snd_nxt - m_snd_una;
        m_ssthresh = talk_base::_max(nInFlight / 2, 2 * m_mss);
        //LOG(LS_INFO) << "m_ssthresh: " << m_ssthresh << "  nInFlight: " << nInFlight << "  m_mss: " << m_mss;
        m_cwnd = m_ssthresh + 3 * m_mss;
      } else if (m_dup_acks > 3) {
        m_cwnd += m_mss;
      }
    } else {
      m_dup_acks = 0;
    }
  }

  // !?! A bit hacky
  if ((m_state == TCP_SYN_RECEIVED) && !bConnect) {
    m_state = TCP_ESTABLISHED;
    LOG(LS_INFO) << "State: TCP_ESTABLISHED";
    adjustMTU();
    if (m_notify) {
      m_notify->OnTcpOpen(this);
    }
    //notify(evOpen);
  }

  // If we make room in the send queue, notify the user
  // The goal it to make sure we always have at least enough data to fill the
  // window.  We'd like to notify the app when we are halfway to that point.
  const uint32 kIdealRefillSize = (m_sbuf_len + m_rbuf_len) / 2;
  size_t snd_buffered = 0;
  m_sbuf.GetBuffered(&snd_buffered);
  if (m_bWriteEnable && static_cast<uint32>(snd_buffered) < kIdealRefillSize) {
    m_bWriteEnable = false;
    if (m_notify) {
      m_notify->OnTcpWriteable(this);
    }
    //notify(evWrite);
  }

  // Conditions were acks must be sent:
  // 1) Segment is too old (they missed an ACK) (immediately)
  // 2) Segment is too new (we missed a segment) (immediately)
  // 3) Segment has data (so we need to ACK!) (delayed)
  // ... so the only time we don't need to ACK, is an empty segment that points to rcv_nxt!

  SendFlags sflags = sfNone;
  if (seg.seq != m_rcv_nxt) {
    sflags = sfImmediateAck; // (Fast Recovery)
  } else if (seg.len != 0) {
    if (m_ack_delay == 0) {
      sflags = sfImmediateAck;
    } else {
      sflags = sfDelayedAck;
    }
  }
#if _DEBUGMSG >= _DBG_NORMAL
  if (sflags == sfImmediateAck) {
    if (seg.seq > m_rcv_nxt) {
      LOG_F(LS_INFO) << "too new";
    } else if (seg.seq + seg.len <= m_rcv_nxt) {
      LOG_F(LS_INFO) << "too old";
    }
  }
#endif // _DEBUGMSG

  // Adjust the incoming segment to fit our receive buffer
  if (seg.seq < m_rcv_nxt) {
    uint32 nAdjust = m_rcv_nxt - seg.seq;
    if (nAdjust < seg.len) {
      seg.seq += nAdjust;
      seg.data += nAdjust;
      seg.len -= nAdjust;
    } else {
      seg.len = 0;
    }
  }

  size_t available_space = 0;
  m_rbuf.GetWriteRemaining(&available_space);

  if ((seg.seq + seg.len - m_rcv_nxt) > static_cast<uint32>(available_space)) {
    uint32 nAdjust = seg.seq + seg.len - m_rcv_nxt - static_cast<uint32>(available_space);
    if (nAdjust < seg.len) {
      seg.len -= nAdjust;
    } else {
      seg.len = 0;
    }
  }

  bool bIgnoreData = (seg.flags & FLAG_CTL) || (m_shutdown != SD_NONE);
  bool bNewData = false;

  if (seg.len > 0) {
    if (bIgnoreData) {
      if (seg.seq == m_rcv_nxt) {
        m_rcv_nxt += seg.len;
      }
    } else {
      uint32 nOffset = seg.seq - m_rcv_nxt;

      talk_base::StreamResult result = m_rbuf.WriteOffset(seg.data, seg.len,
                                                          nOffset, NULL);
      ASSERT(result == talk_base::SR_SUCCESS);
      UNUSED(result);

      if (seg.seq == m_rcv_nxt) {
        m_rbuf.ConsumeWriteBuffer(seg.len);
        m_rcv_nxt += seg.len;
        m_rcv_wnd -= seg.len;
        bNewData = true;

        RList::iterator it = m_rlist.begin();
        while ((it != m_rlist.end()) && (it->seq <= m_rcv_nxt)) {
          if (it->seq + it->len > m_rcv_nxt) {
            sflags = sfImmediateAck; // (Fast Recovery)
            uint32 nAdjust = (it->seq + it->len) - m_rcv_nxt;
#if _DEBUGMSG >= _DBG_NORMAL
            LOG(LS_INFO) << "Recovered " << nAdjust << " bytes (" << m_rcv_nxt << " -> " << m_rcv_nxt + nAdjust << ")";
#endif // _DEBUGMSG
            m_rbuf.ConsumeWriteBuffer(nAdjust);
            m_rcv_nxt += nAdjust;
            m_rcv_wnd -= nAdjust;
          }
          it = m_rlist.erase(it);
        }
      } else {
#if _DEBUGMSG >= _DBG_NORMAL
        LOG(LS_INFO) << "Saving " << seg.len << " bytes (" << seg.seq << " -> " << seg.seq + seg.len << ")";
#endif // _DEBUGMSG
        RSegment rseg;
        rseg.seq = seg.seq;
        rseg.len = seg.len;
        RList::iterator it = m_rlist.begin();
        while ((it != m_rlist.end()) && (it->seq < rseg.seq)) {
          ++it;
        }
        m_rlist.insert(it, rseg);
      }
    }
  }

  attemptSend(sflags);

  // If we have new data, notify the user
  if (bNewData && m_bReadEnable) {
    m_bReadEnable = false;
    if (m_notify) {
      m_notify->OnTcpReadable(this);
    }
    //notify(evRead);
  }

  return true;
}

bool PseudoTcp::transmit(const SList::iterator& seg, uint32 now) {
  if (seg->xmit >= ((m_state == TCP_ESTABLISHED) ? 15 : 30)) {
    LOG_F(LS_VERBOSE) << "too many retransmits";
    return false;
  }

  uint32 nTransmit = talk_base::_min(seg->len, m_mss);

  while (true) {
    uint32 seq = seg->seq;
    uint8 flags = (seg->bCtrl ? FLAG_CTL : 0);
    IPseudoTcpNotify::WriteResult wres = packet(seq,
                                                flags,
                                                seg->seq - m_snd_una,
                                                nTransmit);

    if (wres == IPseudoTcpNotify::WR_SUCCESS)
      break;

    if (wres == IPseudoTcpNotify::WR_FAIL) {
      LOG_F(LS_VERBOSE) << "packet failed";
      return false;
    }

    ASSERT(wres == IPseudoTcpNotify::WR_TOO_LARGE);

    while (true) {
      if (PACKET_MAXIMUMS[m_msslevel + 1] == 0) {
        LOG_F(LS_VERBOSE) << "MTU too small";
        return false;
      }
      // !?! We need to break up all outstanding and pending packets and then retransmit!?!

      m_mss = PACKET_MAXIMUMS[++m_msslevel] - PACKET_OVERHEAD;
      m_cwnd = 2 * m_mss; // I added this... haven't researched actual formula
      if (m_mss < nTransmit) {
        nTransmit = m_mss;
        break;
      }
    }
#if _DEBUGMSG >= _DBG_NORMAL
    LOG(LS_INFO) << "Adjusting mss to " << m_mss << " bytes";
#endif // _DEBUGMSG
  }

  if (nTransmit < seg->len) {
    LOG_F(LS_VERBOSE) << "mss reduced to " << m_mss;

    SSegment subseg(seg->seq + nTransmit, seg->len - nTransmit, seg->bCtrl);
    //subseg.tstamp = seg->tstamp;
    subseg.xmit = seg->xmit;
    seg->len = nTransmit;

    SList::iterator next = seg;
    m_slist.insert(++next, subseg);
  }

  if (seg->xmit == 0) {
    m_snd_nxt += seg->len;
  }
  seg->xmit += 1;
  //seg->tstamp = now;
  if (m_rto_base == 0) {
    m_rto_base = now;
  }

  return true;
}

void PseudoTcp::attemptSend(SendFlags sflags) {
  uint32 now = Now();

  if (talk_base::TimeDiff(now, m_lastsend) > static_cast<long>(m_rx_rto)) {
    m_cwnd = m_mss;
  }

#if _DEBUGMSG
  bool bFirst = true;
  UNUSED(bFirst);
#endif // _DEBUGMSG

  while (true) {
    uint32 cwnd = m_cwnd;
    if ((m_dup_acks == 1) || (m_dup_acks == 2)) { // Limited Transmit
      cwnd += m_dup_acks * m_mss;
    }
    uint32 nWindow = talk_base::_min(m_snd_wnd, cwnd);
    uint32 nInFlight = m_snd_nxt - m_snd_una;
    uint32 nUseable = (nInFlight < nWindow) ? (nWindow - nInFlight) : 0;

    size_t snd_buffered = 0;
    m_sbuf.GetBuffered(&snd_buffered);
    uint32 nAvailable =
        talk_base::_min(static_cast<uint32>(snd_buffered) - nInFlight, m_mss);

    if (nAvailable > nUseable) {
      if (nUseable * 4 < nWindow) {
        // RFC 813 - avoid SWS
        nAvailable = 0;
      } else {
        nAvailable = nUseable;
      }
    }

#if _DEBUGMSG >= _DBG_VERBOSE
    if (bFirst) {
      size_t available_space = 0;
      m_sbuf.GetWriteRemaining(&available_space);

      bFirst = false;
      LOG(LS_INFO) << "[cwnd: " << m_cwnd
                   << "  nWindow: " << nWindow
                   << "  nInFlight: " << nInFlight
                   << "  nAvailable: " << nAvailable
                   << "  nQueued: " << snd_buffered
                   << "  nEmpty: " << available_space
                   << "  ssthresh: " << m_ssthresh << "]";
    }
#endif // _DEBUGMSG

    if (nAvailable == 0) {
      if (sflags == sfNone)
        return;

      // If this is an immediate ack, or the second delayed ack
      if ((sflags == sfImmediateAck) || m_t_ack) {
        packet(m_snd_nxt, 0, 0, 0);
      } else {
        m_t_ack = Now();
      }
      return;
    }

    // Nagle's algorithm.
    // If there is data already in-flight, and we haven't a full segment of
    // data ready to send then hold off until we get more to send, or the
    // in-flight data is acknowledged.
    if (m_use_nagling && (m_snd_nxt > m_snd_una) && (nAvailable < m_mss))  {
      return;
    }

    // Find the next segment to transmit
    SList::iterator it = m_slist.begin();
    while (it->xmit > 0) {
      ++it;
      ASSERT(it != m_slist.end());
    }
    SList::iterator seg = it;

    // If the segment is too large, break it into two
    if (seg->len > nAvailable) {
      SSegment subseg(seg->seq + nAvailable, seg->len - nAvailable, seg->bCtrl);
      seg->len = nAvailable;
      m_slist.insert(++it, subseg);
    }

    if (!transmit(seg, now)) {
      LOG_F(LS_VERBOSE) << "transmit failed";
      // TODO: consider closing socket
      return;
    }

    sflags = sfNone;
  }
}

void
PseudoTcp::closedown(uint32 err) {
  LOG(LS_INFO) << "State: TCP_CLOSED";
  m_state = TCP_CLOSED;
  if (m_notify) {
    m_notify->OnTcpClosed(this, err);
  }
  //notify(evClose, err);
}

void
PseudoTcp::adjustMTU() {
  // Determine our current mss level, so that we can adjust appropriately later
  for (m_msslevel = 0; PACKET_MAXIMUMS[m_msslevel + 1] > 0; ++m_msslevel) {
    if (static_cast<uint16>(PACKET_MAXIMUMS[m_msslevel]) <= m_mtu_advise) {
      break;
    }
  }
  m_mss = m_mtu_advise - PACKET_OVERHEAD;
  // !?! Should we reset m_largest here?
#if _DEBUGMSG >= _DBG_NORMAL
  LOG(LS_INFO) << "Adjusting mss to " << m_mss << " bytes";
#endif // _DEBUGMSG
  // Enforce minimums on ssthresh and cwnd
  m_ssthresh = talk_base::_max(m_ssthresh, 2 * m_mss);
  m_cwnd = talk_base::_max(m_cwnd, m_mss);
}

bool
PseudoTcp::isReceiveBufferFull() const {
  size_t available_space = 0;
  m_rbuf.GetWriteRemaining(&available_space);
  return !available_space;
}

void
PseudoTcp::disableWindowScale() {
  m_support_wnd_scale = false;
}

void
PseudoTcp::queueConnectMessage() {
  talk_base::ByteBuffer buf(talk_base::ByteBuffer::ORDER_NETWORK);

  buf.WriteUInt8(CTL_CONNECT);
  if (m_support_wnd_scale) {
    buf.WriteUInt8(TCP_OPT_WND_SCALE);
    buf.WriteUInt8(1);
    buf.WriteUInt8(m_rwnd_scale);
  }
  m_snd_wnd = static_cast<uint32>(buf.Length());
  queue(buf.Data(), static_cast<uint32>(buf.Length()), true);
}

void
PseudoTcp::parseOptions(const char* data, uint32 len) {
  std::set<uint8> options_specified;

  // See http://www.freesoft.org/CIE/Course/Section4/8.htm for
  // parsing the options list.
  talk_base::ByteBuffer buf(data, len);
  while (buf.Length()) {
    uint8 kind = TCP_OPT_EOL;
    buf.ReadUInt8(&kind);

    if (kind == TCP_OPT_EOL) {
      // End of option list.
      break;
    } else if (kind == TCP_OPT_NOOP) {
      // No op.
      continue;
    }

    // Length of this option.
    ASSERT(len != 0);
    UNUSED(len);
    uint8 opt_len = 0;
    buf.ReadUInt8(&opt_len);

    // Content of this option.
    if (opt_len <= buf.Length()) {
      applyOption(kind, buf.Data(), opt_len);
      buf.Consume(opt_len);
    } else {
      LOG(LS_ERROR) << "Invalid option length received.";
      return;
    }
    options_specified.insert(kind);
  }

  if (options_specified.find(TCP_OPT_WND_SCALE) == options_specified.end()) {
    LOG(LS_WARNING) << "Peer doesn't support window scaling";

    if (m_rwnd_scale > 0) {
      // Peer doesn't support TCP options and window scaling.
      // Revert receive buffer size to default value.
      resizeReceiveBuffer(DEFAULT_RCV_BUF_SIZE);
      m_swnd_scale = 0;
    }
  }
}

void
PseudoTcp::applyOption(char kind, const char* data, uint32 len) {
  if (kind == TCP_OPT_MSS) {
    LOG(LS_WARNING) << "Peer specified MSS option which is not supported.";
    // TODO: Implement.
  } else if (kind == TCP_OPT_WND_SCALE) {
    // Window scale factor.
    // http://www.ietf.org/rfc/rfc1323.txt
    if (len != 1) {
      LOG_F(WARNING) << "Invalid window scale option received.";
      return;
    }
    applyWindowScaleOption(data[0]);
  }
}

void
PseudoTcp::applyWindowScaleOption(uint8 scale_factor) {
  m_swnd_scale = scale_factor;
}

void
PseudoTcp::resizeSendBuffer(uint32 new_size) {
  m_sbuf_len = new_size;
  m_sbuf.SetCapacity(new_size);
}

void
PseudoTcp::resizeReceiveBuffer(uint32 new_size) {
  uint8 scale_factor = 0;

  // Determine the scale factor such that the scaled window size can fit
  // in a 16-bit unsigned integer.
  while (new_size > 0xFFFF) {
    ++scale_factor;
    new_size >>= 1;
  }

  // Determine the proper size of the buffer.
  new_size <<= scale_factor;
  bool result = m_rbuf.SetCapacity(new_size);

  // Make sure the new buffer is large enough to contain data in the old
  // buffer. This should always be true because this method is called either
  // before connection is established or when peers are exchanging connect
  // messages.
  ASSERT(result);
  UNUSED(result);
  m_rbuf_len = new_size;
  m_rwnd_scale = scale_factor;
  m_ssthresh = new_size;

  size_t available_space = 0;
  m_rbuf.GetWriteRemaining(&available_space);
  m_rcv_wnd = static_cast<uint32>(available_space);
}

}  // namespace cricket
