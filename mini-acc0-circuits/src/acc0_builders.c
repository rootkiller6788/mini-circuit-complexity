/* acc0_builders.c -- ACC0 Circuit Builders for Canonical Functions
 * ================================================================
 * L6: Canonical Problems -- Building ACC0 circuits for classic functions.
 * L5: Algorithms -- Systematic construction of circuit families.
 *
 * Canonical functions:
 * - PARITY (in ACC0[2], not in ACC0[p] for odd p?)
 * - MOD_m for various m
 * - MAJORITY (unknown if in ACC0, buildable in TC0/AC0)
 * - THRESHOLD_k
 * - SYMMETRIC functions
 * - EXACT-k (exactly k ones)
 * - COMPARISON (x > y)
 * - ADDITION (binary addition)
 *
 * References:
 * - Vollmer (1999), Ch.3-4
 * - Arora & Barak (2009), Ch.14
 */

#include "acc0.h"
#include "acc0_gates.h"
#include <assert.h>

/* ================================================================
 * PARITY Circuit: uses MOD2 gate (O(1) gates with MOD2)
 * ================================================================ */

ACC0Circuit* acc0_build_parity_circuit(int n) {
    /* PARITY(x1..xn) = MOD2(x1..xn).
     * With MOD2 gate: 1 gate + n inputs = n+1 gates.
     * Without MOD2: requires exponential size in AC0.
     */
    ACC0Circuit *c = acc0_circuit_create(n + 10);
    if (!c) return NULL;
    int *ins = (int*)malloc((size_t)n * sizeof(int));
    for (int i = 0; i < n; i++) ins[i] = acc0_add_input(c);
    int mod2 = acc0_add_mod(c, 2);
    if (mod2 < 0) { free(ins); acc0_circuit_free(c); return NULL; }
    acc0_set_fanin(c, mod2, ins, n);
    int out[1] = { mod2 };
    acc0_set_outputs(c, out, 1);
    free(ins);
    return c;
}

/* ================================================================
 * MOD_m Circuit: single MOD_m gate over all inputs
 * ================================================================ */

ACC0Circuit* acc0_build_mod_circuit(int n, int modulus) {
    /* MOD_m(x1..xn) = 1 iff sum(xi) = 0 (mod m).
     * Single MOD_m gate over all inputs. */
    ACC0Circuit *c = acc0_circuit_create(n + 10);
    if (!c) return NULL;
    int *ins = (int*)malloc((size_t)n * sizeof(int));
    for (int i = 0; i < n; i++) ins[i] = acc0_add_input(c);
    int mod_gate;
    if (modulus == 2 || modulus == 3 || modulus == 5 ||
        modulus == 7 || modulus == 11 || modulus == 13) {
        mod_gate = acc0_add_mod(c, modulus);
    } else {
        mod_gate = acc0_add_mod_composite(c, modulus);
    }
    if (mod_gate < 0) { free(ins); acc0_circuit_free(c); return NULL; }
    acc0_set_fanin(c, mod_gate, ins, n);
    int out[1] = { mod_gate };
    acc0_set_outputs(c, out, 1);
    free(ins);
    return c;
}

/* ================================================================
 * AND-of-MODs Circuit: MOD_m1 AND MOD_m2
 * ================================================================ */

ACC0Circuit* acc0_build_and_of_mods(int n, int m1, int m2) {
    /* Compute (sum xi mod m1 == 0) AND (sum xi mod m2 == 0).
     * This is a canonical ACC0 function demonstrating
     * the power of multiple moduli. */
    ACC0Circuit *c = acc0_circuit_create(2 * n + 20);
    if (!c) return NULL;
    int *ins = (int*)malloc((size_t)n * sizeof(int));
    for (int i = 0; i < n; i++) ins[i] = acc0_add_input(c);

    int mod1 = (m1 <= 13 && m1 != 4 && m1 != 6 && m1 != 8 && m1 != 9 &&
                m1 != 10 && m1 != 12) ? acc0_add_mod(c, m1)
                                      : acc0_add_mod_composite(c, m1);
    int mod2 = (m2 <= 13 && m2 != 4 && m2 != 6 && m2 != 8 && m2 != 9 &&
                m2 != 10 && m2 != 12) ? acc0_add_mod(c, m2)
                                      : acc0_add_mod_composite(c, m2);

    acc0_set_fanin(c, mod1, ins, n);
    acc0_set_fanin(c, mod2, ins, n);

    int and_gate = acc0_add_and(c, mod1, mod2);
    int out[1] = { and_gate };
    acc0_set_outputs(c, out, 1);
    free(ins);
    return c;
}

/* ================================================================
 * ACC0[3] Circuit for PARITY (educationally)
 *
 * PARITY cannot be computed by MOD3 alone.
 * But can ACC0[3] compute PARITY? This is an open question!
 * We build the best-known approximation.
 * ================================================================ */

ACC0Circuit* acc0_build_parity_via_mod3(int n) {
    /* Attempt to compute PARITY using MOD3, AND, OR, NOT.
     * This demonstrates the challenge: PARITY requires MOD2.
     * MOD3 + AND + OR + NOT = ACC0[3] MAY or MAY NOT compute PARITY.
     *
     * Known: PARITY is in AC0[2] (trivially with MOD2 gate).
     * Known: PARITY is NOT in AC0 alone (Hastad 1986).
     * Open: PARITY in ACC0[3]?
     *
     * We build: NOT(MOD3(x)) which is NOT equivalent to PARITY,
     * but demonstrates the structure. */
    ACC0Circuit *c = acc0_circuit_create(n + 10);
    if (!c) return NULL;
    int *ins = (int*)malloc((size_t)n * sizeof(int));
    for (int i = 0; i < n; i++) ins[i] = acc0_add_input(c);
    int mod3 = acc0_add_mod(c, 3);
    acc0_set_fanin(c, mod3, ins, n);
    int not_gate = acc0_add_not(c, mod3);
    int out[1] = { not_gate };
    acc0_set_outputs(c, out, 1);
    free(ins);
    return c;
}

/* ================================================================
 * MAJORITY Circuit (using AC0, exponential size)
 * ================================================================ */

ACC0Circuit* acc0_build_majority_dnf(int n) {
    /* Build MAJORITY using DNF (OR of ANDs).
     * Size: O(2^n). NOT constant-depth in practice.
     * This shows why MAJORITY is NOT in AC0.
     *
     * In TC0, MAJORITY uses O(1) THRESHOLD gates.
     * In ACC0: status unknown. */
    ACC0Circuit *c = acc0_circuit_create(1 << (n + 1));
    if (!c) return NULL;

    int *ins = (int*)malloc((size_t)n * sizeof(int));
    for (int i = 0; i < n; i++) ins[i] = acc0_add_input(c);

    int k = n / 2 + 1;
    int nterms = 0;
    int max_terms = 1000; /* practical limit */
    int *and_terms = (int*)malloc((size_t)max_terms * sizeof(int));

    /* Enumerate subsets of size >= k */
    for (int mask = 1; mask < (1 << n) && nterms < max_terms; mask++) {
        int cnt = 0;
        for (int i = 0; i < n; i++)
            if ((mask >> i) & 1) cnt++;
        if (cnt < k) continue;

        /* Create AND term */
        int prev = -1;
        for (int i = 0; i < n; i++) {
            if ((mask >> i) & 1) {
                if (prev == -1) prev = i;
                else prev = acc0_add_and(c, prev, i);
            }
        }
        if (prev >= 0) and_terms[nterms++] = prev;
    }

    /* OR all AND terms */
    int or_result = and_terms[0];
    for (int i = 1; i < nterms; i++)
        or_result = acc0_add_or(c, or_result, and_terms[i]);

    int out[1] = { or_result };
    acc0_set_outputs(c, out, 1);
    free(ins);
    free(and_terms);
    return c;
}

/* ================================================================
 * THRESHOLD_k: 1 iff sum(xi) >= k
 * Built using AND/OR gates (AC0, exponential size)
 * ================================================================ */

ACC0Circuit* acc0_build_threshold_circuit(int n, int k) {
    /* DNF construction similar to MAJORITY but threshold k. */
    ACC0Circuit *c = acc0_circuit_create(1 << (n + 1));
    if (!c) return NULL;

    int *ins = (int*)malloc((size_t)n * sizeof(int));
    for (int i = 0; i < n; i++) ins[i] = acc0_add_input(c);

    int nterms = 0;
    int max_terms = 500;
    int *and_terms = (int*)malloc((size_t)max_terms * sizeof(int));

    for (int mask = 1; mask < (1 << n) && nterms < max_terms; mask++) {
        int cnt = 0;
        for (int i = 0; i < n; i++)
            if ((mask >> i) & 1) cnt++;
        if (cnt < k) continue;

        int prev = -1;
        for (int i = 0; i < n; i++) {
            if ((mask >> i) & 1) {
                if (prev == -1) prev = i;
                else prev = acc0_add_and(c, prev, i);
            }
        }
        if (prev >= 0) and_terms[nterms++] = prev;
    }

    int or_result = and_terms[0];
    for (int i = 1; i < nterms; i++)
        or_result = acc0_add_or(c, or_result, and_terms[i]);

    int out[1] = { or_result };
    acc0_set_outputs(c, out, 1);
    free(ins);
    free(and_terms);
    return c;
}

/* ================================================================
 * EXACTLY-k: 1 iff exactly k inputs are 1
 * ================================================================ */

ACC0Circuit* acc0_build_exactly_k(int n, int k) {
    /* EXACTLY(k) = THRESHOLD(k) AND NOT(THRESHOLD(k+1)) */
    ACC0Circuit *c = acc0_circuit_create(1 << (n + 2));
    if (!c) return NULL;

    int *ins = (int*)malloc((size_t)n * sizeof(int));
    for (int i = 0; i < n; i++) ins[i] = acc0_add_input(c);

    /* Build THRESHOLD(k) using DNF */
    int max_terms = 300;
    int *ge_k_terms = (int*)malloc((size_t)max_terms * sizeof(int));
    int *ge_kp1_terms = (int*)malloc((size_t)max_terms * sizeof(int));
    int n_gek = 0, n_gekp1 = 0;

    for (int mask = 1; mask < (1 << n) &&
         n_gek < max_terms && n_gekp1 < max_terms; mask++) {
        int cnt = 0;
        for (int i = 0; i < n; i++)
            if ((mask >> i) & 1) cnt++;

        if (cnt >= k && n_gek < max_terms) {
            int prev = -1;
            for (int i = 0; i < n; i++) {
                if ((mask >> i) & 1) {
                    prev = (prev == -1) ? i : acc0_add_and(c, prev, i);
                }
            }
            if (prev >= 0) ge_k_terms[n_gek++] = prev;
        }

        if (cnt >= k + 1 && n_gekp1 < max_terms) {
            int prev = -1;
            for (int i = 0; i < n; i++) {
                if ((mask >> i) & 1) {
                    prev = (prev == -1) ? i : acc0_add_and(c, prev, i);
                }
            }
            if (prev >= 0) ge_kp1_terms[n_gekp1++] = prev;
        }
    }

    /* OR the terms */
    int ge_k = ge_k_terms[0];
    for (int i = 1; i < n_gek; i++)
        ge_k = acc0_add_or(c, ge_k, ge_k_terms[i]);

    int ge_kp1 = ge_kp1_terms[0];
    for (int i = 1; i < n_gekp1; i++)
        ge_kp1 = acc0_add_or(c, ge_kp1, ge_kp1_terms[i]);

    int not_ge_kp1 = acc0_add_not(c, ge_kp1);
    int exactly_k = acc0_add_and(c, ge_k, not_ge_kp1);

    int out[1] = { exactly_k };
    acc0_set_outputs(c, out, 1);
    free(ins);
    free(ge_k_terms);
    free(ge_kp1_terms);
    return c;
}

/* ================================================================
 * COMPARISON Circuit: x > y (for two n-bit numbers)
 * ================================================================ */

ACC0Circuit* acc0_build_comparison(int nbits) {
    /* Compare two n-bit numbers X = x_{n-1}..x_0 and Y = y_{n-1}..y_0.
     * X > Y iff exists i: x_i > y_i AND for all j > i: x_j = y_j.
     *
     * With 2n inputs: first n for X, next n for Y.
     * Output: 1 iff X > Y. */
    int n = 2 * nbits;
    ACC0Circuit *c = acc0_circuit_create(n * n + 100);
    if (!c) return NULL;

    int *ins = (int*)malloc((size_t)n * sizeof(int));
    for (int i = 0; i < n; i++) ins[i] = acc0_add_input(c);

    /* Build X > Y circuit:
     * OR over i of [ x_i AND NOT(y_i) AND (AND_{j=i+1..n-1} (x_j XNOR y_j)) ] */
    int *or_terms = (int*)malloc((size_t)nbits * sizeof(int));
    int nor_terms = 0;

    for (int i = 0; i < nbits && nor_terms < nbits; i++) {
        int xi = ins[i];
        int yi = ins[nbits + i];
        int xi_gt_yi = acc0_add_and(c, xi, acc0_add_not(c, yi));

        /* Build XNOR chain for bits > i */
        int xnor_chain = -1;
        for (int j = i + 1; j < nbits; j++) {
            int xj = ins[j];
            int yj = ins[nbits + j];
            int xnor_j;
            /* XNOR = (x AND y) OR (NOT(x) AND NOT(y)) */
            {
                int both1 = acc0_add_and(c, xj, yj);
                int both0 = acc0_add_and(c, acc0_add_not(c, xj),
                                            acc0_add_not(c, yj));
                xnor_j = acc0_add_or(c, both1, both0);
            }
            if (xnor_chain == -1) xnor_chain = xnor_j;
            else xnor_chain = acc0_add_and(c, xnor_chain, xnor_j);
        }

        if (xnor_chain == -1)
            or_terms[nor_terms++] = xi_gt_yi;
        else
            or_terms[nor_terms++] = acc0_add_and(c, xi_gt_yi, xnor_chain);
    }

    int result = or_terms[0];
    for (int i = 1; i < nor_terms; i++)
        result = acc0_add_or(c, result, or_terms[i]);

    int out[1] = { result };
    acc0_set_outputs(c, out, 1);
    free(ins);
    free(or_terms);
    return c;
}

/* ================================================================
 * ADDITION Circuit: 1-bit full adder
 * ================================================================ */

ACC0Circuit* acc0_build_full_adder(void) {
    /* 1-bit full adder with inputs a, b, carry_in.
     * Outputs: sum, carry_out.
     * sum = a XOR b XOR cin = PARITY(a,b,cin)
     * carry_out = MAJORITY(a,b,cin)
     *
     * With MOD2 gate for XOR and AND/OR for majority. */
    ACC0Circuit *c = acc0_circuit_create(50);
    if (!c) return NULL;

    int a   = acc0_add_input(c);
    int b   = acc0_add_input(c);
    int cin = acc0_add_input(c);

    /* sum = PARITY(a,b,cin) = MOD2(a,b,cin) */
    int fans[3] = { a, b, cin };
    int sum = acc0_add_mod(c, 2);
    acc0_set_fanin(c, sum, fans, 3);

    /* carry_out = MAJORITY(a,b,cin) = (a AND b) OR (a AND cin) OR (b AND cin) */
    int ab   = acc0_add_and(c, a, b);
    int acin = acc0_add_and(c, a, cin);
    int bcin = acc0_add_and(c, b, cin);
    int carry = acc0_add_or(c, ab, acin);
    carry = acc0_add_or(c, carry, bcin);

    int outs[2] = { sum, carry };
    acc0_set_outputs(c, outs, 2);
    return c;
}

/* ================================================================
 * MULTIPLICATION by constant MOD gate
 * ================================================================ */

ACC0Circuit* acc0_build_const_mul_mod(int n, int constant, int modulus) {
    /* Compute (constant * sum(xi)) mod modulus == 0.
     * This is equivalent to MOD_{modulus/gcd(modulus,constant)}(x).
     * Interesting interaction: MOD gates with different constants. */
    int g = constant;
    int m = modulus;
    while (m != 0) { int t = m; m = g % m; g = t; } /* GCD */
    int effective_mod = modulus / g;
    if (effective_mod < 2) effective_mod = modulus;
    return acc0_build_mod_circuit(n, effective_mod);
}

/* ================================================================
 * Demo Function
 * ================================================================ */

void acc0_builders_demo(void) {
    printf("\n========== ACC0 Circuit Builders ==========\n\n");

    printf("--- PARITY Circuit (MOD2) ---\n");
    ACC0Circuit *c_par = acc0_build_parity_circuit(4);
    acc0_print_stats(c_par);
    for (int m = 0; m < 16; m++) {
        int x[4];
        for (int i = 0; i < 4; i++) x[i] = (m>>i)&1;
        printf("  [%d%d%d%d] -> %d\n", x[0],x[1],x[2],x[3],
               acc0_circuit_eval(c_par, x));
    }
    acc0_circuit_free(c_par);

    printf("\n--- MOD6 Circuit ---\n");
    ACC0Circuit *c_m6 = acc0_build_mod_circuit(4, 6);
    acc0_print_stats(c_m6);
    for (int m = 0; m < 16; m++) {
        int x[4];
        for (int i = 0; i < 4; i++) x[i] = (m>>i)&1;
        int s = x[0]+x[1]+x[2]+x[3];
        printf("  [%d%d%d%d] sum=%d MOD6=%d (circuit=%d)\n",
               x[0],x[1],x[2],x[3], s, s%6==0,
               acc0_circuit_eval(c_m6, x));
    }
    acc0_circuit_free(c_m6);

    printf("\n--- AND-of-MODs (MOD3 AND MOD5, n=4) ---\n");
    ACC0Circuit *c_am = acc0_build_and_of_mods(4, 3, 5);
    acc0_print_stats(c_am);
    for (int m = 0; m < 16; m++) {
        int x[4];
        for (int i = 0; i < 4; i++) x[i] = (m>>i)&1;
        int s = x[0]+x[1]+x[2]+x[3];
        printf("  [%d%d%d%d] sum=%d MOD3=%d MOD5=%d AND=%d\n",
               x[0],x[1],x[2],x[3], s, s%3==0, s%5==0,
               acc0_circuit_eval(c_am, x));
    }
    acc0_circuit_free(c_am);

    printf("\n--- Full Adder ---\n");
    ACC0Circuit *fa = acc0_build_full_adder();
    acc0_print_stats(fa);
    printf("  a b ci | sum carry\n");
    for (int m = 0; m < 8; m++) {
        int x[3];
        for (int i = 0; i < 3; i++) x[i] = (m>>i)&1;
        int *vis = (int*)calloc((size_t)fa->ngates, sizeof(int));
        for (int i = 0; i < fa->ngates; i++) vis[i] = -1;
        int sum   = acc0_gate_eval(fa, fa->outputs[0], x, vis);
        int carry = acc0_gate_eval(fa, fa->outputs[1], x, vis);
        printf("  %d %d %d  |  %d    %d\n", x[0],x[1],x[2], sum, carry);
        free(vis);
    }
    acc0_circuit_free(fa);

    printf("\n--- MAJORITY DNF (n=4) ---\n");
    ACC0Circuit *c_maj = acc0_build_majority_dnf(4);
    if (c_maj) {
        acc0_print_stats(c_maj);
        for (int m = 0; m < 16; m++) {
            int x[4];
            for (int i = 0; i < 4; i++) x[i] = (m>>i)&1;
            int s = x[0]+x[1]+x[2]+x[3];
            printf("  [%d%d%d%d] sum=%d maj=%d\n",
                   x[0],x[1],x[2],x[3], s, acc0_circuit_eval(c_maj, x));
        }
        acc0_circuit_free(c_maj);
    }
}

