# Knowledge Graph — mini-razborov-smolensky

## L1: Definitions
- AC^0[p]: AC^0 circuits extended with MOD_p gates
- MOD_p gate: outputs 1 iff sum of inputs ≡ 0 (mod p)
- Prime modulus characterization
- Exponential lower bound
- Polynomial method over GF(p)
- Low-degree polynomials
- Probabilistic polynomials
- Approximating polynomials

## L2: Core Concepts
- Razborov-Smolensky Theorem: MOD_p ∉ AC^0[q] for distinct primes p, q
- AC^0[2] ⊊ AC^0[3] (incomparable power for different primes)
- Polynomial representation of Boolean functions over finite fields
- Degree of AC^0 circuits grows with depth
- Low-degree polynomial approximation
- Random restriction method extension

## L3: Mathematical Structures
- GF(p): finite field of order p (p prime)
- Polynomial ring GF(p)[x_1,...,x_n]
- Boolean function f as multivariate polynomial over GF(p)
- Degree of f: max degree of any monomial
- δ-approximation: Pr[f(x)=p(x)] ≥ 1-δ
- Combined gate: MOD_p + AND/OR
- Algebraic normal form over GF(2)

## L4: Fundamental Laws/Theorems
- **Razborov (1987)**: MAJORITY requires exp(Ω(n^{1/2d})) size in AC^0[2]
- **Smolensky (1987)**: MOD_p requires exp(Ω(n^{1/2d})) in AC^0[q] for p≠q
- **Razborov (1987)**: MOD_3 ∉ AC^0[2] (size exp(Ω(n^{1/2d})))
- **Smolensky (1987)**: MOD_2 ∉ AC^0[3] sym. bound
- **Polynomial method principle**: AC^0 circuits can be approximated by low-degree polynomials over GF(p)
- AC^0[p] ⊊ AC^0[q] for distinct primes (incomparable)

## L5: Algorithms/Methods
- Polynomial approximation of AND/OR gates over GF(p)
- Random restriction for MOD gates
- Degree amplification: depth d → degree O(log^{d-1} n)
- GF(p) arithmetic with precomputed tables
- MOD_p evaluation via sum modulo p
- Polynomial interpolation for Boolean functions
- Kronecker substitution for multilinearization

## L6: Canonical Problems
- MOD_3 on AC^0[2] circuits: requires exp(n^{1/2d}) size
- PARITY (MOD_2) on AC^0[3]: requires exp(n^{1/2d}) size
- MAJORITY on AC^0[2]: requires exp(Ω(n^{1/2d})) size
- MOD_5 on AC^0[2]: requires exponential size
- MOD_composite vs MOD_prime: different power

## L7: Applications (Partial)
- Circuit lower bound education: standard advanced proof
- Cryptography: MOD gates inspired by algebraic attacks
- Error-correcting codes: polynomial-based codes

## L8: Advanced Topics (Partial)
- General MOD gates and ACC0[m] = AC^0[MOD_m]
- Williams algorithm for ACC0 SAT
- Polynomial method vs random restriction comparison
- Degree-rank tradeoffs

## L9: Research Frontiers (Partial)
- Smolensky's bounds for composite moduli
- MAJORITY in ACC0? (open)
- Algebraic natural proofs
- GF(2) polynomial completeness
