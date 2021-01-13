#pragma once

#include "net_common.h"
#include "net_connection.h"
#include "net_message.h"
#include "net_tsqueue.h"

namespace olc {
namespace net {

template <typename T>
class client_interface {
 public:
  client_interface() {
    // Initialize the socket with the io context, so it can do stuff
  }

  virtual ~client_interface() {
    // If the client is destroyed always try to disconnect from server
    Disconnect();
  }

  // Conenct to server with hostname/ip-address and port
  bool Connect(const std::string& host, const uint16_t port) {
    try {
      // Resolve hostname/ip-addressinto tangible physical address
      asio::ip::tcp::resolver resolver(m_context);
      asio::ip::tcp::resolver::results_type endpoints =
          resolver.resolve(host, std::to_string(port));

      // Create connection
      m_connection = std::make_unique<connection<T>>(
          connection<T>::owner::client, m_context,
          asio::ip::tcp::socket(m_context), m_qMessagesIn);

      // Tell the connection object to connect to server
      m_connection->ConnectToServer(endpoints);

      // Start connection thread
      thrContext = std::thread([this]() { m_context.run(); });

    } catch (std::exception& e) {
      std::cerr << "Client Exception: " << e.what() << '\n';
      return false;
    }
    return false;
  }

  // Disconnect from server
  void Disconnect() {
    // If connection exists and is connected...
    if (IsConnected()) {
      // ... disconnect from server gracefully
      m_connection->Disconnect();
    }

    // Finalize with asio context
    m_context.stop();

    // Finalize with context thread
    if (thrContext.joinable()) {
      thrContext.join();
    }

    // Destroy connection object
    m_connection.reset();
  }

  // Check if client is actually connected to a server
  bool IsConnected() {
    if (m_connection) {
      return m_connection->IsConnected();
    }
    return false;
  }

  // Retrieve queue of messages from server
  tsqueue<owned_message<T>>& Incoming() { return m_qMessagesIn; }

  void Send(const message<T>& msg) {
    if (m_connection && IsConnected()) {
      m_connection->Send(msg);
    }
  }

 protected:
  // asio context handles the data transfer
  asio::io_context m_context;

  // Thread for context to execute its commands
  std::thread thrContext;

  // Connection object to handle data transfer
  std::unique_ptr<connection<T>> m_connection;

 private:
  // This is the thread safe queue of incoming messages fro mthe server
  tsqueue<owned_message<T>> m_qMessagesIn;
};

}  // namespace net
}  // namespace olc
