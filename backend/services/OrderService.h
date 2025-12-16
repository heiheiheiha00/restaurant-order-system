#pragma once
#include <optional>
#include <string>
#include <vector>
#include "../models/Order.h"

class OrderService {
public:
	std::optional<int> createOrder(const std::vector<OrderItem>& items, const std::optional<int>& userId, std::string& errMsg);
	std::optional<Order> getOrder(int id, std::string& errMsg);
	std::vector<Order> getAllOrders(std::string& errMsg);
	std::vector<Order> getOrdersByUser(int userId, std::string& errMsg);
	bool updateOrderStatus(int id, const std::string& status, std::string& errMsg);
	bool markPickupNotified(int id, std::string& errMsg);
};


