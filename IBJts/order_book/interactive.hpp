#ifndef __INTERACTIVE_HPP__
#define __INTERACTIVE_HPP__

#if defined ( _WIN32 )
    #ifdef ORDERBOOK_DLL_EXPORTS
    #define INTERACTIVE_DLL_EXPORTS __declspec(dllexport)
    #else
    #define INTERACTIVE_DLL_EXPORTS //__declspec(dllimport)
    #endif
#else
    #define INTERACTIVE_DLL_EXPORTS 
#endif

#include <Contract.h>
#include <Order.h>
#include <boost/serialization/serialization.hpp>


namespace boost {
	namespace serialization {

		template<class Archive>
		void serialize(Archive & ar, Order & order, const unsigned int version)
		{
			ar & order.account;
			ar & order.action;
			ar & order.totalQuantity;
			ar & order.orderType;
			ar & order.lmtPrice;
			ar & order.clientId;
		}

		template<class Archive>
		void serialize(Archive & ar, Contract & contract, const unsigned int version)
		{
			ar & contract.symbol;
			ar & contract.secType;
			ar & contract.exchange;
			ar & contract.currency;
		}


	} // namespace serialization
} // namespace boost



namespace interactive {

	enum class OrderInstruction : std::int8_t {
		PLACE = 1,
		CANCEL = 0,
		UNDEFINED = -1
	};

	enum class OrderStatus : std::int8_t {
		CREATED = 0,
		SUBMITTED = 1,
		PENDING = 2,
		FILLED = 3,
		PARTIAL_FILL = 4,
		ERROR_STATUS = 5
	};

	struct INTERACTIVE_DLL_EXPORTS OrderResponse {
		IBString status;
		int filled;
		int remaining;
		double avgFillPrice;
		int permId;
		int parentId;
		double lastFillPrice;
		int clientId;
		IBString whyHeld;
	};

	struct INTERACTIVE_DLL_EXPORTS OrderContract
	{
		friend class boost::serialization::access;

		OrderContract() {}

		OrderContract(const Order &o, const Contract &c) : OrderContract() {
			order = o;
			contract = c;
			account = order.account;
			ticker = contract.symbol;
			order_id = order.orderId;
		}

		void place() {
			cmd = OrderInstruction::PLACE;
		}
		void cancel() {
			cmd = OrderInstruction::CANCEL;
		}
		void add_response(const OrderResponse & r) {
			oresp = r;
		}

		template<class Archive>
		void serialize(Archive & ar, const unsigned int version = 0)
		{
			ar & cmd;
			ar & order;
			ar & contract;
			//below for de-serialization from Archive
			account = order.account;
			ticker = contract.symbol;
			order_id = order.orderId;
		}

		OrderInstruction cmd{ OrderInstruction::UNDEFINED };
		Order order{};
		Contract contract{};
		std::string account{};
		std::string ticker{};
		long order_id{};
		OrderResponse oresp{};
	};
	
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
#endif