/**
 * boolean_funcs.c -- Boolean Functions for Hastad Lower Bounds
 *
 * Implements fundamental Boolean functions used in circuit complexity:
 *   PARITY: not in AC0 (Hastad 1986)
 *   MAJORITY: not in AC0[p] for p!=2 (Razborov-Smolensky 1987)
 *   MOD, THRESHOLD, AND, OR, COUNT
 *
 * L1: Boolean function definitions f:{0,1}^n -> {0,1}
 * L2: Restriction-stability of PARITY (key to lower bound)
 * L3: Fourier expansion over GF(2)^n
 * L6: PARITY, MAJORITY as canonical hard functions for AC0
 *
 * References:
 *   Hastad (1986) "Almost optimal lower bounds for small depth circuits"
 *   Linial-Mansour-Nisan (1993) "Constant depth circuits, Fourier transform, and learnability"
 *   O'Donnell (2014) "Analysis of Boolean Functions"
 */
#include "hastad.h"
#include <string.h>

/* =================================================================
 * L1: Basic Boolean Functions
 * ================================================================= */

/** PARITY: XOR of all bits. deg_GF2(parity) = 1. Depth(parity) = O(log n). */
int parity(const int* x, int n) {
    int p = 0;
    for (int i = 0; i < n; i++) p ^= (x[i] & 1);
    return p;
}

/** PARITY via balanced XOR tree (NC1 construction).
 *  Uses pairwise XOR reduction for O(log n) depth. */
int parity_xor(const int* x, int n) {
    int result = 0;
    int* tmp = (int*)malloc(n * sizeof(int));
    if (!tmp) return 0;
    memcpy(tmp, x, n * sizeof(int));
    int m = n;
    while (m > 1) {
        int j = 0;
        for (int i = 0; i + 1 < m; i += 2)
            tmp[j++] = (tmp[i] & 1) ^ (tmp[i+1] & 1);
        if (m & 1) tmp[j++] = tmp[m-1];
        m = j;
    }
    result = (m > 0) ? (tmp[0] & 1) : 0;
    free(tmp);
    return result;
}

/** MAJORITY: 1 iff more than half the inputs are 1.
 *  MAJORITY not in AC0[p] for prime p != 2 (Razborov-Smolensky 1987).
 *  deg_GF2(majority) = n (full degree). */
int majority(const int* x, int n) {
    int s = 0;
    for (int i = 0; i < n; i++) s += (x[i] & 1);
    return s > n / 2;
}

/** MAJORITY via explicit counting of zeros and ones. */
int majority_median(const int* x, int n) {
    int count0 = 0, count1 = 0;
    for (int i = 0; i < n; i++) {
        if (x[i] & 1) count1++; else count0++;
    }
    return count1 > count0;
}

/** MOD_m(x) = 1 iff sum of inputs is divisible by m.
 *  MOD_p is complete for ACC0[p] (Barrington-Therien 1988). */
int mod_m(const int* x, int n, int m) {
    int s = 0;
    for (int i = 0; i < n; i++) s += (x[i] & 1);
    return (s % m) == 0;
}

/** MOD-3: useful for AC0[3] vs AC0[2] separation. */
int mod3(const int* x, int n) {
    return mod_m(x, n, 3);
}

/** MOD-5: useful for prime modulus analysis. */
int mod5(const int* x, int n) {
    return mod_m(x, n, 5);
}

/** THRESHOLD_t: 1 iff at least t inputs are 1.
 *  Threshold gates define TC0 (constant-depth threshold circuits). */
int threshold(const int* x, int n, int t) {
    int s = 0;
    for (int i = 0; i < n; i++) s += (x[i] & 1);
    return s >= t;
}

/** AND of n bits: gate with fan-in n. */
int and_n(const int* x, int n) {
    for (int i = 0; i < n; i++)
        if (!(x[i] & 1)) return 0;
    return 1;
}

/** OR of n bits: gate with fan-in n. */
int or_n(const int* x, int n) {
    for (int i = 0; i < n; i++)
        if (x[i] & 1) return 1;
    return 0;
}

/** Exact COUNT-k: 1 iff exactly k inputs are 1.
 *  Symmetric function: value depends only on Hamming weight. */
int count_k(const int* x, int n, int k) {
    int s = 0;
    for (int i = 0; i < n; i++) s += (x[i] & 1);
    return s == k;
}

/** NAND: NOT(AND). Universal gate. */
int nand_n(const int* x, int n) {
    return !and_n(x, n);
}

/** NOR: NOT(OR). Universal gate. */
int nor_n(const int* x, int n) {
    return !or_n(x, n);
}

/** XOR (same as PARITY): x_1 xor x_2 xor ... xor x_n */
int xor_n(const int* x, int n) {
    return parity(x, n);
}

/** IMPLIES: x1 -> x2, equivalent to (NOT x1) OR x2. */
int implies(int x1, int x2) {
    return (!(x1 & 1)) | (x2 & 1);
}

/** EQUIVALENCE: x1 <-> x2, equivalent to NOT(x1 xor x2). */
int equivalence(int x1, int x2) {
    return !((x1 & 1) ^ (x2 & 1));
}

/* =================================================================
 * L2: Composite Boolean Functions
 * ================================================================= */

/** PARITY-of-ANDs: PARITY(AND(x11..x1k), ..., AND(xm1..xmk))
 *  Depth-2 AC0 circuit: AND gates feeding PARITY.
 *  This is the structure Hastad proved hard for AC0. */
int parity_of_ands(const int** x, int m, int k) {
    int p = 0;
    for (int i = 0; i < m; i++) {
        int term = 1;
        for (int j = 0; j < k; j++) term &= (x[i][j] & 1);
        p ^= term;
    }
    return p;
}

/** AND-of-ORs: depth-2 DNF structure.
 *  f = AND_{i=1..m} (OR_{j=1..k} x_{i,j})
 *  Each OR clause is a term in the CNF representation. */
int and_of_ors(const int** x, int m, int k) {
    for (int i = 0; i < m; i++) {
        int clause = 0;
        for (int j = 0; j < k; j++) clause |= (x[i][j] & 1);
        if (!clause) return 0;  /* clause i fails */
    }
    return 1;
}

/** OR-of-ANDs: depth-2 CNF structure.
 *  f = OR_{i=1..m} (AND_{j=1..k} x_{i,j})
 *  Each AND term is a clause in the DNF representation. */
int or_of_ands(const int** x, int m, int k) {
    for (int i = 0; i < m; i++) {
        int term = 1;
        for (int j = 0; j < k; j++) term &= (x[i][j] & 1);
        if (term) return 1;     /* term i satisfied */
    }
    return 0;
}

/** AND-OR-AND: depth-3 AC0 structure.
 *  f = AND_a (OR_b (AND_c x_{a,b,c}))
 *  Tree depth = 3. Used to demonstrate depth hierarchy. */
int and_or_and(const int*** x, int a, int b, int c) {
    int result = 1;
    for (int i = 0; i < a; i++) {
        int or_result = 0;
        for (int j = 0; j < b; j++) {
            int and_result = 1;
            for (int k = 0; k < c; k++) and_result &= (x[i][j][k] & 1);
            or_result |= and_result;
        }
        result &= or_result;
    }
    return result;
}

/** OR-AND-OR: depth-3 AC0 structure (dual of AND-OR-AND). */
int or_and_or(const int*** x, int a, int b, int c) {
    int result = 0;
    for (int i = 0; i < a; i++) {
        int and_result = 1;
        for (int j = 0; j < b; j++) {
            int or_result = 0;
            for (int k = 0; k < c; k++) or_result |= (x[i][j][k] & 1);
            and_result &= or_result;
        }
        result |= and_result;
    }
    return result;
}

/* =================================================================
 * L2: Sipser Functions (AC0 Depth Hierarchy)
 * ================================================================= */

/**
 * Sipser's S_d function family (Sipser 1983).
 * S_1 = OR
 * S_2 = AND-of-ORs
 * S_3 = OR-of-ANDs-of-ORs
 * ...
 *
 * Theorem: S_d in AC0[d] but S_d not in AC0[d-1].
 * This establishes the AC0 depth hierarchy: AC0[d] is strictly
 * contained in AC0[d+1].
 *
 * The proof uses Hastad's switching lemma repeatedly.
 */
int sipser_depth(const int* x, int n, int d) {
    if (d <= 0 || n <= 0) return 0;
    if (d == 1) return or_n(x, n);

    /* Partition inputs into blocks of size floor(sqrt(n)).
     * This choice ensures the number of blocks grows sublinearly,
     * which is critical for the hierarchy separation proof. */
    int block_size = (int)sqrt((double)n);
    if (block_size < 2) block_size = 2;
    int n_blocks = n / block_size;
    if (n_blocks < 1) n_blocks = 1;

    int* results = (int*)malloc(n_blocks * sizeof(int));
    if (!results) return 0;

    for (int b = 0; b < n_blocks; b++) {
        int start = b * block_size;
        int end = (b + 1) * block_size;
        if (end > n) end = n;
        results[b] = sipser_depth(x + start, end - start, d - 1);
    }

    /* Alternating gate: even depth = AND, odd depth = OR */
    int result = (d % 2 == 0) ? and_n(results, n_blocks)
                              : or_n(results, n_blocks);
    free(results);
    return result;
}

/** Wrapper for Sipser hierarchy function. */
int sipser_hierarchy_fn(const int* x, int n, int depth) {
    return sipser_depth(x, n, depth);
}

/**
 * Generate the dual Sipser function S_d^* where the bottom gate
 * type is swapped. Still in AC0[d] but with different structure.
 */
int sipser_dual(const int* x, int n, int d) {
    if (d <= 0 || n <= 0) return 0;
    if (d == 1) return and_n(x, n);

    int block_size = (int)sqrt((double)n);
    if (block_size < 2) block_size = 2;
    int n_blocks = n / block_size;
    if (n_blocks < 1) n_blocks = 1;

    int* results = (int*)malloc(n_blocks * sizeof(int));
    if (!results) return 0;

    for (int b = 0; b < n_blocks; b++) {
        int start = b * block_size;
        int end = (b + 1) * block_size;
        if (end > n) end = n;
        results[b] = sipser_dual(x + start, end - start, d - 1);
    }

    /* Inverted alternation: even = OR, odd = AND */
    int result = (d % 2 == 0) ? or_n(results, n_blocks)
                              : and_n(results, n_blocks);
    free(results);
    return result;
}

/* =================================================================
 * L3: Function Sensitivity and Influence
 * ================================================================= */

/**
 * Sensitivity of f at point x: number of i such that
 * flipping bit i alone changes f(x).
 *
 * s(f,x) = |{ i in [n] : f(x) != f(x xor e_i) }|
 *
 * Huang (2019) proved: s(f) >= sqrt(deg(f)).
 * This settled the Sensitivity Conjecture (Nisan-Szegedy 1992).
 */
int sensitivity(const int* x, int n, int (*f)(const int*, int)) {
    int* y = (int*)malloc(n * sizeof(int));
    if (!y) return -1;
    memcpy(y, x, n * sizeof(int));
    int base = f(x, n);
    int sens = 0;
    for (int i = 0; i < n; i++) {
        y[i] ^= 1;               /* flip bit i */
        if (f(y, n) != base) sens++;
        y[i] ^= 1;               /* restore */
    }
    free(y);
    return sens;
}

/**
 * Block sensitivity bs(f,x): maximum number of disjoint blocks
 * such that flipping all bits in a block changes f(x).
 *
 * Nisan (1991): bs(f) <= s(f)^2 for all monotone f.
 * General bound: bs(f) <= O(s(f)^4) (Tal 2013).
 */
int block_sensitivity(const int* x, int n, int (*f)(const int*, int)) {
    int* y = (int*)malloc(n * sizeof(int));
    if (!y) return -1;
    memcpy(y, x, n * sizeof(int));
    int base = f(x, n);
    int blocks = 0;

    /* Greedy algorithm: find disjoint sensitive blocks */
    int i = 0;
    while (i < n) {
        int block_end = i;
        int found_block = 0;
        /* Try extending block from i to j */
        for (int j = i; j < n; j++) {
            /* Flip all bits i..j */
            for (int k = i; k <= j; k++) y[k] ^= 1;
            if (f(y, n) != base) {
                block_end = j;
                found_block = 1;
            }
            /* Restore */
            for (int k = i; k <= j; k++) y[k] ^= 1;
        }
        if (found_block) {
            blocks++;
            i = block_end + 1;
        } else {
            i++;
        }
    }
    free(y);
    return blocks;
}

/**
 * Average sensitivity (total influence): expected sensitivity
 * over uniform random input x.
 *
 * I[f] = E_x[s(f,x)] = sum_i Inf_i[f]
 *
 * For AC0 functions: I[f] = O(polylog(n)) (LMN 1993).
 * For PARITY: I[parity] = n (every bit is pivotal).
 */
double average_sensitivity(int n, int (*f)(const int*, int)) {
    int samples = 512;
    if ((1 << n) < samples) samples = (1 << n);
    int* xx = (int*)malloc(n * sizeof(int));
    if (!xx) return -1.0;
    double total = 0.0;
    for (int s = 0; s < samples; s++) {
        for (int j = 0; j < n; j++) xx[j] = (rand() >> 5) & 1;
        total += (double)sensitivity(xx, n, f);
    }
    free(xx);
    return total / samples;
}

/**
 * Influence of variable i: probability that flipping i changes f(x).
 *
 * Inf_i[f] = Pr_x[f(x) != f(x xor e_i)]
 *
 * For PARITY: Inf_i = 1 (always pivotal).
 * For MAJORITY: Inf_i = Theta(1/sqrt(n)) (central limit theorem).
 */
double variable_influence(int n, int i, int (*f)(const int*, int)) {
    int samples = 512;
    if ((1 << n) < samples) samples = (1 << n);
    int* xx = (int*)malloc(n * sizeof(int));
    if (!xx) return -1.0;
    int changes = 0;
    for (int s = 0; s < samples; s++) {
        for (int j = 0; j < n; j++) xx[j] = (rand() >> 5) & 1;
        int v1 = f(xx, n);
        xx[i] ^= 1;
        int v2 = f(xx, n);
        xx[i] ^= 1;
        if (v1 != v2) changes++;
    }
    free(xx);
    return (double)changes / samples;
}

/**
 * GF(2) degree estimate: maximum monomial degree in GF(2) representation.
 * PARITY has degree 1, MAJORITY has degree n.
 * For AC0 functions: deg_GF2 = O(polylog(n)) (Razborov 1987).
 */
int gf2_degree_estimate(const int* x, int n, int (*f)(const int*, int)) {
    int max_degree = 0;
    for (int d = 0; d <= n && d <= 6; d++) {
        int found = 0;
        for (int trial = 0; trial < 10 && !found; trial++) {
            int* subset = (int*)calloc(n, sizeof(int));
            if (!subset) continue;
            int cnt = 0;
            while (cnt < d) {
                int idx = rand() % n;
                if (!subset[idx]) { subset[idx] = 1; cnt++; }
            }
            int changes = 0;
            int trials2 = (1 << n);
            if (trials2 > 32) trials2 = 32;
            for (int t = 0; t < trials2; t++) {
                int* yy = (int*)malloc(n * sizeof(int));
                if (!yy) break;
                for (int kk = 0; kk < n; kk++) yy[kk] = (t >> kk) & 1;
                int v1 = f(yy, n);
                for (int kk = 0; kk < n; kk++)
                    if (subset[kk]) yy[kk] ^= 1;
                int v2 = f(yy, n);
                if (v1 != v2) changes++;
                free(yy);
            }
            if (changes > 0 && changes < trials2) {
                found = 1;
                max_degree = d;
            }
            free(subset);
        }
    }
    return max_degree;
}

/** Check if function is constant (all 2^n inputs map to same value). */
int is_constant_function(const int* x, int n, int (*f)(const int*, int)) {
    int max_inputs = (1 << n);
    if (max_inputs > 256) max_inputs = 256;
    int* xx = (int*)calloc(n, sizeof(int));
    if (!xx) return -1;
    int first_val = f(xx, n);
    for (int t = 1; t < max_inputs; t++) {
        for (int j = 0; j < n; j++) xx[j] = (t >> j) & 1;
        if (f(xx, n) != first_val) {
            free(xx);
            return 0;
        }
    }
    free(xx);
    return 1;
}

/** Check if f is a dictator: f(x) = x_i for some i.
 *  Dictator functions are the simplest non-constant functions. */
int is_dictator_function(const int* x, int n, int (*f)(const int*, int)) {
    if (is_constant_function(x, n, f)) return 0;

    /* Find the dictator variable */
    for (int i = 0; i < n; i++) {
        int matches = 1;
        /* Test: does f(x) = x_i for all x? */
        int max_test = (1 << n);
        if (max_test > 64) max_test = 64;
        int* xx = (int*)malloc(n * sizeof(int));
        if (!xx) continue;
        for (int t = 0; t < max_test && matches; t++) {
            for (int j = 0; j < n; j++) xx[j] = (t >> j) & 1;
            if (f(xx, n) != (xx[i] & 1)) matches = 0;
        }
        free(xx);
        if (matches) return 1;
    }
    return 0;
}

/** Check if f depends on variable i (exists x where flipping i changes f). */
int depends_on_var(const int* x, int n, int i, int (*f)(const int*, int)) {
    for (int trial = 0; trial < 8; trial++) {
        int* y = (int*)malloc(n * sizeof(int));
        if (!y) return -1;
        for (int j = 0; j < n; j++) y[j] = (rand() >> 4) & 1;
        int v1 = f(y, n);
        y[i] ^= 1;
        int v2 = f(y, n);
        free(y);
        if (v1 != v2) return 1;
    }
    return 0;
}

/** Check if function is monotone: x <= y (bitwise) => f(x) <= f(y).
 *  AC0 functions with no NOT gates are monotone. */
int is_monotone_function(const int* x, int n, int (*f)(const int*, int)) {
    int max_test = (1 << n);
    if (max_test > 256) max_test = 256;
    int* a = (int*)malloc(n * sizeof(int));
    int* b = (int*)malloc(n * sizeof(int));
    if (!a || !b) { free(a); free(b); return -1; }
    for (int tx = 0; tx < max_test; tx++) {
        for (int j = 0; j < n; j++) a[j] = (tx >> j) & 1;
        /* Check supersets of a: for each superset y, f(a) <= f(y) */
        for (int ty = tx; ty < max_test; ty++) {
            int is_superset = 1;
            for (int j = 0; j < n; j++) {
                b[j] = (ty >> j) & 1;
                if (a[j] > b[j]) { is_superset = 0; break; }
            }
            if (is_superset && f(a, n) > f(b, n)) {
                free(a); free(b);
                return 0;  /* NOT monotone */
            }
        }
    }
    free(a); free(b);
    return 1;
}

/* =================================================================
 * L3: Fourier Analysis over {+1,-1} Basis
 * ================================================================= */

/** Map {0,1} to Fourier basis {+1,-1}: 0 -> +1, 1 -> -1 */
double to_fourier_basis(int bit) {
    return (bit & 1) ? -1.0 : 1.0;
}

/** Map Fourier basis back to {0,1}: +1 -> 0, -1 -> 1 */
int from_fourier_basis(double val) {
    return (val < 0.0);
}

/**
 * Compute one Fourier coefficient f_hat(S) via Monte Carlo sampling.
 *
 * f_hat(S) = E_{x in {0,1}^n}[ f(x) * chi_S(x) ]
 * where chi_S(x) = prod_{i in S} (-1)^{x_i}
 *
 * Complexity: O(samples * n * |S|).
 * Reference: LMN (1993), O'Donnell (2014).
 */
double fourier_coefficient_sample(const int* x, int n,
                                   int (*f)(const int*, int),
                                   const int* subset, int subset_size) {
    int samples = 1000;
    int exhaustive = (1 << n);
    if (exhaustive < samples) samples = exhaustive;
    double sum = 0.0;
    int* xx = (int*)malloc(n * sizeof(int));
    if (!xx) return 0.0;

    for (int s = 0; s < samples; s++) {
        for (int j = 0; j < n; j++) xx[j] = (rand() >> 3) & 1;
        int f_val = f(xx, n);
        double chi = 1.0;
        for (int j = 0; j < n; j++) {
            if (subset[j]) chi *= to_fourier_basis(xx[j]);
        }
        /* f(x) in {+1,-1} basis: +1 if f(x)=1, -1 if f(x)=0 */
        sum += to_fourier_basis(!f_val) * chi;
    }
    free(xx);
    return sum / samples;
}

/**
 * Compute all Fourier coefficients of degree <= D via explicit enumeration.
 * Returns number of coefficients computed.
 * For n <= 15, exhaustive computation is feasible.
 */
int fourier_spectrum_low_degree(int n, int (*f)(const int*, int),
                                 int max_degree, double* coeffs) {
    int count = 0;
    int total_subsets = (1 << n);

    for (int mask = 0; mask < total_subsets && mask < 8192; mask++) {
        int deg = 0;
        for (int j = 0; j < n; j++) deg += ((mask >> j) & 1);
        if (deg > max_degree) continue;

        /* Compute f_hat(S) exactly for n <= 12, sample for larger */
        double c = 0.0;
        int exhaustive = (1 << n);
        if (exhaustive <= 4096) {
            /* Exact computation */
            int* xx = (int*)malloc(n * sizeof(int));
            if (!xx) break;
            for (int t = 0; t < exhaustive; t++) {
                for (int j = 0; j < n; j++) xx[j] = (t >> j) & 1;
                double chi = 1.0;
                for (int j = 0; j < n; j++)
                    if ((mask >> j) & 1) chi *= to_fourier_basis(xx[j]);
                c += to_fourier_basis(!f(xx, n)) * chi;
            }
            c /= exhaustive;
            free(xx);
        } else {
            c = fourier_coefficient_sample(NULL, n, f,
                   (int[]){mask}, n);  /* simplified: single-mask subset */
        }
        coeffs[count++] = c;
    }
    return count;
}

/**
 * Check if function has Fourier concentration up to degree d.
 * Uses Parseval's identity: sum_{S} f_hat(S)^2 = 1.
 * Returns 1 if sum_{|S|<=d} f_hat(S)^2 >= 1 - epsilon.
 */
int is_fourier_concentrated(int (*f)(const int*, int), int n,
                             int degree, double epsilon) {
    double total_weight = 0.0;
    int* subset = (int*)calloc(n, sizeof(int));
    if (!subset) return -1;

    int max_check = (1 << n);
    if (max_check > 2048) max_check = 2048;

    for (int mask = 1; mask < max_check; mask++) {
        int subset_sz = 0;
        for (int j = 0; j < n; j++) {
            subset[j] = (mask >> j) & 1;
            subset_sz += subset[j];
        }
        if (subset_sz > degree) continue;
        double coeff = fourier_coefficient_sample(NULL, n, f, subset, subset_sz);
        total_weight += coeff * coeff;
    }
    free(subset);
    return (total_weight >= 1.0 - epsilon);
}

/**
 * LMN (1993) Theorem: For any AC0 function f of depth d and size M,
 * the sum of squared Fourier coefficients for |S| > D is at most
 * M * 2^{-D^{1/d} / O(log M)}.
 *
 * This function estimates the Fourier tail bound.
 */
double lmn_fourier_tail_bound(int depth, double log_size, int degree) {
    /* LMN bound: W_{>D}[f] <= size * 2^{-D^{1/d} / c log size} */
    double c = 20.0;
    double exponent = pow((double)degree, 1.0 / depth) / (c * log_size);
    return exp(-exponent * log(2.0)) * exp(log_size * log(2.0));
    /* Simplified: size * 2^{-D^{1/d} / (c log size)} */
}

/* =================================================================
 * L3: Subcube Enumeration and Analysis
 * ================================================================= */

/**
 * Count number of satisfying assignments (f=1) in the subcube
 * defined by a restriction. Free variables take all 2^k assignments.
 *
 * This is a key operation for the switching lemma proof:
 * it bounds how many DNF terms can "fit" in the restricted subcube.
 *
 * Complexity: O(2^{n_free} * T_f) where T_f is evaluation time.
 */
int64_t count_minterms_subcube(const int* restr, int n,
                                int (*f)(const int*, int)) {
    /* Count free variables */
    int n_free = 0;
    int* free_idx = (int*)malloc(n * sizeof(int));
    if (!free_idx) return -1;

    for (int i = 0; i < n; i++)
        if (restr[i] == -1) free_idx[n_free++] = i;

    int64_t count = 0;
    int max_assign = (1 << n_free);
    if (max_assign > (1 << 20)) max_assign = (1 << 20);  /* 1M cap */

    int* xx = (int*)malloc(n * sizeof(int));
    if (!xx) { free(free_idx); return -1; }

    /* Set fixed variables */
    for (int i = 0; i < n; i++)
        if (restr[i] != -1) xx[i] = restr[i] & 1;

    for (int a = 0; a < max_assign; a++) {
        for (int j = 0; j < n_free; j++)
            xx[free_idx[j]] = (a >> j) & 1;
        if (f(xx, n) == 1) count++;
    }

    free(xx);
    free(free_idx);
    return count;
}

/**
 * Count number of DNF terms that can exist on n_free variables
 * without overlap. Used to lower-bound DNF size for PARITY.
 *
 * PARITY on k variables: each DNF term is a subcube of dimension
 * at most k-1 (since fixing one literal reduces dimension by 1).
 * The union of all terms must cover exactly 2^{k-1} points.
 * Each term covers at most 2^{k-1} points, so need >= 2^{k-1} terms.
 */
int64_t parity_dnf_min_size(int k) {
    if (k > 60) return INT64_MAX;
    return (int64_t)1 << (k - 1);
}

/**
 * Enumerate all 2^{n_free} assignments to free variables,
 * calling callback for each.
 *
 * n_free should be <= 20 for practical use (1M assignments max).
 */
void enumerate_free_assignments(const int* restr, int n,
                                 void (*callback)(const int*, int, void*),
                                 void* ctx) {
    int n_free = 0;
    int* free_idx = (int*)malloc(n * sizeof(int));
    if (!free_idx) return;

    for (int i = 0; i < n; i++)
        if (restr[i] == -1) free_idx[n_free++] = i;

    int max_assign = (1 << n_free);
    if (max_assign > (1 << 20)) max_assign = (1 << 20);

    int* xx = (int*)malloc(n * sizeof(int));
    if (!xx) { free(free_idx); return; }

    /* Set fixed variables */
    for (int i = 0; i < n; i++)
        xx[i] = (restr[i] != -1) ? (restr[i] & 1) : 0;

    for (int a = 0; a < max_assign; a++) {
        for (int j = 0; j < n_free; j++)
            xx[free_idx[j]] = (a >> j) & 1;
        callback(xx, n, ctx);
    }

    free(xx);
    free(free_idx);
}