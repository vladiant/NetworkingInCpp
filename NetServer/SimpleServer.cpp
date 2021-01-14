#include <iostream>

#include "olc_net.h"

enum class CustomMsgTypes : uint32_t {
  ServerAccept,
  ServerDeny,
  ServerPing,
  MessageAll,
  ServerMessage
};

class CustomServer : public olc::net::server_interface<CustomMsgTypes> {
 public:
  CustomServer(uint16_t nPort)
      : olc::net::server_interface<CustomMsgTypes>(nPort) {}

 protected:
  bool OnClientConnect(
      std::shared_ptr<olc::net::connection<CustomMsgTypes>> client) override {
    olc::net::message<CustomMsgTypes> msg;
    msg.header.id = CustomMsgTypes::ServerAccept;
    client->Send(msg);

    return true;
  }

  // Called when a client appears to have disconnected
  void OnClientDisconnect(
      std::shared_ptr<olc::net::connection<CustomMsgTypes>> client) override {
    std::cout << "Removing Client [" << client->GetID() << "]\n";
  }

  // Called when message arrives
  void OnMessage(std::shared_ptr<olc::net::connection<CustomMsgTypes>> client,
                 olc::net::message<CustomMsgTypes>& msg) override {
    std::cout << "[" << client->GetID()
              << "] : Message type: " << static_cast<uint32_t>(msg.header.id)
              << "\n";

    switch (msg.header.id) {
      case CustomMsgTypes::ServerPing: {
        std::cout << "[" << client->GetID() << "] : Server Ping\n";

        // Simply bounce message back to client
        client->Send(msg);
      } break;

      case CustomMsgTypes::MessageAll: {
        std::cout << "[" << client->GetID() << "] : Message All\n";
        olc::net::message<CustomMsgTypes> msg;
        msg.header.id = CustomMsgTypes::ServerMessage;
        msg << client->GetID();
        MessageAllClients(msg, client);
      } break;
    }
  }
};

int main() {
  // nc 127.0.0.1 60000
  // telnet 127.0.0.1 60000
  CustomServer server(60000);
  server.Start();

  while (true) {
    server.Update(-1, true);
  }

  return 0;
}
