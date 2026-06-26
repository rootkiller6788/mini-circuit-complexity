
import sys
code = []
code.append("""/* circuit_app.c -- Boolean Circuit Applications

 * Application-layer functions: SAT solving (DPLL), verification,
 * OWF candidate (multiplication), and complexity benchmarking.
 *
 * L7: SAT solving, formal verification, cryptography.
 * References: AB (2009) Ch.6,15; Sipser (2013) Ch.9; Goldreich (2008).
 */
""")
with open("src/circuit_app.c", "w", encoding="utf-8") as f:
    for c in code:
        f.write(c)
print(f"Wrote {len(code)} blocks")
