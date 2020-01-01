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

#ifndef TALK_BASE_SOCKETADAPTERS_H_
#define TALK_BASE_SOCKETADAPTERS_H_

#include <map>
#include <string>

#include "base/asyncsocket.h"
#include "base/cryptstring.h"
#include "base/logging.h"

namespace talk_base {

struct HttpAuthContext;
class ByteBuffer;

///////////////////////////////////////////////////////////////////////////////

// Implements a socket adapter that can buffer and process data internally,
// as in the case of connecting to a proxy, where you must speak the proxy
// protocol before commencing normal socket behavior.
class BufferedReadAdapter : public AsyncSocketAdapter {
 public:
  BufferedReadAdapter(AsyncSocket* socket, size_t buffer_size);
  virtual ~BufferedReadAdapter();

  virtual int Send(const void* pv, size_t cb);
  virtual int Recv(void* pv, size_t cb);

 protected:
  int DirectSend(const void* pv, size_t cb) {
    return AsyncSocketAdapter::Send(pv, cb);
  }

  void BufferInput(bool on = true);
  virtual void ProcessInput(char* data, size_t* len) = 0;

  virtual void OnReadEvent(AsyncSocket * socket);

 private:
  char * buffer_;
  size_t buffer_size_, data_len_;
  bool buffering_;
  DISALLOW_EVIL_CONSTRUCTORS(BufferedReadAdapter);
};

///////////////////////////////////////////////////////////////////////////////

// Interface for implementing proxy server sockets.
class AsyncProxyServerSocket : public BufferedReadAdapter {
 public:
  AsyncProxyServerSocket(AsyncSocket* socket, size_t buffer_size)
      : BufferedReadAdapter(socket, buffer_size) {}
  sigslot::signal2<AsyncProxyServerSocket*,
                   const SocketAddress&>  SignalConnectRequest;
  virtual void SendConnectResult(int err, const SocketAddress& addr) = 0;
};

///////////////////////////////////////////////////////////////////////////////

// Implements a socket adapter that performs the client side of a
// fake SSL handshake. Used for "ssltcp" P2P functionality.
class AsyncSSLSocket : public BufferedReadAdapter {
 public:
  explicit AsyncSSLSocket(AsyncSocket* socket);

  virtual int Connect(const SocketAddress& addr);

 protected:
  virtual void OnConnectEvent(AsyncSocket* socket);
  virtual void ProcessInput(char* data, size_t* len);
  DISALLOW_EVIL_CONSTRUCTORS(AsyncSSLSocket);
};

// Implements a socket adapter that performs the server side of a
// fake SSL handshake. Used when implementing a relay server that does "ssltcp".
class AsyncSSLServerSocket : public BufferedReadAdapter {
 public:
  explicit AsyncSSLServerSocket(AsyncSocket* socket);

 protected:
  virtual void ProcessInput(char* data, size_t* len);
  DISALLOW_EVIL_CONSTRUCTORS(AsyncSSLServerSocket);
};

///////////////////////////////////////////////////////////////////////////////

// Implements a socket adapter that speaks the HTTP/S proxy protocol.
class AsyncHttpsProxySocket : public BufferedReadAdapter {
 public:
  AsyncHttpsProxySocket(AsyncSocket* socket, const std::string& user_agent,
    const SocketAddress& proxy,
    const std::string& username, const CryptString& password);
  virtual ~AsyncHttpsProxySocket();

  // If connect is forced, the adapter will always issue an HTTP CONNECT to the
  // target address.  Otherwise, it will connect only if the destination port
  // is not port 80.
  void SetForceConnect(bool force) { force_connect_ = force; }

  virtual int Connect(const SocketAddress& addr);
  virtual SocketAddress GetRemoteAddress() const;
  virtual int Close();
  virtual ConnState GetState() const;

 protected:
  virtual void OnConnectEvent(AsyncSocket* socket);
  virtual void OnCloseEvent(AsyncSocket* socket, int err);
  virtual void ProcessInput(char* data, size_t* len);

  bool ShouldIssueConnect() const;
  void SendRequest();
  void ProcessLine(char* data, size_t len);
  void EndResponse();
  void Error(int error);

 private:
  SocketAddress proxy_, dest_;
  std::string agent_, user_, headers_;
  CryptString pass_;
  bool force_connect_;
  size_t content_length_;
  int defer_error_;
  bool expect_close_;
  enum ProxyState {
    PS_INIT, PS_LEADER, PS_AUTHENTICATE, PS_SKIP_HEADERS, PS_ERROR_HEADERS,
    PS_TUNNEL_HEADERS, PS_SKIP_BODY, PS_TUNNEL, PS_WAIT_CLOSE, PS_ERROR
  } state_;
  HttpAuthContext * context_;
  std::string unknown_mechanisms_;
  DISALLOW_EVIL_CONSTRUCTORS(AsyncHttpsProxySocket);
};

/* TODO: Implement this.
class AsyncHttpsProxyServerSocket : public AsyncProxyServerSocket {
 public:
  explicit AsyncHttpsProxyServerSocket(AsyncSocket* socket);

 private:
  virtual void ProcessInput(char * data, size_t& len);
  void Error(int error);
  DISALLOW_EVIL_CONSTRUCTORS(AsyncHttpsProxyServerSocket);
};
*/

///////////////////////////////////////////////////////////////////////////////

// Implements a socket adapter that speaks the SOCKS proxy protocol.
class AsyncSocksProxySocket : public BufferedReadAdapter {
 public:
  AsyncSocksProxySocket(AsyncSocket* socket, const SocketAddress& proxy,
    const std::string& username, const CryptString& password);

  virtual int Connect(const SocketAddress& addr);
  virtual SocketAddress GetRemoteAddress() const;
  virtual int Close();
  virtual ConnState GetState() const;

 protected:
  virtual void OnConnectEvent(AsyncSocket* socket);
  virtual void ProcessInput(char* data, size_t* len);

  void SendHello();
  void SendConnect();
  void SendAuth();
  void Error(int error);

 private:
  enum State {
    SS_INIT, SS_HELLO, SS_AUTH, SS_CONNECT, SS_TUNNEL, SS_ERROR
  };
  State state_;
  SocketAddress proxy_, dest_;
  std::string user_;
  CryptString pass_;
  DISALLOW_EVIL_CONSTRUCTORS(AsyncSocksProxySocket);
};

// Implements a proxy server socket for the SOCKS protocol.
class AsyncSocksProxyServerSocket : public AsyncProxyServerSocket {
 public:
  explicit AsyncSocksProxyServerSocket(AsyncSocket* socket);

 private:
  virtual void ProcessInput(char* data, size_t* len);
  void DirectSend(const ByteBuffer& buf);

  void HandleHello(ByteBuffer* request);
  void SendHelloReply(int method);
  void HandleAuth(ByteBuffer* request);
  void SendAuthReply(int result);
  void HandleConnect(ByteBuffer* request);
  virtual void SendConnectResult(int result, const SocketAddress& addr);

  void Error(int error);

  static const int kBufferSize = 1024;
  enum State {
    SS_HELLO, SS_AUTH, SS_CONNECT, SS_CONNECT_PENDING, SS_TUNNEL, SS_ERROR
  };
  State state_;
  DISALLOW_EVIL_CONSTRUCTORS(AsyncSocksProxyServerSocket);
};

///////////////////////////////////////////////////////////////////////////////

// Implements a socket adapter that logs everything that it sends and receives.
class LoggingSocketAdapter : public AsyncSocketAdapter {
 public:
  LoggingSocketAdapter(AsyncSocket* socket, LoggingSeverity level,
                 const char * label, bool hex_mode = false);

  virtual int Send(const void *pv, size_t cb);
  virtual int SendTo(const void *pv, size_t cb, const SocketAddress& addr);
  virtual int Recv(void *pv, size_t cb);
  virtual int RecvFrom(void *pv, size_t cb, SocketAddress *paddr);
  virtual int Close();

 protected:
  virtual void OnConnectEvent(AsyncSocket * socket);
  virtual void OnCloseEvent(AsyncSocket * socket, int err);

 private:
  LoggingSeverity level_;
  std::string label_;
  bool hex_mode_;
  LogMultilineState lms_;
  DISALLOW_EVIL_CONSTRUCTORS(LoggingSocketAdapter);
};

///////////////////////////////////////////////////////////////////////////////

}  // namespace talk_base

#endif  // TALK_BASE_SOCKETADAPTERS_H_
