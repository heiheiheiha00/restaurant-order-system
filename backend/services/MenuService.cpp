#include "MenuService.h"
#include "../database/Database.h"

std::vector<Dish> MenuService::getMenu(std::string& errMsg) {
	return Database::instance().getAllDishes(errMsg);
}


