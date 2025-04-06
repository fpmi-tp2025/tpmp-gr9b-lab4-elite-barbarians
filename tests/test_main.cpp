#include <gtest/gtest.h>
#include <sqlite3.h>
#include "../src/sql.h"

class DBTest : public ::testing::Test {
protected:
    sqlite3* db;
    
    void SetUp() override {
        int rc = sqlite3_open(":memory:", &db);
        ASSERT_EQ(rc, SQLITE_OK) << "Failed to open database: " << sqlite3_errmsg(db);
        std::string sql = get_script();
        char* errMsg = nullptr;
        rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errMsg);
        ASSERT_EQ(rc, SQLITE_OK) << "Failed to create tables: " << errMsg;
    }
    
    void TearDown() override {
        sqlite3_close(db);
    }
};

TEST_F(DBTest, CreateBroker) {
    char* errMsg = nullptr;
    std::string sql = "INSERT INTO Brokers (surname, address, birth_year) VALUES ('Test', 'Test Address', 1990);";
    int rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errMsg);
    ASSERT_EQ(rc, SQLITE_OK) << "Failed to insert broker: " << errMsg;
    
    sqlite3_stmt* stmt;
    sql = "SELECT COUNT(*) FROM Brokers WHERE surname = 'Test';";
    rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
    ASSERT_EQ(rc, SQLITE_OK);
    
    rc = sqlite3_step(stmt);
    ASSERT_EQ(rc, SQLITE_ROW);
    ASSERT_EQ(sqlite3_column_int(stmt, 0), 1);
    
    sqlite3_finalize(stmt);
}

TEST_F(DBTest, CreateUser) {
    char* errMsg = nullptr;
    std::string sql = "INSERT INTO Users (username, password, is_admin) VALUES ('test_user', 'test_pass', 0);";
    int rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errMsg);
    ASSERT_EQ(rc, SQLITE_OK) << "Failed to insert user: " << errMsg;
    
    sqlite3_stmt* stmt;
    sql = "SELECT COUNT(*) FROM Users WHERE username = 'test_user';";
    rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
    ASSERT_EQ(rc, SQLITE_OK);
    
    rc = sqlite3_step(stmt);
    ASSERT_EQ(rc, SQLITE_ROW);
    ASSERT_EQ(sqlite3_column_int(stmt, 0), 1);
    
    sqlite3_finalize(stmt);
}

TEST_F(DBTest, CreateDeal) {
    // Подготовка данных
    char* errMsg = nullptr;
    std::string sql = R"(
        INSERT INTO Firms (name, is_supplier) VALUES ('TestSupplier', 1);
        INSERT INTO Firms (name, is_supplier) VALUES ('TestBuyer', 0);
        INSERT INTO Brokers (surname, address, birth_year) VALUES ('TestBroker', 'Address', 1990);
        INSERT INTO Products (name, type, unit_price, supplier_id, shelf_life, quantity_supplied) 
        VALUES ('TestProduct', 'TestType', 100, 1, '2024-12-31', 1000);
    )";
    int rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errMsg);
    ASSERT_EQ(rc, SQLITE_OK) << "Failed to insert test data: " << errMsg;
    
    // Создание сделки
    sql = R"(
        INSERT INTO Deals (deal_date, product_id, quantity_sold, broker_id, buyer_id)
        VALUES ('2023-01-01', 1, 100, 1, 2);
    )";
    rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errMsg);
    ASSERT_EQ(rc, SQLITE_OK) << "Failed to insert deal: " << errMsg;
    
    // Проверка
    sqlite3_stmt* stmt;
    sql = "SELECT COUNT(*) FROM Deals;";
    rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
    ASSERT_EQ(rc, SQLITE_OK);
    
    rc = sqlite3_step(stmt);
    ASSERT_EQ(rc, SQLITE_ROW);
    ASSERT_EQ(sqlite3_column_int(stmt, 0), 1);
    
    sqlite3_finalize(stmt);
}