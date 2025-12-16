import requests
from flask import current_app


def get_backend_base() -> str:
	return current_app.config.get("BACKEND_BASE_URL")


def _request(method: str, path: str, token: str | None = None, **kwargs):
	url = f"{get_backend_base()}{path}"
	headers = kwargs.pop("headers", {})
	if token:
		headers["Authorization"] = f"Bearer {token}"
	resp = requests.request(method, url, headers=headers, timeout=5, **kwargs)
	resp.raise_for_status()
	if not resp.content:
		return None
	try:
		return resp.json()
	except ValueError:
		return resp.text


def fetch_menu():
	result = _request("GET", "/menu") or []
	if not isinstance(result, list):
		raise ValueError(f"菜单API返回的不是列表: {type(result)}")
	return result


def create_order(items, token: str):
	return _request("POST", "/orders", token=token, json={"items": items})


def fetch_order(order_id: int, token: str):
	return _request("GET", f"/orders/{order_id}", token=token)


def fetch_my_orders(token: str):
	result = _request("GET", "/me/orders", token=token) or []
	if not isinstance(result, list):
		raise ValueError("个人订单接口返回的数据格式不正确")
	return result


def acknowledge_pickup(order_id: int, token: str):
	return _request("POST", f"/orders/{order_id}/pickup-ack", token=token)


def user_register(username: str, password: str, phone: str):
	return _request("POST", "/auth/user/register", json={
		"username": username,
		"password": password,
		"phone": phone
	})


def user_login(username: str, password: str):
	return _request("POST", "/auth/user/login", json={
		"username": username,
		"password": password
	})


def merchant_register(username: str, password: str, store_name: str):
	return _request("POST", "/auth/merchant/register", json={
		"username": username,
		"password": password,
		"storeName": store_name
	})


def merchant_login(username: str, password: str):
	return _request("POST", "/auth/merchant/login", json={
		"username": username,
		"password": password
	})


def fetch_all_orders(token: str):
	result = _request("GET", "/admin/orders", token=token) or []
	if not isinstance(result, list):
		raise ValueError("订单接口返回的数据格式不正确")
	return result


def update_order_status(order_id: int, status: str, token: str):
	return _request("PATCH", f"/admin/orders/{order_id}/status", token=token, json={"status": status})


def fetch_admin_menu(token: str):
	result = _request("GET", "/admin/menu", token=token) or []
	if not isinstance(result, list):
		raise ValueError("菜单接口返回的数据格式不正确")
	return result


def create_admin_dish(payload: dict, token: str):
	return _request("POST", "/admin/menu", token=token, json=payload)


def update_admin_dish(dish_id: int, payload: dict, token: str):
	return _request("PATCH", f"/admin/menu/{dish_id}", token=token, json=payload)


