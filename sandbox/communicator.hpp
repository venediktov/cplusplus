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
// communicator().sender<broadcast>(port).distribute([] (...) {}).collect(85ms, [] (...) {}) ;
// communicator().sender<multicast>(port,group_address).distribute([] (...) {}).collect(75ms, [] (...) {}) ;
// communicator().receiver<broadcast>(port).process([] (...) {}) ; //blocks in io_service.run()
// communicator().receiver<multicast>(port,group_address).process([] (...) {}) ; //blocks in io_service.run()
//

#include <iostream>
#include <string>
#include <thread>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>

namespace vanilla { namespace messaging {

    
struct multicast {
    template<typename SocketType, typename IPAddress>
    void receiver_set_option(SocketType && socket, const unsigned short port, IPAddress && listen_address, IPAddress && multicast_address) {
        boost::asio::ip::udp::endpoint listen_endpoint{listen_address, port};
        socket.open(listen_endpoint.protocol());
        socket.set_option(boost::asio::ip::udp::socket::reuse_address(true));
        socket.set_option(boost::asio::ip::multicast::join_group(std::forward<IPAddress>(multicast_address)));
        socket.bind(listen_endpoint);
    }
    template<typename SocketType, typename IPAddress>
    auto sender_endpoint(SocketType && socket, const unsigned short port, IPAddress &&  address) {
        boost::asio::ip::udp::endpoint send_endpoint{std::forward<IPAddress>(address), port};
        socket.open(boost::asio::ip::udp::v4());
        return send_endpoint;
    }
};

struct broadcast {
    template<typename SocketType>
    void receiver_set_option(SocketType && socket, const unsigned short port) {
        boost::asio::ip::udp::endpoint listen_endpoint{boost::asio::ip::udp::v4(), port};
        socket.open(listen_endpoint.protocol());
        socket.set_option(boost::asio::ip::udp::socket::reuse_address(true));
        socket.bind(listen_endpoint);
    }
    template<typename SocketType>
    auto sender_endpoint(SocketType && socket, const unsigned short port) {
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
  receiver(boost::asio::io_service& io_service, const unsigned short port, IPAddress && ...addresses) :
    receive_socket_{io_service}, timer_{io_service} {
    ConnectionPolicy::receiver_set_option(receive_socket_, port , std::forward<IPAddress>(addresses)...);
  }

  template<typename Serializable>
  void send_async( Serializable && data, std::shared_ptr<boost::asio::ip::udp::endpoint> endpoint) {
     std::stringstream ss;
     boost::archive::binary_oarchive oarch(ss);
     oarch << std::forward<Serializable>(data) ;
     auto data_p = std::make_shared<std::string>();
     *data_p = std::move(ss.str()) ;
    
     receive_socket_.async_send_to(
        boost::asio::buffer(*data_p), *endpoint,
        [](const boost::system::error_code& error, std::size_t bytes_transferred) {
     });
  }

  template<typename Handler>
  void receive_async( data_type & data, Handler && handler ) {
      auto from_endpoint = std::make_shared<boost::asio::ip::udp::endpoint>() ;
      receive_socket_.async_receive_from(
          boost::asio::buffer(data.data(), data.size()), *from_endpoint,
              [this,&data,&handler,from_endpoint](const boost::system::error_code& error, size_t bytes_recvd) {
                handler(from_endpoint, std::move(std::string(data.data(),bytes_recvd))); 
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
      auto from_endpoint = std::make_shared<boost::asio::ip::udp::endpoint>() ;
      receive_socket_.async_receive_from(
          boost::asio::buffer(data.data(), data.size()), *from_endpoint,
          [this,&data,&handler,from_endpoint](const boost::system::error_code& error, size_t bytes_recvd) {
          handler(from_endpoint, std::move(std::string(data.data(),bytes_recvd)));
          handle_receive_from(error, std::forward<Handler>(handler), data, bytes_recvd);
      });

    }
  }

  boost::asio::ip::udp::socket receive_socket_;
  boost::asio::deadline_timer timer_;
};

template<typename ConnectionPolicy, unsigned int MAX_DATA_SIZE = 4 * 1024>
class sender : ConnectionPolicy
{
public:
  using data_type = std::array<char, MAX_DATA_SIZE> ;

  template<typename ...IPAddress>
  sender(boost::asio::io_service& io_service, const unsigned short port, IPAddress && ...addresses) :
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
        [](const boost::system::error_code& error, std::size_t bytes_transferred) {
     });
  }

  template<typename Handler>
  void receive_async( data_type & data, Handler && handler ) {
      auto from_endpoint = std::make_shared<boost::asio::ip::udp::endpoint>() ;
      send_socket_.async_receive_from(
          boost::asio::buffer(data.data(), data.size()), *from_endpoint,
              [this,&data,&handler,from_endpoint](const boost::system::error_code& error, size_t bytes_recvd) {
                handler(std::move(std::string(data.data(),bytes_recvd))); 
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
      auto from_endpoint = std::make_shared<boost::asio::ip::udp::endpoint>() ;
      send_socket_.async_receive_from(
          boost::asio::buffer(data.data(), data.size()), *from_endpoint,
          [this,&data,&handler,from_endpoint](const boost::system::error_code& error, size_t bytes_recvd) {
          handler(std::move(std::string(data.data(),bytes_recvd)));
          handle_receive_from(error, std::forward<Handler>(handler), data, bytes_recvd);
      });

    }
  }

  boost::asio::ip::udp::socket send_socket_;
  boost::asio::deadline_timer timer_;
  boost::asio::ip::udp::endpoint endpoint_;
};




template<typename ConnectionPolicy>
class communicator {
    using consumer_type   = std::shared_ptr<receiver<ConnectionPolicy>>;
    using distributor_type = std::shared_ptr<sender<ConnectionPolicy>>;
public:
    using self_type = communicator<ConnectionPolicy> ;
    communicator() : io_service_{}, timer_{io_service_}
    {}
    communicator(communicator &&) = delete;
    communicator(communicator &) = delete;
    communicator &operator=(communicator &) = delete;
    communicator && operator=(communicator &&) = delete;

    template<typename ...IPAddress>
    self_type & outbound(const unsigned short port, IPAddress && ...addresses) {
        distributor_ = std::make_shared<sender<ConnectionPolicy>>(io_service_, port, std::forward<IPAddress>(addresses)...);
        return *this;
    }

    template<typename ...IPAddress>
    self_type &  inbound(const unsigned short port, IPAddress && ...addresses) {
        consumer_ = std::make_shared<receiver<ConnectionPolicy>>(io_service_, port, std::forward<IPAddress>(addresses)...);
        return *this;
    }

    
    template<typename Serializable>
    self_type & distribute(Serializable && data) {
        if(distributor_) {
            distributor_->send_async(std::forward<Serializable>(data));
        }
        return *this;
    }

    template<typename Handler>
    self_type & process(Handler && handler) {
       if( consumer_ ) {
           //intercept a call from receive , get response from handler , send reponse back to from_endpoint
           consumer_->receive_async(data_, [this,&handler](auto from_endpoint, auto data) { //intercept a call
               auto response = std::forward<Handler>(handler)(from_endpoint,data);
               consumer_->send_async(response, from_endpoint);
           });
       }
       return *this;
    }

    template<typename Duration, typename Handler>
    void collect(Duration && timeout, Handler && handler) {
       if( !distributor_ ) {
           return;
       }
       distributor_->receive_async(data_, std::forward<Handler>(handler));
       timer_.expires_from_now(boost::posix_time::milliseconds(timeout.count()));
       timer_.async_wait( [this](const boost::system::error_code& error) {
           io_service_.stop();
       });
       //TODO: need optimized return before timer if all data is collected from all responders
       //std::this_thread::sleep_for(std::forward<Duration>(timeout));
       io_service_.run();
    }

    void dispatch() {
        io_service_.run();
    }
    
private:
    boost::asio::io_service     io_service_;
    boost::asio::deadline_timer timer_;
    typename receiver<ConnectionPolicy>::data_type data_;
    distributor_type distributor_;
    consumer_type   consumer_;
};


}}
