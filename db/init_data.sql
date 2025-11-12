INSERT INTO dishes (id, name, price) VALUES
	(1, 'Margherita Pizza', 8.5),
	(2, 'Caesar Salad', 6.0),
	(3, 'Spaghetti Bolognese', 9.5)
ON CONFLICT(id) DO NOTHING;


