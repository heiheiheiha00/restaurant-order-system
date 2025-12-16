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
	const char* schema = R"SQL(
		CREATE TABLE IF NOT EXISTS users (
			id INTEGER PRIMARY KEY,
			username TEXT NOT NULL UNIQUE,
			password_hash TEXT NOT NULL,
			phone TEXT,
			created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP
		);
		CREATE TABLE IF NOT EXISTS merchants (
			id INTEGER PRIMARY KEY,
			username TEXT NOT NULL UNIQUE,
			password_hash TEXT NOT NULL,
			store_name TEXT,
			created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP
		);
		CREATE TABLE IF NOT EXISTS sessions (
			id INTEGER PRIMARY KEY,
			token TEXT NOT NULL UNIQUE,
			user_id INTEGER,
			merchant_id INTEGER,
			expires_at TEXT NOT NULL,
			created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
			FOREIGN KEY(user_id) REFERENCES users(id) ON DELETE CASCADE,
			FOREIGN KEY(merchant_id) REFERENCES merchants(id) ON DELETE CASCADE
		);
		CREATE TABLE IF NOT EXISTS dishes (
			id INTEGER PRIMARY KEY,
			name TEXT NOT NULL,
			description TEXT,
			category TEXT,
			price REAL NOT NULL,
			is_available INTEGER NOT NULL DEFAULT 1,
			created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
			updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP
		);
		CREATE TABLE IF NOT EXISTS orders (
			id INTEGER PRIMARY KEY,
			user_id INTEGER,
			status TEXT NOT NULL DEFAULT 'pending',
			total REAL NOT NULL DEFAULT 0,
			pickup_notified INTEGER NOT NULL DEFAULT 0,
			created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
			updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
			FOREIGN KEY(user_id) REFERENCES users(id) ON DELETE SET NULL
		);
		CREATE TABLE IF NOT EXISTS order_items (
			id INTEGER PRIMARY KEY,
			order_id INTEGER NOT NULL,
			dish_id INTEGER NOT NULL,
			quantity INTEGER NOT NULL,
			unit_price REAL NOT NULL,
			FOREIGN KEY(order_id) REFERENCES orders(id) ON DELETE CASCADE,
			FOREIGN KEY(dish_id) REFERENCES dishes(id)
		);
	)SQL";
	
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
		"INSERT INTO dishes (id, name, description, category, price, is_available) VALUES"
		"	(1, 'Margherita Pizza', 'Classic tomato, mozzarella and basil', 'Pizza', 8.5, 1),"
		"	(2, 'Caesar Salad', 'Romaine lettuce with parmesan and croutons', 'Salad', 6.0, 1),"
		"	(3, 'Spaghetti Bolognese', 'Slow cooked beef ragu', 'Pasta', 9.5, 1),"
		"	(4, 'Cheeseburger', 'Beef patty, cheddar, pickles', 'Burger', 7.5, 1),"
		"	(5, 'Chicken Caesar Salad', 'Grilled chicken with Caesar dressing', 'Salad', 8.0, 1),"
		"	(6, 'Vegetable Stir Fry', 'Seasonal veggies with soy glaze', 'Wok', 6.5, 1),"
		"	(7, 'Fish and Chips', 'Beer battered cod with fries', 'Seafood', 10.0, 1),"
		"	(8, 'Tiramisu', 'Espresso soaked ladyfingers and mascarpone', 'Dessert', 5.5, 1),"
		"	(9, 'Lemonade', 'Freshly squeezed lemon juice', 'Drinks', 3.0, 1),"
		"	(10, 'Tomato Soup', 'Roasted tomato soup with basil oil', 'Soup', 5.0, 1)"
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
	const char* sql = "SELECT id, name, description, category, price, is_available FROM dishes ORDER BY id;";
	sqlite3_stmt* stmt = nullptr;
	if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
		errMsg = sqlite3_errmsg(db);
		return result;
	}
	while (sqlite3_step(stmt) == SQLITE_ROW) {
		Dish d;
		d.id = sqlite3_column_int(stmt, 0);
		d.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
		const auto* desc = sqlite3_column_text(stmt, 2);
		d.description = desc ? reinterpret_cast<const char*>(desc) : "";
		const auto* cat = sqlite3_column_text(stmt, 3);
		d.category = cat ? reinterpret_cast<const char*>(cat) : "";
		d.price = sqlite3_column_double(stmt, 4);
		d.isAvailable = sqlite3_column_int(stmt, 5) != 0;
		result.push_back(std::move(d));
	}
	sqlite3_finalize(stmt);
	return result;
}

std::optional<Dish> Database::getDish(int dishId, std::string& errMsg) {
	const char* sql = "SELECT id, name, description, category, price, is_available FROM dishes WHERE id = ?;";
	sqlite3_stmt* stmt = nullptr;
	if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
		errMsg = sqlite3_errmsg(db);
		return std::nullopt;
	}
	sqlite3_bind_int(stmt, 1, dishId);
	if (sqlite3_step(stmt) != SQLITE_ROW) {
		sqlite3_finalize(stmt);
		return std::nullopt;
	}
	Dish d;
	d.id = sqlite3_column_int(stmt, 0);
	d.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
	const auto* desc = sqlite3_column_text(stmt, 2);
	d.description = desc ? reinterpret_cast<const char*>(desc) : "";
	const auto* cat = sqlite3_column_text(stmt, 3);
	d.category = cat ? reinterpret_cast<const char*>(cat) : "";
	d.price = sqlite3_column_double(stmt, 4);
	d.isAvailable = sqlite3_column_int(stmt, 5) != 0;
	sqlite3_finalize(stmt);
	return d;
}

std::optional<int> Database::createDish(const Dish& dish, std::string& errMsg) {
	const char* sql = "INSERT INTO dishes(name, description, category, price, is_available) VALUES(?,?,?,?,?);";
	sqlite3_stmt* stmt = nullptr;
	if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
		errMsg = sqlite3_errmsg(db);
		return std::nullopt;
	}
	sqlite3_bind_text(stmt, 1, dish.name.c_str(), -1, SQLITE_STATIC);
	sqlite3_bind_text(stmt, 2, dish.description.c_str(), -1, SQLITE_STATIC);
	sqlite3_bind_text(stmt, 3, dish.category.c_str(), -1, SQLITE_STATIC);
	sqlite3_bind_double(stmt, 4, dish.price);
	sqlite3_bind_int(stmt, 5, dish.isAvailable ? 1 : 0);
	if (sqlite3_step(stmt) != SQLITE_DONE) {
		errMsg = sqlite3_errmsg(db);
		sqlite3_finalize(stmt);
		return std::nullopt;
	}
	sqlite3_finalize(stmt);
	return static_cast<int>(sqlite3_last_insert_rowid(db));
}

bool Database::updateDish(int dishId,
	const std::optional<std::string>& name,
	const std::optional<std::string>& description,
	const std::optional<std::string>& category,
	const std::optional<double>& price,
	const std::optional<bool>& isAvailable,
	std::string& errMsg) {
	std::string sql = "UPDATE dishes SET ";
	bool first = true;
	if (name) {
		sql += first ? "name = ?" : ", name = ?";
		first = false;
	}
	if (description) {
		sql += first ? "description = ?" : ", description = ?";
		first = false;
	}
	if (category) {
		sql += first ? "category = ?" : ", category = ?";
		first = false;
	}
	if (price) {
		sql += first ? "price = ?" : ", price = ?";
		first = false;
	}
	if (isAvailable) {
		sql += first ? "is_available = ?" : ", is_available = ?";
		first = false;
	}
	if (first) {
		return true;
	}
	sql += ", updated_at = CURRENT_TIMESTAMP WHERE id = ?;";

	sqlite3_stmt* stmt = nullptr;
	if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
		errMsg = sqlite3_errmsg(db);
		return false;
	}

	int bindIdx = 1;
	auto bindText = [&](const std::optional<std::string>& value) {
		if (value) {
			sqlite3_bind_text(stmt, bindIdx++, value->c_str(), -1, SQLITE_STATIC);
		}
	};
	bindText(name);
	bindText(description);
	bindText(category);
	if (price) {
		sqlite3_bind_double(stmt, bindIdx++, *price);
	}
	if (isAvailable) {
		sqlite3_bind_int(stmt, bindIdx++, *isAvailable ? 1 : 0);
	}
	sqlite3_bind_int(stmt, bindIdx, dishId);

	if (sqlite3_step(stmt) != SQLITE_DONE) {
		errMsg = sqlite3_errmsg(db);
		sqlite3_finalize(stmt);
		return false;
	}
	sqlite3_finalize(stmt);
	return true;
}

bool Database::createUser(const std::string& username, const std::string& passwordHash, const std::string& phone, std::string& errMsg) {
	const char* sql = "INSERT INTO users(username, password_hash, phone) VALUES(?,?,?);";
	sqlite3_stmt* stmt = nullptr;
	if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
		errMsg = sqlite3_errmsg(db);
		return false;
	}
	sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
	sqlite3_bind_text(stmt, 2, passwordHash.c_str(), -1, SQLITE_STATIC);
	sqlite3_bind_text(stmt, 3, phone.c_str(), -1, SQLITE_STATIC);
	bool ok = sqlite3_step(stmt) == SQLITE_DONE;
	if (!ok) {
		errMsg = sqlite3_errmsg(db);
	}
	sqlite3_finalize(stmt);
	return ok;
}

bool Database::createMerchant(const std::string& username, const std::string& passwordHash, const std::string& storeName, std::string& errMsg) {
	const char* sql = "INSERT INTO merchants(username, password_hash, store_name) VALUES(?,?,?);";
	sqlite3_stmt* stmt = nullptr;
	if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
		errMsg = sqlite3_errmsg(db);
		return false;
	}
	sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
	sqlite3_bind_text(stmt, 2, passwordHash.c_str(), -1, SQLITE_STATIC);
	sqlite3_bind_text(stmt, 3, storeName.c_str(), -1, SQLITE_STATIC);
	bool ok = sqlite3_step(stmt) == SQLITE_DONE;
	if (!ok) {
		errMsg = sqlite3_errmsg(db);
	}
	sqlite3_finalize(stmt);
	return ok;
}

std::optional<User> Database::getUserByUsername(const std::string& username, std::string& errMsg) {
	const char* sql = "SELECT id, username, password_hash, phone, created_at FROM users WHERE username = ?;";
	sqlite3_stmt* stmt = nullptr;
	if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
		errMsg = sqlite3_errmsg(db);
		return std::nullopt;
	}
	sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
	if (sqlite3_step(stmt) != SQLITE_ROW) {
		sqlite3_finalize(stmt);
		return std::nullopt;
	}
	User u;
	u.id = sqlite3_column_int(stmt, 0);
	u.username = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
	u.passwordHash = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
	const auto* phone = sqlite3_column_text(stmt, 3);
	u.phone = phone ? reinterpret_cast<const char*>(phone) : "";
	const auto* created = sqlite3_column_text(stmt, 4);
	u.createdAt = created ? reinterpret_cast<const char*>(created) : "";
	sqlite3_finalize(stmt);
	return u;
}

std::optional<Merchant> Database::getMerchantByUsername(const std::string& username, std::string& errMsg) {
	const char* sql = "SELECT id, username, password_hash, store_name, created_at FROM merchants WHERE username = ?;";
	sqlite3_stmt* stmt = nullptr;
	if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
		errMsg = sqlite3_errmsg(db);
		return std::nullopt;
	}
	sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
	if (sqlite3_step(stmt) != SQLITE_ROW) {
		sqlite3_finalize(stmt);
		return std::nullopt;
	}
	Merchant m;
	m.id = sqlite3_column_int(stmt, 0);
	m.username = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
	m.passwordHash = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
	const auto* store = sqlite3_column_text(stmt, 3);
	m.storeName = store ? reinterpret_cast<const char*>(store) : "";
	const auto* created = sqlite3_column_text(stmt, 4);
	m.createdAt = created ? reinterpret_cast<const char*>(created) : "";
	sqlite3_finalize(stmt);
	return m;
}

std::optional<User> Database::getUserById(int id, std::string& errMsg) {
	const char* sql = "SELECT id, username, password_hash, phone, created_at FROM users WHERE id = ?;";
	sqlite3_stmt* stmt = nullptr;
	if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
		errMsg = sqlite3_errmsg(db);
		return std::nullopt;
	}
	sqlite3_bind_int(stmt, 1, id);
	if (sqlite3_step(stmt) != SQLITE_ROW) {
		sqlite3_finalize(stmt);
		return std::nullopt;
	}
	User u;
	u.id = sqlite3_column_int(stmt, 0);
	u.username = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
	u.passwordHash = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
	const auto* phone = sqlite3_column_text(stmt, 3);
	u.phone = phone ? reinterpret_cast<const char*>(phone) : "";
	const auto* created = sqlite3_column_text(stmt, 4);
	u.createdAt = created ? reinterpret_cast<const char*>(created) : "";
	sqlite3_finalize(stmt);
	return u;
}

std::optional<Merchant> Database::getMerchantById(int id, std::string& errMsg) {
	const char* sql = "SELECT id, username, password_hash, store_name, created_at FROM merchants WHERE id = ?;";
	sqlite3_stmt* stmt = nullptr;
	if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
		errMsg = sqlite3_errmsg(db);
		return std::nullopt;
	}
	sqlite3_bind_int(stmt, 1, id);
	if (sqlite3_step(stmt) != SQLITE_ROW) {
		sqlite3_finalize(stmt);
		return std::nullopt;
	}
	Merchant m;
	m.id = sqlite3_column_int(stmt, 0);
	m.username = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
	m.passwordHash = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
	const auto* store = sqlite3_column_text(stmt, 3);
	m.storeName = store ? reinterpret_cast<const char*>(store) : "";
	const auto* created = sqlite3_column_text(stmt, 4);
	m.createdAt = created ? reinterpret_cast<const char*>(created) : "";
	sqlite3_finalize(stmt);
	return m;
}

bool Database::createSessionToken(const std::string& token, const std::optional<int>& userId, const std::optional<int>& merchantId, const std::string& expiresAt, std::string& errMsg) {
	const char* sql = "INSERT INTO sessions(token, user_id, merchant_id, expires_at) VALUES(?,?,?,?);";
	sqlite3_stmt* stmt = nullptr;
	if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
		errMsg = sqlite3_errmsg(db);
		return false;
	}
	sqlite3_bind_text(stmt, 1, token.c_str(), -1, SQLITE_STATIC);
	if (userId) {
		sqlite3_bind_int(stmt, 2, *userId);
	} else {
		sqlite3_bind_null(stmt, 2);
	}
	if (merchantId) {
		sqlite3_bind_int(stmt, 3, *merchantId);
	} else {
		sqlite3_bind_null(stmt, 3);
	}
	sqlite3_bind_text(stmt, 4, expiresAt.c_str(), -1, SQLITE_STATIC);
	bool ok = sqlite3_step(stmt) == SQLITE_DONE;
	if (!ok) {
		errMsg = sqlite3_errmsg(db);
	}
	sqlite3_finalize(stmt);
	return ok;
}

std::optional<Session> Database::getSessionByToken(const std::string& token, std::string& errMsg) {
	const char* sql = "SELECT id, token, user_id, merchant_id, expires_at, created_at FROM sessions WHERE token = ? AND expires_at > CURRENT_TIMESTAMP;";
	sqlite3_stmt* stmt = nullptr;
	if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
		errMsg = sqlite3_errmsg(db);
		return std::nullopt;
	}
	sqlite3_bind_text(stmt, 1, token.c_str(), -1, SQLITE_STATIC);
	if (sqlite3_step(stmt) != SQLITE_ROW) {
		sqlite3_finalize(stmt);
		return std::nullopt;
	}
	Session s;
	s.id = sqlite3_column_int(stmt, 0);
	s.token = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
	if (sqlite3_column_type(stmt, 2) != SQLITE_NULL) {
		s.userId = sqlite3_column_int(stmt, 2);
	}
	if (sqlite3_column_type(stmt, 3) != SQLITE_NULL) {
		s.merchantId = sqlite3_column_int(stmt, 3);
	}
	s.expiresAt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
	const auto* created = sqlite3_column_text(stmt, 5);
	s.createdAt = created ? reinterpret_cast<const char*>(created) : "";
	sqlite3_finalize(stmt);
	return s;
}

std::optional<int> Database::createOrder(const std::vector<OrderItem>& items, const std::optional<int>& userId, std::string& errMsg) {
	if (items.empty()) {
		errMsg = "Order items cannot be empty";
		return std::nullopt;
	}

	char* em = nullptr;
	if (sqlite3_exec(db, "BEGIN IMMEDIATE;", nullptr, nullptr, &em) != SQLITE_OK) {
		if (em) { errMsg = em; sqlite3_free(em); }
		return std::nullopt;
	}

	const char* insOrderSql = "INSERT INTO orders(user_id, status, total) VALUES(?, 'pending', 0);";
	sqlite3_stmt* insOrderStmt = nullptr;
	if (sqlite3_prepare_v2(db, insOrderSql, -1, &insOrderStmt, nullptr) != SQLITE_OK) {
		errMsg = sqlite3_errmsg(db);
		sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
		return std::nullopt;
	}
	if (userId) {
		sqlite3_bind_int(insOrderStmt, 1, *userId);
	} else {
		sqlite3_bind_null(insOrderStmt, 1);
	}
	if (sqlite3_step(insOrderStmt) != SQLITE_DONE) {
		errMsg = sqlite3_errmsg(db);
		sqlite3_finalize(insOrderStmt);
		sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
		return std::nullopt;
	}
	sqlite3_finalize(insOrderStmt);
	const int orderId = static_cast<int>(sqlite3_last_insert_rowid(db));

	sqlite3_stmt* priceStmt = nullptr;
	const char* priceSql = "SELECT price FROM dishes WHERE id = ? AND is_available = 1;";
	if (sqlite3_prepare_v2(db, priceSql, -1, &priceStmt, nullptr) != SQLITE_OK) {
		errMsg = sqlite3_errmsg(db);
		sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
		return std::nullopt;
	}

	sqlite3_stmt* insItemStmt = nullptr;
	const char* insItemSql = "INSERT INTO order_items(order_id, dish_id, quantity, unit_price) VALUES(?,?,?,?);";
	if (sqlite3_prepare_v2(db, insItemSql, -1, &insItemStmt, nullptr) != SQLITE_OK) {
		errMsg = sqlite3_errmsg(db);
		sqlite3_finalize(priceStmt);
		sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
		return std::nullopt;
	}

	double total = 0.0;
	for (const auto& item : items) {
		sqlite3_bind_int(priceStmt, 1, item.dishId);
		if (sqlite3_step(priceStmt) != SQLITE_ROW) {
			errMsg = "Dish not available";
			sqlite3_finalize(priceStmt);
			sqlite3_finalize(insItemStmt);
			sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
			return std::nullopt;
		}
		const double unitPrice = sqlite3_column_double(priceStmt, 0);
		sqlite3_reset(priceStmt);
		sqlite3_clear_bindings(priceStmt);

		sqlite3_bind_int(insItemStmt, 1, orderId);
		sqlite3_bind_int(insItemStmt, 2, item.dishId);
		sqlite3_bind_int(insItemStmt, 3, item.quantity);
		sqlite3_bind_double(insItemStmt, 4, unitPrice);
		if (sqlite3_step(insItemStmt) != SQLITE_DONE) {
			errMsg = sqlite3_errmsg(db);
			sqlite3_finalize(priceStmt);
			sqlite3_finalize(insItemStmt);
			sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
			return std::nullopt;
		}
		sqlite3_reset(insItemStmt);
		sqlite3_clear_bindings(insItemStmt);
		total += unitPrice * item.quantity;
	}
	sqlite3_finalize(priceStmt);
	sqlite3_finalize(insItemStmt);

	sqlite3_stmt* upd = nullptr;
	const char* updSql = "UPDATE orders SET total = ?, updated_at = CURRENT_TIMESTAMP WHERE id = ?;";
	if (sqlite3_prepare_v2(db, updSql, -1, &upd, nullptr) != SQLITE_OK) {
		errMsg = sqlite3_errmsg(db);
		sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
		return std::nullopt;
	}
	sqlite3_bind_double(upd, 1, total);
	sqlite3_bind_int(upd, 2, orderId);
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
	const char* selOrder = "SELECT status,total,user_id,pickup_notified,created_at,updated_at FROM orders WHERE id = ?;";
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
	if (sqlite3_column_type(st, 2) != SQLITE_NULL) {
		order.userId = sqlite3_column_int(st, 2);
	}
	order.pickupNotified = sqlite3_column_int(st, 3) != 0;
	const auto* created = sqlite3_column_text(st, 4);
	order.createdAt = created ? reinterpret_cast<const char*>(created) : "";
	const auto* updated = sqlite3_column_text(st, 5);
	order.updatedAt = updated ? reinterpret_cast<const char*>(updated) : "";
	sqlite3_finalize(st);

	// items
	const char* selItems = "SELECT dish_id, quantity, unit_price FROM order_items WHERE order_id = ? ORDER BY id;";
	if (sqlite3_prepare_v2(db, selItems, -1, &st, nullptr) != SQLITE_OK) {
		errMsg = sqlite3_errmsg(db);
		return std::nullopt;
	}
	sqlite3_bind_int(st, 1, orderId);
	while (sqlite3_step(st) == SQLITE_ROW) {
		OrderItem it;
		it.dishId = sqlite3_column_int(st, 0);
		it.quantity = sqlite3_column_int(st, 1);
		it.unitPrice = sqlite3_column_double(st, 2);
		order.items.push_back(it);
	}
	sqlite3_finalize(st);
	return order;
}

std::vector<Order> Database::getAllOrders(std::string& errMsg) {
	std::vector<Order> result;
	const char* sql = "SELECT id, status, total, user_id, pickup_notified, created_at, updated_at FROM orders ORDER BY id DESC;";
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
		if (sqlite3_column_type(stmt, 3) != SQLITE_NULL) {
			order.userId = sqlite3_column_int(stmt, 3);
		}
		order.pickupNotified = sqlite3_column_int(stmt, 4) != 0;
		const auto* created = sqlite3_column_text(stmt, 5);
		order.createdAt = created ? reinterpret_cast<const char*>(created) : "";
		const auto* updated = sqlite3_column_text(stmt, 6);
		order.updatedAt = updated ? reinterpret_cast<const char*>(updated) : "";

		const char* selItems = "SELECT dish_id, quantity, unit_price FROM order_items WHERE order_id = ? ORDER BY id;";
		sqlite3_stmt* itemStmt = nullptr;
		if (sqlite3_prepare_v2(db, selItems, -1, &itemStmt, nullptr) == SQLITE_OK) {
			sqlite3_bind_int(itemStmt, 1, order.id);
			while (sqlite3_step(itemStmt) == SQLITE_ROW) {
				OrderItem it;
				it.dishId = sqlite3_column_int(itemStmt, 0);
				it.quantity = sqlite3_column_int(itemStmt, 1);
				it.unitPrice = sqlite3_column_double(itemStmt, 2);
				order.items.push_back(it);
			}
			sqlite3_finalize(itemStmt);
		}

		result.push_back(std::move(order));
	}
	sqlite3_finalize(stmt);
	return result;
}

std::vector<Order> Database::getOrdersByUser(int userId, std::string& errMsg) {
	std::vector<Order> result;
	const char* sql = "SELECT id, status, total, user_id, pickup_notified, created_at, updated_at FROM orders WHERE user_id = ? ORDER BY id DESC;";
	sqlite3_stmt* stmt = nullptr;
	if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
		errMsg = sqlite3_errmsg(db);
		return result;
	}
	sqlite3_bind_int(stmt, 1, userId);

	while (sqlite3_step(stmt) == SQLITE_ROW) {
		Order order;
		order.id = sqlite3_column_int(stmt, 0);
		order.status = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
		order.total = sqlite3_column_double(stmt, 2);
		order.userId = sqlite3_column_int(stmt, 3);
		order.pickupNotified = sqlite3_column_int(stmt, 4) != 0;
		const auto* created = sqlite3_column_text(stmt, 5);
		order.createdAt = created ? reinterpret_cast<const char*>(created) : "";
		const auto* updated = sqlite3_column_text(stmt, 6);
		order.updatedAt = updated ? reinterpret_cast<const char*>(updated) : "";

		const char* selItems = "SELECT dish_id, quantity, unit_price FROM order_items WHERE order_id = ? ORDER BY id;";
		sqlite3_stmt* itemStmt = nullptr;
		if (sqlite3_prepare_v2(db, selItems, -1, &itemStmt, nullptr) == SQLITE_OK) {
			sqlite3_bind_int(itemStmt, 1, order.id);
			while (sqlite3_step(itemStmt) == SQLITE_ROW) {
				OrderItem it;
				it.dishId = sqlite3_column_int(itemStmt, 0);
				it.quantity = sqlite3_column_int(itemStmt, 1);
				it.unitPrice = sqlite3_column_double(itemStmt, 2);
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
	const char* sql = "UPDATE orders SET status = ?, updated_at = CURRENT_TIMESTAMP WHERE id = ?;";
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

bool Database::markOrderPickupNotified(int orderId, std::string& errMsg) {
	const char* sql = "UPDATE orders SET pickup_notified = 1, updated_at = CURRENT_TIMESTAMP WHERE id = ?;";
	sqlite3_stmt* stmt = nullptr;
	if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
		errMsg = sqlite3_errmsg(db);
		return false;
	}
	sqlite3_bind_int(stmt, 1, orderId);
	if (sqlite3_step(stmt) != SQLITE_DONE) {
		errMsg = sqlite3_errmsg(db);
		sqlite3_finalize(stmt);
		return false;
	}
	sqlite3_finalize(stmt);
	return true;
}


