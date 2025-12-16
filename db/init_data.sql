INSERT INTO dishes (id, name, description, category, price, is_available) VALUES
	(1, 'Margherita Pizza', 'Classic tomato, mozzarella and basil', 'Pizza', 8.5, 1),
	(2, 'Caesar Salad', 'Romaine lettuce with parmesan and croutons', 'Salad', 6.0, 1),
	(3, 'Spaghetti Bolognese', 'Slow cooked beef ragu', 'Pasta', 9.5, 1),
	(4, 'Cheeseburger', 'Beef patty, cheddar, pickles', 'Burger', 7.5, 1),
	(5, 'Chicken Caesar Salad', 'Grilled chicken with Caesar dressing', 'Salad', 8.0, 1),
	(6, 'Vegetable Stir Fry', 'Seasonal veggies with soy glaze', 'Wok', 6.5, 1),
	(7, 'Fish and Chips', 'Beer battered cod with fries', 'Seafood', 10.0, 1),
	(8, 'Tiramisu', 'Espresso soaked ladyfingers and mascarpone', 'Dessert', 5.5, 1),
	(9, 'Lemonade', 'Freshly squeezed lemon juice', 'Drinks', 3.0, 1),
	(10, 'Tomato Soup', 'Roasted tomato soup with basil oil', 'Soup', 5.0, 1)
ON CONFLICT(id) DO NOTHING;


