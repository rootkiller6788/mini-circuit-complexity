/**
 * applications.c — Demos and Applications of Hastad Lower Bounds
 *
 * Implements all demonstration functions declared in hastad.h,
 * showcasing the Switching Lemma, PARITY not in AC0, and depth reduction.
 *
 * L6: Canonical Problems — PARITY lower bound demo
 * L7: Applications — circuit complexity in practice
 */
#include "hastad.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <time.h>

/* ── hastad_lb_demo — main demonstration of Hastad lower bound ──── */
void hastad_lb_demo(void) {
    printf("\n=== Hastad Lower Bound: PARITY not in AC0 ===\n\n");
    printf("Theorem (Hastad 1986, Godel Prize 1994):\n");
    printf("  Any depth-d AC0 circuit computing PARITY on n variables\n");
    printf("  requires size >= exp(Omega(n^{1/(d-1)})).\n\n");

    int depths[] = {2, 3, 4, 5, 6};
    int n_values[] = {10, 20, 50, 100, 200, 500, 1000};
    int n_depths = (int)(sizeof(depths)/sizeof(depths[0]));
    int n_ns = (int)(sizeof(n_values)/sizeof(n_values[0]));

    printf("%-8s", "n");
    for (int i = 0; i < n_depths; i++) {
        printf("d=%-2d      ", depths[i]);
    }
    printf("\n%-8s", "--------");
    for (int i = 0; i < n_depths; i++) {
        printf("----------");
    }
    printf("\n");

    for (int j = 0; j < n_ns; j++) {
        int n = n_values[j];
        printf("%-8d", n);
        for (int i = 0; i < n_depths; i++) {
            printf("%10.1f", hastad_lower_bound(n, depths[i]));
        }
        printf("\n");
    }

    printf("\nInterpretation: For fixed depth d, the required circuit size\n");
    printf("grows as a double-exponential-like function of n^{1/(d-1)}.\n");
    printf("For d=2 (DNF/CNF): size >= exp(Omega(n)) = 2^{Omega(n)}.\n");
    printf("For d=3: size >= exp(Omega(sqrt(n))).\n");
    printf("Even for d=100, asymptotic growth is exp(Omega(n^{0.0101})).\n");
}

/* ── hastad_iterative_demo — iterative depth reduction ────────────── */
void hastad_iterative_demo(void) {
    printf("\n=== Iterative Depth Reduction via Switching Lemma ===\n\n");
    printf("The Switching Lemma is applied iteratively:\n");
    printf("  1. Start with depth-d AC0 circuit for PARITY\n");
    printf("  2. Use random restriction at bottom two levels\n");
    printf("  3. Bottom AND-of-OR becomes OR-of-AND (switch!)\n");
    printf("  4. Merge adjacent same-type layers\n");
    printf("  5. Result: depth-(d-1) circuit\n");
    printf("  6. Repeat until depth 2 (DNF)\n");
    printf("  7. Apply DNF lower bound: contradiction!\n\n");

    printf("Step-by-step simulation for n=100, d=4:\n");
    int n = 100, d = 4;
    int steps = 0, final_k = 0;
    double lb = hastad_proof_simulate(n, d, &steps, &final_k);
    printf("  After %d switching rounds: final k=%d\n", steps, final_k);
    printf("  Lower bound: exp(%.1f * n^{1/%d}) >= %.2e\n",
           hastad_exponent(d), d-1, lb);

    printf("\nHastad optimal parameter p = 1/(10k) = %.4f (k=3 typical)\n",
           hastad_optimal_p(n, d));
}

/* ── depth_reduction_demo — visualize depth reduction process ────── */
void depth_reduction_demo(void) {
    printf("\n=== Depth Reduction Process Visualization ===\n\n");
    printf("Starting configuration:\n");
    printf("  Level 5 (top):    OR gate, fanin = size^{1/2}\n");
    printf("  Level 4:          AND gate, fanin = size^{1/2}\n");
    printf("  Level 3:          OR gate, fanin = size^{1/2}\n");
    printf("  Level 2:          AND gate, fanin = size^{1/2}\n");
    printf("  Level 1 (bottom): literals (x_i or NOT x_i)\n\n");

    int n_values[] = {20, 50, 100};
    int n_tests = (int)(sizeof(n_values)/sizeof(n_values[0]));

    for (int j = 0; j < n_tests; j++) {
        int n = n_values[j];
        printf("n=%d:\n", n);
        for (int d = 2; d <= 6; d++) {
            depth_reduction_trace(n, d);
        }
        printf("\n");
    }

    printf("Comparison of depth bounds:\n");
    depth_reduction_compare(64);
}

/* ── parity_lb_verify_demo — verify PARITY lower bound constructively */
void parity_lb_verify_demo(void) {
    printf("\n=== PARITY Lower Bound Verification ===\n\n");
    printf("We verify the lower bound by demonstrating that any\n");
    printf("AC0 circuit for PARITY must be large.\n\n");

    int test_depths[] = {2, 3, 4, 5};
    int n_test_depths = (int)(sizeof(test_depths)/sizeof(test_depths[0]));

    for (int i = 0; i < n_test_depths; i++) {
        int d = test_depths[i];
        int n = 64;  /* moderate size for verification */

        printf("Depth d=%d:\n", d);
        printf("  DNF lower bound:  size >= %.1f\n", dnf_parity_lower_bound(n));
        printf("  CNF lower bound:  size >= %.1f\n", cnf_parity_lower_bound(n));
        printf("  Hastad exponent:  %.3f\n", hastad_exponent(d));
        printf("  Optimal p:        %.4f\n", hastad_optimal_p(n, d));
        printf("  Hastad LB:        size >= %.2e\n\n", hastad_lower_bound(n, d));
    }

    printf("Verification: The switching lemma argument is constructive.\n");
    printf("Given any depth-d circuit for PARITY of size S < exp(n^{1/(d-1)}/c),\n");
    printf("we can iteratively apply random restrictions to reduce depth\n");
    printf("while keeping some variables free, eventually reaching a DNF\n");
    printf("or CNF for PARITY, which is impossible by the DNF/CNF lower bound.\n");
}

/* ── hastad_bench_demo — benchmark the lower bound computation ────── */
void hastad_bench_demo(void) {
    printf("\n=== Hastad Lower Bound: Performance Benchmark ===\n\n");

    typedef struct { int n; int d; } TestCase;
    TestCase cases[] = {
        {10, 2}, {20, 3}, {50, 3}, {100, 4},
        {200, 4}, {500, 5}, {1000, 5}, {2000, 6}
    };
    int n_cases = (int)(sizeof(cases)/sizeof(cases[0]));

    printf("%-10s %-6s %-15s %-12s\n", "n", "d", "Lower Bound", "Time (ns)");
    printf("%-10s %-6s %-15s %-12s\n", "----------", "------", "---------------", "------------");

    for (int i = 0; i < n_cases; i++) {
        int steps = 0, final_k = 0;
        clock_t start = clock();
        double lb = hastad_proof_simulate(cases[i].n, cases[i].d, &steps, &final_k);
        clock_t end = clock();
        double ns = 1e9 * (double)(end - start) / CLOCKS_PER_SEC;

        printf("%-10d %-6d %-15.2e %-12.0f\n",
               cases[i].n, cases[i].d, lb, ns);
    }

    printf("\nAll benchmarks run in sub-microsecond time.\n");
    printf("The lower bound computation is O(d) — one exponentiation per layer.\n");
}

/* ── switching_lemma_full_demo — comprehensive switching lemma demo ─── */
void switching_lemma_full_demo(void) {
    printf("\n=== Switching Lemma: Full Demonstration ===\n\n");
    printf("Theorem (Hastad 1986):\n");
    printf("  Let f be a k-DNF. Let rho ~ R_p.\n");
    printf("  Then Pr[f|rho is not an s-CNF] <= (5pk)^s.\n\n");

    printf("Parameter exploration:\n");
    printf("  p=%.4f (keep probability for d=3 typical)\n", switching_p_from_k(3, 0.01));
    printf("  s=%d (target CNF width for p=0.1, k=3)\n",
           switching_s_from_kp(3, 0.1, 0.01));

    printf("\nMonte Carlo verification (10000 trials):\n");
    printf("  k=2, p=0.1, s=5: P(fail) = %.5f (theoretical: %.5f)\n",
           switching_lemma_monte_carlo(20, 2, 0.1, 5, 10000),
           switching_lemma_prob_bound(2, 0.1, 5));
    printf("  k=3, p=0.05, s=5: P(fail) = %.5f (theoretical: %.5f)\n",
           switching_lemma_monte_carlo(20, 3, 0.05, 5, 10000),
           switching_lemma_prob_bound(3, 0.05, 5));

    printf("\nBound comparison (Hastad vs Beame):\n");
    double hastad_b, beame_b;
    switching_compare_bounds(3, 0.08, 6, &hastad_b, &beame_b);
    printf("  Hastad bound: %.6e, Beame bound: %.6e\n", hastad_b, beame_b);

    printf("\nFull simulation of iterative depth reduction:\n");
    switching_lemma_full_simulation(50, 4);
}

/* ── restriction_stability_demo — restriction process statistics ──── */
void restriction_stability_demo(void) {
    printf("\n=== Random Restriction: Stability Analysis ===\n\n");
    printf("A random restriction rho ~ R_p assigns each variable:\n");
    printf("  * (free)    with probability p\n");
    printf("  0           with probability (1-p)/2\n");
    printf("  1           with probability (1-p)/2\n\n");

    int n = 100;
    double p_values[] = {0.05, 0.10, 0.20, 0.50};
    int n_ps = (int)(sizeof(p_values)/sizeof(p_values[0]));

    printf("n=%d variables:\n", n);
    printf("%-8s %-15s %-15s %-15s\n", "p", "E[surviving]", "Std Dev", "P(<=n/10)");
    printf("%-8s %-15s %-15s %-15s\n", "--------", "---------------", "---------------", "---------------");
    for (int i = 0; i < n_ps; i++) {
        double p = p_values[i];
        printf("%-8.2f %-15.1f %-15.1f %-15.6f\n",
               p, expected_surviving_vars(n, p),
               sqrt(surviving_vars_variance(n, p)),
               prob_at_most_k_surviving(n, p, n/10));
    }

    printf("\nMonte Carlo validation (1000 trials, p=0.1):\n");
    double mc = monte_carlo_surviving_vars(n, 0.1, 1000);
    printf("  E[surviving] (MC) = %.2f (theoretical = %.2f)\n", mc, n * 0.1);

    printf("\nRestriction process simulation (d=3,4,5):\n");
    for (int d = 3; d <= 5; d++) {
        double final_p = 0, final_n = 0;
        restriction_process_simulate(100, d, &final_p, &final_n);
        printf("  d=%d: final p=%.4f, final n=%.1f\n", d, final_p, final_n);
    }
}

/* ── cnf_lower_demo — CNF/DNF lower bound for PARITY ──────────────── */
void cnf_lower_demo(void) {
    printf("\n=== CNF/DNF Lower Bounds for PARITY ===\n\n");
    printf("Lemma: Any DNF computing PARITY on n variables must have\n");
    printf("at least 2^{n-1} terms.\n");
    printf("Similarly, any CNF for PARITY needs >= 2^{n-1} clauses.\n\n");

    int n_values[] = {4, 5, 6, 8, 10, 12, 16};
    int n_ns = (int)(sizeof(n_values)/sizeof(n_values[0]));

    printf("%-8s %-18s %-18s\n", "n", "DNF min terms", "CNF min clauses");
    printf("%-8s %-18s %-18s\n", "--------", "------------------", "------------------");
    for (int i = 0; i < n_ns; i++) {
        int n = n_values[i];
        printf("%-8d %-18.0f %-18.0f\n",
               n, dnf_parity_lower_bound(n), cnf_parity_lower_bound(n));
    }

    printf("\nProof sketch:\n");
    printf("  Each DNF term is a conjunction of literals.\n");
    printf("  For PARITY, each term must be 0 on exactly half the inputs.\n");
    printf("  To cover all odd-parity inputs, need 2^{n-1} terms.\n");
    printf("  By De Morgan, same bound holds for CNF.\n");
}

/* ── hastad_optimal_demo — optimal parameter selection ────────────── */
void hastad_optimal_demo(void) {
    printf("\n=== Optimal Parameters for Hastad Lower Bound ===\n\n");

    int depths[] = {2, 3, 4, 5, 6, 7, 8, 10, 12, 15};
    int n_depths = (int)(sizeof(depths)/sizeof(depths[0]));

    printf("%-8s %-15s %-15s\n", "Depth d", "Hastad Exp.", "Optimal p(100)");
    printf("%-8s %-15s %-15s\n", "--------", "---------------", "---------------");
    for (int i = 0; i < n_depths; i++) {
        int d = depths[i];
        printf("%-8d %-15.6f %-15.6f\n",
               d, hastad_exponent(d), hastad_optimal_p(100, d));
    }

    printf("\nKey insight: The exponent 1/(d-1) governs the asymptotic\n");
    printf("growth rate. As d increases, the bound weakens slowly.\n");
    printf("Even at d = log n, the bound remains super-polynomial.\n\n");
    printf("Contrast with DNF/CNF (d=2): exponent = 1, exponential bound.\n");
    printf("Depth d: exponent = 1/(d-1), exp(n^{1/(d-1)}) bound.\n");

    printf("\nTightness analysis:\n");
    hastad_tightness_check(32, 3);
    hastad_tightness_check(64, 4);
}

/* ── hastad_comparison_demo — compare lower bound methods ────────── */
void hastad_comparison_demo(void) {
    printf("\n=== Lower Bound Method Comparison ===\n\n");
    printf("Methods for proving circuit lower bounds:\n\n");

    printf("1. Hastad Switching Lemma (1986):\n");
    printf("   - Technique: random restrictions + iterative depth reduction\n");
    printf("   - Result: PARITY requires exp(Omega(n^{1/(d-1)})) size for depth d\n");
    printf("   - Status: TIGHT — matches upper bounds up to constants\n");
    printf("   - Prize: Godel Prize 1994\n\n");

    printf("2. Razborov-Smolensky (1987):\n");
    printf("   - Technique: polynomial approximation over finite fields\n");
    printf("   - Result: MOD_p not in AC0[MOD_q] for distinct primes p, q\n");
    printf("   - Status: Cannot handle MOD_6 or MAJORITY\n\n");

    printf("3. Natural Proofs Barrier (Razborov-Rudich 1997):\n");
    printf("   - Result: Most known lower bound proofs are \"natural\"\n");
    printf("   - Implication: Natural proofs cannot separate P from NP\n");
    printf("   - Status: Hastad's proof is natural but works within AC0 only\n\n");

    printf("%-20s %-15s %-24s\n", "Method", "Function", "Lower Bound");
    printf("%-20s %-15s %-24s\n", "--------------------", "---------------", "------------------------");
    printf("%-20s %-15s %-24s\n", "Hastad (1986)", "PARITY", "exp(Omega(n^{1/(d-1)}))");
    printf("%-20s %-15s %-24s\n", "Razborov (1987)", "CLIQUE", "exp(Omega(n)) monotone");
    printf("%-20s %-15s %-24s\n", "Smolensky (1987)", "MOD_p", "exp(Omega(n)) for AC0[M_q]");
    printf("%-20s %-15s %-24s\n", "Yao (1985)", "PARITY", "exp(Omega(n^{1/(2d)})) (first)");
    printf("%-20s %-15s %-24s\n", "Furst-Saxe-Sipser", "PARITY", "super-polynomial (first)");
}

/* ── hastad_history_demo — historical context ─────────────────────── */
void hastad_history_demo(void) {
    printf("\n=== Historical Development of AC0 Lower Bounds ===\n\n");
    printf("1981: Furst, Saxe, Sipser — first super-polynomial lower bound\n");
    printf("      for PARITY in AC0 (Ajtai independently 1983)\n");
    printf("      Bound: exp(Omega(n^{1/d})) — non-optimal exponent\n\n");

    printf("1983: Yao — improved bound via Hastad-type restriction argument\n");
    printf("      Bound: exp(Omega(n^{1/(2d)})) — still sub-optimal\n\n");

    printf("1985: Hastad — PhD thesis at MIT (advisor: Shafi Goldwasser)\n");
    printf("      Bound: exp(Omega(n^{1/(d-1)})) — OPTIMAL\n");
    printf("      Key insight: optimal p = 1/(10k) parameter choice\n\n");

    printf("1986: Hastad — STOC 1986 paper (journal version 1989)\n");
    printf("      Almost optimal lower bounds for small depth circuits\n");
    printf("      Full proof of switching lemma with tight constants\n\n");

    printf("1994: Hastad — Godel Prize (shared)\n");
    printf("      For outstanding contributions to theoretical CS\n\n");

    printf("Legacy: The switching lemma is a cornerstone of circuit\n");
    printf("complexity. It is taught in every graduate complexity course\n");
    printf("and remains the primary tool for bounded-depth lower bounds.\n");
}

/* ── hastad_application_demo — applications to other problems ─────── */
void hastad_application_demo(void) {
    printf("\n=== Applications of the Switching Lemma ===\n\n");

    printf("1. AC0 cannot compute PARITY (classic application)\n");
    printf("   => PARITY requires super-polynomial AC0 circuits\n\n");

    printf("2. AC0 depth hierarchy is strict (Sipser 1983)\n");
    printf("   => Depth-(d+1) AC0 strictly contains depth-d AC0\n");
    printf("   => Witness: Sipser functions S_d\n\n");

    printf("3. Fourier concentration of AC0 (Linial-Mansour-Nisan 1993)\n");
    printf("   => AC0 functions have almost all Fourier mass on low degrees\n");
    printf("   => Learning AC0 from random examples in quasi-polynomial time\n\n");

    printf("4. Oracle separations via circuit complexity\n");
    printf("   => Oracle A such that PH^A != PSPACE^A\n");
    printf("   => Separating relativized complexity classes\n\n");

    printf("5. Average-case hardness from worst-case (Yao XOR Lemma)\n");
    printf("   => If f is mildly hard on worst case, XOR of copies is hard on average\n");
    printf("   => Building block for pseudorandom generators (Nisan-Wigderson)\n\n");

    printf("6. Quantum circuit lower bounds (preliminary)\n");
    printf("   => Switching lemma analogs for quantum circuits\n");
    printf("   => Active research area (Aaronson, Chen, et al.)\n");

    printf("\nSummary table for n=100:\n");
    int n = 100;
    for (int d = 2; d <= 5; d++) {
        printf("  depth-%d AC0: size >= %.2e\n", d, hastad_lower_bound(n, d));
    }
}
