/* natural.h -- Master include for the Natural Proofs Barrier module.
 *
 * This header aggregates all sub-headers:
 *   natural_core.h   : TruthTable, NaturalProperty, BooleanCircuit, PRF
 *   shannon.h        : Shannon counting functions
 *   barrier_theorem.h: Razborov-Rudich theorem and proof steps
 *   three_barriers.h : Relativization, Natural Proofs, Algebrization
 *
 * Include this single header for access to the full API.
 */
#ifndef NATURAL_H
#define NATURAL_H

#include "natural_core.h"
#include "shannon.h"
#include "barrier_theorem.h"
#include "three_barriers.h"

/* === Legacy compatibility wrappers === */

/* These functions are now in natural_core.h / shannon.h / barrier_theorem.h */

/*
 * shannon_circuit_count: Alias for shannon_circuit_upper_bound.
 * Deprecated: use shannon_circuit_upper_bound directly.
 */
#define shannon_circuit_count(n, S) shannon_circuit_upper_bound(n, S)

/*
 * shannon_function_count: Count of Boolean functions on n inputs.
 * Now defined in shannon.h; kept here for backward compatibility.
 */

/*
 * shannon_feasible_fraction: Fraction of functions computable at size S.
 * Now defined in shannon.h.
 */

/*
 * test_natural_property: Alias for shannon_natural_criteria_check.
 * Tests whether "complexity > S" is a natural property.
 */
#define test_natural_property(n, S) shannon_natural_criteria_check(n, S)

/* Legacy demo functions - now implemented in src/natural.c */
void natural_proofs_demo(void);
void natural_bench_demo(void);
void property_tester_demo(void);
void largeness_exp_demo(void);
void prf_sim_demo(void);
void three_barriers_demo(void);
void bypass_candidates_demo(void);
void natural_examples_demo(void);
void razborov_rudich_proof_demo(void);
void constructive_violation_demo(void);
void prf_construction_demo(void);

#endif /* NATURAL_H */