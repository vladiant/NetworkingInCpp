#include <asio.hpp>
#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>
#include <chrono>
#include <iostream>

// Sufficiently large buffer
std::vector<char> vBuffer(20 * 1024);

void GrabSomeData(asio::ip::tcp::socket& socket) {
  socket.async_read_some(asio::buffer(vBuffer.data(), vBuffer.size()),
                         [&](std::error_code ec, std::size_t length) {
                           if (!ec) {
                             std::cout << "\n\nRead " << length << " bytes\n\n";

                             for (int i = 0; i < length; i++) {
                               std::cout << vBuffer[i];
                             }

                             GrabSomeData(socket);
                           }
                         });
}

int main() {
  asio::error_code ec;

  // Create a context
  asio::io_context context;

  // Give some fake tasks to asio so the context doesn't finish
  asio::io_context::work idleWork(context);

  // Start the context
  std::thread thrContext = std::thread([&]() { context.run(); });

  // Get the address to connect to "51.38.81.49" to returns lengthy data
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
    GrabSomeData(socket);

    std::string sRequest =
        "GET /index.html HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "Connection: close\r\n\r\n";

    socket.write_some(asio::buffer(sRequest.data(), sRequest.size()), ec);

    // Program does something else, while asio handles data transfer in the
    // background It should be long enough to get all the data!
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(20000ms);

    context.stop();
  }

  if (thrContext.joinable()) {
    thrContext.join();
  }

  return 0;
}