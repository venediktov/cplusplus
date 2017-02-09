
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
#include "messaging/communicator.hpp"

#define LOG(x) BOOST_LOG_TRIVIAL(x) //TODO: move to core.hpp

extern void init_framework_logging(const std::string &) ;

namespace po = boost::program_options;
using namespace vanilla::messaging;

int main(int argc, char**argv) {

  init_framework_logging("/tmp/openrtb_cache_test_log");

  std::string local_address;
  std::string remote_address;
  std::string type;
  short port;
  po::options_description desc;
        desc.add_options()
            ("help", "produce help message")
            ("local_address", po::value<std::string>(&local_address)->default_value("0.0.0.0"), "listen to this address")
            ("remote_address",po::value<std::string>(&remote_address)->required(), "respond to remote address")
            ("port",po::value<short>(&port)->required(), "port")
            ("type", po::value<std::string>(&type)->default_value("broadcast"), "")
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


  boost::asio::io_service io_service;
  communicator<broadcast> comm(io_service,
      port,
      boost::asio::ip::address::from_string(local_address),
      boost::asio::ip::address::from_string(remote_address)
  );
  //comm.broadcast() ;
  LOG(info) << "Started..." ;
  io_service.run();
  
}


