#pragma once
#include <vector>
#include <string>
#include "../models/Dish.h"

class MenuService {
public:
	std::vector<Dish> getMenu(std::string& errMsg);
};


