# Knowledge Graph — mini-natural-proofs-barrier

## L1: Definitions
- Natural property: P ⊆ {f: {0,1}^n → {0,1}} satisfying constructivity and largeness
- Constructivity: membership in P can be decided in time 2^{O(n)}
- Largeness: |P| ≥ 2^{-O(n)} fraction of all Boolean functions
- Usefulness: P implies f ∉ C (target circuit class)
- Natural proof: proof using a natural property to show hardness
- P/poly-natural: property computable in P/poly
- Razborov-Rudich barrier: no natural proof can separate P from P/poly (under OWF assumption)

## L2: Core Concepts
- The Natural Proofs Barrier (Razborov-Rudich 1997)
- If one-way functions exist, no natural proof can show NP ⊄ P/poly
- Most known lower bound proofs are natural
- Naturalization: any proof that "naturalizes" is subject to the barrier
- P/poly-natural = quasi-polynomial natural
- Constructivity + largeness → pseudorandomness distinguisher
- Non-natural proof strategies: algebraic, interactive, etc.

## L3: Mathematical Structures
- Boolean function space F_n = {f: {0,1}^n → {0,1}} of size 2^{2^n}
- Circuit class C_n ⊆ F_n (size-s circuits on n inputs)
- Property as subset P_n ⊆ F_n
- Pseudorandom function ensembles: hard to distinguish from random
- Hard-core predicates: Goldreich-Levin theorem
- One-way functions and their connection to natural proofs
- Combinatorial designs (NW generator)

## L4: Fundamental Laws/Theorems
- **Razborov-Rudich (1997)**: Natural proofs barrier theorem
  - If subexponentially hard OWF exist, no P/poly-natural proof can show NP ⊄ P/poly
- **Goldreich-Levin (1989)**: every OWF has a hard-core predicate
- **Nisan-Wigderson (1994)**: PRG from hard functions
- **Razborov (1985)**: exponential monotone circuit lower bound for CLIQUE
- **Impagliazzo-Rudich (1989)**: black-box separations
- **Kannan (1982)**: Σ_2^P ∩ Π_2^P ⊄ SIZE(n^k)

## L5: Algorithms/Methods
- Boolean function random sampling (2^n truth tables)
- Pseudorandom generator construction from hard predicates
- One-way function candidate implementation (RSA, discrete log)
- NW generator: stretch using combinatorial designs
- Hard-core bit extraction via Goldreich-Levin
- Circuit lower bound proof verification framework
- Natural property checker construction

## L6: Canonical Problems
- P vs NP: ultimate target of circuit lower bounds
- NP ⊄ P/poly: would suffice for P ≠ NP
- CLIQUE^(n,k): monotone NP-complete problem
- OWF existence: cryptographic assumption needed for barrier
- Pseudorandom function families: distinguishability from random
- MCSP (Minimum Circuit Size Problem): meta-complexity

## L7: Applications (Partial)
- Cryptography: OWF, PRF, PRG constructions
- Derandomization: PRG from circuit lower bounds
- Complexity theory: understanding limits of proof techniques
- Lower bound methodology classification

## L8: Advanced Topics (Partial)
- Algebraic proofs: non-natural lower bound strategies
- Williams' algorithmic method for ACC0
- MCSP and meta-complexity
- Interactive proofs and IP=PSPACE (non-natural)
- Arithmetic circuit lower bounds
- GCT (Geometric Complexity Theory)

## L9: Research Frontiers (Partial)
- Can we overcome the natural proofs barrier?
- MCSP NP-hardness?
- Quantum natural proofs barrier?
- Meta-complexity and Kolmogorov complexity
- Fine-grained barriers (MW hypothesis)
