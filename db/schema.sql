-- SQLite schema for restaurant-order-system
PRAGMA foreign_keys = ON;

CREATE TABLE IF NOT EXISTS dishes (
	id INTEGER PRIMARY KEY,
	name TEXT NOT NULL,
	price REAL NOT NULL
);

CREATE TABLE IF NOT EXISTS orders (
	id INTEGER PRIMARY KEY,
	status TEXT NOT NULL DEFAULT 'pending',
	total REAL NOT NULL DEFAULT 0
);

CREATE TABLE IF NOT EXISTS order_items (
	id INTEGER PRIMARY KEY,
	order_id INTEGER NOT NULL,
	dish_id INTEGER NOT NULL,
	quantity INTEGER NOT NULL,
	FOREIGN KEY(order_id) REFERENCES orders(id) ON DELETE CASCADE,
	FOREIGN KEY(dish_id) REFERENCES dishes(id)
);


