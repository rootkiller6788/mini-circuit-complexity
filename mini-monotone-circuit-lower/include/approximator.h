/* approximator.h -- Razborov's Method of Approximations (1985)
 *
 * Razborov's method: replace each gate in a monotone circuit with
 * a "small" approximator - a restricted set of positive/negative
 * examples that approximates the function computed at that gate.
 *
 * Key components:
 *   1. Approximator A = (A+, A-) where:
 *      - A+ is a set of positive examples (minterms)
 *      - A- is a set of negative examples (maxterms)
 *   2. For AND gate: A_and approx = intersect(A1, A2) - use sunflower
 *      lemma to bound the size of the resulting approximator
 *   3. For OR gate: A_or approx = union(A1, A2) - additive blowup
 *   4. The output approximator differs from the target function
 *      either on some positive or some negative input
 *   5. If original circuit is too small, approximator stays small,
 *      but CLIQUE requires large approximators -> contradiction
 *
 * This was the FIRST super-polynomial circuit lower bound in history.
 *
 * References:
 *   Razborov (1985), Doklady AN SSSR 281(4), 798-801
 *   Boppana & Sipser (1990), "The complexity of finite functions"
 *   Jukna (2012), "Boolean Function Complexity", Ch. 10
 *   Vollmer (1999), "Introduction to Circuit Complexity", Ch. 5
 */
#ifndef APPROXIMATOR_H
#define APPROXIMATOR_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include "monotone_graph.h"

/* ============================================================
 * L1 Definitions: Approximator
 * ============================================================ */

/* A positive example for CLIQUE_{n,k}:
 * a graph that has a k-clique = a YES instance */
typedef struct {
    Graph  *graph;
    Clique *witness;
} PositiveExample;

/* A negative example for CLIQUE_{n,k}:
 * a graph that has NO k-clique = a NO instance */
typedef enum {
    NEG_TYPE_COLORING,
    NEG_TYPE_TURAN,
    NEG_TYPE_CERTIFICATE,
} NegativeType;

typedef struct {
    Graph       *graph;
    NegativeType type;
    int          num_colors;
    int        **color_classes;
    int         *color_sizes;
} NegativeExample;

/* ============================================================
 * L1: Approximator Structure
 * ============================================================ */

/* An approximator for CLIQUE_{n,k}: A = (A+, A-) */
typedef struct {
    int  n, k;
    PositiveExample **pos;
    int               num_pos, pos_capacity;
    NegativeExample **neg;
    int               num_neg, neg_capacity;
    int  max_size;
    int  and_gates_processed;
    int  or_gates_processed;
    int  sunflower_compressions;
} Approximator;

/* ============================================================
 * L2 Core Concepts: Approximator Operations
 * ============================================================ */

Approximator* approx_create(int n, int k, int max_size);
void approx_free(Approximator *a);
void approx_add_positive(Approximator *a, const PositiveExample *ex);
void approx_add_negative(Approximator *a, const NegativeExample *ex);

PositiveExample* positive_example_create_random(int n, int k);
NegativeExample* negative_example_create_turan(int n, int k);
void positive_example_free(PositiveExample *ex);
void negative_example_free(NegativeExample *ex);

/* ============================================================
 * L2: Approximator Evaluation
 * ============================================================ */

int approx_accepts(const Approximator *a, const Graph *g);
int approx_rejects(const Approximator *a, const Graph *g);
void approx_count_errors(const Approximator *a,
                          int *false_pos, int *false_neg,
                          int *correct_pos, int *correct_neg);

/* ============================================================
 * L3: Approximator at Gates
 * ============================================================ */

Approximator* approx_and_gate(const Approximator *a1,
                               const Approximator *a2, int max_size);

Approximator* approx_or_gate(const Approximator *a1,
                              const Approximator *a2, int max_size);

/* ============================================================
 * L4: Razborov Simulator
 * ============================================================ */

struct MonotoneCircuit;

Approximator* approx_simulate_circuit(const struct MonotoneCircuit *c,
                                       const int *input_assignment,
                                       int max_size);

/* ============================================================
 * L5: Size Analysis
 * ============================================================ */

double approx_expected_size(int initial_size, int depth,
                             double and_prob, int p, int k);
int approx_is_valid_positive(const Approximator *a);
int approx_is_valid_negative(const Approximator *a);

/* ============================================================
 * L5: Clique Approximator Construction
 * ============================================================ */

Approximator* approx_input_edge(int n, int k, int u, int v, int max_size);

/* ============================================================
 * L6: Canonical Examples
 * ============================================================ */

void approx_demo_tiny_circuit(void);
double approx_lower_bound_size(int n, int k, int depth);

/* ============================================================
 * L8 Advanced: Frontier Approximator
 * ============================================================ */

typedef struct {
    int                n, k, max_size;
    Clique           **frontier_pos;
    NegativeExample  **frontier_neg;
    int                num_pos, num_neg;
} FrontierApproximator;

FrontierApproximator* frontier_approx_create(int n, int k, int max_size);
void frontier_approx_free(FrontierApproximator *fa);
int frontier_approx_accepts(const FrontierApproximator *fa, const Graph *g);

Approximator* approx_andreev_construct(int n, int k, int max_size);

/* ============================================================
 * L9: Extended Approximators
 * ============================================================ */

Approximator* approx_perfect_matching(int n, int max_size);
Approximator* approx_st_connectivity(int n, int s, int t, int max_size);

#endif /* APPROXIMATOR_H */
