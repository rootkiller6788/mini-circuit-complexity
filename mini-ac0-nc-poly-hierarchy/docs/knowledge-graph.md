# Knowledge Graph — mini-ac0-nc-poly-hierarchy

## L1: Definitions
- AC^0: constant-depth, polynomial-size, unbounded fan-in AND/OR/NOT circuits
- AC^0[m]: AC^0 extended with MOD_m gates
- ACC^0: union of AC^0[m] over all m
- NC: polylog-depth, polynomial-size, bounded fan-in circuits
- NC^i: depth O(log^i n), size poly(n)
- AC^i: depth O(log^i n), unbounded fan-in = NC^{i+1} ⊆ AC^i ⊆ NC^{i+1}
- TC^0: constant-depth with MAJORITY/THRESHOLD gates
- P/poly: languages decided by polynomial-size circuit families
- Circuit uniformity: L-uniform, P-uniform, DLOGTIME-uniform

## L2: Core Concepts
- AC^0 hierarchy: AC^0 ⊂ AC^1 ⊆ NC^1 ⊆ L ⊆ NL ⊆ NC^2 ⊆ P ⊆ P/poly
- AC^0 is the weakest "interesting" circuit class
- PARITY not in AC^0 (FSS 1981, Ajtai 1983, Hastad 1987)
- AC^0 ≠ NC^1 (via PARITY lower bound)
- NC = AC (with different depth conventions)
- NC = languages with efficient parallel algorithms
- TC^0 contains MAJORITY, counting, multiplication, division
- AC^0 ⊂ TC^0 ⊆ NC^1 (known separations)
- Non-uniform vs uniform: P/poly ⊇ P; BPP ⊆ P/poly (Adleman)

## L3: Mathematical Structures
- Circuit family C = {C_n}_{n≥1}: one circuit per input size
- Circuit DAG with level assignment
- Gate types: AND_∞, OR_∞, NOT, MOD_m, MAJ, THR_k
- Boolean function f: {0,1}^n → {0,1}
- Symmetric functions: depend only on Hamming weight
- Polynomial representation over GF(2): f(x) = ⊕_S c_S x^S
- Communication matrix for KW game

## L4: Fundamental Laws/Theorems
- **Furst-Saxe-Sipser (1981)**: PARITY ∉ AC^0
- **Ajtai (1983)**: PARITY requires super-polynomial AC^0 size
- **Hastad (1987)**: PARITY requires exp(n^{1/(d-1)}) size for depth d
- **Razborov-Smolensky (1987)**: MOD_p ∉ AC^0[q] for distinct primes p,q
- **Williams (2014)**: NEXP ⊄ ACC^0 (major breakthrough)
- **NC = AC**: unbounded fan-in does not change power at polylog depth
- **Barrington (1989)**: NC^1 = width-5 branching programs
- **Allender (1989)**: AC^0 proper hierarchy theorem

## L5: Algorithms/Methods
- AC^0 circuit simulation in O(size) time
- Parity via XOR tree (size O(n)) — shows PARITY ∈ NC^1
- Majority via AKS sorting network — O(n log n) comparators
- Mod3 via depth-2 exponential DNF analysis
- Circuit depth computation via longest-path DAG algorithm
- Fan-in reduction: unbounded → bounded (at depth cost)
- Gate elimination for AC^0 lower bound proofs

## L6: Canonical Problems
- PARITY: in NC^1, not in AC^0 (Hastad)
- MAJORITY: in TC^0, in NC^1 (via sorting networks)
- MOD_p: in AC^0[p], not in AC^0[q] (Razborov-Smolensky)
- SORTING: in AC^0, NC^1 bitonic sort
- MULTIPLICATION: in TC^0 (via Chinese Remainder)
- DIVISION: in TC^0 (Beame-Cook-Hoover 1986)
- CONNECTIVITY (st-conn): ∈ NL ⊆ NC^2, not known in NC^1
- PERFECT MATCHING: in P (Edmonds), in RNC^2 (Mulmuley-Vazirani-Vazirani)

## L7: Applications (Partial)
- Parallel algorithm design: NC = efficiently parallelizable
- Hardware verification: circuit equivalence checking
- VLSI design: area-time complexity tradeoffs
- Cryptanalysis: lower bounds inform cipher design

## L8: Advanced Topics (Partial)
- Williams' algorithm: NEXP ∉ ACC^0 via circuit SAT
- Barrington's theorem: NC^1 = width-5 permutation branching programs
- ACC^0 circuit lower bounds and the frontier
- Switching lemma and its extensions
- Polynomial method (Beigel-Reingold-Spielman)

## L9: Research Frontiers (Partial)
- Is MAJORITY in ACC^0? (open since 1987)
- ACC^0 vs NC^1 separation
- GCT approach to circuit lower bounds
- Meta-complexity: MCSP and circuit complexity
