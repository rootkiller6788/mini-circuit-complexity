/* tc0_arithmetic.h -- Arithmetic in TC0 (L5-L6)
 *
 * Addition, multiplication, division, and iterated addition are all in TC0.
 * This header declares the arithmetic operations and circuit constructors.
 */

#ifndef TC0_ARITHMETIC_H
#define TC0_ARITHMETIC_H

#include "tc0.h"

/* ================================================================
 * BINARY ARITHMETIC OPERATIONS
 * ================================================================ */

/* Binary addition: a + b -> result (n+1 bits, LSB first).
 * In TC0: depth O(log n), size O(n log n) via carry-lookahead. */
void tc0_add_binary(const int *a, const int *b, int *result, int n);

/* Iterated addition: sum of m numbers, each n bits.
 * Result has n + ceil(log2(m)) bits.
 * In TC0: depth O(log max(m,n)), size O(m * n). */
void tc0_iterated_add(const int **numbers, int m, int n, int *result);

/* Binary multiplication: a * b -> product (2n bits).
 * In TC0: depth O(log n), size O(n^2) via iterated addition. */
void tc0_multiply_binary(const int *a, const int *b, int *product, int n);

/* Binary division: a / b -> quotient (n bits), remainder (n bits).
 * In TC0 via Newton iteration (Beame-Cook-Hoover 1986).
 * Depth O(log n), size O(n^2). */
void tc0_divide_binary(const int *a, const int *b, int *quotient,
                        int *remainder, int n);

/* ================================================================
 * TC0 ARITHMETIC CIRCUITS
 * ================================================================ */

/* Build a TC0 circuit for n-bit addition */
TC0Circuit *tc0_build_addition_circuit(int n);

/* Build a TC0 circuit for n-bit multiplication */
TC0Circuit *tc0_build_multiplication(int n);

/* ================================================================
 * DEMO
 * ================================================================ */
void tc0_arithmetic_demo(void);

#endif /* TC0_ARITHMETIC_H */
