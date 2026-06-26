#!/usr/bin/env python3
"""Build all source files for mini-acc0-circuits."""
import os, sys

BASE = os.path.dirname(os.path.abspath(__file__))
def w(path, content):
    full = os.path.join(BASE, path)
    os.makedirs(os.path.dirname(full), exist_ok=True)
    with open(full, 'w', encoding='utf-8') as f:
        f.write(content)
    return len(content.splitlines())

total = 0
total += w("src/acc0.c", open(os.path.join(BASE, "_acc0_core.c"), encoding='utf-8').read())
total += w("src/acc0_gates.c", open(os.path.join(BASE, "_acc0_gates.c"), encoding='utf-8').read())
total += w("src/acc0_polynomial.c", open(os.path.join(BASE, "_acc0_poly.c"), encoding='utf-8').read())
total += w("src/acc0_lower_bounds.c", open(os.path.join(BASE, "_acc0_lb.c"), encoding='utf-8').read())
print(f"Total lines written: {total}")

