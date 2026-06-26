# mini-boolean-circuits-model

Boolean circuit model: DAG-based computation with AND/OR/NOT gates. Foundational module for circuit complexity theory.

## Module Status: COMPLETE

- **L1-L6**: Complete (all core definitions, concepts, structures, theorems, algorithms, and canonical problems)
- **L7**: Partial+ (3 applications: logic synthesis, optimization, technology mapping)
- **L8**: Partial+ (3/5 advanced topics: lower bounds, genetic/annealing synthesis, BDD synthesis)
- **L9**: Partial (documented: meta-complexity, natural proofs, GCT)

## Line Count
include/ + src/ = 4971 lines (minimum: 3000)

## Core Definitions (L1)
- `BooleanCircuit`: DAG of gates with size/depth/width parameters
- `Gate`: typed node (INPUT, AND, OR, NOT, NAND, NOR, XOR, MAJ, THR, CONST0, CONST1)
- `GateType`: enumeration of 11 gate types
- `CircuitFamily`: sequence of circuits {C_n} for inputs of length n
- `CircuitClass`: enumeration (AC^0, NC^1, TC^0, P/poly, etc.)

## Core Theorems (L4)
- **Shannon (1949)**: Almost all n-variable Boolean functions require circuit size Ω(2^n / n)
- **Lupanov (1958)**: Every n-variable Boolean function has circuits of size O(2^n / n)
- **Hastad (1986)**: PARITY requires exp(Ω(n^{1/(d-1)})) size in depth-d AC^0 circuits
- **Razborov-Smolensky (1987)**: MOD-p requires exponential size in AC^0[MOD-q] for p≠2, q≠p^k
- **Karchmer-Wigderson (1988)**: Circuit depth equals communication complexity of the KW game
- **Nechiporuk (1966)**: Independent subfunctions give Ω(n^2 / log^2 n) lower bounds

## Core Algorithms (L5)
- Memoized recursive circuit evaluation: O(|C|) per input
- Topological-order evaluation: parallel-simulation friendly
- Quine-McCluskey exact 2-level minimization: O(3^n / n)
- Reed-Muller / ESOP synthesis via fast Walsh transform: O(n·2^n)
- Shannon decomposition synthesis: recursive MUX tree, O(2^n)
- NAND/NOR-only transformation: functional completeness, ≤3x size blowup
- Circuit balancing: chain-to-tree restructuring, O(n log n)
- Constant propagation: partial evaluation for optimization
- Carry-save adder tree: O(n) gate count for symmetric functions
- Batcher odd-even merge sort: O(n log^2 n) sorting network

## Canonical Problems (L6)
- **PARITY**: Separates AC^0 from NC^1 (Furst-Saxe-Sipser, Hastad)
- **MAJORITY**: Canonical TC^0 function (also in NC^1 via Valiant)
- **CLIQUE(k)**: NP-complete, in P/poly for fixed k
- **HAMILTONIAN PATH**: NP-complete, brute-force circuit
- **PERFECT MATCHING**: In P (Edmonds), poly-size circuits
- **MOD-m**: Not in AC^0 for prime m≠2 (Razborov-Smolensky)
- **THRESHOLD(k)**: Symmetric function, AC^0 DNF for constant k
- **MULTIPLICATION**: In TC^0 via iterated addition
- **ADDITION**: In AC^0 (also in NC^1 via CLA)
- **SORTING**: In TC^0 and NC^1

## Circuit Complexity Class Hierarchy
```
AC^0 ⊂ TC^0 ⊂ NC^1 ⊂ L/poly ⊂ NL/poly ⊂ AC^1 ⊂ NC^2 ⊂ ... ⊂ NC ⊂ P/poly
```
Known separations: AC^0 ≠ NC^1 (PARITY). Open: NC^1 = TC^0? P = P/poly?

## Build
```bash
make clean && make        # builds and runs tests (C99, libc+libm)
make test                 # run test suite
```

## Files
```
include/
  circuit.h              Core data structures and API (260 lines)
  circuit_analysis.h     Analysis and transformations (140 lines)
  circuit_builder.h      Circuit construction factories (159 lines)
  circuit_complexity.h   Complexity measures and classes (151 lines)
  circuit_synthesis.h    Logic synthesis and mapping (140 lines)
src/
  circuit.c              Core implementation (886 lines)
  circuit_analysis.c     Analysis implementation (894 lines)
  circuit_builder.c      Builder implementation (997 lines)
  circuit_complexity.c   Complexity implementation (697 lines)
  circuit_synthesis.c    Synthesis implementation (609 lines)
  circuit_app.c          Application utilities (38 lines)
tests/
  test_circuit.c         Comprehensive test suite (177 lines)
examples/
  example_parity.c       PARITY circuit demonstration
  example_synthesis.c    Logic synthesis methods
  example_complexity.c   Complexity class benchmarks
docs/
  knowledge-graph.md     L1-L9 knowledge coverage
  coverage-report.md     Completeness evaluation
  gap-report.md          Missing knowledge points
  course-alignment.md    Nine-university curriculum mapping
  course-tree.md         Prerequisite dependency tree
```

## References
- Arora & Barak (2009): *Computational Complexity: A Modern Approach*, Ch.6, Ch.14
- Vollmer (1999): *Introduction to Circuit Complexity*
- Jukna (2012): *Boolean Function Complexity*
- O'Donnell (2014): *Analysis of Boolean Functions*
- Sipser (2013): *Introduction to the Theory of Computation*, Ch.9
- Stanford CS358: Circuit Complexity
- MIT 6.841: Advanced Complexity Theory
