#include <iostream>
#include <string>
#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <thread>

void reader(boost::asio::ip::tcp::socket& socket) {
    try {
        while (true) {
            boost::array<char, 128> buf;
            boost::system::error_code error;

            size_t len = socket.read_some(boost::asio::buffer(buf), error);

            if (error == boost::asio::error::eof)
                break; // Connection closed cleanly by peer.
            else if (error)
                throw boost::system::system_error(error); // Some other error.

            std::cout << "Server response: " << std::string(buf.data(), len) << std::endl;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Exception in reader: " << e.what() << "\n";
    }
}

int main() {
    try {
        // Specify the host and port directly
        std::string host = "127.0.0.1";
        short port = 12345;

        boost::asio::io_context io_context;

        // Resolve the host name and port number to an IP address and service name
        boost::asio::ip::tcp::resolver resolver(io_context);
        auto endpoints = resolver.resolve(host, std::to_string(port));

        // Create a socket and connect to the server
        boost::asio::ip::tcp::socket socket(io_context);
        boost::asio::connect(socket, endpoints);

        std::thread readerThread(reader, std::ref(socket));

        while (true) {
            std::string message;
            std::cout << "Enter message: ";
            std::getline(std::cin, message);

            // Write the message to the server
            boost::asio::write(socket, boost::asio::buffer(message));
        }

        readerThread.join();
    }
    catch (const std::exception& e) {
        std::cerr << "Exception in main: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
