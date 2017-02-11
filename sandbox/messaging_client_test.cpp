
#include <chrono>
#include <iterator>
#include <fstream>
#include <algorithm>
#include <random>
#include <memory>
#include <iostream>
#include <boost/program_options.hpp>
#include <boost/log/trivial.hpp>
#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>
#include "messaging/communicator.hpp"

#define LOG(x) BOOST_LOG_TRIVIAL(x) //TODO: move to core.hpp

extern void init_framework_logging(const std::string &) ;

namespace po = boost::program_options;
using namespace vanilla::messaging;
using namespace std::literals;
int main(int argc, char**argv) {

  init_framework_logging("/tmp/openrtb_messaging_test_log");

  std::string remote_address;
  short port;
  po::options_description desc;
        desc.add_options()
            ("help", "produce help message")
            ("remote_address",po::value<std::string>(&remote_address), "respond to remote address")
            ("port", po::value<short>(&port), "port")
        ;

  boost::program_options::variables_map vm;
  try {
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);
  } catch ( const std::exception& ) {
    LOG(error) << desc;
    return 1;
  }

  if (vm.count("help")) {
      LOG(info) << desc;
      return 0;
  }


communicator<broadcast>().outbound(port).distribute(std::string("hello"))
.collect(2000ms, [](const std::string serialized_data) {
    LOG(info) << "Received back:" << serialized_data ;
});


/*********

//  boost::asio::io_service io_service;
//  sender<broadcast> s(io_service, port);
//  std::string message("hello");
//  s.send_async(message) ;

   boost::system::error_code err;
   boost::asio::ip::udp::socket socket(io_service);

    socket.open(boost::asio::ip::udp::v4(), err);
    if (!err)
    {
        socket.set_option(boost::asio::ip::udp::socket::reuse_address(true));
        socket.set_option(boost::asio::socket_base::broadcast(true));

        boost::asio::ip::udp::endpoint senderEndpoint(boost::asio::ip::address_v4::broadcast(), port);

        std::string message("hello");
        socket.send_to(boost::asio::buffer(message.data(), message.size()), senderEndpoint);
        socket.close(err);
    } else {
        LOG(error) << "Cannot create udp socket " << err;
    } 
*******/
}


