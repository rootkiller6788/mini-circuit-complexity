# Knowledge Graph — mini-acc0-circuits

## L1: Definitions
- ACC0 = AC^0 + MOD_m gates for all m ≥ 2
- ACC0[m] = AC^0 circuits with MOD_m gates
- MOD_m gate: outputs 1 iff Σ inputs ≡ 0 (mod m)
- MOD-m counting: sum mod m
- ACC0 = ∪_{m≥2} ACC0[m]
- Gate types: AND_∞, OR_∞, NOT, MOD_m for multiple m
- Polynomial method: represent circuit as polynomial over GF(p)
- Williams' algorithm: circuit SAT for ACC0 in non-trivial time

## L2: Core Concepts
- ACC0 is a proper superset of AC^0
- PARITY (MOD_2) ∈ ACC0 (via single MOD2 gate)
- MAJORITY ∈ ACC0? OPEN since 1987
- ACC0[m] ⊊ ACC0[mn] (proper hierarchy by Williams 2014)
- NEXP ⊄ ACC0 (Williams 2014)
- MOD_p for prime p: algebraic structure via GF(p)
- MOD_m for composite m: via CRT decomposition
- Separation from TC^0: unknown (ACC0 ⊆ TC^0 or incomparable?)

## L3: Mathematical Structures
- ACC0Circuit: gates array + metadata
- MOD gate: computes sum modulo integer
- Polynomial representation over GF(p) for MOD_p gates
- Chinese Remainder decomposition for composite MOD gates
- Circuit evaluation: O(size) time, bottom-up
- ACC0CircuitFamily: {C_n} for each input size
- Truth-table verification

## L4: Fundamental Laws/Theorems
- **Razborov-Smolensky (1987)**: MOD_p ∉ ACC0[q] for distinct primes p,q
- **Williams (2014)**: NEXP ⊄ ACC0 (circuit SAT in O(2^{n-n^δ}) time)
- **Allender-Gore (1994)**: ACC0 ⊂ TC^0 (upper bound)
- **PARITY ∈ ACC0**: trivial with MOD2 gate
- **MOD_p vs MOD_q**: distinct primes give incomparable power
- **Williams' algorithm**: non-trivially faster SAT implies ACC0 lower bounds

## L5: Algorithms/Methods
- ACC0 circuit evaluation (bottom-up traversal)
- GF(p) polynomial conversion of ACC0 circuit
- MOD_gate to polynomial transformation
- Williams' ACC0-SAT algorithm (polynomial method + fast evaluation)
- Circuit size/depth computation
- Random circuit family generation
- Equivalence checking via truth tables
- DPLL-based SAT solving for small circuits

## L6: Canonical Problems
- PARITY: trivial in ACC0 (one MOD2 gate)
- MOD_3 on ACC0[2] circuits: lower bound by Razborov
- MAJORITY in ACC0? OPEN since 1987
- SYMMETRIC functions: all in ACC0[m] for appropriate m
- MOD_COMPOSITE: decomposable via CRT
- ACC0-CIRCUIT-SAT: solved by Williams' algorithm

## L7: Applications (Partial)
- Circuit complexity education: teaching MOD gates
- Cryptography inspiration: MOD-based constructions
- Algebraic circuit analysis

## L8: Advanced Topics (Partial)
- Williams' NEXP ⊄ ACC0 proof outline
- Polynomial method for ACC0
- ACC0 vs. NC^1 relationship
- Lower bound techniques (random restriction + polynomial)
- Degree lower bounds for ACC0 polynomials

## L9: Research Frontiers (Partial)
- Is MAJORITY ∈ ACC0? (open since 1987)
- ACC0 vs TC^0: are they equal? incomparable?
- ACC0 circuit SAT improvements
- Meta-complexity of ACC0
- Quantum ACC0 analog
