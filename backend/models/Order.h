#pragma once
#include <vector>

struct OrderItem {
	int dishId;
	int quantity;
};

struct Order {
	int id;
	std::vector<OrderItem> items;
	std::string status;
	double total;
};


