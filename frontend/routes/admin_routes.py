from flask import Blueprint, render_template, request, redirect, url_for, flash
from services.api_client import fetch_all_orders, update_order_status, fetch_menu
import requests

admin_bp = Blueprint("admin", __name__, url_prefix="/admin")


@admin_bp.route("/orders")
def orders_list():
	try:
		# 获取订单列表
		orders = fetch_all_orders()
		print(f"DEBUG: orders type = {type(orders)}, value = {orders}")
		if not isinstance(orders, list):
			flash(f"后端返回的数据格式错误: {type(orders)}", "error")
			orders = []
		
		# 获取菜单（用于显示菜品名称）
		menu_map = {}
		try:
			menu = fetch_menu()
			print(f"DEBUG: menu type = {type(menu)}, is callable = {callable(menu)}")
			if callable(menu):
				flash(f"菜单数据是函数对象，不是列表", "error")
				menu = []
			if isinstance(menu, list):
				try:
					menu_map = {item["id"]: item for item in menu}
					print(f"DEBUG: menu_map created, size = {len(menu_map)}")
				except Exception as e:
					print(f"DEBUG: Error creating menu_map: {e}")
					flash(f"创建菜单映射失败: {str(e)}", "error")
			else:
				flash(f"菜单数据格式错误: 期望列表，得到 {type(menu)}", "error")
		except Exception as e:
			print(f"DEBUG: Exception in menu fetch: {e}")
			import traceback
			print(traceback.format_exc())
			flash(f"获取菜单失败: {str(e)}", "error")
		
		# Enrich orders with dish names
		for order in orders:
			if not isinstance(order, dict):
				continue
			order_items = order.get("items", [])
			if not isinstance(order_items, list):
				order_items = []
			for item in order_items:
				if not isinstance(item, dict):
					continue
				dish_id = item.get("dishId")
				if dish_id in menu_map:
					item["dishName"] = menu_map[dish_id]["name"]
					item["dishPrice"] = menu_map[dish_id]["price"]
		
		# Group orders by status
		orders_by_status = {
			"pending": [],
			"preparing": [],
			"ready": [],
			"completed": []
		}
		for order in orders:
			if not isinstance(order, dict):
				continue
			status = order.get("status", "pending")
			if isinstance(status, str) and status in orders_by_status:
				orders_by_status[status].append(order)
		
		# 如果没有订单，不显示错误，只显示空状态
		
		return render_template("admin.html", orders_by_status=orders_by_status, all_orders=orders)
	except requests.exceptions.ConnectionError as e:
		flash(f"无法连接到后端服务，请确保后端服务已启动（http://127.0.0.1:8081）", "error")
		return render_template("admin.html", orders_by_status={}, all_orders=[])
	except requests.exceptions.HTTPError as e:
		error_msg = str(e)
		if hasattr(e, 'response') and e.response is not None:
			try:
				error_detail = e.response.json()
				error_msg = error_detail.get("error", error_msg)
			except:
				error_msg = f"HTTP {e.response.status_code}: {e.response.text[:200]}"
		flash(f"后端API错误: {error_msg}", "error")
		return render_template("admin.html", orders_by_status={}, all_orders=[])
	except Exception as e:
		import traceback
		error_detail = traceback.format_exc()
		# 打印详细错误信息到控制台（用于调试）
		print(f"错误详情:\n{error_detail}")
		flash(f"加载订单失败: {str(e)}", "error")
		# 确保 orders_by_status 是一个字典，包含所有状态键
		empty_orders_by_status = {
			"pending": [],
			"preparing": [],
			"ready": [],
			"completed": []
		}
		return render_template("admin.html", orders_by_status=empty_orders_by_status, all_orders=[])


@admin_bp.route("/orders/<int:order_id>/status", methods=["POST"])
def update_status(order_id):
	try:
		status = request.form.get("status")
		if not status:
			flash("状态不能为空", "error")
			return redirect(url_for("admin.orders_list"))
		
		update_order_status(order_id, status)
		flash(f"订单 #{order_id} 状态已更新为: {status}", "success")
	except Exception as e:
		flash(f"更新状态失败: {str(e)}", "error")
	
	return redirect(url_for("admin.orders_list"))


