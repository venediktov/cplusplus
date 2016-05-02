/* 
 * File:   test_move.cpp
 * Author: vlad1819
 *
 * Created on March 13, 2016, 10:35 PM
 */

#include <iostream>
#include <vector>

struct A {
    A() {} 
    
    A(const A &a) {
        ;
    }
    A & operator=( const A & other) {
        ;
    }
    
    A(A && a) = delete ;
    
    A & operator=( A && other) = delete ;
    
    
};

std::vector<A> ret_temp() {
    std::vector<A> a(10, A());
    return a;
}
/*
 * 
 */
int main(int argc, char** argv) {

    std::vector<A> a  = ret_temp() ;
    std::vector<A> b = a;
    std::cout << "completed testing move" << std::endl;
    return 0;
}

