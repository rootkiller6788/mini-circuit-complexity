/* monotone_circuit.h -- Monotone Boolean Circuit Model
 *
 * A monotone Boolean circuit is a directed acyclic graph (DAG)
 * with gates of type AND, OR, and INPUT. NO NOT gates allowed.
 *
 * References:
 *   Razborov (1985), "Lower bounds for the monotone complexity 
 *     of some Boolean functions"
 *   Alon & Boppana (1987), "The monotone circuit complexity of 
 *     Boolean functions"
 *   Vollmer (1999), "Introduction to Circuit Complexity", Ch. 4
 *   Jukna (2012), "Boolean Function Complexity", Ch. 9
 *   Arora & Barak (2009), "Computational Complexity", Ch. 14.4
 */
#ifndef MONOTONE_CIRCUIT_H
#define MONOTONE_CIRCUIT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>

/* ============================================================
 * L1 Definitions: Gate Types and Circuit Structure
 * ============================================================ */

/* Gate types in a monotone circuit */
typedef enum {
    MONO_GATE_INPUT,
    MONO_GATE_AND,
    MONO_GATE_OR,
    MONO_GATE_CONST_0,
    MONO_GATE_CONST_1
} MonoGateType;

/* A single gate in a monotone circuit */
typedef struct MonoGate {
    MonoGateType type;
    int          id;
    int          input_idx;    /* For INPUT: which variable x_i */
    int          fanin[2];     /* Gate IDs of inputs (-1 if none) */
    int          fanin_count;
    int          level;
    int          value;
    int          evaluated;
} MonoGate;

/* A monotone Boolean circuit: DAG of AND/OR/INPUT gates */
typedef struct {
    MonoGate     *gates;
    int           num_gates;
    int           num_inputs;
    int           output_gate;
    int           num_and;
    int           num_or;
    int           size;
    int           depth;
    int           capacity;
} MonotoneCircuit;

/* ============================================================
 * L1 Definitions: Monotone Boolean Function
 * ============================================================ */

typedef struct {
    int     n;
    int    *truth_table;
    char   *name;
} MonotoneBooleanFunction;

/* ============================================================
 * L2 Core Concepts: Circuit Construction & Manipulation
 * ============================================================ */

MonotoneCircuit* mono_circuit_create(int num_inputs);
int mono_circuit_add_input(MonotoneCircuit *c, int var_idx);
int mono_circuit_add_and(MonotoneCircuit *c, int in1, int in2);
int mono_circuit_add_or(MonotoneCircuit *c, int in1, int in2);
int mono_circuit_add_const(MonotoneCircuit *c, int value);
void mono_circuit_set_output(MonotoneCircuit *c, int gate_id);
void mono_circuit_free(MonotoneCircuit *c);

/* Evaluate circuit on input assignment. Returns output value. */
int mono_circuit_evaluate(MonotoneCircuit *c, const int *input_bits);

/* Evaluate on all 2^n inputs, fill truth table */
int mono_circuit_truth_table(MonotoneCircuit *c, int *truth_table);

/* Verify monotonicity: f(x) <= f(y) whenever x <= y component-wise */
int mono_verify_monotonicity(const int *truth_table, int n);

int mono_circuit_size(const MonotoneCircuit *c);
int mono_circuit_depth(MonotoneCircuit *c);
void mono_circuit_count_gates(const MonotoneCircuit *c,
                               int *num_and, int *num_or,
                               int *num_input, int *num_const);

/* Balance circuit to reduce depth using associative law */
MonotoneCircuit* mono_circuit_balance(const MonotoneCircuit *c);

/* Convert formula string to circuit (prefix: & for AND, | for OR) */
MonotoneCircuit* mono_formula_to_circuit(const char *formula);

/* Convert circuit to formula string. Caller must free. */
char* mono_circuit_to_formula(const MonotoneCircuit *c);

/* Evaluate with caching */
int mono_circuit_evaluate_memoized(MonotoneCircuit *c, const int *input_bits);

/* Topological sort for efficient bottom-up evaluation */
int* mono_circuit_topological_order(const MonotoneCircuit *c);

/* Write to DOT format for Graphviz */
void mono_circuit_write_dot(const MonotoneCircuit *c, FILE *out);

/* Print statistics */
void mono_circuit_print_stats(const MonotoneCircuit *c);

/* Eliminate redundant gates (idempotence) */
MonotoneCircuit* mono_circuit_eliminate_redundancy(const MonotoneCircuit *c);

/* Constant propagation */
MonotoneCircuit* mono_circuit_constant_propagation(const MonotoneCircuit *c);

/* Negation-limited circuit (Fischer 1974) */
typedef struct {
    int             num_not_gates;
    int            *not_gate_ids;
    int             not_gate_count;
    MonotoneCircuit **layers;
    int             num_layers;
} NegationLimitedCircuit;

#endif /* MONOTONE_CIRCUIT_H */
