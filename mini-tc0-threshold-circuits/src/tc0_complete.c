/* tc0_complete.c -- TC0-Complete Problem Constructions (L6)
 *
 * TC0-complete problems (under AC0 many-one reductions):
 *   - MAJORITY: depth-1 threshold circuit (trivial)
 *   - THRESHOLD(k): single THR gate
 *   - SORTING: depth-O(log^2 n) via sorting networks
 *   - MULTIPLICATION: depth-O(log n) via iterated addition
 *   - DIVISION: depth-O(log n) via Newton iteration (Beame-Cook-Hoover)
 *   - ITERATED ADDITION: depth-O(log n) via 3-to-2 reduction
 *   - COUNT: TC0-complete (equivalent to MAJORITY)
 *
 * Each TC0-complete problem has:
 *   - A natural TC0 circuit construction
 *   - Proof that it is in TC0 (circuit family)
 *   - Proof of TC0-hardness: every problem in TC0 reduces to it
 *
 * References:
 *   - Chandra, Stockmeyer, Vishkin (1984) "Constant depth reducibility"
 *   - Beame, Cook, Hoover (1986) "Log depth circuits for division"
 *   - Vollmer (1999), Ch. 4
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include "tc0.h"

/* ================================================================
 * MAJORITY CIRCUIT (depth 1, size n+1)
 *
 * MAJORITY(x_1,...,x_n) = 1 iff sum(x_i) > n/2
 *
 * Circuit: n INPUT gates + 1 MAJ gate connected to all inputs.
 * Depth: 1. Size: n+1.
 *
 * MAJORITY is THE canonical TC0-complete problem.
 * It is trivially in TC0, and every TC0 language reduces to it.
 * ================================================================ */

TC0Circuit *tc0_build_majority(int n) {
    if (n <= 0 || n > 256) return NULL;

    TC0Circuit *c = tc0_circuit_create(n + 2);
    snprintf(c->name, sizeof(c->name), "MAJORITY_%d", n);

    /* n input gates */
    for (int i = 0; i < n; i++)
        tc0_add_input(c);

    /* 1 MAJ gate */
    int maj_gate = tc0_add_gate(c, TC_MAJ);
    for (int i = 0; i < n; i++)
        tc0_add_wire(c, i, maj_gate);

    tc0_set_output(c, maj_gate);
    tc0_compute_depth(c);
    return c;
}

/* ================================================================
 * THRESHOLD(k) CIRCUIT (depth 1, size n+1)
 *
 * THRESHOLD(x; k) = 1 iff sum(x_i) >= k
 * ================================================================ */

TC0Circuit *tc0_build_threshold(int n, int k) {
    if (n <= 0 || n > 256 || k < 0 || k > n) return NULL;

    TC0Circuit *c = tc0_circuit_create(n + 2);
    snprintf(c->name, sizeof(c->name), "THRESHOLD_%d_%d", n, k);

    for (int i = 0; i < n; i++)
        tc0_add_input(c);

    int thr_gate = tc0_add_gate(c, TC_THR);
    tc0_set_threshold(c, thr_gate, k);
    for (int i = 0; i < n; i++)
        tc0_add_wire(c, i, thr_gate);

    tc0_set_output(c, thr_gate);
    tc0_compute_depth(c);
    return c;
}

/* ================================================================
 * PARITY in TC0? (L8: Advanced/Boundary Topics)
 *
 * PARITY(x) = sum(x_i) mod 2
 *
 * PARITY is in NC1 but believed NOT in TC0.
 * However, we can build a TC0 circuit using depth-2 threshold gates
 * with exponential size. The known construction:
 *   PARITY_n requires 2^{n^{o(1)}} gates in depth-2 TC0,
 *   or O(n) gates in depth O(log n).
 *
 * We implement a depth-O(n) construction using THR gates
 * (which is NOT in TC0 — depth must be constant — but serves
 * to demonstrate the boundary).
 * ================================================================ */

TC0Circuit *tc0_build_parity_threshold(int n) {
    if (n <= 0 || n > 64) return NULL;

    /* Construct PARITY using threshold gates.
     * PARITY(x) = THR_1(x) XOR THR_2(x) XOR ... modulo counting
     *
     * Actually: PARITY_n cannot be computed by a single THR gate
     * (not linearly separable). Known construction:
     *   PARITY_n = THR(1) XOR THR(2) XOR ...
     * But XOR is not a TC0 gate! Using only THR/MAJ:
     *   O(n) gates in depth 2.
     */

    /* Simple depth-2 construction:
     * For each odd k in {1,3,5,...,n}, create a THR(k) gate.
     * PARITY = OR over (THR(k) AND NOT THR(k+1)) for odd k.
     *
     * This is depth 3 (NOT + AND + OR) after threshold gates.
     * For a pure depth-2 TC0 circuit:
     * PARITY = NOT THR(0) AND (THR_1 XOR THR_3 ...) is not TC0.
     *
     * The Razborov-Smolensky result shows PARITY requires
     * exponential-size depth-2 threshold circuits.
     */

    TC0Circuit *c = tc0_circuit_create(10 * n);
    snprintf(c->name, sizeof(c->name), "PARITY_TC0_%d", n);

    /* Inputs */
    for (int i = 0; i < n; i++) tc0_add_input(c);

    /* Create THR(k) gates for k = 1,3,5,... */
    int *thr_gates = (int *)malloc((size_t)(n + 2) * sizeof(int));
    for (int k = 1; k <= n; k += 2) {
        thr_gates[k] = tc0_add_gate(c, TC_THR);
        tc0_set_threshold(c, thr_gates[k], k);
        for (int i = 0; i < n; i++)
            tc0_add_wire(c, i, thr_gates[k]);
    }

    /* OR-tree to combine: PARITY = OR across odd k
     * (This only approximates parity for monotone counting) */
    for (int k = 1; k <= n; k += 2)
        tc0_set_output(c, thr_gates[k]);

    free(thr_gates);
    tc0_compute_depth(c);
    return c;
}

/* ================================================================
 * TC0 CIRCUIT FAMILY MANAGEMENT
 * ================================================================ */

/* Create a circuit family description */
TC0CircuitFamily *tc0_family_create(const char *name, int is_tc0,
                                      int is_complete) {
    TC0CircuitFamily *fam = (TC0CircuitFamily *)malloc(sizeof(TC0CircuitFamily));
    if (!fam) return NULL;
    strncpy(fam->language_name, name ? name : "unnamed", 127);
    fam->language_name[127] = '\0';
    fam->is_tc0 = is_tc0;
    fam->is_tc0_complete = is_complete;
    fam->circuit_example = NULL;
    fam->max_depth_bound = -1;
    fam->size_exponent = 0.0;
    return fam;
}

void tc0_family_free(TC0CircuitFamily *fam) {
    if (!fam) return;
    if (fam->circuit_example) tc0_circuit_free(fam->circuit_example);
    free(fam);
}

/* Build a canonical circuit family for a TC0-complete problem */
TC0CircuitFamily *tc0_family_majority(void) {
    TC0CircuitFamily *fam = tc0_family_create("MAJORITY", 1, 1);
    if (!fam) return NULL;
    fam->max_depth_bound = 1;
    fam->size_exponent = 1.0;  /* size = O(n) */
    fam->circuit_example = tc0_build_majority(8);
    return fam;
}

TC0CircuitFamily *tc0_family_sorting(void) {
    TC0CircuitFamily *fam = tc0_family_create("SORTING", 1, 1);
    if (!fam) return NULL;
    fam->max_depth_bound = (int)(log2(8.0) * log2(8.0));  /* O(log^2 n) */
    fam->size_exponent = 2.0;  /* size = O(n^2) */
    fam->circuit_example = tc0_build_sorting_network(4, 1);
    return fam;
}

TC0CircuitFamily *tc0_family_multiplication(void) {
    TC0CircuitFamily *fam = tc0_family_create("MULTIPLICATION", 1, 1);
    if (!fam) return NULL;
    fam->max_depth_bound = (int)log2(8.0);  /* O(log n) */
    fam->size_exponent = 2.0;  /* size = O(n^2) */
    fam->circuit_example = tc0_build_multiplication(4);
    return fam;
}

/* ================================================================
 * VERIFICATION: check that a circuit correctly computes a function
 * ================================================================ */

/* Verify that a circuit computes a given truth table correctly.
 * Returns 1 if outputs match all 2^n inputs, 0 on first mismatch. */
int tc0_verify_circuit_truth_table(const TC0Circuit *c,
                                     const BoolFunc *f) {
    if (!c || !f || !f->truth_table) return 0;
    if (c->num_inputs != f->num_vars) return 0;

    int *input = (int *)malloc((size_t)f->num_vars * sizeof(int));
    for (int x = 0; x < f->table_size; x++) {
        for (int i = 0; i < f->num_vars; i++)
            input[i] = (x >> i) & 1;

        int circuit_out = tc0_evaluate_circuit(c, input);
        int expected = f->truth_table[x];

        if (circuit_out != expected) {
            free(input);
            return 0;
        }
    }
    free(input);
    return 1;
}

/* Verify majority circuit for given n by exhaustive check */
int tc0_verify_majority_circuit(int n) {
    TC0Circuit *c = tc0_build_majority(n);
    if (!c) return 0;

    int *input = (int *)malloc((size_t)n * sizeof(int));
    int correct = 1;

    for (int x = 0; x < (1 << n) && correct; x++) {
        int ones = 0;
        for (int i = 0; i < n; i++) {
            input[i] = (x >> i) & 1;
            ones += input[i];
        }
        int expected = (ones > n / 2) ? 1 : 0;
        int result = tc0_evaluate_circuit(c, input);
        if (result != expected) correct = 0;
    }

    free(input);
    tc0_circuit_free(c);
    return correct;
}

/* ================================================================
 * COMPLEXITY CLASS HIERARCHY
 * ================================================================ */

/* The Boolean circuit complexity hierarchy:
 *
 *  AC0 ⊆ ACC0 ⊆ TC0 ⊆ NC1 ⊆ AC1 ⊆ NC ⊆ P
 *
 * Known separations:
 *   AC0 ≠ TC0  (MAJORITY ∉ AC0, Furst-Saxe-Sipser/Hastad)
 *   ACC0 ≠ NC1 (Razborov-Smolensky: MODp not computable by ACC0)
 *   Gap: TC0 vs NC1 OPEN since 1989
 */

static const char *complexity_class_desc[] = {
    "AC0:  Constant-depth, unbounded fan-in AND/OR/NOT gates, poly size",
    "ACC0: AC0 + MODm gates (for any fixed m)",
    "TC0:  Constant-depth, poly-size with MAJ/THR gates",
    "NC1:  O(log n) depth, bounded fan-in, poly size",
    "NC:   Polylog depth, bounded fan-in, poly size",
    "P:    Polynomial-time Turing machines",
    NULL
};

void tc0_complexity_hierarchy(void) {
    printf("Boolean Circuit Complexity Hierarchy:\n");
    printf("======================================\n");
    for (int i = 0; complexity_class_desc[i]; i++)
        printf("  %s\n", complexity_class_desc[i]);

    printf("\nKnown Inclusions and Separations:\n");
    printf("  AC0 ⊂ ACC0 (Razborov: MAJORITY needs super-poly size in ACC0)\n");
    printf("  TC0 ⊆ NC1  (Barrington 1989: threshold gates in NC1)\n");
    printf("  NC1 ⊆ P    (All NC1 languages are in P)\n");
    printf("\nOPEN: TC0 = NC1?  (Since 1989)\n");
    printf("  Equivalent to: is PARITY in TC0?\n");
    printf("  Most complexity theorists believe TC0 ⊊ NC1.\n");
}

/* ================================================================
 * DEMONSTRATION
 * ================================================================ */

void tc0_completeness_demo(void) {
    printf("\n============================================================\n");
    printf("  TC0-COMPLETENESS DEMO\n");
    printf("============================================================\n\n");

    /* MAJORITY */
    printf("--- MAJORITY in TC0 (depth 1) ---\n");
    TC0Circuit *maj = tc0_build_majority(6);
    tc0_print_circuit(maj);

    int input[6] = {1,0,1,0,1,1};  /* 4 ones → majority */
    printf("  Input: 101011 → MAJ=%d (expected: 1)\n",
           tc0_evaluate_circuit(maj, input));

    /* Verify */
    printf("\n  Verifying MAJORITY_%d: ", 5);
    printf("%s\n", tc0_verify_majority_circuit(5) ? "PASS" : "FAIL");
    tc0_circuit_free(maj);

    /* THRESHOLD */
    printf("\n--- THRESHOLD(3) in TC0 ---\n");
    TC0Circuit *thr = tc0_build_threshold(5, 3);
    printf("  Input 11100 → THR(3)=%d (expected: 1)\n",
           tc0_evaluate_circuit(thr, input));
    tc0_circuit_free(thr);

    /* Complexity hierarchy */
    printf("\n");
    tc0_complexity_hierarchy();

    /* TC0-complete problems */
    printf("\n--- TC0-Complete Problems ---\n");
    const char *probs[] = {
        "MAJORITY:          depth 1, size O(n)",
        "THRESHOLD(k):      depth 1, size O(n)",
        "SORTING:           depth O(log^2 n), size O(n^2)",
        "MULTIPLICATION:    depth O(log n), size O(n^2)",
        "DIVISION:          depth O(log n), size O(n^2) (Beame-Cook-Hoover)",
        "ITERATED ADDITION: depth O(log n), size O(n^2)",
        "COUNT:             depth O(log n), size O(n log n)",
    };
    for (int i = 0; i < 7; i++)
        printf("  %s\n", probs[i]);

    printf("\nAll are equivalent under AC0 many-one reductions.\n");
}
