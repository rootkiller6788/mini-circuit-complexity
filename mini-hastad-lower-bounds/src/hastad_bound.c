/**
 * hastad_bound.c — Core Hastad Lower Bound Verification
 *
 * Provides additional verification and analysis functions for
 * the Hastad Switching Lemma and its circuit lower bound consequences.
 *
 * Implements non-trivial lower-bound computations that complement
 * the depth_reduction.c and switching_lemma.c implementations.
 *
 * L4: Fundamental Laws — Hastad lower bound verification
 * L5: Algorithms — Lower bound computation, counter construction
 * L8: Advanced — Tightness analysis, optimal parameter regions
 */
#include "hastad.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

/* ── Constructive lower bound: build witness for given parameters ─── */
/**
 * Given n variables and depth d, compute the minimal circuit size
 * needed for PARITY by iterating the switching lemma bounds.
 *
 * This is distinct from hastad_lower_bound() — it verifies the
 * bound by simulating the full iterative restriction process
 * and checking that the contradiction holds at each step.
 */
void hastad_lower_bound_verify(int n, int d) {
    printf("Verifying Hastad LB for n=%d, depth=%d:\n", n, d);

    /* Step 1: The base case — DNF/CNF lower bound */
    double dnf_lb = dnf_parity_lower_bound(n);
    printf("  [DNF base] Any DNF for PARITY needs >= %.0f terms\n", dnf_lb);

    /* Step 2: Show that if depth-d circuit is too small, switching lemma
     *        reduces it to a small DNF, contradicting the base case */
    double fastad_lb = hastad_lower_bound(n, d);
    printf("  [Hastad] Depth-%d AC0 for PARITY needs size >= %.2e\n", d, fastad_lb);

    /* Step 3: Check that the iterative reduction is sound */
    int steps = 0, final_k = 0;
    double sim_lb = hastad_proof_simulate(n, d, &steps, &final_k);
    printf("  [Simulation] After %d rounds, final k=%d, LB=%.2e\n",
           steps, final_k, sim_lb);

    /* Step 4: Optimal parameters */
    double p_opt = hastad_optimal_p(n, d);
    double exp_val = hastad_exponent(d);
    printf("  [Optimal] p=%.6f, exponent=1/%d=%.6f\n", p_opt, d-1, exp_val);

    /* Step 5: Consistency check */
    if (sim_lb >= fastad_lb * (1.0 - 1e-12)) {
        printf("  [CONSISTENCY] Simulation matches theoretical bound ✓\n");
    } else {
        printf("  [WARNING] Simulation (%.2e) < theoretical (%.2e)\n",
               sim_lb, fastad_lb);
    }
}

/* ── Switching lemma parameter space exploration ─────────────────── */
/**
 * Compute the "safe region" of (p, s) parameters where the
 * switching lemma guarantees success with probability > 1-epsilon.
 *
 * For a given k (DNF width) and target confidence (1-epsilon),
 * find pairs (p, s) that satisfy (5pk)^s <= epsilon.
 */
void switching_lemma_parameter_space(int k, double epsilon) {
    printf("Switching Lemma parameter space for k=%d, epsilon=%.1e:\n", k, epsilon);
    printf("  Condition: (5pk)^s <= epsilon\n");
    printf("  p * 5k = %.1f * p\n", 5.0 * k);

    /* Sample p values and compute minimum s */
    printf("\n  %-8s %-8s %-8s %-15s\n", "p", "5pk", "s_min", "P(fail)");
    printf("  %-8s %-8s %-8s %-15s\n", "--------", "--------", "--------", "---------------");

    double p_samples[] = {0.01, 0.02, 0.05, 0.10, 0.15, 0.20};
    int n_samples = (int)(sizeof(p_samples)/sizeof(p_samples[0]));

    for (int i = 0; i < n_samples; i++) {
        double p = p_samples[i];
        double five_kp = 5.0 * k * p;
        /* Find minimum s such that (5pk)^s <= epsilon */
        int s_min = 1;
        double prob = five_kp;
        while (prob > epsilon && s_min < 100) {
            s_min++;
            prob = pow(five_kp, s_min);
        }
        printf("  %-8.2f %-8.3f %-8d %-15.2e\n", p, five_kp, s_min, prob);
    }
}

/* ── Compare Sipser, Yao, and Hastad lower bounds ──────────────── */
/**
 * Historical comparison of the three main lower bounds for PARITY in AC0.
 * Shows how the bound improved over time.
 */
void lower_bound_historical_compare(int n, int d) {
    printf("Historical comparison of PARITY lower bounds (n=%d, d=%d):\n\n", n, d);

    /* Sipser 1983: exp(n^{1/d}) */
    double sipser_lb = exp(pow((double)n, 1.0 / d) * 0.01);
    printf("  Sipser (1983):   exp(Omega(n^{1/%d})) > %.2e\n", d, sipser_lb);

    /* Yao 1985: exp(n^{1/(2d)}) */
    double yao_lb = exp(pow((double)n, 1.0 / (2.0 * d)) * 0.01);
    printf("  Yao (1985):      exp(Omega(n^{1/%d}))  > %.2e\n", 2*d, yao_lb);

    /* Hastad 1986: exp(n^{1/(d-1)}) — OPTIMAL */
    double hastad_lb = hastad_lower_bound(n, d);
    printf("  Hastad (1986):   exp(Omega(n^{1/%d}))  > %.2e", d-1, hastad_lb);

    if (d >= 2) {
        printf("  ← OPTIMAL\n");
    } else {
        printf("\n");
    }

    /* Show that Hastad's exponent is strictly better for d >= 3 */
    if (d >= 3) {
        double ratio_sipser = log(hastad_lb) / log(sipser_lb);
        double ratio_yao = log(hastad_lb) / log(yao_lb);
        printf("\n  Ratio (Hastad/Sipser): %.2fx in log-space\n", ratio_sipser);
        printf("  Ratio (Hastad/Yao):    %.2fx in log-space\n", ratio_yao);
    }
}

/* ── Analytic upper bound (alternative formula) ─────────────────── */
/**
 * An analytic closed-form estimate of the AC0 upper bound for PARITY.
 * Unlike parity_upper_bound_size() (recursive construction), this
 * computes directly: exp(O(n^{1/(d-1)})).
 *
 * This is used to cross-validate the recursive bound and provide
 * an analytically simpler expression.
 */
double parity_upper_bound_analytic(int n, int d) {
    if (d <= 1) return 1e308;
    if (d == 2) return pow(2.0, n);
    double exponent = pow((double)n, 1.0 / (d - 1));
    return exp(2.5 * exponent);
}

/* ── Gap analysis of Hastad bound tightness ──────────────────── */
/**
 * Analyze the gap between lower and upper bounds for Hastad's theorem.
 * Complements hastad_tightness_check() in depth_reduction.c with
 * a probabilistic analysis of the bound tightness.
 */
void hastad_bound_gap_analysis(int n, int d) {
    double lb = hastad_lower_bound(n, d);
    double ub = parity_upper_bound_analytic(n, d);

    printf("Gap analysis for n=%d, d=%d:\n", n, d);
    printf("  Lower bound (Hastad): log2(LB) = %.1f\n", log2(lb));
    printf("  Upper bound (const):  log2(UB) = %.1f\n", log2(ub));

    double log_ratio = log(ub) / log(lb);
    printf("  Log-space ratio (UB/LB): %.4f\n", log_ratio);
    printf("  Exponent: 1/(d-1) = %.4f\n", 1.0 / (d - 1));

    if (log_ratio < 100.0) {
        printf("  Gap: narrow — bounds are tight up to constant factors\n");
    } else {
        printf("  Gap: wide in absolute terms, but both are exponential\n");
    }
    printf("  Note: The gap is in the constant c in exp(c * n^{1/(d-1)}).\n");
    printf("  Hastad proves c >= 1/10 (lower), construction gives c <= 2.5 (upper).\n");
}

/* ── Probability of switching success across layers ───────────── */
/**
 * Compute the probability that the switching lemma succeeds at
 * each layer of the depth reduction. The overall probability
 * of the proof is the product of success probabilities.
 */
void switching_success_probability(int n, int d) {
    printf("Switching success probability across %d layers:\n\n", d-2);

    double total_prob = 1.0;
    int remaining_vars = n;
    int remaining_depth = d;
    int k = 3;  /* typical bottom fan-in */

    printf("  %-8s %-8s %-12s %-15s %-15s\n",
           "Layer", "k", "p", "s", "P(success)");
    printf("  %-8s %-8s %-12s %-15s %-15s\n",
           "--------", "--------", "------------", "---------------", "---------------");

    for (int layer = 0; layer < d - 2; layer++) {
        double p = hastad_switching_p(remaining_vars, remaining_depth);
        int s = (int)(pow((double)remaining_vars, 1.0 / (remaining_depth - 1)));

        double fail_prob = switching_lemma_prob_bound(k, p, s);
        double success_prob = 1.0 - fail_prob;
        if (success_prob > 1.0) success_prob = 1.0;
        if (success_prob < 0.0) success_prob = 0.0;

        printf("  %-8d %-8d %-12.6f %-15d %-15.6f\n",
               layer+1, k, p, s, success_prob);

        total_prob *= success_prob;
        remaining_depth--;
        k = s;
    }

    printf("\n  Total success probability: %.6e\n", total_prob);
    printf("  Interpretation: For the proof to fail, at least one layer\n");
    printf("  must fail. With proper parameter selection, total failure\n");
    printf("  probability is exponentially small.\n");
}

/* ── Comparison with non-constructive random restriction bound ─── */
/**
 * Demonstrate that the random restriction method gives a
 * non-constructive lower bound — it proves EXISTENCE of a
 * restriction that kills the circuit, without constructing it.
 */
void nonconstructive_vs_constructive(int n, int d) {
    printf("Non-constructive vs constructive lower bounds:\n\n");

    double nonconst_lb = hastad_lower_bound(n, d);
    printf("  Non-constructive (Hastad):\n");
    printf("    Proves EXISTENCE of a restriction killing the circuit.\n");
    printf("    Uses probabilistic method: Pr[failure] < 1 => exists good restriction.\n");
    printf("    Lower bound: size >= %.2e\n\n", nonconst_lb);

    printf("  Constructive:\n");
    printf("    To actually BUILD a restriction requires enumerating\n");
    printf("    2^n possible assignments — no efficient algorithm known.\n");
    printf("    This is the characteristic of probabilistic method proofs.\n\n");

    printf("  Note: The non-constructive nature does not weaken the\n");
    printf("  lower bound — it's still a mathematically valid proof.\n");
    printf("  The restriction EXISTS, even if we can't find it efficiently.\n");
}

/* ── Exponential lower bound for constant-depth circuits ───────── */
/**
 * Demonstrate that for ANY constant depth d, the lower bound
 * is exponential (exp(n^c) for c = 1/(d-1) > 0).
 * This is the core result: PARITY ∉ AC0.
 */
void parity_not_in_ac0_proof(int n) {
    printf("Proof sketch: PARITY not in AC0\n\n");
    printf("Assume, for contradiction, that PARITY can be computed\n");
    printf("by an AC0 circuit family {C_n} of polynomial size and\n");
    printf("constant depth d.\n\n");

    printf("Then for any n, PARITY on n bits has a depth-d circuit\n");
    printf("of size S = poly(n).\n\n");

    printf("Hastad lower bound says: S >= exp(Omega(n^{1/(d-1)})).\n\n");

    printf("For constant d:\n");
    for (int d = 2; d <= 8; d++) {
        double lb = hastad_lower_bound(n, d);
        printf("  d=%d: S >= exp(Omega(n^{%.4f})) = %.2e\n",
               d, 1.0/(d-1), lb);
    }

    printf("\nFor any constant d, exp(Omega(n^{1/(d-1)})) grows faster\n");
    printf("than any polynomial in n. Therefore, S = poly(n) is impossible.\n");
    printf("Contradiction! Hence PARITY ∉ AC0. ∎\n");
}

/* ── Extension to MOD_m functions ─────────────────────────────── */
/**
 * Briefly discuss the general case: MOD_m not in AC0 for odd m
 * (follows from Razborov-Smolensky, not Hastad, but uses similar techniques).
 */
void mod_m_lower_bounds_note(void) {
    printf("Extension: MOD functions and AC0\n\n");
    printf("Hastad's switching lemma handles PARITY (MOD_2) optimally.\n");
    printf("For MOD_p with p prime and q prime distinct from p:\n");
    printf("  Razborov (1987), Smolensky (1987): MOD_p not in AC0[MOD_q]\n\n");

    printf("Key difference: Razborov-Smolensky uses polynomial method\n");
    printf("over GF(p) instead of switching lemma.\n\n");

    printf("For MOD_6 (composite modulus):\n");
    printf("  Whether MOD_6 ∈ AC0 is a MAJOR OPEN PROBLEM.\n");
    printf("  Neither switching lemma nor polynomial method resolves it.\n");
    printf("  Connected to: ACC0 vs NEXP, circuit lower bounds frontier.\n");
}
