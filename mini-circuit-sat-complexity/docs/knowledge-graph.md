# Knowledge Graph — mini-circuit-sat-complexity

## L1: Definitions
- CIRCUIT-SAT: given circuit C, ∃ x: C(x)=1
- CIRCUIT-TAUTOLOGY: circuit always outputs 1
- CIRCUIT-EQUIVALENCE: C1(x)=C2(x) ∀x
- #CIRCUIT-SAT: count satisfying assignments
- Circuit-SAT variants: CNF-SAT, 3-SAT, k-SAT
- SAT solver: algorithm finding satisfying assignment
- DPLL: Davis-Putnam-Logemann-Loveland (1962)
- CDCL: Conflict-Driven Clause Learning (1996)
- BCP: Boolean Constraint Propagation

## L2: Core Concepts
- CIRCUIT-SAT is NP-complete (Cook-Levin core)
- CIRCUIT-TAUTOLOGY is coNP-complete
- #CIRCUIT-SAT is #P-complete (Valiant 1979)
- CIRCUIT-EQUIVALENCE is in coNP (non-deterministic guessing)
- Tseitin transformation: circuit → CNF in linear time
- Decision vs optimization: SAT solving vs MAX-SAT
- Phase transitions in random 3-SAT (k=4.26)
- UNSAT certificates: resolution proofs

## L3: Mathematical Structures
- CNF formula: conjunction of clauses (disjunctions of literals)
- DNF formula: disjunction of terms (conjunctions of literals)
- Implication graph for 2-SAT (Tarjan SCC)
- Resolution rule: (A ∨ x) ∧ (B ∨ ¬x) ⊢ (A ∨ B)
- Resolution proof DAG
- Variable assignment as Boolean vector
- Partial assignment and unit propagation
- Learned clauses and implication graph in CDCL

## L4: Fundamental Laws/Theorems
- **Cook-Levin (1971)**: CIRCUIT-SAT is NP-complete
- **Tseitin (1968)**: Circuit to CNF in O(size) time
- **DPLL completeness**: Davis-Putnam is refutation-complete
- **CDCL completeness**: same as DPLL but more efficient
- **Krom (1967)**: 2-SAT in P (implication graph SCC)
- **Schaefer (1978)**: Dichotomy theorem for SAT variants
- **Haken (1985)**: Exponential resolution lower bounds (pigeonhole)
- **ASPECT: SAT solving is NP-hard (worst-case exponential)

## L5: Algorithms/Methods
- DPLL: branching + unit propagation + pure literal elimination
- CDCL: watch literals, conflict analysis, clause learning, restarts
- Two-watched-literal scheme (Moskewicz et al. 2001)
- VSIDS branching heuristic (variable state independent decaying sum)
- Phase saving
- Restart policies (Luby, geometric)
- Clause deletion (reduceDB in MiniSAT)
- Preprocessing: subsumption, self-subsuming resolution
- Implication graph analysis for UIP (Unique Implication Point)
- 2-SAT linear-time algorithm (Aspvall-Plass-Tarjan 1979)

## L6: Canonical Problems
- CIRCUIT-SAT: NP-complete (Cook 1971)
- 3-SAT: NP-complete (Karp 1972)
- 2-SAT: in P (Krom 1967)
- HORN-SAT: in P (Dowling-Gallier 1984)
- XOR-SAT: in P (Gaussian elimination over GF(2))
- MAX-SAT: NP-hard, APX-complete
- #SAT: #P-complete
- QBF (QSAT): PSPACE-complete

## L7: Applications (Partial)
- Formal verification: bounded model checking
- AI planning: SATPLAN (Kautz-Selman 1992)  
- Cryptanalysis: SAT-based attack on hash functions
- Software verification: CBMC, ESBMC
- Hardware verification: equivalence checking via SAT
- Bioinformatics: haplotype inference

## L8: Advanced Topics (Partial)
- Proof complexity: resolution width-size tradeoffs (Ben-Sasson-Wigderson)
- PCR (polynomial calculus + resolution)
- Local search SAT: WalkSAT, ProbSAT
- Lookahead SAT solvers (march, OKsolver)
- Parallel SAT (ManySAT, Plingeling)
- SMT: SAT modulo theories
- MaxSAT algorithms (branch-and-bound, core-guided)

## L9: Research Frontiers (Partial)
- Machine learning for SAT (NeuroSAT, GNN-guided search)
- Proof logging and DRAT checking
- Crypto-mining (SAT-based cryptocurrency)
- Quantum SAT algorithms (Grover-based)
- SAT competition trends and state-of-art
