from flask import Blueprint, render_template, session, request, redirect, url_for, flash
from services.api_client import fetch_menu
import requests

menu_bp = Blueprint("menu", __name__)


@menu_bp.get("/")
def menu_page():
	try:
		menu = fetch_menu()
	except requests.RequestException as exc:
		flash(f"加载菜单失败：{exc}", "error")
		menu = []
	cart = session.get("cart", {})
	total_items = sum(cart.values())
	return render_template("menu.html", menu=menu, cart_count=total_items)


@menu_bp.post("/cart/add")
def add_to_cart_from_menu():
	dish_id = request.form.get("dish_id")
	quantity = request.form.get("quantity", "1")
	try:
		dish_id_int = int(dish_id)
		quantity_int = max(1, int(quantity))
	except (TypeError, ValueError):
		flash("无效的菜品或数量", "error")
		return redirect(url_for("menu.menu_page"))

	cart = session.setdefault("cart", {})
	key = str(dish_id_int)
	cart[key] = cart.get(key, 0) + quantity_int
	session["cart"] = cart
	flash("菜品已加入购物车", "success")
	return redirect(url_for("menu.menu_page"))


