/* 
 * File:   ordermgr.cpp
 * Author: Vladimr Venediktov
 *
 * Created on May 1, 2016, 1:00 PM
 * Work in progress ...
 */

#include "Contract.h"
#include "Order.h"

#include "orderbook.hpp"
#include "memory_types.hpp"
#include <sstream>
#include <boost/program_options.hpp>
#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <future>
#include <chrono>
#include <boost/optional/optional.hpp>
 
namespace po = boost::program_options;
 
namespace constant {
    const std::string PP_PROGRAM_NAME = "pp_program_name" ;
    const std::string ORDER_QUEUE_NAME = "orders_queue" ;
}



using mpclmi::ipc::Shared;
using interactive::OrderContract;
using namespace boost::interprocess;
 
boost::optional<OrderContract>  fetch_order() ;
extern void init_framework_logging(const std::string&);

int main(int argc, char **argv) {
       
    bool success = false;
    po::variables_map vm;
    po::options_description desc("Allowed options");
    int opt;
    std::string host;
    int port ;
    int reconnect_n;
    desc.add_options()
            ("help,h", "display help screen")
            ("attempts,N",  po::value<int>(&reconnect_n), "specify number of attempts to reconnect before giving up")
            ("host,H",  po::value<std::string>(&host) , "specify host")
            ("port,P",  po::value<int>(&port), "specify port numer");
 
    try {
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);
    } catch (const boost::program_options::error &e) {
        std::cerr << desc << std::endl;
        return -1;
    }
    vm.insert(po::variables_map::value_type(constant::PP_PROGRAM_NAME, po::variable_value(std::string(argv[0]), false)));
    //display usage
    if (vm.count("help") ) {
        std::clog << desc << std::endl;
        return 0;
    }
   
    init_framework_logging("/tmp/order_book") ;
    
    //Erase previous message queue
    if (!message_queue::remove(constant::ORDER_QUEUE_NAME.c_str())) {
        std::cout << "Unable to remove message queue " << constant::ORDER_QUEUE_NAME << std::endl ;
    }
    
    //Create a message_queue.
    message_queue mq
    (create_only              //only create
    ,constant::ORDER_QUEUE_NAME.c_str() //name
    ,1024                     //max message number
    ,512                      //max message size
    );

    //Create live book object conencted to IB Gateway
    interactive::OrderBook<mpclmi::ipc::Shared> book("order_book_cache", [](){
        return fetch_order() ;
    });
    
    book.connect(host,port) ; //will start a single thread dispatcher inside the book
    book.run() ; // will wait for dispatcher thread to terminate 

}
 

boost::optional<OrderContract>  fetch_order() 
{
    using namespace boost::interprocess;
    OrderContract order;
    try {
         //Open a message queue.
        message_queue mq (open_only,constant::ORDER_QUEUE_NAME.c_str()) ;
        unsigned int priority;
        message_queue::size_type recvd_size;
        char buf[1024] ;
        
        //100 milliseconds timeout
        boost::posix_time::time_duration delay(boost::posix_time::millisec(100));
        // current time
        boost::posix_time::ptime timeout =
        boost::posix_time::ptime(boost::posix_time::second_clock::local_time());
        timeout += delay;
        if ( !mq.timed_receive(&buf[0], sizeof(buf), recvd_size, priority, timeout) ) {
            return boost::optional<OrderContract>() ;
        }
        
        if (recvd_size > sizeof (buf)) {
            std::string err = std::string("boost::interpcess::mq::receive size exceeds expected max of ") + 
            boost::lexical_cast<std::string>(sizeof(buf)) + " bytes" ; 
            throw std::runtime_error(err);
        }
        std::string s(buf,recvd_size) ;
        std::cout << "draining order = [" << s <<  "]" << std::endl ;
        std::stringstream ss (std::string(buf,recvd_size));
        boost::archive::text_iarchive iarch(ss);
        iarch >> order; //de-serialize
        std::cout << "de-serialized order = [" << order.account << "]" << std::endl ;
    } catch(const interprocess_exception &ex){
       throw ;
    } catch (const std::exception & e) {
       throw ;
    }
    
    return boost::optional<OrderContract>(order);
}
