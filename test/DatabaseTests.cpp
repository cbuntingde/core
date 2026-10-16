#include "TestFramework.h"
#include "Database/SqlPreparedStatement.h"

// SqlStmtFieldData can be constructed and queried without a DB connection.

TEST(SqlStmtFieldData_StringType)
{
    SqlStmtFieldData data("hello");
    ASSERT_EQ(data.type(), SqlStmtFieldType::FIELD_STRING);
    ASSERT_EQ(std::string(data.toStr()), "hello");
    ASSERT_EQ(data.size(), size_t(5));
}

TEST(SqlStmtFieldData_UInt32Type)
{
    SqlStmtFieldData data(uint32_t(42));
    ASSERT_EQ(data.type(), SqlStmtFieldType::FIELD_UI32);
    ASSERT_EQ(data.toUint32(), 42u);
    ASSERT_EQ(data.size(), sizeof(uint32_t));
}

TEST(SqlStmtFieldData_Int32Type)
{
    SqlStmtFieldData data(int32_t(-7));
    ASSERT_EQ(data.type(), SqlStmtFieldType::FIELD_I32);
    ASSERT_EQ(data.toInt32(), -7);
    ASSERT_EQ(data.size(), sizeof(int32_t));
}

TEST(SqlStmtFieldData_NullType)
{
    SqlStmtFieldData data(nullptr);
    ASSERT_EQ(data.type(), SqlStmtFieldType::FIELD_NONE);
    ASSERT_EQ(data.size(), size_t(0));
}

TEST(SqlStmtFieldData_BoolType)
{
    SqlStmtFieldData data(true);
    ASSERT_EQ(data.type(), SqlStmtFieldType::FIELD_BOOL);
    ASSERT_EQ(data.toBool(), true);
    ASSERT_EQ(data.size(), sizeof(bool));
}

TEST(SqlStmtFieldData_FloatType)
{
    SqlStmtFieldData data(3.14f);
    ASSERT_EQ(data.type(), SqlStmtFieldType::FIELD_FLOAT);
    ASSERT_EQ(data.toFloat(), 3.14f);
    ASSERT_EQ(data.size(), sizeof(float));
}

TEST(SqlStmtFieldData_CharPtrType)
{
    SqlStmtFieldData data("world");
    ASSERT_EQ(data.type(), SqlStmtFieldType::FIELD_STRING);
    ASSERT_EQ(std::string(data.toStr()), "world");
    ASSERT_EQ(data.size(), size_t(5));
}

TEST(SqlStmtParameters_AddAndCount)
{
    SqlStmtParameters params(3);
    params.addParam(SqlStmtFieldData(uint32_t(1)));
    params.addParam(SqlStmtFieldData("abc"));
    params.addParam(SqlStmtFieldData(nullptr));
    ASSERT_EQ(params.boundParams(), 3);
    ASSERT_EQ(params.params()[0].type(), SqlStmtFieldType::FIELD_UI32);
    ASSERT_EQ(params.params()[1].type(), SqlStmtFieldType::FIELD_STRING);
    ASSERT_EQ(params.params()[2].type(), SqlStmtFieldType::FIELD_NONE);
}
