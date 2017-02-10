//
// communicator.hpp
// ~~~~~~~~~~~~
//
// Copyright (c) 2003-2008 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Refactored beyond recognizable By: Vladimir Venediktov
// introduced multicast and broadcast policies and replaced class variables
// for endpoint to new managed heap instance for every incoming as it can be from a different 
// incoming IP and if io_service is running on multiple threads it can create race condition
//
// Possible use cases: 
// communicator(io_service).sender<broadcast>(port).distribute([] (...) {}).collect(85ms, [] (...) {}) ;
// communicator(io_service).sender<multicast>(port,group_address).distribute([] (...) {}).collect(75ms, [] (...) {}) ;
// communicator(io_service).receiver<broadcast>(port).process([] (...) {}) ; //blocks in io_service.run()
// communicator(io_service).receiver<multicast>(port,group_address).process([] (...) {}) ; //blocks in io_service.run()
//

#include <iostream>
#include <string>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>

namespace vanilla { namespace messaging {

struct multicast {
    template<typename SocketType, typename IPAddress>
    void receiver_set_option(SocketType && socket, const ushort port, IPAddress && listen_address, IPAddress && multicast_address) {
        boost::asio::ip::udp::endpoint listen_endpoint{listen_address, port};
        socket.open(listen_endpoint.protocol());
        socket.set_option(boost::asio::ip::udp::socket::reuse_address(true));
        socket.bind(listen_endpoint);
        socket.set_option(boost::asio::ip::multicast::join_group(std::forward<IPAddress>(multicast_address)));
    }
    template<typename SocketType, typename IPAddress>
    auto sender_endpoint(SocketType && socket, const ushort port, IPAddress &&  address) {
        boost::asio::ip::udp::endpoint send_endpoint{std::forward<IPAddress>(address), port};
        socket.open(send_endpoint.protocol());
        return send_endpoint;
    }
};

struct broadcast {
    template<typename SocketType>
    void receiver_set_option(SocketType && socket, const ushort port) {
        boost::asio::ip::udp::endpoint listen_endpoint{boost::asio::ip::udp::v4(), port};
        socket.open(listen_endpoint.protocol());
        socket.bind(listen_endpoint);
        socket.set_option(boost::asio::ip::udp::socket::reuse_address(true));
    }
    template<typename SocketType>
    auto sender_endpoint(SocketType && socket, const ushort port) {
        socket.open(boost::asio::ip::udp::v4());
        socket.set_option(boost::asio::socket_base::broadcast(true));
        return boost::asio::ip::udp::endpoint(boost::asio::ip::address_v4::broadcast(), port);
    }
};


template<typename ConnectionPolicy, unsigned int MAX_DATA_SIZE = 4 * 1024>
class receiver : ConnectionPolicy
{
public:
  using data_type = std::array<char, MAX_DATA_SIZE> ;

  template<typename ...IPAddress>
  receiver(boost::asio::io_service& io_service, const ushort port, IPAddress && ...addresses) : 
    receive_socket_{io_service} {
    ConnectionPolicy::receiver_set_option(receive_socket_, port , std::forward<IPAddress>(addresses)...);
  }

  template<typename Handler>
  void receive_async( data_type & data, Handler && handler ) {
      auto from_endpoint = std::make_shared<boost::asio::ip::udp::endpoint>() ;
      receive_socket_.async_receive_from(
          boost::asio::buffer(data.data(), data.size()), *from_endpoint,
              [this,&data,&handler,from_endpoint](const boost::system::error_code& error, size_t bytes_recvd) {
                handler(std::string(data.data(),bytes_recvd));
                handle_receive_from(error, std::forward<Handler>(handler), data, bytes_recvd);
      });
  }

private:
  template<typename Handler>
  void handle_receive_from(const boost::system::error_code& error, 
                           Handler && handler, 
                           data_type &data, 
                           size_t bytes_recvd) 
  {
    if (!error) {
      //std::cout.write(data.data(), bytes_recvd);
      //std::cout << std::endl;
      auto from_endpoint = std::make_shared<boost::asio::ip::udp::endpoint>() ;
      receive_socket_.async_receive_from(
          boost::asio::buffer(data.data(), data.size()), *from_endpoint,
          [this,&data,&handler,from_endpoint](const boost::system::error_code& error, size_t bytes_recvd) {
          handler(std::string(data.data(),bytes_recvd));
          handle_receive_from(error, std::forward<Handler>(handler), data, bytes_recvd);
      });

    }
  }

  boost::asio::ip::udp::socket receive_socket_;
};

template<typename ConnectionPolicy>
class sender : ConnectionPolicy
{
public:
  template<typename ...IPAddress>
  sender(boost::asio::io_service& io_service, const short port, IPAddress && ...addresses) :
    send_socket_{io_service}, timer_{io_service},
    endpoint_{ConnectionPolicy::sender_endpoint(send_socket_, port , std::forward<IPAddress>(addresses)...)}
  {}

  template<typename Serializable>
  void send_async( Serializable && data) {
     std::stringstream ss;
     boost::archive::binary_oarchive oarch(ss);
     oarch << std::forward<Serializable>(data) ;
     auto data_p = std::make_shared<std::string>();
     *data_p = std::move(ss.str()) ;
    
     send_socket_.async_send_to(
        boost::asio::buffer(*data_p), endpoint_,
        [this,data_p](const boost::system::error_code& error, std::size_t bytes_transferred) {
           handle_send_to(error, data_p);
        });
  }

private:
  void handle_send_to(const boost::system::error_code& error, std::shared_ptr<std::string> data) {
    if (!error) {
      timer_.expires_from_now(boost::posix_time::seconds(1));
      timer_.async_wait( [this,data](const boost::system::error_code& error) {
          handle_timeout(error,data) ;
      }) ;
    }
  }

  void handle_timeout(const boost::system::error_code& error, std::shared_ptr<std::string> data ) {
    if (!error) {
      send_socket_.async_send_to(
          boost::asio::buffer(*data), endpoint_,
          [this,data](const boost::system::error_code& error,std::size_t bytes_transferred) {
             handle_send_to(error, data);
      });
    }
  }

  boost::asio::ip::udp::socket send_socket_;
  boost::asio::deadline_timer timer_;
  boost::asio::ip::udp::endpoint endpoint_;
};


}}


