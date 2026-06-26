/* monotone.h -- Monotone Circuit Lower Bounds: Main Header
 *
 * Master include file for the monotone circuit lower bounds module.
 * Includes all sub-module headers:
 *   - monotone_circuit.h   : monotone circuit model (AND/OR/INPUT gates)
 *   - monotone_graph.h     : graph structures & clique detection
 *   - sunflower.h          : Erdos-Rado sunflower lemma
 *   - approximator.h       : Razborov's method of approximations
 *   - monotone_lowerbounds.h : lower bound computations
 *
 * This module covers Razborov's 1985 theorem: CLIQUE requires
 * super-polynomial monotone circuits. This was the FIRST
 * super-polynomial circuit lower bound in history.
 *
 * References:
 *   Razborov (1985), Doklady AN SSSR
 *   Alon & Boppana (1987), Combinatorica
 *   Jukna (2012), "Boolean Function Complexity"
 *   Arora & Barak (2009), "Computational Complexity"
 */
#ifndef MONOTONE_H
#define MONOTONE_H

/* Standard library includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include <time.h>

/* Sub-module headers */
#include "monotone_circuit.h"
#include "monotone_graph.h"
#include "sunflower.h"
#include "approximator.h"
#include "monotone_lowerbounds.h"

/* ============================================================
 * Backward-compatible aliases and convenience wrappers
 * ============================================================ */

/* Graph convenience aliases (kept for backward compatibility with
 * original monotone.h API) */
#define graph_create      graph_create
#define graph_add_edge    graph_add_edge
#define graph_has_edge    graph_has_edge
#define graph_complete    graph_complete
#define graph_random      graph_random
#define graph_has_clique  graph_has_clique
#define graph_free        graph_free

/* Convenience: check if a set of p k-sets forms a sunflower.
 * Alias for sunflower_check(). */
static inline int is_sunflower(uint64_t *sets, int p, uint64_t *core) {
    return sunflower_check(sets, p, 0, core);
}

/* Convenience: Erdos-Rado sunflower bound */
static inline double sunflower_bound(int p, int k) {
    return sunflower_bound_erdos_rado(p, k);
}

/* Convenience: Razborov lower bound */
static inline double razborov_clique_lower(int n, int k) {
    return razborov_clique_lower_bound(n, k);
}

/* Convenience: monotone size estimate */
static inline double monotone_size_estimate_wrapper(int n, int k, int depth) {
    return monotone_size_estimate(n, k, depth);
}

/* ============================================================
 * Demo functions (provided by src/monotone.c)
 * ============================================================ */

void monotone_demo(void);
void monotone_approx_demo(void);
void sunflower_demo(void);
void clique_search_demo(void);
void monotone_bench_demo(void);
void monotone_circuit_demo(void);
void monotone_lower_bounds_demo(void);
void natural_proofs_demo(void);

#endif /* MONOTONE_H */