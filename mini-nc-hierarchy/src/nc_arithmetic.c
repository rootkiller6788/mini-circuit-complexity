/* nc_arithmetic.c -- Parallel Arithmetic in NC^1 and NC^2
 *
 * Integer operations and their circuit complexity classification:
 *
 * Addition:       NC^1-complete under AC^0 reductions
 * Multiplication: NC^1 (via Wallace/Dadda trees → carry-save addition)
 * Division:       NC^2 (Beame-Cook-Hoover 1986)
 *                 Actually NC^1 (Chiu-Davida-Litow 2001 via Newton iteration)
 * Powering (a^b): NC^1 (via repeated squaring with parallel multiplication)
 * GCD:            NC^2 (via parallel Euclidean algorithm)
 *
 * The carry-lookahead adder is the canonical NC^1 arithmetic circuit.
 * The carry computation IS a parallel prefix of the (generate, propagate)
 * semigroup with operator:
 *   (g1, p1) o (g2, p2) = (g1 OR (p1 AND g2), p1 AND p2)
 *
 * Reference:
 *   Ladner & Fischer (1980) "Parallel Prefix Computation"
 *   Beame, Cook, Hoover (1986) "Log Depth Circuits for Division"
 *   Chiu, Davida, Litow (2001) "Division in Logspace-Uniform NC^1"
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include "nc.h"

/* ================================================================
 * INTEGER ADDITION — NC^1 via Carry-Lookahead
 * ================================================================ */

/* Ripple-carry addition: O(n) depth baseline */
void ripple_carry_add_bits(const int *a, const int *b, int n,
                            int *sum, int *carry_out) {
    int carry = 0;
    for (int i = 0; i < n; i++) {
        int s = a[i] ^ b[i] ^ carry;
        carry = (a[i] & b[i]) | (a[i] & carry) | (b[i] & carry);
        sum[i] = s;
    }
    if (carry_out) *carry_out = carry;
}

/* Carry-lookahead addition: O(log n) depth via parallel prefix.
 *
 * For each bit position i:
 *   g_i = a_i AND b_i  (generate: carry is generated at position i)
 *   p_i = a_i XOR b_i  (propagate: carry passes through position i)
 *
 * Carry recurrence:
 *   c_0    = 0 (carry-in)
 *   c_{i+1}= g_i OR (p_i AND c_i)
 *
 * This is a prefix computation with operator:
 *   (g, p) • (g', p') = (g OR (p AND g'), p AND p')
 *
 * The identity element is (0, 1). */
void carry_lookahead_add_bits(const int *a, const int *b, int n, int *sum) {
    /* Compute (g_i, p_i) pairs */
    int *g = (int *)malloc((size_t)n * sizeof(int));
    int *p = (int *)malloc((size_t)n * sizeof(int));
    assert(g && p);
    for (int i = 0; i < n; i++) {
        g[i] = a[i] & b[i];
        p[i] = a[i] ^ b[i];
    }

    /* Parallel prefix computation of carries */
    /* c_{i+1} = g_i OR (p_i AND c_i) */
    /* This is the prefix of the (g,p) operator */

    /* Use tree-based parallel prefix (Kogge-Stone style) */
    int steps = (int)ceil(log2((double)n));
    if (n == 0) steps = 0;

    int **gg = (int **)malloc((size_t)(steps + 1) * sizeof(int *));
    int **pp = (int **)malloc((size_t)(steps + 1) * sizeof(int *));
    assert(gg && pp);
    gg[0] = g; pp[0] = p;

    for (int s = 1; s <= steps; s++) {
        int dist = 1 << (s - 1);
        gg[s] = (int *)malloc((size_t)n * sizeof(int));
        pp[s] = (int *)malloc((size_t)n * sizeof(int));
        assert(gg[s] && pp[s]);

        for (int i = 0; i < n; i++) {
            if (i >= dist) {
                /* Combine: (gg[i], pp[i]) after (gg[i-dist], pp[i-dist]) */
                gg[s][i] = gg[s-1][i] | (pp[s-1][i] & gg[s-1][i - dist]);
                pp[s][i] = pp[s-1][i] & pp[s-1][i - dist];
            } else {
                gg[s][i] = gg[s-1][i];
                pp[s][i] = pp[s-1][i];
            }
        }
    }

    /* Compute sum bits:
     * s_0 = a_0 XOR b_0 XOR c_0 = p_0 XOR 0 = p_0
     * s_i = a_i XOR b_i XOR c_i = p_i XOR c_i
     * where c_i = g_{i-1} carried forward (i.e., gg[steps][i-1])
     */
    sum[0] = a[0] ^ b[0]; /* c_0 = 0 */
    for (int i = 1; i < n; i++) {
        int c_i = gg[steps][i - 1];
        sum[i] = a[i] ^ b[i] ^ c_i;
    }

    /* Cleanup */
    for (int s = 1; s <= steps; s++) {
        free(gg[s]); free(pp[s]);
    }
    free(gg); free(pp);
}

/* Carry-select addition: trade-off between ripple and full lookahead.
 * Splits n bits into blocks of size k. For each block, compute two
 * possible results (carry-in = 0 and carry-in = 1). The final
 * carry-out selects which version to use.
 * Depth: O(n/k + k), size: O(n * 2k/k) = O(n). */
void carry_select_add_bits(const int *a, const int *b, int n, int *sum) {
    int k = (int)sqrt((double)n); /* block size for balanced depth */
    if (k < 4) k = 4;

    int num_blocks = (n + k - 1) / k;

    /* For each block: compute sum_0 (cin=0) and sum_1 (cin=1) */
    int **sum0 = (int **)malloc((size_t)num_blocks * sizeof(int *));
    int **sum1 = (int **)malloc((size_t)num_blocks * sizeof(int *));
    int *carry0 = (int *)malloc((size_t)num_blocks * sizeof(int));
    int *carry1 = (int *)malloc((size_t)num_blocks * sizeof(int));
    assert(sum0 && sum1 && carry0 && carry1);

    for (int blk = 0; blk < num_blocks; blk++) {
        int start = blk * k;
        int blk_n = (start + k <= n) ? k : n - start;

        sum0[blk] = (int *)malloc((size_t)blk_n * sizeof(int));
        sum1[blk] = (int *)malloc((size_t)blk_n * sizeof(int));
        assert(sum0[blk] && sum1[blk]);

        /* Compute with cin=0 */
        int c = 0;
        for (int i = 0; i < blk_n; i++) {
            int s = a[start + i] ^ b[start + i] ^ c;
            c = (a[start + i] & b[start + i]) | (a[start + i] & c) | (b[start + i] & c);
            sum0[blk][i] = s;
        }
        carry0[blk] = c;

        /* Compute with cin=1 */
        c = 1;
        for (int i = 0; i < blk_n; i++) {
            int s = a[start + i] ^ b[start + i] ^ c;
            c = (a[start + i] & b[start + i]) | (a[start + i] & c) | (b[start + i] & c);
            sum1[blk][i] = s;
        }
        carry1[blk] = c;
    }

    /* Select based on actual carry chain */
    int prev_carry = 0;
    for (int blk = 0; blk < num_blocks; blk++) {
        int start = blk * k;
        int blk_n = (start + k <= n) ? k : n - start;

        if (prev_carry == 0) {
            memcpy(sum + start, sum0[blk], (size_t)blk_n * sizeof(int));
            prev_carry = carry0[blk];
        } else {
            memcpy(sum + start, sum1[blk], (size_t)blk_n * sizeof(int));
            prev_carry = carry1[blk];
        }
    }

    /* Cleanup */
    for (int blk = 0; blk < num_blocks; blk++) {
        free(sum0[blk]); free(sum1[blk]);
    }
    free(sum0); free(sum1); free(carry0); free(carry1);
}

/* ================================================================
 * CARRY-SAVE ADDER (3:2 compressor)
 *
 * Takes three n-bit numbers (a, b, c) and produces two n+1-bit
 * numbers (sum, carry) such that a + b + c = sum + carry.
 * This is the fundamental building block of Wallace/Dadda multipliers.
 *
 * Depth: O(1) — all bits computed independently
 * Size: O(n)
 * ================================================================ */

void carry_save_adder_3to2(const int *a, const int *b, const int *c,
                            int n, int *sum, int *carry) {
    for (int i = 0; i < n; i++) {
        int total = a[i] + b[i] + c[i];
        sum[i]   = total % 2;     /* sum bit */
        carry[i] = total / 2;     /* carry bit (shifted left by 1) */
    }
    carry[n] = 0; /* no carry into bit n from bit n-1 */
}

/* ================================================================
 * MULTIPLICATION — Wallace Tree (NC^1)
 *
 * Wallace tree (1964): Reduce n partial products using carry-save
 * adders in a tree structure. Each CSA level reduces 3 rows to 2.
 *
 * Depth: O(log n) CSA layers × O(1) per CSA = O(log n)
 * After reduction to 2 rows: add with carry-lookahead → O(log n)
 * Total depth: O(log n) → NC^1
 *
 * Size: O(n^2) (one gate per partial product bit)
 * ================================================================ */

void wallace_tree_multiply(const int *a, int na, const int *b, int nb, int *prod) {
    int total_bits = na + nb;

    /* Generate partial products: pp[i] = a_i AND b (shifted by i) */
    int **pp = (int **)malloc((size_t)na * sizeof(int *));
    assert(pp);
    for (int i = 0; i < na; i++) {
        pp[i] = (int *)calloc((size_t)(total_bits + 1), sizeof(int));
        assert(pp[i]);
        for (int j = 0; j < nb; j++)
            pp[i][i + j] = a[i] & b[j];
    }

    /* Reduce using carry-save adder tree */
    int active = na;
    while (active > 2) {
        int new_rows = 0;
        int **next = (int **)malloc((size_t)((active * 2 + 2) / 3 + 1) * sizeof(int *));

        /* Group by 3 */
        for (int i = 0; i < active; i += 3) {
            if (i + 2 < active) {
                /* 3 rows → 2 via CSA */
                int *sum = (int *)calloc((size_t)(total_bits + 1), sizeof(int));
                int *car = (int *)calloc((size_t)(total_bits + 1), sizeof(int));
                carry_save_adder_3to2(pp[i], pp[i+1], pp[i+2], total_bits, sum, car);
                next[new_rows++] = sum;
                next[new_rows++] = car;
            } else if (i + 1 < active) {
                /* 2 rows → pass through */
                next[new_rows++] = pp[i];
                next[new_rows++] = pp[i+1];
            } else {
                /* 1 row → pass through */
                next[new_rows++] = pp[i];
            }
        }

        /* Free old rows that were consumed by CSA */
        for (int i = 0; i < active; i += 3) {
            if (i + 2 < active) {
                free(pp[i]); free(pp[i+1]); free(pp[i+2]);
            }
        }
        free(pp);
        pp = next;
        active = new_rows;
    }

    /* Final addition: if 2 rows remain, add them */
    if (active == 2) {
        carry_lookahead_add_bits(pp[0], pp[1], total_bits, prod);
    } else if (active == 1) {
        memcpy(prod, pp[0], (size_t)total_bits * sizeof(int));
    }

    /* Cleanup */
    for (int i = 0; i < active; i++) free(pp[i]);
    free(pp);
}

/* Dadda tree multiplier: optimizes CSA placement for fewer half-adders.
 * Same asymptotic depth O(log n) but better constant. */
void dadda_tree_multiply(const int *a, int na, const int *b, int nb, int *prod) {
    /* For simplicity, delegate to Wallace tree.
     * Dadda differs only in optimal CSA arrangement;
     * same O(log n) depth, O(n^2) size. */
    wallace_tree_multiply(a, na, b, nb, prod);
}

/* ================================================================
 * INTEGER DIVISION — Restoring algorithm (O(n) depth baseline)
 *
 * NC division is complex. Beame-Cook-Hoover (1986) gave an O(log^2 n)
 * depth circuit (NC^2). Later improved to NC^1.
 *
 * Here we implement the baseline restoring division with O(n) depth
 * and the parallel approach skeleton.
 * ================================================================ */

int restoring_division_bits(const int *dividend, const int *divisor,
                              int n, int *quotient, int *remainder) {
    /* Check for division by zero */
    int div_zero = 1;
    for (int i = 0; i < n; i++)
        if (divisor[i] != 0) { div_zero = 0; break; }
    if (div_zero) return -1;

    /* Initialize remainder = 0 */
    int *rem = (int *)calloc((size_t)(n + 1), sizeof(int));
    assert(rem);

    for (int i = n - 1; i >= 0; i--) {
        /* Shift remainder left by 1, bring down next dividend bit */
        for (int j = n; j > 0; j--)
            rem[j] = rem[j - 1];
        rem[0] = dividend[i];

        /* Try to subtract divisor from remainder */
        /* Compute rem - divisor */
        int *diff = (int *)malloc((size_t)(n + 1) * sizeof(int));
        /* 2's complement subtraction: diff = rem + ~divisor + 1 */
        int borrow = 0;
        for (int j = 0; j <= n; j++) {
            int dv = (j < n) ? divisor[j] : 0;
            int sub = rem[j] - dv - borrow;
            if (sub < 0) { sub += 2; borrow = 1; }
            else borrow = 0;
            diff[j] = sub;
        }

        if (borrow == 0) {
            /* Subtraction succeeded: quotient bit = 1 */
            quotient[i] = 1;
            memcpy(rem, diff, (size_t)(n + 1) * sizeof(int));
        } else {
            /* Subtraction failed: quotient bit = 0, restore remainder */
            quotient[i] = 0;
        }
        free(diff);
    }

    memcpy(remainder, rem, (size_t)n * sizeof(int));
    free(rem);
    return 0; /* success */
}

/* ================================================================
 * BIT CONVERSION UTILITIES
 * ================================================================ */

/* Convert integer to bit array (LSB first) */
static void to_bits(int val, int *bits, int n) {
    for (int i = 0; i < n; i++) { bits[i] = val & 1; val >>= 1; }
}

/* Convert bit array to integer */
static int from_bits(const int *bits, int n) {
    int v = 0;
    for (int i = n - 1; i >= 0; i--) v = (v << 1) | bits[i];
    return v;
}

/* ================================================================
 * DEMO: Carry-lookahead adder
 * ================================================================ */

void nc_adder_demo(void) {
    printf("\n================================================================\n");
    printf("  NC^1 CARRY-LOOKAHEAD ADDER\n");
    printf("  Addition is NC^1-complete under AC^0 reductions\n");
    printf("================================================================\n\n");

    int n = 8;
    printf("--- Ripple-Carry vs Carry-Lookahead ---\n");
    printf("Ripple-carry: depth=%d, size=%d\n", n, 5*n);
    printf("Lookahead:    depth=%.0f, size=%.0f\n\n",
           ceil(log2((double)n))*2, 5.0*n*log2((double)n));

    printf("--- 8-bit Addition Examples ---\n");
    int tests[][2] = {{5,3}, {15,1}, {127,1}, {85,42}, {255,0}};
    int a[8], b[8], sum_rip[8], sum_cla[8], sum_cs[8];

    for (int t = 0; t < 5; t++) {
        to_bits(tests[t][0], a, n);
        to_bits(tests[t][1], b, n);
        ripple_carry_add_bits(a, b, n, sum_rip, NULL);
        carry_lookahead_add_bits(a, b, n, sum_cla);
        carry_select_add_bits(a, b, n, sum_cs);
        int vr = from_bits(sum_rip, n);
        int vc = from_bits(sum_cla, n);
        int vs = from_bits(sum_cs, n);
        printf("  %3d + %3d = %3d (ripple) = %3d (cla) = %3d (csel) %s\n",
               tests[t][0], tests[t][1], vr, vc, vs,
               (vr == vc && vc == vs) ? "OK" : "FAIL");
    }

    printf("\n--- Depth vs n Scaling ---\n");
    printf("  n      ripple_depth  cla_depth  cla_size\n");
    for (int n = 4; n <= 1024; n *= 2)
        printf("  %-6d %-13d %-10.0f %-10.0f\n",
               n, n, 2.0*ceil(log2((double)n)), 5.0*n*log2((double)n));

    printf("\n--- Carry Generation Visualization (n=8) ---\n");
    to_bits(106, a, 8); /* 01101010 */
    to_bits(89, b, 8);  /* 01011001 → 106+89=195 */
    printf("  a  = ");
    for (int i = 7; i >= 0; i--) printf("%d", a[i]);
    printf(" (106)\n  b  = ");
    for (int i = 7; i >= 0; i--) printf("%d", b[i]);
    printf(" (89)\n");
    printf("  g  = "); /* generate: a AND b */
    for (int i = 7; i >= 0; i--) printf("%d", a[i] & b[i]);
    printf("\n  p  = "); /* propagate: a XOR b */
    for (int i = 7; i >= 0; i--) printf("%d", a[i] ^ b[i]);
    printf("\n");

    carry_lookahead_add_bits(a, b, 8, sum_cla);
    printf("  sum= ");
    for (int i = 7; i >= 0; i--) printf("%d", sum_cla[i]);
    printf(" = %d\n", from_bits(sum_cla, 8));

    printf("\n--- NC^1-Completeness ---\n");
    printf("Addition is NC^1-complete under AC^0 reductions.\n");
    printf("Reduction: Formula Eval ≤_AC0 Addition.\n");
    printf("This means: if Addition ∈ AC^0, then NC^1 = AC^0 (false).\n");
    printf("The carry-lookahead adder gives O(log n) depth, proving Addition ∈ NC^1.\n");
}

/* ================================================================
 * DEMO: Multiplier
 * ================================================================ */

static void multiplier_demo(void) {
    printf("\n--- WALLACE TREE MULTIPLIER ---\n");
    int a[4], b[4], prod[8];

    int tests[][2] = {{3,5}, {7,3}, {15,15}};
    for (int t = 0; t < 3; t++) {
        to_bits(tests[t][0], a, 4);
        to_bits(tests[t][1], b, 4);
        memset(prod, 0, 8 * sizeof(int));
        wallace_tree_multiply(a, 4, b, 4, prod);
        printf("  %2d × %2d = %3d (Wallace) = %3d (expected)\n",
               tests[t][0], tests[t][1],
               from_bits(prod, 8), tests[t][0] * tests[t][1]);
    }

    printf("\n--- Multiplication Complexity ---\n");
    printf("  Schoolbook:  O(n^2) gates, O(n) depth (ripple-carry final add)\n");
    printf("  Wallace/Dadda: O(n^2) gates, O(log n) depth (NC^1)\n");
    printf("  Schönhage-Strassen: O(n log n log log n) gates (galactic)\n");
    printf("  Multiplication ∈ NC^1 (completeness status unknown).\n");
}
