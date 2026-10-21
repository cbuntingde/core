/*
 * Copyright (C) 2005-2011 MaNGOS <http://getmangos.com/>
 * Copyright (C) 2009-2011 MaNGOSZero <https://github.com/mangos/zero>
 * Copyright (C) 2011-2016 Nostalrius <https://nostalrius.org>
 * Copyright (C) 2016-2017 Elysium Project <https://github.com/elysium-project>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <errmsg.h>
#include <mysqld_error.h>
#include <type_traits>
#include "Log.h"
#include "Errors.h"
#include "Util.h"
#include "Policies/SingletonImp.h"
#include "Platform/Define.h"
#include "DatabaseEnv.h"
#include "Timer.h"

size_t DatabaseMysql::db_count = 0;

void DatabaseMysql::ThreadStart()
{
    mysql_thread_init();
}

void DatabaseMysql::ThreadEnd()
{
    mysql_thread_end();
}

DatabaseMysql::DatabaseMysql()
{
    // before first connection
    if (db_count++ == 0)
    {
        // Mysql Library Init
        mysql_library_init(-1, nullptr, nullptr);

        if (!mysql_thread_safe())
        {
            sLog.Out(LOG_BASIC, LOG_LVL_ERROR, "FATAL ERROR: Used MySQL library isn't thread-safe.");
            Log::WaitBeforeContinueIfNeed();
            exit(1);
        }
    }
}

DatabaseMysql::~DatabaseMysql()
{
    StopServer();

    //Free Mysql library pointers for last ~DB
    if(--db_count == 0)
        mysql_library_end();
}

SqlConnection* DatabaseMysql::CreateConnection()
{
    return new MySQLConnection(*this);
}

MySQLConnection::~MySQLConnection()
{
    FreePreparedStatements();
    if (mMysql)
        mysql_close(mMysql);
}

bool MySQLConnection::OpenConnection(bool /*reconnect*/)
{
    MYSQL* mysqlInit = mysql_init(nullptr);
    if (!mysqlInit)
    {
        sLog.Out(LOG_BASIC, LOG_LVL_ERROR, "Could not initialize Mysql connection");
        return false;
    }

    mysql_options(mysqlInit, MYSQL_SET_CHARSET_NAME, "utf8");
    // The client must never reconnect behind the transaction state machine.
    bool reconnectAutomatically = false;
    if (mysql_options(mysqlInit, MYSQL_OPT_RECONNECT, &reconnectAutomatically))
    {
        mysql_close(mysqlInit);
        return false;
    }

    if (m_use_socket)                                           // socket use option (Unix/Linux)
    {
        unsigned int opt = MYSQL_PROTOCOL_SOCKET;
        mysql_options(mysqlInit, MYSQL_OPT_PROTOCOL, (char const*)&opt);
    }

    mMysql = mysql_real_connect(mysqlInit, m_host.c_str(), m_user.c_str(),
        m_password.c_str(), m_database.c_str(), m_port, nullptr, 0);

    if (mMysql)
    {
        sLog.Out(LOG_BASIC, LOG_LVL_DETAIL, "Connected to MySQL database at %s",
            m_host.c_str());
        sLog.Out(LOG_BASIC, LOG_LVL_DETAIL, "MySQL client library: %s", mysql_get_client_info());
        sLog.Out(LOG_BASIC, LOG_LVL_DETAIL, "MySQL server ver: %s ", mysql_get_server_info(mMysql));

        /*----------SET AUTOCOMMIT ON---------*/
        // It seems mysql 5.0.x have enabled this feature
        // by default. In crash case you can lose data!!!
        // So better to turn this off
        // ---
        // This is wrong since mangos use transactions,
        // autocommit is turned of during it.
        // Setting it to on makes atomic updates work
        // ---
        // LEAVE 'AUTOCOMMIT' MODE ALWAYS ENABLED!!!
        // W/O IT EVEN 'SELECT' QUERIES WOULD REQUIRE TO BE WRAPPED INTO 'START TRANSACTION'<>'COMMIT' CLAUSES!!!
        if (!mysql_autocommit(mMysql, 1))
            sLog.Out(LOG_BASIC, LOG_LVL_DETAIL, "AUTOCOMMIT SUCCESSFULLY SET TO 1");
        else
        {
            sLog.Out(LOG_BASIC, LOG_LVL_ERROR, "Could not enable database autocommit: %s", mysql_error(mMysql));
            mysql_close(mMysql);
            mMysql = nullptr;
            return false;
        }
        /*-------------------------------------*/

        // set connection properties to UTF8 to properly handle locales for different
        // server configs - core sends data in UTF8, so MySQL must expect UTF8 too
        if (mysql_set_character_set(mMysql, "utf8"))
        {
            sLog.Out(LOG_BASIC, LOG_LVL_ERROR, "Could not set database character set: %s", mysql_error(mMysql));
            mysql_close(mMysql);
            mMysql = nullptr;
            return false;
        }

        return true;
    }
    else
    {
        sLog.Out(LOG_BASIC, LOG_LVL_ERROR, "Could not connect to MySQL database at %s: %s\n",
            m_host.c_str(), mysql_error(mysqlInit));
        mysql_close(mysqlInit);
        return false;
    }
}

bool MySQLConnection::Reconnect()
{
    sLog.Out(LOG_BASIC, LOG_LVL_MINIMAL, "Reconnection attempt to database %s (on %s)", m_database.c_str(), m_host.c_str());

    if (m_inTransaction)
        return false;
    // Statements must be destroyed while their original MYSQL handle is alive.
    FreePreparedStatements();
    if (mMysql)
        mysql_close(mMysql);
    mMysql = nullptr;
    if (OpenConnection(true))
    {
        m_needsReconnect = false;
        sLog.Out(LOG_BASIC, LOG_LVL_MINIMAL, "Successfully reconnected to %s @%s:%u.",
            m_database.c_str(), m_host.c_str(), m_port);

        return true;
    }

    return false; // Failed to reconnect
}

bool MySQLConnection::EnsureConnection()
{
    m_lastError = 0;
    return (mMysql && !m_needsReconnect) || (!m_inTransaction && Reconnect());
}

bool MySQLConnection::CanRetryTransaction() const
{
    // MySQL guarantees the complete transaction was rolled back for a deadlock.
    // Network errors, timeouts and failed COMMIT have no such guarantee.
    return m_lastError == ER_LOCK_DEADLOCK;
}

bool MySQLConnection::HandleMySQLError(uint32 errNo)
{
    m_lastError = errNo;
    if (!errNo)
        return false;
    switch (errNo)
    {
        case CR_SERVER_GONE_ERROR:
        case CR_SERVER_LOST:
            #if !(MARIADB_VERSION_ID >= 100200)
        case CR_INVALID_CONN_HANDLE:
            #endif
        case CR_SERVER_LOST_EXTENDED:
        {
            m_needsReconnect = true;
            return false;
        }

        case ER_LOCK_DEADLOCK:
        case ER_LOCK_WAIT_TIMEOUT:
            return false;
        // Query related errors - skip query
        case ER_WRONG_VALUE_COUNT:
        case ER_DUP_ENTRY:
            return false;

        // Outdated table or database structure - terminate core
        case ER_BAD_FIELD_ERROR:
        case ER_NO_SUCH_TABLE:
            sLog.Out(LOG_DBERROR, LOG_LVL_MINIMAL, "Your database structure is not up to date. Please make sure you have executed all the queries in the sql/updates folders.");
            ASSERT(false);
            return false;
        case ER_PARSE_ERROR:
            sLog.Out(LOG_DBERROR, LOG_LVL_MINIMAL, "Error while parsing SQL. Core fix required.");
            ASSERT(false);
            return false;
        default:
            sLog.Out(LOG_DBERROR, LOG_LVL_MINIMAL, "Unhandled MySQL errno %u. Unexpected behaviour possible.", errNo);
            ASSERT(false);
            return false;
    }
}

bool MySQLConnection::_Query(std::string const& sql, MYSQL_RES** pResult, MYSQL_FIELD** pFields, uint64* pRowCount, uint32* pFieldCount)
{
    if (!EnsureConnection())
        return false;

    uint32 _s = WorldTimer::getMSTime();

    if (mysql_query(mMysql, sql.c_str()))
    {
        uint32 lErrno = mysql_errno(mMysql);

        sLog.Out(LOG_DBERROR, LOG_LVL_MINIMAL, "SQL: %s", sql.c_str());
        sLog.Out(LOG_DBERROR, LOG_LVL_MINIMAL, "[%u] %s", lErrno, mysql_error(mMysql));

        HandleMySQLError(lErrno);

        return false;
    }
    else
    {
        DEBUG_FILTER_LOG(LOG_FILTER_SQL_TEXT, "[%u ms] SQL: %s", WorldTimer::getMSTimeDiff(_s,WorldTimer::getMSTime()), sql.c_str());
    }

    *pResult = mysql_store_result(mMysql);
    *pRowCount = mysql_affected_rows(mMysql);
    *pFieldCount = mysql_field_count(mMysql);

    if (!*pResult)
        return false;

    if (!*pRowCount)
    {
        mysql_free_result(*pResult);
        return false;
    }

    *pFields = mysql_fetch_fields(*pResult);
    return true;
}

std::unique_ptr<QueryResult> MySQLConnection::Query(std::string const& sql)
{
    MYSQL_RES* result = nullptr;
    MYSQL_FIELD* fields = nullptr;
    uint64 rowCount = 0;
    uint32 fieldCount = 0;

    if(!_Query(sql, &result, &fields, &rowCount, &fieldCount))
        return nullptr;

    std::unique_ptr<QueryResultMysql> queryResult(new QueryResultMysql(result, fields, rowCount, fieldCount));

    queryResult->NextRow();
    return queryResult;
}

std::unique_ptr<QueryNamedResult> MySQLConnection::QueryNamed(std::string const& sql)
{
    MYSQL_RES* result = nullptr;
    MYSQL_FIELD* fields = nullptr;
    uint64 rowCount = 0;
    uint32 fieldCount = 0;

    if(!_Query(sql, &result, &fields, &rowCount, &fieldCount))
        return nullptr;

    QueryFieldNames names(fieldCount);
    for (uint32 i = 0; i < fieldCount; i++)
        names[i] = fields[i].name;

    std::unique_ptr<QueryResultMysql> queryResult(new QueryResultMysql(result, fields, rowCount, fieldCount));

    queryResult->NextRow();
    return std::make_unique<QueryNamedResult>(std::move(queryResult), names);
}

bool MySQLConnection::Execute(std::string const& sql)
{
    if (!EnsureConnection())
        return false;

    uint32 _s = WorldTimer::getMSTime();

    if (mysql_query(mMysql, sql.c_str()))
    {
        uint32 lErrno = mysql_errno(mMysql);

        sLog.Out(LOG_DBERROR, LOG_LVL_MINIMAL, "SQL: %s", sql.c_str());
        sLog.Out(LOG_DBERROR, LOG_LVL_MINIMAL, "[%u] %s", lErrno, mysql_error(mMysql));

        HandleMySQLError(lErrno);
        return false;
    }
    else
    {
        DEBUG_FILTER_LOG(LOG_FILTER_SQL_TEXT, "[%u ms] SQL: %s", WorldTimer::getMSTimeDiff(_s,WorldTimer::getMSTime()), sql.c_str());
    }

    return true;
}

bool MySQLConnection::_TransactionCmd(std::string const& sql)
{
    if (!mMysql || m_needsReconnect)
        return false;
    m_lastError = 0;
    if (mysql_query(mMysql, sql.c_str()))
    {
        sLog.Out(LOG_BASIC, LOG_LVL_ERROR, "SQL: %s", sql.c_str());
        sLog.Out(LOG_BASIC, LOG_LVL_ERROR, "SQL ERROR: %s", mysql_error(mMysql));
        HandleMySQLError(mysql_errno(mMysql));
        return false;
    }
    else
    {
        DEBUG_FILTER_LOG(LOG_FILTER_SQL_TEXT, "SQL: %s", sql.c_str());
    }
    return true;
}

bool MySQLConnection::BeginTransaction()
{
    if (m_inTransaction || !EnsureConnection() || !_TransactionCmd("START TRANSACTION"))
        return false;
    m_inTransaction = true;
    return true;
}

bool MySQLConnection::CommitTransaction()
{
    if (!m_inTransaction)
        return false;
    bool const success = _TransactionCmd("COMMIT");
    if (success)
        m_inTransaction = false;
    return success;
}

bool MySQLConnection::RollbackTransaction()
{
    bool const success = m_inTransaction && _TransactionCmd("ROLLBACK");
    m_inTransaction = false;
    if (!success)
        m_needsReconnect = true; // Never reuse a connection with uncertain transaction state.
    return success;
}

unsigned long MySQLConnection::escape_string(char* to, char const* from, unsigned long length)
{
    if (!mMysql || !to || !from || !length)
        return 0;

    return mysql_real_escape_string(mMysql, to, from, length);
}

//////////////////////////////////////////////////////////////////////////
SqlPreparedStatement* MySQLConnection::CreateStatement(std::string const& fmt)
{
    return new MySqlPreparedStatement(fmt, *this, mMysql);
}


//////////////////////////////////////////////////////////////////////////
MySqlPreparedStatement::MySqlPreparedStatement(std::string const& fmt, SqlConnection& conn, MYSQL* mysql) : SqlPreparedStatement(fmt, conn),
    m_pMySQLConn(mysql), m_stmt(nullptr), m_pInputArgs(nullptr), m_pResultMetadata(nullptr)
{
}

MySqlPreparedStatement::~MySqlPreparedStatement()
{
    RemoveBinds();
}

bool MySqlPreparedStatement::prepare()
{
    if(isPrepared())
        return true;

    //remove old binds
    RemoveBinds();

    //create statement object
    m_stmt = mysql_stmt_init(m_pMySQLConn);
    if (!m_stmt)
    {
        sLog.Out(LOG_BASIC, LOG_LVL_ERROR, "SQL: mysql_stmt_init()() failed ");
        return false;
    }

    //prepare statement
    if (mysql_stmt_prepare(m_stmt, m_szFmt.c_str(), m_szFmt.length()))
    {
        sLog.Out(LOG_BASIC, LOG_LVL_ERROR, "SQL: mysql_stmt_prepare() failed for '%s'", m_szFmt.c_str());
        sLog.Out(LOG_BASIC, LOG_LVL_ERROR, "SQL ERROR: %s", mysql_stmt_error(m_stmt));
        static_cast<MySQLConnection&>(m_pConn).HandleMySQLError(mysql_stmt_errno(m_stmt));
        return false;
    }

    //ask the client to refresh per-column maximum lengths on store, so
    //prepared SELECT fetches can size their output buffers from them
    my_bool updateMaxLength = 1;
    if(mysql_stmt_attr_set(m_stmt, STMT_ATTR_UPDATE_MAX_LENGTH, &updateMaxLength))
    {
        sLog.Out(LOG_BASIC, LOG_LVL_ERROR, "SQL: mysql_stmt_attr_set() failed for '%s'", m_szFmt.c_str());
        sLog.Out(LOG_BASIC, LOG_LVL_ERROR, "SQL ERROR: %s", mysql_stmt_error(m_stmt));
        static_cast<MySQLConnection&>(m_pConn).HandleMySQLError(mysql_stmt_errno(m_stmt));
        return false;
    }

    /* Get the parameter count from the statement */
    m_nParams = mysql_stmt_param_count(m_stmt);

    /* Fetch result set meta information */
    m_pResultMetadata = mysql_stmt_result_metadata(m_stmt);
    //if we do not have result metadata
    if (!m_pResultMetadata && StringStartsWithCaseInsensitive(m_szFmt, "select"))
    {
        sLog.Out(LOG_BASIC, LOG_LVL_ERROR, "SQL: no meta information for '%s'", m_szFmt.c_str());
        sLog.Out(LOG_BASIC, LOG_LVL_ERROR, "SQL ERROR: %s", mysql_stmt_error(m_stmt));
        static_cast<MySQLConnection&>(m_pConn).HandleMySQLError(mysql_stmt_errno(m_stmt));
        return false;
    }

    //bind input buffers
    if(m_nParams)
    {
        m_pInputArgs = std::make_unique<MYSQL_BIND[]>(m_nParams);
    }

    //check if we have a statement which returns result sets
    if(m_pResultMetadata)
    {
        //our statement is query
        m_bIsQuery = true;
        /* Get total columns in the query */
        m_nColumns = mysql_num_fields(m_pResultMetadata);

        //bind output buffers
    }

    m_bPrepared = true;
    return true;
}

void MySqlPreparedStatement::bind(SqlStmtParameters const& holder)
{
    m_bindValid = false;
    if(!isPrepared())
    {
        MANGOS_ASSERT(false);
        return;
    }

    //finalize adding params
    if(!m_pInputArgs)
    {
        m_bindValid = holder.boundParams() == 0;
        return;
    }

    //verify if we bound all needed input parameters
    if(m_nParams != holder.boundParams())
    {
        MANGOS_ASSERT(false);
        return;
    }

    int nIndex = 0;
    SqlStmtParameters::ParameterContainer const& _args = holder.params();

    SqlStmtParameters::ParameterContainer::const_iterator iter_last = _args.end();
    for (SqlStmtParameters::ParameterContainer::const_iterator iter = _args.begin(); iter != iter_last; ++iter)
    {
        //bind parameter
        addParam(nIndex++, (*iter));
    }

    //bind input arguments
    if (mysql_stmt_bind_param(m_stmt, m_pInputArgs.get()))
    {
        sLog.Out(LOG_BASIC, LOG_LVL_ERROR, "SQL ERROR: mysql_stmt_bind_param() failed\n");
        sLog.Out(LOG_BASIC, LOG_LVL_ERROR, "SQL ERROR: %s", mysql_stmt_error(m_stmt));
        static_cast<MySQLConnection&>(m_pConn).HandleMySQLError(mysql_stmt_errno(m_stmt));
        return;
    }
    m_bindValid = true;
}

void MySqlPreparedStatement::addParam(int nIndex, SqlStmtFieldData const& data)
{
    MANGOS_ASSERT(m_pInputArgs);
    MANGOS_ASSERT(nIndex < static_cast <int32> (m_nParams));

    MYSQL_BIND& pData = m_pInputArgs[nIndex];

    my_bool bUnsigned = 0;
    enum_field_types dataType = ToMySQLType(data, bUnsigned);

    //setup MYSQL_BIND structure
    pData.buffer_type = dataType;
    pData.is_unsigned = bUnsigned;
    pData.buffer = data.buff();
    pData.length = 0;
    pData.buffer_length = data.type() == FIELD_STRING ? data.size() : 0;
}

void MySqlPreparedStatement::RemoveBinds()
{
    if(!m_stmt)
        return;

    m_pInputArgs.reset();

    mysql_free_result(m_pResultMetadata);
    mysql_stmt_close(m_stmt);

    m_stmt = nullptr;
    m_pResultMetadata = nullptr;

    m_bPrepared = false;
}

bool MySqlPreparedStatement::execute()
{
    if(!isPrepared() || !m_bindValid)
        return false;

    if(mysql_stmt_execute(m_stmt))
    {
        sLog.Out(LOG_BASIC, LOG_LVL_ERROR, "SQL: cannot execute '%s'", m_szFmt.c_str());
        sLog.Out(LOG_BASIC, LOG_LVL_ERROR, "SQL ERROR: %s", mysql_stmt_error(m_stmt));
        static_cast<MySQLConnection&>(m_pConn).HandleMySQLError(mysql_stmt_errno(m_stmt));
        return false;
    }

    return true;
}

std::unique_ptr<QueryResult> MySqlPreparedStatement::query()
{
    if(!isPrepared() || !m_bindValid)
        return nullptr;

    if(mysql_stmt_execute(m_stmt))
    {
        sLog.Out(LOG_BASIC, LOG_LVL_ERROR, "SQL: cannot execute '%s'", m_szFmt.c_str());
        sLog.Out(LOG_BASIC, LOG_LVL_ERROR, "SQL ERROR: %s", mysql_stmt_error(m_stmt));
        static_cast<MySQLConnection&>(m_pConn).HandleMySQLError(mysql_stmt_errno(m_stmt));
        return nullptr;
    }

    if(mysql_stmt_store_result(m_stmt))
    {
        sLog.Out(LOG_BASIC, LOG_LVL_ERROR, "SQL: cannot store result of '%s'", m_szFmt.c_str());
        sLog.Out(LOG_BASIC, LOG_LVL_ERROR, "SQL ERROR: %s", mysql_stmt_error(m_stmt));
        static_cast<MySQLConnection&>(m_pConn).HandleMySQLError(mysql_stmt_errno(m_stmt));
        return nullptr;
    }

    uint64 rowCount = mysql_stmt_num_rows(m_stmt);
    uint32 fieldCount = mysql_stmt_field_count(m_stmt);
    if(!rowCount || !fieldCount)
    {
        //empty result sets report no rows, like MySQLConnection::Query;
        //zero columns means this was not a SELECT statement at all
        if(!fieldCount)
            sLog.Out(LOG_BASIC, LOG_LVL_ERROR, "SQL ERROR: prepared query '%s' returned no columns; is it a SELECT?", m_szFmt.c_str());
        mysql_stmt_free_result(m_stmt);
        return nullptr;
    }

    MYSQL_RES* pMeta = mysql_stmt_result_metadata(m_stmt);
    if(!pMeta)
    {
        sLog.Out(LOG_BASIC, LOG_LVL_ERROR, "SQL ERROR: %s", mysql_stmt_error(m_stmt));
        mysql_stmt_free_result(m_stmt);
        return nullptr;
    }

    uint32 nFields = mysql_num_fields(pMeta);
    MYSQL_FIELD* pFields = mysql_fetch_fields(pMeta);

    //size output buffers from the stored per-column maximum lengths,
    //so fetched values fit without truncation
    std::vector<std::unique_ptr<char[]>> buffers(nFields);
    std::vector<unsigned long> lengths(nFields);
    // MYSQL_BIND::is_null and ::error are my_bool* (char) in the Windows client
    // headers but bool* in the Linux ones, so take the indicator type from the
    // field instead of assuming my_bool. Plain arrays rather than
    // std::vector<bool>, whose proxy reference cannot be address-taken.
    typedef std::remove_pointer<decltype(MYSQL_BIND::is_null)>::type NullIndicator;
    std::unique_ptr<NullIndicator[]> isNull = std::make_unique<NullIndicator[]>(nFields);
    std::unique_ptr<NullIndicator[]> isError = std::make_unique<NullIndicator[]>(nFields);
    std::vector<MYSQL_BIND> outBinds(nFields);
    for(uint32 i = 0; i < nFields; ++i)
    {
        buffers[i] = std::make_unique<char[]>(pFields[i].max_length + 1);
        outBinds[i].buffer_type = MYSQL_TYPE_STRING;
        outBinds[i].buffer = buffers[i].get();
        outBinds[i].buffer_length = pFields[i].max_length + 1;
        outBinds[i].length = &lengths[i];
        outBinds[i].is_null = &isNull[i];
        outBinds[i].error = &isError[i];
    }

    std::unique_ptr<QueryResultStmt> result;
    if(mysql_stmt_bind_result(m_stmt, &outBinds[0]))
    {
        sLog.Out(LOG_BASIC, LOG_LVL_ERROR, "SQL ERROR: mysql_stmt_bind_result() failed\n");
        sLog.Out(LOG_BASIC, LOG_LVL_ERROR, "SQL ERROR: %s", mysql_stmt_error(m_stmt));
    }
    else
    {
        QueryResultStmt::RowVec rows;
        rows.reserve((size_t)rowCount);
        std::vector<enum Field::DataTypes> types;
        for(uint32 i = 0; i < nFields; ++i)
            types.push_back(QueryResultMysql::ConvertNativeType(pFields[i].type));

        bool fetchOk = true;
        int fetchState = 0;
        while((fetchState = mysql_stmt_fetch(m_stmt)) == 0)
        {
            QueryResultStmt::RowCells cells;
            for(uint32 i = 0; i < nFields; ++i)
            {
                if(isNull[i])
                    cells.push_back(QueryResultStmt::Cell());
                else
                    cells.push_back(QueryResultStmt::Cell(std::string(buffers[i].get(), lengths[i])));
            }
            rows.push_back(std::move(cells));
        }
        if(fetchState != MYSQL_NO_DATA)
        {
            //unreachable in practice: buffers are sized from the stored
            //maximum lengths, so MYSQL_DATA_TRUNCATED cannot occur
            sLog.Out(LOG_BASIC, LOG_LVL_ERROR, "SQL ERROR: mysql_stmt_fetch() failed\n");
            sLog.Out(LOG_BASIC, LOG_LVL_ERROR, "SQL ERROR: %s", mysql_stmt_error(m_stmt));
            fetchOk = false;
        }

        if(fetchOk)
        {
            result.reset(new QueryResultStmt(std::move(rows), types));
            result->NextRow();
        }
    }

    mysql_free_result(pMeta);
    mysql_stmt_free_result(m_stmt);
    return result;
}

enum_field_types MySqlPreparedStatement::ToMySQLType(SqlStmtFieldData const& data, my_bool& bUnsigned)
{
    bUnsigned = 0;
    enum_field_types dataType = MYSQL_TYPE_NULL;

    switch (data.type())
    {
    case FIELD_NONE:    dataType = MYSQL_TYPE_NULL;                     break;
    case FIELD_BOOL:    dataType = MYSQL_TYPE_BIT;      bUnsigned = 1;  break;
    case FIELD_I8:      dataType = MYSQL_TYPE_TINY;                     break;
    case FIELD_UI8:     dataType = MYSQL_TYPE_TINY;     bUnsigned = 1;  break;
    case FIELD_I16:     dataType = MYSQL_TYPE_SHORT;                    break;
    case FIELD_UI16:    dataType = MYSQL_TYPE_SHORT;    bUnsigned = 1;  break;
    case FIELD_I32:     dataType = MYSQL_TYPE_LONG;                     break;
    case FIELD_UI32:    dataType = MYSQL_TYPE_LONG;     bUnsigned = 1;  break;
    case FIELD_I64:     dataType = MYSQL_TYPE_LONGLONG;                 break;
    case FIELD_UI64:    dataType = MYSQL_TYPE_LONGLONG; bUnsigned = 1;  break;
    case FIELD_FLOAT:   dataType = MYSQL_TYPE_FLOAT;                    break;
    case FIELD_DOUBLE:  dataType = MYSQL_TYPE_DOUBLE;                   break;
    case FIELD_STRING:  dataType = MYSQL_TYPE_STRING;                   break;
    }

    return dataType;
}
