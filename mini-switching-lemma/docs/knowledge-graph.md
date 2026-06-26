# Knowledge Graph — mini-switching-lemma

## L1: Definitions (Complete)
- **DNF (Disjunctive Normal Form)**: OR of ANDs; sum of products
- **CNF (Conjunctive Normal Form)**: AND of ORs; product of sums
- **k-DNF**: DNF where each term has at most k literals
- **s-CNF**: CNF where each clause has at most s literals
- **Decision Tree**: Binary tree querying variables, leaves = output
- **Random Restriction**: Partial assignment rho: vars -> {0,1,*}
- **AC0**: Constant-depth, unbounded fan-in circuits with AND/OR/NOT
- **PARITY function**: XOR of all input bits
- **Circuit size**: Number of gates in a Boolean circuit
- **Circuit depth**: Length of longest input-to-output path

## L2: Core Concepts (Complete)
- **Switching Lemma**: k-DNF becomes s-CNF w.h.p. after random restriction
- **Depth Reduction**: Iteratively applying switching reduces circuit depth
- **Probabilistic Method**: Random restrictions as a proof technique
- **Circuit Lower Bound**: Proving a function requires large circuits
- **Boolean Function Complexity**: Minimum circuit size for a function
- **DNF/CNF Duality**: De Morgan's laws connecting the two forms
- **Restriction as Projection**: Partial assignment simplifies formulas

## L3: Mathematical Structures (Complete)
- **Formula representation**: Flat array encoding for DNF/CNF
- **Literal encoding**: (variable_index << 1) | sign_bit
- **Restriction probability space**: Each var independently *,0,1
- **Circuit DAG**: Directed acyclic graph of gates
- **Truth table**: Exhaustive enumeration of 2^n assignments
- **Fourier representation**: Boolean functions as polynomials over GF(2)

## L4: Fundamental Laws (Complete)
- **Hastad Switching Lemma (1986)**: Pr[failure] < (5pk)^s
- **PARITY ∉ AC0**: Furst-Saxe-Sipser (1981), Ajtai (1983), Hastad (1986)
- **Exponential Lower Bound**: size >= exp(Omega(n^{1/(d-1)})) for depth d
- **AC0 ⊊ NC1**: AC0 is a proper subclass of NC1
- **Decision Tree Depth = n for PARITY**: Must query all variables

## L5: Algorithms/Methods (Complete)
- **Random restriction generation**: Independent sampling per variable
- **DNF restriction algorithm**: Simplify by removing fixed literals
- **Switching experiment**: Monte Carlo validation of the lemma
- **Iterative depth reduction**: d-1 rounds of switching
- **DNF-to-CNF conversion**: Via truth table enumeration
- **DT-to-circuit conversion**: Decision tree as AC0 circuit

## L6: Canonical Problems (Complete)
- **PARITY lower bound**: Full proof simulation
- **MAJORITY function**: Decision tree and AC0 analysis
- **Threshold functions**: T_k(x) = 1 iff sum >= k
- **Modular counting**: MOD_m functions
- **AND/OR functions**: Trivial for AC0

## L7: Applications (Partial — 3 applications)
- **Pseudorandom Generators**: Nisan-Wigderson from AC0 lower bounds
- **PAC Learning**: Linial-Mansour-Nisan Fourier learning
- **Nisan's PRG**: Seed length O(log^{2d} n) for AC0

## L8: Advanced Topics (Partial — 2 topics)
- **Multi-Switching Lemma**: Switch multiple layers simultaneously
- **Average-Case Hardness**: PARITY hard for AC0 on random inputs
- (Future: Correlation bounds, natural proofs barrier)

## L9: Research Frontiers (Partial)
- Meta-complexity: Hardness vs randomness connections
- GCT: Geometric complexity theory approach
- Quantum circuit lower bounds
