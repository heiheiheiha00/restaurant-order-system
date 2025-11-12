#pragma once
#include <string>
#include <vector>
#include <optional>
#include <sqlite3.h>
#include "../models/Dish.h"
#include "../models/Order.h"

class Database {
public:
	static Database& instance();

	bool open(const std::string& path, std::string& errMsg);
	void close();
	bool initializeSchema(std::string& errMsg);
	bool hasInitialData();
	bool insertInitialData(std::string& errMsg);

	std::vector<Dish> getAllDishes(std::string& errMsg);

	// Returns created order id
	std::optional<int> createOrder(const std::vector<OrderItem>& items, std::string& errMsg);
	std::optional<Order> getOrder(int orderId, std::string& errMsg);
	std::vector<Order> getAllOrders(std::string& errMsg);
	bool updateOrderStatus(int orderId, const std::string& status, std::string& errMsg);

private:
	Database() = default;
	~Database();
	Database(const Database&) = delete;
	Database& operator=(const Database&) = delete;

	sqlite3* db{nullptr};
};


