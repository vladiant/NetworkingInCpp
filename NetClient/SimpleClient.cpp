#include <iostream>

// Headers to check pressed key in Linux
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

#include "olc_net.h"

enum class CustomMsgTypes : uint32_t {
  ServerAccept,
  ServerDeny,
  ServerPing,
  MessageAll,
  ServerMessage
};

class CustomClient : public olc::net::client_interface<CustomMsgTypes> {
 public:
  void PingServer() {
    olc::net::message<CustomMsgTypes> msg;
    msg.header.id = CustomMsgTypes::ServerPing;

    // Caution with this
    std::chrono::system_clock::time_point time_now =
        std::chrono::system_clock::now();

    msg << time_now;
    Send(msg);
  }

  void MessageAll() {
    olc::net::message<CustomMsgTypes> msg;
    msg.header.id = CustomMsgTypes::MessageAll;
    Send(msg);
  }
};

int main() {
  CustomClient c;
  c.Connect("127.0.0.1", 60000);

  // Structures to check for pressed key in Linux
  struct termios oldSettings, newSettings;

  tcgetattr(fileno(stdin), &oldSettings);
  newSettings = oldSettings;
  newSettings.c_lflag &= (~ICANON & ~ECHO);
  tcsetattr(fileno(stdin), TCSANOW, &newSettings);

  bool bQuit = false;
  while (!bQuit) {
    // Check for pressed key in Linux
    fd_set set;
    struct timeval tv;

    tv.tv_sec = 0;
    tv.tv_usec = 100;

    FD_ZERO(&set);
    FD_SET(fileno(stdin), &set);

    int res = select(fileno(stdin) + 1, &set, NULL, NULL, &tv);
    if (res > 0) {
      char chr;
      read(fileno(stdin), &chr, 1);

      std::cout << "Got char: " << chr << std::endl;

      switch (chr) {
        case '1':
          c.PingServer();
          break;
        case '2':
          c.MessageAll();
          break;
        case '3':
          bQuit = true;
          break;
      }

    } else if (res < 0) {
      // Select Error
      break;
    } else {
      // Select Timeout
    }

    if (c.IsConnected()) {
      if (!c.Incoming().empty()) {
        auto msg = c.Incoming().pop_front().msg;

        switch (msg.header.id) {
          case CustomMsgTypes::ServerAccept: {
            // Server has responded to a ping request
            std::cout << "Server has accepted connection\n";
          } break;

          case CustomMsgTypes::ServerPing: {
            std::chrono::system_clock::time_point timeNow =
                std::chrono::system_clock::now();
            std::chrono::system_clock::time_point timeThen;
            msg >> timeThen;
            std::cout
                << "Ping "
                << std::chrono::duration<double>(timeNow - timeThen).count()
                << '\n';
          } break;

          case CustomMsgTypes::ServerMessage: {
            // Server has responded to a ping request
            uint32_t clientID;
            msg >> clientID;
            std::cout << "Hello from [" << clientID << "]\n";
          } break;
        }
      }
    } else {
      std::cout << "Server Down\n";
      bQuit = true;
    }
  }

  // Check for pressed key in Linux
  tcsetattr(fileno(stdin), TCSANOW, &oldSettings);

  return 0;
}