/* acc0_gates.h — MOD Gate Theory and Properties
 * ================================================
 * L2: Core Concepts — MOD gates as modular counting primitives.
 * L3: Mathematical Structures — Algebraic characterization of gates.
 *
 * MOD_m(x₁,...,xₙ) = 1 iff ∑ xᵢ ≡ 0 (mod m).
 *
 * Key Properties:
 * - MOD₂ = PARITY = XOR of all inputs.
 * - MODₚ for prime p: cannot be computed by AC0 alone (Razborov-Smolensky).
 * - MODₚ gates are closed under negation but not under AND/OR.
 * - Composite MOD gates: MOD_{ab} can simulate MOD_a and MOD_b (with AND).
 *
 * References:
 * - Razborov (1987): "Lower bounds on the size of bounded depth circuits
 *   over a complete basis with logical addition", Mat. Zametki 41(4).
 * - Smolensky (1987): "Algebraic methods in the theory of lower bounds
 *   for Boolean circuit complexity", STOC 1987.
 * - Vollmer (1999): Introduction to Circuit Complexity, §4.2–4.4.
 */

#ifndef ACC0_GATES_H
#define ACC0_GATES_H

#include "acc0.h"

/* ================================================================
 * L1: Gate type queries and properties
 * ================================================================ */

/* Return the ASCII name of a gate type. */
const char* acc0_gate_type_name(ACC0GateType t);

/* Return 1 if the gate type is a MOD_m gate (any modulus). */
int acc0_is_mod_gate(ACC0GateType t);

/* Return 1 if the gate type is a MOD_p gate for prime p. */
int acc0_is_prime_mod_gate(ACC0GateType t);

/* Return the modulus m for a MOD gate type, or 0 if not MOD. */
int acc0_gate_modulus(ACC0GateType t);

/* Return 1 if the gate type is monotone (AND, OR, no NOT). */
int acc0_is_monotone(ACC0GateType t);

/* Return 1 if the gate type is linear over GF(2). */
int acc0_is_linear_gf2(ACC0GateType t);

/* ================================================================
 * L2: MOD gate computation
 * ================================================================ */

/* Compute MOD_m(x) for an array of bits.
 * Returns 1 iff sum(xᵢ) ≡ 0 (mod m).
 * Complexity: O(n). */
int acc0_mod_compute(int m, const int *x, int n);

/* Compute MOD_m value from pre-computed sum.
 * Returns 1 iff sum ≡ 0 (mod m). */
int acc0_mod_from_sum(int m, int sum);

/* Compute PARITY (MOD2) of bit array.
 * Complexity: O(n). */
int acc0_parity(const int *x, int n);

/* Compute MAJORITY of bit array.
 * Returns 1 iff sum(xᵢ) > n/2.
 * Complexity: O(n).
 * Note: MAJORITY may NOT be in ACC0 (open since 1987). */
int acc0_majority(const int *x, int n);

/* Compute THRESHOLD_k: 1 iff sum(xᵢ) ≥ k. */
int acc0_threshold(const int *x, int n, int k);

/* ================================================================
 * L3: Algebraic Properties of MOD Gates
 * ================================================================ */

/* Check if two moduli are coprime. */
int acc0_coprime(int a, int b);

/* Chinese Remainder Theorem: given values a mod m1 and b mod m2
 * with coprime m1,m2, find x mod m1*m2.
 * Stores result in *result. Returns 0 on success, -1 if not coprime. */
int acc0_crt_combine(int a, int m1, int b, int m2, int *result);

/* Check whether a MODₐ gate can simulate a MOD_b test.
 * (This is non-trivial; generally requires composite modulus.) */
int acc0_mod_simulates(int a, int b);

/* ================================================================
 * L5: Gate-level operations on circuits
 * ================================================================ */

/* Count the number of gates of each type in a circuit. */
void acc0_count_gate_types(ACC0Circuit *c, int counts[12]);

/* Find all MOD gates in the circuit.
 * Returns array of gate indices, sets *count.
 * Caller must free returned array. */
int* acc0_find_mod_gates(ACC0Circuit *c, int *count);

/* Compute the maximum fan-in among all MOD gates in the circuit. */
int acc0_max_mod_fanin(ACC0Circuit *c);

/* Check if all MOD gates in the circuit use the same modulus.
 * If so, sets *modulus and returns 1. Otherwise returns 0. */
int acc0_uniform_modulus(ACC0Circuit *c, int *modulus);

/* Compute the set of distinct moduli used in a circuit.
 * Returns array of moduli, sets *count. Caller frees. */
int* acc0_distinct_moduli(ACC0Circuit *c, int *count);

/* ================================================================
 * L6: Gate composition and construction helpers
 * ================================================================ */

/* Build a MODₘ gate over n input gates (indices 0..n-1 are assumed inputs).
 * Returns the output gate index. */
int acc0_build_mod_gate(ACC0Circuit *c, int modulus, int ninputs);

/* Build a PARITY circuit over n inputs using MOD2.
 * Returns output gate index.
 * Complexity: O(n) gates. */
int acc0_build_parity(ACC0Circuit *c, int ninputs);

/* Build a MAJORITY circuit using AND/OR/NOT only (no MOD gates).
 * This is EXPONENTIAL in depth for constant-depth circuits.
 * Returns output gate index.
 * Note: This demonstrates WHY MOD gates are needed — MAJORITY ∉ AC0.
 * Complexity: O(2ⁿ) gates, depth O(log n) — not constant-depth! */
int acc0_build_majority_ac0(ACC0Circuit *c, int ninputs);

/* Build a circuit that computes (sum xᵢ mod m₁ = 0) AND (sum xᵢ mod m₂ = 0).
 * Returns output gate index. */
int acc0_build_multi_mod(ACC0Circuit *c, int ninputs, int m1, int m2);

/* Build a circuit computing the symmetric function S_k^n:
 * 1 iff exactly k of the n inputs are 1.
 * Uses AND/OR/NOT only. Returns output gate index. */
int acc0_build_symmetric(ACC0Circuit *c, int ninputs, int k);

/* ================================================================
 * L8: Prime modulus theory
 * ================================================================ */

/* Check if p is prime using trial division up to sqrt(p). */
int acc0_is_prime(int p);

/* Get the nth prime (2,3,5,7,11,...). n ≥ 1. */
int acc0_nth_prime(int n);

/* Euler's totient φ(m) = count of integers in [1,m] coprime to m. */
int acc0_euler_phi(int m);

/* Compute a^e mod m efficiently (fast exponentiation).
 * Complexity: O(log e). */
int acc0_mod_pow(int a, int e, int m);

/* ================================================================
 * L9: MOD gate lower bound parameters
 * ================================================================ */

/* Compute the Razborov-Smolensky threshold:
 * minimum ACC0 circuit size for a function known to be outside AC0.
 * This is an exponential lower bound: ~exp(n^{1/(2d)}).
 * n = number of inputs, d = depth. */
double acc0_razborov_size_lower_bound(int n, int d);

/* Demo functions */
void acc0_mod_gate_eval_demo(void);
void acc0_crt_demo(void);
void acc0_gate_type_demo(void);

#endif /* ACC0_GATES_H */
