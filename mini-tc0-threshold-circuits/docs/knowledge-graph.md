# Knowledge Graph — mini-tc0-threshold-circuits

## L1: Definitions
- TC^0: constant-depth, polynomial-size circuits with MAJORITY/THRESHOLD gates
- THRESHOLD_k gate: outputs 1 iff ≥ k inputs are 1
- MAJORITY gate: THRESHOLD_{n/2} (unbounded fan-in)
- Exact-count gate: outputs 1 iff exactly k inputs are 1
- Symmetric gate: output depends only on Hamming weight
- Weighted threshold: Σ w_i x_i ≥ T
- TC^i: depth O(log^i n) threshold circuits

## L2: Core Concepts
- AC^0 ⊂ TC^0 ⊆ NC^1 (known proper inclusion)
- TC^0 contains: MAJORITY, counting, multiplication, division
- Multiplication ∈ TC^0 (CRT + counting)
- Division ∈ TC^0 (Beame-Cook-Hoover 1986)
- Sorting ∈ TC^0 (via sorting networks)
- TC^0 = languages decided by constant-depth threshold circuits
- Threshold circuits can simulate AND/OR (MAJ with padding)
- TC^0 = neural networks with polynomial weights (Maass et al.)

## L3: Mathematical Structures
- Threshold function: f(x) = sgn(Σ w_i x_i - t)
- Linear threshold element: neuron model
- Symmetric function representation
- Chinese Remainder representation for multiplication
- Carry-save adder tree for counting
- Weight representation: integers vs reals (equivalent by Minsky-Papert)
- Sigmoid approximation of threshold

## L4: Fundamental Laws/Theorems
- **Beame-Cook-Hoover (1986)**: Division ∈ TC^0
- **Chandra-Stockmeyer-Vishkin (1984)**: Majority via polynomial-weight neural nets
- **Reif-Tate (1992)**: TC^0 = constant-depth threshold circuits
- **Allender (1999)**: TC^0 proper hierarchy (for depth)
- **Furst-Saxe-Sipser/Hastad**: MAJORITY ∉ AC^0
- **Maass-Schnitger-Sontag (1991)**: TC^0 = poly-weight neural nets
- **Hajnal et al. (1993)**: MAJORITY ∉ AC^0[p] for any prime p

## L5: Algorithms/Methods
- Majority via AKS sorting network: O(n log n) comparators
- Chvátal's polynomial-weight construction
- Carry-save addition: count 3→2 reduction, O(log n) depth
- Chinese Remainder multiplication: reduce to parallel counts
- Threshold circuit composition
- Symmetric function decomposition
- Binary to unary conversion

## L6: Canonical Problems
- MAJORITY: canonical TC^0-complete function
- MULTIPLICATION: in TC^0 (via CRT)
- DIVISION: in TC^0 (Beame-Cook-Hoover)
- SORTING: in TC^0 (sorting networks are TC^0)
- COUNTING (Hamming weight): TC^0-complete
- THRESHOLD-k: TC^0-complete (reducible to MAJORITY)
- ITERATED-MULTIPLICATION: in TC^0
- MATRIX-POWERING: in TC^0 (iterated multiplication)

## L7: Applications (Partial)
- Neural network theory: TC^0 = polynomial-weight, constant-depth nets
- Digital signal processing: FIR filters via threshold logic
- Hardware design: threshold logic gates (SET, TLL)

## L8: Advanced Topics (Partial)
- TC^0 vs NC^1: unknown if TC^0 = NC^1
- Probabilistic threshold circuits
- Bounded-weight threshold circuits
- Majority is hardest threshold function
- Depth-2 threshold circuit lower bounds

## L9: Research Frontiers (Partial)
- TC^0 = NC^1? (major open problem)
- Threshold circuit lower bound techniques
- Quantum threshold circuits
- Bio-inspired computing (DNA strand displacement)
