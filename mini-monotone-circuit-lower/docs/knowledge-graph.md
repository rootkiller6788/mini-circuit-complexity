# Knowledge Graph — Monotone Circuit Lower Bounds

## L1: Definitions

| # | Concept | Type | C Implementation | Lean Formalization |
|---|---------|------|-----------------|-------------------|
| 1 | Monotone Boolean circuit | Core | `MonotoneCircuit` in `monotone_circuit.h` | `MonotoneCircuit` in `monotone.lean` |
| 2 | Monotone gate types (AND, OR, INPUT) | Core | `MonoGateType` enum in `monotone_circuit.h` | - |
| 3 | Monotone Boolean function | Core | `MonotoneBooleanFunction` in `monotone_circuit.h` | - |
| 4 | CLIQUE function CLIQUE(n,k) | Core | `monotone_clique_function()` in `monotone_graph.h` | - |
| 5 | Graph (undirected) | Core | `Graph` in `monotone_graph.h` | - |
| 6 | Clique (k-clique) | Core | `Clique` in `monotone_graph.h` | - |
| 7 | Sunflower (p petals) | Core | `Sunflower` in `sunflower.h` | - |
| 8 | SetMask / Set family | Core | `SetMask`, `SetFamily` in `sunflower.h` | - |
| 9 | Approximator (A+, A-) | Core | `Approximator` in `approximator.h` | - |
| 10 | Positive example | Core | `PositiveExample` in `approximator.h` | - |
| 11 | Negative example | Core | `NegativeExample` in `approximator.h` | - |
| 12 | Turan graph | Core | `NegativeExample` (NEG_TYPE_TURAN) in `approximator.h` | - |
| 13 | Monotone-NP-completeness | Core | documented in `monotone.h` | - |
| 14 | Monotone-P-completeness | Core | documented in `monotone_graph.h` | - |
| 15 | Negation-limited circuit | Advanced | `NegationLimitedCircuit` in `monotone_circuit.h` | - |
| 16 | Frontier approximator | Advanced | `FrontierApproximator` in `approximator.h` | - |

## L2: Core Concepts

| # | Concept | Implementation |
|---|---------|---------------|
| 1 | Monotonicity property | `mono_verify_monotonicity()` in `monotone_circuit.c` |
| 2 | Method of approximations | `approx_simulate_circuit()` in `approximator.c` |
| 3 | Sunflower compression | `sunflower_compress()` in `sunflower.c` |
| 4 | Approximator blowup at AND gates | `approx_and_gate()` in `approximator.c` |
| 5 | Approximator blowup at OR gates | `approx_or_gate()` in `approximator.c` |
| 6 | Positive/negative example generation | `positive_example_create_random()`, `negative_example_create_turan()` |
| 7 | Monotone vs general circuit gap | `tardos_gap_lower_bound()` in `monotone_lowerbounds.c` |
| 8 | Circuit evaluation (bottom-up) | `mono_circuit_evaluate()` in `monotone_circuit.c` |
| 9 | Truth table computation | `mono_circuit_truth_table()` in `monotone_circuit.c` |
| 10 | Monotone circuit size/depth | `mono_circuit_size()`, `mono_circuit_depth()` |

## L3: Mathematical Structures

| # | Structure | Implementation |
|---|-----------|---------------|
| 1 | Boolean circuit as DAG | `MonotoneCircuit` with gates and fanin edges |
| 2 | Adjacency matrix graph | `Graph` with `char** adj` in `monotone_graph.h` |
| 3 | Set system / family of k-sets | `SetFamily` (bitmask), `KSetFamily` (int arrays) |
| 4 | Sunflower: (p petals, core K) | `Sunflower` with `core`, `petals[]`, `full_sets[]` |
| 5 | Approximator: (A+, A-) | `Approximator` with `pos[]`, `neg[]` |
| 6 | k-uniform hypergraphs | Implicit in `KSetFamily` |
| 7 | Topological ordering | `mono_circuit_topological_order()` |
| 8 | Complete/empty/random/bipartite/cycle graphs | `graph_complete()`, `graph_empty()`, etc. |
| 9 | Complement graph | `graph_complement()` |
| 10 | Graph coloring (for negative examples) | `NegativeExample` with `color_classes` |

## L4: Fundamental Laws (Theorems)

| # | Theorem | C Verification | Lean Statement |
|---|---------|---------------|---------------|
| 1 | **Razborov's Theorem (1985)** | `razborov_clique_lower_bound()` | theorem in `monotone.lean` |
| 2 | **Erdos-Rado Sunflower Lemma (1960)** | `sunflower_bound_erdos_rado()`, `sunflower_find_brute()` | theorem in `monotone.lean` |
| 3 | **Alon-Boppana Bound (1987)** | `alon_boppana_clique_lower_bound()` | - |
| 4 | **Andreev's Bound (1985)** | `andreev_matrix_lower_bound()` | - |
| 5 | **Tardos Gap Theorem (1988)** | `tardos_gap_lower_bound()` | - |
| 6 | **Razborov PM Bound (1985)** | `razborov_pm_lower_bound()` | - |
| 7 | **Natural Proofs Barrier (1997)** | `natural_proofs_barrier_explanation()` | - |
| 8 | **Karchmer-Wigderson (1988)** | `kw_monotone_depth_lower_bound()` | - |
| 9 | Fischer's negation-limited theorem | `NegationLimitedCircuit` type | - |
| 10 | Constant-depth monotone bounds | `constant_depth_monotone_bound()` | - |

## L5: Algorithms/Methods

| # | Algorithm | Implementation | Complexity |
|---|-----------|---------------|-----------|
| 1 | Method of approximations (Razborov) | `approx_simulate_circuit()` | per-gate analysis |
| 2 | Sunflower search (brute force) | `sunflower_find_brute()` | O(C(m,p) * p * k) |
| 3 | Sunflower search (greedy) | `sunflower_find_greedy()` | O(m^2 * k) |
| 4 | Sunflower with specific core | `sunflower_find_with_core()` | O(m * k) |
| 5 | Sunflower compression | `sunflower_compress()` | iterative |
| 6 | Clique detection (backtracking) | `graph_has_clique()` | O(n^k) worst case |
| 7 | Bron-Kerbosch with pivoting | `graph_has_clique_bk()` | O(3^(n/3)) worst case |
| 8 | Clique enumeration | `graph_enumerate_cliques()` | O(3^(n/3)) |
| 9 | Circuit evaluation (bottom-up) | `mono_circuit_evaluate()` | O(size) |
| 10 | Circuit balancing | `mono_circuit_balance()` | O(size * log size) |
| 11 | Constant propagation | `mono_circuit_constant_propagation()` | O(size) |
| 12 | Redundancy elimination | `mono_circuit_eliminate_redundancy()` | O(size) |
| 13 | BFS for ST-connectivity | `graph_has_path()` | O(n + m) |
| 14 | Perfect matching (brute force) | `graph_has_perfect_matching()` | O(n!) |

## L6: Canonical Problems

| # | Problem | Implementation | Status |
|---|---------|---------------|--------|
| 1 | **CLIQUE(n,k)** | `graph_has_clique()`, `graph_has_clique_bk()` | Complete |
| 2 | **VERTEX-COVER** | `graph_has_vertex_cover()` | Complete |
| 3 | **INDEPENDENT SET** | `graph_has_independent_set()` | Complete |
| 4 | **ST-CONNECTIVITY** | `graph_has_path()` | Complete |
| 5 | **PERFECT MATCHING** | `graph_has_perfect_matching()` | Partial (n ≤ 12) |
| 6 | Monotone circuit value problem | `mono_circuit_evaluate()` | Complete |
| 7 | Maximum clique | `graph_max_clique_size()` | Complete |
| 8 | Turan graph construction | `graph_generate_clique_negative()` | Complete |
| 9 | CLIQUE positive example generation | `graph_generate_clique_positive()` | Complete |

## L7: Applications

| # | Application | Implementation | Status |
|---|-------------|---------------|--------|
| 1 | Circuit optimization (crypto-grade) | `circuit_crypto_grade()` | Complete |
| 2 | Constant propagation in circuits | `mono_circuit_constant_propagation()` | Complete |
| 3 | Redundancy elimination | `mono_circuit_eliminate_redundancy()` | Complete |
| 4 | DOT visualization (Graphviz) | `mono_circuit_write_dot()` | Complete |
| 5 | Formula conversion | `mono_circuit_to_formula()`, `mono_formula_to_circuit()` | Complete |
| 6 | Empirical bound verification | `verify_lower_bound_empirically()` | Partial |

## L8: Advanced Topics

| # | Topic | Implementation | Status |
|---|-------|---------------|--------|
| 1 | **Natural Proofs Barrier** | `natural_proofs_barrier_explanation()` | Documented |
| 2 | **Karchmer-Wigderson connection** | `kw_monotone_depth_lower_bound()` | Partial |
| 3 | Frontier approximator | `FrontierApproximator`, `frontier_approx_*()` | Partial |
| 4 | Andreev approximator | `approx_andreev_construct()` | Stub |
| 5 | Negation-limited circuits | `NegationLimitedCircuit` type | Defined |
| 6 | ALWZ improved sunflower bound | `sunflower_bound_alwz()` | Partial |
| 7 | Meta-complexity connection | `meta_complexity_monotone_approach()` | Documented |

## L9: Research Frontiers

| # | Topic | Implementation | Status |
|---|-------|---------------|--------|
| 1 | Extending to general circuits | `open_problems_monotone_lower_bounds()` | Documented |
| 2 | Meta-complexity approach | `meta_complexity_monotone_approach()` | Documented |
| 3 | GCT (Geometric Complexity Theory) | Documented only | - |
| 4 | Quantum complexity connections | Not implemented | - |
| 5 | Hardness of approximation | Not implemented | - |
| 6 | Extended approximators (PM, ST-Conn) | `approx_perfect_matching()`, `approx_st_connectivity()` | Stub |
| 7 | Monotone projection reductions | Not implemented | - |
| 8 | Monotone switching lemmas | Not implemented | - |

## Summary

| Level | Status | Items Covered |
|-------|--------|--------------|
| L1 Definitions | **Complete** | 16/16 |
| L2 Core Concepts | **Complete** | 10/10 |
| L3 Math Structures | **Complete** | 10/10 |
| L4 Fundamental Laws | **Complete** | 10/10 |
| L5 Algorithms | **Complete** | 14/14 |
| L6 Canonical Problems | **Complete** | 9/9 |
| L7 Applications | **Partial+** | 6/6 (some stubs) |
| L8 Advanced Topics | **Partial+** | 7/7 (2 stubs) |
| L9 Research Frontiers | **Partial** | 3/8 documented |
