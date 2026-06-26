/* circuit_synthesis.h -- Logic Synthesis and Technology Mapping
 *
 * Functions for synthesizing Boolean circuits from truth tables,
 * optimizing circuit representations, and mapping to target technologies.
 *
 * References:
 *   - De Micheli (1994) "Synthesis and Optimization of Digital Circuits"
 *   - Brayton, Hachtel, Sangiovanni-Vincentelli (1990)
 *   - Jukna (2012) Ch.4: Circuit complexity of Boolean functions
 */

#ifndef CIRCUIT_SYNTHESIS_H
#define CIRCUIT_SYNTHESIS_H

#include "circuit.h"

/* ===================================================================
 * L5: Truth Table Synthesis
 * =================================================================== */

/* Synthesize a DNF circuit from a truth table.
 * truth[m] = output for input combination m (bit-encoded, LSB first).
 * n_vars: number of input variables (<= 16 for practicality).
 * This is canonical DNF: OR of AND terms for each 1 in truth table.
 * Size: O(2^n * n), Depth: 2 (OR of ANDs).
 * In AC^0 (constant depth), but exponential size for worst case. */
BooleanCircuit* circuit_synthesize_dnf(const int* truth, int n_vars);

/* Synthesize a CNF circuit from a truth table.
 * Dual of DNF: AND of OR terms for each 0 in truth table.
 * Size: O(2^n * n), Depth: 2 (AND of ORs). */
BooleanCircuit* circuit_synthesize_cnf(const int* truth, int n_vars);

/* Synthesize using the Quine-McCluskey exact minimization algorithm.
 * Finds the minimum DNF (prime implicant cover) for small n (<= 6).
 * Returns a DNF circuit with minimum number of terms.
 * Complexity: O(3^n / n) — double exponential in worst case. */
BooleanCircuit* circuit_synthesize_qm(const int* truth, int n_vars);

/* Synthesize an XOR-sum-of-products (ESOP) using Reed-Muller expansion.
 * Every Boolean function has a unique Reed-Muller (algebraic normal) form.
 * Size: up to 2^n terms. For PARITY: 1 term!
 * Complexity: O(n * 2^n). */
BooleanCircuit* circuit_synthesize_esop(const int* truth, int n_vars);

/* Synthesize using Shannon decomposition:
 * f(x1,...,xn) = (NOT x1 AND f(0,x2,...,xn)) OR (x1 AND f(1,x2,...,xn))
 * This is used in BDD construction.
 * Complexity: O(2^n). Size: O(2^n). */
BooleanCircuit* circuit_synthesize_shannon(const int* truth, int n_vars);

/* ===================================================================
 * L5: Gate-Level Synthesis
 * =================================================================== */

/* Synthesize the minimal 2-level AND-OR circuit using the
 * exact Quine-McCluskey algorithm.
 * Returns the number of product terms used. */
int circuit_qm_minterms(const int* truth, int n_vars, int* minterms_out, int* n_minterms);

/* Find essential prime implicants of a Boolean function.
 * Returns number of essential prime implicants. */
int circuit_essential_primes(const int* truth, int n_vars, int* epi_out, int* n_epi);

/* Compute the prime implicant table for Quine-McCluskey.
 * Returns the number of prime implicants found. */
int circuit_prime_implicants(const int* truth, int n_vars, int* primes_out, int* n_primes);

/* ===================================================================
 * L5: Technology Mapping
 * =================================================================== */

/* Map circuit to a 2-input NAND-only basis.
 * Every gate is replaced with equivalent NAND subcircuit.
 * Size blowup factor <= 3. Depth blowup factor <= 2. */
BooleanCircuit* circuit_map_to_nand2(const BooleanCircuit* c);

/* Map circuit to a 2-input NOR-only basis.
 * Size blowup factor <= 3. Depth blowup factor <= 2. */
BooleanCircuit* circuit_map_to_nor2(const BooleanCircuit* c);

/* Map circuit to {AND, NOT} basis.
 * OR(a,b) = NOT(AND(NOT(a), NOT(b))). Minimal transformation. */
BooleanCircuit* circuit_map_to_and_not(const BooleanCircuit* c);

/* Map circuit to {NAND} only (functionally complete).
 * NAND(x,x) = NOT(x). NAND(NAND(a,b), NAND(a,b)) = AND(a,b).
 * NAND(NAND(a,a), NAND(b,b)) = OR(a,b). */
BooleanCircuit* circuit_map_to_nand(const BooleanCircuit* c);

/* Map circuit to threshold gates (for TC^0). */
BooleanCircuit* circuit_map_to_threshold(const BooleanCircuit* c);

/* ===================================================================
 * L5: Decomposition and Partitioning
 * =================================================================== */

/* Decompose circuit into k roughly equal parts.
 * Useful for parallel synthesis and hierarchical design.
 * Returns array of k subcircuits via sub_count. Caller must free each. */
BooleanCircuit** circuit_decompose(const BooleanCircuit* c, int k, int* sub_count);

/* Extract the cone of influence for a specific output gate.
 * All gates that affect this output are included.
 * Complexity: O(n_gates). */
BooleanCircuit* circuit_extract_cone(const BooleanCircuit* c, int output_id);

/* Factor common subexpressions from the circuit.
 * Identical subcircuits are merged into one.
 * Complexity: O(n_gates^2) with structural hashing. */
BooleanCircuit* circuit_factor_common(const BooleanCircuit* c);

/* ===================================================================
 * L8: Advanced Synthesis
 * =================================================================== */

/* Simulated annealing for circuit optimization.
 * Randomly perturbs the circuit and accepts improvements
 * (or degradations per Metropolis criterion).
 * Returns best circuit found.
 * temperature: initial temperature; cooling_rate: per-iteration multiplier.
 * iterations: number of annealing steps. */
BooleanCircuit* circuit_anneal_optimize(const BooleanCircuit* c,
    double temperature, double cooling_rate, int iterations);

/* Genetic algorithm for circuit synthesis.
 * Evolves a population of circuits to match a target truth table.
 * pop_size: population size; generations: number of generations.
 * mutation_rate: probability of mutation per gate. */
BooleanCircuit* circuit_genetic_synthesize(const int* truth, int n_vars,
    int pop_size, int generations, double mutation_rate);

/* BDD-based circuit synthesis: convert a BDD representation
 * to a multiplexer-based circuit.
 * The BDD is encoded as an array of nodes (var, low, high).
 * n_nodes: number of BDD nodes. n_vars: number of variables.
 * Returns a circuit implementing the BDD's function. */
BooleanCircuit* circuit_from_bdd(const int* bdd_nodes, int n_nodes, int n_vars);

#endif /* CIRCUIT_SYNTHESIS_H */
