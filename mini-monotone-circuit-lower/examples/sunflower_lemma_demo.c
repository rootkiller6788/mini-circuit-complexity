/* sunflower_demo.c -- Sunflower Lemma: Interactive Demonstration
 *
 * Demonstrates the Erdos-Rado Sunflower Lemma (1960), the key
 * combinatorial tool used in Razborov's monotone circuit lower
 * bound proof.
 *
 * The sunflower lemma states: any family of more than (p-1)^k * k!
 * sets of size k contains a sunflower with p petals.
 *
 * Run: make sunflower_demo && ./sunflower_demo
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "monotone.h"

int main(void) {
    printf("================================================================\n");
    printf("  SUNFLOWER LEMMA (Erdos-Rado 1960)\n");
    printf("  The Key to Razborov's Lower Bound\n");
    printf("================================================================\n\n");

    srand(31415);

    /* Part 1: What is a sunflower? */
    printf("PART 1: Definition\n\n");
    printf("A SUNFLOWER with p petals is p sets S_1,...,S_p such that\n");
    printf("for all i != j:  S_i ? S_j = K  (the 'core')\n");
    printf("The petals S_i \\ K are pairwise disjoint.\n\n");

    printf("Example (p=3, k=3):\n");
    printf("  S_1 = {0, 1, 2}   (core={0,1}, petal={2})\n");
    printf("  S_2 = {0, 1, 3}   (core={0,1}, petal={3})\n");
    printf("  S_3 = {0, 1, 4}   (core={0,1}, petal={4})\n");
    printf("  Core K = {0, 1}\n\n");

    /* Verify with code */
    SetMask sets[3];
    sets[0] = (1ULL << 0) | (1ULL << 1) | (1ULL << 2);
    sets[1] = (1ULL << 0) | (1ULL << 1) | (1ULL << 3);
    sets[2] = (1ULL << 0) | (1ULL << 1) | (1ULL << 4);
    SetMask core;
    if (sunflower_check(sets, 3, 3, &core)) {
        printf("Verification: CORRECT - these 3 sets form a sunflower.\n");
        printf("  Core = {");
        int first = 1;
        for (int i = 0; i < 10; i++) {
            if (core & (1ULL << i)) {
                if (!first) printf(", ");
                printf("%d", i);
                first = 0;
            }
        }
        printf("}\n\n");
    }

    /* Part 2: The bound */
    printf("PART 2: The Erdos-Rado Bound\n\n");
    printf("Theorem: Any family of MORE than (p-1)^k * k! k-sets\n");
    printf("contains a sunflower with p petals.\n\n");

    printf("  p   k   Bound = (p-1)^k * k!\n");
    printf(" --- --- --------------------\n");
    for (int p = 3; p <= 9; p += 2) {
        for (int k_val = 2; k_val <= 6; k_val++) {
            double b = sunflower_bound_erdos_rado(p, k_val);
            printf("  %d   %d   %.0f\n", p, k_val, b);
        }
    }
    printf("\n");

    /* Part 3: Why is the bound tight? */
    printf("PART 3: Tightness of the Bound\n\n");
    printf("The bound is tight up to the k! factor.\n");
    printf("Consider the 'block construction':\n");
    printf("  Partition universe [n] into k blocks of size ~n/k.\n");
    printf("  Take ALL sets with exactly one element from each block.\n");
    printf("  This gives ~(n/k)^k sets, no sunflower for p > n/k.\n\n");

    for (int n = 6; n <= 12; n += 2) {
        int k_val = 3;
        KSetFamily *kf = sunflower_free_family_construct(n, k_val);
        printf("  n=%d, k=%d: sunflower-free family has %d sets\n",
               n, k_val, kf->num_sets);
        double bound = sunflower_bound_erdos_rado(n/k_val + 1, k_val);
        printf("    Bound for p=%d: %.0f (must exceed this to guarantee sunflower)\n",
               n/k_val + 1, bound);
        ksetfamily_free(kf);
    }
    printf("\n");

    /* Part 4: Sunflower search demo */
    printf("PART 4: Searching for Sunflowers\n\n");

    for (int trial = 0; trial < 3; trial++) {
        KSetFamily *kf = ksetfamily_create(12, 3, 32);
        srand(1000 + trial);

        /* Generate random 3-sets */
        int n_sets = 15 + trial * 5;
        for (int i = 0; i < n_sets; i++) {
            ksetfamily_add_random(kf);
        }

        printf("  Trial %d: %d random 3-sets on [12]\n", trial + 1, n_sets);

        Sunflower sf;
        if (sunflower_find_brute(kf, 3, &sf)) {
            printf("    Found sunflower! Core size = %d\n",
                   setmask_popcount(sf.core));
            sunflower_free(&sf);
        } else if (sunflower_find_greedy(kf, 3, &sf)) {
            printf("    Found sunflower (greedy)! Core size = %d\n",
                   setmask_popcount(sf.core));
            sunflower_free(&sf);
        } else {
            printf("    No sunflower found (family too small for guaranteed existence)\n");
        }

        double er_bound = sunflower_bound_erdos_rado(3, 3);
        printf("    Erdos-Rado bound for p=3,k=3: %.0f sets\n", er_bound);
        printf("    Family size: %d (%s bound)\n\n", n_sets,
               n_sets > er_bound ? "EXCEEDS" : "below");

        ksetfamily_free(kf);
    }

    /* Part 5: Explicit sunflower construction */
    printf("PART 5: Constructing Explicit Sunflowers\n\n");

    for (int p = 3; p <= 7; p += 2) {
        Sunflower *sf = sunflower_construct_example(p, 5);
        printf("  p=%d petals, k=5: core=", p);
        setmask_print(sf->core);
        int petal_sizes[8] = {0};
        for (int i = 0; i < p; i++) {
            petal_sizes[i] = setmask_popcount(sf->petals[i]);
        }
        printf(", petal sizes: ");
        for (int i = 0; i < p; i++) {
            printf("%d ", petal_sizes[i]);
        }
        printf("\n");
        sunflower_free(sf);
    }
    printf("\n");

    /* Part 6: Role in Razborov's proof */
    printf("PART 6: Why Sunflowers Matter for Circuit Lower Bounds\n\n");
    printf("In Razborov's method of approximations:\n");
    printf("  1. At an AND gate, we intersect two approximators.\n");
    printf("  2. Naive intersection: |A1+| * |A2+| sets (BLOWUP!).\n");
    printf("  3. If the intersecting family is too large, it contains\n");
    printf("     a sunflower (by the Erdos-Rado lemma).\n");
    printf("  4. We replace p petals with the core, reducing size.\n");
    printf("  5. Result: approximator stays manageable, even through\n");
    printf("     many AND gates.\n\n");
    printf("Without the sunflower lemma, the approximator would blow up\n");
    printf("exponentially and the method would fail!\n\n");

    /* Part 7: ALWZ improved bounds (2020) */
    printf("PART 7: Improved Bounds (ALWZ 2020)\n\n");
    printf("In 2020, Alweiss, Lovett, Wu, and Zhang proved:\n");
    printf("  Any family of > (log k)^k * (p log log k)^{O(k)} k-sets\n");
    printf("  contains a p-petal sunflower.\n\n");
    printf("This improved the dependence on p from exponential to\n");
    printf("polynomial! This was a major breakthrough that resolved\n");
    printf("a 60-year-old open problem.\n\n");

    printf("  p   k   Erdos-Rado      ALWZ (approx)\n");
    printf(" --- --- --------------- ---------------\n");
    for (int p = 3; p <= 9; p += 2) {
        for (int k_val = 2; k_val <= 5; k_val++) {
            double er = sunflower_bound_erdos_rado(p, k_val);
            double alwz = sunflower_bound_alwz(p, k_val);
            printf("  %d   %d   %-15.0f %-15.0f\n", p, k_val, er, alwz);
        }
    }

    printf("\n================================================================\n");
    printf("  END OF SUNFLOWER DEMONSTRATION\n");
    printf("================================================================\n");

    return 0;
}
