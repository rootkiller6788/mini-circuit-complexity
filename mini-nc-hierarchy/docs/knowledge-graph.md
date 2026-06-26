# Knowledge Graph — mini-nc-hierarchy

## L1: Definitions
- NC^i: depth O(log^i n), fan-in 2, poly-size circuits
- AC^i: depth O(log^i n), unbounded fan-in, poly-size
- NC = ∪_i NC^i = ∪_i AC^i
- NC^0: constant depth, constant fan-in (trivial)
- NC^1: log depth, fan-in 2 (formulas)
- NL (nondeterministic logspace): ⊆ NC^2
- DLOGTIME-uniform: uniform with O(log n) time TM
- ALOGTIME = NC^1-uniform AC^0

## L2: Core Concepts
- NC = efficiently parallelizable (polylog time, poly processors)
- NC^0 ⊂ NC^1 ⊆ L ⊆ NL ⊆ NC^2 ⊆ NC^3 ⊆ ... ⊆ NC ⊆ P
- AC^i ⊆ NC^{i+1} ⊆ AC^{i+1}
- NC^1 = polynomial-size formulas (Spira 1971)
- NC^1 = width-5 branching programs (Barrington 1989)
- Borodin (1977): NC hierarchy proper if uniform
- Known: NC^1 ⊊ NC^2 (via PARITY ∈ NC^1, BFS tree not in NC^1)

## L3: Mathematical Structures
- Boolean formulas = fan-out 1 circuits = trees
- Branching programs: layered DAG with labeled edges
- Permutation branching programs (Barrington's construction)
- Width-w branching program: at most w states per layer
- Circuit-to-formula conversion (tree expansion)
- Stack of depth hierarchies

## L4: Fundamental Laws/Theorems
- **Barrington (1989)**: NC^1 = width-5 permutation branching programs
- **Spira (1971)**: formula size = 2^{O(depth)}
- **Borodin (1977)**: NC hierarchy is proper (uniform setting)
- **Ruzzo (1981)**: NC = ALOGTIME-uniform circuits (characterization)
- **Cook (1985)**: NC^1 = ALOGTIME
- **Immerman-Szelepcsényi (1988)**: NL = coNL
- **Mulmuley-Vazirani-Vazirani (1987)**: Perfect Matching ∈ RNC^2

## L5: Algorithms/Methods
- Fan-in 2 reduction via balanced trees (O(log n) depth)
- Formula evaluation with O(log n) depth
- Matrix multiplication in NC (iterated multiplication)
- st-connectivity ∈ NL ⊆ NC^2
- Parity computation via XOR tree (NC^1)
- Majority via sorting networks (NC^1)
- Integer addition in AC^0
- Integer multiplication in TC^0 ⊆ NC^1

## L6: Canonical Problems
- PARITY: in NC^1, not in AC^0
- MAJORITY: in NC^1 (via sorting), in TC^0
- st-CONNECTIVITY: NL-complete, in NC^2
- MATRIX-MULT: in NC (O(log^2 n) depth)
- PERFECT-MATCHING: in RNC^2
- REACHABILITY: in NC^2
- FORMULA-EVAL: in NC^1
- BFS-TREE: not in NC^1 (separates NC^1 from NC^2)

## L7: Applications (Partial)
- Parallel algorithm design: NC = practical parallel
- GPU computing model alignment
- VLSI layout constraints
- Parallel database query evaluation

## L8: Advanced Topics (Partial)
- RNC: randomized NC (perfect matching)
- Quasi-NC: slightly super-polylog
- Counting hierarchies: #NC, GapNC
- Planar reachability and UL
- Fractional NC

## L9: Research Frontiers (Partial)
- NC^1 vs L: open problem
- NL vs NC^1: open problem
- Deterministic NC algorithm for Perfect Matching
- Derandomization of RNC
