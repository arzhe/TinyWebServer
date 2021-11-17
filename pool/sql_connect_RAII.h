#ifndef WEB_SERVER_POOL_SQL_CONNNECT_RAII_H
#define WEB_SERVER_POOL_SQL_CONNNECT_RAII_H

#include "./sql_connect_pool.h"

class SqlConnectRAII {
private:
    MYSQL* sql;
    SqlConnectPool* connect_pool;

public:
    SqlConnectRAII(MYSQL** sql, SqlConnectPool* connect_pool) {
        assert(connect_pool);
        *sql = connect_pool->GetConnect();
        this->sql = *sql;
        this->connect_pool = connect_pool;
    }

    ~SqlConnectRAII() {
        if(this->sql) {
            this->connect_pool->FreeConnect(this->sql);
        }
    }
};

#endif
