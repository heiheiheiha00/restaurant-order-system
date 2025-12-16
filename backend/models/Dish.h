#pragma once
#include <string>

struct Dish {
	int id;
	std::string name;
	std::string description;
	std::string category;
	double price;
	bool isAvailable;
};


