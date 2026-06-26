/* circuit.h -- Boolean Circuit Model: Core Data Structures and API
 *
 * Defines the fundamental Boolean circuit model as studied in:
 *   - Arora & Barak (2009) Ch.6: Boolean Circuits
 *   - Vollmer (1999) "Introduction to Circuit Complexity"
 *   - Jukna (2012) "Boolean Function Complexity"
 *   - Stanford CS358: Circuit Complexity
 *   - MIT 6.841: Advanced Complexity Theory
 *
 * A Boolean circuit is a directed acyclic graph where:
 *   - Input nodes (no incoming edges) are labeled by variables x_i.
 *   - Internal nodes (gates) are labeled by AND, OR, NOT.
 *   - Output nodes produce the function value.
 *
 * Circuit complexity classes:
 *   AC^0: constant depth, unbounded fan-in, poly size
 *   NC^1: O(log n) depth, bounded fan-in, poly size
 *   AC^1: O(log n) depth, unbounded fan-in, poly size
 *   TC^0: constant depth, threshold gates, poly size
 *   P/poly: poly-size circuit families (non-uniform)
 *   NC: polylog depth, poly size = efficient parallel
 */

#ifndef CIRCUIT_H
#define CIRCUIT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include <time.h>

/* ============================================================================
 * L1: Core Definitions
 * ============================================================================ */

/* Gate types: the fundamental building blocks of Boolean circuits.
 * CONST0 and CONST1 are zero-input gates for constants.
 * Threshold gates MAJ and THR(k) are used in TC^0 circuits. */
typedef enum {
    INPUT  = 0,  /* variable x_i */
    AND    = 1,  /* binary AND gate */
    OR     = 2,  /* binary OR gate */
    NOT    = 3,  /* unary NOT gate (input1 only) */
    CONST0 = 4,  /* constant 0 */
    CONST1 = 5,  /* constant 1 */
    XOR    = 6,  /* binary XOR gate */
    NAND   = 7,  /* NAND gate = NOT(AND) */
    NOR    = 8,  /* NOR gate = NOT(OR) */
    MAJ    = 9,  /* majority gate (unbounded fan-in, outputs 1 iff >half inputs=1) */
    THR    = 10  /* threshold gate (parameterized by k) */
} GateType;

/* A single gate in the circuit DAG.
 *   id:       unique identifier within the circuit
 *   type:     gate operation
 *   input1:   first input gate id (-1 if none)
 *   input2:   second input gate id (-1 if unary)
 *   fanin:    number of inputs for MAJ/THR gates
 *   value:    cached evaluation result (-1 = unevaluated)
 *   label:    optional string label for the gate
 */
typedef struct {
    int      id;
    GateType type;
    int      input1;
    int      input2;
    int      fanin;        /* for MAJ/THR gates: number of inputs */
    int*     fanin_ids;    /* for MAJ/THR gates: array of input gate ids */
    int      value;        /* memoization cache: -1 = dirty */
    int      level;        /* topological level (for depth computation) */
} Gate;

/* A Boolean circuit: a DAG of gates computing a function f:{0,1}^n -> {0,1}^m.
 *
 * Key invariants:
 *   - The gate graph must be acyclic (DAG).
 *   - For INPUT gates, input1/input2 = -1.
 *   - For CONST0/CONST1, input1/input2 = -1.
 *   - For NOT gates, only input1 is used; input2 = -1.
 *   - For MAJ/THR gates, fanin and fanin_ids are used instead of input1/input2.
 *   - output_ids points to gates that are the circuit outputs.
 *
 * Size(c)  = n_gates (total gate count)
 * Depth(c) = length of longest path from input to output
 * Width(c) = max gates at any single level
 */
typedef struct {
    Gate* gates;           /* array of all gates */
    int   n_gates;         /* total number of gates */
    int   max_gates;       /* allocated capacity */
    int   n_inputs;        /* number of INPUT gates */
    int   n_outputs;       /* number of output gates */
    int*  output_ids;      /* array of output gate ids */
    int   n_edges;         /* total number of wires */
    int   depth_cache;     /* cached depth (-1 if dirty) */
    int   basis;           /* 0=standard (AND,OR,NOT), 1=NAND-only, 2=NOR-only */
} BooleanCircuit;

/* ============================================================================
 * Forward declarations of complex types used across headers
 * ============================================================================ */

/* CircuitFamily: a sequence of circuits {C_n} for inputs of length n */
typedef struct CircuitFamily CircuitFamily;

/* CircuitClass: enumeration of complexity classes */
typedef enum {
    CLASS_AC0, CLASS_AC1, CLASS_AC2,
    CLASS_NC1, CLASS_NC2,
    CLASS_TC0, CLASS_TC1,
    CLASS_P_POLY, CLASS_L_POLY,
    CLASS_NC, CLASS_AC,
    CLASS_UNKNOWN
} CircuitClass;

/* ============================================================================
 * L1 + L2: Core Circuit API
 * ============================================================================ */

/* Create a new Boolean circuit with pre-allocated capacity.
 * Complexity: O(1). */
BooleanCircuit* circuit_create(int max_gates);

/* Add a gate to the circuit. Returns the gate id.
 * Binary gate: in1, in2 = input gate ids.
 * Unary gate:  in2 = -1.
 * Leaf gate:   both = -1.
 * Complexity: O(1) amortized. */
int circuit_add_gate(BooleanCircuit* c, GateType type, int in1, int in2);

/* Add a MAJ/THR gate with unbounded fan-in.
 * fanin: number of input gates
 * fanin_ids: array of input gate ids (copied internally)
 * Complexity: O(fanin). */
int circuit_add_maj_gate(BooleanCircuit* c, GateType type, int fanin, const int* fanin_ids);

/* Set the output gates of the circuit.
 * ids: array of gate ids that produce output.
 * Complexity: O(n). */
void circuit_set_output(BooleanCircuit* c, const int* ids, int n);

/* Evaluate the circuit on given input bits.
 * inputs[i] = value of INPUT gate i (0 or 1).
 * Returns the AND of all output bits (for multi-output circuits,
 *   use circuit_evaluate_all).
 * Uses memoization: O(size) per evaluation.
 * Complexity: O(n_gates) worst case. */
int circuit_evaluate(BooleanCircuit* c, const int* inputs);

/* Evaluate all outputs, returns array via out_count.
 * Caller must free the returned array.
 * Complexity: O(n_gates). */
int* circuit_evaluate_all(BooleanCircuit* c, const int* inputs, int* out_count);

/* Compute circuit depth = longest input-to-output path.
 * Complexity: O(n_gates). Result is cached. */
int circuit_depth(const BooleanCircuit* c);

/* Return circuit size = total number of gates.
 * Complexity: O(1). */
int circuit_size(const BooleanCircuit* c);

/* Compute circuit width = max gates at any level.
 * Complexity: O(n_gates). */
int circuit_width(const BooleanCircuit* c);

/* Return number of input gates.
 * Complexity: O(1). */
int circuit_input_count(const BooleanCircuit* c);

/* Return number of output gates.
 * Complexity: O(1). */
int circuit_output_count(const BooleanCircuit* c);

/* Free all memory associated with the circuit.
 * Complexity: O(n_gates). */
void circuit_free(BooleanCircuit* c);

/* Print a human-readable summary of the circuit.
 * Complexity: O(n_gates). */
void circuit_print(const BooleanCircuit* c);

/* Print a detailed gate-by-gate listing.
 * Complexity: O(n_gates). */
void circuit_print_detailed(const BooleanCircuit* c);

/* Reset evaluation cache (for re-evaluation).
 * Complexity: O(n_gates). */
void circuit_reset(BooleanCircuit* c);

/* Count active gates during last evaluation.
 * Complexity: O(n_gates). */
int circuit_active_gates(BooleanCircuit* c, const int* inputs);

/* ============================================================================
 * L5: Circuit Evaluation Engine
 * ============================================================================ */

/* Evaluate in topological order (no recursion, no memoization).
 * Useful for parallel evaluation simulation.
 * Complexity: O(n_gates). */
int circuit_evaluate_topological(BooleanCircuit* c, const int* inputs);

/* Compute the topological order of gates.
 * order must be pre-allocated with n_gates entries.
 * Complexity: O(n_gates). */
void circuit_topological_order(const BooleanCircuit* c, int* order);

/* Benchmark: evaluate circuit repeatedly and return average microseconds.
 * Complexity: O(iterations * n_gates). */
double circuit_bench_eval(BooleanCircuit* c, const int* inputs, int iterations);

/* ============================================================================
 * L3: Gate and Edge Queries
 * ============================================================================ */

/* Get the gate at a given index.
 * Returns NULL if index out of bounds. */
const Gate* circuit_get_gate(const BooleanCircuit* c, int id);

/* Count edges (wires) in the circuit.
 * Complexity: O(n_gates). */
int circuit_edge_count(const BooleanCircuit* c);

/* Check if the circuit is a valid DAG (no cycles).
 * Complexity: O(n_gates + n_edges). */
int circuit_is_dag(const BooleanCircuit* c);

/* Check if all gate references are valid.
 * Complexity: O(n_gates). */
int circuit_is_valid(const BooleanCircuit* c);

/* ============================================================================
 * L5: I/O and Serialization
 * ============================================================================ */

/* Export circuit as Graphviz DOT format.
 * Complexity: O(n_gates + n_edges). */
void circuit_export_dot(const BooleanCircuit* c, const char* filename);

/* Export circuit statistics to file.
 * Complexity: O(n_gates). */
void circuit_export_stats(const BooleanCircuit* c, const char* filename);

/* Import circuit from simple text format.
 * Format: one line per gate: I(nput) A(nd) O(r) N(ot) X(or) 0 1
 * Returns NULL on failure. */
BooleanCircuit* circuit_import_text(const char* filename);

/* Serialize circuit to binary format.
 * Returns malloc'd buffer; *size gets the byte count. */
uint8_t* circuit_serialize(const BooleanCircuit* c, size_t* size);

/* Deserialize circuit from binary format.
 * Returns NULL on failure. */
BooleanCircuit* circuit_deserialize(const uint8_t* data, size_t size);

#endif /* CIRCUIT_H */
