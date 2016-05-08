/* 
 * File:   testclnt.cpp
 * Author: Vladimr Venediktov
 *
 * Created on May 8, 2016, 11:30 AM
 * Work in progress ...
 */

#include "Contract.h"
#include "Order.h"

#include "orderbook.hpp" //included for OrderContract.class 
#include <sstream>
#include <boost/program_options.hpp>
#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <future>
#include <chrono>
 
namespace po = boost::program_options;
 
namespace constant {
    const std::string PP_PROGRAM_NAME = "pp_program_name" ;
    const std::string ORDER_QUEUE_NAME = "orders_queue" ;
}



using interactive::OrderContract;
using namespace boost::interprocess;
 

int main(int argc, char **argv) {
       
    bool success = false;
    po::variables_map vm;
    po::options_description desc("Allowed options");
    int opt;
    std::string ticker;
    long qty ;
    std::string order_type;
    desc.add_options()
            ("help,h", "display help screen")
            ("ticker,S",  po::value<std::string>(&ticker)->required(), "specify number of attempts to reconnect before giving up")
            ("qty,Q",  po::value<long>(&qty)->required() , "specify quantity of your order")
            ("order_type,T",  po::value<std::string>(&order_type)->default_value("MKT"), "specify order type MKT/LMT");
 
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
   
    
    //Open a message_queue.
    message_queue mq
    (open_only              //only open 
    ,constant::ORDER_QUEUE_NAME.c_str() //name
    );

    std::cout << "adding order to the queue=" << constant::ORDER_QUEUE_NAME ;
    
    std::future<void> order_client = std::async(std::launch::async, [&mq,&ticker,qty,&order_type]() {
        Order order;
        Contract contract;
        contract.symbol = ticker; //"IBM";
	contract.secType = "STK";
	contract.exchange = "ARCA" ; //SMART";
	contract.currency = "USD";
 
        order.account = "DUC00074" ;
	order.action = "BUY";
	order.totalQuantity = 1000;
	order.orderType = order_type; //"LMT";
	order.lmtPrice = 0.01;
        OrderContract order_with_contract(order,contract) ;
        order_with_contract.place() ;
        std::stringstream ss;
        boost::archive::text_oarchive oarch(ss);
        oarch << order_with_contract;
        std::string wire_order(ss.str()) ;
        mq.send(wire_order.data(), wire_order.size(), 0) ; //priority 0
        std::cout << "placing order to queue=[" << wire_order << "]" << std::endl;
    });
    
    order_client.wait();
   
}
 
