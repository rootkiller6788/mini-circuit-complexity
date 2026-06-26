# Course Tree — mini-switching-lemma

## Prerequisites (what you need to know first)
- **Boolean algebra**: AND, OR, NOT, De Morgan's laws
- **Basic complexity**: P, NP, polynomial time
- **Circuit model**: Gates, fan-in, depth, size
- **Probability**: Basic probability, expectation, union bound
- **Asymptotics**: Big-O, Omega, polynomial, exponential

## This Module Depends On
- `mini-boolean-circuits-model/`: Circuit definition and basic properties
- `mini-ac0-nc-poly-hierarchy/`: AC0 class definition
- `mini-circuit-sat-complexity/`: Circuit satisfiability background

## This Module Is Required By
- `mini-hastad-lower-bounds/`: Direct application of switching lemma
- `mini-razborov-smolensky/`: Extends to MOD_p gates
- `mini-natural-proofs-barrier/`: Switching lemma as natural property
- `mini-tc0-threshold-circuits/`: Contrast with threshold circuits

## Learning Path
```
Boolean Circuits Model
    |
    v
AC0 / NC Hierarchy
    |
    v
SWITCHING LEMMA (this module)
    |
    +---> Hastad Lower Bounds (PARITY ∉ AC0)
    |
    +---> Razborov-Smolensky (MOD_p lower bounds)
    |
    +---> Natural Proofs Barrier
    |
    +---> TC0 (contrast: threshold circuits CAN compute majority)
```
