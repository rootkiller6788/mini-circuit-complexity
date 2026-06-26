/* shannon_count.c -- Shannon Counting for Circuit Lower Bounds
 *
 * L4: Shannon's Theorem (1949):
 *   Almost all Boolean functions require circuits of size Omega(2^n / n).
 *
 * Counting lemma:
 *   Number of circuits of size S with n inputs <= 2^{O(S log(n+S))}
 *   Number of Boolean functions on n inputs = 2^{2^n}
 *
 * When S = poly(n):
 *   2^{O(poly(n) * log n)} << 2^{2^n}  for large n
 *   => Almost all functions are "hard" (require super-poly size)
 *
 * L5: Algorithms
 *   1. Circuit upper bound computation (with log-space for stability)
 *   2. Feasible fraction computation
 *   3. Largeness analysis for natural property testing
 *   4. Constructiveness cost estimation
 *
 * Key insight (Shannon 1949):
 *   For a circuit of size S:
 *     - Each gate type: AND, OR, NOT (for standard basis)
 *     - Inputs to each gate: any pair of previous gates or inputs
 *     - So at most (n+S)^2 choices per gate
 *     - With gate type choice: <= 3 * (n+S)^2 options per gate
 *     - Conservative: <= 4 * (n+S)^2
 *   Total circuits of size <= S: <= sum_{i=0}^{S} (4*(n+S)^2)^i
 *                                <= (S+1) * (4*(n+S)^2)^S
 *                                <= 2^{S * (2 + 2*log2(n+S)) + log2(S+1)}
 *
 *   This is 2^{O(S log S)} = 2^{O(poly(n) log n)} when S = poly(n).
 *   While total functions = 2^{2^n} = doubly exponential.
 *   So fraction feasible = 2^{poly(n) - 2^n} -> 0.
 *
 * Lupanov (1958): Every function has circuits of size <= 2^n/n * (1+o(1)).
 *   So the Shannon lower bound is asymptotically tight.
 *
 * References:
 *   Shannon (1949): BSTJ 28(1):59-98
 *   Lupanov (1958): "A method of circuit synthesis" (Doklady AN SSSR)
 *   Arora-Barak (2009), Section 6.1.1
 *   Jukna (2012), Section 1.4
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include "shannon.h"
#include "natural_core.h"

/* ========================================================================
 * Shannon Circuit Counting
 * ======================================================================== */

double shannon_circuit_upper_bound(int n, int S) {
    /* Upper bound on number of circuits of size <= S on n inputs.
     *
     * Derivation (detailed):
     *   Each of S gates has:
     *     - Type: 3 choices (AND, OR, NOT) -- NOT has fan-in 1
     *       — More precisely: AND/OR need 2 inputs each from n+S gates
     *       — NOT needs 1 input from n+S gates
     *     - Inputs: for AND/OR: (n+S)^2 choices of 2 inputs
     *                for NOT: (n+S) choices of 1 input
     *   Conservative overall: 4*(n+S)^2 choices per gate.
     *
     *   For exactly S gates: (4*(n+S)^2)^S
     *   For <= S gates: sum_{i=0}^{S} (4*(n+S)^2)^i
     *                  <= (S+1) * (4*(n+S)^2)^S  (geometric series)
     *
     *   log2: S * (2 + 2*log2(n+S)) + log2(S+1)
     */

    if (n < 0 || S < 0) return 0.0;
    if (S == 0) return 1.0;  /* empty circuit (constants) */

    double nS = (double)(n + S);
    if (nS <= 0) nS = 1.0;

    /* log2(4 * (n+S)^2) = 2 + 2*log2(n+S) */
    double log_per_gate = 2.0 + 2.0 * log2(nS);

    /* total log = S * log_per_gate + log2(S+1) */
    double log_total = (double)S * log_per_gate + log2((double)(S + 1));

    /* Convert to actual count, with overflow protection */
    if (log_total > 1000.0) {
        /* Would overflow double; return a large sentinel */
        return 1e300;
    }

    return pow(2.0, log_total);
}

double shannon_circuit_upper_bound_log(int n, int S) {
    /* Return log2 of the circuit count.
     * This is numerically stable for large values. */
    if (n < 0 || S < 0) return 0.0;
    if (S == 0) return 0.0;

    double nS = (double)(n + S);
    if (nS <= 0) nS = 1.0;

    double log_per_gate = 2.0 + 2.0 * log2(nS);
    return (double)S * log_per_gate + log2((double)(S + 1));
}

double shannon_function_count(int n) {
    /* Total Boolean functions: 2^{2^n}
     * n=0: 2^{1}=2     n=1: 2^{2}=4      n=2: 2^{4}=16
     * n=3: 2^{8}=256   n=4: 2^{16}=65536 n=5: 2^{32}~4.3e9
     * n=6: 2^{64}~1.8e19                   n=7: 2^{128}~3.4e38
     * n=8: 2^{256}~1.2e77                   n=9: 2^{512}~1.3e154
     * n=10: INF (2^{1024} > DBL_MAX ~ 1.8e308)
     */
    if (n < 0) return 0.0;
    double exponent = pow(2.0, (double)n);
    if (exponent > 1000.0) return INFINITY;
    return pow(2.0, exponent);
}

double shannon_function_count_log(int n) {
    /* log2 of total functions: log2(2^{2^n}) = 2^n.
     * This is exact and fits in double for n up to 1023. */
    if (n < 0) return 0.0;
    return pow(2.0, (double)n);
}

double shannon_feasible_fraction(int n, int S) {
    /* Fraction of functions computable by circuits of size <= S.
     * fraction = min(1.0, circuit_upper_bound / total_functions)
     *
     * For poly S: circuit_count << total_functions => fraction ~ 0
     * For S = 2^n/n: fraction approaches 1 (Lupanov 1958)
     */

    double circuits = shannon_circuit_upper_bound(n, S);
    double functions = shannon_function_count(n);

    if (functions == INFINITY || circuits >= functions) {
        return (circuits >= 1e300) ? 0.0 : 1.0;
    }
    if (functions <= 0.0) return 0.0;

    double frac = circuits / functions;
    return (frac > 1.0) ? 1.0 : frac;
}

double shannon_feasible_fraction_log(int n, int S) {
    /* Log2 of the feasible fraction.
     * log2(circuits / total) = log2(circuits) - 2^n
     * When this is negative: fraction << 1.
     */
    double log_circuits = shannon_circuit_upper_bound_log(n, S);
    double log_functions = shannon_function_count_log(n);
    double diff = log_circuits - log_functions;
    return (diff < 0.0) ? diff : 0.0;
}

/* ========================================================================
 * Largeness Analysis
 * ======================================================================== */

double shannon_largeness_fraction(int n, int S) {
    /* Fraction of functions with circuit complexity > S.
     * This is the "hardness" property: P(f) = "f requires size > S".
     *
     * fraction = 1 - feasible_fraction
     *
     * For poly S: fraction ~ 1 (almost all functions are hard)
     * => P is LARGE (much larger than 2^{-O(n)})
     */
    double feasible = shannon_feasible_fraction(n, S);
    return 1.0 - feasible;
}

int shannon_hardness_threshold(int n) {
    /* Binary search for S where ~50% of functions are harder.
     * median_S = argmin_S |largeness_fraction(n, S) - 0.5|
     *
     * Search range: [1, min(2^n, 1000000)] */
    if (n <= 0) return 0;

    int lo = 1;
    int hi = (int)fmin(pow(2.0, n) / fmax((double)n, 1.0), 1000000.0);
    if (hi < lo) hi = lo;

    /* Sample the fraction at endpoints to guide search */
    double frac_lo = shannon_largeness_fraction(n, lo);
    double frac_hi = shannon_largeness_fraction(n, hi);

    /* If even at hi, most functions are hard, reduce hi */
    if (frac_hi < 0.49) {
        /* S is too large — most functions are easy already */
        for (int S = lo; S <= hi; S *= 2) {
            if (shannon_largeness_fraction(n, S) >= 0.5) return S;
        }
        return hi;
    }

    /* Binary search for 50% point */
    while (lo < hi) {
        int mid = lo + (hi - lo) / 2;
        double frac = shannon_largeness_fraction(n, mid);
        if (frac >= 0.5) {
            lo = mid + 1;
        } else {
            hi = mid;
        }
    }
    return lo;
}

int shannon_min_size_for_fraction(int n, double frac) {
    /* Find minimum S such that at most 'frac' of functions have size <= S.
     * i.e., shannon_feasible_fraction(n, S) <= frac.
     *
     * This is used to find the size bound that makes the largeness
     * criterion satisfied with parameter c = -log2(1-frac)/n. */

    if (n <= 0 || frac <= 0.0) return 0;
    if (frac >= 1.0) return INT_MAX;

    /* Binary search: find S where feasible_fraction crosses frac */
    int lo = 0;
    int hi = (int)fmin(pow(2.0, n) / fmax(1.0, log2((double)n)), 1000000.0);
    if (hi < lo) hi = lo;

    while (lo < hi) {
        int mid = lo + (hi - lo) / 2;
        double f = shannon_feasible_fraction(n, mid);
        if (f > frac) {
            lo = mid + 1;
        } else {
            hi = mid;
        }
    }
    return lo;
}

/* ========================================================================
 * Constructiveness Analysis
 * ======================================================================== */

double shannon_constructiveness_cost(int n, int S) {
    /* Cost to check if a function has complexity > S by enumeration.
     *
     * Algorithm: for each circuit of size <= S, evaluate on the
     * given truth table and check if matches.
     *
     * Naive: O(circuit_count * 2^n) evaluations.
     * Cost = circuit_count * 2^n = 2^{S*log(4*(n+S)^2) + n}
     *
     * For S = poly(n): cost = 2^{O(n log n)} = 2^{O(n)} => constructive!
     * For S = exp(n): cost = 2^{exp(n)} => NOT constructive.
     */
    double log_cost = shannon_circuit_upper_bound_log(n, S) + (double)n;
    if (log_cost > 1000.0) return INFINITY;
    return pow(2.0, log_cost);
}

int shannon_is_constructive(int n, int S) {
    /* Check if the naive enumeration runs in 2^{O(n)} time.
     *
     * We consider it constructive if log2(cost) <= c * n for some
     * reasonable constant c (say c = 10).
     *
     * log2(cost) = S * (2 + 2*log2(n+S)) + log2(S+1) + n
     * We check if this is <= 10 * n.
     */
    double log_cost = shannon_circuit_upper_bound_log(n, S) + (double)n;
    return (log_cost <= 10.0 * (double)n) ? 1 : 0;
}

/* ========================================================================
 * Natural Property Criteria Check (Core Function)
 * ======================================================================== */

NaturalProperty shannon_natural_criteria_check(int n, int S) {
    /* Test whether P(f) = "f has circuit complexity > S" is natural.
     *
     * 1. CONSTRUCTIVE: can we test P in time 2^{O(n)}?
     * 2. LARGE: does P hold for >= 2^{-O(n)} fraction of functions?
     * 3. USEFUL: does P imply circuit lower bound against some class?
     *
     * For P = "circuit size > S":
     *   - Constructive if S = poly(n) (enumeration in 2^{O(n)})
     *   - Large if S is not too large (most functions need size >> poly(n))
     *   - Useful by definition (implies non-membership in SIZE[S])
     */

    NaturalProperty np;
    np.constructive = shannon_is_constructive(n, S);
    np.large = (shannon_largeness_fraction(n, S) >=
                pow(2.0, -2.0 * (double)n));  /* 2^{-2n} threshold */
    np.useful = 1;  /* P(f) => f requires size > S, which is a lower bound */

    /* Compute quantitative measures */
    double log_cost = shannon_circuit_upper_bound_log(n, S) + (double)n;
    np.constructiveness_bound = (n > 0) ? log_cost / (double)n : 0.0;

    double frac = shannon_largeness_fraction(n, S);
    np.largeness_exponent = (frac > 0 && n > 0) ?
        -log2(frac) / (double)n : 0.0;

    np.target_circuit_size = S;
    np.is_natural = (np.constructive && np.large && np.useful) ? 1 : 0;

    return np;
}

/* ========================================================================
 * Printing and Analysis
 * ======================================================================== */

void shannon_print_analysis(int n, int S, FILE *fp) {
    if (!fp) fp = stdout;

    fprintf(fp, "\n=== Shannon Analysis: n=%d, S=%d ===\n\n", n, S);

    /* Circuit count */
    double circs = shannon_circuit_upper_bound(n, S);
    double log_circs = shannon_circuit_upper_bound_log(n, S);
    fprintf(fp, "Circuits of size <= S:\n");
    fprintf(fp, "  Upper bound: ~ 2^{%.2f} = %.2e\n", log_circs, circs);

    /* Function count */
    double funcs = shannon_function_count(n);
    double log_funcs = shannon_function_count_log(n);
    fprintf(fp, "Total Boolean functions:\n");
    fprintf(fp, "  Count: 2^{2^%d} = 2^{%.2f}", n, log_funcs);
    if (isinf(funcs)) fprintf(fp, " (overflow)");
    else fprintf(fp, " = %.2e", funcs);
    fprintf(fp, "\n");

    /* Feasible fraction */
    double frac = shannon_feasible_fraction(n, S);
    double log_frac = shannon_feasible_fraction_log(n, S);
    fprintf(fp, "Feasible fraction (size <= S):\n");
    fprintf(fp, "  log2(fraction) = %.2f\n", log_frac);
    fprintf(fp, "  fraction = %.2e\n", frac);

    /* Largeness */
    double large = shannon_largeness_fraction(n, S);
    fprintf(fp, "Largeness (fraction with complexity > S):\n");
    fprintf(fp, "  fraction = %.10f\n", large);
    fprintf(fp, "  Is large? %s (needs >= 2^{-%d.n} = %.2e)\n",
            (large >= pow(2.0, -2.0 * n)) ? "YES" : "NO",
            (int)(2*n), pow(2.0, -2.0 * n));

    /* Constructiveness */
    double cost = shannon_constructiveness_cost(n, S);
    int constructive = shannon_is_constructive(n, S);
    fprintf(fp, "Constructiveness:\n");
    fprintf(fp, "  Check cost: %.2e\n", cost);
    fprintf(fp, "  Is constructive? %s (needs <= 2^{10n})\n",
            constructive ? "YES" : "NO");

    /* Natural property check */
    NaturalProperty np = shannon_natural_criteria_check(n, S);
    fprintf(fp, "\nNatural Property Verdict:\n");
    fprintf(fp, "  Constructive: %s\n", np.constructive ? "YES" : "NO");
    fprintf(fp, "  Large:        %s\n", np.large ? "YES" : "NO");
    fprintf(fp, "  Useful:       %s\n", np.useful ? "YES" : "NO");
    fprintf(fp, "  => %s\n",
            np.is_natural ? "NATURAL (barrier applies!)" : "NOT natural");
}

void shannon_print_table(int n_min, int n_max, int n_step,
                         int S_style, FILE *fp) {
    if (!fp) fp = stdout;
    if (n_min < 1) n_min = 1;
    if (n_max < n_min) n_max = n_min;
    if (n_step < 1) n_step = 1;

    fprintf(fp, "\n=== Shannon Counting Table ===\n\n");
    fprintf(fp, "%-5s %-12s %-14s %-14s %-12s %-12s %s\n",
            "n", "S(n)", "log2(circuits)", "log2(functions)",
            "frac_feasible", "frac_hard", "Natural?");
    fprintf(fp, "----- ------------ -------------- --------------- "
            "------------ ------------ --------\n");

    for (int n = n_min; n <= n_max; n += n_step) {
        int S;
        switch (S_style) {
            case 0: S = n; break;        /* linear */
            case 1: S = n * n; break;    /* quadratic (poly) */
            case 2: S = n * n * n; break; /* cubic */
            default: S = (int)pow(2.0, n/4.0); break; /* exponential */
        }

        double log_c = shannon_circuit_upper_bound_log(n, S);
        double log_f = shannon_function_count_log(n);
        double frac = shannon_feasible_fraction(n, S);
        double hard = shannon_largeness_fraction(n, S);
        NaturalProperty np = shannon_natural_criteria_check(n, S);

        fprintf(fp, "%-5d %-12d %-14.1f %-14.1f %-12.2e %-11.4f %s\n",
                n, S, log_c, log_f, frac, hard,
                np.is_natural ? "NATURAL" : "no");
    }
}
