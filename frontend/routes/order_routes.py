from flask import Blueprint, request, redirect, url_for, render_template, session, flash
from services.api_client import create_order, fetch_order, fetch_menu
import requests

order_bp = Blueprint("order", __name__, url_prefix="/order")


@order_bp.post("/create")
def create():
	user = session.get("user")
	if not user:
		return redirect(url_for("auth.user_login_view", next=url_for("cart.view_cart")))

	cart = session.get("cart", {})
	items = []
	for did, qty in cart.items():
		try:
			dish_id = int(did)
			quantity = int(qty)
		except (TypeError, ValueError):
			continue
		if quantity > 0:
			items.append({"dishId": dish_id, "quantity": quantity})

	if not items:
		flash("请先选择菜品再下单", "error")
		return redirect(url_for("menu.menu_page"))
	try:
		order = create_order(items, user["token"])
	except requests.RequestException as exc:
		flash(f"下单失败：{exc}", "error")
		return redirect(url_for("cart.view_cart"))
	session.pop("cart", None)
	flash(f"订单已创建，编号 #{order['id']}", "success")
	return redirect(url_for("order.status", order_id=order["id"]))


@order_bp.get("/<int:order_id>")
def status(order_id: int):
	user = session.get("user")
	if not user:
		return redirect(url_for("auth.user_login_view", next=url_for("order.status", order_id=order_id)))
	try:
		order = fetch_order(order_id, user["token"])
		menu = fetch_menu()
	except requests.RequestException as exc:
		flash(f"查询订单失败：{exc}", "error")
		return redirect(url_for("menu.menu_page"))
	menu_map = {item["id"]: item for item in menu}
	items = []
	for it in order.get("items", []):
		info = menu_map.get(it["dishId"])
		items.append({
			"dishId": it["dishId"],
			"name": info["name"] if info else f"菜品 {it['dishId']}",
			"quantity": it["quantity"],
			"price": info["price"] if info else None,
			"subtotal": (info["price"] * it["quantity"]) if info else None,
		})
	pickup_ready = order.get("status") == "completed" and not order.get("pickupNotified", False)
	return render_template("order_status.html", order=order, order_items=items, pickup_ready=pickup_ready)


