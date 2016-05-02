/* 
 * File:   ordermgr.cpp
 * Author: Vladimr Venediktov
 *
 * Created on May 1, 2016, 1:00 PM
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
#include <typeinfo>
#include <boost/core/demangle.hpp>
#include <future>
#include <chrono>
 
namespace po = boost::program_options;
 
namespace constant {
    const std::string CONFIG = "config";
    const std::string DATE = "date" ;
    const std::string PP_PROGRAM_NAME = "pp_program_name" ;
    const std::string ORDER_QUEUE_NAME = "orders_queue" ;
}
 
using mpclmi::ipc::Shared;
 
 
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
    
    interactive::OrderBook<mpclmi::ipc::Shared> book("order_book_cache");
    book.connect(host,port) ; //will start dispatching 
    std::this_thread::sleep_for(std::chrono::seconds(3)) ;
    std::cout << "start reading from order_queue ..." << std::endl ;
    //read from queue and push orders into book
    std::future<void> order_client = std::async(std::launch::async, [&book]() {
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
        while(true) {
            book.place_order(contract, order) ;
            std::this_thread::sleep_for(std::chrono::seconds(5)) ;
        }
    });
    
    order_client.wait();
    
    //TODO: use boost::program_options validate function instead
//    if ( vm.count(constant::CACHE) ) {
//        if ( cache_str == "core") {
//            cache_type = constant::SECURITY ;
//        } else if (cache_str == "underlying") {
//            cache_type = constant::U_SECURITY ;
//        } else {
//            std::clog << "invalid cache type passed " << cache_str  << std::endl;
//            return EXIT_FAILURE ;
//        }
//    }
    
    //Create a message_queue.
//        message_queue mq
//           (open_or_create        //only create
//           ,constant::ORDER_QUEUE_NAME.c_str() //name
//           ,keys.size()        //max message number
//           ,32                 //max message size
//           );
    
}
 
/*
   
void spawn_distribute_wait(const po::variables_map &vm,
                           const std::string &queue_name, const std::set<std::string> &keys)
{
    using namespace boost::interprocess;
    try {
        //Erase previous message queue
        if (!message_queue::remove(queue_name.c_str())) {
            LOG(std::string("Unable to remove message queue ") + queue_name, pimco::base::ERROR ) ;
        }
        //Create a message_queue.
        message_queue mq
           (create_only        //only create
           ,queue_name.c_str() //name
           ,keys.size()        //max message number
           ,32                 //max message size
           );
 
        //Fill up the boost ipc message queue
        for ( const std::string &ssm_id : keys) {
            mq.send(ssm_id.c_str(), ssm_id.size(), 0) ; //priority 0
        }
        LOG(std::string("Completed populating ") + queue_name + " with " +
            boost::lexical_cast<std::string>(keys.size()) +
            " cusips", pimco::base::INFO ) ;
        std::string config = vm[constant::CONFIG].as<std::string>();
        std::string program = vm[constant::PP_PROGRAM_NAME].as<std::string>();
        int thresh_hold = vm[constant::PP_THRESH].as<int>();
        std::string asof_date = vm[constant::DATE].as<std::string>();
        std::string batch = boost::lexical_cast<std::string> ( vm[constant::PP_BATCH_SIZE].as<long>() ) ;
        std::string stream_name = std::string("--") + constant::PP_STDIN ;
        std::string cache_opt  = std::string("--") + constant::CACHE ;
        std::string cache_name = vm[constant::CACHE].as<std::string>() ;
        std::vector<std::string> args { program, stream_name, cache_opt , cache_name, "--batch" , batch,  "-d" , asof_date, "-c", config } ;
        std::vector<std::string> q_names(thresh_hold, queue_name);
        pimco::base::Process p ;
        p.spawn(args, thresh_hold) ;
        p.distribute(q_names) ;
        p.wait() ;
    } catch(const interprocess_exception &e){
        LOG(e.what(), pimco::base::ERROR) ;
    } catch( const std::exception &e) {
        LOG(e.what(), pimco::base::ERROR) ;
    }
    return;
}
template<typename DAO, typename CacheT>
void process_single(const po::variables_map &vm,
                    const std::string & queue_name,
                    const std::string & request,
                    CacheT &cache,
                    DAO &)
{
    using namespace boost::interprocess;
    try {
        std::string asof_date_str = vm[constant::DATE].as<std::string>();
        long batch_size = vm[constant::PP_BATCH_SIZE].as<long>() ;
        pimco::base::DateTime asof_date(PCDate(asof_date_str, PDDATE_FMT_MM_DD_CCYY)) ;
        pimco::dbaccess::IConnection *con =
             pimco::dbaccess::ConnectionMgr::Instance()->GetConnection("PIMCO_DB_OTL");
        //Open a message queue.
        message_queue mq
        (open_only        //only create
        ,queue_name.c_str()  //name
        );
 
        unsigned int priority;
        message_queue::size_type recvd_size;
        char buf[32] ;
       
        while (true)
        {
            DAO dao;
            std::vector<DAO> values;
            for (long n{batch_size}; n;) {
                // 5 second delay
                boost::posix_time::time_duration delay(boost::posix_time::seconds(5));
                // current time
                boost::posix_time::ptime timeout =
                boost::posix_time::ptime(boost::posix_time::second_clock::local_time());
                //std::cout << "Current time: " <<
                //boost::posix_time::to_simple_string(timeout) << std::endl;
                timeout += delay;
                --n;
                //std::cout << "After delay: " <<
                //boost::posix_time::to_simple_string(timeout) << std::endl;
                if ( !mq.timed_receive(&buf[0], sizeof (buf), recvd_size, priority, timeout) ) {
                    LOG( std::string("boost::interpcess::mq::receive timed out exiting process")
                    + boost::lexical_cast<std::string > (getpid()) , pimco::base::INFO ) ;
                    if ( !n ) { break ; }
                    update_cache(values, cache) ;
                    return ;
                }
                if (recvd_size > sizeof (buf)) {
                    LOG("boost::interpcess::mq::receive size exceeds expected max of 32 bytes", pimco::base::ERROR);
                    return;
                }
                std::string ssm_id(buf, recvd_size);
                fetch_dao_data(con, dao, boost::make_tuple(ssm_id, asof_date), request) ;
                values.emplace_back(dao);
            }
            update_cache(values, cache) ;
        }
    } catch (OTL_CONST_EXCEPTION otl_exception & e) {
        std::stringstream msg;
        msg << "Code=" << e.code
            << ",Msg=" << e.msg
            << ",sql_state=" << e.sqlstate
            << ",info=" << e.var_info
            << ",SQL=" << e.stm_text << std::endl;
        LOG(msg.str(), pimco::base::ERROR) ;
    } catch(const interprocess_exception &ex){
       LOG( std::string("IPC: ") + ex.what(), pimco::base::ERROR);
    } catch (const std::exception & e) {
       LOG(e.what(), pimco::base::ERROR);
    } catch( const Exception &e ) {
       LOG(e.asString(), pimco::base::ERROR);
    }
}
 
*/
