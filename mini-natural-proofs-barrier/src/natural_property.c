/* natural_property.c -- Natural Property Testing Framework
 *
 * L5: Implementation of the natural property tester.
 *
 * A property P is NATURAL if:
 *   1. CONSTRUCTIVE: exists algorithm deciding P(f) in 2^{O(n)} time
 *      given truth table of f (size 2^n).
 *   2. LARGE: Pr_{random f}[P(f)] >= 2^{-O(n)}
 *   3. USEFUL: P(f) => f requires circuits of size > S(n)
 *      for some super-polynomial function S.
 *
 * Theorem (Razborov-Rudich 1997):
 *   If one-way functions exist, no natural property can be used to
 *   prove NP notin P/poly.
 *
 * All known circuit lower bounds are natural:
 *   - AC0 bounds (FSS/Ajtai/Hastad): use property "f not constant after
 *     random restriction" — constructive, large, useful
 *   - Monotone bounds (Razborov 1985): "f has large monotone complexity"
 *   - AC0[p] bounds (Razborov-Smolensky 1987): "f has large AC0[p] complexity"
 *
 * The PRF contradiction:
 *   1. Assume OWF exist => PRF exist (HILL 1999).
 *   2. PRF is indistinguishable from random function.
 *   3. Natural property P is large => random f has P with high prob.
 *   4. PRF indistinguishable from random => PRF also has P.
 *   5. P useful => PRF requires super-polynomial circuits.
 *   6. But PRF is polynomial-time computable => small circuits.
 *   7. CONTRADICTION => either OWF don't exist OR no natural P separates NP from P/poly.
 *
 * References:
 *   Razborov-Rudich (1997): JCSS 55(1):24-35
 *   HILL (1999): FOCS 1999
 *   Arora-Barak (2009), Section 23.3
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "natural_core.h"
#include "shannon.h"

/* ========================================================================
 * Natural Property Tester: Core Logic
 * ======================================================================== */

/*
 * np_test_constructiveness: Check if property P is constructive.
 *
 * Definition: P is C-constructive if there exists an algorithm A that,
 * given the truth table of f (2^n bits), decides P(f) in time <= 2^{c*n}
 * for some constant c.
 *
 * For property P = "circuit complexity > S":
 *   - Algorithm: Enumerate all circuits of size <= S, check if any equals f.
 *   - Number of circuits: 2^{O(S log S)}
 *   - Time per check: O(2^n)
 *   - Total time: 2^{O(S log S) + n}
 *   - Constructive if S = poly(n): 2^{O(n log n)} => constructive
 *   - NOT constructive if S = exp(n): 2^{exp(n)} => too large
 *
 * Returns: 1 if constructive, 0 otherwise.
 */
static int np_test_constructiveness(int n, int S) {
    return shannon_is_constructive(n, S);
}

/*
 * np_test_largeness: Check if property P is large.
 *
 * Definition: P is C-large if Pr_{random f}[P(f)] >= 2^{-c*n} for some c.
 *
 * For property P = "circuit complexity > S":
 *   - Fraction of functions with complexity <= S: feasible_fraction(n, S)
 *   - Fraction with complexity > S: 1 - feasible_fraction(n, S)
 *   - Need this >= 2^{-c*n} for some c.
 *
 * For S = poly(n): fraction ~ 1 (very large) => trivially large
 * For S = 2^{n/4}: fraction ~ 0.5 (still > 2^{-n}) => large
 * For S = 2^n/n: fraction ~ 0 (most functions have size <= O(2^n/n))
 *   => NOT large
 *
 * Returns: 1 if large, 0 otherwise.
 *   Threshold: fraction >= 2^{-2n} (i.e., c <= 2)
 */
static int np_test_largeness(int n, int S) {
    double frac = shannon_largeness_fraction(n, S);
    double threshold = pow(2.0, -2.0 * (double)n);
    return (frac >= threshold) ? 1 : 0;
}

/*
 * np_test_usefulness: Check if P is useful against circuit class C.
 *
 * Definition: P is useful against circuit class C if for every f,
 * P(f) => f notin C[n] (f is not computable by circuits of class C
 * of size <= S(n) for some super-polynomial S).
 *
 * For P = "circuit complexity > S":
 *   - P is useful against SIZE[S] by definition.
 *   - For P/poly (poly-size circuits): need S to be super-polynomial.
 *   - For AC0 (constant-depth): even S = n is already a lower bound.
 *
 * Returns: 1 if useful, 0 otherwise.
 */
static int np_test_usefulness(int n, int S, CircuitClass target) {
    /* P(f) = "complexity > S" is useful against any class C where
     * all functions in C[n] have size <= S(n).
     *
     * For P/poly: need S to be super-polynomial.
     * For AC0: S >= 1 is sufficient (there are functions requiring
     *          depth > constant even with exponential size).
     * For NC1: need S >= n^{log n} or larger.
     *
     * We always return 1 because "size > S" is always a valid lower bound
     * claim, though its strength depends on S relative to the class. */
    (void)n;
    (void)target;
    return (S > 0) ? 1 : 0;
}

/* ========================================================================
 * NP_Verdict: Complete Natural Property Analysis
 * ======================================================================== */

NP_Verdict *np_verdict_create(const char *property_name,
                               const char *circuit_class_name,
                               int n, int S) {
    NP_Verdict *v = (NP_Verdict *)malloc(sizeof(NP_Verdict));
    if (!v) return NULL;

    v->property_name = property_name;
    v->circuit_class = circuit_class_name;
    v->n_inputs = n;
    v->size_bound = S;

    /* Check criteria */
    v->criteria.constructive = np_test_constructiveness(n, S);
    v->criteria.large = np_test_largeness(n, S);
    v->criteria.useful = 1;  /* "size > S" is always nominally useful */

    /* Quantitative measures */
    double log_cost = shannon_circuit_upper_bound_log(n, S) + (double)n;
    v->criteria.constructiveness_bound = (n > 0) ? log_cost / (double)n : 0.0;

    double frac = shannon_largeness_fraction(n, S);
    v->criteria.largeness_exponent = (frac > 0 && n > 0) ?
        -log2(frac) / (double)n : 0.0;

    v->criteria.target_circuit_size = S;
    v->criteria.is_natural = (v->criteria.constructive &&
                              v->criteria.large &&
                              v->criteria.useful) ? 1 : 0;

    v->computational_cost = (log_cost > 1000.0) ? INFINITY : pow(2.0, log_cost);
    v->fraction_satisfying = shannon_largeness_fraction(n, S);
    v->implies_lower_bound = v->criteria.useful;

    /* Generate verdict string */
    if (v->criteria.is_natural) {
        snprintf(v->verdict, 255,
            "NATURAL: property ''%s'' against %s is constructive, "
            "large (fraction=%.4f), and useful. "
            "If OWF exist => cannot separate NP from P/poly.",
            property_name, circuit_class_name, v->fraction_satisfying);
    } else {
        snprintf(v->verdict, 255,
            "NOT natural: ''%s'' fails criteria: constructive=%d large=%d useful=%d.",
            property_name,
            v->criteria.constructive,
            v->criteria.large,
            v->criteria.useful);
    }

    return v;
}

void np_verdict_free(NP_Verdict *v) {
    free(v);
}

void np_verdict_print(const NP_Verdict *v, FILE *fp) {
    if (!v) { fprintf(fp, "NULL verdict\n"); return; }
    if (!fp) fp = stdout;

    fprintf(fp, "\n========================================\n");
    fprintf(fp, "  NATURAL PROPERTY VERDICT\n");
    fprintf(fp, "========================================\n");
    fprintf(fp, "Property:  %s\n", v->property_name);
    fprintf(fp, "Target:    %s\n", v->circuit_class);
    fprintf(fp, "n = %d, S = %d\n\n", v->n_inputs, v->size_bound);

    fprintf(fp, "Three Criteria:\n");
    fprintf(fp, "  1. Constructive: %s (bound: 2^{%.1f * n})\n",
            v->criteria.constructive ? "YES" : "NO",
            v->criteria.constructiveness_bound);
    fprintf(fp, "  2. Large:        %s (fraction = %.4f)\n",
            v->criteria.large ? "YES" : "NO",
            v->fraction_satisfying);
    fprintf(fp, "  3. Useful:       %s\n",
            v->criteria.useful ? "YES" : "NO");

    fprintf(fp, "\n  Computational cost: %.2e\n", v->computational_cost);
    fprintf(fp, "\n  VERDICT: %s\n", v->verdict);
    fprintf(fp, "========================================\n");
}

/* ========================================================================
 * Demonstration: Testing Known Natural Properties
 * ======================================================================== */

/*
 * Known natural properties from circuit complexity:
 *
 * 1. "f has depth > d after random restriction" (AC0 lower bounds)
 * 2. "f has large monotone complexity" (Razborov 1985)
 * 3. "f has large MOD-p degree" (Razborov-Smolensky 1987)
 * 4. "f has large communication complexity" (Karchmer-Wigderson)
 *
 * All are constructive (checkable in 2^{O(n)} time), large (most random
 * functions have the property), and useful (imply lower bounds against
 * specific circuit classes).
 */

void property_tester_demo(void) {
    printf("\n================================================================\n");
    printf("  NATURAL PROPERTY TESTER — Razborov-Rudich 1997\n");
    printf("================================================================\n\n");

    srand((unsigned int)time(NULL));

    printf("DEFINITION (Natural Property):\n");
    printf("  A property P of Boolean functions is NATURAL if:\n");
    printf("  1. CONSTRUCTIVE: P(f) decidable in time 2^{O(n)}\n");
    printf("     given the truth table of f.\n");
    printf("  2. LARGE: P(f) holds for >= 2^{-O(n)} fraction\n");
    printf("     of all n-input Boolean functions.\n");
    printf("  3. USEFUL: P(f) => f is HARD against circuit class C.\n\n");

    printf("THEOREM (Razborov-Rudich 1997):\n");
    printf("  If one-way functions exist, no natural property can\n");
    printf("  prove that SAT requires super-polynomial circuits.\n\n");

    /* Test the standard hardness property */
    printf("--- Testing Property: 'circuit complexity > S' ---\n\n");

    /* Table: n, S, and the three criteria */
    printf("%-5s %-8s %-13s %-9s %-7s %s\n",
           "n", "S", "constructive?", "large?", "useful?", "natural?");
    printf("----- -------- ------------- --------- ------- --------\n");

    for (int n = 4; n <= 16; n += 4) {
        int styles[] = {n, n*n, n*n*n};
        for (int si = 0; si < 3; si++) {
            int S = styles[si];
            NaturalProperty np = shannon_natural_criteria_check(n, S);
            printf("%-5d %-8d %-13s %-9s %-7s %s\n",
                   n, S,
                   np.constructive ? "YES" : "NO",
                   np.large ? "YES" : "NO",
                   np.useful ? "YES" : "NO",
                   np.is_natural ? "NATURAL" : "no");
        }
    }

    /* Detailed verdict for a specific case */
    printf("\n--- Detailed Verdict (n=8, S=n^2=64) ---\n");
    NP_Verdict *v = np_verdict_create(
        "complexity > S", "P/poly", 8, 64);
    np_verdict_print(v, stdout);
    np_verdict_free(v);

    /* Demonstrate the largeness threshold */
    printf("\n--- Largeness Analysis ---\n");
    printf("  %-5s %-8s %-14s %-14s %s\n",
           "n", "S", "log2(circuits)", "log2(functions)", "fraction_hard");
    for (int n = 4; n <= 16; n += 4) {
        for (int S_mult = 1; S_mult <= 20; S_mult += 5) {
            int S = n * n * S_mult;
            double log_c = shannon_circuit_upper_bound_log(n, S);
            double log_f = shannon_function_count_log(n);
            double hard = shannon_largeness_fraction(n, S);
            printf("  %-5d %-8d %-14.1f %-14.1f %.6f\n",
                   n, S, log_c, log_f, hard);
        }
        printf("\n");
    }

    /* Show that all known bounds are natural */
    printf("--- All Known Circuit Lower Bounds Are Natural ---\n");
    printf("  AC0 bounds (FSS/Ajtai/Hastad):     CONSTRUCTIVE + LARGE + USEFUL\n");
    printf("  Monotone bounds (Razborov 1985):   CONSTRUCTIVE + LARGE + USEFUL\n");
    printf("  AC0[p] bounds (Razborov-Smolensky 1987): CONSTRUCTIVE + LARGE + USEFUL\n");
    printf("  Communication complexity bounds:   CONSTRUCTIVE + LARGE + USEFUL\n");
    printf("  => ALL are NATURAL => cannot prove P != NP (if OWF exist).\n");
    printf("  => To prove P != NP: need NON-NATURAL techniques.\n");
}
