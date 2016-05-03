/* 
 * File:   orderclient.hpp
 * Author: vlad1819
 *
 * Created on May 1, 2016, 1:34 PM
 */

#ifndef __INTERACTIVE_ORDER_BOOK_HPP__
#define	__INTERACTIVE_ORDER_BOOK_HPP__

#include "order_entity.hpp"
#include "entity_cache.hpp"
#include <EWrapper.h>
#include <EPosixClientSocket.h>
#include <memory>
#include <future>
#include <string>
#include <list>
#include <iostream>
#include <boost/tuple/tuple.hpp>

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

struct OrderContract 
{
    friend class boost::serialization::access;
    
    OrderContract() {}
    
    OrderContract(const Order &o , const Contract &c) : OrderContract() {
        order = order;
        contract = contract ; 
        account  = order.account;
        ticker   = contract.symbol;
        order_id = order.orderId ;
    }
    
    void place() { 
        cmd = OrderInstruction::PLACE;
    }
    void cancel() {
        cmd = OrderInstruction::CANCEL;
    }
    
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version=0)
    {
        ar & cmd;
        ar & order;
        ar & contract;
        //below for de-serialization from Archive
        account  = order.account;
        ticker   = contract.symbol;
        order_id = order.orderId ;
    }
    
    OrderInstruction cmd{OrderInstruction::UNDEFINED};
    Order order{} ;
    Contract contract{};
    std::string account{};
    std::string ticker{} ;
    long order_id{} ;    
};  


template<typename Alloc>
class OrderBook : protected EWrapper {
public:
    OrderBook(const std::string &cname, const std::function<OrderContract()> queue) : 
        client_{new EPosixClientSocket(this)}, queue_{queue}, cache_{cname}, next_order_ids_{}
    {}
    OrderBook(const OrderBook& orig) = delete ;
    virtual ~OrderBook() {disconnect();}
    bool connect(const std::string &host, unsigned int port, int client_id = 0) {
         // trying to connect
        std::cout << "OrderClient::connect connecting to " <<  host <<  ":" << port << " client_id=" << client_id << std::endl;
        bool is_success = client_->eConnect( host.c_str(), port, client_id, /* extraAuth */ false);
        if (!is_success) {
             printf( "Cannot connect to %s:%d clientId:%d\n", host.c_str(), port, client_id);
             return is_success ;
        }

        dispatcher_ = std::async(std::launch::async, [this]() {
            while(isConnected()) {
                dispatch_messages();
            }
        });

        printf("Connected to %s:%d clientId:%d\n", host.c_str(), port, client_id);
        return is_success;
    }
    void disconnect() const {
	client_->eDisconnect();
    }
    bool isConnected() const {
	return client_->isConnected();
    }
    //template<typename Contract , typename Order>
    //void place_order(const Contract &contract , const Order &order ) { //const {
    //printf( "Storing Order : %s %ld %s at %f\n", order.action.c_str(), order.totalQuantity, contract.symbol.c_str(), order.lmtPrice);
	//client_->placeOrder( next_order_ids_, contract, order);
    // cache_.insert(OrderContact(order, contract)) ;
    //}
    //void cancel_order(OrderId order_id) {
    //    client_->cancelOrder(order_id);
    //}
    
protected:    
    // events from EWrapper
    void tickPrice(TickerId tickerId, TickType field, double price, int canAutoExecute) {}
    void tickSize(TickerId tickerId, TickType field, int size) {}
    void tickOptionComputation( TickerId tickerId, TickType tickType, double impliedVol, double delta,
            double optPrice, double pvDividend, double gamma, double vega, double theta, double undPrice){}
    void tickGeneric(TickerId tickerId, TickType tickType, double value) {}
    void tickString(TickerId tickerId, TickType tickType, const IBString& value){}
    void tickEFP(TickerId tickerId, TickType tickType, double basisPoints, const IBString& formattedBasisPoints,
            double totalDividends, int holdDays, const IBString& futureExpiry, double dividendImpact, double dividendsToExpiry) {}
    //Order Management events below
    void orderStatus(OrderId orderId, const IBString &status, int filled,
            int remaining, double avgFillPrice, int permId, int parentId,
            double lastFillPrice, int clientId, const IBString& whyHeld) {
        //Implement order_book_cache.update(orderId , order) ;
        std::cout << "TODO:  Order: id" << orderId << ", status=" << status ;
    }
    void openOrder(OrderId orderId, const Contract&, const Order&, const OrderState&) {
        std::cout << "TODO: OrderClient::openOrder " << orderId << std::endl ;
    }
    void openOrderEnd() {}
    void winError(const IBString &str, int lastError) {}
    void connectionClosed() {}
    //Account related messages below
    void updateAccountValue(const IBString& key, const IBString& val,
            const IBString& currency, const IBString& accountName) {}
    void updatePortfolio(const Contract& contract, int position,
            double marketPrice, double marketValue, double averageCost,
            double unrealizedPNL, double realizedPNL, const IBString& accountName) {}
    void updateAccountTime(const IBString& timeStamp) {}
    void accountDownloadEnd(const IBString& accountName) {}
    void nextValidId(OrderId order_id) {
        std::cout << "nextValidId=" << order_id << ", call back from API , forward to queue" << std::endl ;
	next_order_ids_.push_back(order_id);
        //should block here untill order is available from client GUI
        dispatch_order(queue_()) ;
    }
    void contractDetails(int reqId, const ContractDetails& contractDetails) {}
    void bondContractDetails(int reqId, const ContractDetails& contractDetails) {}
    void contractDetailsEnd(int reqId) {}
    void execDetails(int reqId, const Contract& contract, const Execution& execution) {}
    void execDetailsEnd(int reqId) {}
    void error(const int id, const int errorCode, const IBString errorString) {
        std::cerr << "Error id=" << id << " errorCode=" << errorCode << ", msg=" << errorString << std::endl;
	if( id == -1 && errorCode == 1100) // if "Connectivity between IB and TWS has been lost"
		disconnect();
    }
    void updateMktDepth(TickerId id, int position, int operation, int side,
            double price, int size) {}
    void updateMktDepthL2(TickerId id, int position, IBString marketMaker, int operation,
            int side, double price, int size) {}
    void updateNewsBulletin(int msgId, int msgType, const IBString& newsMessage, const IBString& originExch) {}
    void managedAccounts(const IBString& accountsList) {}
    void receiveFA(faDataType pFaDataType, const IBString& cxml) {}
    void historicalData(TickerId reqId, const IBString& date, double open, double high,
            double low, double close, int volume, int barCount, double WAP, int hasGaps) {}
    void scannerParameters(const IBString &xml) {}
    void scannerData(int reqId, int rank, const ContractDetails &contractDetails,
            const IBString &distance, const IBString &benchmark, const IBString &projection,
            const IBString &legsStr) {}
    void scannerDataEnd(int reqId) {}
    void realtimeBar(TickerId reqId, long time, double open, double high, double low, double close,
            long volume, double wap, int count) {}
    void currentTime(long time) {}
    void fundamentalData(TickerId reqId, const IBString& data) {}
    void deltaNeutralValidation(int reqId, const UnderComp& underComp) {}
    void tickSnapshotEnd(int reqId) {}
    void marketDataType(TickerId reqId, int marketDataType) {}
    void commissionReport( const CommissionReport& commissionReport) {}
    void position( const IBString& account, const Contract& contract, int position, double avgCost) {}
    void positionEnd() {}
    void accountSummary( int reqId, const IBString& account, const IBString& tag, const IBString& value, const IBString& curency) {}
    void accountSummaryEnd( int reqId) {}
    void verifyMessageAPI( const IBString& apiData) {}
    void verifyCompleted( bool isSuccessful, const IBString& errorText) {}
    void displayGroupList( int reqId, const IBString& groups) {}
    void displayGroupUpdated( int reqId, const IBString& contractInfo) {}
private:
    void dispatch_messages()  {
        fd_set readSet, writeSet, errorSet;
	struct timeval tval;
	tval.tv_usec = 500000; //TODO: pass it into dispatch_messages
	tval.tv_sec = 0;
	
	if( client_->fd() < 0 ) {
            return;
        }
        
        FD_ZERO( &readSet);
        errorSet = writeSet = readSet;

        FD_SET( client_->fd(), &readSet);

        if( !client_->isOutBufferEmpty())
                FD_SET( client_->fd(), &writeSet);

        FD_SET( client_->fd(), &errorSet);

        int ret = select( client_->fd() + 1, &readSet, &writeSet, &errorSet, &tval);

        if( ret == 0) { // timeout
            return;
        }

        if( ret < 0) {	// error
                disconnect();
                return;
        }

        if( client_->fd() < 0)
                return;

        if( FD_ISSET( client_->fd(), &errorSet)) {
                // error on socket
                client_->onError();
        }

        if( client_->fd() < 0)
                return;

        if( FD_ISSET( client_->fd(), &writeSet)) {
                // socket is ready for writing
                client_->onSend();
        }

        if( client_->fd() < 0)
                return;

        if( FD_ISSET( client_->fd(), &readSet)) {
                // socket is ready for reading
                client_->onReceive();
        }
	
    }
    void dispatch_order(const OrderContract &value) {
        //using Tag = typename ipc::data::order_entity<Alloc>::status_account_tag ;
        if ( value.cmd != OrderInstruction::PLACE || value.cmd != OrderInstruction::CANCEL) {
            printf( "Bad Order : %s %ld %s at %f\n", value.order.action.c_str(), value.order.totalQuantity, value.contract.symbol.c_str(), value.order.lmtPrice);
            return;
        }
        OrderId next_order_id = next_order_ids_.front() ;
        next_order_ids_.pop_front() ;
        //std::vector<OrderContact> orders;
        //if ( !cache_.template retrieve<Tag>(orders,OrderStatus::CREATED) ) {
        //    return;   
        //}
        //const auto &value = orders.at(0) ;
        printf( "Submitting Order %ld: %s %ld %s at %f\n", 
                next_order_id, value.order.action.c_str(), 
                value.order.totalQuantity, 
                value.contract.symbol.c_str(), value.order.lmtPrice
        );
	if ( value.cmd == OrderInstruction::PLACE) {
            client_->placeOrder(next_order_id, value.contract, value.order);
        } else if (value.cmd == OrderInstruction::CANCEL) {
             client_->cancelOrder(value.order_id);
        }
        if ( !cache_.insert(value) ) {
            std::cout  << "failed to insert order in cache:" << std::endl;
        }
        
    }
    //using WireOrder_t = std::tuple<Contract,Order> ;
    std::unique_ptr<EPosixClientSocket> client_;
    std::future<void> dispatcher_ {};
    std::function<OrderContract()> queue_;
    std::list<OrderId> next_order_ids_ {};
    datacache::entity_cache<Alloc, ipc::data::order_container>  cache_ ;
    time_t sleep_deadline;
};

} //namespace
#endif	/* ORDERCLIENT_HPP */

