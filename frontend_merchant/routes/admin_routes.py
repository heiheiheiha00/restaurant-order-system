from flask import Blueprint, render_template, request, redirect, url_for, flash, session
from services.api_client import (
	fetch_all_orders,
	update_order_status,
	fetch_admin_menu,
	create_admin_dish,
	update_admin_dish
)
import requests

admin_bp = Blueprint("admin", __name__, url_prefix="/admin")


def _require_merchant(next_url):
	merchant = session.get("merchant")
	if not merchant:
		flash("请先登录商家账号", "error")
		return redirect(url_for("auth.merchant_login_view", next=next_url))
	return merchant


@admin_bp.route("/orders")
def orders_list():
	merchant = session.get("merchant")
	if not merchant:
		return redirect(url_for("auth.merchant_login_view", next=url_for("admin.orders_list")))
	token = merchant["token"]
	try:
		orders = fetch_all_orders(token)
		if not isinstance(orders, list):
			flash(f"后端返回的数据格式错误: {type(orders)}", "error")
			orders = []

		menu_map = {}
		try:
			menu = fetch_admin_menu(token)
			if isinstance(menu, list):
				menu_map = {item["id"]: item for item in menu}
		except Exception as e:
			flash(f"获取菜单失败: {str(e)}", "error")

		for order in orders:
			if not isinstance(order, dict):
				continue
			items = order.get("items", [])
			if isinstance(items, list):
				for item in items:
					if not isinstance(item, dict):
						continue
					dish_id = item.get("dishId")
					if dish_id in menu_map:
						item["dishName"] = menu_map[dish_id]["name"]
						item["dishPrice"] = menu_map[dish_id]["price"]

		orders_by_status = {status: [] for status in ["pending", "preparing", "ready", "completed"]}
		for order in orders:
			if not isinstance(order, dict):
				continue
			status = order.get("status", "pending")
			if status in orders_by_status:
				orders_by_status[status].append(order)

		return render_template("admin.html", orders_by_status=orders_by_status, all_orders=orders)
	except requests.exceptions.ConnectionError:
		flash("无法连接到后端服务，请确保后端已启动（http://127.0.0.1:8081）", "error")
		return render_template("admin.html", orders_by_status={}, all_orders=[])
	except requests.exceptions.HTTPError as e:
		error_msg = str(e)
		if hasattr(e, "response") and e.response is not None:
			try:
				error_detail = e.response.json()
				error_msg = error_detail.get("error", error_msg)
			except Exception:
				error_msg = f"HTTP {e.response.status_code}: {e.response.text[:200]}"
		flash(f"后端API错误: {error_msg}", "error")
		return render_template("admin.html", orders_by_status={}, all_orders=[])
	except Exception as e:
		flash(f"加载订单失败: {str(e)}", "error")
		empty = {status: [] for status in ["pending", "preparing", "ready", "completed"]}
		return render_template("admin.html", orders_by_status=empty, all_orders=[])


@admin_bp.route("/orders/<int:order_id>/status", methods=["POST"])
def update_status(order_id):
	merchant = session.get("merchant")
	if not merchant:
		return redirect(url_for("auth.merchant_login_view", next=url_for("admin.orders_list")))
	status = request.form.get("status")
	if not status:
		flash("状态不能为空", "error")
		return redirect(url_for("admin.orders_list"))
	try:
		update_order_status(order_id, status, merchant["token"])
		flash(f"订单 #{order_id} 状态已更新为: {status}", "success")
	except Exception as e:
		flash(f"更新状态失败: {str(e)}", "error")
	return redirect(url_for("admin.orders_list"))


@admin_bp.route("/menu/manage")
def menu_manage():
	merchant = _require_merchant(url_for("admin.menu_manage"))
	if not merchant or isinstance(merchant, str):
		return merchant
	try:
		menu = fetch_admin_menu(merchant["token"])
	except requests.RequestException as exc:
		flash(f"加载菜单失败：{exc}", "error")
		menu = []
	return render_template("admin_menu.html", menu=menu)


@admin_bp.route("/menu/add", methods=["POST"])
def add_menu_item():
	merchant = _require_merchant(url_for("admin.menu_manage"))
	if not merchant or isinstance(merchant, str):
		return merchant
	name = request.form.get("name", "").strip()
	category = request.form.get("category", "").strip()
	description = request.form.get("description", "").strip()
	price = request.form.get("price", "").strip()
	is_available = request.form.get("is_available") == "on"
	if not name or not price:
		flash("菜品名称和价格不能为空", "error")
		return redirect(url_for("admin.menu_manage"))
	try:
		create_admin_dish({
			"name": name,
			"category": category,
			"description": description,
			"price": float(price),
			"isAvailable": is_available
		}, merchant["token"])
		flash("菜品已创建", "success")
	except Exception as exc:
		flash(f"创建菜品失败：{exc}", "error")
	return redirect(url_for("admin.menu_manage"))


@admin_bp.route("/menu/<int:dish_id>/update", methods=["POST"])
def update_menu_item(dish_id: int):
	merchant = _require_merchant(url_for("admin.menu_manage"))
	if not merchant or isinstance(merchant, str):
		return merchant
	payload = {}
	name = request.form.get("name", "").strip()
	category = request.form.get("category", "").strip()
	description = request.form.get("description", "").strip()
	price = request.form.get("price", "").strip()
	availability = request.form.get("is_available_choice")

	if name:
		payload["name"] = name
	if category:
		payload["category"] = category
	if description:
		payload["description"] = description
	if price:
		try:
			payload["price"] = float(price)
		except ValueError:
			flash("价格格式不正确", "error")
			return redirect(url_for("admin.menu_manage"))
	if availability == "true":
		payload["isAvailable"] = True
	elif availability == "false":
		payload["isAvailable"] = False

	if not payload:
		flash("没有需要更新的字段", "error")
		return redirect(url_for("admin.menu_manage"))

	try:
		update_admin_dish(dish_id, payload, merchant["token"])
		flash("菜品信息已更新", "success")
	except Exception as exc:
		flash(f"更新菜品失败：{exc}", "error")
	return redirect(url_for("admin.menu_manage"))


