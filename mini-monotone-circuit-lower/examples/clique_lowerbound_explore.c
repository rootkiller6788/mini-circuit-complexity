/* clique_lowerbound_demo.c -- CLIQUE Monotone Lower Bound Exploration
 *
 * Explores the monotone circuit lower bound for the CLIQUE function.
 * Demonstrates the interplay between n (graph size), k (clique size),
 * and the resulting lower bounds from Razborov and Alon-Boppana.
 *
 * Also explores the gap between monotone and general circuits,
 * and the implications for the P vs NP problem.
 *
 * Run: make clique_lowerbound_demo && ./clique_lowerbound_demo
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "monotone.h"

/* Helper to classify bound type */
static const char* classify_growth(double bound, int n) {
    double log_bound = log(bound);
    double log_n = log((double)n);
    /* Polynomial: bound <= n^c for moderate c */
    if (log_bound <= 10 * log_n) return "POLYNOMIAL";
    /* Sub-exponential: between poly and exp */
    if (log_bound <= sqrt((double)n) * log_n) return "SUB-EXPONENTIAL";
    return "EXPONENTIAL";
}

int main(void) {
    printf("================================================================\n");
    printf("  CLIQUE MONOTONE CIRCUIT LOWER BOUND EXPLORATION\n");
    printf("================================================================\n\n");

    srand(777);

    /* Part 1: Fixed n, varying k */
    printf("PART 1: Monotone Lower Bound for Various (n,k)\n\n");
    printf("  n     k     Razborov LB    Alon-Boppana LB   Growth\n");
    printf(" ----  ----  --------------  --------------   ----------\n");

    int test_cases[][2] = {
        {8, 2}, {8, 3}, {8, 4},
        {16, 2}, {16, 3}, {16, 4}, {16, 5},
        {32, 3}, {32, 4}, {32, 5}, {32, 8},
        {64, 3}, {64, 4}, {64, 8}, {64, 16},
    };
    int num_cases = sizeof(test_cases) / sizeof(test_cases[0]);

    for (int i = 0; i < num_cases; i++) {
        int n = test_cases[i][0], k = test_cases[i][1];
        if (k > n) continue;
        double rb = razborov_clique_lower_bound(n, k);
        double ab = alon_boppana_clique_lower_bound(n, k);
        const char *growth = classify_growth(rb, n);
        printf("  %-5d %-5d %-15.1e %-15.1e %s\n", n, k, rb, ab, growth);
    }
    printf("\n");

    /* Part 2: The special case k = n^{1/4} */
    printf("PART 2: The Critical Regime: k = n^{1/4}\n\n");
    printf("Razborov's proof works for k <= n^{1/4}.\n");
    printf("For k = n^{1/4}, the lower bound is exp(Omega(n^{1/8})).\n\n");

    printf("  n     n^{1/4}  k     Razborov LB    Poly/Super-Poly?\n");
    printf(" ----  -------  ----  --------------  --------------\n");
    for (int n = 16; n <= 256; n *= 2) {
        int k_opt = (int)pow((double)n, 0.25);
        if (k_opt < 2) k_opt = 2;
        double rb = razborov_clique_lower_bound(n, k_opt);
        const char *type = (rb > pow((double)n, 10.0)) ?
                           "SUPER-POLY" : "POLY";
        printf("  %-5d %-8.1f %-5d %-15.1e %s\n",
               n, pow((double)n, 0.25), k_opt, rb, type);
    }
    printf("\n");

    /* Part 3: Graph construction and clique testing */
    printf("PART 3: Empirical Clique Detection vs Lower Bounds\n\n");

    for (int n = 5; n <= 9; n++) {
        int k = (int)(n / 2.0);
        if (k < 2) k = 2;

        /* Create a random graph and check for k-clique */
        Graph *g = graph_random(n, 0.5);
        int has_clique = graph_has_clique(g, k);
        int max_clique = graph_max_clique_size(g);

        printf("  G(%d, 0.5): max-clique=%d, %d-clique=%s\n",
               n, max_clique, k, has_clique ? "YES" : "NO");

        /* Generate Turan graph (no k-clique) */
        Graph *turan = graph_generate_clique_negative(n, k + 1);
        assert(!graph_has_clique(turan, k + 1));
        printf("    Turan T(%d,%d): edges=%d, %d-clique=%s\n",
               n, k, turan->edges, k + 1,
               graph_has_clique(turan, k + 1) ? "YES" : "NO");

        graph_free(g);
        graph_free(turan);
    }
    printf("\n");

    /* Part 4: The Monotone vs General Gap */
    printf("PART 4: Monotone vs General Circuit Gap\n\n");
    printf("Monotone circuits are STRICTLY WEAKER than general circuits.\n");
    printf("Tardos (1988) showed an EXPONENTIAL gap:\n\n");

    printf("  n     Tardos Gap (monotone size)   General UB\n");
    printf(" ---   ---------------------------   ----------\n");
    for (int n = 4; n <= 32; n *= 2) {
        double gap = tardos_gap_lower_bound(n);
        printf("  %-5d %-29.1e poly(%d)\n", n, gap, n);
    }
    printf("\n");

    /* Part 5: Monotone circuit construction */
    printf("PART 5: Building a Tiny Monotone Circuit for CLIQUE\n\n");
    printf("Constructing a monotone circuit for CLIQUE(4,3):\n\n");

    /* CLIQUE(4,3): x_{01} AND x_{02} AND x_{12}  OR  ... other triangles
     * In a 4-vertex graph, there are C(4,3)=4 possible triangles:
     * {0,1,2}, {0,1,3}, {0,2,3}, {1,2,3}
     * CLIQUE(4,3) = (x01 AND x02 AND x12) OR (x01 AND x03 AND x13)
     *               OR (x02 AND x03 AND x23) OR (x12 AND x13 AND x23) */

    /* We map edges to input variables:
     * x01->input 0, x02->input 1, x03->input 2,
     * x12->input 3, x13->input 4, x23->input 5 */
    int n_verts = 4;
    int n_edges = n_verts * (n_verts - 1) / 2;

    MonotoneCircuit *c = mono_circuit_create(n_edges);

    /* Create input gates for each edge */
    int edge_gates[4][4] = {{-1}};
    int gate_idx = 0;
    for (int i = 0; i < n_verts; i++) {
        for (int j = i + 1; j < n_verts; j++) {
            edge_gates[i][j] = mono_circuit_add_input(c, gate_idx++);
        }
    }

    /* Build AND for each triangle */
    int tri_gates[4];
    int tri_verts[4][3] = {{0,1,2}, {0,1,3}, {0,2,3}, {1,2,3}};

    for (int t = 0; t < 4; t++) {
        int a = tri_verts[t][0], b = tri_verts[t][1], cv = tri_verts[t][2];
        int e1 = edge_gates[a][b];
        int e2 = edge_gates[a][cv];
        int e3 = edge_gates[b][cv];
        int and12 = mono_circuit_add_and(c, e1, e2);
        tri_gates[t] = mono_circuit_add_and(c, and12, e3);
    }

    /* OR all triangles together */
    int or12 = mono_circuit_add_or(c, tri_gates[0], tri_gates[1]);
    int or34 = mono_circuit_add_or(c, tri_gates[2], tri_gates[3]);
    int output = mono_circuit_add_or(c, or12, or34);
    mono_circuit_set_output(c, output);

    mono_circuit_print_stats(c);
    printf("\n");

    /* Test the circuit */
    printf("Testing the CLIQUE(4,3) circuit:\n");
    printf("  Complete graph K4 (all edges = 1): ");
    int all_ones[20] = {1,1,1,1,1,1};
    printf("%s\n", mono_circuit_evaluate(c, all_ones) ? "CLIQUE" : "NO CLIQUE");

    printf("  Path P4 (edges: 0-1,1-2,2-3): ");
    int path[20] = {1,0,0,1,0,1};
    printf("%s\n", mono_circuit_evaluate(c, path) ? "CLIQUE" : "NO CLIQUE");

    printf("  No edges: ");
    int no_edges[20] = {0};
    printf("%s\n", mono_circuit_evaluate(c, no_edges) ? "CLIQUE" : "NO CLIQUE");

    /* Formula representation */
    char *formula = mono_circuit_to_formula(c);
    printf("\nFormula: %s\n", formula);
    free(formula);

    /* Lower bound for this circuit size */
    printf("\nLower bound for CLIQUE(4,3):\n");
    printf("  Razborov LB: %.1e\n", razborov_clique_lower_bound(n_verts, 3));
    printf("  Alon-Boppana LB: %.1e\n", alon_boppana_clique_lower_bound(n_verts, 3));
    printf("  Actual circuit size: %d\n", c->size);
    printf("  (Note: lower bound is asymptotic, n=4 is too small)\n");

    mono_circuit_free(c);
    printf("\n");

    /* Part 6: Perfect Matching bound */
    printf("PART 6: Perfect Matching Monotone Lower Bound\n\n");
    printf("PERFECT MATCHING is in P (Edmonds' O(n^3) algorithm),\n");
    printf("but its monotone circuit complexity is SUPER-POLYNOMIAL!\n\n");

    printf("  n     PM Monotone LB   General UB\n");
    printf(" ---   ---------------   ----------\n");
    for (int n = 4; n <= 32; n += 4) {
        double lb = razborov_pm_lower_bound(n);
        printf("  %-5d %-17.1e O(n^3)\n", n, lb);
    }
    printf("\n  This shows monotone circuits cannot exploit the\n");
    printf("  polynomial-time algorithm for matching.\n\n");

    /* Part 7: Implications */
    printf("PART 7: Implications and Open Problems\n\n");
    printf("The monotone lower bounds show:\n");
    printf("  1. Monotone circuits are provably WEAK for some functions.\n");
    printf("  2. The method of approximations is a powerful technique.\n");
    printf("  3. But it does NOT extend to general circuits (yet).\n\n");

    printf("The BIG open problem:\n");
    printf("  Prove that CLIQUE requires super-polynomial size for\n");
    printf("  GENERAL (non-monotone) Boolean circuits.\n");
    printf("  This would prove P != NP (since CLIQUE is NP-complete).\n\n");

    printf("Why this is hard:\n");
    printf("  - Natural Proofs barrier (Razborov-Rudich 1997):\n");
    printf("    Any 'natural' proof would break cryptography.\n");
    printf("  - The approximator method fails for general circuits\n");
    printf("    because NOT gates can 'hide' information.\n");
    printf("  - New techniques are needed.\n\n");

    printf("================================================================\n");
    printf("  END OF CLIQUE LOWER BOUND EXPLORATION\n");
    printf("================================================================\n");

    return 0;
}
