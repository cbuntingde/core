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

#if !defined(QUERYRESULTMYSQL_H)
#define QUERYRESULTMYSQL_H

#include "Common.h"

#ifdef WIN32
#include <winsock2.h>
#endif
#include <mysql.h>

// QueryResult over rows materialized from a MYSQL_STMT fetch. The stmt API
// exposes no MYSQL_RES row access, so cells are copied into owned storage.
class QueryResultStmt : public QueryResult
{
    public:
        struct Cell
        {
            Cell() : value(), isNull(true) {}
            explicit Cell(std::string const& v) : value(v), isNull(false) {}

            std::string value;
            bool isNull;
        };
        typedef std::vector<Cell> RowCells;
        typedef std::vector<RowCells> RowVec;

        // Takes ownership of already-fetched rows; types holds one
        // Field::DataTypes per column. Like QueryResultMysql, the result is
        // positioned on the first row by an initial NextRow() call.
        QueryResultStmt(RowVec rows, std::vector<enum Field::DataTypes> const& types);
        ~QueryResultStmt() override;

        bool NextRow() override;

    private:
        void EndQuery();

        RowVec mRows;
        size_t mRowIndex;
        Field* mFields;
};

class QueryResultMysql : public QueryResult
{
    public:
        QueryResultMysql(MYSQL_RES* result, MYSQL_FIELD* fields, uint64 rowCount, uint32 fieldCount);

        ~QueryResultMysql() override;

        bool NextRow() override;

        static enum Field::DataTypes ConvertNativeType(enum_field_types mysqlType);

    private:
        void EndQuery();

        MYSQL_RES* mResult;
};
#endif
