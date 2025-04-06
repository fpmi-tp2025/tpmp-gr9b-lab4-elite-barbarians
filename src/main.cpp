#include <iostream>
#include "sqlite3.h"
#include <string>
#include "sql.h"
using namespace std;

// в sql.h только скрипт, который создает бд


sqlite3* db;
char* errMsg = nullptr;

int current_user_id = -1;
bool is_admin = false;
int current_broker_id = -1;

void executeSQL(const string& sql) {
    int rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        cerr << "Ошибка SQL: " << errMsg << endl;
        sqlite3_free(errMsg);
        errMsg = nullptr;
    }
    else {
        cout << "Операция выполнена успешно.\n";
    }
}

int callback(void* NotUsed, int argc, char** argv, char** azColName) {
    for (int i = 0; i < argc; i++) {
        cout << azColName[i] << ": " << (argv[i] ? argv[i] : "NULL") << "\n";
    }
    cout << "--------------------------\n";
    return 0;
}

void initTables() {
    string sql = get_script();

    executeSQL(sql);
}

void showBrokerInfo() {
    string sql = "SELECT surname, address, birth_year FROM Brokers;";
    sqlite3_exec(db, sql.c_str(), callback, nullptr, &errMsg);
}

void showProductsInfo() {
    string sql = R"(
        SELECT P.name, P.type, P.unit_price, F.name AS supplier, P.shelf_life, P.quantity_supplied
        FROM Products P
        JOIN Firms F ON P.supplier_id = F.firm_id;
    )";
    sqlite3_exec(db, sql.c_str(), callback, nullptr, &errMsg);
}

void showDealsInfo() {
    string sql = R"(
        SELECT D.deal_date, P.name, P.type, D.quantity_sold,
               B.surname AS broker, F.name AS buyer
        FROM Deals D
        JOIN Products P ON D.product_id = P.product_id
        JOIN Brokers B ON D.broker_id = B.broker_id
        JOIN Firms F ON D.buyer_id = F.firm_id;
    )";
    sqlite3_exec(db, sql.c_str(), callback, nullptr, &errMsg);
}

void showSalesByProduct(const string& startDate, const string& endDate) {
    string sql = R"(
        SELECT P.name, SUM(D.quantity_sold) AS total_sold,
               SUM(D.quantity_sold * P.unit_price) AS total_amount
        FROM Deals D
        JOIN Products P ON D.product_id = P.product_id
        WHERE D.deal_date BETWEEN ')" + startDate + "' AND '" + endDate + R"('
        GROUP BY P.name;
    )";
    sqlite3_exec(db, sql.c_str(), callback, nullptr, &errMsg);
}

void updateBrokerStats(int broker_id, int quantity, double amount) {
    
    string sql = R"(
        UPDATE BrokerStats
        SET total_units_sold = total_units_sold + )" + to_string(quantity) + R"(,
            total_sales_amount = total_sales_amount + )" + to_string(int(amount)) + R"(
        WHERE broker_id = )" + to_string(broker_id) + ";";
    cout << sql << endl;
    executeSQL(sql);
    
}

void cleanupDealsBeforeDate(const string& date) {
    string sql = R"(
        UPDATE Products SET quantity_supplied = quantity_supplied - (
            SELECT IFNULL(SUM(D.quantity_sold), 0)
            FROM Deals D
            WHERE D.product_id = Products.product_id AND D.deal_date <= ')" + date + R"('
        );
        DELETE FROM Deals WHERE deal_date <= ')" + date + R"(';
    )";
    executeSQL(sql);
}

void showDealsOnDate(const string& date) {
    string sql = R"(
        SELECT D.deal_date, P.name, P.type, D.quantity_sold,
               B.surname AS broker, F.name AS buyer
        FROM Deals D
        JOIN Products P ON D.product_id = P.product_id
        JOIN Brokers B ON D.broker_id = B.broker_id
        JOIN Firms F ON D.buyer_id = F.firm_id
        WHERE D.deal_date = ')" + date + R"(';
    )";
    sqlite3_exec(db, sql.c_str(), callback, nullptr, &errMsg);
}

bool loginUser() {
    string username, password;
    cout << "Введите логин: ";
    cin >> username;
    cout << "Введите пароль: ";
    cin >> password;

    string sql = "SELECT user_id, is_admin, broker_id FROM Users WHERE username = ? AND password = ?;";
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, password.c_str(), -1, SQLITE_STATIC);

    int rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        current_user_id = sqlite3_column_int(stmt, 0);
        is_admin = sqlite3_column_int(stmt, 1);
        current_broker_id = sqlite3_column_type(stmt, 2) != SQLITE_NULL ? sqlite3_column_int(stmt, 2) : -1;

        cout << "Успешный вход. ";
        cout << (is_admin ? "[Администратор]" : "[Брокер]") << "\n";
        sqlite3_finalize(stmt);
        return true;
    }
    else {
        cout << "Неверные логин или пароль.\n";
    }

    sqlite3_finalize(stmt);
    return false;
}

void deleteUser() {
    string username;
    cout << "Введите имя пользователя, которого нужно удалить: ";
    cin >> username;
    

    string sql_select = "SELECT user_id, broker_id, is_admin FROM Users WHERE username = ?;";
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db, sql_select.c_str(), -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);

    int rc = sqlite3_step(stmt);
    if (rc != SQLITE_ROW) {
        cout << "Пользователь не найден.\n";
        sqlite3_finalize(stmt);
        return;
    }

    int user_id = sqlite3_column_int(stmt, 0);
    int broker_id = sqlite3_column_type(stmt, 1) != SQLITE_NULL ? sqlite3_column_int(stmt, 1) : -1;
    bool user_is_admin = sqlite3_column_int(stmt, 2);
    sqlite3_finalize(stmt);

    if (broker_id == -1) {
        cout << "Админа удалить нельзя\n";
        return;
    }

    string sql_delete_user = "DELETE FROM Users WHERE user_id = " + to_string(user_id) + ";";
    executeSQL(sql_delete_user);

    if (!user_is_admin && broker_id != -1) {
        string sql_delete_stats = "DELETE FROM BrokerStats WHERE broker_id = " + to_string(broker_id) + ";";
        string sql_delete_broker = "DELETE FROM Brokers WHERE broker_id = " + to_string(broker_id) + ";";
        executeSQL(sql_delete_stats);
        executeSQL(sql_delete_broker);
    }

    cout << "Пользователь успешно удалён.\n";
}

void addDealByAdmin() {
    setlocale(LC_ALL, "ru");

    cout << "\n== Добавление сделки ==\n";

    cout << "\nСписок товаров:\n";
    string sql = R"(SELECT product_id, name, type, unit_price, quantity_supplied FROM Products;)";
    sqlite3_exec(db, sql.c_str(), callback, nullptr, &errMsg);

    cout << "\nСписок брокеров:\n";
    sql = R"(SELECT broker_id, surname FROM Brokers;)";
    sqlite3_exec(db, sql.c_str(), callback, nullptr, &errMsg);

    cout << "\nСписок фирм-покупателей:\n";
    sql = R"(SELECT firm_id, name FROM Firms WHERE is_supplier = 0;)";
    sqlite3_exec(db, sql.c_str(), callback, nullptr, &errMsg);

    int product_id, broker_id, buyer_id, quantity;
    string deal_date;

    cout << "\nВведите ID товара: ";
    cin >> product_id;
    cout << "Введите ID брокера: ";
    cin >> broker_id;
    cout << "Введите ID фирмы-покупателя: ";
    cin >> buyer_id;
    cout << "Введите дату сделки (YYYY-MM-DD): ";
    cin >> deal_date;
    cout << "Введите количество проданного товара: ";
    cin >> quantity;

    double unit_price = 0;
    int quantity_available = 0;
    string check_sql = "SELECT unit_price, quantity_supplied FROM Products WHERE product_id = ?;";
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db, check_sql.c_str(), -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, product_id);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        unit_price = sqlite3_column_double(stmt, 0);
        quantity_available = sqlite3_column_int(stmt, 1);
    }
    else {
        cout << "Ошибка: товар не найден.\n";
        sqlite3_finalize(stmt);
        return;
    }
    sqlite3_finalize(stmt);

    if (quantity > quantity_available) {
        cout << "Недостаточно товара на складе. Доступно: " << quantity_available << "\n";
        return;
    }

    string insert_sql = R"(
        INSERT INTO Deals (deal_date, product_id, quantity_sold, broker_id, buyer_id)
        VALUES (?, ?, ?, ?, ?);
    )";

    sqlite3_prepare_v2(db, insert_sql.c_str(), -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, deal_date.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, product_id);
    sqlite3_bind_int(stmt, 3, quantity);
    sqlite3_bind_int(stmt, 4, broker_id);
    sqlite3_bind_int(stmt, 5, buyer_id);

    if (sqlite3_step(stmt) == SQLITE_DONE) {
        cout << "Сделка успешно добавлена.\n";

        string update_sql = "UPDATE Products SET quantity_supplied = quantity_supplied - ? WHERE product_id = ?;";
        sqlite3_stmt* update_stmt;
        sqlite3_prepare_v2(db, update_sql.c_str(), -1, &update_stmt, nullptr);
        sqlite3_bind_int(update_stmt, 1, quantity);
        sqlite3_bind_int(update_stmt, 2, product_id);
        sqlite3_step(update_stmt);
        sqlite3_finalize(update_stmt);

        double total = unit_price * quantity;
        updateBrokerStats(broker_id, quantity, total);
    }
    else {
        cout << "Ошибка добавления сделки: " << sqlite3_errmsg(db) << "\n";
    }
    sqlite3_finalize(stmt);
}

void adminMenu() {
    setlocale(LC_ALL, "ru");
    int choice;
    do {
        cout << "\n== Меню администратора ==\n";
        cout << "1. Список брокеров\n";
        cout << "2. Список товаров\n";
        cout << "3. Список сделок\n";
        cout << "4. Статистика продаж по товарам за период\n";
        cout << "5. Удалить пользователя\n";
        cout << "6. Добавить сделку\n";
        cout << "0. Выход\n";
        cout << "Выбор: ";
        cin >> choice;

        if (choice == 1) {
            showBrokerInfo();
        }
        else if (choice == 2) {
            showProductsInfo();
        }
        else if (choice == 3) {
            showDealsInfo();
        }
        else if (choice == 4) {
            string from, to;
            cout << "Введите дату начала (YYYY-MM-DD): ";
            cin >> from;
            cout << "Введите дату конца (YYYY-MM-DD): ";
            cin >> to;
            showSalesByProduct(from, to);
        }
        else if (choice == 5) {
            deleteUser();
        }
        else if (choice == 6) {
            addDealByAdmin();
        }


    } while (choice != 0);
}

void showTopProductTypeSalesByBuyer() {
    string sql_top_type = R"(
        SELECT P.type
        FROM Deals D
        JOIN Products P ON D.product_id = P.product_id
        WHERE D.broker_id = )" + to_string(current_broker_id) + R"(
        GROUP BY P.type
        ORDER BY SUM(D.quantity_sold) DESC
        LIMIT 1;
    )";

    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db, sql_top_type.c_str(), -1, &stmt, nullptr);
    int rc = sqlite3_step(stmt);

    if (rc != SQLITE_ROW) {
        cout << "Нет данных по сделкам.\n";
        sqlite3_finalize(stmt);
        return;
    }

    string top_type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
    sqlite3_finalize(stmt);

    cout << "Наиболее популярный вид товара: " << top_type << "\n";

    string sql_report = R"(
        SELECT F.name AS buyer_name,
               SUM(D.quantity_sold) AS total_quantity,
               SUM(D.quantity_sold * P.unit_price) AS total_amount
        FROM Deals D
        JOIN Products P ON D.product_id = P.product_id
        JOIN Firms F ON D.buyer_id = F.firm_id
        WHERE P.type = ? AND D.broker_id = ?
        GROUP BY F.name;
    )";

    sqlite3_prepare_v2(db, sql_report.c_str(), -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, top_type.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, current_broker_id);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_ROW) {
        cout << "Нет продаж по данному виду товара.\n";
    }

    while (rc == SQLITE_ROW) {
        string firm = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        int qty = sqlite3_column_int(stmt, 1);
        double total = sqlite3_column_double(stmt, 2);

        cout << "Покупатель: " << firm << "\n";
        cout << "Количество: " << qty << "\n";
        cout << "Сумма: " << total << "\n";
        cout << "------------------------\n";

        rc = sqlite3_step(stmt);
    }

    sqlite3_finalize(stmt);
}

void brokerMenu() 
{
    setlocale(LC_ALL, "ru");

    int choice;
    do {
        cout << "\n== Меню брокера ==\n";
        cout << "1. Мои сделки за дату\n";
        cout << "2. Все сделки за дату\n";
        cout << "3. Отчёт по самому популярному виду товара\n";
        cout << "0. Выход\n";
        cout << "Выбор: ";
        cin >> choice;

        if (choice == 1) {
            string date;
            cout << "Введите дату (YYYY-MM-DD): ";
            cin >> date;

            string sql = R"(
                SELECT D.deal_date, P.name, P.type, D.quantity_sold,
                       F.name AS buyer
                FROM Deals D
                JOIN Products P ON D.product_id = P.product_id
                JOIN Firms F ON D.buyer_id = F.firm_id
                WHERE D.broker_id = )" + to_string(current_broker_id) +
                " AND D.deal_date = '" + date + "';";
            sqlite3_exec(db, sql.c_str(), callback, nullptr, &errMsg);
        }
        else if (choice == 2) {
            string date;
            cout << "Введите дату (YYYY-MM-DD): ";
            cin >> date;

            string sql = R"(
            SELECT 
            d.deal_id, 
            d.deal_date, 
            p.name AS product_name, 
            p.type AS product_type, 
            d.quantity_sold, 
            b.surname AS broker_name, 
            f.name AS buyer_name, 
            d.quantity_sold * p.unit_price AS total_amount
            FROM Deals d
            JOIN Products p ON d.product_id = p.product_id
            JOIN Brokers b ON d.broker_id = b.broker_id
            JOIN Firms f ON d.buyer_id = f.firm_id
            WHERE d.deal_date = ')" + date + "';";
            sqlite3_exec(db, sql.c_str(), callback, nullptr, &errMsg);
        }
        else if (choice == 3) {
            showTopProductTypeSalesByBuyer();
        }
    } while (choice != 0);
}

void registerUser() {
    int type;
    cout << "\nКого вы хотите зарегистрировать?\n";
    cout << "1. Администратора\n";
    cout << "2. Брокера\n";
    cout << "Выбор: ";
    cin >> type;

    string username, password;
    cout << "Введите имя пользователя: ";
    cin >> username;
    cout << "Введите пароль: ";
    cin >> password;

    if (type == 1) {
        string sql = "INSERT INTO Users (username, password, is_admin) VALUES (?, ?, 1);";
        sqlite3_stmt* stmt;
        sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
        sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, password.c_str(), -1, SQLITE_STATIC);

        if (sqlite3_step(stmt) == SQLITE_DONE) {
            cout << "Администратор зарегистрирован.\n";
        }
        else {
            cout << "Ошибка регистрации: " << sqlite3_errmsg(db) << "\n";
        }

        sqlite3_finalize(stmt);
    }
    else if (type == 2) {
        
        string surname, address;
        int birth_year;
        cout << "Введите фамилию брокера: ";
        cin >> surname;
        cout << "Введите адрес: ";
        cin.ignore();
        getline(cin, address);
        cout << "Введите год рождения: ";
        cin >> birth_year;

        string sql_broker = "INSERT INTO Brokers (surname, address, birth_year) VALUES (?, ?, ?);";
        sqlite3_stmt* stmt_broker;
        sqlite3_prepare_v2(db, sql_broker.c_str(), -1, &stmt_broker, nullptr);
        sqlite3_bind_text(stmt_broker, 1, surname.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt_broker, 2, address.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt_broker, 3, birth_year);

        if (sqlite3_step(stmt_broker) == SQLITE_DONE) {
            int broker_id = (int)sqlite3_last_insert_rowid(db);
            sqlite3_finalize(stmt_broker);

            string sql_user = "INSERT INTO Users (username, password, is_admin, broker_id) VALUES (?, ?, 0, ?);";
            sqlite3_stmt* stmt_user;
            sqlite3_prepare_v2(db, sql_user.c_str(), -1, &stmt_user, nullptr);
            sqlite3_bind_text(stmt_user, 1, username.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt_user, 2, password.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_int(stmt_user, 3, broker_id);

            if (sqlite3_step(stmt_user) == SQLITE_DONE) {
                cout << "Брокер и пользователь успешно зарегистрированы.\n";
            }
            else {
                cout << "Ошибка регистрации пользователя: " << sqlite3_errmsg(db) << "\n";
            }
            updateBrokerStats(broker_id, 0, 0);
            sqlite3_finalize(stmt_user);
        }
        else {
            cout << "Ошибка регистрации брокера: " << sqlite3_errmsg(db) << "\n";
            sqlite3_finalize(stmt_broker);
        }
        
    }
}

int main() {
    setlocale(LC_ALL, "ru");
    int rc = sqlite3_open("trading.db", &db);
    if (rc) {
        cerr << "Ошибка открытия БД: " << sqlite3_errmsg(db) << endl;
        return 1;
    }

    cout << "=== Торговая система ===\n";

    int choice;
    do {
        cout << "\n1. Войти\n2. Зарегистрироваться\n0. Выход\nВыбор: ";
        cin >> choice;

        if (choice == 1) {
            if (loginUser()) {
                break;
            }
        }
        else if (choice == 2) {
            registerUser();
        }
    } while (choice != 0);

    if (current_user_id != -1) {
        if (is_admin) {
            adminMenu();
        }
        else {
            brokerMenu();
        }
    }

    sqlite3_close(db);
    return 0;
}
