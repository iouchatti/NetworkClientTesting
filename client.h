#pragma once

#include <boost/asio.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <mutex>
#include <string>

class Client : public std::enable_shared_from_this<Client> {
public:
    Client(boost::asio::io_context& ioContext, const std::string& host, short port, std::string clientName );
    void connect();
    void doRead();

    void startTimer();
    void doPeriodicWrite();
    void disconnect();
    void setMessage(const std::shared_ptr<std::string>& value) { wMessage_ = value; }
    std::shared_ptr<std::string> getMessage() const { return wMessage_; }
    void doWrite();
    void doWrite(const std::shared_ptr<std::string>& message);
    std::string getClientName() const  
    {
        return clientName_;
    }
private:


    boost::asio::io_context& ioContext_;
    boost::asio::ip::tcp::socket socket_;
    boost::asio::ip::tcp::endpoint endpoint_;
    boost::asio::io_context::strand strand_;
    boost::asio::steady_timer timer_;
    std::array<char, 128> rData_;
    std::array<char, 128> wData_;
    std::shared_ptr<std::string> rMessage_;
    std::shared_ptr<std::string> wMessage_;
    std::mutex messageMutex_;
    boost::asio::io_context::work work_;
    std::string clientName_;

};
