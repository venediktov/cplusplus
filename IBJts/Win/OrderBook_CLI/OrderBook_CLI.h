// OrderBook_CLI.h

#pragma once
#pragma managed(push, off)
#include <interactive.hpp>
#include <string>
#pragma managed(pop)
#include <msclr\marshal_cppstd.h>


using namespace System;
using namespace msclr::interop;

namespace OrderBook_CLI {

	public ref class Account {
	public:
		Account(String ^a) {
			account = a;
		}
		String^ account;
	};

	public ref class OrderContract {
	public:
		OrderContract(const interactive::OrderContract& oc) {
			account = marshal_as<String^>(oc.account);
			order_id = oc.order_id;
			ticker = marshal_as<String^>(oc.ticker);
			cmd = (int)oc.cmd;

		}

		String^ account;
		long order_id;
		String^ ticker;
		int cmd;
	};

	public ref class OrderBookCache
	{
	public:
		OrderBookCache(System::String ^ name) : ipc_order_book_(new Cache(marshal_as<std::string>(name)))
	    {}
		~OrderBookCache() { delete ipc_order_book_;  }
		
		array<OrderBook_CLI::OrderContract^>^ GetOrders() 
		{
			auto entries = ipc_order_book_->GetEntries();
			array<OrderBook_CLI::OrderContract^>^ all_orders = gcnew array<OrderBook_CLI::OrderContract^>(entries.size());
			Console::WriteLine("Retrieved all cache entries:");
			int i = 0;
			for (auto oc : entries) {
				Console::WriteLine("ORDER: {0} {1} {3} {4} ", 
					marshal_as<String^>(oc->account), 
					oc->order_id,
				    marshal_as<String^>(oc->ticker),
					(int)oc->cmd
				);
				all_orders[i++] = gcnew OrderBook_CLI::OrderContract(*oc);
			}
			return all_orders;
		}


		
		array<OrderBook_CLI::OrderContract^>^ GetOrders(OrderBook_CLI::Account ^ a)
		{
			auto entries = ipc_order_book_->GetEntriesByAccount(marshal_as<std::string>(%*a->account));
			array<OrderBook_CLI::OrderContract^>^ all_orders = gcnew array<OrderBook_CLI::OrderContract^>(entries.size());
			Console::WriteLine("Retrieved all cache entries:");
			int i = 0;
			for (auto oc : entries) {
				Console::WriteLine("ORDER: {0} {1} {3} {4}",
					marshal_as<String^>(oc->account),
					oc->order_id,
					marshal_as<String^>(oc->ticker),
					(int)oc->cmd
				);
				all_orders[i++] = gcnew OrderBook_CLI::OrderContract(*oc);
			}
			return all_orders;
		}
		

	internal:
		using Cache = interactive::OrderBookCache;
		Cache *ipc_order_book_;
	};
}
