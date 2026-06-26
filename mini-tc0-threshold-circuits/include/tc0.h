/* tc0.h -- TC0: Threshold Circuit Library
 *
 * TC0: The class of languages decidable by constant-depth, polynomial-size
 * circuits with unbounded fan-in MAJORITY and THRESHOLD gates.
 *
 * Hierarchy: AC0 ⊂ TC0 ⊆ NC1 ⊆ P  (where ⊂ is proper for AC0 ⊂ TC0)
 *
 * Key results:
 *   - MAJORITY ∉ AC0 (Furst-Saxe-Sipser 1981, Håstad 1987)
 *   - MAJORITY ∈ TC0 (trivially: one MAJ gate)
 *   - TC0 ⊆ NC1 (Barrington 1989 via polynomial representation)
 *   - SORTING, MULTIPLICATION, DIVISION all in TC0
 *   - TC0 vs NC1: open since 1989 whether TC0 = NC1
 *
 * TC0-complete problems (under AC0 reductions):
 *   - MAJORITY, THRESHOLD, SORTING, MULTIPLICATION, DIVISION
 *   - ITERATED ADDITION, ITERATED MULTIPLICATION
 *   - COUNT (number of 1s in input)
 *
 * References:
 *   - Vollmer (1999) "Introduction to Circuit Complexity"
 *   - Barrington, Immerman, Straubing (1990) "On uniformity within NC1"
 *   - Beame, Cook, Hoover (1986) "Log depth circuits for division"
 */
#ifndef TC0_H
#define TC0_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <stdint.h>

/* ================================================================
 * SECTION 1: DATA TYPES (L1: Definitions)
 * ================================================================ */

/* Gate types for threshold circuits.
 *
 * A TC0 circuit may use:
 *   T_IN    - input literal (x_i or ¬x_i)
 *   T_CON   - constant (0 or 1)
 *   T_NOT   - negation (unbounded fan-in 1)
 *   T_AND   - AND gate (unbounded fan-in, but in TC0, not needed)
 *   T_OR    - OR gate (unbounded fan-in, but in TC0, not needed)
 *   T_MAJ   - MAJORITY gate: output 1 iff > half of inputs are 1
 *   T_THR   - THRESHOLD gate: output 1 iff ≥ k inputs are 1
 *   T_MODp  - MOD_p gate: output 1 iff sum of inputs ≡ 0 mod p
 *              (MOD3, MOD5 etc. - for ACC0 comparisons)
 *   T_WTHR  - WEIGHTED THRESHOLD: ∑ w_i·x_i ≥ θ
 *   T_COMP  - COMPARATOR: (min, max) output pair for sorting networks
 *   T_XOR   - XOR gate (PARITY, for NC1 comparisons)
 */
typedef enum {
    TC_INPUT   = 0,   /* Input literal */
    TC_CONST   = 1,   /* Constant 0/1 */
    TC_NOT     = 2,   /* Negation */
    TC_AND     = 3,   /* Unbounded AND */
    TC_OR      = 4,   /* Unbounded OR */
    TC_MAJ     = 5,   /* MAJORITY gate */
    TC_THR     = 6,   /* THRESHOLD(k) gate */
    TC_WTHR    = 7,   /* Weighted threshold: ∑ w_i·x_i ≥ θ */
    TC_MODP    = 8,   /* MOD_p gate (for ACC0) */
    TC_COMP    = 9,   /* Comparator (for sorting networks) */
    TC_XOR     = 10,  /* XOR (PARITY) */
    TC_NAND    = 11,  /* NAND gate */
    TC_NOR     = 12,  /* NOR gate */
    TC_MUX     = 13   /* Multiplexer: sel ? a : b */
} TC0GateType;

/* A single gate in a threshold circuit.
 *
 * For MAJ/THR gates, the gate evaluates to 1 iff:
 *   count(fan_in[i] == 1) > |fan_in|/2    (MAJ)
 *   count(fan_in[i] == 1) >= thr_k         (THR)
 *
 * For WTHR gates:
 *   ∑ weight[i] * input[fan_in[i]] >= thr_theta
 */
typedef struct {
    TC0GateType type;            /* Gate type */
    int         gate_id;         /* Unique gate identifier */

    /* Fan-in: inputs to this gate (indices into circuit's gate array) */
    int         fan_in[256];     /* Up to 256 fan-in wires */
    int         fan_in_count;    /* Number of actual fan-in wires */

    /* Fan-out: gates that use this gate's output (for DAG analysis) */
    int         fan_out[256];    /* Up to 256 fan-out gates */
    int         fan_out_count;

    /* For THR gates: threshold value k */
    int         thr_k;

    /* For WTHR gates: weight vector and threshold */
    double      weights[256];    /* Weight for each fan-in */
    double      thr_theta;       /* Bias/threshold for WTHR */

    /* For CONST gates: constant value */
    int         const_val;

    /* For MODp gates: modulus */
    int         mod_p;

    /* Gate metadata */
    int         depth;           /* Depth from inputs (computed lazily) */
    int         is_output;       /* 1 if this is a circuit output */

    /* For COMP gates (sorting networks): paired gate id */
    int         comp_pair;       /* Paired comparator gate */
} TC0Gate;

/* A threshold circuit = a DAG of TC0Gates.
 *
 * The circuit computes a Boolean function f : {0,1}^n → {0,1}^m.
 * Inputs are always the first n gates.
 * Outputs are specified by output_gate_ids.
 *
 * TC0 circuits must have:
 *   - Constant depth (independent of n)
 *   - Polynomial size (in n)
 *   - Unbounded fan-in MAJ/THR gates
 */
typedef struct {
    TC0Gate    *gates;           /* Array of all gates */
    int         num_gates;       /* Total gates allocated */
    int         gate_count;      /* Currently used gates */

    int         num_inputs;      /* Number of input bits (n) */
    int         num_outputs;     /* Number of output bits (m) */
    int        *output_gate_ids; /* Which gates are outputs */

    int         max_depth;       /* Maximum depth of circuit (computed) */
    int         total_wires;     /* Total wire count */

    /* Circuit metadata */
    char        name[128];       /* Descriptive name */
    int         is_uniform;      /* 1 if DLOGTIME-uniform construction */
} TC0Circuit;

/* A circuit family {C_n}_{n∈N} — the formal definition of a TC0 language.
 *
 * A language L ∈ TC0 if there exists a circuit family {C_n} where:
 *   - each C_n decides L ∩ {0,1}^n
 *   - depth(C_n) = O(1)
 *   - size(C_n) = n^{O(1)}
 *   - the family is DLOGTIME-uniform (or at least P-uniform)
 */
typedef struct {
    char        language_name[128];  /* e.g., "MAJORITY", "SORTING" */
    int         is_tc0;              /* Proven TC0 membership */
    int         is_tc0_complete;     /* TC0-complete under AC0 reductions */
    TC0Circuit *circuit_example;     /* Example circuit for some n */
    int         max_depth_bound;     /* Proven depth upper bound */
    double      size_exponent;       /* size = O(n^k) where k is this */
} TC0CircuitFamily;

/* Boolean function represented as a truth table.
 * For functions on n variables, truth table has 2^n entries.
 */
typedef struct {
    char        name[128];       /* Function name */
    int         num_vars;        /* Number of boolean variables */
    int         table_size;      /* 2^num_vars */
    int        *truth_table;     /* truth_table[assignment] ∈ {0,1} */
    int         is_monotone;     /* 1 if x ≤ y ⇒ f(x) ≤ f(y) */
    int         is_symmetric;    /* 1 if f depends only on # of 1s */
    int         is_self_dual;    /* 1 if f(¬x) = ¬f(x) */
} BoolFunc;

/* ================================================================
 * SECTION 2: CORE OPERATIONS (L2: Core Concepts)
 * ================================================================ */

/* --- Circuit lifecycle --- */

/* Allocate a new circuit with capacity for max_gates gates.
 * Returns NULL on allocation failure.
 * Complexity: O(max_gates) for calloc. */
TC0Circuit *tc0_circuit_create(int max_gates);

/* Free all memory associated with a circuit.
 * Complexity: O(1) + free(). Safe to call with NULL. */
void tc0_circuit_free(TC0Circuit *c);

/* Reset a circuit to empty state, preserving allocated memory.
 * Complexity: O(1). Useful for iterative construction. */
void tc0_circuit_reset(TC0Circuit *c);

/* --- Gate operations --- */

/* Add an INPUT gate. Returns gate index.
 * Input gates have id 0..n-1 and are the circuit's primary inputs. */
int tc0_add_input(TC0Circuit *c);

/* Add a CONSTANT gate with value val (0 or 1). */
int tc0_add_constant(TC0Circuit *c, int val);

/* Add a gate of specified type. Fan-in is initially empty; wire it up
 * with tc0_add_wire(). Returns gate index or -1 on error. */
int tc0_add_gate(TC0Circuit *c, TC0GateType type);

/* Add a wire from source gate to target gate. Both must exist.
 * Returns 0 on success, -1 on error (overflow, invalid indices). */
int tc0_add_wire(TC0Circuit *c, int from_gate, int to_gate);

/* Set the threshold k for a THR gate. gate must be of type TC_THR. */
int tc0_set_threshold(TC0Circuit *c, int gate_id, int k);

/* Set weights and threshold for a WTHR gate. */
int tc0_set_weighted_threshold(TC0Circuit *c, int gate_id,
                                const double *weights, int n_weights,
                                double theta);

/* Mark a gate as circuit output. Multiple outputs allowed. */
int tc0_set_output(TC0Circuit *c, int gate_id);

/* --- Circuit evaluation --- */

/* Evaluate a single gate given input assignment.
 * Uses memoization (visited array) for DAG evaluation.
 * visited[] must be initialized to -1, sized num_gates.
 * Returns gate value (0 or 1). */
int tc0_evaluate_gate(const TC0Circuit *c, int gate_id,
                      const int *input_assign, int *visited);

/* Evaluate entire circuit on an input assignment.
 * Returns 0/1 for single-output circuits.
 * For multi-output circuits, returns the AND of all outputs
 * (satisfies circuit outputs positive test). */
int tc0_evaluate_circuit(const TC0Circuit *c, const int *input_assign);

/* Evaluate all outputs separately.
 * output_vals[] must be pre-allocated to num_outputs.
 * Returns 0 on success. */
int tc0_evaluate_all_outputs(const TC0Circuit *c, const int *input_assign,
                              int *output_vals);

/* --- Circuit analysis --- */

/* Compute the depth of every gate. Gate depth = max fan-in depth + 1.
 * Input gates have depth 0.
 * Returns the maximum circuit depth. */
int tc0_compute_depth(TC0Circuit *c);

/* Verify the circuit is a valid DAG (no cycles).
 * Uses DFS-based cycle detection.
 * Returns 1 if valid DAG, 0 if cycle detected. */
int tc0_verify_dag(const TC0Circuit *c);

/* Count total number of gates by type. Useful for complexity analysis.
 * counts[] must be pre-allocated to TC_XOR+1 elements. */
void tc0_count_gate_types(const TC0Circuit *c, int *counts);

/* Compute circuit size metrics. */
int tc0_circuit_size(const TC0Circuit *c);     /* Total gates */
int tc0_circuit_wire_count(const TC0Circuit *c); /* Total wires */
int tc0_circuit_max_fanin(const TC0Circuit *c);  /* Maximum fan-in */

/* --- Circuit printing / debugging --- */

/* Print circuit structure in human-readable format. */
void tc0_print_circuit(const TC0Circuit *c);

/* Print a single gate with its fan-in. */
void tc0_print_gate(const TC0Circuit *c, int gate_id);

/* Export circuit to DOT format (for Graphviz visualization). */
void tc0_export_dot(const TC0Circuit *c, FILE *fp);

/* ================================================================
 * SECTION 3: THRESHOLD FUNCTIONS (L3: Mathematical Structures)
 * ================================================================ */

/* A linear threshold function (LTF) on n variables:
 *   f(x) = 1 iff ∑ w_i·x_i ≥ θ
 * where w_i are integer weights and θ is the threshold.
 *
 * LTF are exactly the functions computable by a single THR gate.
 */
typedef struct {
    int         num_vars;
    int        *weights;        /* w_1, ..., w_n (integers) */
    int         theta;          /* threshold */
    int         weight_sum;     /* ∑ |w_i| (for complexity bounds) */
} LinearThresholdFunc;

/* A polynomial threshold function (PTF) of degree d:
 *   f(x) = 1 iff p(x_1, ..., x_n) ≥ 0
 * where p is a degree-d multilinear polynomial with integer coefficients.
 *
 * deg(f) = minimum d such that f has a degree-d PTF representation.
 * TC0 ⊆ functions with polylog(n) PTF degree.
 */
typedef struct {
    int         num_vars;
    int         degree;         /* Polynomial degree */
    int         num_terms;      /* Number of monomials */
    int        *monomial_vars;  /* Flat array: [vars_per_monomial, v1, v2, ..., ...] */
    int        *coefficients;   /* Coefficient per monomial */
    int         theta;          /* Threshold value */
} PolyThresholdFunc;

/* ================================================================
 * SECTION 4: SORTING NETWORKS (L5: Algorithms/Methods)
 * ================================================================ */

/* A comparator = a 2-input, 2-output gate that sorts its inputs:
 *   (min, max) = (a∧b, a∨b) in Boolean; (min(a,b), max(a,b)) in integers
 */

/* Bitonic sort: O(log² n) depth, O(n log² n) size.
 * Reference: Batcher (1968) "Sorting networks and their applications" */
void tc0_bitonic_sort(int *a, int lo, int cnt, int dir);
void tc0_bitonic_merge(int *a, int lo, int cnt, int dir);

/* Odd-even merge sort (Batcher's algorithm).
 * Depth: O(log² n), Size: O(n log² n). */
void tc0_oddeven_sort(int *a, int lo, int n);
void tc0_oddeven_merge(int *a, int lo, int n, int step);

/* Pairwise sorting network (simpler, O(log² n) depth). */
void tc0_pairwise_sort(int *a, int n);

/* Counting network: approximate counting without full sort.
 * Counts number of 1s in O(log n) depth using threshold gates. */
int tc0_count_ones(const int *bits, int n);

/* ================================================================
 * SECTION 5: ARITHMETIC IN TC0 (L5-L6)
 * ================================================================ */

/* Binary addition of two n-bit numbers.
 * In TC0 via carry-lookahead: O(log n) depth, O(n) size.
 * result must be n+1 bits (includes carry out). */
void tc0_add_binary(const int *a, const int *b, int *result, int n);

/* Iterated addition: sum of m n-bit numbers.
 * In TC0 via 3-to-2 carry-save reduction: O(log m) depth.
 * Result has n + ceil(log2(m)) bits. */
void tc0_iterated_add(const int **numbers, int m, int n, int *result);

/* Binary multiplication: n-bit × n-bit → 2n-bit.
 * In TC0 via iterated addition of partial products.
 * Schoolbook method shown; optimal is O(log n) depth, O(n²) size. */
void tc0_multiply_binary(const int *a, const int *b, int *product, int n);

/* Population count (number of 1s in input).
 * In TC0 via iterated 3-to-2 reduction: O(log n) depth, O(n) size. */
int tc0_popcount(const int *bits, int n);

/* Integer division (Beame-Cook-Hoover 1986).
 * In TC0 via Newton iteration + multiplication.
 * Returns quotient q = a / b for n-bit integers. */
void tc0_divide_binary(const int *a, const int *b, int *quotient,
                        int *remainder, int n);

/* ================================================================
 * SECTION 6: TC0-COMPLETE CIRCUIT CONSTRUCTORS
 * ================================================================ */

/* Build a TC0 circuit for MAJORITY_n.
 * Depth: 1 (single MAJ gate). Size: n+1.
 * This is trivial but establishes MAJORITY ∈ TC0. */
TC0Circuit *tc0_build_majority(int n);

/* Build a TC0 circuit for THRESHOLD(n, k).
 * Depth: 1. Size: n+1. */
TC0Circuit *tc0_build_threshold(int n, int k);

/* Build a TC0 circuit for PARITY_n (using only THR gates).
 * Proven: requires O(n) gates in depth 2, or O(√n) gates with larger depth.
 * This is NOT efficient — we use it to demonstrate the boundary of TC0. */
TC0Circuit *tc0_build_parity_threshold(int n);

/* Build a comparator network for sorting n elements.
 * This constructs a TC0 circuit using COMP gates.
 * The circuit sorts n k-bit numbers in O(log² n) depth. */
TC0Circuit *tc0_build_sorting_network(int n, int bits_per_element);

/* Build a TC0 circuit for n-bit multiplication.
 * Uses iterated addition: depth O(log n), size O(n²). */
TC0Circuit *tc0_build_multiplication(int n);

/* ================================================================
 * SECTION 7: BOOLEAN FUNCTION ANALYSIS (L3-L4)
 * ================================================================ */

/* Check if a Boolean function (given as truth table) is symmetric.
 * Symmetric functions depend only on the number of 1s in the input. */
int tc0_is_symmetric(const BoolFunc *f);

/* Check if a Boolean function is monotone:
 *   x ≤ y (bitwise) ⇒ f(x) ≤ f(y) */
int tc0_is_monotone(const BoolFunc *f);

/* Check if a Boolean function is self-dual:
 *   f(¬x_1, ..., ¬x_n) = ¬f(x_1, ..., x_n) */
int tc0_is_self_dual(const BoolFunc *f);

/* Check if a Boolean function is linearly separable (single THR gate).
 * True iff f is computable by one linear threshold function.
 * Uses the perceptron algorithm to test separability.
 * For small n (< 20), enumerates all possible weight assignments. */
int tc0_is_linear_separable(const BoolFunc *f);

/* Compute the influence of variable i on function f.
 * Inf_i(f) = Pr_x[f(x) ≠ f(x⊕i)] where x⊕i flips bit i.
 * High influence variables are "important" for the function. */
double tc0_variable_influence(const BoolFunc *f, int var_idx);

/* Compute total influence (sum of all variable influences).
 * Related to circuit size lower bounds via Hastad's switching lemma. */
double tc0_total_influence(const BoolFunc *f);

/* ================================================================
 * SECTION 8: THRESHOLD CIRCUIT SEARCH (L4-L8)
 * ================================================================ */

/* Enumerate all possible threshold circuits with given parameters.
 * Returns the number of distinct circuits (exact for small params). */
double tc0_enumerate_circuits(int n_vars, int n_gates, int max_fanin);

/* Find a minimal threshold circuit for a given Boolean function.
 * Uses exhaustive search (NP-hard in general, feasible for n ≤ 8).
 * Returns circuit or NULL if none found within search bounds. */
TC0Circuit *tc0_find_minimal_circuit(const BoolFunc *f,
                                       int max_gates, int max_fanin);

/* Compute the PTF degree of a Boolean function.
 * PTF degree = minimum d such that f has a polynomial threshold
 * function representation of degree d. */
int tc0_compute_ptf_degree(const BoolFunc *f);

/* ================================================================
 * SECTION 9: DEMO FUNCTIONS
 * ================================================================ */

void tc0_demo(void);
void tc0_majority_demo(void);
void tc0_sorting_demo(void);
void tc0_arithmetic_demo(void);
void tc0_completeness_demo(void);
void tc0_neural_demo(void);
void tc0_search_demo(void);
void tc0_benchmark_demo(void);

#endif /* TC0_H */
