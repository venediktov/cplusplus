/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   otlv4_pool.h
 * Author: vvenedik
 *
 * Created on March 18, 2016, 10:05 AM
 */

#ifndef __OTLV4POOL_H_
#define __OTLV4POOL_H_

#include <otlv4.h>
#include <utility>
#include <string>
#include <memory>
#include <functional>

const int otl_error_code_45 = 32046;
#define otl_error_msg_45 "pool main_policy : OCI initialization failed"

namespace otlv4_pools {
    
struct basic_pool_impl 
{
    basic_pool_impl(const std::string & tnsname, bool is_threaded) OTL_THROWS_OTL_EXCEPTION : 
        otl_impl(), tnsname(tnsname), envhp(), errhp()
    {
        if ( !init(is_threaded)) {
            throw otl_exception(otl_error_msg_45, otl_error_code_45);
        }
    }
    OCIEnv* get_envhp() { return envhp; }

protected :
    otl_conn otl_impl ;
    std::string tnsname;
    OCIEnv *envhp;     // OCI environment handle
    OCIError *errhp;   // OCI Error handle
         
    bool init(bool threaded_mode) {
        int is_ok = otl_impl.server_attach(tnsname.c_str()) ; 
        envhp  = otl_impl.get_envhp();
        errhp  = otl_impl.get_errhp();
        return is_ok ;
    }
    
    template<typename size_type, typename Func>
    std::string open(Func & CreatePool, const std::string &user_name, const std::string &passwd, int min_con, int max_con, int incr)  {
       unsigned char *pool_name;
       size_type pool_name_length;
       CreatePool (
            envhp,
            errhp,
            //poolhp,
            &pool_name,
            &pool_name_length,
            OTL_RCAST(OraText*, OTL_CCAST(char*, tnsname.data())),
            tnsname.size(),
            min_con,
            max_con,
            incr,
            OTL_RCAST(OraText*, OTL_CCAST(char*, user_name.data())),
            user_name.size(),
            OTL_RCAST(OraText*, OTL_CCAST(char*, passwd.data())),
            passwd.size()
            //mode
        ) ;
        return std::string(OTL_RCAST(char*, pool_name), pool_name_length) ;
    }
};


struct stateless_pool_impl : protected basic_pool_impl 
{
    using Base = basic_pool_impl;
    stateless_pool_impl(const std::string &tnsname, bool is_threaded) : Base(tnsname,is_threaded), poolhp() {
        //Allocate SPool poolhp
        int status = OCIHandleAlloc(OTL_RCAST(dvoid *, Base::envhp), 
                                OTL_RCAST(dvoid **, &poolhp), 
                                OCI_HTYPE_SPOOL, 0, nullptr);
    }
    std::string open(const std::string &user_name, const std::string &passwd, int min_con, int max_con, int incr )  {
        using namespace std::placeholders;
        auto create_pool = std::bind(OCISessionPoolCreate,_1,_2,poolhp,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,OCI_SPC_HOMOGENEOUS) ;
        std::string name = Base::open<ub4>(create_pool,user_name,passwd,min_con,max_con,incr) ;
        return name;
    }
    void get(const std::string &pool_name, OCISvcCtx *& svchp) {
        OCISessionGet(Base::envhp, Base::errhp, &svchp, NULL, 
                       OTL_RCAST(OraText*, OTL_CCAST(char*, pool_name.data())), pool_name.size(),
                       NULL, 0, NULL, NULL, NULL, OCI_SESSGET_SPOOL ) ;
    }
    void release(OCISvcCtx * svchp) {
        OCISessionRelease(svchp, Base::errhp, NULL, 0, OCI_DEFAULT) ;   
    }
    void destroy() {
        OCISessionPoolDestroy(poolhp, Base::errhp, OCI_DEFAULT) ;
    }
    OCIEnv* get_envhp() { 
        return Base::envhp; 
    }
protected:
     OCISPool   *poolhp; // OCI SPool handle
};


struct statefull_pool_impl : protected basic_pool_impl 
{
    using Base = basic_pool_impl;
    statefull_pool_impl(const std::string &tnsname, bool is_threaded) : Base(tnsname,is_threaded), poolhp() {
        //Allocate CPool poolhp
        int status = OCIHandleAlloc(OTL_RCAST(dvoid *, Base::envhp), 
                                OTL_RCAST(dvoid **, &poolhp), 
                                OCI_HTYPE_CPOOL, 0, nullptr);
    }
    std::string open(const std::string &user_name, const std::string &passwd, int min_con, int max_con, int incr )  {
        using namespace std::placeholders;
        auto create_pool = std::bind(OCIConnectionPoolCreate,_1,_2,poolhp,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,OCI_DEFAULT) ;
        std::string name = Base::open<sb4>(create_pool,user_name,passwd,min_con,max_con,incr) ;
        return name;
    }
    void get(const std::string &pool_name, OCISvcCtx *& svchp) {
        OCISessionGet(Base::envhp, Base::errhp, &svchp, NULL, 
                       OTL_RCAST(OraText*, OTL_CCAST(char*, pool_name.data())), pool_name.size(),
                       NULL, 0, NULL, NULL, NULL, OCI_SESSGET_CPOOL ) ;
    }
    void release(OCISvcCtx * svchp) {
        OCISessionRelease(svchp, Base::errhp, NULL, 0, OCI_DEFAULT) ;   
    }
    void destroy() {
          OCIConnectionPoolDestroy(poolhp, Base::errhp, OCI_DEFAULT) ;
    }
    OCIEnv* get_envhp() { 
        return Base::envhp; 
    }
protected:
     OCICPool   *poolhp; // OCI CPool handle
};

template<typename PoolImpl>
struct otl_connection_pool {
    using otl_connect_shared_ptr = std::shared_ptr<otl_connect> ;
    using otl_connect_unique_ptr = std::unique_ptr<otl_connect, std::function<void(otl_connect*)>> ;
    otl_connection_pool(const std::string &tnsname, bool is_threaded=true) : pool_impl(tnsname, is_threaded), db_name()
    {}

    virtual ~otl_connection_pool() {
        pool_impl.destroy();  
    }
    
    void open(const std::string &user_name, const std::string &passwd, int min_con, int max_con, int incr )  {
        db_name = pool_impl.open(user_name, passwd, min_con, max_con, incr);
    }
    
    otl_connect_shared_ptr get_shared(bool auto_commit=false) {
        OCISvcCtx * svchp ;
        pool_impl.get(db_name,svchp) ;
        otl_connect_shared_ptr  ptr = otl_connect_shared_ptr(new otl_connect(), [this,svchp](otl_connect *con) {
            pool_impl.release(svchp) ;
            delete con;
        }) ;
        
        ptr->rlogon(pool_impl.get_envhp(), svchp) ;
        auto_commit ? ptr->auto_commit_on() : ptr->auto_commit_off() ;
        return ptr;
    }
    
    otl_connect_unique_ptr get_unique(bool auto_commit=false) {
        OCISvcCtx * svchp ;
        pool_impl.get(db_name,svchp) ;
        otl_connect_unique_ptr ptr = otl_connect_unique_ptr(new otl_connect(), [this,svchp](otl_connect *con) {
            pool_impl.release(svchp) ;
            delete con;
        }) ;
        
        ptr->rlogon(pool_impl.get_envhp(), svchp) ;
        auto_commit ? ptr->auto_commit_on() : ptr->auto_commit_off() ;
        return ptr; // ptr is temporary, RVO calls std::move(ptr) automatically
    }
    
private:
    PoolImpl pool_impl;
    std::string db_name;
};
   
} //namespace 

#endif /* OTLV4POOL_H */

