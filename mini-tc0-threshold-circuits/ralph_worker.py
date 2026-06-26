#!/usr/bin/env python3
"""Ralph Worker - runs in its own terminal for one submodule"""
import subprocess, sys, json, time
from pathlib import Path

MODULE_DIR = Path(sys.argv[1])
SKILL = "F:/nano-everything/mini-theory-of-computation/SKILL.md"
MAX_ITER, TIMEOUT = 2, 300
LOGFILE = MODULE_DIR / "ralph.log"

def log(event):
    with open(LOGFILE, "a", encoding="utf-8") as f:
        f.write(json.dumps(event, ensure_ascii=False) + "\n")
        f.flush()

def count_lines():
    n = 0
    for sd in ["include", "src"]:
        p = MODULE_DIR / sd
        if p.exists():
            for f in p.rglob("*.c"):
                try: n += len(f.read_text(encoding="utf-8").splitlines())
                except: pass
            for f in p.rglob("*.h"):
                try: n += len(f.read_text(encoding="utf-8").splitlines())
                except: pass
    return n

name = MODULE_DIR.name
print(f"\n{'='*60}")
print(f"  Ralph Worker: {name}")
print(f"  Max rounds: {MAX_ITER} | Timeout: {TIMEOUT}s")
print(f"  Log: {LOGFILE}")
print(f"{'='*60}\n")

# Clear old log
LOGFILE.write_text("", encoding="utf-8")
log({"event": "start", "name": name, "time": time.time()})

for rnd in range(1, MAX_ITER + 1):
    before = count_lines()
    print(f"\n--- Round {rnd}/{MAX_ITER} | Lines: {before} ---\n")
    log({"event": "round_start", "round": rnd, "before": before, "time": time.time()})
    
    try:
        r = subprocess.run(
            ["claude", "-p", SKILL],
            cwd=str(MODULE_DIR),
            timeout=TIMEOUT,
            shell=True,
        )
        exit_code = r.returncode
    except subprocess.TimeoutExpired:
        exit_code = -1
        print("  TIMEOUT")
    except Exception as e:
        exit_code = -2
        print(f"  ERROR: {e}")
    
    after = count_lines()
    delta = after - before
    print(f"  Result: {before} -> {after} ({'+' if delta >= 0 else ''}{delta})")
    log({"event": "round_end", "round": rnd, "after": after, "delta": delta, "exit": exit_code, "time": time.time()})
    
    # Check COMPLETE
    readme = MODULE_DIR / "README.md"
    if readme.exists() and "COMPLETE" in readme.read_text(encoding="utf-8"):
        print(f"\n  *** COMPLETE detected! ***")
        log({"event": "complete", "time": time.time()})
        break

# Final check
final = count_lines()
readme_ok = (MODULE_DIR / "README.md").exists()
print(f"\n{'='*60}")
print(f"  FINAL: {final} lines | README: {'YES' if readme_ok else 'NO'}")
print(f"{'='*60}")
log({"event": "done", "lines": final, "readme": readme_ok, "time": time.time()})
print("\nPress Ctrl+C or close this window.")
try:
    while True:
        time.sleep(60)
except KeyboardInterrupt:
    pass
