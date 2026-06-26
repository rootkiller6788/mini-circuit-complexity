#ifndef RS_CORE_H
#define RS_CORE_H
/*============================================================================
 * rs_core.h — Razborov-Smolensky: Circuit Model and Core Definitions
 *
 * Defines the AC0 and AC0[p] circuit models that are the subject of
 * the Razborov-Smolensky lower bounds.
 *
 * AC0: constant-depth, unbounded-fanin, polynomial-size circuits
 *      with AND, OR, NOT gates.
 * AC0[p]: same as AC0 but additionally allows MOD_p gates.
 *
 * Knowledge: L1 (circuit definitions), L2 (circuit complexity classes),
 * L3 (Boolean circuit as DAG), L6 (canonical problems: MAJORITY, PARITY).
 *
 * References:
 *   Vollmer, "Introduction to Circuit Complexity" (1999), §4
 *   Arora & Barak, "Computational Complexity", §6, §14
 *==========================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>

/*----------------------------------------------------------------------------
 * L1 — Gate types
 *----------------------------------------------------------------------------*/
typedef enum {
    GATE_INPUT   = 0,   /* input variable x_i                  */
    GATE_AND     = 1,   /* unbounded fanin AND                 */
    GATE_OR      = 2,   /* unbounded fanin OR                  */
    GATE_NOT     = 3,   /* fanin-1 NOT                         */
    GATE_MODP    = 4,   /* MOD_p gate: 1 iff sum inputs≡0 mod p */
    GATE_CONST0  = 5,   /* constant 0                          */
    GATE_CONST1  = 6,   /* constant 1                          */
    GATE_OUTPUT  = 7    /* output marker                       */
} RSGateType;

/*----------------------------------------------------------------------------
 * L1 — Circuit gate
 *----------------------------------------------------------------------------*/
typedef struct {
    RSGateType  type;          /* gate type                           */
    int         id;            /* unique gate-id in circuit            */
    int         modp;          /* prime for MOD_p gates (0 otherwise)  */
    int         n_inputs;      /* fanin                                */
    int*        inputs;        /* gate-ids of input gates (sorted)     */
    int         input_var;     /* variable index (for INPUT gates)    */
    int         depth;         /* depth from inputs                    */
    int         value;         /* cached evaluation value              */
} RSGate;

/*----------------------------------------------------------------------------
 * L1 — Circuit (DAG)
 *----------------------------------------------------------------------------*/
typedef struct {
    int         n_inputs;       /* number of input variables           */
    int         n_gates;        /* total number of gates               */
    int         capacity;       /* allocated gate capacity             */
    RSGate**    gates;          /* array of gate pointers              */
    int         output_id;      /* gate-id of output gate              */
    int         depth;          /* maximum depth of circuit            */
    int64_t     size;           /* number of non-input gates           */
    int         is_constant;    /* circuit computes constant function  */
    int         has_modp;       /* uses MOD_p gates                    */
} RSCircuit;

/*============================================================================
 * L5 — Circuit Construction API
 *==========================================================================*/

/* Lifecycle */
RSCircuit*  rs_circuit_create(int n_inputs);
void        rs_circuit_free(RSCircuit* C);
RSCircuit*  rs_circuit_copy(const RSCircuit* C);

/* Gate construction */
int rs_circuit_add_input(RSCircuit* C, int var_idx);
int rs_circuit_add_constant(RSCircuit* C, int value);
int rs_circuit_add_not(RSCircuit* C, int input_id);
int rs_circuit_add_and(RSCircuit* C, const int* input_ids, int n_in);
int rs_circuit_add_or(RSCircuit* C, const int* input_ids, int n_in);
int rs_circuit_add_modp(RSCircuit* C, int p, const int* input_ids, int n_in);

/* Circuit evaluation */
int  rs_circuit_eval(const RSCircuit* C, const int* x);
void rs_circuit_reeval(RSCircuit* C, const int* x);  /* update cached values */

/* Topology queries */
int  rs_circuit_depth(const RSCircuit* C);
int  rs_circuit_n_gates(const RSCircuit* C);
int  rs_circuit_size(const RSCircuit* C);
int  rs_circuit_fanin_max(const RSCircuit* C);
void rs_circuit_set_output(RSCircuit* C, int gate_id);

/* Print */
void rs_circuit_print(const RSCircuit* C);
void rs_circuit_print_stats(const RSCircuit* C);

/*============================================================================
 * L5 — Circuit construction (helper: build specific circuits)
 *==========================================================================*/

/* Build AND of all inputs: AND(x_0, ..., x_{n-1})                    */
RSCircuit* rs_circuit_and_all(int n);

/* Build OR of all inputs: OR(x_0, ..., x_{n-1})                      */
RSCircuit* rs_circuit_or_all(int n);

/* Build MAJORITY circuit (naive DNF, size ~2^n)                      */
RSCircuit* rs_circuit_majority_dnf(int n);

/* Build PARITY(XOR) circuit (naive DNF, size ~2^n)                    */
RSCircuit* rs_circuit_parity_dnf(int n);

/* Build MOD_p circuit (1 iff Σx_i ≡ 0 mod p)                         */
RSCircuit* rs_circuit_modp_naive(int n, int p);

/*============================================================================
 * L5 — Circuit analysis
 *==========================================================================*/

/* Truth table extraction (array of 2^n values)                         */
int* rs_circuit_truth_table(const RSCircuit* C);

/* Check if circuit computes exactly the given truth table               */
int  rs_circuit_checks(const RSCircuit* C, const int* tt);

/* Check if two circuits are equivalent (brute force for small n)        */
int  rs_circuit_equivalent(const RSCircuit* A, const RSCircuit* B);

/* Count gates of each type */
void rs_circuit_gate_counts(const RSCircuit* C,
                            int* n_input, int* n_and, int* n_or,
                            int* n_not, int* n_modp);

/*----------------------------------------------------------------------------
 * L6 — Canonical problem verification
 *----------------------------------------------------------------------------*/

/* Verify: MAJORITY on n bits returns 1 iff more than n/2 inputs are 1 */
int fn_majority(const int* x, int n);

/* Verify: PARITY on n bits returns 1 iff sum of inputs is odd          */
int fn_parity(const int* x, int n);

/* Verify: MOD_p on n bits returns 1 iff Σx_i ≡ 0 mod p                 */
int fn_modp(const int* x, int n, int p);

/* Verify: THRESHOLD_k on n bits returns 1 iff Σx_i ≥ k                 */
int fn_threshold(const int* x, int n, int k);

/* Verify: AND, OR, NOT functions on n bits (reduction to first k bits) */
int fn_and_k(const int* x, int n, int k);
int fn_or_k(const int* x, int n, int k);

#endif /* RS_CORE_H */
