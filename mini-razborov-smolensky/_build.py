import os, sys

BASE = os.path.dirname(os.path.abspath(__file__))

def w(path, content):
    full = os.path.join(BASE, path)
    os.makedirs(os.path.dirname(full), exist_ok=True)
    with open(full, "w", encoding="utf-8", newline="\n") as f:
        f.write(content)
    print(f"  Written: {path}")

w("_test.txt", "test ok")
print("Build script base ready.")
