#pragma once

#include "net_common.h"
#include "net_connection.h"
#include "net_message.h"
#include "net_tsqueue.h"

namespace olc {
namespace net {
template <typename T>
class server_interface {
 public:
  server_interface(uint16_t port)
      : m_asioAcceptor(m_asioContext,
                       asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)) {}

  virtual ~server_interface() { Stop(); }

  bool Start() {
    try {
      WaitForClientConnection();

      m_threadContext = std::thread([this]() { m_asioContext.run(); });

    } catch (std::exception& e) {
      // Something prohibited the server from listening
      std::cerr << "[SERVER] Exception: " << e.what() << '\n';
      return false;
    }

    std::cout << "[SERVER] Started! \n";
    return true;
  }

  void Stop() {
    // Request context to close
    m_asioContext.stop();

    // Tidy the context thread
    if (m_threadContext.joinable()) {
      m_threadContext.join();
    }

    // Inform whom might be concerned
    std::cout << "[SERVER] Stopped!\n";
  }

  // ASYNC - Instruct asio to wait for connection
  void WaitForClientConnection() {
    m_asioAcceptor.async_accept([this](std::error_code ec,
                                       asio::ip::tcp::socket socket) {
      if (!ec) {
        std::cout << "[SERVER] New Connection: " << socket.remote_endpoint()
                  << '\n';

        std::shared_ptr<connection<T>> newconn =
            std::make_shared<connection<T>>(connection<T>::owner::server,
                                            m_asioContext, std::move(socket),
                                            m_qMessagesIn);

        // Give the user a chance to deny connection
        if (OnClientConnect(newconn)) {
          // Connection allowed - add to container of new connections
          m_deqConnections.push_back(std::move(newconn));

          m_deqConnections.back()->ConnectToClient(nIDCounter++);

          std::cout << "[" << m_deqConnections.back()->GetID()
                    << "] Connection Approved\n";
        } else {
          std::cout << "[-----] Connection Denied\n";
        }
      } else {
        // Error has occured during acceptance
        std::cout << "[SERVER] New Connection Error: " << ec.message() << '\n';
      }

      // Prime the asio context with more work - again simply wait
      // for another connection
      WaitForClientConnection();
    });
  }

  // Send a message to a specific client
  void MessageClient(std::shared_ptr<connection<T>> client,
                     const message<T>& msg) {
    if (client && client->IsConnected()) {
      client->Send(msg);
    } else {
      OnClientDisconnect(client);
      client.reset();
      m_deqConnections.erase(
          std::remove(m_deqConnections.begin(), m_deqConnections.end(), client),
          m_deqConnections.end());
    }
  }

  // Send message to all the clients
  void MessageAllClients(
      const message<T>& msg,
      std::shared_ptr<connection<T>> pIgnoreClient = nullptr) {
    bool bInvalidClientExists = false;

    for (auto& client : m_deqConnections) {
      // Check if client is connected
      if (client && client->IsConnected()) {
        if (client != pIgnoreClient) {
          client->Send(msg);
        }
      } else {
        // The client cannot be contacted.
        // So it is assumed disconnected.
        OnClientDisconnect(client);
        client.reset();
        bInvalidClientExists = true;
      }
    }

    if (bInvalidClientExists) {
      m_deqConnections.erase(std::remove(m_deqConnections.begin(),
                                         m_deqConnections.end(), nullptr),
                             m_deqConnections.end());
    }
  }

  // TODO: Refactor flag
  void Update(size_t nMaxMessages = -1, bool bWait = false) {
    // The server should not occupy 100% CPU Core
    if (bWait) {
      m_qMessagesIn.wait();
    }

    size_t nMessageCount = 0;
    while (nMessageCount < nMaxMessages && !m_qMessagesIn.empty()) {
      // Grab the front message
      auto msg = m_qMessagesIn.pop_front();

      // Pass to message handler
      OnMessage(msg.remote, msg.msg);

      nMessageCount++;
    }
  }

 protected:
  // Called when client connects, you can veto the connection by returning false
  virtual bool OnClientConnect(std::shared_ptr<connection<T>> client) {
    return false;
  }

  // Called when a client appears to have disconnected
  virtual void OnClientDisconnect(std::shared_ptr<connection<T>> client) {}

  // Called when message arrives
  virtual void OnMessage(std::shared_ptr<connection<T>> client,
                         message<T>& msg) {}

  // Thread sage queue for incoming message packets
  tsqueue<owned_message<T>> m_qMessagesIn;

  // Container of active validated connections
  std::deque<std::shared_ptr<connection<T>>> m_deqConnections;

  // Order of declaration is important as it is the order of initialization
  asio::io_context m_asioContext;
  std::thread m_threadContext;

  // These things need an asio context
  asio::ip::tcp::acceptor m_asioAcceptor;

  // Clients will be identified in the "wider system" via an ID
  uint32_t nIDCounter = 10000;
};
}  // namespace net
}  // namespace olc
