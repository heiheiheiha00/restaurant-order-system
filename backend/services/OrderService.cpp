#include "OrderService.h"
#include "../database/Database.h"

std::optional<int> OrderService::createOrder(const std::vector<OrderItem>& items, std::string& errMsg) {
	return Database::instance().createOrder(items, errMsg);
}

std::optional<Order> OrderService::getOrder(int id, std::string& errMsg) {
	return Database::instance().getOrder(id, errMsg);
}

std::vector<Order> OrderService::getAllOrders(std::string& errMsg) {
	return Database::instance().getAllOrders(errMsg);
}

bool OrderService::updateOrderStatus(int id, const std::string& status, std::string& errMsg) {
	return Database::instance().updateOrderStatus(id, status, errMsg);
}


