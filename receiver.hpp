//
// receiver.hpp
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

const short multicast_port = 30001;


struct multicast_connection_policy { 
    template<typename SocketType, typename IPAddress>
    void set_option(SocketType && socket, IPAddress && listen_address, IPAddress && multicast_address, const short port) { 
        boost::asio::ip::udp::endpoint listen_endpoint( listen_address, port);
        socket_.open(listen_endpoint.protocol());
        socket_.set_option(boost::asio::ip::udp::socket::reuse_address(true));
        socket_.bind(listen_endpoint);
        socket.set_option(boost::asio::ip::multicast::join_group(std::forwartd<IPAddress>(multicast_address)));
    }
};

struct broadcast_connection_policy {
    template<typename SocketType, typename ...IPAddress>
    void set_option(SocketType && socket, IPAddress &&  ...addresses, const short port) {
        boost::asio::ip::udp::endpoint listen_endpoint(ip::udp::v4(), 0);
        socket.set_option(boost::asio::socket_base::broadcast(true));
    }
};



template<typename ConnectionPolicy>
class receiver : ConnectionPolicy
{
public:
  template<typename ...IPAddress>
  receiver(boost::asio::io_service& io_service, IPAddress && ...addresses) : socket_(io_service) {
    ConnectionPolicy::set_option(socket_, std::forward<IPAddress>(addresses)..., multicast_port);
    socket_.async_receive_from(
        boost::asio::buffer(data_, max_length), sender_endpoint_,
        boost::bind(&receiver::handle_receive_from, this,
          boost::asio::placeholders::error,
          boost::asio::placeholders::bytes_transferred));
  }

  void handle_receive_from(const boost::system::error_code& error,
      size_t bytes_recvd)
  {
    if (!error)
    {
      //std::cout.write(data_, bytes_recvd);
      //std::cout << std::endl;

      socket_.async_receive_from(
          boost::asio::buffer(data_), sender_endpoint_,
          boost::bind(&receiver::handle_receive_from, this,
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred));
    }
  }

private:
  boost::asio::ip::udp::socket socket_;
  boost::asio::ip::udp::endpoint sender_endpoint_;
  enum { max_length =  };
  boost::array<char, 65*1024> data_;
};


