/*
 * File:   order_entity.hpp
 * Author: Vladimir Venediktov vvenedict@gmail.com
 * Copyright (c) 2016-2018 Venediktes Gruppe, LLC
 *
 * Created on April 29, 2015, 10:57 AM
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
*/
 
#ifndef __IPC_DATA_ORDER_ENTITY_HPP__
#define __IPC_DATA_ORDER_ENTITY_HPP__
 
#include "interactive.hpp"
#include <string>
#include <sstream>
#include <boost/tuple/tuple.hpp>
#include <boost/interprocess/containers/string.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
  
namespace ipc { namespace data {
    
    template < typename Alloc>
    struct order_entity
    {
        using char_string =  boost::interprocess::basic_string<char, std::char_traits<char>, Alloc> ;
      
        //for tagging in multi_index_container
        struct account_ticker_tag {}; // search on account+ticker or account
        struct status_account_tag {}; // search on status+account or status
        struct order_tag {}; //unique index
       
        order_entity( const Alloc & a ) :
        _allocator(a),
        account(a),
        ticker(a),
        order_id(),
        order_status(interactive::OrderStatus::CREATED),// this is for search of orders by status
        blob(a)
        {} //ctor END
       
        Alloc _allocator ;
        char_string account;
        char_string ticker;
        long order_id;
        interactive::OrderStatus order_status;
        char_string blob;
 
        template<typename Serializable>
        void store(const Serializable  &data)  {       
            std::stringstream ss;
            boost::archive::binary_oarchive oarch(ss);
            oarch << data ;
            std::string blob_str = std::move(ss.str()) ;
            blob = char_string(blob_str.data(), blob_str.length(), _allocator) ;
            //Store keys
            account  = char_string(data.account.data(), data.account.size(), _allocator);
            ticker   = char_string(data.ticker.data(), data.ticker.size(), _allocator) ;
            order_id = data.order_id;
        }
        template<typename Serializable>
        static std::size_t size(const Serializable &data) {
            std::stringstream ss;
            boost::archive::binary_oarchive oarch(ss);
            oarch << data ;
            return sizeof(data._allocator)       +
                   sizeof(data.account)          +
                   sizeof(data.ticker)           +
                   sizeof(data.order_id)         +
                   sizeof(data.order_status)     +
                   ss.str().size() ;
        }
        template<typename Serializable>
        void retrieve(Serializable  &data) const {           
            std::stringstream ss (std::string(blob.data(),blob.length()));
            boost::archive::binary_iarchive iarch(ss);
            iarch >> data;
        }
        //needed for ability to update after matching by calling index.modify(itr,entry)
        void operator()(order_entity &entry) const {
            entry.account=account;
            entry.ticker=ticker;
            entry.order_id=order_id;
            entry.order_status=order_status;
            entry.blob=blob;
        }
    } ;
   
 
 
template<typename Alloc>
using order_container =
boost::multi_index_container<
    order_entity<Alloc>,
    boost::multi_index::indexed_by<
        boost::multi_index::ordered_unique<
            boost::multi_index::tag<typename order_entity<Alloc>::order_tag>,
                BOOST_MULTI_INDEX_MEMBER(order_entity<Alloc>,long,order_id)
        >,
        boost::multi_index::ordered_non_unique<
            boost::multi_index::tag<typename order_entity<Alloc>::account_ticker_tag>,
            boost::multi_index::composite_key<
                order_entity<Alloc>,
                BOOST_MULTI_INDEX_MEMBER(order_entity<Alloc>,typename order_entity<Alloc>::char_string,account),
                BOOST_MULTI_INDEX_MEMBER(order_entity<Alloc>,typename order_entity<Alloc>::char_string,ticker)
            >
        >,
        boost::multi_index::ordered_non_unique<
            boost::multi_index::tag<typename order_entity<Alloc>::status_account_tag>,
            boost::multi_index::composite_key<
                order_entity<Alloc>,
                BOOST_MULTI_INDEX_MEMBER(order_entity<Alloc>,interactive::OrderStatus,order_status),
                BOOST_MULTI_INDEX_MEMBER(order_entity<Alloc>,typename order_entity<Alloc>::char_string,account)
            >
        >
    >,
    boost::interprocess::allocator<order_entity<Alloc>,typename Alloc::segment_manager>
> ;
  
    
}}
 
#endif     /* __IPC_DATA_ORDER_ENTITY_HPP__  */







