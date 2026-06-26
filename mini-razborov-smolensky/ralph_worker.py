#!/usr/bin/env python3
import subprocess, sys, json, time
from pathlib import Path

MODULE_DIR = Path(sys.argv[1])
SKILL = "F:/nano-everything/mini-theory-of-computation/SKILL.md"
MAX_ITER, TIMEOUT, TARGET = 50, 300, 3000
LOGFILE = MODULE_DIR / "ralph.log"

def log(event):
    with open(LOGFILE, "a", encoding="utf-8") as f:
        f.write(json.dumps(event, ensure_ascii=False) + "\n")
        f.flush()

def count_lines():
    n = 0
    for sd in ["include", "src"]:
        p = MODULE_DIR / sd
        if not p.exists(): continue
        for f in p.rglob("*.c"):
            try: n += len(f.read_text(encoding="utf-8").splitlines())
            except: pass
        for f in p.rglob("*.h"):
            try: n += len(f.read_text(encoding="utf-8").splitlines())
            except: pass
    return n

def mark_done(lines):
    (MODULE_DIR / "README.md").write_text(
        f"## Module Status: COMPLETE\n\nLines: {lines}/{TARGET}\n", encoding="utf-8")

name = MODULE_DIR.name
print(f"Worker: {name} | Target: {TARGET}")
LOGFILE.write_text("", encoding="utf-8")
log({"event": "start", "name": name, "time": time.time()})

rnd = 0
while rnd < MAX_ITER:
    rnd += 1
    before = count_lines()

    # Already done?
    if before >= TARGET:
        mark_done(before)
        print(f"DONE: {before} lines")
        log({"event": "complete", "time": time.time()})
        break

    print(f"\nRound {rnd} | {before}/{TARGET}")
    log({"event": "round_start", "round": rnd, "before": before, "time": time.time()})

    skill = open(SKILL, encoding="utf-8", errors="replace").read()
    prompt = (
        f"DO NOT ask questions. DO NOT explain the spec. JUST write code.\n"
        f"Your ONLY task: add C code to {MODULE_DIR} to reach {TARGET} lines.\n"
        f"Now: {before}/{TARGET}. Rules: no filler, no stubs, C99, libc+libm only.\n"
        f"make clean && make must pass.\n"
        f"When >= {TARGET} lines, write README.md with '## Module Status: COMPLETE'.\n"
        f"WRITE FILES NOW. Do not output text.\n"
        f"\n=== SPEC ===\n{skill}"
    )
    try:
        r = subprocess.run(["claude", "-p", prompt], cwd=str(MODULE_DIR),
            timeout=TIMEOUT, shell=True)
        exit_code = r.returncode
    except subprocess.TimeoutExpired: exit_code = -1
    except Exception as e: exit_code = -2

    after = count_lines()
    delta = after - before
    print(f"  {before} -> {after} (+{delta})")
    log({"event": "round_end", "round": rnd, "after": after, "delta": delta,
         "exit": exit_code, "time": time.time()})

    if after >= TARGET:
        mark_done(after)
        print(f"COMPLETE: {after} lines")
        log({"event": "complete", "time": time.time()})
        break

print(f"FINAL: {count_lines()} lines")
log({"event": "done", "lines": count_lines(), "readme": (MODULE_DIR/"README.md").exists(), "time": time.time()})
