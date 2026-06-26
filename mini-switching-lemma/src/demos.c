/**
 * src/demos.c — Remaining Demonstration Functions
 * ==============================================
 *
 * Implements the demonstration functions declared in switching.h
 * that are not covered by switching_core.c or parity_lower.c.
 *
 * These provide end-to-end demonstrations of:
 *   - Decision tree construction and properties
 *   - DNF-to-CNF conversion (exact and via switching)
 *   - Random restriction experiments
 *   - AC0 circuit construction
 *   - Applications (PRG, learning, Fourier)
 *   - Advanced topics
 */

#include "switching.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * decision_tree_demo — Demonstrate decision trees for PARITY and MAJORITY.
 *
 * Shows the key insight: PARITY requires decision tree depth n,
 * while AC0 circuits can be simulated by shallow decision trees.
 * This contradiction proves PARITY ∉ AC0.
 */
void decision_tree_demo(void) {
    printf("\n");
    printf("══════════════════════════════════════════════════════\n");
    printf("  DECISION TREE DEMONSTRATION\n");
    printf("══════════════════════════════════════════════════════\n");
    printf("\n");

    /* PARITY decision trees */
    printf("PARITY Decision Trees:\n");
    printf("  %-3s  %-6s  %-6s  %-6s  %s\n",
           "n", "depth", "size", "leaves", "verified");
    for (int n = 1; n <= 5; n++) {
        DTNode* tree = dt_parity(n);
        if (!tree) continue;

        int d = dt_depth(tree);
        int sz = dt_size(tree);
        int lc = dt_leaf_count(tree);

        /* Verify on all inputs */
        int table_size = 1 << n;
        int correct = 1;
        int* assign = (int*)calloc((size_t)n, sizeof(int));
        if (assign) {
            for (int mask = 0; mask < table_size && correct; mask++) {
                for (int v = 0; v < n; v++) assign[v] = (mask >> v) & 1;
                int expected = 0;
                for (int v = 0; v < n; v++) expected ^= assign[v];
                if (dt_eval(tree, assign) != expected) correct = 0;
            }
            free(assign);
        }

        printf("  %-3d  %-6d  %-6d  %-6d  %s\n",
               n, d, sz, lc, correct ? "YES" : "NO");
        dt_free(tree);
    }
    printf("\n");

    /* MAJORITY decision tree */
    printf("MAJORITY Decision Tree:\n");
    printf("  n=3, depth=%d (optimal)\n", dt_depth(dt_majority3()));
    dt_free(dt_majority3());
    printf("\n");

    printf("KEY INSIGHT:\n");
    printf("  PARITY requires decision tree depth = n.\n");
    printf("  AC0 circuits → shallow decision trees (switching lemma).\n");
    printf("  Therefore: PARITY cannot be computed by AC0.\n");
    printf("\n");
}

/**
 * dnf_to_cnf_demo — Demonstrate DNF-to-CNF conversion.
 *
 * Shows both exact (exponential) and probabilistic (switching lemma)
 * approaches to converting DNF to CNF.
 */
void dnf_to_cnf_demo(void) {
    printf("\n");
    printf("══════════════════════════════════════════════════════\n");
    printf("  DNF → CNF CONVERSION\n");
    printf("══════════════════════════════════════════════════════\n");
    printf("\n");

    /* Create a small DNF */
    int n = 8, k = 4;
    DNF* d = dnf_random(n, 6, k);
    if (!d) return;

    printf("Original DNF:\n");
    dnf_print(d);
    printf("  Width (k) = %d\n\n", dnf_width(d));

    /* Exact conversion */
    printf("Exact DNF → CNF conversion:\n");
    CNF* exact = dnf_to_cnf_exact(d);
    if (exact) {
        printf("  Clauses: %d, max width: %d\n",
               exact->n_terms, cnf_width(exact));
        if (exact->n_terms > 20) {
            printf("  (too many clauses to display)\n");
        } else {
            cnf_print(exact);
        }
        cnf_free(exact);
    } else {
        printf("  (conversion too large — exponential blowup)\n");
    }
    printf("\n");

    /* Switching lemma approach */
    printf("Switching Lemma approach (p=0.25):\n");
    for (int trial = 0; trial < 3; trial++) {
        Restriction* r = restriction_create(n);
        if (!r) continue;
        restriction_random(r, 0.25);

        DNF* restricted = dnf_restrict(d, r);
        if (!restricted) { restriction_free(r); continue; }

        int w_before = dnf_width(d);
        int w_after = dnf_width(restricted);
        int terms_before = dnf_term_count(d);
        int terms_after = dnf_term_count(restricted);

        printf("  Trial %d: width %d→%d, terms %d→%d\n",
               trial + 1, w_before, w_after, terms_before, terms_after);

        for (int s = 1; s <= 5; s++) {
            if (dnf_is_s_cnf(restricted, s)) {
                printf("    Restricted DNF IS a %d-CNF! (width ≤ %d)\n", s, s);
                break;
            }
            if (s == 5) {
                printf("    Restricted DNF NOT an s-CNF for s≤5 (width=%d)\n", w_after);
            }
        }

        dnf_free(restricted);
        restriction_free(r);
    }
    dnf_free(d);
    printf("\n");
}

/**
 * restriction_exp_demo — Random restriction experiments.
 *
 * Shows how different star probabilities affect DNF width reduction.
 */
void restriction_exp_demo(void) {
    printf("\n");
    printf("══════════════════════════════════════════════════════\n");
    printf("  RANDOM RESTRICTION EXPERIMENTS\n");
    printf("══════════════════════════════════════════════════════\n");
    printf("\n");

    int n = 12;
    printf("Effect of p on DNF width (k=4, 8 terms):\n");
    printf("  %-6s  %-12s  %-12s  %-12s\n",
           "p", "width_before", "width_after", "reduction");
    printf("  ------------------------------------------\n");

    for (double p = 0.05; p <= 0.50; p += 0.05) {
        DNF* d = dnf_random(n, 8, 4);
        if (!d) continue;

        int w_before = dnf_width(d);

        Restriction* r = restriction_create(n);
        if (!r) { dnf_free(d); continue; }
        restriction_random(r, p);

        DNF* restricted = dnf_restrict(d, r);
        int w_after = restricted ? dnf_width(restricted) : 0;

        double reduction = (w_before > 0) ?
            1.0 - (double)w_after / (double)w_before : 0.0;

        printf("  %-6.2f  %-12d  %-12d  %-11.0f%%\n",
               p, w_before, w_after, reduction * 100.0);

        if (restricted) dnf_free(restricted);
        restriction_free(r);
        dnf_free(d);
    }
    printf("\n");

    /* Expected surviving variables */
    printf("Expected surviving variables (n=%d):\n", n);
    printf("  %-6s  %-12s\n", "p", "E[#free]");
    printf("  ----------------\n");
    for (double p = 0.05; p <= 0.50; p += 0.05) {
        printf("  %-6.2f  %-12.1f\n", p, p * (double)n);
    }
    printf("\n");
}

/**
 * ac0_circuit_demo — AC0 circuit model demonstration.
 *
 * Builds example circuits and verifies their correctness.
 */
void ac0_circuit_demo(void) {
    printf("\n");
    printf("══════════════════════════════════════════════════════\n");
    printf("  AC0 CIRCUIT MODEL DEMONSTRATION\n");
    printf("══════════════════════════════════════════════════════\n");
    printf("\n");

    /* Example 1: Simple depth-2 circuit */
    printf("1. Circuit: (x0 ∧ x1) ∨ (x2 ∧ x3)\n\n");

    Circuit* c = circuit_create(4);
    if (c) {
        CircuitNode* x0 = circuit_add_input(c, 0);
        CircuitNode* x1 = circuit_add_input(c, 1);
        CircuitNode* x2 = circuit_add_input(c, 2);
        CircuitNode* x3 = circuit_add_input(c, 3);

        CircuitNode* fa0[2] = {x0, x1};
        CircuitNode* fa1[2] = {x2, x3};
        CircuitNode* a0 = circuit_add_and(c, fa0, 2);
        CircuitNode* a1 = circuit_add_and(c, fa1, 2);

        CircuitNode* fo[2] = {a0, a1};
        CircuitNode* og = circuit_add_or(c, fo, 2);
        circuit_set_output(c, og);

        printf("  size=%d, depth=%d\n", circuit_size(c), circuit_depth(c));

        /* Verify */
        int correct = 1;
        int assign[4];
        for (int mask = 0; mask < 16; mask++) {
            assign[0] = (mask >> 0) & 1;
            assign[1] = (mask >> 1) & 1;
            assign[2] = (mask >> 2) & 1;
            assign[3] = (mask >> 3) & 1;
            int expected = (assign[0] && assign[1]) || (assign[2] && assign[3]);
            if (circuit_eval(c, assign) != expected) correct = 0;
        }
        printf("  Verification: %s\n\n", correct ? "PASS ✓" : "FAIL ✗");
        circuit_free(c);
    }

    /* Example 2: DNF parity circuit (depth 2) */
    printf("2. PARITY on n=3, depth-2 circuit:\n");

    Circuit* cp = circuit_parity_depth2(3);
    if (cp) {
        printf("  size=%d, depth=%d\n", circuit_size(cp), circuit_depth(cp));

        int correct = 1;
        int assign3[3];
        for (int mask = 0; mask < 8; mask++) {
            assign3[0] = (mask >> 0) & 1;
            assign3[1] = (mask >> 1) & 1;
            assign3[2] = (mask >> 2) & 1;
            int expected = assign3[0] ^ assign3[1] ^ assign3[2];
            if (circuit_eval(cp, assign3) != expected) correct = 0;
        }
        printf("  Verification: %s\n\n", correct ? "PASS ✓" : "FAIL ✗");
        circuit_free(cp);
    }

    /* Example 3: From DNF to circuit */
    printf("3. Circuit from random DNF (n=5, k=2):\n");
    DNF* d = dnf_random(5, 4, 2);
    if (d) {
        Circuit* cd = circuit_from_dnf(d);
        if (cd) {
            printf("  DNF: %d terms, circuit: %d gates\n",
                   dnf_term_count(d), circuit_size(cd));

            /* Verify circuit matches DNF */
            int correct = 1;
            int assign5[5];
            for (int mask = 0; mask < 32; mask++) {
                for (int v = 0; v < 5; v++) assign5[v] = (mask >> v) & 1;
                if (circuit_eval(cd, assign5) != dnf_evaluate(d, assign5)) correct = 0;
            }
            printf("  Verification: %s\n", correct ? "PASS ✓" : "FAIL ✗");
            circuit_free(cd);
        }
        dnf_free(d);
    }
    printf("\n");
}

/**
 * switching_applications_demo — Applications of the switching lemma.
 *
 * Covers PRG construction, Fourier analysis, and learning theory
 * applications that follow from the switching lemma.
 */
void switching_applications_demo(void) {
    printf("\n");
    printf("══════════════════════════════════════════════════════\n");
    printf("  SWITCHING LEMMA APPLICATIONS (L7)\n");
    printf("══════════════════════════════════════════════════════\n");
    printf("\n");

    printf("1. NISAN-WIGDERSON PSEUDORANDOM GENERATOR:\n");
    printf("   Hardness of PARITY for AC0 → PRG for AC0.\n");
    printf("   %-6s  %-12s  %-12s\n", "n", "hardness", "stretch");
    for (int n = 10; n <= 100; n *= 2) {
        double h = hastad_parity_lower_bound(n, 3);
        double s = nw_generator_stretch(h, n);
        printf("   %-6d  %-12.1e  %-12.4f\n", n, h, s);
    }
    printf("\n");

    printf("2. LINIAL-MANSOUR-NISAN FOURIER TAIL BOUND:\n");
    printf("   AC0 functions have exponentially decaying Fourier tails.\n");
    printf("   %-6s  %-6s  %-6s  %-12s\n", "d", "S", "L", "W_>L <");
    for (int d = 1; d <= 4; d++) {
        double tail = lmn_fourier_tail_bound(100.0, d, 5, 20);
        printf("   %-6d  %-6.0f  %-6d  %-12.6f\n", d, 100.0, 5, tail);
    }
    printf("\n");

    printf("3. NISAN'S PRG FOR AC0:\n");
    printf("   Seed length: O(log^{2d}(n/ε))\n");
    printf("   %-6s  %-12s\n", "d", "seed_len");
    for (int d = 1; d <= 4; d++) {
        double sl = nisan_prg_seed_length(100, d, 0.1);
        printf("   %-6d  %-12.1f\n", d, sl);
    }
    printf("\n");

    printf("KEY INSIGHT:\n");
    printf("  Switching lemma is not just for lower bounds.\n");
    printf("  It is a fundamental tool connecting circuit complexity\n");
    printf("  to pseudorandomness, learning theory, and Fourier analysis.\n");
    printf("\n");
}

/**
 * switching_advanced_demo — Advanced topics overview.
 *
 * Covers L8 topics: multi-switching, average-case hardness,
 * correlation bounds, and the natural proofs barrier.
 */
void switching_advanced_demo(void) {
    printf("\n");
    printf("══════════════════════════════════════════════════════\n");
    printf("  ADVANCED TOPICS (L8)\n");
    printf("══════════════════════════════════════════════════════\n");
    printf("\n");

    printf("1. MULTI-SWITCHING LEMMA:\n");
    printf("   Switch multiple adjacent layers simultaneously.\n");
    printf("   Multi(k=3, t=2, p=0.20, s=4) = %.6f\n",
           multi_switching_bound(3, 2, 0.20, 4));
    printf("   Multi(k=3, t=2, p=0.10, s=4) = %.6f\n",
           multi_switching_bound(3, 2, 0.10, 4));
    printf("   Used for: tighter constant-depth lower bounds.\n\n");

    printf("2. AVERAGE-CASE HARDNESS:\n");
    printf("   PARITY is hard for AC0 even on RANDOM inputs.\n");
    printf("   %-6s  %-14s\n", "ε", "min_size ≥");
    for (double eps = 0.05; eps <= 0.40; eps += 0.10) {
        double lb = average_case_parity_lower(20, 3, eps);
        printf("   %-6.2f  %-14.2e\n", eps, lb);
    }
    printf("\n");

    printf("3. CORRELATION BOUNDS:\n");
    printf("   AC0 cannot even APPROXIMATE modular counting.\n");
    printf("   %-6s  %-14s\n", "MOD_m", "correlation ≤");
    for (int m = 2; m <= 7; m++) {
        double corr = correlation_bound_modm_ac0(m, 20, 3, 100);
        printf("   MOD_%-3d  %-14.6f\n", m, corr);
    }
    printf("\n");

    printf("4. NATURAL PROOFS BARRIER (Razborov-Rudich 1997):\n");
    printf("   The switching lemma gives a 'natural proof' that\n");
    printf("   PARITY ∉ AC0. This proof is:\n");
    printf("     • Constructive: poly-time checkable\n");
    printf("     • Large: most functions are \"simple\"\n");
    printf("     • Useful: implies PARITY ∉ AC0\n");
    printf("\n");
    printf("   IMPLICATION: Natural proofs cannot prove P ≠ NP\n");
    printf("   under standard cryptographic assumptions.\n");
    printf("   This is a MAJOR BARRIER in complexity theory.\n");
    printf("\n");
}
