from flask import Blueprint, render_template, request, redirect, url_for, flash, session
import requests
from services.api_client import merchant_login, merchant_register

auth_bp = Blueprint("auth", __name__)


def _next_url(default):
	return request.args.get("next") or request.form.get("next") or default


@auth_bp.route("/merchant/login", methods=["GET", "POST"])
def merchant_login_view():
	next_url = request.args.get("next") or request.form.get("next")
	if request.method == "POST":
		username = request.form.get("username", "").strip()
		password = request.form.get("password", "").strip()
		if not username or not password:
			flash("请输入商家账号与密码", "error")
			return render_template("auth_merchant_login.html", username=username, next_url=next_url)
		try:
			resp = merchant_login(username, password)
			session["merchant"] = {
				"token": resp.get("token"),
				"id": resp.get("merchantId"),
				"username": resp.get("username", username)
			}
			flash("商家登录成功", "success")
			return redirect(_next_url(url_for("admin.orders_list")))
		except requests.RequestException as exc:
			flash(f"登录失败：{exc}", "error")
	return render_template("auth_merchant_login.html", next_url=next_url)


@auth_bp.route("/merchant/register", methods=["GET", "POST"])
def merchant_register_view():
	if request.method == "POST":
		username = request.form.get("username", "").strip()
		password = request.form.get("password", "").strip()
		store_name = request.form.get("store_name", "").strip()
		if not username or not password or not store_name:
			flash("请填写商家账号、密码和门店名称", "error")
			return render_template("auth_merchant_register.html", username=username, store_name=store_name)
		try:
			merchant_register(username, password, store_name)
			flash("商家注册成功，请登录", "success")
			return redirect(url_for("auth.merchant_login_view"))
		except requests.RequestException as exc:
			flash(f"注册失败：{exc}", "error")
	return render_template("auth_merchant_register.html")


@auth_bp.route("/merchant/logout")
def merchant_logout():
	session.pop("merchant", None)
	flash("已退出商家账号", "info")
	return redirect(url_for("auth.merchant_login_view"))


