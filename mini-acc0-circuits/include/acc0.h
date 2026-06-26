/* acc0.h -- ACC0: AC0 + MOD_m gates
 * ==========================================
 * ACC0 = AC0 + MOD_m gates (for all m >= 2).
 *
 * ACC0[m] = AC0 circuits with MOD_m gates.
 * ACC0 = union over all m of AC0[m].
 *
 * Key Results:
 * - Razborov (1987), Smolensky (1987): MOD_q not in AC0[p] (p,q distinct primes).
 * - Williams (2014): NEXP not in ACC0 (breakthrough).
 * - Open (since 1987): Is MAJORITY in ACC0?
 *
 * References:
 * - Arora & Barak (2009), Ch.6, Ch.14
 * - Vollmer (1999), Ch.4
 * - Williams (2014), JACM 61(1)
 */

#ifndef ACC0_H
#define ACC0_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#define ACC0_MAX_FANIN 256

/* ================================================================
 * Gate Types
 * ================================================================ */
typedef enum {
    ACC0_INPUT       = 0,
    ACC0_AND         = 1,
    ACC0_OR          = 2,
    ACC0_NOT         = 3,
    ACC0_CONST       = 4,
    ACC0_MOD2        = 5,
    ACC0_MOD3        = 6,
    ACC0_MOD5        = 7,
    ACC0_MOD7        = 8,
    ACC0_MOD11       = 9,
    ACC0_MOD13       = 10,
    ACC0_MOD_COMPOSITE = 11
} ACC0GateType;

/* ================================================================
 * Gate and Circuit Structures
 * ================================================================ */
typedef struct {
    ACC0GateType type;
    int          i1;
    int          i2;
    int          fans[ACC0_MAX_FANIN];
    int          nfans;
    int          modulus;
    int          level;
} ACC0Gate;

typedef struct {
    ACC0Gate    *gates;
    int          ngates;
    int          max_gates;
    int          ninputs;
    int          noutputs;
    int         *outputs;
    int          depth;
    int          size;
} ACC0Circuit;

typedef struct {
    int size, depth, wire_count, max_fanin;
    int mod_gate_count, and_gate_count, or_gate_count, not_gate_count;
} ACC0CircuitMeasures;

typedef struct {
    ACC0Circuit **circuits;
    int max_n, *sizes, *depths;
} ACC0CircuitFamily;

/* ================================================================
 * Core API — Circuit Lifecycle
 * ================================================================ */
ACC0Circuit* acc0_circuit_create(int max_gates);
void         acc0_circuit_free(ACC0Circuit *c);

/* ================================================================
 * Gate Addition
 * ================================================================ */
int acc0_add_input(ACC0Circuit *c);
int acc0_add_constant(ACC0Circuit *c, int value);
int acc0_add_not(ACC0Circuit *c, int in);
int acc0_add_and(ACC0Circuit *c, int in1, int in2);
int acc0_add_or(ACC0Circuit *c, int in1, int in2);
int acc0_add_mod(ACC0Circuit *c, int modulus);
int acc0_add_mod_composite(ACC0Circuit *c, int modulus);
int acc0_set_fanin(ACC0Circuit *c, int gate_id, const int *fans, int n);
int acc0_add_fanin_one(ACC0Circuit *c, int gate_id, int fanin);
int acc0_set_outputs(ACC0Circuit *c, const int *outputs, int n);

/* ================================================================
 * Evaluation
 * ================================================================ */
int acc0_gate_eval(ACC0Circuit *c, int gate_id, const int *inputs, int *visited);
int acc0_circuit_eval(ACC0Circuit *c, const int *inputs);

/* ================================================================
 * Analysis
 * ================================================================ */
int                acc0_circuit_depth(ACC0Circuit *c);
ACC0CircuitMeasures acc0_circuit_measures(ACC0Circuit *c);
int*               acc0_topological_order(ACC0Circuit *c);
int                acc0_is_acyclic(ACC0Circuit *c);
int*               acc0_fanin_closure(ACC0Circuit *c, int gate_id);
int*               acc0_fanout_closure(ACC0Circuit *c, int gate_id, int *count);
ACC0Circuit*       acc0_circuit_clone(ACC0Circuit *c);

/* ================================================================
 * Verification
 * ================================================================ */
int acc0_verify_truth_table(ACC0Circuit *c, int (*target)(const int*, int), int n);
int acc0_circuits_equivalent(ACC0Circuit *c1, ACC0Circuit *c2, int ninputs);

/* ================================================================
 * Output
 * ================================================================ */
void acc0_print_circuit(ACC0Circuit *c);
void acc0_print_stats(ACC0Circuit *c);

/* ================================================================
 * Williams Bound
 * ================================================================ */
double acc0_williams_bound(int n);

/* ================================================================
 * Circuit Builders (from acc0_builders.c)
 * ================================================================ */
ACC0Circuit* acc0_build_parity_circuit(int n);
ACC0Circuit* acc0_build_mod_circuit(int n, int modulus);
ACC0Circuit* acc0_build_and_of_mods(int n, int m1, int m2);
ACC0Circuit* acc0_build_parity_via_mod3(int n);
ACC0Circuit* acc0_build_majority_dnf(int n);
ACC0Circuit* acc0_build_threshold_circuit(int n, int k);
ACC0Circuit* acc0_build_exactly_k(int n, int k);
ACC0Circuit* acc0_build_comparison(int nbits);
ACC0Circuit* acc0_build_full_adder(void);
ACC0Circuit* acc0_build_const_mul_mod(int n, int constant, int modulus);

/* ================================================================
 * SAT Solvers (from acc0_sat.c)
 * ================================================================ */
int        acc0_sat_exhaustive_simple(ACC0Circuit *c, int *assignment);
int        acc0_sat_dpll(ACC0Circuit *c, int *out_assignment);
int        acc0_sat_randomized(ACC0Circuit *c, int *out_assignment, int max_trials);
int        acc0_sat_mod2(ACC0Circuit *c, int *out_assignment);
long long  acc0_count_solutions(ACC0Circuit *c);

/* ================================================================
 * Demo Functions
 * ================================================================ */
void acc0_demo(void);
void acc0_verify_demo(void);
void acc0_fuzz_demo(void);
void acc0_mod_gate_eval_demo(void);
void acc0_crt_demo(void);
void acc0_gate_type_demo(void);
void acc0_poly_demo(void);
void acc0_circuit_poly_demo(void);
void acc0_poly_identity_demo(void);
void acc0_lower_bound_demo(void);
void acc0_sat_demo(void);
void acc0_williams_demo(void);
void acc0_builders_demo(void);
void acc0_sat_demo_full(void);
void acc0_research_frontier_demo(void);

#endif /* ACC0_H */
