/* switching_core.c -- Switching Lemma: Probability Bounds and Experiments
 * ======================================================================
 * Implements L4-L5: Core switching lemma probability computation,
 * experimental validation, and statistical analysis.
 *
 * THEOREM (Hastad 1986): For k-DNF f and random restriction R_p:
 *   Pr_R_p[ f|_rho is not an s-CNF ] < (5pk)^s
 *
 * References: Hastad (1986), Beame (1994), Arora & Barak Ch.14
 */
#include "switching.h"
#include <time.h>

double switching_prob_bound(int k, double p, int s) {
    if (k <= 0 || p < 0.0 || s <= 0) return 1.0;
    double base = 5.0 * p * (double)k;
    if (base >= 1.0) return 1.0;
    return pow(base, (double)s);
}

double switching_prob_bound_refined(int k, double p, int s, double C) {
    if (k <= 0 || p < 0.0 || s <= 0 || C <= 0.0) return 1.0;
    double base = C * p * (double)k;
    if (base >= 1.0) return 1.0;
    return pow(base, (double)s);
}

double switching_max_p(int k, int s, double epsilon) {
    if (k <= 0 || s <= 0) return 0.0;
    return pow(epsilon, 1.0 / (double)s) / (5.0 * (double)k);
}

int switching_min_s(int k, double p, double epsilon) {
    if (k <= 0 || p < 0.0) return 1;
    double base = 5.0 * p * (double)k;
    if (base >= 1.0) return 1;
    if (epsilon <= 0.0) return 1;
    double s_exact = log(epsilon) / log(base);
    if (s_exact < 1.0) return 1;
    return (int)ceil(s_exact);
}

int switching_single_trial(int n_vars, int n_terms, int k, double p, int s) {
    DNF* d = dnf_random(n_vars, n_terms, k);
    if (!d) return 0;
    Restriction* r = restriction_create(n_vars);
    if (!r) { dnf_free(d); return 0; }
    restriction_random(r, p);
    DNF* rd = dnf_restrict(d, r);
    if (!rd) { restriction_free(r); dnf_free(d); return 0; }
    int ok = dnf_is_s_cnf(rd, s);
    dnf_free(rd); restriction_free(r); dnf_free(d);
    return ok;
}

SwitchStats* switching_experiment(int n_vars, int n_terms, int k,
                                   double p, int s, int trials) {
    SwitchStats* stats = (SwitchStats*)malloc(sizeof(SwitchStats));
    if (!stats) return NULL;
    memset(stats, 0, sizeof(SwitchStats));
    stats->k = k; stats->p = p; stats->s = s; stats->trials = trials;
    stats->theory_bound = switching_prob_bound(k, p, s);
    stats->successes = 0;
    for (int t = 0; t < trials; t++) {
        if (switching_single_trial(n_vars, n_terms, k, p, s)) stats->successes++;
    }
    stats->exp_success_rate = (trials > 0)
        ? (double)stats->successes / (double)trials : 0.0;
    return stats;
}

void switching_stats_print(const SwitchStats* stats) {
    if (!stats) return;
    printf("SwitchStats(k=%d, p=%.4f, s=%d):\n", stats->k, stats->p, stats->s);
    printf("  Theory bound:  Pr[fail] < %.6f\n", stats->theory_bound);
    printf("  Theory success: >= %.2f%%\n",
           100.0 * (1.0 - (stats->theory_bound < 1.0 ? stats->theory_bound : 1.0)));
    printf("  Experiment: %d/%d (%.1f%%)\n",
           stats->successes, stats->trials, stats->exp_success_rate * 100.0);
}

void switch_stats_free(SwitchStats* stats) { free(stats); }

void switching_lemma_demo(int n, int k, double p) {
    printf("\n================================================================\n");
    printf("  HASTAD SWITCHING LEMMA (1986) -- Godel Prize 1994\n");
    printf("================================================================\n\n");
    printf("THEOREM: k-DNF -> s-CNF after R_p. Pr[fail] < (5pk)^s\n\n");
    printf("Parameters: n=%d, k=%d, p=%.3f\n\n", n, k, p);
    int nt = k * 3;
    DNF* d = dnf_random(n, nt, k);
    if (!d) return;
    printf("--- 1. Random %d-DNF: %d vars, %d terms ---\n", k, n, nt);
    int* assn = (int*)malloc((size_t)n * sizeof(int));
    if (assn) {
        for (int i = 0; i < n; i++) assn[i] = rand() % 2;
        printf("Eval on random input: %d\n", dnf_evaluate(d, assn));
        free(assn);
    }
    printf("\n--- 2. Random Restriction (p=%.3f) ---\n", p);
    Restriction* rr = restriction_create(n);
    if (rr) {
        restriction_random(rr, p);
        int nz, no, ns; restriction_counts(rr, &nz, &no, &ns);
        printf("  Set to 0: %d, Set to 1: %d, Free(*): %d (expected ~%.0f)\n",
               nz, no, ns, p * n);
        DNF* rd = dnf_restrict(d, rr);
        if (rd) {
            printf("  Terms: %d -> %d, Width: %d -> %d\n",
                   d->n_terms, rd->n_terms, dnf_width(d), dnf_width(rd));
            dnf_free(rd);
        }
        restriction_free(rr);
    }
    printf("\n--- 3. Switching Bound ---\n");
    int s = (int)(log((double)n) / log(2.0)) + 2;
    double bound = switching_prob_bound(k, p, s);
    printf("  s = log2(n)+2 = %d\n", s);
    printf("  Pr[fail] < (5*%.3f*%d)^%d = %.6f\n", p, k, s, bound);
    printf("  Success probability >= %.2f%%\n", 100.0 * (1.0 - bound));
    printf("\n--- 4. Experimental (100 trials) ---\n");
    SwitchStats* ss = switching_experiment(n, nt, k, p, s, 100);
    if (ss) { switching_stats_print(ss); switch_stats_free(ss); }
    printf("\n--- 5. PARITY Lower Bound from Switching ---\n");
    for (int dep = 2; dep <= 5; dep++) {
        double exp_term = pow((double)n, 1.0/(dep-1)) / 10.0;
        double lb = pow(2.0, exp_term);
        if (lb > 1e100) lb = 1e100;
        printf("  d=%d: size >= 2^{%.1f} = %.1e\n", dep, exp_term, lb);
    }
    dnf_free(d);
}

void switch_bench_demo(void) {
    srand((unsigned)time(NULL));
    printf("\n================================================================\n");
    printf("  SWITCHING LEMMA: Experimental Validation\n");
    printf("================================================================\n\n");
    printf("%6s %3s %6s %3s %7s %8s %9s\n",
           "n", "k", "p", "s", "trials", "success%", "theory%");
    int cf_n[] = {8, 12, 16, 20, 24, 32, 48, 64};
    int cf_k[] = {2, 3, 3, 4, 4, 5, 5, 6};
    for (int ci = 0; ci < 8; ci++) {
        int n = cf_n[ci], k = cf_k[ci];
        double p = 1.0 / (10.0 * k);
        int s = (int)(log((double)n) / log(2.0)) + 2;
        SwitchStats* ss = switching_experiment(n, k * 3, k, p, s, 200);
        if (!ss) continue;
        printf("%6d %3d %6.3f %3d %7d %7.1f%% %8.1f%%\n",
               n, k, p, s, 200, ss->exp_success_rate * 100.0,
               (1.0 - ss->theory_bound) * 100.0);
        switch_stats_free(ss);
    }
    printf("\nWhen p*k << 1, switching succeeds with high probability.\n");
}
