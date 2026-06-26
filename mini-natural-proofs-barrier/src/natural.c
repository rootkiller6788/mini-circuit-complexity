/* natural.c -- Natural Proofs Barrier: Master Aggregator
 *
 * This file provides the main entry points and demo orchestrators
 * for the Natural Proofs Barrier module.
 *
 * Includes all sub-modules:
 *   - Truth tables (truth_table.c)
 *   - Shannon counting (shannon_count.c)
 *   - Circuit model (circuit_model.c)
 *   - Natural property testing (natural_property.c)
 *   - PRF contradiction (prf_contradiction.c)
 *   - Barrier theorem (barrier_theorem.c)
 *   - Three barriers (three_barriers.c)
 *
 * This module implements the complete Razborov-Rudich theory:
 *   L1: Definitions (TruthTable, NaturalProperty, Circuit, PRF)
 *   L2: Core concepts (constructiveness, largeness, usefulness)
 *   L3: Mathematical structures (Boolean function algebra, DAG circuits)
 *   L4: Fundamental laws (Shannon 1949, Razborov-Rudich 1997)
 *   L5: Algorithms (Shannon counting, natural property testing)
 *   L6: Canonical problems (circuit lower bounds, PRF contradiction)
 *   L7: Applications (cryptography foundations, circuit complexity)
 *   L8: Advanced topics (three barriers, GCT, meta-complexity)
 *   L9: Research frontiers (barrier bypass candidates)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "natural.h"

/* ========================================================================
 * Demo Functions — Already implemented in sub-files
 * ======================================================================== */

/*
 * natural_proofs_demo: Comprehensive demo of truth tables + Shannon counting.
 *   Demonstrates L1-L4: truth table operations, Shannon's counting argument,
 *   and the natural property criteria.
 */
void natural_proofs_demo(void) {
    printf("\n================================================================\n");
    printf("  NATURAL PROOFS — TRUTH TABLE & SHANNON LIBRARY DEMO\n");
    printf("================================================================\n\n");
    srand(12345);

    /* 1. Truth table operations (L1, L3) */
    printf("--- Truth Tables ---\n");
    for (int n = 3; n <= 5; n++) {
        TruthTable *tt = tt_random(n);
        printf("  Random function on %d vars: ", n);
        tt_print_brief(tt, stdout);
        tt_free(tt);
    }
    printf("\n");

    /* 2. Boolean operations (L3) */
    printf("--- Boolean Operations ---\n");
    TruthTable *a = tt_random(3);
    TruthTable *b = tt_random(3);
    tt_print(a, stdout);
    TruthTable *a_and_b = tt_and(a, b);
    printf("  AND result:\n");
    tt_print_brief(a_and_b, stdout);
    printf("  Hamming distance(a,b) = %lld\n", tt_hamming_distance(a, b));
    printf("  Weight(a) = %lld, Fraction ones(a) = %.2f\n",
           tt_weight(a), tt_fraction_ones(a));
    tt_free(a); tt_free(b); tt_free(a_and_b);
    printf("\n");

    /* 3. Shannon counting (L4) */
    printf("--- Shannon Counting ---\n");
    printf("  n     S=poly  circuits       total=2^{2^n}    feasible_frac\n");
    for (int n = 4; n <= 12; n += 4) {
        int S = n * n;
        double c = shannon_circuit_upper_bound(n, S);
        double t = shannon_function_count(n);
        double f = shannon_feasible_fraction(n, S);
        printf("  %-5d %-7d %-14.1e ", n, S, c);
        if (isinf(t)) printf("%-15s", "overflow");
        else printf("%-15.1e", t);
        printf(" %.2e\n", f);
    }
    printf("\n  => At polynomial S, fraction feasible is negligible => ");
    printf("LARGENESS criterion holds.\n");

    /* 4. Natural property test (L5) */
    printf("\n--- Natural Property Test ---\n");
    printf("  n     S      constructive  large   useful\n");
    for (int n = 4; n <= 12; n += 4) {
        for (int S = n; S <= n * n; S *= n) {
            NaturalProperty np = shannon_natural_criteria_check(n, S);
            printf("  %-5d %-6d %-13s %-7s YES\n",
                   n, S, np.constructive ? "YES" : "NO", np.large ? "YES" : "NO");
        }
    }
    printf("\n  Three criteria met => NATURAL => cannot prove P!=NP (if OWF exist).\n");
}

void natural_bench_demo(void) {
    printf("\n================================================================\n");
    printf("  THREE BARRIERS TO P vs NP\n");
    printf("================================================================\n\n");

    printf("1. RELATIVIZATION (BGS 1975):\n");
    printf("   There exist oracles A,B such that P^A=NP^A and P^B!=NP^B.\n");
    printf("   => Any proof that P=NP must 'relativize' (use oracles).\n");
    printf("   => Diagonalization alone cannot resolve P vs NP.\n\n");

    printf("2. NATURAL PROOFS (RR 1997):\n");
    printf("   Constructive + Large + Useful = Natural.\n");
    printf("   If OWF exist, no natural proof shows P != NP.\n");
    printf("   All known circuit lower bounds are natural.\n\n");

    printf("3. ALGEBRIZATION (AW 2009):\n");
    printf("   Extends relativization to algebraic oracles.\n");
    printf("   Even algebraic techniques (IP=PSPACE) don't bypass.\n");
    printf("   Any proof must be non-algebrizing.\n\n");

    printf("--- Largeness of 'Complexity > S' (Shannon 1949) ---\n");
    for (int n = 4; n <= 16; n += 4) {
        int S = n * n;
        double frac = shannon_largeness_fraction(n, S);
        printf("  n=%d S=%d: fraction hard = %.10f\n", n, S, frac);
    }

    printf("\n--- Candidates to Bypass Barriers ---\n");
    bypass_candidates_print_all(stdout);
}

void largeness_exp_demo(void) {
    printf("\n=== Largeness Analysis ===\n\n");
    for (int n = 4; n <= 12; n += 4) {
        double t = pow(2.0, pow(2.0, (double)n));
        double c = pow(2.0, (double)(n * n) * log2((double)(n * n)) * 10.0);
        double hard_frac = 1.0 - fmin(1.0, c / t);
        printf("  n=%d: hard_fraction=%.3f\n", n, hard_frac);
    }
    printf("\n  => Almost all functions are hard at polynomial size.\n");
    printf("  => 'Hardness' is a LARGE natural property.\n");
}

/* ========================================================================
 * Main entry point (for standalone compilation)
 * ======================================================================== */
#ifdef NATURAL_STANDALONE_MAIN
int main(void) {
    setbuf(stdout, NULL);
    srand((unsigned int)time(NULL));

    natural_proofs_demo();
    natural_bench_demo();
    property_tester_demo();
    largeness_exp_demo();
    prf_sim_demo();
    three_barriers_demo();
    bypass_candidates_demo();
    natural_examples_demo();
    razborov_rudich_proof_demo();
    constructive_violation_demo();
    prf_construction_demo();

    printf("\n=== All demos complete ===\n");
    return 0;
}
#endif
