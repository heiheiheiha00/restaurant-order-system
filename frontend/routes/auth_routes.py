from flask import (
	Blueprint,
	render_template,
	request,
	redirect,
	url_for,
	flash,
	session
)
import requests
from services.api_client import (
	user_login,
	user_register,
	fetch_my_orders,
	acknowledge_pickup
)

auth_bp = Blueprint("auth", __name__)


def _next_url(default):
	return request.args.get("next") or request.form.get("next") or default


def _require_user():
	user = session.get("user")
	if not user:
		flash("请先登录用户账号", "error")
		return None
	return user


@auth_bp.route("/login", methods=["GET", "POST"])
def user_login_view():
	next_url = request.args.get("next") or request.form.get("next")
	if request.method == "POST":
		username = request.form.get("username", "").strip()
		password = request.form.get("password", "").strip()
		if not username or not password:
			flash("请输入账号和密码", "error")
			return render_template("auth_user_login.html", username=username, next_url=next_url)
		try:
			resp = user_login(username, password)
			session["user"] = {
				"token": resp.get("token"),
				"id": resp.get("userId"),
				"username": resp.get("username", username)
			}
			flash("登录成功", "success")
			return redirect(_next_url(url_for("menu.menu_page")))
		except requests.RequestException as exc:
			flash(f"登录失败：{exc}", "error")
	return render_template("auth_user_login.html", next_url=next_url)


@auth_bp.route("/register", methods=["GET", "POST"])
def user_register_view():
	next_url = request.args.get("next") or request.form.get("next")
	if request.method == "POST":
		username = request.form.get("username", "").strip()
		password = request.form.get("password", "").strip()
		phone = request.form.get("phone", "").strip()
		if not username or not password:
			flash("账号和密码不能为空", "error")
			return render_template("auth_user_register.html", username=username, phone=phone, next_url=next_url)
		try:
			user_register(username, password, phone)
			flash("注册成功，请登录", "success")
			return redirect(url_for("auth.user_login_view", next=next_url))
		except requests.RequestException as exc:
			flash(f"注册失败：{exc}", "error")
	return render_template("auth_user_register.html", next_url=next_url)


@auth_bp.route("/logout")
def user_logout():
	session.pop("user", None)
	flash("已退出用户账号", "info")
	return redirect(url_for("menu.menu_page"))


@auth_bp.route("/profile")
def user_profile():
	user = _require_user()
	if not user:
		return redirect(url_for("auth.user_login_view", next=request.path))
	try:
		orders = fetch_my_orders(user["token"])
	except requests.RequestException as exc:
		flash(f"加载订单失败：{exc}", "error")
		orders = []
	return render_template("profile.html", user=user, orders=orders)


@auth_bp.route("/orders/<int:order_id>/pickup", methods=["POST"])
def ack_pickup(order_id: int):
	user = _require_user()
	if not user:
		return redirect(url_for("auth.user_login_view", next=request.referrer or url_for("auth.user_profile")))
	try:
		acknowledge_pickup(order_id, user["token"])
		flash("已确认取餐提示", "success")
	except requests.RequestException as exc:
		flash(f"确认失败：{exc}", "error")
	return redirect(request.referrer or url_for("auth.user_profile"))



