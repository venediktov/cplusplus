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
#include <boost/tuple/tuple.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <typeinfo>
#include <boost/core/demangle.hpp>
#include <future>
#include <chrono>
 
namespace po = boost::program_options;
 
namespace constant {
    const std::string PP_PROGRAM_NAME = "pp_program_name" ;
    const std::string ORDER_QUEUE_NAME = "orders_queue" ;
}



using mpclmi::ipc::Shared;
using interactive::OrderContract;
using namespace boost::interprocess;
 
OrderContract  fetch_order() ;

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
            ("attempts,-N",  po::value<int>(&reconnect_n), "specify number of attempts to reconnect before giving up")
            ("host,-H",  po::value<std::string>(&host) , "specify host")
            ("port,-P",  po::value<int>(&port), "specify port numer");
 
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
    
    /*
     * Code below will go on the client side GUI and will dubmit orders via boost IPC queue
     */
    
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
    std::this_thread::sleep_for(std::chrono::seconds(3)) ;
    std::cout << "start reading from " << constant::ORDER_QUEUE_NAME << std::endl ;
    //read from queue and push orders into book
    std::future<void> order_client = std::async(std::launch::async, [&mq]() {
        
        Order order;
        Contract contract;
        contract.symbol = "IBM";
	contract.secType = "STK";
	contract.exchange = "ARCA" ; //SMART";
	contract.currency = "USD";
 
        order.account = "DUC00074" ;
	order.action = "BUY";
	order.totalQuantity = 1000;
	order.orderType = "LMT";
	order.lmtPrice = 0.01;
        OrderContract order_with_contract(order,contract) ;
        order_with_contract.place() ;
        std::stringstream ss;
        boost::archive::text_oarchive oarch(ss);
        order_with_contract.serialize(oarch) ;
        std::string wire_order(ss.str()) ;
        while(true) {
//            book.place_order(contract, order) ;
            //Fill up the boost ipc message queue
            mq.send(wire_order.data(), wire_order.size(), 0) ; //priority 0
            std::cout << "placing order to queue=" << wire_order << std::endl ;
            std::this_thread::sleep_for(std::chrono::seconds(2)) ;
        }
    });
    
    order_client.wait();
   
}
 

OrderContract  fetch_order() 
{
    using namespace boost::interprocess;
    OrderContract order;
    try {
         //Open a message queue.
        message_queue mq (open_only,constant::ORDER_QUEUE_NAME.c_str()) ;
        unsigned int priority;
        message_queue::size_type recvd_size;
        char buf[1024] ;
        
        mq.receive(&buf[0], sizeof(buf), recvd_size, priority) ;
        
        if (recvd_size > sizeof (buf)) {
            std::string err = std::string("boost::interpcess::mq::receive size exceeds expected max of ") + 
            boost::lexical_cast<std::string>(sizeof(buf)) + " bytes" ; 
            throw std::runtime_error(err);
        }
        std::cout << "draining order = " << std::string(buf,recvd_size) << std::endl ;
        std::stringstream ss (std::string(buf,recvd_size));
        boost::archive::text_iarchive iarch(ss);
        iarch >> order; //de-serialize
    } catch(const interprocess_exception &ex){
       throw ;
    } catch (const std::exception & e) {
       throw ;
    }
    
    return order;
}
