#include <asio.hpp>
#include <chrono>
#include <iostream>

// #define ASIO_STANDALONE
#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>

int main() {
  asio::error_code ec;

  // Create a context
  asio::io_context context;

  // Get the address to connect to example.com "93.184.216.34"
  // "51.38.81.49" - returns more data
  asio::ip::tcp::endpoint endpoint(asio::ip::make_address("51.38.81.49", ec),
                                   80);

  // Create a socket, the context will deliver the implementation
  asio::ip::tcp::socket socket(context);

  // Tell socket to try and connect
  socket.connect(endpoint, ec);

  if (!ec) {
    std::cout << "Connected!\n";
  } else {
    std::cout << "Failed to connect to address:\n" << ec.message() << '\n';
  }

  if (socket.is_open()) {
    std::string sRequest =
        "GET /index.html HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "Connection: close\r\n\r\n";

    socket.write_some(asio::buffer(sRequest.data(), sRequest.size()), ec);

    // Hack: Wait for response
    // using namespace std::chrono_literals;
    // std::this_thread::sleep_for(200ms);

    socket.wait(socket.wait_read);

    size_t bytes = socket.available();
    std::cout << "Bytes available: " << bytes << '\n';

    if (bytes > 0) {
      std::vector<char> vBuffer(bytes);
      socket.read_some(asio::buffer(vBuffer.data(), vBuffer.size()), ec);

      for (auto c : vBuffer) {
        std::cout << c;
      }
    }
  }

  return 0;
}