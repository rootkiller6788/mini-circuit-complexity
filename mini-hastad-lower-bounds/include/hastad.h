/**
 * hastad.h — Core Definitions for Hastad Lower Bounds
 *
 * Hastad (1986): PARITY ∉ AC0. Depth-d AC0 circuits for PARITY
 * require size ≥ exp(Ω(n^{1/(d-1)})). Optimal lower bound.
 *
 * References:
 *   Hastad, J. "Almost optimal lower bounds for small depth circuits"
 *     STOC 1986, pp. 6-20. (Godel Prize 1994)
 *   Arora & Barak (2009), Ch.14: Circuit Lower Bounds
 *   Vollmer (1999), Ch.4: The Switching Lemma
 */
#ifndef HASTAD_H
#define HASTAD_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

/* Gate type for circuit model */
typedef enum {
    GATE_AND, GATE_OR, GATE_NOT,
    GATE_INPUT, GATE_CONST_ZERO, GATE_CONST_ONE,
    GATE_MOD3, GATE_MOD5, GATE_MAJORITY, GATE_PARITY
} GateType;

/* Boolean value: -1=free(star), 0=false, 1=true */
typedef enum { BOOL_UNDEF = -1, BOOL_FALSE = 0, BOOL_TRUE = 1 } BoolVal;

/* Hastad's theorem: depth-d AC0 for PARITY needs size ≥ exp(n^{1/(d-1)}/c) */
double hastad_lower_bound(int n, int d);
double hastad_optimal_p(int n, int d);
double hastad_exponent(int d);
double dnf_parity_lower_bound(int n);
double cnf_parity_lower_bound(int n);
int    parity_restricted_decompose(const int* x, int n, const int* restr, int* k, int* c);
double hastad_proof_simulate(int n, int d, int* steps, int* final_k);

/* Subsystem includes */
#include "boolean_funcs.h"
#include "circuit_model.h"
#include "restriction.h"
#include "switching_lemma.h"

/* ── Additional declarations from depth_reduction.c ─────────────── */
void   depth_reduction_trace(int n, int d);
void   depth_reduction_compare(int n);
void   hastad_tightness_check(int n, int d);
double parity_upper_bound_size(int n, int d);

/* ── Additional declarations from restriction.c ─────────────────── */
void restriction_process_simulate(int n, int d, double* final_p,
                                   double* final_n);

/* ── Additional declarations from switching_lemma.c ─────────────── */
void switching_lemma_full_simulation(int n, int d);
double switching_p_from_k(int k, double epsilon);
int    switching_s_from_kp(int k, double p, double epsilon);
double switching_lemma_monte_carlo(int n, int k, double p, int s, int trials);
double switching_lemma_prob_bound(int k, double p, int s);
double hastad_switching_p(int n, int depth);

/* Demos */
void hastad_lb_demo(void);
void hastad_iterative_demo(void);
void depth_reduction_demo(void);
void parity_lb_verify_demo(void);
void hastad_bench_demo(void);
void switching_lemma_full_demo(void);
void restriction_stability_demo(void);
void cnf_lower_demo(void);
void hastad_optimal_demo(void);
void hastad_comparison_demo(void);
void hastad_history_demo(void);
void hastad_application_demo(void);

#endif
