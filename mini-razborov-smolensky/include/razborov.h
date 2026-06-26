#ifndef RAZBOROV_H
#define RAZBOROV_H
/*============================================================================
 * mini-razborov-smolensky — Main Header
 *
 * Implements the Razborov-Smolensky (1987) polynomial method for proving
 * AC0[p] circuit lower bounds.  MAJORITY ∉ AC0[p] for any prime p≠2.
 *
 * Knowledge coverage: L1 (Definitions), L2 (Core Concepts),
 * L3 (Mathematical Structures), L4 (Fundamental Theorems),
 * L5 (Algorithms), L6 (Canonical Problems),
 * L7 (Applications), L8 (Advanced Topics)
 *
 * References:
 *   Razborov, "Lower bounds on the size of bounded depth circuits
 *     over a complete basis with logical addition" (Mat. Zametki, 1987)
 *   Smolensky, "Algebraic methods in the theory of lower bounds for
 *     Boolean circuit complexity" (STOC 1987)
 *   Arora & Barak, Chapter 14.4
 *==========================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>

/* Include sub-module headers */
#include "gf_poly.h"
#include "rs_core.h"
#include "rs_approx.h"
#include "rs_bounds.h"

/*----------------------------------------------------------------------------
 * Demo entry points — each exercises one facet of the polynomial method
 *----------------------------------------------------------------------------*/
void razborov_smolensky_demo(void);
void poly_arithmetic_demo(void);
void gf_poly_sim_demo(void);
void mod_collision_demo(void);
void rs_bench_demo(void);
void rs_history_demo(void);

#endif /* RAZBOROV_H */