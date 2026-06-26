# Knowledge Graph — mini-hastad-lower-bounds

## L1: Definitions
- Switching Lemma: key tool for AC^0 lower bounds
- Decision tree: depth = query complexity; computes function by querying inputs
- Random restriction: assign random subset of variables, leaving others free
- DT(f|ρ): decision tree depth of f under restriction ρ
- CNF/DNF switching: convert CNF to shallow decision tree
- Clauses and terms: OR of ANDs (CNF), AND of ORs (DNF)
- Decision tree depth lower bounds

## L2: Core Concepts
- Hastad's Switching Lemma (1986): After random restriction with parameter p,
  a t-CNF switches to a decision tree of depth < s with probability 1-δ
- Iterative switching: depth-d AC^0 circuit → alternating CNF/DNF
- Exponential lower bound: PARITY requires exp(Ω(n^{1/(d-1)})) size AC^0 circuits
- Optimality: The switching lemma bound is tight
- Lemma parameters: p = (10t)^{-1} for s = log(1/δ)
- Connection to Razborov's method

## L3: Mathematical Structures
- Decision tree as labeled binary tree
- Random restriction ρ: each variable set to * (free) with prob p,
  0 with prob (1-p)/2, 1 with prob (1-p)/2
- CNF formula as AND of clauses (OR of literals)
- DNF formula as OR of terms (AND of literals)
- Hitting set for decision trees
- Fourier representation connection

## L4: Fundamental Laws/Theorems
- **Hastad Switching Lemma (1986)**: For any t-CNF F and s≥2,
  Pr_ρ[DT(F|ρ) ≥ s] ≤ (5pt)^s where p = random restriction parameter
- **PARITY lower bound**: Depth-d AC^0 circuits for PARITY require
  size ≥ exp(Ω(n^{1/(d-1)}))
- **Optimality**: exp(Ω(n^{1/(d-1)})) is tight (matching upper bound exists)
- **Separation**: PARITY ∈ NC^1 ∖ AC^0
- **Furst-Saxe-Sipser (1981)**: PARITY ∉ AC^0 (superpolynomial)
- **Ajtai (1983)**: improved bound
- **Beame (1994)**: optimal switching lemma constants

## L5: Algorithms/Methods
- Random restriction application to circuit
- Bottom-up switching: transform bottom AND/OR layers to decision trees
- Decision tree to circuit conversion (and back)
- Iterative depth reduction proof technique
- Probability amplification via union bound
- Switching lemma parameter optimization

## L6: Canonical Problems
- PARITY: requiress exp(Ω(n^{1/(d-1)})) AC^0 size
- MAJORITY: also requires exp(Ω(n^{1/(d-1)})) but via different method
- k-CLIQUE: requires exp(n^{Ω(1)}) monotone (Razborov)
- Perfect Matching: requires exp(n^{Ω(1)}) monotone

## L7: Applications (Partial)
- Circuit lower bound proofs: the standard toolkit
- Cryptography: constructing PRGs from hard-on-average functions
- Derandomization: Nisan-Wigderson PRG

## L8: Advanced Topics (Partial)
- Multi-switching lemma (Hastad 2014)
- Switching lemma for ACC^0
- Correlation bounds for AC^0
- Switching lemma in proof complexity

## L9: Research Frontiers (Partial)
- Switching lemma for small-depth threshold circuits?
- Quantum switching lemma analog
- Switching lemma in communication complexity
