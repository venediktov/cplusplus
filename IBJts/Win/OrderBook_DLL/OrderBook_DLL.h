#pragma once

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

#if defined ( _WIN32 )
#ifdef ORDERBOOK_DLL_EXPORTS
#define INTERACTIVE_DLL_EXPORTS __declspec(dllexport)
#else
#define INTERACTIVE_DLL_EXPORTS //__declspec(dllimport)
#endif
#else
#define INTERACTIVE_DLL_EXPORTS 
#endif

#include "interactive.hpp"
#include <memory>
#include <string>
#include <vector>

namespace interactive {

	class INTERACTIVE_DLL_EXPORTS OrderBookCache {
	public:
		using collection_t = std::vector<std::shared_ptr<OrderContract>>;
		OrderBookCache(const std::string &name) : cache_name_(name) {}
		std::vector<std::shared_ptr<OrderContract>> GetTest() {
			auto p = std::make_shared<OrderContract>(OrderContract());
			std::vector<std::shared_ptr<OrderContract>> v{ p };
			return v;
		}
		collection_t  GetEntries();
		collection_t  GetEntriesByAccount(const std::string &account);
	private:
		std::string cache_name_;
	};
}