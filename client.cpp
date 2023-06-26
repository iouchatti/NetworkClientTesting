#include "client.h"
#include "logger.h"

Client::Client(boost::asio::io_context& ioContext, const std::string& host, short port, std::string clientName)
    : ioContext_(ioContext),
    socket_(ioContext_),
    endpoint_(boost::asio::ip::address::from_string(host), port),
    strand_(ioContext_),
    timer_(ioContext_),
    work_(ioContext_),
    clientName_(clientName)
{
}

void Client::connect()
{
    auto self = shared_from_this();
    try {
        socket_.async_connect(endpoint_, [this, self](boost::system::error_code ec) {
            if (!ec) {
                Logger::log("Connected to server.");
                doRead();
                startTimer();
            }
            else {
                Logger::log("Connect error: " + ec.message());
                disconnect(); 
            }
            });
    }
    catch (const std::exception& e) {
        Logger::log("Exception in connect: " + std::string(e.what()));
    }
}

void Client::doRead()
{
    auto self(shared_from_this());
    try {
        socket_.async_read_some(boost::asio::buffer(rData_), [this, self](boost::system::error_code ec, std::size_t length) {
            if (!ec) {
                {
                    std::lock_guard<std::mutex> lock(messageMutex_);
                    rMessage_ = std::make_shared<std::string>(rData_.data(), length);
                }
                Logger::log("Received message: " + *rMessage_);
                doRead();
            }
            else {
                Logger::log("Read error: " + ec.message());
                disconnect();  
            }
            });
    }
    catch (const std::exception& e) {
        Logger::log("Exception in doRead: " + std::string(e.what()));
    }
}

void Client::doWrite()
{
    auto self = shared_from_this();
    try {
        std::lock_guard<std::mutex> lock(messageMutex_);
        std::size_t length = wMessage_->size();

        boost::asio::async_write(
            socket_, boost::asio::buffer(*wMessage_, length),
            [this](boost::system::error_code ec, std::size_t) {
                if (!ec) {
                    Logger::log("Sent message: " + *wMessage_);
                }
                else {
                    Logger::log("Write error: " + ec.message());
                    disconnect();   
                }
            });
    }
    catch (const std::exception& e) {
        Logger::log("Exception in doWrite: " + std::string(e.what()));
    }
}

void Client::startTimer()
{
    auto self = shared_from_this();
    timer_.expires_after(std::chrono::seconds(5));
    timer_.async_wait([this, self](boost::system::error_code ec) {
        if (!ec) {
            //doPeriodicWrite();
        }
        else {
            Logger::log("Timer error: " + ec.message());
            disconnect();  
        }
        });
}

void Client::doPeriodicWrite()
{
    auto self = shared_from_this();
    ioContext_.post([this, self]() {
        doWrite();
        startTimer();
        });
}

void Client::disconnect()
{
    auto self = shared_from_this();
    ioContext_.post([this, self]() {
        if (socket_.is_open()) {
            boost::system::error_code ec;
            socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
            socket_.close(ec);
        }
        });
}
