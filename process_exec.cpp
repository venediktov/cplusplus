/* 
 * File:   process_simple.cpp
 * Author: Vladimir Venediktov
 *
 * Created on Feb 19, 2016, 7:53:10 PM
 */

#include "process.hpp"
#include <iostream>
#include <string>
#include <boost/program_options.hpp>
#include <boost/optional.hpp>

namespace po = boost::program_options ;
using os::unix::Process ;

void process_single(const po::variables_map &vm, const std::string &queue_name );

const std::string PP_THRESH = "thresh-hold" ;
const std::string PP_PROGRAM_NAME = "program-name" ;
const std::string PP_STDIN = "stdin" ; //child reads accounts from stdin ( pipe )
const std::string QUEUE_NAME = "child_queue_name" ;

int main(int argc, char** argv) {
    bool success = false ;
    po::variables_map vm;
    po::options_description desc("Allowed options") ;
    int opt;
    desc.add_options()
        ("help,h", "display help screen")
        (PP_STDIN.c_str(), "message passed in via pipe bound to stdin")
        ((PP_THRESH + ",t").c_str(), po::value<int>(&opt)->default_value(50), "max number of processes to spawn");
    try {
        po::store(po::parse_command_line(argc, argv, desc), vm) ;
        po::notify(vm) ;
    } catch(const boost::program_options::error &e) {
        std::cerr << desc << std::endl;
        return (EXIT_FAILURE);
    }
   
    vm.insert(po::variables_map::value_type(PP_PROGRAM_NAME, po::variable_value(std::string(argv[0]), false))) ;

    //display usage
    if (vm.count("help")) { std::clog << desc << std::endl; return 0;}
    try { 
        auto handle = []( const po::variables_map &vm, const std::string &qn) { process_single(vm,qn) ; } ;
        using Handler = decltype(handle) ;
        Process<> parent_proc;
        Process<Handler> child_proc(handle) ;
        if ( vm.count(PP_STDIN) ) { // if --stdin is passed to the executable it means it reads from fd, see the else part
            std::string queue_name;
            while (!std::getline(std::cin, queue_name).eof() && !queue_name.empty()) {
                child_proc(vm,queue_name) ;            
            }
        } else {
            std::string program = vm[PP_PROGRAM_NAME].as<std::string>() ;
            int thresh_hold     = vm[PP_THRESH].as<int>() ;
            std::string stream_name = std::string("--") + PP_STDIN ;
            std::vector<std::string> args { program , stream_name } ;
            std::vector<std::string> queue_names(thresh_hold, QUEUE_NAME + "\n");
            std::vector<decltype(handle)> child_procs(thresh_hold, handle) ;
            auto spawned_exec_procs = parent_proc.spawn_exec(child_procs, args) ;
            parent_proc.distribute(spawned_exec_procs.second, queue_names) ;
            parent_proc.wait(spawned_exec_procs.first) ;
        }
    } catch ( const std::exception &e) {
        std::cerr << e.what() << std::endl;
        return (EXIT_FAILURE);
    }
    return (EXIT_SUCCESS);
}

void process_single(const po::variables_map &vm, const std::string &queue_name ) {
    std::clog << 
    queue_name +  ",pid=" + 
    boost::lexical_cast<std::string>(getpid()) + 
    ", program_name=" + vm[PP_PROGRAM_NAME].as<std::string>() + "\n";
    //while speeping check with ps -ef your processes running
    sleep(15) ;
}
