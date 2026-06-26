/* circuit_builder.h -- Circuit Construction Functions
 *
 * Factory functions for building common Boolean circuits.
 * Each function constructs a circuit computing a specific Boolean function.
 *
 * References:
 *   - Vollmer (1999) Ch.1-3: circuit constructions
 *   - Jukna (2012) Ch.1: Boolean functions and their circuits
 *   - AB (2009) Ch.6.2: P/poly and circuit families
 */

#ifndef CIRCUIT_BUILDER_H
#define CIRCUIT_BUILDER_H

#include "circuit.h"

/* ===================================================================
 * L5: Basic Combinational Circuits
 * =================================================================== */

/* NAND2: computes NAND(x0, x1) = NOT(AND(x0, x1)).
 * NAND is universal: any Boolean function can be built from NAND gates.
 * Size: 2 gates, Depth: 2. */
BooleanCircuit* circuit_nand2(void);

/* NOR2: computes NOR(x0, x1) = NOT(OR(x0, x1)).
 * NOR is also universal. Size: 2 gates, Depth: 2. */
BooleanCircuit* circuit_nor2(void);

/* MUX: 2-to-1 multiplexer. if(s) return b else return a.
 * Size: 5 gates, Depth: 3. */
BooleanCircuit* circuit_mux2(void);

/* MUX4: 4-to-1 multiplexer with 2 select bits.
 * Size: ~15 gates, Depth: 4. */
BooleanCircuit* circuit_mux4(void);

/* Half adder: sum = a XOR b, carry = a AND b.
 * Size: 2 gates, Depth: 1. Two outputs: [sum, carry]. */
BooleanCircuit* circuit_half_adder(void);

/* Full adder: sum = a XOR b XOR cin, cout = MAJ(a,b,cin).
 * Size: ~6 gates, Depth: 3. Two outputs: [sum, cout]. */
BooleanCircuit* circuit_full_adder(void);

/* Equality: EQ(x0..xn-1, y0..yn-1) = AND_i (NOT(XOR(xi, yi))).
 * 2n inputs. Size: O(n), Depth: O(log n) if tree-reduced.
 * Note: caller provides n; circuit has 2n INPUT gates. */
BooleanCircuit* circuit_equality(int n);

/* Comparison (less than): LT(a[n-1:0], b[n-1:0]).
 * 2n inputs. Size: O(n), Depth: O(n) ripple. */
BooleanCircuit* circuit_comparator(int n);

/* ===================================================================
 * L5: Arithmetic Circuits
 * =================================================================== */

/* Ripple-carry adder: n-bit a + n-bit b -> n-bit sum + carry.
 * 2n inputs, n+1 outputs. Size: O(n), Depth: O(n). */
BooleanCircuit* circuit_adder(int n, int* carry_out);

/* Carry-lookahead adder: n-bit addition with O(log n) depth.
 * Size: O(n log n), Depth: O(log n). In NC^1. */
BooleanCircuit* circuit_cla_adder(int n, int* carry_out);

/* n-bit multiplier: a * b -> 2n-bit product.
 * Uses shift-and-add. Size: O(n^2), Depth: O(n log n). */
BooleanCircuit* circuit_multiplier(int n);

/* ===================================================================
 * L5: Symmetric Functions
 * =================================================================== */

/* PARITY: XOR of all n input bits. PARITY(x) = sum_i x_i (mod 2).
 * Circuit class: NOT in AC^0 (Furst-Saxe-Sipser, Hastad 1986).
 * In NC^1: balanced XOR tree. Size: O(n), Depth: O(log n). */
BooleanCircuit* circuit_parity(int n);

/* MAJORITY: 1 iff more than half of n inputs are 1.
 * Circuit class: TC^0 (can be done with threshold gates).
 * Via CSA tree. Size: O(n), Depth: O(log^2 n).
 * Also known to be in NC^1 via Valiant's construction. */
BooleanCircuit* circuit_majority(int n);

/* THRESHOLD(k): 1 iff at least k of n inputs are 1.
 * AC^0 via DNF (exponential size for k=n/2).
 * Size: O(n^(k+1)) as DNF. Depth: 2 (DNF). */
BooleanCircuit* circuit_threshold(int n, int k);

/* MOD-m: 1 iff sum of inputs ≡ 0 (mod m).
 * For m=2 this is PARITY. MOD-3 is not in AC^0 (Razborov-Smolensky).
 * Size: O(n) in NC^1 via symmetric circuit construction. */
BooleanCircuit* circuit_mod(int n, int m);

/* COUNT: outputs the binary representation of the number of 1's.
 * n inputs, ceil(log2(n+1)) outputs.
 * Size: O(n log n), Depth: O(log^2 n). TC^0. */
BooleanCircuit* circuit_count(int n);

/* SORT: n-bit sorting network. n inputs, n outputs sorted.
 * Uses Batcher's odd-even mergesort. Size: O(n log^2 n), Depth: O(log^2 n).
 * In NC^1 (actually in TC^0 via comparison gates). */
BooleanCircuit* circuit_sort(int n);

/* ===================================================================
 * L5: Graph-Theoretic Circuits
 * =================================================================== */

/* CLIQUE(k): 1 iff the n-vertex graph encoded in inputs has a k-clique.
 * Input encoding: n*(n-1)/2 bits for edges.
 * NP-complete for parameter k. Circuit size O(n^k) for fixed k.
 * Circuit class: P/poly for any fixed k. */
BooleanCircuit* circuit_clique(int n, int k);

/* HAMILTONIAN PATH: 1 iff the n-vertex graph has a Hamiltonian path.
 * Input encoding: n*(n-1)/2 bits for edges.
 * NP-complete. Circuit size O(n! * poly(n)) via brute force.
 * In P/poly for fixed n (trivially). */
BooleanCircuit* circuit_ham_path(int n);

/* MATCHING: 1 iff the n-vertex bipartite graph has a perfect matching.
 * 2 sets of n/2 vertices each, n^2/4 edge bits.
 * In P (Edmonds' algorithm), circuit size poly(n). In P/poly. */
BooleanCircuit* circuit_perfect_matching(int n);

/* ===================================================================
 * L5: Special Constructions
 * =================================================================== */

/* XOR of two individual inputs. Size: 1 gate, Depth: 1.
 * Inputs a, b are INPUT gate IDs within the circuit. */
BooleanCircuit* circuit_xor2(void);

/* Replicate a single input bit n times.
 * This is trivial in circuits: just use the input gate multiple times. */
BooleanCircuit* circuit_fanout(int n);

/* AND tree: AND of all n inputs. Depth: ceil(log2 n). */
BooleanCircuit* circuit_and_tree(int n);

/* OR tree: OR of all n inputs. Depth: ceil(log2 n). */
BooleanCircuit* circuit_or_tree(int n);

/* All 16 Boolean functions of 2 variables (truth table index 0-15).
 * This demonstrates the completeness of the standard basis. */
BooleanCircuit* circuit_boolean2(int func_index);

/* ===================================================================
 * L5: Random Circuit Generation
 * =================================================================== */

/* Generate a random circuit with ni inputs and ng total gates.
 * Gates are chosen uniformly from {AND, OR, XOR} with random wiring.
 * seed: random seed for reproducibility.
 * This implements Shannon's random circuit model (1949). */
BooleanCircuit* circuit_random(int ni, int ng, unsigned int seed);

#endif /* CIRCUIT_BUILDER_H */
