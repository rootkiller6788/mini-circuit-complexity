/* nc_circuit.h — NCⁱ Circuit Definitions
 * =====================================================================
 * NCⁱ: fan-in 2, depth O(logⁱ n), polynomial-size circuits.
 * NC = ∪_{i≥1} NCⁱ ("Nick's Class")
 *
 * NCⁱ corresponds to problems solvable in parallel time O(logⁱ n)
 * with poly(n) processors on a PRAM (Parallel Random Access Machine).
 *
 * Key facts:
 *   NC¹ ⊆ L ⊆ NL ⊆ NC² ⊆ NC³ ⊆ ... ⊆ NC ⊆ P
 *   NC¹ = poly-size width-5 branching programs (Barrington 1989)
 *   NL = coNL (Immerman-Szelepcsényi 1988)
 *   PARITY ∈ NC¹, MAJORITY ∈ TC⁰ ⊆ NC¹
 *   CVP (Circuit Value) is P-complete → if CVP ∈ NC then P = NC
 *
 * NC¹-complete problems:
 *   - Boolean Formula Evaluation (BFE, Buss 1987)
 *   - Regular language membership
 *   - Matrix multiplication over fixed ring
 *
 * References:
 *   AB §6.7, §14.2
 *   Barrington (1989) — NC¹ = width-5 BP
 *   Buss (1987) — BFE is NC¹-complete
 *   Cook (1985) — Taxonomy of problems in NC
 */
#ifndef NC_CIRCUIT_H
#define NC_CIRCUIT_H

#include "ac0nc.h"

/* ─── NCⁱ Circuit Constraints ──────────────────────────────────── */
/* NCⁱ circuits have fan-in 2 (bounded) and depth O(logⁱ n).
 * In practice: NC¹ depth ≤ log₂ n, NC² depth ≤ log²₂ n, etc. */
#define NC1_MAX_FANIN       2
#define NC_MAX_FANIN        2

/* ─── NCⁱ Circuit Type ──────────────────────────────────────────── */
typedef struct {
    AC0Circuit *circuit;
    int         i;             /* NCⁱ level (1 for NC¹, 2 for NC²...) */
    int         depth_bound;   /* Maximum allowed depth */
} NCCircuit;

NCCircuit* nc_circuit_create(int i, int n_inputs, int capacity);
void       nc_circuit_free(NCCircuit *c);
int        nc_circuit_add_and(NCCircuit *c, int i1, int i2);
int        nc_circuit_add_or(NCCircuit *c, int i1, int i2);
int        nc_circuit_add_not(NCCircuit *c, int i1);
int        nc_circuit_add_xor(NCCircuit *c, int i1, int i2);

/* ─── NC¹-specific builders ─────────────────────────────────────── */
/* Balanced XOR tree: depth = ceil(log₂ n), size = 2n */
NCCircuit* nc1_build_parity_circuit(int n);

/* Full adder: sum = (a⊕b)⊕c_in, carry = (a∧b)∨(c_in∧(a⊕b)) */
NCCircuit* nc1_build_full_adder_circuit(void);

/* Ripple-carry addition of two n-bit numbers: depth O(n), size O(n). */
NCCircuit* nc1_build_ripple_adder(int n_bits);

/* Carry-lookahead addition of two n-bit numbers: depth O(log n), size O(n log n). */
NCCircuit* nc1_build_carry_lookahead(int n_bits);

/* Comparator: x ≥ y for two n-bit numbers, depth O(log n) */
NCCircuit* nc1_build_comparator(int n_bits);

/* Multiplication: depth O(log² n) → in NC², not NC¹ */
NCCircuit* nc2_build_multiplier(int n_bits);

/* ─── Branching Programs (Barrington's Theorem) ─────────────────── */
/* A width-w branching program is a sequence of instructions:
 * (i, f₀, f₁) where f₀,f₁ : [w] → [w] are permutations. */
#define BP_MAX_WIDTH        5
#define BP_MAX_INSTRUCTIONS 4096

typedef struct {
    int  variable;          /* which input variable is tested */
    int  perm0[BP_MAX_WIDTH]; /* permutation if x_i = 0 */
    int  perm1[BP_MAX_WIDTH]; /* permutation if x_i = 1 */
} BPInstruction;

typedef struct {
    BPInstruction *instructions;
    int            n_instructions;
    int            n_variables;
    int            width;
} BranchingProgram;

BranchingProgram* bp_create(int n_vars, int width, int max_instructions);
void              bp_free(BranchingProgram *bp);
void              bp_add_instruction(BranchingProgram *bp, int var,
                                      const int *perm0, const int *perm1);
int               bp_evaluate(BranchingProgram *bp, const int *inputs);
BranchingProgram* nc1_to_bp(AC0Circuit *c);
double            barrington_blowup(int circuit_depth);

/* ─── PRAM Model ────────────────────────────────────────────────── */
/* EREW PRAM (Exclusive Read Exclusive Write) is the standard
 * model for NC. NCⁱ = languages accepted by EREW PRAM in time
 * O(logⁱ n) with poly(n) processors. */
typedef struct {
    int *memory;
    int  memory_size;
    int  n_processors;
} PRAM;

PRAM* pram_create(int memory_size, int n_proc);
void  pram_free(PRAM *p);
void  pram_erew_read(PRAM *p, int proc, int addr, int *dest);
void  pram_erew_write(PRAM *p, int proc, int addr, int value);

/* ─── L vs NL vs NC² ───────────────────────────────────────────── */
/* Reachability in directed graph: NL-complete.
 * Reachability in undirected graph: L-complete (Reingold 2008). */
int graph_reachability(int n, const int *edges, int n_edges,
                        int src, int dst);

#endif /* NC_CIRCUIT_H */
