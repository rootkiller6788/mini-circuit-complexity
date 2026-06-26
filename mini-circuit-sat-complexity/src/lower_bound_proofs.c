/* lower_bound_proofs.c -- Circuit Lower Bounds
 *
 * Implements classic circuit lower bound methods:
 *   - Shannon's counting bound (1949)
 *   - Lupanov's upper bound (1958)
 *   - Gate elimination method
 *   - Random restriction method
 *   - Hastad's Switching Lemma
 *   - Neciporuk's bound
 *   - Krapchenko's formula lower bound
 *   - Andreev function bound
 *
 * Methods for proving that certain functions
 * require large circuits/formulas.
 *
 * References:
 *   Shannon (1949) "The synthesis of two-terminal switching circuits"
 *   Lupanov (1958) "On the synthesis of contact circuits"
 *   Hastad (1986) "Almost optimal lower bounds for small depth circuits"
 *   Jukna (2012) "Boolean Function Complexity" Ch.4-6
 *   Vollmer (1999) Ch.6-7
 */

#include "csat.h"

/* ============================================================================
 * L6: Circuit Lower Bounds
 * ============================================================================ */

/* Shannon's counting bound:
 * Number of Boolean functions on n inputs: 2^{2^n}
 * Number of circuits of size s (with t gate types): ~ (2t * s^2)^s
 * Setting s = 2^n / (c*n) gives: #circuits << #functions
 * => Almost all functions need Omega(2^n / n) gates.
 *
 * Returns: lower bound 2^n / n */
double shannon_bound(int n)
{
    if (n <= 0) return 0;
    return pow(2.0, (double)n) / (double)n;
}

/* Lupanov's upper bound:
 * Every Boolean function has a circuit of size at most
 *   2^n/n * (1 + O(log n / n))
 * Uses "Lupanov representation": decompose truth table
 * into blocks and encode each block efficiently.
 *
 * Returns: upper bound 2^n/n * (1 + log n / n) */
double lupanov_bound(int n)
{
    if (n <= 0) return 0;
    double t = pow(2.0, (double)n);
    return t / (double)n * (1.0 + log((double)n) / (double)n);
}

/* Gate elimination bound:
 * Each gate elimination step removes at most its fan-in
 * variables from consideration. After removing k gates,
 * the remaining function depends on at most n - k variables.
 *
 * For a circuit of size m computing a function that depends
 * on all n variables: m >= n / d for depth-d circuits.
 *
 * More generally: any formula of size m computing a function
 * of n variables has depth at least log_{f}(n) where f
 * is the maximum fan-in.
 *
 * Returns: lower bound estimate. */
double gate_elimination_bound(int n, int m, int d)
{
    if (n <= 0 || m <= 0 || d <= 0) return 0;
    /* For formula of size m and depth d:
     * leaf-size >= n => m >= n^{1/d} * (branching_factor-1)
     * Return the implied leaf-size lower bound. */
    return pow((double)m, 1.0 / (double)d) * log((double)n);
}

/* Random restriction bound:
 * Under random restriction with probability p of
 * keeping each variable unset, the expected circuit
 * shrinks significantly for small-depth circuits.
 *
 * For AC0 circuits of depth d:
 *   Pr[circuit collapses to constant] >= 1 - (5p * t)^k
 * where t = bottom fan-in, k = target decision tree depth.
 *
 * Returns: expected circuit size after restriction. */
double random_restriction_bound(int n, int d, double p)
{
    if (n <= 0 || d <= 1 || p <= 0 || p >= 1) return 0;
    /* After restriction with probability p, expected size
     * of depth-d circuit shrinks by factor ~ p^{1/d} */
    double remaining_vars = p * (double)n;
    if (remaining_vars < 1) return 1;
    /* Lower bound: need size at least */
    return exp(pow(remaining_vars, 1.0 / (double)(d - 1)));
}

/* Hastad Switching Lemma:
 * Theorem: Let f be a k-DNF (or k-CNF). Then for a random
 * restriction rho with probability p, the probability that
 * f|_rho is not computable by a decision tree of depth t
 * is at most (5pk)^t.
 *
 * Corollary (PARITY not in AC0):
 *   Any depth-d circuit computing PARITY_n has size at least
 *   exp(n^{1/(d-1)}).
 *
 * Parameters:
 *   n: number of variables
 *   d: circuit depth
 *   k: target decision tree depth
 *
 * Returns: lower bound on required circuit size. */
double switching_lemma_bound(int n, int d, int k)
{
    if (n <= 0 || d <= 1 || k <= 0) return 0;
    /* Set p to balance: p = n^{-1/(2d)} */
    double exponent = 1.0 / (2.0 * (double)d);
    double p = pow((double)n, -exponent);
    /* Bottom fan-in after restriction: ~ log s */
    double s_lb = pow(2.0, pow((double)n, 1.0 / (3.0 * (double)(d - 1))));
    /* Success probability of switching lemma */
    double bot_fan = log(s_lb) / log(2.0);
    double prob = pow(5.0 * p * bot_fan, (double)k);
    if (prob > 1.0) prob = 1.0;
    /* If prob < 1, there exists a restriction making it a decision tree */
    (void)prob;
    return s_lb;
}

/* Neciporuk's bound:
 * For a Boolean function f, partition the n input variables
 * into S disjoint subsets. If f depends on each subset
 * independently (i.e., the subfunctions are distinct),
 * then any formula for f has size at least Omega(S).
 *
 * More precisely: let S be the number of distinct subfunctions
 * on disjoint variable sets. Then L(f) >= S - n.
 *
 * Returns: formula size lower bound. */
double neciporuk_bound(int n, int* sub_counts, int n_sub)
{
    if (n <= 0 || !sub_counts || n_sub <= 0) return 0;
    int total_sub = 0;
    for (int i = 0; i < n_sub; i++)
        total_sub += sub_counts[i];
    /* Formula size >= total_sub - n */
    double bound = (double)(total_sub - n);
    return (bound > 0) ? bound : 0;
}

/* Krapchenko's formula lower bound:
 * For PARITY_n, any formula over {AND, OR, NOT} has size
 * at least n^2.
 *
 * This is tight: PARITY has formula size Theta(n^2) over
 * the De Morgan basis.
 *
 * Returns: n^2 */
double krapchenko_bound(int n, int parities)
{
    if (n <= 0) return 0;
    /* PARITY_n requires formula size >= n^2 */
    double base = (double)n * (double)n;
    /* For multiple parities, bound multiplies */
    if (parities > 1) base *= (double)parities;
    return base;
}

/* Andreev function:
 * A function that nearly achieves the maximum possible
 * formula size of n^{2-o(1)}.
 *
 * Construction: encode error-correcting code into
 * function values, then use binary tree structure.
 *
 * Returns: Andreev function formula size lower bound. */
double andreev_bound(int n)
{
    if (n <= 1) return 0;
    /* Andreev function: formula size >= n^{2 - o(1)} */
    double logn = log((double)n);
    return pow((double)n, 2.0) / (logn * logn);
}

/* ============================================================================
 * Derived measures
 * ============================================================================ */

/* Compute the Shannon-Lupanov gap: shows that
 * Shannon's lower bound and Lupanov's upper bound
 * differ by only a logarithmic factor.
 * Gap factor = Lupanov / Shannon ~ 1 + log n / n.
 * As n grows, gap -> 1. */
double shannon_lupanov_gap(int n)
{
    if (n <= 0) return 0;
    double sb = shannon_bound(n);
    if (sb <= 0) return 0;
    return lupanov_bound(n) / sb;
}

/* Estimate circuit size for a given function class.
 *   func_class: 0=random, 1=parity, 2=majority, 3=AC0, 4=NP
 *   n: number of inputs
 * Returns estimated minimum circuit size. */
double estimate_circuit_size(int func_class, int n)
{
    if (n <= 0) return 0;
    switch (func_class) {
        case 0: /* Random: Shannon bound */
            return shannon_bound(n);
        case 1: /* PARITY: requires exponential size in AC0,
                 * Theta(n^2) formula size, O(n) in NC1 */
            return (double)n * log2((double)n);
        case 2: /* MAJORITY: in TC0, O(n) threshold circuit,
                 * but exponential in AC0 (Razborov-Smolensky) */
            return pow(2.0, pow((double)n, 0.5));
        case 3: /* AC0 function: poly size, constant depth */
            return pow((double)n, 3.0);
        case 4: /* NP function: conjectured super-poly */
            return pow(2.0, pow((double)n, 0.01));
        default:
            return shannon_bound(n);
    }
}

/* Print a formatted lower bound report */
void circuit_lower_bound_report(int n)
{
    if (n <= 0) return;
    printf("\n--- Circuit Lower Bound Report (n=%d) ---\n", n);
    printf("Shannon lower bound:        %.1f\n", shannon_bound(n));
    printf("Lupanov upper bound:        %.1f\n", lupanov_bound(n));
    printf("Shannon-Lupanov gap factor: %.4f\n", shannon_lupanov_gap(n));
    printf("Gate elimination (d=3):     %.1f\n",
           gate_elimination_bound(n, (int)shannon_bound(n), 3));
    printf("Switching lemma (d=3):      %.1e\n",
           switching_lemma_bound(n, 3, n/2));
    printf("Krapchenko (parity, fml):   %.1f\n",
           krapchenko_bound(n, 1));
    printf("Andreev function:           %.1f\n", andreev_bound(n));
    printf("\nEstimated sizes:\n");
    for (int cls = 0; cls <= 4; cls++) {
        const char* names[] = {"Random", "PARITY", "MAJORITY", "AC0", "NP"};
        printf("  %-10s: %.1e\n", names[cls], estimate_circuit_size(cls, n));
    }
}