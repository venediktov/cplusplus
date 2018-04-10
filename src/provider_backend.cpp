/* 
 * File:   provider_manager_test.cpp
 * Author: vladimir venediktov
 *
 * Created on March 12, 2017, 10:22 PM
 * 
 *  General idea is to map CRUD commands to functions
 * 
 *  campaign_budget { campaign_id , campaign_budget }
 *  PUT  /v1/campaign/budget/id/123
 *  PUT  /v1/campaign/budget/id/456
 *  GET  /v1/campaign/budget/ -> {[{id:123}, {id:456}]}
 *  GET  /v1/campaign/budget/id/123 -> {{id:123}}
 *  POST /v1/campaign/budget/id/123
 *  DEL  /v1/campaign/budget/id/123
 * 
 *  cmapign_site { campaign_id , campaign_site} 
 *  GET /v1/campaign/site/id/123 
 *  GET /v1/campaign/site/name/some-name
 *
 *  campaign_geo  { campaign_id , campaign_city , campaign_country }
 *  GET /v1/campaign/geo/id/123
 *  GET /v1/campaign/geo/name/some-name
 *  GET /v1/campaign/os/id/123
 *  GET /v1/campaign/os/name/some-name
 * 
 *  GET /v1/campaign/123  ->  campaign :
 *                            {
 *                             "id" : "123"
 *                              campaign_budget :
 *                              {
 *                                 "budget":1000000,
 *                                 "metric" : { id:1 , name:"CPM" , "value":300000},
 *                                 "id":123,
 *                                 "spent":1000
 *                              },
 *                              site :
 *                              {
 *                                "id":"123", 
 *                                "name" : null
 *                              },
 *                              os : {
 *                                "id":123, 
 *                                "name":null
 *                              }
 *                           }
 *
 *
 *  GET /v1/campaign/budget/123 -> will return budget object associated with this compaign
 * 
 *  How it can be compactly achieved utilizing our CRUD regexes 
 *  We know it's possible to retrieve data by unique key or composite key from the data store
 *  to achieve that \\d+ requires 1 or more digits , but \\d* means 0 or more , so the regex group can be empty 
 *  
 *  then we need to figure out in those mapping function keys and type of keys for all 
 *  other type of campaign structures not only budget
 *  for now just budget cache by campaign_id will be coded but ideally it should be one single map
 *  of CRUD handlers to cache functions
 * 
 *  std::map<std::string, std::function<void(CampaignBudgets&,uint32_t)>> read_commands = {
 *       {"id"   , [&cache](auto &data, auto id){cache.retrieve(data,id);}},
 *       {"site" , [&cache](auto &data, auto site){cache.retrieve(data,site);}},
 *       {"geo"  , [&cache](auto &data, auto geo){cache.retrieve(data,geo);}},
 *       {"os"   , [&cache](auto &data, auto os){cache.retrieve(data,os);}}
 *   };
 */

#include <boost/log/trivial.hpp>
#include <boost/program_options.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/optional.hpp>
#include "CRUD/service/server.hpp"
#include "CRUD/handlers/crud_dispatcher.hpp"
#include "DSL/campaign_dsl.hpp"
#include "config.hpp"
#include "campaign_cache.hpp"
#include "serialization.hpp"
#include "campaign_budget_mapper.hpp"
#include "rtb/core/core.hpp"
#include "rtb/core/banker.hpp"

extern void init_framework_logging(const std::string &) ;


int main(int argc, char *argv[]) {
    using namespace vanilla;
    using restful_dispatcher_t =  http::crud::crud_dispatcher<http::server::request, http::server::reply> ;
    using CampaignCacheType  = CampaignCache<CampaignManagerConfig>;
    using CampaignBudgets = typename CampaignCacheType::DataCollection;
    using CampaignBudgetMapper = DSL::campaign_budget_mapper<>;
    
    CampaignManagerConfig config([](campaign_manager_config_data &d, boost::program_options::options_description &desc){
        desc.add_options()
            ("provider-manager.log", boost::program_options::value<std::string>(&d.log_file_name), "provider_manager_test log file name log")
            ("provider-manager.host", "provider_manager_test Host")
            ("provider-manager.port", "provider_manager_test Port")
            ("provider-manager.root", "provider_manager_test Root")
            ("provider-manager.ipc_name", boost::program_options::value<std::string>(&d.ipc_name),"provider_manager_test IPC name")
            ("provider-manager.budget_source", boost::program_options::value<std::string>(&d.campaign_budget_source)->default_value("data/campaign_budget"),"campaign_budget source file name")
        ;
    });
    
    try {
        config.parse(argc, argv);
    }
    catch(std::exception const& e) {
        LOG(error) << e.what();
        return 0;
    }
    LOG(debug) << config;
    init_framework_logging(config.data().log_file_name);

    //CampaignCacheType  cache(config);
    //core::Banker<BudgetManager> banker;
    try {
    //    cache.load();
    }
    catch(std::exception const& e) {
        LOG(error) << e.what(); // It still can work without preloaded campaign budgets
    }

/****** 
    std::map<std::string, std::function<void(const CampaignBudget&,uint32_t)>> create_commands = {
        {"budget/id/" , [&cache](auto cb, auto id){cache.insert(cb,id);}}
    };
    std::map<std::string, std::function<void(CampaignBudgets&,uint32_t)>> read_commands = {
        {"budget/id/" , [&cache](auto &data, auto id){cache.retrieve(data,id);}},
        {"budget" ,   [&cache](auto &data, auto id){cache.retrieve(data);}}
    };
    std::map<std::string, std::function<void(const CampaignBudget&,uint32_t)>> update_commands = {
        {"budget/id/" , [&cache](auto cb, auto id){cache.update(cb,id);}}
    };
    std::map<std::string, std::function<void(uint32_t)>> delete_commands = {
        {"budget/id/" , [&cache](auto id){cache.remove(id);}}
    };

****/

    //initialize and setup CRUD dispatcher
    restful_dispatcher_t dispatcher(config.get("provider-manager.root")) ;
    dispatcher.crud_match(boost::regex("/provider/(\\w*)"))
              .put([&](http::server::reply & r, const http::crud::crud_match<boost::cmatch> & match) {
              LOG(info) << "Create received cache update event url=" << match[0];
                try {
                    LOG(info) << "received=" << match[1] << " " << match[1];
                } catch (std::exception const& e) {
                    LOG(error) << e.what();
                }
              })
              .post([&](http::server::reply & r, const http::crud::crud_match<boost::cmatch> & match) {
                LOG(info) << "Update received event url=" << match[0];
                try {
                    LOG(info) << "received=" << match[1] << " " << match[1];
                } catch (std::exception const& e) {
                    LOG(error) << e.what();
                }
              })
              .del([&](http::server::reply & r, const http::crud::crud_match<boost::cmatch> & match) {
                LOG(info) << "Delete received event url=" << match[0];
                try {
                    LOG(info) << "received=" << match[1] << " " << match[1];
                } catch (std::exception const& e) {
                    LOG(error) << e.what();
                }
    });
    dispatcher.crud_match(boost::regex("/provider/(\\w*)"))
        .get([&](http::server::reply & r, const http::crud::crud_match<boost::cmatch> & match) {
              LOG(info) << "Read received event url=" << match[0];
              try {
                  LOG(info) << "received=" << match[1] ;
                  r << "[{"ProviderEmail":"vlad1@email.com","ProviderUrl":"http://email.com","AcceptedMedia":"native,video"},{"ProviderEmail":"vlad2@email.com","ProviderUrl":"http://email2.com","AcceptedMedia":"native,video"}]"
                  r << http::server::reply::flush("json");
                  //for testing only , works with POST only and AngularJS still sends OPTIONS with DELETE, PUT
                  r.headers.push_back(http::server::header("Access-Control-Allow-Origin", "*"));
              } catch (std::exception const& e) {
                  LOG(error) << e.what();
              }
    });

    auto host = config.get("provider-manager.host");
    auto port = config.get("provider-manager.port");
    http::server::server<restful_dispatcher_t> server(host,port,dispatcher);
    server.run();
}

