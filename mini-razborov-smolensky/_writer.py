import os, sys

BASE = r"F:\nano-everything\mini-theory-of-computation\1. mini-circuit-complexity\mini-razborov-smolensky"

def w(path, content):
    full = os.path.join(BASE, path)
    os.makedirs(os.path.dirname(full), exist_ok=True)
    with open(full, 'w', encoding='utf-8', newline='\n') as f:
        f.write(content)
    print(f"  Written: {path} ({len(content.splitlines())} lines)")

# Test
w("_test.txt", "hello world\n")
print("Writer ready")
