// OrderBook_DLL.cpp : Defines the exported functions for the DLL application.
//
/*
* File:   interactive.hpp
* Author: Vladimir Venediktov
* Copyright (c) 2016-2018 Venediktes Gruppe, LLC
*
* Created on May 27, 2016, 7:34 PM
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
*
*/

#include "stdafx.h"


#include "OrderBook_DLL.h"
#include "memory_types.hpp"
#include "entity_cache.hpp"
#include "order_entity.hpp"

using mpclmi::ipc::Shared;
using ipc::data::order_entity;

namespace interactive {

	OrderBookCache::collection_t INTERACTIVE_DLL_EXPORTS
		OrderBookCache::GetEntries() {
		std::vector<std::shared_ptr < OrderContract>> entries;
		using Cache = datacache::entity_cache<Shared, ipc::data::order_container>;
		Cache ipc_order_book(cache_name_);
		ipc_order_book.retrieve(entries);
		return entries;
	}

	OrderBookCache::collection_t INTERACTIVE_DLL_EXPORTS
		OrderBookCache::GetEntriesByAccount(const std::string &account) {
		using Cache = datacache::entity_cache<Shared, ipc::data::order_container>;
		using Tag = typename ipc::data::order_entity<Cache::char_allocator>::account_ticker_tag;
		std::vector<std::shared_ptr < OrderContract>> entries;
		Cache ipc_order_book(cache_name_);
		auto acct_key = ipc_order_book.create_ipc_key(account);
		ipc_order_book.retrieve<Tag>(entries, acct_key);
		return entries;
	}

}


