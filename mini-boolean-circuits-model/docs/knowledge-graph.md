# Knowledge Graph — mini-boolean-circuits-model

## L1: Definitions
- Boolean circuit: DAG of gates (AND, OR, NOT, NAND, NOR, XOR, MAJ, THR)
- Gate types and truth tables
- Circuit size = number of gates
- Circuit depth = longest input-to-output path
- Circuit width = max gates at any level
- Fan-in and fan-out
- Circuit basis (standard, NAND-only, NOR-only)
- INPUT, CONST0, CONST1 leaf gates
- Multi-output circuits
- Circuit DAG property

## L2: Core Concepts
- P/poly: languages with poly-size circuit families
- NC: polylog depth, poly size (efficient parallel)
- AC: constant depth, unbounded fan-in
- TC: threshold gate circuits
- Non-uniform computation
- Circuit family = {C_n} for each input size n
- Uniform vs non-uniform circuit families
- Circuit complexity measures: size-depth product, density, tradeoff
- Boolean function computed by a circuit
- Functional completeness of {NAND} and {NOR}

## L3: Mathematical Structures
- BooleanCircuit struct: gates array, metadata
- Gate struct: type, inputs, fanin, value cache, level
- Circuit DAG topology and topological ordering
- Truth tables as Boolean function representations
- Binary serialization format
- Graphviz DOT export
- Implicant representation (value, mask)
- CircuitFamily struct: array of circuits, metadata

## L4: Fundamental Laws/Theorems
- Shannon lower bound (1949): Omega(2^n / n) circuit size for almost all functions
- Lupanov upper bound (1958): O(2^n / n) circuit size for all functions
- Hastad switching lemma (1986): PARITY not in AC^0, exp(n^{1/(d-1)}) bound
- Razborov-Smolensky (1987): MOD-p requires exponential size in AC^0[MOD-q]
- Karchmer-Wigderson (1988): circuit depth = communication complexity
- Nechiporuk bound (1966): independent subfunctions lower bound
- Karp-Lipton theorem: NP in P/poly implies PH collapses
- De Morgan duality: DNF/CNF equivalence

## L5: Algorithms/Methods
- Brute-force circuit evaluation (memoized recursive)
- Topological-order evaluation (non-recursive)
- Circuit SAT via brute-force enumeration
- Circuit equivalence via brute-force checking
- Quine-McCluskey exact 2-level minimization
- Reed-Muller/ESOP synthesis via fast Walsh transform
- Shannon decomposition (MUX-based synthesis)
- NAND-only transformation (functional completeness)
- NOR-only transformation
- Fan-in 2 decomposition
- Redundant gate removal
- Circuit composition
- Circuit balancing (chain-to-tree)
- Constant propagation
- Circuit family generation
- Random circuit generation (Shannon model)
- Carry-save adder (CSA) tree for counting
- Ripple-carry and carry-lookahead addition
- Batcher odd-even merge sort network
- Technology mapping (NAND2, NOR2, AND-NOT bases)
- Circuit extraction (cone of influence)
- Common subexpression factorization

## L6: Canonical Problems
- PARITY: separates AC^0 from NC^1
- MAJORITY: canonical TC^0 function
- CLIQUE(k): NP-complete, P/poly for fixed k
- HAMILTONIAN PATH: NP-complete
- PERFECT MATCHING: in P (Edmonds)
- CIRCUIT-SAT: NP-complete
- #CIRCUIT-SAT: #P-complete
- CIRCUIT TAUTOLOGY: coNP-complete
- MOD-m: not in AC^0 for prime m != 2
- THRESHOLD(k): canonical symmetric function
- MULTIPLICATION: in TC^0
- ADDITION: in AC^0
- SORTING: in TC^0, NC^1

## L7: Applications (Partial)
- Logic synthesis for digital circuit design
- Circuit optimization (minimization, technology mapping)
- Complexity class membership testing
- Circuit family benchmarking
- Truth-table-based function synthesis

## L8: Advanced Topics (Partial)
- Hastad lower bound estimates
- Razborov-Smolensky modular gates analysis
- Karchmer-Wigderson game depth lower bounds
- Genetic circuit synthesis
- Simulated annealing optimization
- BDD-based circuit synthesis

## L9: Research Frontiers (Partial)
- Meta-complexity: circuit lower bounds via natural proofs barrier
- Proof complexity: circuit-based proof systems
- GCT (Geometric Complexity Theory) approach
