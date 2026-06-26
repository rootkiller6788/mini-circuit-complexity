/* circuit_analysis.h -- Circuit Analysis, Verification, and Transformation
 *
 * Functions for analyzing circuit structure, verifying properties,
 * and transforming circuits between representations.
 *
 * References:
 *   - Vollmer (1999) Ch.4: Reductions and completeness
 *   - Jukna (2012) Ch.3: Circuit lower bound methods
 *   - AB (2009) Ch.6.3: Circuit lower bounds
 */

#ifndef CIRCUIT_ANALYSIS_H
#define CIRCUIT_ANALYSIS_H

#include "circuit.h"

/* ===================================================================
 * L5: Gate-Level Analysis
 * =================================================================== */

/* Count gates of each type in the circuit.
 * Output arrays are filled with counts per GateType.
 * gates_by_type must have 11 entries (one per GateType). */
void circuit_count_gates(const BooleanCircuit* c, int gates_by_type[11]);

/* Compute fan-in distribution: min, max, average.
 * Only considers non-input, non-constant gates. */
void circuit_fanin_stats(const BooleanCircuit* c, int* min_fi, int* max_fi, double* avg_fi);

/* Compute fan-out distribution: min, max, average.
 * Fan-out of gate g = number of gates that use g as input. */
void circuit_fanout_stats(const BooleanCircuit* c, int* min_fo, int* max_fo, double* avg_fo);

/* Gate type distribution as percentages.
 * pct array must have 11 entries. */
void circuit_type_distribution(const BooleanCircuit* c, double pct[11]);

/* Compute level-width at each level of the circuit.
 * Returns array via n_levels. Caller must free.
 * Circuit is leveled: level 0 = inputs, level d = outputs. */
int* circuit_level_widths(const BooleanCircuit* c, int* n_levels);

/* ===================================================================
 * L5: Circuit Verification & Properties
 * =================================================================== */

/* Check if two circuits compute the same function on ni inputs.
 * Brute-force: enumerates all 2^ni input combinations.
 * Limited to ni <= 16 for practicality.
 * Complexity: O(2^ni * (size_a + size_b)). */
int circuits_equivalent(const BooleanCircuit* a, const BooleanCircuit* b, int ni);

/* Check if circuit computes constant 1 on all inputs.
 * coNP-complete in general. Brute-force for ni <= 16.
 * Complexity: O(2^ni * size). */
int circuit_is_tautology(const BooleanCircuit* c, int ni);

/* Check if circuit is satisfiable (outputs 1 on some input).
 * NP-complete in general (CIRCUIT-SAT). Brute-force for ni <= 16.
 * Complexity: O(2^ni * size). */
int circuit_is_satisfiable(const BooleanCircuit* c, int ni);

/* Count the number of satisfying assignments.
 * #P-complete in general. Brute-force for ni <= 16.
 * Complexity: O(2^ni * size). */
long long circuit_count_sat_assignments(const BooleanCircuit* c, int ni);

/* Verify that circuit computes given truth table.
 * truth[i] = expected output for input i (LSB-first).
 * Complexity: O(2^ni * size). */
int circuit_verify_truth_table(const BooleanCircuit* c, int ni, const int* truth);

/* ===================================================================
 * L5: Circuit Comparison
 * =================================================================== */

/* Compare two circuits by size. Returns -1 if a smaller, 1 if b smaller, 0 if equal. */
int circuit_compare_size(const BooleanCircuit* a, const BooleanCircuit* b);

/* Compare two circuits by depth. Returns -1 if a shallower, 1 if b shallower, 0 if equal. */
int circuit_compare_depth(const BooleanCircuit* a, const BooleanCircuit* b);

/* Compute size ratio a/b. */
double circuit_size_ratio(const BooleanCircuit* a, const BooleanCircuit* b);

/* Compare size-depth product: S(a)*D(a) vs S(b)*D(b). */
int circuit_compare_complexity(const BooleanCircuit* a, const BooleanCircuit* b);

/* ===================================================================
 * L5: Circuit Transformations
 * =================================================================== */

/* Convert any circuit to depth-2 DNF (OR of ANDs of literals).
 * This may cause exponential size blowup.
 * Complexity: O(2^n * size) for n inputs. */
BooleanCircuit* circuit_to_dnf(const BooleanCircuit* c);

/* Convert any circuit to depth-2 CNF (AND of ORs of literals).
 * Dual of DNF conversion. Exponential blowup possible.
 * Complexity: O(2^n * size). */
BooleanCircuit* circuit_to_cnf(const BooleanCircuit* c);

/* Convert to NAND-only circuit (functionally complete basis).
 * Every gate is replaced by an equivalent NAND subcircuit.
 * Size blowup: at most 3x. Depth blowup: at most 2x. */
BooleanCircuit* circuit_to_nand_only(const BooleanCircuit* c);

/* Convert to NOR-only circuit.
 * Size blowup: at most 3x. Depth blowup: at most 2x. */
BooleanCircuit* circuit_to_nor_only(const BooleanCircuit* c);

/* Convert bounded fan-in circuit to fan-in 2.
 * For MAJ/THR gates: decompose into tree of binary gates.
 * Size blowup: O(fanin) per MAJ/THR gate. */
BooleanCircuit* circuit_to_fanin2(const BooleanCircuit* c);

/* Remove redundant gates (gates whose output is never used).
 * Complexity: O(n_gates + n_edges). */
BooleanCircuit* circuit_remove_redundant(const BooleanCircuit* c);

/* Compose two circuits: C2's inputs are fed by C1's outputs.
 * C1 must have n_inputs2 outputs.
 * Implements f(g_1(x), ..., g_m(x)). Useful for reductions.
 * Complexity: O(size1 + size2). */
BooleanCircuit* circuit_compose(const BooleanCircuit* c1, const BooleanCircuit* c2);

/* Balance the circuit: restructure to reduce depth.
 * For associative gates (AND, OR, XOR), convert chains to trees.
 * Complexity: O(n_gates * log n_gates). Depth: O(log size). */
BooleanCircuit* circuit_balance(const BooleanCircuit* c);

/* Negate output: add a NOT gate after the output.
 * Complexity: O(1). */
BooleanCircuit* circuit_negate(const BooleanCircuit* c);

/* Repeat circuit n times for bitwise operation on n-bit inputs.
 * Complexity: O(n * size). */
BooleanCircuit* circuit_replicate(const BooleanCircuit* c, int n);

#endif /* CIRCUIT_ANALYSIS_H */
