/* tc0_complete.h -- TC0-Complete Problems (L6)
 *
 * TC0-complete problems under AC0 many-one reductions:
 *   MAJORITY, THRESHOLD, SORTING, MULTIPLICATION, DIVISION,
 *   ITERATED ADDITION, COUNT
 */

#ifndef TC0_COMPLETE_H
#define TC0_COMPLETE_H

#include "tc0.h"

/* ================================================================
 * TC0 CIRCUIT CONSTRUCTORS (for TC0-complete problems)
 * ================================================================ */

/* Build MAJORITY_n circuit: depth 1, size n+1 */
TC0Circuit *tc0_build_majority(int n);

/* Build THRESHOLD(n,k) circuit: depth 1, size n+1 */
TC0Circuit *tc0_build_threshold(int n, int k);

/* Build PARITY_n using threshold gates (NOT in TC0, for comparison).
 * Depth O(n) (bad), included to show the TC0 boundary. */
TC0Circuit *tc0_build_parity_threshold(int n);

/* ================================================================
 * CIRCUIT FAMILY MANAGEMENT
 * ================================================================ */

/* Create/free a circuit family descriptor */
TC0CircuitFamily *tc0_family_create(const char *name, int is_tc0,
                                      int is_complete);
void tc0_family_free(TC0CircuitFamily *fam);

/* Create canonical circuit families for TC0-complete problems */
TC0CircuitFamily *tc0_family_majority(void);
TC0CircuitFamily *tc0_family_sorting(void);
TC0CircuitFamily *tc0_family_multiplication(void);

/* ================================================================
 * VERIFICATION
 * ================================================================ */

/* Check if a circuit correctly computes a Boolean function.
 * Exhaustive: checks all 2^n inputs. */
int tc0_verify_circuit_truth_table(const TC0Circuit *c,
                                     const BoolFunc *f);

/* Exhaustively verify MAJORITY_n circuit */
int tc0_verify_majority_circuit(int n);

/* ================================================================
 * COMPLEXITY CLASS HIERARCHY
 * ================================================================ */

/* Print the Boolean circuit complexity hierarchy */
void tc0_complexity_hierarchy(void);

/* ================================================================
 * DEMO
 * ================================================================ */
void tc0_completeness_demo(void);

#endif /* TC0_COMPLETE_H */
