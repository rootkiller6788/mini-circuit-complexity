/**
 * src/applications.c — Switching Lemma Applications (L7-L8)
 * =======================================================
 *
 * Implements L7 (Applications) and L8 (Advanced Topics) of the
 * nine-layer knowledge framework. These functions demonstrate how
 * the switching lemma extends beyond lower bounds to pseudorandom
 * generators, learning theory, and correlation bounds.
 *
 * L7 APPLICATIONS:
 *   - Nisan-Wigderson PRG: Hardness → pseudorandomness
 *   - LMN Fourier tail bound: AC0 circuits are Fourier-concentrated
 *   - Nisan's PRG for AC0: Seed length O(log^{2d} n)
 *
 * L8 ADVANCED TOPICS:
 *   - Multi-switching lemma: Switch multiple layers at once
 *   - Average-case hardness: PARITY is hard on random inputs
 *   - Correlation bounds: AC0 cannot even approximate MOD_m
 *   - Natural proofs barrier: Switching lemma is a natural proof
 *
 * COURSE MAPPING:
 *   - MIT 6.841: Advanced switching lemma applications
 *   - Stanford CS254: Pseudorandomness from hardness
 *   - Berkeley CS278: Fourier analysis of AC0
 *   - Princeton COS 522: Cryptography and circuit lower bounds
 *   - CMU 15-855: Natural proofs barrier
 *
 * REFERENCES:
 *   - Nisan & Wigderson (1994), "Hardness vs Randomness"
 *   - Linial, Mansour, Nisan (1993), "Constant depth circuits,
 *     Fourier transform, and learnability"
 *   - Nisan (1991), "Pseudorandom bits for constant depth circuits"
 *   - Razborov & Rudich (1997), "Natural Proofs"
 *   - Arora & Barak (2009), Chapters 19-20, 23
 */

#include "switching.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ================================================================
 * L7: NISAN-WIGDERSON PRG
 * ================================================================ */

/**
 * nw_generator_stretch — Compute NW PRG stretch factor.
 *
 * If a Boolean function f on n variables has circuit complexity
 * at least H (no circuit of size < H can compute f), then the
 * Nisan-Wigderson PRG can stretch a random seed into pseudorandom
 * bits. The stretch factor is:
 *
 *   stretch ≈ output_bits / seed_bits
 *
 * For a function with hardness H, we can generate ~H/(n^2) bits
 * from a seed of length O(n) bits.
 *
 * For PARITY: hardness ≈ exp(Ω(n^{1/(d-1)})) for AC0 of depth d.
 * So the stretch is exponential.
 *
 * @param hardness  Minimum circuit size needed to compute f
 * @param n         Number of input variables to f
 * @return          Stretch factor (output bits / seed bits)
 *
 * Reference: Nisan & Wigderson (1994), "Hardness vs Randomness", JCSS
 */
double nw_generator_stretch(double hardness, int n) {
    if (n <= 0 || hardness <= 1.0) return 1.0;

    /* Simplified NW stretch formula:
     * stretch ≈ hardness / (n^2 * seed_length_factor)
     * where seed_length_factor ≈ 10 * n (typical) */
    double seed_len = 10.0 * (double)n;
    double output_bits = hardness / ((double)n * (double)n * 2.0);
    if (output_bits < seed_len) return output_bits / seed_len;
    return output_bits / seed_len;
}

/* ================================================================
 * L7: LMN FOURIER TAIL BOUND
 * ================================================================ */

/**
 * lmn_fourier_tail_bound — Compute LMN Fourier concentration bound.
 *
 * THEOREM (Linial-Mansour-Nisan 1993):
 *   For any Boolean function f computable by an AC0 circuit of
 *   size S and depth d, the sum of squared Fourier coefficients
 *   at levels above L is bounded by:
 *
 *     Σ_{|U| > L} f̂(U)² ≤ S · 2^{-Ω(L^{1/d})}
 *
 * This means AC0 functions are well-approximated by low-degree
 * polynomials, which implies they can be learned in quasi-polynomial
 * time under the uniform distribution.
 *
 * This theorem uses the switching lemma as its core tool.
 *
 * @param S  Circuit size (number of gates)
 * @param d  Circuit depth
 * @param L  Fourier level cutoff
 * @param n  Number of variables
 * @return   Upper bound on high-level Fourier weight
 *
 * Complexity: O(1)
 */
double lmn_fourier_tail_bound(double S, int d, int L, int n) {
    if (d <= 0 || L <= 0 || S <= 0.0 || n <= 0) return 1.0;

    /* LMN bound: W_>L ≤ S · 2^{-c · L^{1/d}}
     * where c is a constant depending on the proof.
     * Using c ≈ 1/20 (from standard formulations). */
    double c = 1.0 / 20.0;
    double exponent = -c * pow((double)L, 1.0 / (double)d);
    double bound = S * pow(2.0, exponent);

    /* Clamp to [0, 1] */
    if (bound > 1.0) bound = 1.0;
    if (bound < 0.0) bound = 0.0;
    return bound;
}

/* ================================================================
 * L7: NISAN'S PRG FOR AC0
 * ================================================================ */

/**
 * nisan_prg_seed_length — Compute seed length for Nisan's PRG.
 *
 * THEOREM (Nisan 1991):
 *   There exists a pseudorandom generator that fools AC0 circuits
 *   of size n and depth d with seed length O(log^{2d}(n/ε)).
 *
 * The construction uses the switching lemma iteratively.
 *
 * @param n       Circuit size / input length
 * @param d       Circuit depth
 * @param epsilon Error parameter
 * @return        Approximate seed length in bits
 *
 * Reference: Nisan (1991), "Pseudorandom bits for constant depth circuits"
 */
double nisan_prg_seed_length(int n, int d, double epsilon) {
    if (n <= 0 || d <= 0 || epsilon <= 0.0) return 0.0;

    /* Nisan's bound: seed_len = O(log^{2d}(n/ε))
     * The constant hidden in O is roughly 2^d.
     * For practical estimates: seed_len ≈ (2^d) * log(n/ε)^{2d} */
    double ratio = (double)n / epsilon;
    if (ratio < 1.0) ratio = 1.0;
    double log_ratio = log(ratio) / log(2.0);  /* log base 2 */

    double seed_len = pow(2.0, (double)d) * pow(log_ratio, 2.0 * (double)d);
    return seed_len;
}

/* ================================================================
 * L8: MULTI-SWITCHING LEMMA
 * ================================================================ */

/**
 * multi_switching_bound — Multi-switching lemma bound.
 *
 * The multi-switching lemma generalizes the switching lemma to
 * handle switching of composed DNF formulas (k-DNF of t-DNFs).
 *
 * Original bound: Pr[fail] < (5pk)^s for k-DNF → s-CNF
 * Multi bound:   Pr[fail] < (C · p^k · t)^{s} for k-DNF of t-DNFs
 *
 * @param k  Inner DNF width
 * @param t  Outer DNF width (or number of inner DNFs)
 * @param p  Star probability
 * @param s  Target width
 * @return   Upper bound on failure probability
 *
 * Reference: Hastad (circa 2014), "On the correlation of parity
 *   and small-depth circuits" (multi-switching)
 */
double multi_switching_bound(int k, int t, double p, int s) {
    if (k <= 0 || t <= 0 || s <= 0) return 1.0;

    /* Multi-switching uses a different constant.
     * Rough bound: Pr[fail] < (C * p^k * t)^s
     * where C ≈ 10 (depends on proof details). */
    double C = 10.0;
    double base = C * pow(p, (double)k) * (double)t;
    if (base >= 1.0) return 1.0;
    return pow(base, (double)s);
}

/* ================================================================
 * L8: AVERAGE-CASE HARDNESS
 * ================================================================ */

/**
 * average_case_parity_lower — Average-case lower bound for PARITY.
 *
 * Hastad's switching lemma implies not just worst-case but
 * AVERAGE-CASE hardness: any AC0 circuit that agrees with PARITY
 * on at least 1/2 + ε fraction of inputs still requires
 * exponential size.
 *
 * The bound: S ≥ exp(Ω(n^{1/(d-1)})) · poly(1/ε)
 *
 * This is stronger than worst-case: even allowing many errors
 * doesn't help AC0 compute PARITY efficiently.
 *
 * @param n       Number of variables
 * @param d       Circuit depth
 * @param epsilon Advantage over random guessing (> 0)
 * @return        Minimum circuit size required
 *
 * Reference: Hastad (1986), correlation bounds for PARITY
 */
double average_case_parity_lower(int n, int d, double epsilon) {
    if (n <= 0 || d <= 1) return (double)n;
    if (epsilon <= 0.0 || epsilon >= 0.5) epsilon = 0.1;

    /* Same exponential term as worst-case, plus ε-dependent factor */
    double base = hastad_parity_lower_bound(n, d);
    double eps_factor = 1.0 / (epsilon * epsilon);
    return base / eps_factor;
}

/* ================================================================
 * L8: CORRELATION BOUNDS (Razborov-Smolensky style)
 * ================================================================ */

/**
 * correlation_bound_modm_ac0 — Correlation bound for MOD_m vs AC0.
 *
 * THEOREM (Razborov 1987, Smolensky 1987):
 *   For odd prime p, MOD_p cannot be computed by AC0 circuits
 *   of sub-exponential size. Moreover, the correlation between
 *   any AC0 circuit of depth d and MOD_p is at most
 *   exp(-Ω(n^{1/(2d-2)})) for size-S circuits.
 *
 * This function computes an estimated correlation bound.
 *
 * For m = 2 (PARITY): the switching lemma directly gives the bound.
 * For odd m: we use the polynomial method (Razborov-Smolensky).
 *
 * @param m  Modulus
 * @param n  Number of variables
 * @param d  Circuit depth
 * @param S  Circuit size
 * @return   Upper bound on correlation
 *
 * Reference: Razborov (1987), Smolensky (1987),
 *   "Lower bounds for MOD_p circuits"
 */
double correlation_bound_modm_ac0(int m, int n, int d, int S) {
    if (n <= 0 || d <= 1 || S <= 1) return 1.0;

    double corr;
    if (m == 2) {
        /* MOD_2 = PARITY. Switching lemma directly:
         * correlation ≤ S · 2^{-Ω(n^{1/(d-1)})} */
        double exp_term = pow((double)n, 1.0 / (double)(d - 1)) / 20.0;
        corr = (double)S * pow(2.0, -exp_term);
    } else {
        /* Odd modulus: use polynomial approximation.
         * Correlation ≤ S · n^{-Ω(n^{1/(2d)})} */
        double exp_term = pow((double)n, 1.0 / (double)(2 * d)) / 10.0;
        corr = (double)S * pow((double)n, -exp_term);
    }

    /* Clamp */
    if (corr > 1.0) corr = 1.0;
    if (corr < 1e-100) corr = 1e-100;
    return corr;
}

/* ================================================================
 * L8: NATURAL PROOFS BARRIER
 * ================================================================ */

/**
 * natural_property_check — Check if a Boolean function satisfies
 * the "natural property" derived from the switching lemma.
 *
 * Natural property (Razborov-Rudich 1997):
 *   - Constructive: Can check in polynomial time whether a function
 *     (represented as truth table) has the property.
 *   - Largeness: A random function has the property with high prob.
 *   - Usefulness: Any function with the property is hard for some class.
 *
 * The switching lemma gives the property:
 *   "f has a decision tree of depth ≤ n^{ε}"
 * Any function with this property is easy for AC0.
 * PARITY does NOT have this property.
 *
 * This function checks whether the property holds for a given
 * Boolean function by building its decision tree and comparing
 * depth against a threshold.
 *
 * @param bf    Boolean function (as truth table)
 * @param depth AC0 circuit depth threshold
 * @return      1 if function might be in depth-d AC0, 0 otherwise
 *
 * Complexity: O(2^n_vars) (building decision tree)
 */
int natural_property_check(const BoolFunc* bf, int depth) {
    if (!bf || depth < 1) return 0;

    /* The natural property: function has decision tree depth ≤ n^{1/depth} */
    double threshold = pow((double)bf->n_vars, 1.0 / (double)depth);

    /* Build decision tree from truth table */
    DTNode* tree = dt_from_truthtable(bf->n_vars, bf->table);
    if (!tree) return 0;

    int tree_depth = dt_depth(tree);
    dt_free(tree);

    /* If tree depth ≤ threshold, function is "simple" for AC0 */
    return ((double)tree_depth <= threshold) ? 1 : 0;
}

/* ================================================================
 * L8: ADDITIONAL BOUNDS
 * ================================================================ */

/**
 * switching_optimal_parameters — Compute optimal switching parameters.
 *
 * Given n (variables) and d (depth), compute Hastad's recommended
 * parameters for proving PARITY ∉ AC0:
 *   - p = n^{-1/(d-1)}  (star probability)
 *   - k = O(log S)       (DNF width bound)
 *   - s = O(log S)       (CNF width bound after switching)
 *
 * Where S is the hypothetical circuit size.
 *
 * This function prints the optimal parameter table.
 *
 * @param n      Number of variables
 * @param d      Circuit depth
 * @param S_est  Estimated circuit size to check
 */
void switching_optimal_parameters(int n, int d, double S_est) {
    printf("\nOptimal Switching Parameters (n=%d, d=%d):\n", n, d);
    printf("  p = n^{-1/(d-1)} = %.6f\n", pow((double)n, -1.0 / (double)(d - 1)));
    printf("  For S = %.1e:\n", S_est);
    double k_bound = (S_est > 1.0) ? log(S_est) / log(2.0) : 1.0;
    printf("    k (DNF width bound) ≤ %.1f\n", k_bound);
    double p_opt = pow((double)n, -1.0 / (double)(d - 1));
    double prod = 5.0 * p_opt * k_bound;
    printf("    5pk = %.4f %s\n", prod, (prod < 1.0) ? "(< 1: switching works)" : "(≥ 1: bad)");
}

/* ================================================================
 * L8: SWITCHING LEMMA WITH ERROR HANDLING
 * ================================================================ */

/**
 * switching_lemma_robust — Robust switching lemma computation.
 *
 * Same as switching_prob_bound but with input validation and
 * proper handling of edge cases.
 *
 * Returns -1.0 if parameters are invalid.
 */
double switching_lemma_robust(int k, double p, int s) {
    if (k <= 0 || p < 0.0 || p > 1.0 || s <= 0) return -1.0;
    if (5.0 * p * (double)k >= 1.0) return 1.0;
    return pow(5.0 * p * (double)k, (double)s);
}

/* ================================================================
 * L9: RESEARCH FRONTIERS (documented, not implemented)
 * ================================================================ */

/**
 * research_frontiers_switching — Print research frontier summary.
 *
 * L9 topics are documented here (no implementation required per SKILL.md):
 *
 * 1. META-COMPLEXITY:
 *    - Can we prove circuit lower bounds using the minimum circuit
 *      size problem (MCSP)? Switching lemma gives AC0 lower bounds
 *      but MCSP is about general circuits.
 *    - Current frontier: proving MCSP is NP-hard.
 *
 * 2. GEOMETRIC COMPLEXITY THEORY (GCT):
 *    - Mulmuley-Sohoni approach to P vs NP via algebraic geometry
 *    - Switching lemma lives in Boolean world; GCT uses
 *      representation theory over C.
 *    - Can switching be "lifted" to algebraic setting?
 *
 * 3. QUANTUM COMPLEXITY:
 *    - BQP vs classical: do quantum circuits have switching lemmas?
 *    - Early work on quantum switching lemma for QAC0 circuits
 *      (Bravyi et al., 2020)
 *
 * 4. HARDNESS OF APPROXIMATION:
 *    - Switching lemma used to prove optimal inapproximability
 *      for MAX-3SAT and related problems.
 *    - Current: closing the gap for unique games.
 *
 * 5. FINE-GRAINED COMPLEXITY:
 *    - Can we prove ETH-based lower bounds using switching?
 *    - Connections between circuit lower bounds and SETH.
 */
void research_frontiers_switching(void) {
    printf("\n=== L9: RESEARCH FRONTIERS ===\n\n");
    printf("1. META-COMPLEXITY:\n");
    printf("   MCSP (Minimum Circuit Size Problem) and circuit lower bounds.\n");
    printf("   Can switching lemma techniques be used for MCSP hardness?\n\n");

    printf("2. GEOMETRIC COMPLEXITY THEORY:\n");
    printf("   Algebraic approach to P vs NP. Can switching be algebraized?\n\n");

    printf("3. QUANTUM SWITCHING LEMMA:\n");
    printf("   QAC0 circuits and quantum switching lemma (Bravyi et al. 2020).\n");
    printf("   Active research: lower bounds for shallow quantum circuits.\n\n");

    printf("4. HARDNESS OF APPROXIMATION:\n");
    printf("   Switching lemma → optimal inapproximability results.\n");
    printf("   Kadish-Santhanam (2020): switching lemma for low-depth Frege.\n\n");

    printf("5. FINE-GRAINED COMPLEXITY:\n");
    printf("   Connections between switching lemma and ETH/SETH.\n");
    printf("   Can we get tighter bounds using fine-grained techniques?\n");
    printf("\n");
    printf("Hastad's switching lemma (1986) remains one of the most\n");
    printf("influential results in theoretical computer science.\n");
    printf("Godel Prize 1994. ACM Doctoral Dissertation Award 1986.\n");
    printf("\n");
}
