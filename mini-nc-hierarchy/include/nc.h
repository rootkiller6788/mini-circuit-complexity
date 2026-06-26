/* nc.h -- NC Hierarchy: NC^0, AC^0, TC^0, NC^1, NC^2, ..., NC = polylog depth
 *
 * Nick's Class (Pippenger 1979): problems solvable by circuits of
 * polynomial size and polylogarithmic depth.
 *
 * Formal definition (L1):
 *   NC^i = { L | ∃ uniform Boolean circuit family {C_n} deciding L,
 *           size(C_n) = poly(n), depth(C_n) = O(log^i n), fan-in = 2 }
 *   NC   = ∪_{i≥0} NC^i
 *
 *   AC^i = same but with unbounded fan-in AND/OR gates
 *   TC^i = same but also allows MAJORITY (threshold) gates
 *
 * Known hierarchy (L4):
 *   NC^0 ⊊ AC^0 ⊊ TC^0 ⊆ NC^1 ⊆ L ⊆ NL = coNL ⊆ NC^2 ⊆ NC ⊆ P
 *
 * Major theorems (L4):
 *   - AC^0 ≠ NC^1: Furst-Saxe-Sipser 1981, Ajtai 1983, Håstad 1986
 *   - NL = coNL: Immerman-Szelepcsényi 1988
 *   - Barrington's Theorem: NC^1 = bounded-width branching programs (1986)
 *   - NC ⊆ P (trivial); NC = P is OPEN
 *
 * NC^1-complete problems (L6):
 *   - PARITY (actually ∉ AC^0, but in NC^1)
 *   - Formula Evaluation (Buss 1987)
 *   - Addition of integers (under AC^0 reductions)
 *   - Graph reachability for bounded-width graphs
 *
 * NC^2-complete:
 *   - Matrix determinant (Berkowitz 1984)
 *   - Context-free language membership
 *   - Minimum spanning tree
 *
 * References:
 *   Arora & Barak (2009) Ch.6: Circuit Complexity
 *   Vollmer (1999): Introduction to Circuit Complexity
 *   Barrington (1986): Bounded-width branching programs
 */
#ifndef NC_H
#define NC_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <assert.h>
#include <stdint.h>
#include <limits.h>

/* ================================================================
 * L1: DEFINITIONS — Core data structures
 * ================================================================ */

/* Gate types: covers NC, AC, TC circuit models */
typedef enum {
    NC_INPUT,        /* input variable (fan-in 0) */
    NC_CONST_ZERO,   /* constant 0 */
    NC_CONST_ONE,    /* constant 1 */
    NC_NOT,          /* NOT gate (fan-in 1) */
    NC_AND,          /* AND gate (fan-in 2 for NC; arbitrary for AC) */
    NC_OR,           /* OR gate (fan-in 2 for NC; arbitrary for AC) */
    NC_NAND,         /* NAND gate (universal) */
    NC_NOR,          /* NOR gate */
    NC_XOR,          /* XOR gate (fan-in 2, parity) */
    NC_MAJORITY,     /* threshold (majority) gate — TC circuit model */
    NC_THRESHOLD,    /* general threshold gate T_k — TC circuit model */
    NC_MOD3,         /* MOD_3 gate — ACC circuit model */
    NC_OUTPUT        /* output marker */
} NCGateType;

/* A single gate in the circuit DAG */
typedef struct NCGate {
    NCGateType type;          /* gate operation */
    int        *inputs;       /* indices of input gates (null-terminated or counted) */
    int        num_inputs;    /* fan-in (≥0) */
    int        input_capacity;/* allocated capacity for inputs array */
    int        threshold;     /* threshold k for NC_THRESHOLD gates */
    int        input_bit;     /* for NC_INPUT: which input variable index */
    int        output_index;  /* for NC_OUTPUT: which output number (0-indexed) */
    int        cached_value;  /* memoized evaluation result (-1 = unevaluated) */
    int        topological_order; /* for scheduling */
} NCGate;

/* A Boolean circuit: directed acyclic graph of gates.
 * Mathematical structure (L3): C = (V, E, type, inputs, outputs)
 *   V = {0, ..., n-1} gates
 *   E ⊆ V × V with topological ordering (no cycles)
 *   type: V → GateType
 *   inputs: IN ⊆ V (gates with fan-in 0)
 *   outputs: OUT ⊆ V
 */
typedef struct {
    NCGate  *gates;          /* dynamic array of gates */
    int      num_gates;      /* current number of gates in the circuit */
    int      capacity;       /* allocated capacity */
    int      num_inputs;     /* number of input variables */
    int      num_outputs;    /* number of output bits */
    int     *output_gates;   /* indices of output gates */
    int      max_fan_in;     /* maximum fan-in (2 for NC, unbounded for AC) */
    double   log_depth;      /* ceil(log n) for depth purposes */
    int      has_been_toposorted; /* whether topological order has been computed */
} BoolCircuit;

/* Circuit family: {C_n}_{n∈N} — one circuit per input size */
typedef struct {
    BoolCircuit **circuits;  /* array of circuits indexed by input size */
    int           max_n;     /* maximum input size in family */
    int           min_n;     /* minimum input size */
    char         *name;      /* identifier for the family */
} CircuitFamily;

/* ================================================================
 * L1: NC^i hierarchy classification
 * ================================================================ */

/* Class identifiers for the NC/AC/TC hierarchy */
typedef enum {
    CLASS_NC0,    /* NC^0: constant depth, constant fan-in */
    CLASS_AC0,    /* AC^0: constant depth, unbounded fan-in */
    CLASS_TC0,    /* TC^0: constant depth, majority gates */
    CLASS_ACC0,   /* ACC^0: AC^0 + MOD gates */
    CLASS_NC1,    /* NC^1: O(log n) depth */
    CLASS_L,      /* L: logarithmic space */
    CLASS_NL,     /* NL: nondeterministic logarithmic space */
    CLASS_LOGDCFL,/* LogDCFL: log-depth deterministic CFL */
    CLASS_NC2,    /* NC^2: O(log^2 n) depth */
    CLASS_NC3,    /* NC^3: O(log^3 n) depth */
    CLASS_NC,     /* NC = ∪ NC^i */
    CLASS_P,      /* P: polynomial time */
    CLASS_PSPACE, /* PSPACE: polynomial space */
    CLASS_NC_POLY /* NC^i for general i */
} CircuitClass;

/* Bounds characterization for a circuit family */
typedef struct {
    CircuitClass class_id;
    int          depth_class;     /* i for NC^i (depth = O(log^i n)) */
    double       size_polynomial; /* exponent: size = O(n^k) */
    int          is_uniform;      /* 0 = non-uniform, 1 = uniform */
    int          gate_types;      /* bitmask of gate types used */
    char         description[256];
} ClassBounds;

/* ================================================================
 * L2: CORE CONCEPTS — Parallel time, work, efficiency
 * ================================================================ */

/* Brent's Theorem (1974): Any computation taking T sequential steps
 * can be simulated on p processors in time T/p + T_∞ where T_∞ is
 * the critical path length (circuit depth).
 *
 * Corollary: NC = problems with poly-log parallel time on poly processors.
 */
double brent_parallel_time(double T_seq, int num_processors, double T_inf);
int    brent_optimal_processors(double T_seq, double T_inf);
double brent_speedup(double T_seq, double T_par);
double brent_efficiency(double T_seq, double T_par, int num_processors);

/* PRAM model variants (Fortune-Wyllie 1978) */
typedef enum {
    PRAM_EREW,   /* Exclusive Read, Exclusive Write */
    PRAM_CREW,   /* Concurrent Read, Exclusive Write */
    PRAM_CRCW    /* Concurrent Read, Concurrent Write */
} PRAMModel;

/* Work-Depth model (Shiloach-Vishkin 1982) */
typedef struct {
    double work;        /* total operations = circuit size */
    double depth;       /* parallel time = circuit depth */
    int    processors;  /* PRAM processors needed */
    double work_efficiency; /* work / (best_sequential) */
    PRAMModel pram_model;
} WorkDepthAnalysis;

/* ================================================================
 * L2: Uniformity — connecting circuits to Turing machines
 * ================================================================ */

/* A circuit family {C_n} is uniform if there exists a Turing machine
 * that on input 1^n outputs a description of C_n using limited resources.
 *
 * Uniformity types (Ruzzo 1981, Barrington-Immerman-Straubing 1990):
 */
typedef enum {
    UNIFORM_NONE,         /* non-uniform */
    UNIFORM_DLOGTIME,     /* DLOGTIME-uniform (standard for AC^0) */
    UNIFORM_ALOGTIME,     /* ALOGTIME-uniform (standard for NC^1) */
    UNIFORM_LOGTIME,      /* L-uniform (logspace-uniform) */
    UNIFORM_PTIME,        /* P-uniform (polynomial-time uniform) */
    UNIFORM_EXPTIME       /* EXPTIME-uniform (trivial: all families) */
} UniformityType;

/* ================================================================
 * L5: ALGORITHMS — Parallel primitives
 * ================================================================ */

/* ---- Associative Binary Operation (for prefix/suffix/scan) ---- */
typedef int (*BinOp)(int a, int b);
typedef long long (*BinOpLL)(long long a, long long b);

/* ---- Parallel Prefix (scan) — fundamental NC^1 primitive ----
 * Ladner-Fischer (1980): prefix can be computed in O(log n) depth,
 * O(n) size with fan-in 2 gates.
 *
 * Application domains:
 *   1. Carry-lookahead addition (for NC^1 integer addition)
 *   2. Polynomial evaluation (Horner's rule parallelized)
 *   3. List ranking (Cole-Vishkin 1986)
 *   4. Tree contraction (Miller-Reif 1985)
 *   5. Lexical analysis / parsing
 */
void prefix_sequential(const int *x, int n, int *p, BinOp op);
void prefix_parallel_tree(const int *x, int n, int *p, BinOp op);
void prefix_ladner_fischer(const int *x, int n, int *p, BinOp op);
void prefix_kogge_stone(const int *x, int n, int *p, BinOp op);
void prefix_brent_kung(const int *x, int n, int *p, BinOp op);
int  verify_prefix(const int *x, int n, const int *p, BinOp op);

/* Generalized scan on long long integers */
void prefix_scan_ll(const long long *x, int n, long long *p, BinOpLL op);

/* Parallel suffix (reverse scan): s_i = x_i op x_{i+1} op ... op x_{n-1} */
void suffix_sequential(const int *x, int n, int *s, BinOp op);

/* Segmented scan: independent scans on segments separated by flags */
void segmented_scan(const int *x, const int *flags, int n, int *out, BinOp op);

/* ================================================================
 * L5: PARALLEL SORTING — Sorting networks (NC)
 * ================================================================ */

/* Compare-and-swap element */
void compare_swap(int *a, int *b);
void compare_swap_desc(int *a, int *b);

/* Bitonic Sort (Batcher 1968): O(log^2 n) depth, O(n log^2 n) size
 * Belongs to NC^2 (actually NC^1 for sorting network model).
 */
void bitonic_merge(int *a, int n, int dir);
void bitonic_sort_rec(int *a, int n, int dir);
void bitonic_sort(int *a, int n);

/* Odd-Even Merge (Batcher 1968): O(log n) depth merge network */
void odd_even_merge(int *a, int n);
void odd_even_merge_sort(int *a, int n);

/* AKT sorting network: O(log n) depth (Ajtai-Komlos-Szemeredi 1983)
 * Note: theoretically optimal depth but huge constant. This is a
 * demonstration of the asymptotic result, not the full AKS network.
 */
void aks_sort_demonstration(int *a, int n);

/* Pairwise sorting network */
void pairwise_sort(int *a, int n);

/* ================================================================
 * L5: PARALLEL ARITHMETIC in NC^1
 * ================================================================ */

/* Integer addition: NC^1-complete under AC^0 reductions
 * Implementation: carry-lookahead via parallel prefix on (g,p) signals.
 *
 * For n-bit numbers:
 *   Sequential (ripple-carry): depth O(n), size O(n)
 *   NC^1 (carry-lookahead):   depth O(log n), size O(n log n)
 *
 * generate: g_i = a_i AND b_i
 * propagate: p_i = a_i XOR b_i
 * carry recurrence: c_{i+1} = g_i OR (p_i AND c_i)
 *
 * The carry computation IS a parallel prefix with operator:
 *   (g,p) • (g',p') = (g OR (p AND g'), p AND p')
 */
void ripple_carry_add_bits(const int *a, const int *b, int n,
                           int *sum, int *carry_out);
void carry_lookahead_add_bits(const int *a, const int *b, int n, int *sum);
void carry_select_add_bits(const int *a, const int *b, int n, int *sum);
void carry_save_adder_3to2(const int *a, const int *b, const int *c,
                           int n, int *sum, int *carry);

/* Integer multiplication: NC^1 (Schönhage-Strassen not needed;
 * schoolbook parallelized is NC^1 via carry-save adder tree).
 * Wallace tree: O(log n) depth, O(n^2) size.
 * Dadda tree: slightly better area.
 */
void wallace_tree_multiply(const int *a, int na, const int *b, int nb, int *prod);
void dadda_tree_multiply(const int *a, int na, const int *b, int nb, int *prod);

/* Integer division: NC^2 (Beame-Cook-Hoover 1986);
 * actually in NC^1 via Newton iteration (Chiu-Davida-Litow 2001)
 */
int  restoring_division_bits(const int *dividend, const int *divisor,
                              int n, int *quotient, int *remainder);

/* ================================================================
 * L5: PARALLEL MATRIX OPERATIONS (NC^2)
 * ================================================================ */

/* Matrix multiplication: C = A × B, all n×n
 * Sequential: O(n^3)
 * NC: O(log n) depth, O(n^3) size (standard 3-loop parallelization)
 * Strassen: O(log n) depth, O(n^{log_2 7}) size
 */
void matrix_multiply_sequential(const double *A, const double *B,
                                 double *C, int n);
void matrix_multiply_nc(const double *A, const double *B,
                         double *C, int n);
void matrix_multiply_strassen(const double *A, const double *B,
                               double *C, int n);

/* Matrix powering: A^k for k up to n
 * Sequential: O(k n^3)
 * NC: O(log k · log n) depth via repeated squaring
 */
void matrix_power_nc(const double *A, int n, int k, double *result);
void matrix_power_modular(const int *A, int n, int k, int mod, int *result);

/* ================================================================
 * L5: GRAPH ALGORITHMS (NC^2)
 * ================================================================ */

/* Transitive closure: reachability in directed graphs.
 * Sequential: O(n^3) Floyd-Warshall
 * NC: O(log^2 n) depth using matrix multiplication over
 *     Boolean semiring (min-plus for distances).
 */
void transitive_closure_nc(const int *adj_matrix, int n, int *closure);
void all_pairs_shortest_path_nc(const int *adj_matrix, int n, int *dist);

/* Graph connectivity (undirected): in L (Reingold 2004),
 * also in NC^2 via matrix powering of adjacency + random walk.
 */
int graph_is_connected(const int *adj_matrix, int n);
void connected_components_nc(const int *adj_matrix, int n, int *component);

/* Minimum spanning tree: NC^2 (Cole-Klein-Tarjan 1991) */
void mst_boruvka_step(const double *weights, int n, int *mst_edges, int *num_edges);

/* Planar graph reachability: in NC^2 (via separator theorem) */
int planar_reachability(const int *vertex_edges, int n);

/* ================================================================
 * L6: CANONICAL PROBLEMS — Complete problems for NC classes
 * ================================================================ */

/* ---- PARITY function ----
 * parity(x_1, ..., x_n) = x_1 ⊕ x_2 ⊕ ... ⊕ x_n
 * In NC^1 (XOR tree of depth ⌈log n⌉)
 * NOT in AC^0 (Furst-Saxe-Sipser 1981; Håstad 1986)
 * This separation proves AC^0 ≠ NC^1.
 */
int  parity_function(const int *x, int n);
int *parity_circuit_evaluate(int n, const int *input_bits);
BoolCircuit *build_parity_circuit(int n);

/* ---- MAJORITY function ----
 * MAJ(x_1, ..., x_n) = 1 iff ∑ x_i ≥ n/2
 * In TC^0 (via depth-2 threshold circuits)
 * NOT in AC^0 (FSS 1981)
 * Open: whether MAJORITY ∈ NC^1 (TC^0 vs NC^1 problem)
 */
int majority_function(const int *x, int n);
int *majority_circuit_evaluate(int n, const int *input_bits);
BoolCircuit *build_majority_circuit(int n);

/* ---- MOD functions ----
 * MOD_k(x) = 1 iff ∑ x_i ≡ 0 (mod k)
 * MOD_p for prime p ∉ AC^0 (Razborov 1987, Smolensky 1987)
 * MOD_6 ∉ ACC^0 (still open for some k)
 */
int mod_k_function(const int *x, int n, int k);

/* ================================================================
 * L7: APPLICATIONS — Real-world uses of NC theory
 * ================================================================ */

/* VLSI layout: area-time tradeoffs from circuit depth bounds.
 * Thompson (1979): AT^2 = Ω(n^2) for any VLSI layout of an n-input function.
 * Brent-Kung (1982): tree-based layouts achieve AT^2 = O(n^2).
 */
typedef struct {
    double area;           /* normalized VLSI area */
    double time_per_op;    /* cycle time */
    double at2_bound;      /* AT^2 lower bound */
} VLSIBounds;

VLSIBounds vlsi_area_time_bound(int num_inputs, double depth);

/* Cryptography: NC and one-way functions.
 * If NC = P, then all poly-time computable functions have poly-log depth
 * circuits — would affect assumptions about OWF security.
 */
int  nc_crypto_one_way_candidate(const int *x, int n, int *output);

/* Formal verification: bounded model checking with NC circuits.
 * Converting sequential circuits to parallel form for faster SAT solving.
 */
void unroll_circuit_to_depth(const BoolCircuit *c, int k, BoolCircuit *unrolled);

/* ================================================================
 * L8: ADVANCED TOPICS
 * ================================================================ */

/* Barrington's Theorem (1986): NC^1 = languages recognized by
 * polynomial-length, width-5 permutation branching programs.
 *
 * A width-w branching program of length ℓ is a sequence of instructions
 * (i_t, σ_{t,0}, σ_{t,1}) where σ_t are permutations on [w].
 * On input x, the program composes permutations to compute acceptance.
 */
typedef struct {
    int  width;        /* width of branching program */
    int  length;       /* number of instructions */
    int *var_index;    /* which variable to read at each step */
    int *perm0;        /* permutation to apply if var=0 (flattened w×w) */
    int *perm1;        /* permutation to apply if var=1 (flattened w×w) */
} BranchingProgram;

BranchingProgram *branching_program_from_formula(void *formula, int num_vars);
int  branching_program_evaluate(BranchingProgram *bp, const int *input);
void branching_program_free(BranchingProgram *bp);

/* Razborov-Smolensky lower bounds (L8):
 * For prime p, MOD_p ∉ ACC[q] for any prime q ≠ p.
 * Proof uses polynomial approximation over GF(p).
 */
int  razborov_approximation_test(int n, int prime);

/* Natural Proofs barrier (Razborov-Rudich 1997):
 * Any "natural" proof of circuit lower bounds would break cryptography.
 * This implies circuit lower bounds for P/poly may be very hard.
 */
typedef struct {
    int is_constructive;   /* property computable in poly-time */
    int is_large;          /* property true for ≥ 2^n / poly(n) functions */
    int is_useful;         /* property false for all functions in P/poly */
} NaturalProperty;

int  is_natural_property(NaturalProperty *prop);

/* ================================================================
 * L9: RESEARCH FRONTIERS — Documentation level
 * ================================================================ */

/* Open problems in NC hierarchy:
 * 1. NC^1 vs TC^0 — can majority be computed by log-depth, fan-in 2 circuits?
 * 2. NC vs P — does every poly-time algorithm have a poly-log depth circuit?
 * 3. ACC^0 lower bounds — is there a problem in NEXP not in ACC^0?
 *    (Solved: Williams 2010, 2014 for NEXP ⊄ ACC^0. For lower classes: open)
 * 4. Uniform NC^1 vs non-uniform NC^1 — are they equal?
 */

/* ================================================================
 * FORMULA EVALUATION (NC^1-complete, Buss 1987)
 * ================================================================ */

typedef enum {
    FORM_INPUT,    /* x_i: variable leaf */
    FORM_AND,      /* AND node */
    FORM_OR,       /* OR node */
    FORM_NOT,      /* NOT node */
    FORM_CONST_0,  /* constant 0 */
    FORM_CONST_1   /* constant 1 */
} FormulaNodeType;

typedef struct FormulaNode {
    FormulaNodeType type;
    int             var_index;    /* for FORM_INPUT: which variable */
    int             const_value;  /* for constants: 0 or 1 */
    struct FormulaNode *left;
    struct FormulaNode *right;
    struct FormulaNode *parent;   /* for tree contraction */
    int             value;        /* computed value */
    int             evaluated;    /* flag */
    int             node_id;      /* unique identifier */
    int             contract_round; /* for parallel tree contraction */
} FormulaNode;

/* Core formula operations (L5: NC^1 tree contraction algorithm) */
FormulaNode *formula_leaf(int var_idx);
FormulaNode *formula_const(int val);
FormulaNode *formula_and(FormulaNode *l, FormulaNode *r);
FormulaNode *formula_or(FormulaNode *l, FormulaNode *r);
FormulaNode *formula_not(FormulaNode *c);
FormulaNode *formula_implies(FormulaNode *a, FormulaNode *b);
FormulaNode *formula_xor(FormulaNode *a, FormulaNode *b);
FormulaNode *formula_equiv(FormulaNode *a, FormulaNode *b);
FormulaNode *formula_majority3(FormulaNode *a, FormulaNode *b, FormulaNode *c);
void         formula_free(FormulaNode *f);
FormulaNode *formula_copy(const FormulaNode *f);
int          formula_size(const FormulaNode *f);
int          formula_depth(const FormulaNode *f);
int          formula_num_vars(const FormulaNode *f);

/* Evaluation methods */
int  formula_eval_recursive(const FormulaNode *f, const int *assignment);
int  formula_eval_nc1(const FormulaNode *f, int num_vars, const int *assignment);
void formula_to_circuit(const FormulaNode *f, BoolCircuit *c);

/* Formula generators (L6: canonical Boolean formulas) */
FormulaNode *formula_from_truth_table(const int *table, int num_vars);
FormulaNode *formula_parity(int num_vars);
FormulaNode *formula_majority(int num_vars);
FormulaNode *formula_threshold(int num_vars, int k);
FormulaNode *formula_cnf_to_formula(int **clauses, int num_clauses,
                                     int *clause_sizes, int num_vars);
FormulaNode *formula_dnf_to_formula(int **terms, int num_terms,
                                     int *term_sizes, int num_vars);

/* Tree contraction for parallel evaluation (L5) */
void tree_contraction_rake(FormulaNode *root, int *result, int num_vars,
                            const int *assignment);

/* ================================================================
 * CIRCUIT CONVERSIONS AND TRANSFORMATIONS
 * ================================================================ */

/* Convert between circuit models */
BoolCircuit *circuit_nc_to_ac(const BoolCircuit *nc_circuit);
BoolCircuit *circuit_add_threshold_gate(BoolCircuit *c, int threshold);
int          circuit_is_monotone(const BoolCircuit *c);
int          circuit_is_tree(const BoolCircuit *c);
int          circuit_is_formula(const BoolCircuit *c); /* every gate has fan-out 1 */

/* Circuit balancing (L5):
 * Any formula of size s can be rebalanced to depth O(log s)
 * (Spira 1971, Brent 1974, Bonet-Buss 1994)
 */
FormulaNode *formula_balance_spira(const FormulaNode *f);

/* ================================================================
 * CORE CIRCUIT API
 * ================================================================ */

/* Circuit construction */
BoolCircuit *circuit_create(int initial_capacity, int max_fan_in);
void         circuit_free(BoolCircuit *c);
int          circuit_add_gate(BoolCircuit *c, NCGateType type,
                              const int *inputs, int num_inputs);
int          circuit_add_input(BoolCircuit *c, int var_index);
int          circuit_add_output(BoolCircuit *c, int gate_index);
int          circuit_add_constant(BoolCircuit *c, int value);

/* Circuit query */
int  circuit_evaluate_gate(BoolCircuit *c, int gate_id,
                            const int *input_values, int *visited);
int *circuit_evaluate(BoolCircuit *c, const int *input_values);
int  circuit_depth(BoolCircuit *c);
int  circuit_size(BoolCircuit *c);
int  circuit_num_gates_of_type(BoolCircuit *c, NCGateType type);
int  circuit_fan_in_sum(BoolCircuit *c);

/* Circuit analysis */
void circuit_compute_topological_order(BoolCircuit *c);
int  circuit_has_cycle(BoolCircuit *c);
void circuit_print_stats(BoolCircuit *c);
void circuit_print_dot(BoolCircuit *c, FILE *f);

/* NC class analysis */
ClassBounds circuit_classify(const BoolCircuit *c, int input_size);
int         circuit_is_nc_i(const BoolCircuit *c, int i, int input_size);

/* ================================================================
 * CIRCUIT FAMILY OPERATIONS
 * ================================================================ */

CircuitFamily *circuit_family_create(const char *name, int max_n);
void           circuit_family_free(CircuitFamily *cf);
void           circuit_family_set(CircuitFamily *cf, int n, BoolCircuit *c);
BoolCircuit   *circuit_family_get(CircuitFamily *cf, int n);
ClassBounds    circuit_family_classify(CircuitFamily *cf);
int            circuit_family_is_uniform(CircuitFamily *cf,
                                          UniformityType u_type);

/* ================================================================
 * HIERARCHY ANALYSIS
 * ================================================================ */

/* Known inclusion relationships (L4) */
int  hierarchy_nc0_in_ac0(void);    /* NC^0 ⊂ AC^0 — Håstad 1986 */
int  hierarchy_ac0_in_tc0(void);    /* AC^0 ⊆ TC^0 — trivial inclusion */
int  hierarchy_ac0_neq_tc0(void);   /* AC^0 ≠ TC^0 — since PARITY ∉ AC^0 */
int  hierarchy_tc0_in_nc1(void);    /* TC^0 ⊆ NC^1 — trivial? Actually OPEN */
int  hierarchy_nc1_in_l(void);      /* NC^1 ⊆ L — Borodin 1977 */
int  hierarchy_l_in_nl(void);       /* L ⊆ NL — trivial */
int  hierarchy_nl_equals_conl(void);/* NL = coNL — Immerman-Szelepcsényi 1988 */
int  hierarchy_nl_in_nc2(void);     /* NL ⊆ NC^2 — Borodin et al. */
int  hierarchy_nc_in_p(void);       /* NC ⊆ P — trivial */
int  hierarchy_p_in_pspace(void);   /* P ⊆ PSPACE — trivial */

/* Compute depth function for NC^i classification */
double nc_i_depth_bound(int i, int n);  /* O(log^i n) */
int    is_depth_nc_i(double depth, int i, int n);

/* ================================================================
 * BENCHMARKS & VISUALIZATION
 * ================================================================ */

void nc_hierarchy_depth_table(int max_n);
void nc_hierarchy_class_table(void);
void nc_bench_demo(void);

/* ================================================================
 * DEMOS
 * ================================================================ */

void nc_hierarchy_demo(void);
void nc_adder_demo(void);
void parallel_prefix_demo(void);
void formula_eval_demo(void);
void bitonic_sort_demo(void);
void matrix_mult_demo(void);
void graph_reachability_demo(void);
void barrington_theorem_demo(void);
void vlsi_bounds_demo(void);
void circuit_conversion_demo(void);

#endif /* NC_H */
