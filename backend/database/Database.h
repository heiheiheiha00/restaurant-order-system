#pragma once
#include <string>
#include <vector>
#include <optional>
#include <sqlite3.h>
#include "../models/Dish.h"
#include "../models/Order.h"
#include "../models/User.h"
#include "../models/Merchant.h"
#include "../models/Session.h"

class Database {
public:
	static Database& instance();

	bool open(const std::string& path, std::string& errMsg);
	void close();
	bool initializeSchema(std::string& errMsg);
	bool hasInitialData();
	bool insertInitialData(std::string& errMsg);

	std::vector<Dish> getAllDishes(std::string& errMsg);
	std::optional<Dish> getDish(int dishId, std::string& errMsg);
	std::optional<int> createDish(const Dish& dish, std::string& errMsg);
	bool updateDish(int dishId,
		const std::optional<std::string>& name,
		const std::optional<std::string>& description,
		const std::optional<std::string>& category,
		const std::optional<double>& price,
		const std::optional<bool>& isAvailable,
		std::string& errMsg);

	// User & merchant management
	bool createUser(const std::string& username, const std::string& passwordHash, const std::string& phone, std::string& errMsg);
	bool createMerchant(const std::string& username, const std::string& passwordHash, const std::string& storeName, std::string& errMsg);
	std::optional<User> getUserByUsername(const std::string& username, std::string& errMsg);
	std::optional<Merchant> getMerchantByUsername(const std::string& username, std::string& errMsg);
	std::optional<User> getUserById(int id, std::string& errMsg);
	std::optional<Merchant> getMerchantById(int id, std::string& errMsg);

	// Session tokens
	bool createSessionToken(const std::string& token, const std::optional<int>& userId, const std::optional<int>& merchantId, const std::string& expiresAt, std::string& errMsg);
	std::optional<Session> getSessionByToken(const std::string& token, std::string& errMsg);

	// Returns created order id
	std::optional<int> createOrder(const std::vector<OrderItem>& items, const std::optional<int>& userId, std::string& errMsg);
	std::optional<Order> getOrder(int orderId, std::string& errMsg);
	std::vector<Order> getAllOrders(std::string& errMsg);
	std::vector<Order> getOrdersByUser(int userId, std::string& errMsg);
	bool updateOrderStatus(int orderId, const std::string& status, std::string& errMsg);
	bool markOrderPickupNotified(int orderId, std::string& errMsg);

private:
	Database() = default;
	~Database();
	Database(const Database&) = delete;
	Database& operator=(const Database&) = delete;

	sqlite3* db{nullptr};
};


