import os
BASE = r"F:\nano-everything\mini-theory-of-computation\1. mini-circuit-complexity\mini-acc0-circuits\src"

def w(name, content):
    path = os.path.join(BASE, name)
    with open(path, "w", encoding="utf-8") as f:
        f.write(content)
    print(f"  {name}: {content.count(chr(10))} lines")

