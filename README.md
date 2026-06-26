# Mini Circuit Complexity

A collection of **from-scratch, zero-dependency C implementations** of circuit complexity theory — the study of Boolean circuits as a model of computation, lower bound techniques, and the hierarchy of circuit classes (AC⁰, ACC⁰, TC⁰, NC, P/poly). Each module maps to MIT, Stanford, and Berkeley graduate-level courses, translating landmark theorems (Håstad switching lemma, Razborov-Smolensky polynomial method, Natural Proofs barrier) into runnable C code.

## Sub-Modules

| Sub-Module | Topics | Key Courses |
|-----------|--------|-------------|
| [mini-ac0-nc-poly-hierarchy](mini-ac0-nc-poly-hierarchy/) | AC⁰/NC¹/TC⁰/P/poly class hierarchy, separations, depth-size tradeoffs, complete problems | Stanford CS358, MIT 6.841 |
| [mini-acc0-circuits](mini-acc0-circuits/) | ACC⁰ = AC⁰ + MODₘ gates, polynomial method, Williams' NEXP ∉ ACC⁰ breakthrough | Stanford CS358, MIT 6.841 |
| [mini-boolean-circuits-model](mini-boolean-circuits-model/) | Boolean circuit DAG model, gate types, fan-in/fan-out, circuit analysis and synthesis | MIT 6.841, UC Berkeley CS278 |
| [mini-circuit-sat-complexity](mini-circuit-sat-complexity/) | Circuit-SAT (first NP-complete problem), Williams' SAT↔lower-bounds program, class descriptors | MIT 6.841, Stanford CS358 |
| [mini-hastad-lower-bounds](mini-hastad-lower-bounds/) | Håstad (1986) optimal AC⁰ lower bounds, PARITY ∉ AC⁰, exp(Ω(n^(1/(d-1)))) size bound | Stanford CS358, MIT 6.845 |
| [mini-monotone-circuit-lower](mini-monotone-circuit-lower/) | Razborov's monotone circuit bounds, CLIQUE super-polynomial, sunflower lemma, approximator method | MIT 6.841, UC Berkeley CS278 |
| [mini-natural-proofs-barrier](mini-natural-proofs-barrier/) | Razborov-Rudich barrier (1994), three barriers: relativization, natural proofs, algebrization | Stanford CS358, MIT 6.845 |
| [mini-nc-hierarchy](mini-nc-hierarchy/) | Nick's Class NCⁱ = poly-size polylog-depth, NC¹-complete (PARITY, formula eval), NC²-complete (determinant, CFL) | MIT 6.841, UC Berkeley CS278 |
| [mini-razborov-smolensky](mini-razborov-smolensky/) | Razborov-Smolensky polynomial method, MAJORITY ∉ AC⁰[p] for prime p≠2, GF(p) polynomial approximation | Stanford CS358, MIT 6.841 |
| [mini-switching-lemma](mini-switching-lemma/) | Håstad Switching Lemma (Gödel Prize 1994), DNF→CNF conversion under random restrictions, depth reduction | Stanford CS358, MIT 6.845 |
| [mini-tc0-threshold-circuits](mini-tc0-threshold-circuits/) | TC⁰ threshold circuits, MAJORITY gates, SORTING/MULTIPLICATION/DIVISION in TC⁰, TC⁰ vs NC¹ open problem | Stanford CS358, MIT 6.841 |

## Design Philosophy

- **Zero external dependencies** — pure C (C99/C11), only `libc` and `libm`
- **Self-contained modules** — each directory has its own `Makefile`, `include/`, `src/`, `examples/`, `demos/`, `tests/`
- **Theory-to-code mapping** — every module includes `docs/` with course-alignment notes and theorem references
- **Landmark theorems executed** — Håstad switching lemma, Razborov-Smolensky polynomial method, Razborov-Rudich natural proofs barrier are all runnable, not just stated

## Building

Each module is standalone. Navigate to a module directory and run:

```bash
cd mini-ac0-nc-poly-hierarchy
make all    # build everything
make test   # run tests
```

Requires **GCC** and **GNU Make**.

## Project Structure

```
mini-circuit-complexity/
├── mini-ac0-nc-poly-hierarchy/   # AC⁰/NC¹/TC⁰/P/poly hierarchy, separations, depth-size curves
├── mini-acc0-circuits/           # ACC⁰ = AC⁰ + MODₘ gates, polynomial method, Williams' theorem
├── mini-boolean-circuits-model/  # Boolean circuit DAG, gate types, analysis, synthesis
├── mini-circuit-sat-complexity/  # Circuit-SAT NP-completeness, Williams' program
├── mini-hastad-lower-bounds/     # Håstad optimal AC⁰ lower bounds, PARITY ∉ AC⁰
├── mini-monotone-circuit-lower/  # Razborov monotone bounds, CLIQUE, sunflower lemma
├── mini-natural-proofs-barrier/  # Razborov-Rudich barrier, relativization, algebrization
├── mini-nc-hierarchy/            # Nick's Class, NC¹-complete, NC²-complete problems
├── mini-razborov-smolensky/      # Polynomial method, MAJORITY ∉ AC⁰[p], GF(p) approximations
├── mini-switching-lemma/         # Håstad Switching Lemma, random restrictions, depth reduction
└── mini-tc0-threshold-circuits/  # TC⁰ threshold circuits, MAJORITY, open TC⁰ vs NC¹
```

## License

MIT
