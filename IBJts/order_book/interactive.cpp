
#include "interactive.hpp"
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

