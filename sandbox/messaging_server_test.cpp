
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
#include <boost/archive/binary_iarchive.hpp>
#include "messaging/communicator.hpp"

#define LOG(x) BOOST_LOG_TRIVIAL(x) //TODO: move to core.hpp

extern void init_framework_logging(const std::string &) ;

namespace po = boost::program_options;
using namespace vanilla::messaging;

int main(int argc, char**argv) {

  init_framework_logging("/tmp/openrtb_cache_test_log");

  std::string local_address;
  std::string group_address;
  std::string type;
  short port;
  po::options_description desc;
        desc.add_options()
            ("help", "produce help message")
            ("local_address", po::value<std::string>(&local_address)->default_value("0.0.0.0"), "bind to this address locally")
            ("group_address",po::value<std::string>(&group_address)->default_value("0.0.0.0"), "join on remote address (only used for multicast)")
            ("port",po::value<short>(&port)->required(), "port")
            ("type", po::value<std::string>(&type)->default_value("broadcast"), "communication types : multicast , broadcast")
        ;

  boost::program_options::variables_map vm;
  try {
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);
  } catch ( const std::exception& e) {
    LOG(error) << e.what() << ":" << desc;
    return 1;
  }

  if (vm.count("help")) {
      LOG(info) << desc;
      return 0;
  }


LOG(info) << "Started..." ;
communicator<broadcast>().inbound(port).process([](auto from_endpoint, std::string data) { //data is std::moved
    LOG(info) << "Received(Broadcast:" << *from_endpoint  << "):" << data;
}).run();

/*******************
  boost::asio::io_service io_service;

  receiver<broadcast>::data_type data;
    receiver<broadcast> comm(io_service, port);
       comm.receive_async(data, [](auto from_endpoint, std::string serialized_data) {
          std::stringstream ss (serialized_data);
          boost::archive::binary_iarchive iarch(ss);
          std::string data;
          iarch >> data;
          LOG(info) << "Received(Broadcast:" << *from_endpoint  << "):" << data;
       }) ;


  auto data = std::make_shared<receiver<broadcast>::data_type>();
  if ( type == "broadcast" ) {
    receiver<broadcast> comm(io_service, port);
       comm.receive_async(*data, [data](std::string serialized_data) {
          if ( serialized_data.empty() ) return;
          std::stringstream ss (serialized_data);
          boost::archive::binary_iarchive iarch(ss);
          std::string data;
          iarch >> data;
          LOG(info) << "Received(Broadcast):" << data;
       }) ;
  } else {
      receiver<multicast> comm(io_service, 
          port, 
          boost::asio::ip::address::from_string(local_address), 
          boost::asio::ip::address::from_string(group_address)
      );
      comm.receive_async(*data, [data](std::string serialized_data) {
          if ( serialized_data.empty() ) return;
          std::stringstream ss (serialized_data);
          boost::archive::binary_iarchive iarch(ss);
          std::string data;
          iarch >> data;
          LOG(info) << "Received(Multicast):" << data;
      });
  }
*********************/

  //template<typename Serializible>
  //communicator.distribute(Serializibe &&data) ;
  //template<typename Serializible>
  //communicator.gather( std::vector<Serializible> &data) ;

  //LOG(info) << "Started..." ;
  //io_service.run();
  
}


