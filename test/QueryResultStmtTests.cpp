#include "TestFramework.h"
#include "Database/DatabaseEnv.h"

// QueryResultStmt owns its Field array (unique_ptr) and publishes it
// through the base-class row view. No database connection needed:
// rows are supplied already fetched.

static QueryResultStmt::RowVec MakeRows()
{
    QueryResultStmt::RowVec rows;
    QueryResultStmt::RowCells first;
    first.push_back(QueryResultStmt::Cell(std::string("abc")));
    first.push_back(QueryResultStmt::Cell());
    rows.push_back(first);
    QueryResultStmt::RowCells second;
    second.push_back(QueryResultStmt::Cell(std::string("xyz")));
    second.push_back(QueryResultStmt::Cell(std::string("7")));
    rows.push_back(second);
    return rows;
}

static std::vector<enum Field::DataTypes> MakeTypes()
{
    std::vector<enum Field::DataTypes> types;
    types.push_back(Field::DB_TYPE_STRING);
    types.push_back(Field::DB_TYPE_STRING);
    return types;
}

TEST(QueryResultStmt_FirstRowValues)
{
    QueryResultStmt result(MakeRows(), MakeTypes());
    ASSERT_EQ(result.NextRow(), true);
    Field* row = result.Fetch();
    ASSERT_EQ(row[0].GetCppString(), std::string("abc"));
    ASSERT_EQ(row[1].IsNULL(), true);
}

TEST(QueryResultStmt_IteratesAllRows)
{
    QueryResultStmt result(MakeRows(), MakeTypes());
    uint32 seen = 0;
    while (result.NextRow())
        ++seen;
    ASSERT_EQ(seen, 2u);
    ASSERT_EQ(result.NextRow(), false);
}

TEST(QueryResultStmt_EmptyRowsYieldNothing)
{
    QueryResultStmt::RowVec rows;
    QueryResultStmt result(rows, MakeTypes());
    ASSERT_EQ(result.NextRow(), false);
}

TEST(QueryResultStmt_SecondRowValues)
{
    QueryResultStmt result(MakeRows(), MakeTypes());
    ASSERT_EQ(result.NextRow(), true);
    ASSERT_EQ(result.NextRow(), true);
    Field* row = result.Fetch();
    ASSERT_EQ(row[0].GetCppString(), std::string("xyz"));
    ASSERT_EQ(row[1].GetCppString(), std::string("7"));
}
