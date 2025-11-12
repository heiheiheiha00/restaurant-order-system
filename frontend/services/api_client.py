import requests
from flask import current_app


def get_backend_base() -> str:
	return current_app.config.get("BACKEND_BASE_URL")


def fetch_menu():
	url = f"{get_backend_base()}/menu"
	resp = requests.get(url, timeout=5)
	resp.raise_for_status()
	result = resp.json()
	# 确保返回的是列表
	if not isinstance(result, list):
		raise ValueError(f"菜单API返回的不是列表: {type(result)}")
	return result


def create_order(items):
	url = f"{get_backend_base()}/orders"
	resp = requests.post(url, json={"items": items}, timeout=5)
	resp.raise_for_status()
	return resp.json()


def fetch_order(order_id: int):
	url = f"{get_backend_base()}/orders/{order_id}"
	resp = requests.get(url, timeout=5)
	resp.raise_for_status()
	return resp.json()


def fetch_all_orders():
	url = f"{get_backend_base()}/admin/orders"
	try:
		resp = requests.get(url, timeout=5)
		resp.raise_for_status()
		result = resp.json()
		# 确保返回的是列表
		if not isinstance(result, list):
			raise ValueError(f"订单API返回的不是列表: {type(result)}, value: {result}")
		return result
	except requests.exceptions.HTTPError as e:
		# 获取详细的错误信息
		try:
			error_detail = e.response.json()
			error_msg = error_detail.get("error", str(e))
		except:
			error_msg = f"HTTP {e.response.status_code}: {e.response.text}"
		raise requests.exceptions.HTTPError(f"{error_msg} (URL: {url})", response=e.response)


def update_order_status(order_id: int, status: str):
	url = f"{get_backend_base()}/admin/orders/{order_id}/status"
	resp = requests.patch(url, json={"status": status}, timeout=5)
	resp.raise_for_status()
	return resp.json()


