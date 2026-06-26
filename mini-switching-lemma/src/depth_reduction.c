/* depth_reduction.c -- Iterative Switching and Depth Reduction
 * ================================================================
 * Implements L5: Full iterative switching process reducing a
 * depth-d AC0 circuit to depth-2 via Hastad's switching lemma.
 *
 * ALGORITHM: Apply random restriction R_p with p = n^{-1/(d-1)}.
 * Bottom two layers merge via switching lemma. Circuit depth
 * decreases by 1. Repeat d-1 times. Result: depth-2 DNF/CNF.
 *
 * After d-1 rounds, ~1 variable survives. For PARITY on that
 * variable we need a small circuit. Working backward:
 * If original AC0 circuit for PARITY had size < exp(n^{1/(d-1)}),
 * the depth reduction would produce a DNF too small for PARITY.
 * Contradiction! Thus PARITY requires exp(n^{1/(d-1)}) size.
 *
 * References: Hastad (1986), Arora & Barak Ch.14.3, Beame (1994)
 */
#include "switching.h"
#include <math.h>

DepthReduction* depth_reduction_simulate(int n, int depth) {
    if (n <= 0 || depth < 2) return NULL;
    DepthReduction* dr = (DepthReduction*)malloc(sizeof(DepthReduction));
    if (!dr) return NULL;
    memset(dr, 0, sizeof(DepthReduction));
    dr->n_initial = n; dr->depth = depth; dr->n_rounds = depth - 1;
    dr->rounds = (SwitchRound*)malloc((size_t)(dr->n_rounds) * sizeof(SwitchRound));
    if (!dr->rounds) { free(dr); return NULL; }
    double p = pow((double)n, -1.0 / (double)(depth - 1));
    int cur_n = n;
    for (int r = 0; r < dr->n_rounds; r++) {
        dr->rounds[r].depth = depth - r;
        dr->rounds[r].n_vars = cur_n;
        dr->rounds[r].p_star = p;
        int surv = (int)((double)cur_n * p);
        if (surv < 1) surv = 1;
        dr->rounds[r].surviving_vars = surv;
        int s = (int)(log((double)cur_n) / log(2.0)) + 2;
        dr->rounds[r].width_bound_s = (double)s;
        dr->rounds[r].failure_prob = pow(5.0 * p * (double)cur_n, (double)s);
        cur_n = surv;
    }
    dr->final_vars = cur_n;
    dr->size_lower = pow(2.0, pow((double)n, 1.0/(depth-1)) / 10.0);
    if (dr->size_lower > 1e100) dr->size_lower = 1e100;
    dr->parity_hard = (dr->size_lower > (double)n * (double)n) ? 1 : 0;
    return dr;
}

void depth_reduction_print(const DepthReduction* dr) {
    if (!dr) return;
    printf("\n=== ITERATIVE DEPTH REDUCTION ===\n\n");
    printf("Initial: n=%d vars, depth=%d circuit\n", dr->n_initial, dr->depth);
    printf("Rounds: %d (d-1 switching steps)\n\n", dr->n_rounds);
    printf("%5s %8s %8s %10s %10s %10s\n",
           "Round", "Depth", "n_vars", "p_star", "surv", "Pr[fail]");
    printf("----- -------- -------- ---------- ---------- ----------\n");
    for (int r = 0; r < dr->n_rounds; r++) {
        printf("%5d %8d %8d %10.4f %10d %10.6f\n",
               r+1, dr->rounds[r].depth, dr->rounds[r].n_vars,
               dr->rounds[r].p_star, dr->rounds[r].surviving_vars,
               dr->rounds[r].failure_prob);
    }
    printf("\nAfter %d rounds: ~%d free variables remain.\n",
           dr->n_rounds, dr->final_vars);
    printf("Circuit depth reduced to 2 (single DNF or CNF).\n");
    if (dr->parity_hard) {
        printf("\nPARITY requires super-polynomial size for depth-%d AC0!\n", dr->depth);
        printf("Lower bound: size >= %.1e\n", dr->size_lower);
    }
}

void depth_reduction_free(DepthReduction* dr) {
    if (dr) { free(dr->rounds); free(dr); }
}

/* Study: how p varies with depth for fixed n */
void depth_reduction_parameter_study(int nmax, int dmax) {
    printf("\n=== DEPTH REDUCTION: Parameter Study ===\n\n");
    printf("p = n^{-1/(d-1)}. After d-1 rounds: n*p^{d-1} = 1.\n\n");
    int ns[] = {16, 64, 256, 1024, 4096, 16384};
    for (int d = 3; d <= dmax && d <= 5; d++) {
        printf("Depth d=%d:\n", d);
        printf("  %8s %8s %8s %12s\n", "n", "p_opt", "surv", "DNF_terms");
        for (int i = 0; i < 6; i++) {
            int n = ns[i]; if (n > nmax) continue;
            double p = pow((double)n, -1.0/(d-1));
            int surv = (int)((double)n * pow(p, (double)(d-1)));
            if (surv < 1) surv = 1;
            long long dnf = 1LL << (surv - 1);
            printf("  %8d %8.4f %8d %12lld\n", n, p, surv, dnf);
        }
    }
}

void depth_reduction_visualize(int n, int depth) {
    DepthReduction* dr = depth_reduction_simulate(n, depth);
    if (!dr) return;
    printf("\n=== DEPTH REDUCTION VISUALIZATION ===\n");
    printf("n=%d, d=%d\n\n", n, depth);
    int cur_n = n;
    for (int r = 0; r < dr->n_rounds; r++) {
        for (int i = 0; i < r; i++) printf("  ");
        printf("depth-%d circuit(%d vars)\n", depth - r, cur_n);
        for (int i = 0; i < r; i++) printf("  ");
        printf("  | R_p (p=%.4f)\n", dr->rounds[r].p_star);
        for (int i = 0; i < r; i++) printf("  ");
        printf("  v\n");
        cur_n = dr->rounds[r].surviving_vars;
    }
    for (int i = 0; i < dr->n_rounds; i++) printf("  ");
    printf("depth-2 DNF/CNF(%d vars)\n", cur_n);
    printf("\nPARITY needs 2^{%d-1} = %.0f terms as depth-2 DNF.\n",
           cur_n, pow(2.0, (double)(cur_n - 1)));
    depth_reduction_free(dr);
}

double hastad_parity_lower_bound(int n, int d) {
    if (d <= 1) return (double)n;
    double exp_term = pow((double)n, 1.0 / (double)(d - 1)) / 10.0;
    return pow(2.0, exp_term);
}

/* Show the Hastad lower bound for various n and d */
void hastad_lower_bound_table(int nmax) {
    printf("\n=== HASTAD LOWER BOUND: PARITY not in AC0 ===\n\n");
    printf("Size lower bound: exp(Omega(n^{1/(d-1)}))\n\n");
    printf("%6s", "n");
    for (int d = 2; d <= 5; d++) printf("  d=%d_size", d);
    printf("\n");
    printf("------");
    for (int d = 2; d <= 5; d++) printf("  ----------");
    printf("\n");
    for (int n = 16; n <= nmax; n *= 2) {
        printf("%6d", n);
        for (int d = 2; d <= 5; d++) {
            double lb = hastad_parity_lower_bound(n, d);
            if (lb > 1e50) printf("  >1e50");
            else printf("  %.1e", lb);
        }
        printf("\n");
    }
    printf("\nFor constant depth d, the size lower bound is super-polynomial.\n");
    printf("Therefore: PARITY requires exponential size in AC0.\n");
    printf("This proves: AC0 is a proper subclass of NC1. (FSS 1981, Ajtai 1983, Hastad 1986)\n");
}
