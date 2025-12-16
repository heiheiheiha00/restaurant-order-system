#pragma once
#include <vector>
#include <string>
#include <optional>
#include "../models/Dish.h"

class MenuService {
public:
	std::vector<Dish> getMenu(std::string& errMsg);
	std::optional<Dish> getDish(int dishId, std::string& errMsg);
	std::optional<int> createDish(const Dish& dish, std::string& errMsg);
	bool updateDish(int dishId,
		const std::optional<std::string>& name,
		const std::optional<std::string>& description,
		const std::optional<std::string>& category,
		const std::optional<double>& price,
		const std::optional<bool>& isAvailable,
		std::string& errMsg);
};


