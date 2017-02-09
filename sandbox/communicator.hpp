//
// communicator.hpp
// ~~~~~~~~~~~~
//
// Copyright (c) 2003-2008 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
// Modified By: Vladimir Venediktov
// introduced multicast and broadcast policies
//

#include <iostream>
#include <string>
#include <boost/asio.hpp>
#include "boost/bind.hpp"

namespace vanilla { namespace messaging {

struct multicast {
    template<typename SocketType, typename IPAddress>
    void set_option(SocketType && socket, const short port, IPAddress && listen_address, IPAddress && multicast_address) {
        boost::asio::ip::udp::endpoint listen_endpoint( listen_address, port);
        socket.open(listen_endpoint.protocol());
        socket.set_option(boost::asio::ip::udp::socket::reuse_address(true));
        socket.bind(listen_endpoint);
        socket.set_option(boost::asio::ip::multicast::join_group(std::forward<IPAddress>(multicast_address)));
    }
};

struct broadcast {
    template<typename SocketType, typename ...IPAddress>
    void set_option(SocketType && socket, const short port, IPAddress &&  ...addresses) {
        boost::asio::ip::udp::endpoint listen_endpoint(boost::asio::ip::udp::v4(), port);
        socket.open(listen_endpoint.protocol());
        socket.bind(listen_endpoint);
        socket.set_option(boost::asio::socket_base::broadcast(true));
    }
};


/***
communicator<broadcast>().distribute([] (...) {}) ;
communicator<broadcast>().collect([] (...) {}) ;
***/

template<typename ConnectionPolicy>
class communicator : ConnectionPolicy
{
public:
  template<typename ...IPAddress>
  communicator(boost::asio::io_service& io_service, const short port, IPAddress && ...addresses) : socket_(io_service) {
    ConnectionPolicy::set_option(socket_, port , std::forward<IPAddress>(addresses)...);
    socket_.async_receive_from(
        boost::asio::buffer(data_.data(), data_.size()), sender_endpoint_,
        boost::bind(&communicator::handle_receive_from, this,
          boost::asio::placeholders::error,
          boost::asio::placeholders::bytes_transferred));
  }
/******
  void send_async(const std::string &data) {
     socket_.async_send_to(
        boost::asio::buffer(data), endpoint_,
        boost::bind(&communicator::handle_send_to, this,
          boost::asio::placeholders::error));
  }
******/
private:
  void handle_receive_from(const boost::system::error_code& error, size_t bytes_recvd) {
    if (!error) {
      std::cout.write(data_.data(), bytes_recvd);
      std::cout << std::endl;

      socket_.async_receive_from(
          boost::asio::buffer(data_), sender_endpoint_,
          boost::bind(&communicator::handle_receive_from, this,
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred));
    }
  }

  boost::asio::ip::udp::socket socket_;
  boost::asio::ip::udp::endpoint sender_endpoint_;
  boost::array<char, 65*1024> data_;
};

}}
