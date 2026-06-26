/* razborov_demo.c -- Razborov's Theorem: Full Proof Walkthrough
 *
 * Demonstrates the key steps in Razborov's 1985 proof that CLIQUE
 * requires super-polynomial monotone circuits.
 *
 * The proof has 5 main steps:
 *   1. Replace each gate with an approximator (A+, A-)
 *   2. At AND gates: intersection (controlled by sunflower lemma)
 *   3. At OR gates: union (additive, no blowup)
 *   4. Output approximator must differ from CLIQUE
 *   5. If circuit is small, approximator stays small -> contradiction
 *
 * Run: make razborov_demo && ./razborov_demo
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "monotone.h"

int main(void) {
    printf("================================================================\n");
    printf("  RAZBOROV'S THEOREM (1985): A Walkthrough\n");
    printf("  Monotone Circuit Lower Bounds for CLIQUE\n");
    printf("================================================================\n\n");

    srand(42);

    /* Step 1: Set up the problem */
    printf("STEP 1: The Problem\n");
    printf("  We want to prove that CLIQUE(n,k) requires LARGE monotone\n");
    printf("  circuits. A monotone circuit uses only AND and OR gates.\n\n");

    int n = 10, k = 3;
    printf("  Example: n = %d vertices, k = %d (looking for triangles)\n\n", n, k);

    /* Step 2: Method of Approximations */
    printf("STEP 2: Method of Approximations\n");
    printf("  Replace each gate output with an 'approximator' (A+, A-):\n");
    printf("    A+ = set of positive examples (graphs with k-cliques)\n");
    printf("    A- = set of negative examples (graphs without k-cliques)\n\n");

    Approximator *a = approx_create(n, k, 100);
    printf("  Created empty approximator: n=%d, k=%d, max_size=%d\n",
           a->n, a->k, a->max_size);
    printf("  Initial: pos=%d, neg=%d\n\n", a->num_pos, a->num_neg);

    /* Step 3: Input approximators */
    printf("STEP 3: Input Approximators (for each edge variable)\n");
    printf("  For edge (u,v), the input approximator is:\n");
    printf("    A+: a specific k-clique containing edge (u,v)\n");
    printf("    A-: a (k-1)-coloring where u,v have the same color\n\n");

    for (int u = 0; u < n && u < 3; u++) {
        for (int v = u + 1; v < n && v < u + 2; v++) {
            Approximator *edge = approx_input_edge(n, k, u, v, 100);
            printf("  Edge(%d,%d): pos=%d neg=%d\n", u, v,
                   edge->num_pos, edge->num_neg);
            approx_free(edge);
        }
    }
    printf("  ... (all n(n-1)/2 edges)\n\n");

    /* Step 4: AND gate approximator */
    printf("STEP 4: AND Gate Approximator\n");
    printf("  At an AND gate, we intersect the approximators:\n");
    printf("    A_and+ = A1+ intersect A2+\n");
    printf("    A_and- = union of A1- and A2-\n\n");

    Approximator *e1 = approx_input_edge(n, k, 0, 1, 100);
    Approximator *e2 = approx_input_edge(n, k, 1, 2, 100);
    Approximator *e_and = approx_and_gate(e1, e2, 100);
    printf("  AND(edge(0,1), edge(1,2)):\n");
    printf("    Before: pos=%d+%d, neg=%d+%d\n",
           e1->num_pos, e2->num_pos, e1->num_neg, e2->num_neg);
    printf("    After:  pos=%d, neg=%d\n",
           e_and->num_pos, e_and->num_neg);
    printf("  Note: AND may blow up, but sunflower lemma bounds it.\n\n");

    /* Step 5: OR gate approximator */
    printf("STEP 5: OR Gate Approximator\n");
    printf("  At an OR gate, we union the approximators:\n");
    printf("    A_or+ = union of A1+ and A2+ (additive)\n");
    printf("    A_or- = A1- intersect A2-\n\n");

    Approximator *e_or = approx_or_gate(e1, e2, 100);
    printf("  OR(edge(0,1), edge(1,2)):\n");
    printf("    After:  pos=%d, neg=%d\n",
           e_or->num_pos, e_or->num_neg);
    printf("  Note: OR is additive, no blowup beyond summing.\n\n");

    /* Step 6: Sunflower Lemma */
    printf("STEP 6: Sunflower Lemma (Erdos-Rado 1960)\n");
    printf("  The key: any large family of k-sets contains a sunflower.\n\n");

    for (int p = 3; p <= 7; p += 2) {
        for (int test_k = 2; test_k <= 4; test_k++) {
            double bound = sunflower_bound_erdos_rado(p, test_k);
            printf("  p=%d k=%d: bound = %.0f sets\n", p, test_k, bound);
        }
    }
    printf("\n  If approximator exceeds this bound, we can compress it\n");
    printf("  by replacing p petals with the common core.\n\n");

    /* Step 7: Lower bound computation */
    printf("STEP 7: Lower Bound Computation\n");
    printf("  Using the approximator analysis:\n\n");

    printf("  n    k    Razborov LB      Alon-Boppana LB\n");
    printf("  ---  ---  ---------------  ---------------\n");
    for (int test_n = 8; test_n <= 64; test_n *= 2) {
        int test_k = (int)sqrt((double)test_n);
        double rb = razborov_clique_lower_bound(test_n, test_k);
        double ab = alon_boppana_clique_lower_bound(test_n, test_k);
        printf("  %-4d %-4d %-16.1e %-16.1e\n", test_n, test_k, rb, ab);
    }

    /* Step 8: Conclusion */
    printf("\nSTEP 8: Conclusion\n");
    printf("  For k = n^(1/4): the monotone circuit size must be\n");
    printf("  at least exp(Omega(n^(1/8))), which is SUPER-POLYNOMIAL.\n\n");
    printf("  For k = n^(1/2): size >= exp(Omega(n^(1/4))).\n\n");
    printf("  This was the FIRST super-polynomial circuit lower bound!\n");
    printf("  (But it only works for monotone circuits.)\n\n");

    printf("HISTORICAL NOTE:\n");
    printf("  Before 1985, no super-polynomial lower bounds were known\n");
    printf("  for ANY explicit Boolean function, even for monotone circuits.\n");
    printf("  Razborov's proof was a breakthrough that opened the field.\n\n");

    printf("OPEN PROBLEM (since 1985):\n");
    printf("  Prove that CLIQUE requires super-polynomial\n");
    printf("  GENERAL (non-monotone) circuits.\n\n");

    /* Cleanup */
    approx_free(a);
    approx_free(e1);
    approx_free(e2);
    approx_free(e_and);
    approx_free(e_or);

    printf("================================================================\n");
    printf("  END OF WALKTHROUGH\n");
    printf("================================================================\n");

    return 0;
}
