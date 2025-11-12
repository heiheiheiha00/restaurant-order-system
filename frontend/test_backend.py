#!/usr/bin/env python
"""测试后端API连接"""
import requests
import json

BACKEND_URL = "http://127.0.0.1:8081"

def test_backend():
	print("=" * 50)
	print("测试后端API连接")
	print("=" * 50)
	
	# 1. 测试健康检查
	print("\n1. 测试健康检查...")
	try:
		resp = requests.get(f"{BACKEND_URL}/health", timeout=5)
		print(f"   状态码: {resp.status_code}")
		print(f"   响应: {resp.json()}")
		if resp.status_code == 200:
			print("   ✅ 健康检查通过")
		else:
			print("   ❌ 健康检查失败")
	except Exception as e:
		print(f"   ❌ 连接失败: {e}")
		return
	
	# 2. 测试菜单API
	print("\n2. 测试菜单API...")
	try:
		resp = requests.get(f"{BACKEND_URL}/menu", timeout=5)
		print(f"   状态码: {resp.status_code}")
		menu = resp.json()
		print(f"   菜单项数量: {len(menu)}")
		if menu:
			print(f"   第一个菜品: {menu[0]}")
		print("   ✅ 菜单API正常")
	except Exception as e:
		print(f"   ❌ 菜单API失败: {e}")
	
	# 3. 测试商家订单API
	print("\n3. 测试商家订单API...")
	try:
		resp = requests.get(f"{BACKEND_URL}/admin/orders", timeout=5)
		print(f"   状态码: {resp.status_code}")
		orders = resp.json()
		print(f"   订单数量: {len(orders)}")
		if orders:
			print(f"   第一个订单: {json.dumps(orders[0], indent=2, ensure_ascii=False)}")
		else:
			print("   ⚠️  当前没有订单（这是正常的，如果没有下单）")
		print("   ✅ 商家订单API正常")
	except Exception as e:
		print(f"   ❌ 商家订单API失败: {e}")
		if hasattr(e, 'response') and e.response is not None:
			print(f"   响应状态码: {e.response.status_code}")
			print(f"   响应内容: {e.response.text[:200]}")
	
	print("\n" + "=" * 50)
	print("测试完成")
	print("=" * 50)

if __name__ == "__main__":
	test_backend()

