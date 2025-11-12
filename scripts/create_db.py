import sqlite3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DB_PATH = ROOT / "restaurant.db"
SCHEMA = ROOT / "db" / "schema.sql"
INIT = ROOT / "db" / "init_data.sql"

def main() -> None:
	schema = SCHEMA.read_text(encoding="utf-8")
	init_data = INIT.read_text(encoding="utf-8")

	conn = sqlite3.connect(str(DB_PATH))
	cur = conn.cursor()
	for stmt in schema.split(";"):
		if stmt.strip():
			cur.execute(stmt)
	for stmt in init_data.split(";"):
		if stmt.strip():
			cur.execute(stmt)
	conn.commit()
	conn.close()
	print(f"Created {DB_PATH} with schema and initial data.")


if __name__ == "__main__":
	main()


