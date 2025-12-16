#include "MenuService.h"
#include "../database/Database.h"

std::vector<Dish> MenuService::getMenu(std::string& errMsg) {
	return Database::instance().getAllDishes(errMsg);
}

std::optional<Dish> MenuService::getDish(int dishId, std::string& errMsg) {
	return Database::instance().getDish(dishId, errMsg);
}

std::optional<int> MenuService::createDish(const Dish& dish, std::string& errMsg) {
	return Database::instance().createDish(dish, errMsg);
}

bool MenuService::updateDish(int dishId,
	const std::optional<std::string>& name,
	const std::optional<std::string>& description,
	const std::optional<std::string>& category,
	const std::optional<double>& price,
	const std::optional<bool>& isAvailable,
	std::string& errMsg) {
	return Database::instance().updateDish(dishId, name, description, category, price, isAvailable, errMsg);
}


