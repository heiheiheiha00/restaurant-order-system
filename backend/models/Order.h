#pragma once
#include <vector>
#include <string>
#include <optional>

struct OrderItem {
	int dishId;
	int quantity;
	double unitPrice;
};

struct Order {
	int id;
	std::optional<int> userId;
	std::vector<OrderItem> items;
	std::string status;
	double total;
	bool pickupNotified;
	std::string createdAt;
	std::string updatedAt;
};


