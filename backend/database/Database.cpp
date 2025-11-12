#include "Database.h"
#include <stdexcept>

Database::~Database() {
	close();
}

Database& Database::instance() {
	static Database inst;
	return inst;
}

bool Database::open(const std::string& path, std::string& errMsg) {
	if (db) return true;
	int rc = sqlite3_open(path.c_str(), &db);
	if (rc != SQLITE_OK) {
		errMsg = sqlite3_errmsg(db);
		close();
		return false;
	}
	// Enforce foreign keys
	char* em = nullptr;
	sqlite3_exec(db, "PRAGMA foreign_keys = ON;", nullptr, nullptr, &em);
	if (em) sqlite3_free(em);

	// Initialize schema if tables don't exist
	if (!initializeSchema(errMsg)) {
		close();
		return false;
	}

	return true;
}

void Database::close() {
	if (db) {
		sqlite3_close(db);
		db = nullptr;
	}
}

bool Database::initializeSchema(std::string& errMsg) {
	// Check if dishes table exists
	const char* checkTable = "SELECT name FROM sqlite_master WHERE type='table' AND name='dishes';";
	sqlite3_stmt* stmt = nullptr;
	bool tableExists = false;
	if (sqlite3_prepare_v2(db, checkTable, -1, &stmt, nullptr) == SQLITE_OK) {
		if (sqlite3_step(stmt) == SQLITE_ROW) {
			tableExists = true;
		}
		sqlite3_finalize(stmt);
	}

	// Always create tables (CREATE TABLE IF NOT EXISTS is safe)
	char* em = nullptr;
	const char* schema = 
		"CREATE TABLE IF NOT EXISTS dishes ("
		"	id INTEGER PRIMARY KEY,"
		"	name TEXT NOT NULL,"
		"	price REAL NOT NULL"
		");"
		"CREATE TABLE IF NOT EXISTS orders ("
		"	id INTEGER PRIMARY KEY,"
		"	status TEXT NOT NULL DEFAULT 'pending',"
		"	total REAL NOT NULL DEFAULT 0"
		");"
		"CREATE TABLE IF NOT EXISTS order_items ("
		"	id INTEGER PRIMARY KEY,"
		"	order_id INTEGER NOT NULL,"
		"	dish_id INTEGER NOT NULL,"
		"	quantity INTEGER NOT NULL,"
		"	FOREIGN KEY(order_id) REFERENCES orders(id) ON DELETE CASCADE,"
		"	FOREIGN KEY(dish_id) REFERENCES dishes(id)"
		");";
	
	if (sqlite3_exec(db, schema, nullptr, nullptr, &em) != SQLITE_OK) {
		if (em) {
			errMsg = em;
			sqlite3_free(em);
		} else {
			errMsg = "Failed to create database schema";
		}
		return false;
	}

	// Verify table was created
	if (sqlite3_prepare_v2(db, checkTable, -1, &stmt, nullptr) == SQLITE_OK) {
		tableExists = (sqlite3_step(stmt) == SQLITE_ROW);
		sqlite3_finalize(stmt);
	}
	
	if (!tableExists) {
		errMsg = "Table 'dishes' was not created after schema execution";
		return false;
	}

	// Initialize data if table is empty
	if (!hasInitialData()) {
		if (!insertInitialData(errMsg)) {
			return false;
		}
	}

	return true;
}

bool Database::hasInitialData() {
	const char* checkData = "SELECT COUNT(*) FROM dishes;";
	sqlite3_stmt* stmt = nullptr;
	int count = 0;
	if (sqlite3_prepare_v2(db, checkData, -1, &stmt, nullptr) == SQLITE_OK) {
		if (sqlite3_step(stmt) == SQLITE_ROW) {
			count = sqlite3_column_int(stmt, 0);
		}
		sqlite3_finalize(stmt);
	}
	return count > 0;
}

bool Database::insertInitialData(std::string& errMsg) {
	char* em = nullptr;
	const char* initData = 
		"INSERT INTO dishes (id, name, price) VALUES"
		"	(1, 'Margherita Pizza', 8.5),"
		"	(2, 'Caesar Salad', 6.0),"
		"	(3, 'Spaghetti Bolognese', 9.5),"
		"	(4, 'Cheeseburger', 7.5),"
		"	(5, 'Chicken Caesar Salad', 8.0),"
		"	(6, 'Vegetable Stir Fry', 6.5),"
		"	(7, 'Fish and Chips', 10.0),"
		"	(8, 'Pizza Margherita', 8.5),"
		"	(9, 'Caesar Salad', 6.0),"
		"	(10, 'Spaghetti Bolognese', 9.5)"
		"ON CONFLICT(id) DO NOTHING;";
	
	if (sqlite3_exec(db, initData, nullptr, nullptr, &em) != SQLITE_OK) {
		if (em) {
			errMsg = em;
			sqlite3_free(em);
		} else {
			errMsg = "Failed to insert initial data";
		}
		return false;
	}
	return true;
}

std::vector<Dish> Database::getAllDishes(std::string& errMsg) {
	std::vector<Dish> result;
	const char* sql = "SELECT id, name, price FROM dishes ORDER BY id;";
	sqlite3_stmt* stmt = nullptr;
	if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
		errMsg = sqlite3_errmsg(db);
		return result;
	}
	while (sqlite3_step(stmt) == SQLITE_ROW) {
		Dish d;
		d.id = sqlite3_column_int(stmt, 0);
		d.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
		d.price = sqlite3_column_double(stmt, 2);
		result.push_back(std::move(d));
	}
	sqlite3_finalize(stmt);
	return result;
}

std::optional<int> Database::createOrder(const std::vector<OrderItem>& items, std::string& errMsg) {
	char* em = nullptr;
	if (sqlite3_exec(db, "BEGIN IMMEDIATE;", nullptr, nullptr, &em) != SQLITE_OK) {
		if (em) { errMsg = em; sqlite3_free(em); }
		return std::nullopt;
	}
	// create order row
	const char* insOrder = "INSERT INTO orders(status,total) VALUES('pending',0);";
	if (sqlite3_exec(db, insOrder, nullptr, nullptr, &em) != SQLITE_OK) {
		if (em) { errMsg = em; sqlite3_free(em); }
		sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
		return std::nullopt;
	}
	int orderId = (int)sqlite3_last_insert_rowid(db);

	// insert items
	const char* insItem = "INSERT INTO order_items(order_id,dish_id,quantity) VALUES(?,?,?);";
	sqlite3_stmt* stmt = nullptr;
	if (sqlite3_prepare_v2(db, insItem, -1, &stmt, nullptr) != SQLITE_OK) {
		errMsg = sqlite3_errmsg(db);
		sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
		return std::nullopt;
	}
	for (const auto& it : items) {
		sqlite3_bind_int(stmt, 1, orderId);
		sqlite3_bind_int(stmt, 2, it.dishId);
		sqlite3_bind_int(stmt, 3, it.quantity);
		if (sqlite3_step(stmt) != SQLITE_DONE) {
			errMsg = sqlite3_errmsg(db);
			sqlite3_finalize(stmt);
			sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
			return std::nullopt;
		}
		sqlite3_reset(stmt);
		sqlite3_clear_bindings(stmt);
	}
	sqlite3_finalize(stmt);

	// compute total
	const char* updTotal =
		"UPDATE orders SET total = ("
		" SELECT IFNULL(SUM(oi.quantity * d.price),0)"
		" FROM order_items oi JOIN dishes d ON d.id = oi.dish_id"
		" WHERE oi.order_id = orders.id"
		") WHERE id = ?;";
	sqlite3_stmt* upd = nullptr;
	if (sqlite3_prepare_v2(db, updTotal, -1, &upd, nullptr) != SQLITE_OK) {
		errMsg = sqlite3_errmsg(db);
		sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
		return std::nullopt;
	}
	sqlite3_bind_int(upd, 1, orderId);
	if (sqlite3_step(upd) != SQLITE_DONE) {
		errMsg = sqlite3_errmsg(db);
		sqlite3_finalize(upd);
		sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
		return std::nullopt;
	}
	sqlite3_finalize(upd);

	if (sqlite3_exec(db, "COMMIT;", nullptr, nullptr, &em) != SQLITE_OK) {
		if (em) { errMsg = em; sqlite3_free(em); }
		return std::nullopt;
	}
	return orderId;
}

std::optional<Order> Database::getOrder(int orderId, std::string& errMsg) {
	Order order{};
	order.id = orderId;

	// header
	const char* selOrder = "SELECT status,total FROM orders WHERE id = ?;";
	sqlite3_stmt* st = nullptr;
	if (sqlite3_prepare_v2(db, selOrder, -1, &st, nullptr) != SQLITE_OK) {
		errMsg = sqlite3_errmsg(db);
		return std::nullopt;
	}
	sqlite3_bind_int(st, 1, orderId);
	if (sqlite3_step(st) != SQLITE_ROW) {
		sqlite3_finalize(st);
		return std::nullopt;
	}
	order.status = reinterpret_cast<const char*>(sqlite3_column_text(st, 0));
	order.total = sqlite3_column_double(st, 1);
	sqlite3_finalize(st);

	// items
	const char* selItems = "SELECT dish_id, quantity FROM order_items WHERE order_id = ? ORDER BY id;";
	if (sqlite3_prepare_v2(db, selItems, -1, &st, nullptr) != SQLITE_OK) {
		errMsg = sqlite3_errmsg(db);
		return std::nullopt;
	}
	sqlite3_bind_int(st, 1, orderId);
	while (sqlite3_step(st) == SQLITE_ROW) {
		OrderItem it;
		it.dishId = sqlite3_column_int(st, 0);
		it.quantity = sqlite3_column_int(st, 1);
		order.items.push_back(it);
	}
	sqlite3_finalize(st);
	return order;
}

std::vector<Order> Database::getAllOrders(std::string& errMsg) {
	std::vector<Order> result;
	const char* sql = "SELECT id, status, total FROM orders ORDER BY id DESC;";
	sqlite3_stmt* stmt = nullptr;
	if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
		errMsg = sqlite3_errmsg(db);
		return result;
	}
	
	while (sqlite3_step(stmt) == SQLITE_ROW) {
		Order order;
		order.id = sqlite3_column_int(stmt, 0);
		order.status = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
		order.total = sqlite3_column_double(stmt, 2);
		
		// Load items for this order
		const char* selItems = "SELECT dish_id, quantity FROM order_items WHERE order_id = ? ORDER BY id;";
		sqlite3_stmt* itemStmt = nullptr;
		if (sqlite3_prepare_v2(db, selItems, -1, &itemStmt, nullptr) == SQLITE_OK) {
			sqlite3_bind_int(itemStmt, 1, order.id);
			while (sqlite3_step(itemStmt) == SQLITE_ROW) {
				OrderItem it;
				it.dishId = sqlite3_column_int(itemStmt, 0);
				it.quantity = sqlite3_column_int(itemStmt, 1);
				order.items.push_back(it);
			}
			sqlite3_finalize(itemStmt);
		}
		
		result.push_back(std::move(order));
	}
	sqlite3_finalize(stmt);
	return result;
}

bool Database::updateOrderStatus(int orderId, const std::string& status, std::string& errMsg) {
	const char* sql = "UPDATE orders SET status = ? WHERE id = ?;";
	sqlite3_stmt* stmt = nullptr;
	if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
		errMsg = sqlite3_errmsg(db);
		return false;
	}
	sqlite3_bind_text(stmt, 1, status.c_str(), -1, SQLITE_STATIC);
	sqlite3_bind_int(stmt, 2, orderId);
	
	if (sqlite3_step(stmt) != SQLITE_DONE) {
		errMsg = sqlite3_errmsg(db);
		sqlite3_finalize(stmt);
		return false;
	}
	sqlite3_finalize(stmt);
	return true;
}


