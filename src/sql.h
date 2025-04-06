#pragma once
#include <string>

std::string get_script() {
    std::string script = R"(

    DROP TABLE IF EXISTS BrokerStats;
    DROP TABLE IF EXISTS Deals;
    DROP TABLE IF EXISTS Products;
    DROP TABLE IF EXISTS Firms;
    DROP TABLE IF EXISTS Brokers;

    CREATE TABLE Brokers(
        broker_id INTEGER PRIMARY KEY AUTOINCREMENT,
        surname TEXT NOT NULL,
        address TEXT,
        birth_year INTEGER
    );

    CREATE TABLE Firms(
        firm_id INTEGER PRIMARY KEY AUTOINCREMENT,
        name TEXT NOT NULL UNIQUE,
        is_supplier INTEGER NOT NULL-- 1 = поставщик, 0 = покупатель
    );

    CREATE TABLE Products(
        product_id INTEGER PRIMARY KEY AUTOINCREMENT,
        name TEXT NOT NULL,
        type TEXT,
        unit_price REAL NOT NULL,
        supplier_id INTEGER NOT NULL,
        shelf_life TEXT,
        quantity_supplied INTEGER NOT NULL,
        FOREIGN KEY(supplier_id) REFERENCES Firms(firm_id)
    );

    CREATE TABLE Deals(
        deal_id INTEGER PRIMARY KEY AUTOINCREMENT,
        deal_date TEXT NOT NULL,
        product_id INTEGER NOT NULL,
        quantity_sold INTEGER NOT NULL,
        broker_id INTEGER NOT NULL,
        buyer_id INTEGER NOT NULL,
        FOREIGN KEY(product_id) REFERENCES Products(product_id),
        FOREIGN KEY(broker_id) REFERENCES Brokers(broker_id),
        FOREIGN KEY(buyer_id) REFERENCES Firms(firm_id)
    );

    CREATE TABLE BrokerStats(
        broker_id INTEGER PRIMARY KEY,
        total_units_sold INTEGER DEFAULT 0,
        total_sales_amount REAL DEFAULT 0.0,
        FOREIGN KEY(broker_id) REFERENCES Brokers(broker_id)
    );

    CREATE TABLE Users(
        user_id INTEGER PRIMARY KEY AUTOINCREMENT,
        username TEXT NOT NULL UNIQUE,
        password TEXT NOT NULL,
        broker_id INTEGER,
        is_admin BOOLEAN NOT NULL,
        FOREIGN KEY(broker_id) REFERENCES Brokers(broker_id)
    );


    )";
    return script;
}
