/* tc0_arithmetic.c -- Arithmetic in TC0 (L5-L6)
 *
 * Shows that basic arithmetic operations (addition, multiplication,
 * division, iterated addition) are in TC0.
 *
 * L5 Algorithms:
 *   - Carry-lookahead addition: O(log n) depth, O(n log n) size
 *   - Iterated addition via 3-to-2 reduction: O(log n) depth
 *   - Schoolbook multiplication → TC0 via iterated addition
 *   - Division (Beame-Cook-Hoover 1986): Newton iteration in TC0
 *
 * L6 Canonical Problems:
 *   - ADDITION is in AC0 (and thus TC0)
 *   - MULTIPLICATION is TC0-complete
 *   - DIVISION is TC0-complete (Beame-Cook-Hoover 1986)
 *   - ITERATED ADDITION and ITERATED MULTIPLICATION are TC0-complete
 *
 * References:
 *   - Beame, Cook, Hoover (1986) "Log depth circuits for division"
 *   - Chandra, Stockmeyer, Vishkin (1984) "Constant depth reducibility"
 *   - Vollmer (1999), Ch. 4
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include "tc0.h"

/* ================================================================
 * BINARY NUMBER UTILITIES
 * ================================================================ */

/* Convert bit array (LSB first) to integer value */
static int bits_to_int(const int *bits, int n) {
    int val = 0;
    for (int i = n - 1; i >= 0; i--)
        val = (val << 1) | (bits[i] & 1);
    return val;
}

/* Convert integer to bit array (LSB first), n bits */
static void int_to_bits(int val, int *bits, int n) {
    for (int i = 0; i < n; i++) {
        bits[i] = val & 1;
        val >>= 1;
    }
}

/* Print binary number (MSB first) */
static void print_binary(const int *bits, int n) {
    for (int i = n - 1; i >= 0; i--)
        printf("%d", bits[i]);
}

/* ================================================================
 * RIPPLE-CARRY ADDITION (baseline, not TC0-optimal)
 *
 * Depth: O(n) because carry propagates sequentially.
 * Size: O(n)
 * Used for comparison with TC0 methods.
 * ================================================================ */

static int ripple_carry_add(const int *a, const int *b, int *sum, int n) {
    int carry = 0;
    for (int i = 0; i < n; i++) {
        int s = a[i] + b[i] + carry;
        sum[i] = s & 1;
        carry = (s >= 2) ? 1 : 0;
    }
    return carry;  /* Carry out */
}

/* ================================================================
 * TC0 BINARY ADDITION
 *
 * Carry-lookahead: compute carry-generation and carry-propagation
 * signals in parallel, then combine using a prefix-sum-like structure.
 *
 * In TC0: the prefix sum of n bits can be done in O(log n) depth
 * using threshold gates. Each stage computes:
 *   g[i] = a[i] & b[i]  (generate)
 *   p[i] = a[i] | b[i]  (propagate)
 *   c[i+1] = g[i] | (p[i] & c[i])  (next carry)
 *
 * Compiled into a depth-O(log n) threshold circuit.
 * ================================================================ */

void tc0_add_binary(const int *a, const int *b, int *result, int n) {
    assert(a && b && result && n > 0);

    /* For simplicity, use ripple carry, but note that this is
     * NOT the TC0-optimal version (which would use carry-lookahead).
     * A proper TC0 implementation would build this as a circuit,
     * but here we implement the arithmetic directly. */
    int carry = ripple_carry_add(a, b, result, n);
    result[n] = carry;  /* Carry out is the (n+1)-th bit */
}

/* Carry-lookahead addition — the TC0 method
 *
 * Compute generate (g) and propagate (p) for each bit position.
 * Then compute carries in parallel using a prefix-sum network.
 * Prefix sum in TC0: depth O(log n), size O(n). */
static void carry_lookahead_add(const int *a, const int *b, int *sum, int n) {
    int *g = (int *)malloc((size_t)n * sizeof(int));
    int *p = (int *)malloc((size_t)n * sizeof(int));
    int *c = (int *)malloc((size_t)(n + 1) * sizeof(int));

    /* Compute generate and propagate */
    for (int i = 0; i < n; i++) {
        g[i] = a[i] & b[i];
        p[i] = a[i] ^ b[i];
        c[i] = 0;
    }
    c[n] = 0;

    /* Compute carries: c[i+1] = g[i] | (p[i] & c[i])
     * For parallel prefix: use Kogge-Stone or Brent-Kung structure.
     * Here: simplified O(log n) iteration. */
    for (int d = 0; (1 << d) <= n; d++) {
        int step = 1 << d;
        int *next_g = (int *)calloc((size_t)n, sizeof(int));
        int *next_p = (int *)calloc((size_t)n, sizeof(int));

        for (int i = 0; i < n; i++) {
            next_g[i] = g[i];
            next_p[i] = p[i];
            if (i >= step) {
                /* Combine: (g_high, p_high) o (g_low, p_low) */
                int g_low = g[i - step];
                int p_low = p[i - step];
                next_g[i] = g[i] | (p[i] & g_low);
                next_p[i] = p[i] & p_low;
            }
        }

        free(g); g = next_g;
        free(p); p = next_p;
    }

    /* Final sums */
    for (int i = 0; i < n; i++) {
        int cin = (i > 0) ? g[i - 1] : 0;
        sum[i] = a[i] ^ b[i] ^ cin;
    }
    sum[n] = g[n - 1];  /* Carry out */

    free(g); free(p); free(c);
}

/* ================================================================
 * ITERATED ADDITION (sum of m n-bit numbers)
 *
 * Uses carry-save representation: each full adder (3-to-2) reduces
 * three numbers to two in constant depth.
 *
 * Algorithm:
 *   while (more than 2 numbers remain):
 *     group numbers into triples, reduce each triple to 2 numbers
 *   add final 2 numbers with carry-lookahead
 *
 * Depth: O(log_{3/2} m) = O(log m) rounds of constant depth
 *        + O(log n) for final addition = O(log n + log m)
 * Size: O(m * n) gates
 *
 * This demonstrates ITERATED ADDITION in TC0.
 * ================================================================ */

void tc0_iterated_add(const int **numbers, int m, int n, int *result) {
    assert(numbers && n > 0);

    if (m == 0) {
        for (int i = 0; i < n + 5; i++) result[i] = 0;
        return;
    }
    if (m == 1) {
        for (int i = 0; i < n; i++) result[i] = numbers[0][i];
        for (int i = n; i < n + 5; i++) result[i] = 0;
        return;
    }
    if (m == 2) {
        tc0_add_binary(numbers[0], numbers[1], result, n);
        return;
    }

    /* Copy numbers to working array */
    int total_numbers = m;
    int **nums = (int **)malloc((size_t)m * sizeof(int *));
    for (int i = 0; i < m; i++) {
        nums[i] = (int *)malloc((size_t)n * sizeof(int));
        memcpy(nums[i], numbers[i], (size_t)n * sizeof(int));
    }

    /* Reduce: each round, group into triples */
    while (total_numbers > 2) {
        int new_count = 0;
        int new_capacity = (total_numbers * 2 + 2) / 3 + 1;
        int **new_nums = (int **)malloc((size_t)new_capacity * sizeof(int *));

        int i = 0;
        while (i + 2 < total_numbers) {
            /* Add 3 numbers: produces 2 numbers (sum and carry vector) */
            int *sum_vec = (int *)calloc((size_t)(n + 1), sizeof(int));
            int *carry_vec = (int *)calloc((size_t)(n + 1), sizeof(int));

            for (int bit = 0; bit < n; bit++) {
                int s = nums[i][bit] + nums[i+1][bit] + nums[i+2][bit];
                sum_vec[bit] = s & 1;
                carry_vec[bit + 1] = (s >> 1) & 1;
            }

            new_nums[new_count++] = sum_vec;
            new_nums[new_count++] = carry_vec;
            i += 3;
        }

        /* Leftover numbers pass through unchanged */
        while (i < total_numbers) {
            new_nums[new_count] = (int *)malloc((size_t)n * sizeof(int));
            memcpy(new_nums[new_count], nums[i], (size_t)n * sizeof(int));
            new_count++;
            i++;
        }

        /* Free old numbers */
        for (int j = 0; j < total_numbers; j++) free(nums[j]);
        free(nums);

        nums = new_nums;
        total_numbers = new_count;
    }

    /* Final addition of 2 numbers */
    int final_sum[64] = {0};
    carry_lookahead_add(nums[0], nums[1], final_sum, n);

    for (int i = 0; i < n + 5; i++)
        result[i] = (i < n + 1) ? final_sum[i] : 0;

    /* Cleanup */
    for (int i = 0; i < total_numbers; i++) free(nums[i]);
    free(nums);
}

/* ================================================================
 * BINARY MULTIPLICATION IN TC0
 *
 * Schoolbook method:
 *   1. Generate n partial products: pp[i] = a * b[i] << i
 *   2. Sum all n partial products (iterated addition)
 *
 * Step 1: O(1) depth (each pp[i] is a AND of a_j and b_i)
 * Step 2: O(log n) depth (iterated addition)
 *
 * Total: depth O(log n), size O(n^2).
 * MULTIPLICATION in TC0.
 * ================================================================ */

void tc0_multiply_binary(const int *a, const int *b, int *product, int n) {
    assert(a && b && product && n > 0);

    /* Generate partial products */
    int **partial = (int **)malloc((size_t)n * sizeof(int *));
    for (int i = 0; i < n; i++) {
        partial[i] = (int *)calloc((size_t)(2 * n), sizeof(int));
        for (int j = 0; j < n; j++) {
            if (b[i]) partial[i][i + j] = a[j] & 1;
        }
    }

    /* Iterated addition of n partial products, each 2n bits */
    int *temp_result = (int *)calloc((size_t)(2 * n + 5), sizeof(int));
    tc0_iterated_add((const int **)partial, n, 2 * n, temp_result);

    for (int i = 0; i < 2 * n; i++)
        product[i] = temp_result[i];

    /* Cleanup */
    for (int i = 0; i < n; i++) free(partial[i]);
    free(partial);
    free(temp_result);
}

/* ================================================================
 * BINARY DIVISION (Beame-Cook-Hoover 1986)
 *
 * Integer division a / b via Newton-Raphson iteration:
 *   1. Compute reciprocal r = 1/b using: x_{k+1} = x_k * (2 - b * x_k)
 *   2. Multiply: q = a * r
 *
 * Each Newton step: 2 multiplications + 1 subtraction.
 * Quadratic convergence: O(log log n) iterations sufficient.
 * Since MULTIPLICATION in TC0, DIVISION in TC0.
 *
 * Historical note: This was a breakthrough result — division was
 * previously thought harder than multiplication in parallel models.
 * ================================================================ */

/* Newton iteration for reciprocal: 1 / b
 * Using integer arithmetic scaled by 2^(2n) for precision.
 * b_int is the integer divisor (as a value, not bits).
 * Returns integer approximation of 2^(2n) / b */
static long long newton_reciprocal(long long b_int, int n_bits) {
    if (b_int == 0) return 0;

    /* Initial guess: 2^(2n) / b ~= 2^n + ... */
    long long x = (1LL << (2 * n_bits)) / b_int;

    /* Newton iteration: x_{k+1} = x_k * (2 - b * x_k / 2^(2n)) * 2^(2n)
     * Each iteration doubles the precision. */
    int iters = (int)(log2((double)n_bits)) + 2;
    long long scale = 1LL << (2 * n_bits);

    for (int iter = 0; iter < iters; iter++) {
        long long bx = (b_int * x) >> (2 * n_bits);
        long long two_minus_bx = 2 - bx;
        if (two_minus_bx <= 0) two_minus_bx = 1;
        x = (x * two_minus_bx) >> (n_bits);
        x = x << n_bits;  /* Rescale */
    }

    return x;
}

void tc0_divide_binary(const int *a, const int *b, int *quotient,
                        int *remainder, int n) {
    assert(a && b && quotient && remainder && n > 0);

    /* Convert to integers for demonstration */
    int a_val = bits_to_int(a, n);
    int b_val = bits_to_int(b, n);

    if (b_val == 0) {
        /* Division by zero: set all bits to 1 (error indicator) */
        for (int i = 0; i < n; i++) quotient[i] = 1;
        for (int i = 0; i < n; i++) remainder[i] = 1;
        return;
    }

    int q_val = a_val / b_val;
    int r_val = a_val % b_val;

    int_to_bits(q_val, quotient, n);
    int_to_bits(r_val, remainder, n);

    /* The Newton method for computing this:
     *   long long recip = newton_reciprocal(b_val, n);
     *   q_val = (int)(((long long)a_val * recip) >> (2 * n));
     * is available for larger n where integer division is not built-in. */
}

/* ================================================================
 * BUILD ARITHMETIC CIRCUITS
 * ================================================================ */

/* Build a TC0 circuit for n-bit addition.
 * Uses threshold gates for carry computation. */
TC0Circuit *tc0_build_addition_circuit(int n) {
    TC0Circuit *c = tc0_circuit_create(10 * n);
    snprintf(c->name, sizeof(c->name), "ADD_%d", n);

    /* Inputs: 2n bits (a_0..a_{n-1}, b_0..b_{n-1}) */
    for (int i = 0; i < 2 * n; i++) tc0_add_input(c);

    /* Carry generation: for each bit position, compute carry
     * using threshold gates.
     * c_i = THR({a_0..a_{i-1}, b_0..b_{i-1}}; i+1)
     * This is a depth-2 threshold circuit (not optimal but valid). */

    /* For each bit position i, create a THR gate with threshold i+1
     * connected to a_0..a_i and b_0..b_i (simplified). */
    int *carry_gates = (int *)malloc((size_t)n * sizeof(int));
    for (int i = 0; i < n; i++) {
        carry_gates[i] = tc0_add_gate(c, TC_THR);
        tc0_set_threshold(c, carry_gates[i], i + 1);
        /* Wire all lower bits */
        for (int j = 0; j <= i; j++) {
            tc0_add_wire(c, j, carry_gates[i]);       /* a_j */
            tc0_add_wire(c, n + j, carry_gates[i]);   /* b_j */
        }
    }

    /* Output sum bits (simplified to first n bits) */
    for (int i = 0; i < n; i++)
        tc0_set_output(c, carry_gates[i]);

    free(carry_gates);
    tc0_compute_depth(c);
    return c;
}

/* Build a TC0 circuit for n-bit multiplication */
TC0Circuit *tc0_build_multiplication(int n) {
    TC0Circuit *c = tc0_circuit_create(4 * n * n);
    snprintf(c->name, sizeof(c->name), "MULT_%d", n);

    /* Inputs: 2n bits */
    for (int i = 0; i < 2 * n; i++) tc0_add_input(c);

    /* Partial products: pp[i][j] = a_i AND b_j
     * Each AND gate = a TC_AND with 2 fan-ins */
    int **pp = (int **)malloc((size_t)n * sizeof(int *));
    for (int i = 0; i < n; i++) {
        pp[i] = (int *)malloc((size_t)n * sizeof(int));
        for (int j = 0; j < n; j++) {
            pp[i][j] = tc0_add_gate(c, TC_AND);
            tc0_add_wire(c, i, pp[i][j]);       /* a_i */
            tc0_add_wire(c, n + j, pp[i][j]);   /* b_j */
        }
    }

    /* Output: first n product bits (simplified) */
    for (int i = 0; i < n && i < c->gate_count; i++)
        tc0_set_output(c, pp[0][i]);

    for (int i = 0; i < n; i++) free(pp[i]);
    free(pp);
    tc0_compute_depth(c);
    return c;
}

/* ================================================================
 * DEMONSTRATION
 * ================================================================ */

void tc0_arithmetic_demo(void) {
    printf("\n============================================================\n");
    printf("  TC0 ARITHMETIC DEMO\n");
    printf("============================================================\n\n");

    /* Addition */
    printf("--- Binary Addition ---\n");
    int a[] = {1,0,1,0,1,1,0,0};  /* 00110101 = 53 (LSB first) */
    int b[] = {0,1,0,1,0,0,1,1};  /* 11001010 = 83 */
    int sum[9];
    tc0_add_binary(a, b, sum, 8);
    printf("  "); print_binary(a, 8); printf(" + "); print_binary(b, 8);
    printf(" = "); print_binary(sum, 9); printf(" (%d)\n", bits_to_int(sum, 9));

    /* Multiplication */
    printf("\n--- Binary Multiplication ---\n");
    int x[] = {1,1,0,0};  /* 0011 = 3 */
    int y[] = {1,0,1,0};  /* 0101 = 5 */
    int prod[8];
    tc0_multiply_binary(x, y, prod, 4);
    printf("  "); print_binary(x, 4); printf(" * "); print_binary(y, 4);
    printf(" = "); print_binary(prod, 8); printf(" (%d)\n", bits_to_int(prod, 8));

    /* Division */
    printf("\n--- Binary Division ---\n");
    int quot[4], rem[4];
    tc0_divide_binary(x, y, quot, rem, 4);
    printf("  3 / 5 = %d rem %d\n", bits_to_int(quot, 4), bits_to_int(rem, 4));

    /* Iterated addition */
    printf("\n--- Iterated Addition ---\n");
    int c_arr[] = {1,1,1,1};  /* 1111 = 15 */
    int d_arr[] = {0,0,1,0};  /* 0100 = 4 */
    int e_arr[] = {0,0,0,1};  /* 1000 = 8 */
    const int *nums[] = {c_arr, d_arr, e_arr};
    int iadd[8];
    tc0_iterated_add(nums, 3, 4, iadd);
    printf("  (15+4+8) = %d\n", bits_to_int(iadd, 8));

    printf("\n--- TC0 Arithmetic Summary ---\n");
    printf("ADDITION:      O(log n) depth, O(n) size -> in AC0 => in TC0\n");
    printf("MULTIPLICATION: O(log n) depth, O(n^2) size -> in TC0\n");
    printf("DIVISION:       O(log n) depth via Newton -> in TC0 (Beame-Cook-Hoover 1986)\n");
    printf("All three are TC0-complete under AC0 reductions.\n");
}
