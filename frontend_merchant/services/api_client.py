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


