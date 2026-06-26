#ifndef SWITCHING_H
#define SWITCHING_H

/**
 * mini-switching-lemma: Hastad Switching Lemma (1986)
 * ===================================================
 *
 * THEOREM (Hastad 1986, Godel Prize 1994):
 *   Let f be a k-DNF formula. Apply a random restriction rho where each
 *   variable is independently set to * (free) with probability p, and to
 *   0 or 1 each with probability (1-p)/2. Then for any s >= 1:
 *
 *     Pr[ f|_rho is not equivalent to an s-CNF ] < (5pk)^s
 *
 * CONSEQUENCE (PARITY not in AC0):
 *   Any depth-d AC0 circuit computing PARITY on n variables requires
 *   size at least exp(Omega(n^{1/(d-1)})). In particular, PARITY requires
 *   super-polynomial size for constant-depth circuits.
 *
 * REFERENCES:
 *   - Hastad, "Almost optimal lower bounds for small depth circuits" (1986)
 *   - Arora & Barak, "Computational Complexity: A Modern Approach", Ch.14
 *   - Furst, Saxe, Sipser (1981) + Ajtai (1983): First AC0 lower bounds
 *
 * KNOWLEDGE COVERAGE:
 *   L1: DNF, CNF, decision tree, restriction, AC0, PARITY definitions
 *   L2: Switching lemma, depth reduction, probabilistic method
 *   L3: Boolean algebra, probability spaces, circuit DAGs
 *   L4: Hastad Switching Lemma, PARITY lower bound, AC0 != NC1
 *   L5: Random restriction, iterative switching, DNF->CNF conversion
 *   L6: PARITY, MAJORITY, Threshold, modular counting
 *   L7: Pseudorandom generators, PAC learning, cryptography
 *   L8: Multi-switching, correlation bounds, average-case hardness
 *   L9: Current research directions (documented)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>

/* ================================================================
 * L1: CORE DEFINITIONS — Data Structures
 * ================================================================ */

/* Literal encoding: (variable_index << 1) | sign_bit
 *   sign_bit = 0: negated literal (~x_i)
 *   sign_bit = 1: positive literal (x_i)
 *   Special value: LITERAL_NULL = -1 (deleted/empty slot)
 */
#define LITERAL_NULL      (-1)
#define LITERAL_VAR(lit)  ((lit) >> 1)
#define LITERAL_SIGN(lit) ((lit) & 1)
#define LITERAL_POS       1       /* positive sign */
#define LITERAL_NEG       0       /* negated sign */
#define MAKE_LITERAL(var,sign) (((var) << 1) | ((sign) & 1))

/* Boolean values */
#define BOOL_TRUE   1
#define BOOL_FALSE  0
#define BOOL_UNDEF (-1)  /* for unassigned variables */

/* ---- DNF (Disjunctive Normal Form) ----
 * DNF = OR of ANDs. Each term is a conjunction of literals.
 * A k-DNF has at most k literals per term.
 * DNF(terms) = term_1 OR term_2 OR ... OR term_n
 *
 * Representation: flat array terms[t * k + l] for term t, literal l.
 * Unused slots within a term are set to LITERAL_NULL. */
typedef struct {
    int*  terms;      /* flat array: terms[term_idx * k_alloc + lit_pos] */
    int   n_terms;    /* number of terms in the OR */
    int   n_vars;     /* number of Boolean variables */
    int   k_alloc;    /* allocated width per term (>= actual max width) */
    int   is_cnf;     /* 0 = DNF (OR of ANDs), 1 = CNF (AND of ORs) */
} Formula;

/* ---- CNF (Conjunctive Normal Form) ----
 * CNF = AND of ORs. Each clause is a disjunction of literals.
 * An s-CNF has at most s literals per clause.
 * Reuses Formula struct (is_cnf = 1). */
typedef Formula CNF;
typedef Formula DNF;

/* ---- Decision Tree Node ----
 * Binary tree: internal nodes query a variable,
 * leaves store the output value (0 or 1).
 * Depth = maximum number of variables queried on any path. */
typedef struct DTNode {
    int   var;          /* variable to query, -1 for leaf */
    int   leaf_val;     /* output value (0/1) if leaf */
    struct DTNode* child0;  /* subtree when x_var = 0 */
    struct DTNode* child1;  /* subtree when x_var = 1 */
} DTNode;

/* ---- Random Restriction ----
 * A restriction rho: {x_1,...,x_n} -> {0, 1, *}
 * where * means "leave free", 0/1 means "fix to value".
 * value[i] = -1 for *, 0 for fixed-0, 1 for fixed-1. */
typedef struct {
    int   n_vars;      /* total number of variables */
    int*  values;      /* values[i] = -1(*), 0, or 1 */
    int   n_free;      /* count of * (free variables) */
} Restriction;

/* ---- AC0 Circuit Node ----
 * AC0 = constant-depth, unbounded fan-in, polynomial-size circuits
 * with AND, OR, NOT gates. Depth is the length of the longest
 * path from an input to the output. */
typedef enum {
    GATE_AND,
    GATE_OR,
    GATE_NOT,
    GATE_INPUT,
    GATE_OUTPUT
} GateType;

typedef struct CircuitNode {
    GateType type;
    int      var;          /* variable index (for INPUT gates) */
    int      fanin_count;
    struct CircuitNode** fanin;   /* array of input nodes */
    int      depth;        /* depth from inputs (0 for INPUT) */
    int      id;           /* unique node identifier */
} CircuitNode;

/* ---- AC0 Circuit ----
 * Top-level circuit structure with size, depth, and output gate. */
typedef struct {
    int          n_vars;      /* number of Boolean inputs */
    int          n_nodes;     /* total number of gates */
    int          depth;       /* circuit depth */
    CircuitNode* output;      /* output gate */
    CircuitNode** all_nodes;  /* all nodes for memory management */
} Circuit;

/* ---- Switching Lemma Bounds ----
 * The switching lemma provides a probability bound.
 * We track both theoretical bounds and experimental results. */
typedef struct {
    int    k;              /* DNF term width */
    double p;              /* star probability */
    int    s;              /* target CNF width */
    double theory_bound;   /* theoretical bound: (5pk)^s */
    int    trials;         /* number of trials run */
    int    successes;      /* number of successful switches */
    double exp_success_rate; /* experimental success rate */
    double avg_width_before;
    double avg_width_after;
    double avg_terms_before;
    double avg_terms_after;
} SwitchStats;

/* ---- Switching Round (for iterative depth reduction) ----
 * One round of switching: AND-of-ORs -> OR-of-ANDs (or vice versa).
 * After each round, the circuit depth decreases by 1.
 * With p = n^{-1/(d-1)}, after d-1 rounds, ~1 variable remains. */
typedef struct {
    int    depth;           /* circuit depth before this round */
    int    n_vars;          /* number of variables before this round */
    double p_star;          /* star probability for this round */
    int    surviving_vars;  /* expected variables after restriction */
    double width_bound_s;   /* target width bound for this round */
    double failure_prob;    /* probability switching fails this round */
} SwitchRound;

/* ---- Boolean Function Representation ----
 * For representing and manipulating Boolean functions
 * beyond DNF/CNF (truth tables, Fourier, etc.) */
typedef struct {
    int   n_vars;        /* number of variables */
    int   table_size;    /* 2^n_vars */
    int*  table;         /* truth table: table[assignment] = f(assignment) */
    char* name;          /* function name (e.g., "PARITY", "MAJORITY") */
} BoolFunc;

/* ---- Depth Reduction Result ----
 * Tracks the full iterative switching process on a depth-d circuit. */
typedef struct {
    int          n_initial;     /* initial number of variables */
    int          depth;         /* initial circuit depth */
    int          n_rounds;      /* d-1 rounds of switching */
    SwitchRound* rounds;        /* per-round data */
    int          final_vars;    /* remaining free variables */
    double       size_lower;    /* lower bound from Hastad: exp(Omega(n^{1/(d-1)})) */
    int          parity_hard;   /* 1 if size_lower is super-polynomial */
} DepthReduction;

/* ================================================================
 * L1-L3: FORMULA (DNF/CNF) API
 * ================================================================ */

/* Create a DNF formula with n_vars variables and n_terms terms.
 * k_alloc is the allocated width per term. */
DNF* dnf_create(int n_vars, int n_terms, int k_alloc);

/* Create a CNF formula (AND of ORs). */
CNF* cnf_create(int n_vars, int n_clauses, int s_alloc);

/* Set a literal in a specific position of a DNF term.
 * term: term index, lit_pos: position within term,
 * var: variable index, sign: LITERAL_POS or LITERAL_NEG. */
void dnf_set_literal(DNF* d, int term, int lit_pos, int var, int sign);

/* Set a literal in a CNF clause. */
void cnf_set_literal(CNF* c, int clause, int lit_pos, int var, int sign);

/* Evaluate a formula (DNF or CNF) on a given assignment.
 * assign[v] = 0 or 1 for each variable v. */
int  formula_evaluate(const Formula* f, const int* assign);

/* Evaluate DNF on an assignment. */
int  dnf_evaluate(const DNF* d, const int* assign);

/* Evaluate CNF on an assignment. */
int  cnf_evaluate(const CNF* c, const int* assign);

/* Generate a random k-DNF with n_vars variables and n_terms terms.
 * Each term has exactly k distinct literals, signs chosen randomly. */
DNF* dnf_random(int n_vars, int n_terms, int k);

/* Generate a random s-CNF. */
CNF* cnf_random(int n_vars, int n_clauses, int s);

/* Count actual (non-null) literals in a term/clause. */
int  formula_term_width(const Formula* f, int term_idx);

/* Maximum width over all terms. (k for k-DNF, s for s-CNF). */
int  dnf_width(const DNF* d);
int  cnf_width(const CNF* c);

/* Count the number of non-null terms. */
int  dnf_term_count(const DNF* d);

/* Deep copy a formula. */
DNF* dnf_clone(const DNF* d);
CNF* cnf_clone(const CNF* c);

/* Free a formula. */
void formula_free(Formula* f);
void dnf_free(DNF* d);
void cnf_free(CNF* c);

/* Print a formula in human-readable form. */
void dnf_print(const DNF* d);
void cnf_print(const CNF* c);

/* ---- DNF/CNF Operations ---- */

/* Check if two formulas are equivalent (brute force for small n_vars). */
int  dnf_equivalent(const DNF* a, const DNF* b);

/* Convert a DNF to an equivalent CNF (exact, may be exponential). */
CNF* dnf_to_cnf_exact(const DNF* d);

/* Convert a CNF to an equivalent DNF (exact, may be exponential). */
DNF* cnf_to_dnf_exact(const CNF* c);

/* Check if DNF can be represented as s-CNF after simplification. */
int  dnf_is_s_cnf(const DNF* d, int s);

/* Get the minimum term that evaluates to 1, or -1 if none. */
int  dnf_first_satisfying_term(const DNF* d, const int* assign);

/* Count assignments that satisfy the DNF (exact, small n_vars only). */
long long dnf_satisfying_count(const DNF* d);

/* ================================================================
 * L3-L5: RESTRICTION API
 * ================================================================ */

/* Create a restriction on n_vars, all variables initially free (*). */
Restriction* restriction_create(int n_vars);

/* Generate a random restriction with star probability p_star.
 * Each variable independently:
 *   - * (free) with prob p_star
 *   - 0 with prob (1-p_star)/2
 *   - 1 with prob (1-p_star)/2 */
void restriction_random(Restriction* r, double p_star);

/* Generate restriction with specified seeds for reproducibility. */
void restriction_random_seeded(Restriction* r, double p_star, unsigned int seed);

/* Set a specific variable in the restriction.
 * value: -1 for *, 0 for fixed-0, 1 for fixed-1. */
void restriction_set(Restriction* r, int var, int value);

/* Get the value of a variable in the restriction. */
int  restriction_get(const Restriction* r, int var);

/* Count the number of free (*) variables. */
int  restriction_free_count(const Restriction* r);

/* Count variables set to 0 or 1. */
void restriction_counts(const Restriction* r, int* n_zero, int* n_one, int* n_star);

/* Apply restriction to a DNF: simplifies the formula.
 * Returns the restricted DNF. Surviving terms may have reduced width.
 * Falsified terms are removed; satisfied terms become empty (always true). */
DNF* dnf_restrict(const DNF* d, const Restriction* r);

/* Apply restriction to a CNF. */
CNF* cnf_restrict(const CNF* c, const Restriction* r);

/* Create a restriction from user-specified settings.
 * settings[v] = -1(*), 0, or 1. */
Restriction* restriction_from_array(int n_vars, const int* settings);

/* Deep copy a restriction. */
Restriction* restriction_clone(const Restriction* r);

/* Free a restriction. */
void restriction_free(Restriction* r);

/* Print restriction in human-readable form. */
void restriction_print(const Restriction* r);

/* ================================================================
 * L4: SWITCHING LEMMA — Core Probability Bound
 * ================================================================ */

/* Compute the Hastad switching lemma probability bound:
 *   Pr[failure] < (5pk)^s
 * where:
 *   k = DNF term width
 *   p = star probability
 *   s = target CNF clause width
 * Returns the upper bound on failure probability. */
double switching_prob_bound(int k, double p, int s);

/* Tighter bound including constant factor (Hastad's refined bound):
 *   Pr[failure] < (C * p * k)^s  where C is a small constant. */
double switching_prob_bound_refined(int k, double p, int s, double C);

/* Compute the optimal p that makes (5pk)^s < 1.
 * For given k and desired s, returns max p such that bound < epsilon. */
double switching_max_p(int k, int s, double epsilon);

/* Compute minimum s such that bound < epsilon for given k, p. */
int    switching_min_s(int k, double p, double epsilon);

/* ================================================================
 * L4-L5: EXPERIMENTAL SWITCHING VALIDATION
 * ================================================================ */

/* Run a single switching experiment:
 *   1. Generate random k-DNF
 *   2. Apply random restriction with prob p
 *   3. Check if restricted DNF is an s-CNF
 * Returns 1 if switching succeeded, 0 if failed. */
int  switching_single_trial(int n_vars, int n_terms, int k, double p, int s);

/* Run multiple switching trials and collect statistics. */
SwitchStats* switching_experiment(int n_vars, int n_terms, int k,
                                   double p, int s, int trials);

/* Print switching experiment results in formatted table. */
void switching_stats_print(const SwitchStats* stats);

/* Free switch statistics. */
void switch_stats_free(SwitchStats* stats);

/* ================================================================
 * L5: ITERATIVE SWITCHING — Depth Reduction
 * ================================================================ */

/* Simulate d-1 rounds of switching on a depth-d circuit.
 * Starting with n variables and using p = n^{-1/(d-1)},
 * each round keeps approximately p * n_i variables.
 * After d-1 rounds, approximately 1 variable remains.
 *
 * Returns a DepthReduction struct with per-round data. */
DepthReduction* depth_reduction_simulate(int n, int depth);

/* Given depth d and variable count n, compute the Hastad lower bound
 * on circuit size for computing PARITY:
 *   size >= exp(Omega(n^{1/(d-1)}))
 *
 * It is super-polynomial when depth is constant. */
double hastad_parity_lower_bound(int n, int d);

/* Compute the depth reduction tree:
 * shows how the circuit transforms through each switching round.
 * Prints a formatted table. */
void depth_reduction_print(const DepthReduction* dr);

/* Free depth reduction data. */
void depth_reduction_free(DepthReduction* dr);

/* ================================================================
 * L5-L6: DECISION TREE API
 * ================================================================ */

/* Create a leaf node with given output value. */
DTNode* dt_leaf(int value);

/* Create an internal node querying variable var,
 * with child0 for x_var=0 and child1 for x_var=1. */
DTNode* dt_node(int var, DTNode* child0, DTNode* child1);

/* Evaluate decision tree on assignment. */
int     dt_eval(const DTNode* tree, const int* assign);

/* Compute the depth of the decision tree (longest root-to-leaf path). */
int     dt_depth(const DTNode* tree);

/* Count the number of nodes in the decision tree. */
int     dt_size(const DTNode* tree);

/* Count the number of leaf nodes. */
int     dt_leaf_count(const DTNode* tree);

/* Compute the average depth (over uniform random input). */
double  dt_avg_depth(const DTNode* tree);

/* Build a complete decision tree for the PARITY function on n variables.
 * This creates a tree of depth n (must query ALL variables).
 * Size is 2^{n+1} - 1 nodes. */
DTNode* dt_parity(int n);

/* Build decision tree for MAJORITY on 3 variables (optimal depth 3). */
DTNode* dt_majority3(void);

/* Build decision tree for MAJORITY on n variables (greedy, O(2^n)). */
DTNode* dt_majority(int n);

/* Build a decision tree from a DNF formula.
 * The tree queries variables greedily to minimize depth. */
DTNode* dt_from_formula(const Formula* f);

/* Build a decision tree from a truth table. */
DTNode* dt_from_truthtable(int n_vars, const int* table);

/* Minimize a decision tree (remove redundant nodes).
 * Two children identical -> collapse to that child. */
DTNode* dt_minimize(DTNode* tree);

/* Check if a decision tree computes a specific Boolean function. */
int     dt_implements(const DTNode* tree, int n_vars, const int* truth_table);

/* Print decision tree structure. */
void    dt_print(const DTNode* tree);

/* Free decision tree memory. */
void    dt_free(DTNode* tree);

/* ================================================================
 * L6: CANONICAL PROBLEMS — Boolean Functions
 * ================================================================ */

/* Create a truth table for PARITY on n variables.
 * PARITY(x) = x_1 XOR x_2 XOR ... XOR x_n */
BoolFunc* bf_parity(int n);

/* Create truth table for MAJORITY on n variables.
 * MAJORITY(x) = 1 iff sum(x_i) > n/2 */
BoolFunc* bf_majority(int n);

/* Create truth table for THRESHOLD_k on n variables.
 * THRESHOLD(x) = 1 iff sum(x_i) >= k */
BoolFunc* bf_threshold(int n, int k);

/* Create truth table for MOD_m on n variables.
 * MOD_m(x) = 1 iff sum(x_i) mod m == 0 */
BoolFunc* bf_mod(int n, int m);

/* Create truth table for AND on n variables. */
BoolFunc* bf_and(int n);

/* Create truth table for OR on n variables. */
BoolFunc* bf_or(int n);

/* Evaluate a Boolean function on an assignment. */
int  bf_eval(const BoolFunc* bf, const int* assign);

/* Count the number of satisfying assignments. */
long long bf_satisfying_count(const BoolFunc* bf);

/* Compute the weight (number of 1s in truth table). */
long long bf_weight(const BoolFunc* bf);

/* Check if two Boolean functions are equal. */
int  bf_equal(const BoolFunc* a, const BoolFunc* b);

/* Compute the DNF for a Boolean function (from truth table). */
DNF* bf_to_dnf(const BoolFunc* bf);

/* Compute the CNF for a Boolean function (from truth table). */
CNF* bf_to_cnf(const BoolFunc* bf);

/* Print truth table. */
void bf_print(const BoolFunc* bf);

/* Free Boolean function. */
void bf_free(BoolFunc* bf);

/* ================================================================
 * L6-L7: AC0 CIRCUIT MODEL
 * ================================================================ */

/* Create an AC0 circuit with given number of variables. */
Circuit* circuit_create(int n_vars);

/* Add an INPUT gate to the circuit. */
CircuitNode* circuit_add_input(Circuit* c, int var);

/* Add an AND gate with specified fan-in gates. */
CircuitNode* circuit_add_and(Circuit* c, CircuitNode** inputs, int n_inputs);

/* Add an OR gate with specified fan-in gates. */
CircuitNode* circuit_add_or(Circuit* c, CircuitNode** inputs, int n_inputs);

/* Add a NOT gate. */
CircuitNode* circuit_add_not(Circuit* c, CircuitNode* input);

/* Set the output gate. */
void circuit_set_output(Circuit* c, CircuitNode* output);

/* Evaluate an AC0 circuit on an assignment. */
int  circuit_eval(const Circuit* c, const int* assign);

/* Compute the size (number of gates) of the circuit. */
int  circuit_size(const Circuit* c);

/* Compute the actual depth (longest path from input to output). */
int  circuit_depth(const Circuit* c);

/* Build an AC0 circuit for PARITY on n variables (requires depth n). */
Circuit* circuit_parity_ac0(int n);

/* Build a depth-2 circuit (DNF or CNF) for PARITY.
 * DNF: 2^{n-1} terms (exponential in n).
 * CNF: 2^{n-1} clauses. */
Circuit* circuit_parity_depth2(int n);

/* Build AC0 circuit for a given DNF formula. */
Circuit* circuit_from_dnf(const DNF* d);

/* Build AC0 circuit for a given decision tree.
 * Decision tree of depth D -> AC0 circuit of depth D.
 * But switching lemma says: small AC0 -> shallow DT -> contradiction. */
Circuit* circuit_from_dt(const DTNode* tree);

/* Verify that the circuit computes a given Boolean function. */
int  circuit_implements(const Circuit* c, const BoolFunc* bf);

/* Print circuit structure. */
void circuit_print(const Circuit* c);

/* Free circuit memory. */
void circuit_free(Circuit* c);

/* ================================================================
 * L7: APPLICATIONS
 * ================================================================ */

/* ---- Pseudorandom Generator from AC0 Lower Bounds ----
 * Nisan-Wigderson (1994): AC0 lower bounds imply PRGs.
 * If a function is hard for AC0 circuits, it can be used to
 * stretch a short random seed into a longer pseudorandom string
 * that fools AC0 circuits. */

/* Compute the NW-generator stretch from hardness parameters.
 * hardness: min circuit size needed to compute the function.
 * Returns the stretch factor (output bits / seed bits). */
double nw_generator_stretch(double hardness, int n);

/* ---- PAC Learning Connection ----
 * Linial-Mansour-Nisan (1993) + switching lemma:
 * AC0 circuits can be learned in quasi-polynomial time under
 * the uniform distribution using Fourier analysis. */

/* Compute the LMN Fourier concentration bound.
 * For an AC0 circuit of size S and depth d, the sum of
 * squared Fourier coefficients at level > L is bounded by: */
double lmn_fourier_tail_bound(double S, int d, int L, int n);

/* ---- Nisan's PRG for AC0 ----
 * Nisan (1991): Constructs PRG with seed O(log^{2d} n)
 * using the switching lemma as the main tool. */

/* Compute seed length for Nisan's PRG:
 * seed_len = O(log^{2d}(n/epsilon)) */
double nisan_prg_seed_length(int n, int d, double epsilon);

/* ================================================================
 * L8: ADVANCED TOPICS
 * ================================================================ */

/* ---- Multi-Switching Lemma ----
 * Extension: switch multiple adjacent layers simultaneously.
 * Used for tighter lower bounds and correlation bounds. */

/* Compute multi-switching bound for switching k1-DNF of k2-DNFs.
 * Original: switch one AND/OR layer.
 * Multi: switch AND-of-ANDs, OR-of-ORs, or deeper compositions. */
double multi_switching_bound(int k, int t, double p, int s);

/* ---- Average-Case Hardness ----
 * Hastad's lemma implies not just worst-case but average-case
 * hardness: PARITY is hard for AC0 on random inputs.
 * Even with 0.499 fraction of errors, AC0 circuits need
 * exponential size for PARITY. */

/* Compute the average-case lower bound.
 * Any AC0 circuit computing PARITY on >= 1/2 + epsilon fraction
 * of inputs requires the same exponential size. */
double average_case_parity_lower(int n, int d, double epsilon);

/* ---- Correlation Bounds ----
 * Switching lemma + Razborov-Smolensky: AC0 cannot compute
 * MAJORITY or MOD_p (p != 2) with high correlation.
 * For MOD_3: any AC0 circuit of depth d has correlation
 * at most exp(-Omega(n^{1/(2d-2)})). */

/* Compute correlation bound for MOD_m vs AC0 of depth d. */
double correlation_bound_modm_ac0(int m, int n, int d, int S);

/* ---- Natural Proofs Barrier ----
 * Razborov-Rudich (1997): Most known circuit lower bounds
 * (including switching lemma) are "natural proofs" and
 * thus cannot prove P != NP under standard cryptographic
 * assumptions. The switching lemma yields a natural property:
 *   - Constructive: can check if function is "simple" in poly time
 *   - Largeness: random functions are not simple
 *   - Usefulness: simple functions cannot compute PARITY */

/* Check if a Boolean function satisfies the "natural property"
 * derived from the switching lemma (rough estimate). */
int  natural_property_check(const BoolFunc* bf, int depth);

/* ================================================================
 * L4-L6: COMPREHENSIVE DEMONSTRATION FUNCTIONS
 * ================================================================ */

/* Full switching lemma demonstration.
 * Shows: DNF construction, restriction, switching bound,
 * experimental validation, depth reduction, and PARITY lower bound. */
void switching_lemma_demo(int n, int k, double p);

/* Benchmark-style demonstration with multiple parameter sets. */
void switch_bench_demo(void);

/* Decision tree demonstration: PARITY, MAJORITY, connection to switching. */
void decision_tree_demo(void);

/* DNF-to-CNF conversion via switching lemma: theoretical and experimental. */
void dnf_to_cnf_demo(void);

/* Random restriction experiments: effect on DNF size and width. */
void restriction_exp_demo(void);

/* Full PARITY lower bound proof simulation. */
void parity_lower_bound_demo(void);

/* AC0 circuit model demonstration. */
void ac0_circuit_demo(void);

/* Applications demonstration: PRG, learning, Fourier. */
void switching_applications_demo(void);

/* Advanced topics demonstration. */
void switching_advanced_demo(void);

#endif /* SWITCHING_H */