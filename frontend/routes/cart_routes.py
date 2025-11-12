from flask import Blueprint, session, redirect, url_for, request, render_template, flash
from services.api_client import fetch_menu
import requests

cart_bp = Blueprint("cart", __name__, url_prefix="/cart")


def _get_cart():
	return session.setdefault("cart", {})


def _cart_items_with_details():
	menu = fetch_menu()
	menu_map = {str(item["id"]): item for item in menu}
	cart = session.get("cart", {})
	items = []
	total = 0.0
	for dish_id, qty in cart.items():
		try:
			qty_int = int(qty)
		except (TypeError, ValueError):
			continue
		if qty_int <= 0:
			continue
		info = menu_map.get(str(dish_id))
		if not info:
			continue
		subtotal = info["price"] * qty_int
		total += subtotal
		items.append({
			"id": info["id"],
			"name": info["name"],
			"price": info["price"],
			"quantity": qty_int,
			"subtotal": subtotal,
		})
	return items, total


@cart_bp.get("/")
def view_cart():
	try:
		items, total = _cart_items_with_details()
	except requests.RequestException as exc:
		flash(f"加载购物车数据失败：{exc}", "error")
		items, total = [], 0.0
	return render_template("cart.html", cart_items=items, cart_total=total)


@cart_bp.post("/add")
def add_to_cart():
	dish_id = request.form.get("dish_id")
	quantity = request.form.get("quantity", "1")
	try:
		dish_id_int = int(dish_id)
		quantity_int = max(1, int(quantity))
	except (TypeError, ValueError):
		flash("无效的菜品或数量", "error")
		return redirect(url_for("menu.menu_page"))

	cart = _get_cart()
	key = str(dish_id_int)
	cart[key] = cart.get(key, 0) + quantity_int
	session["cart"] = cart
	flash("菜品已加入购物车", "success")
	return redirect(url_for("menu.menu_page"))


@cart_bp.post("/update")
def update_cart():
	cart = {}
	for key, value in request.form.items():
		if not key.startswith("qty_"):
			continue
		try:
			dish_id = int(key.split("_", 1)[1])
			qty = max(0, int(value))
		except (ValueError, IndexError):
			continue
		if qty > 0:
			cart[str(dish_id)] = qty
	session["cart"] = cart
	flash("购物车已更新", "success")
	return redirect(url_for("cart.view_cart"))


@cart_bp.post("/clear")
def clear_cart():
	session.pop("cart", None)
	flash("购物车已清空", "info")
	return redirect(url_for("cart.view_cart"))


