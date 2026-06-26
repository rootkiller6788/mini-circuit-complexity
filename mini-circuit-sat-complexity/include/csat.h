/* csat.h -- Circuit-SAT Complexity
 *
 * Circuit-SAT: given Boolean circuit C, exists x with C(x)=1?
 * NP-complete (Cook 1971). First problem proved NP-complete.
 *
 * Williams' program (2010-2014): SAT algorithms <=> circuit lower bounds.
 *   If C-SAT solvable in 2^n/n^{omega(1)} time => NEXP not in C.
 *   Proved for ACC0 (Williams 2014). Open for P/poly.
 *
 * References:
 *   Arora & Barak (2009) Ch.6: Boolean Circuits
 *   Vollmer (1999) "Introduction to Circuit Complexity"
 *   Jukna (2012) "Boolean Function Complexity"
 *   Williams (2014) "NEXP not in ACC0", JACM
 *   Stanford CS358 / MIT 6.841 / Berkeley CS278
 */

#ifndef CSAT_H
#define CSAT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include <time.h>
#include "circuit.h"
#include "circuit_builder.h"

/* Backward compatibility typedef */
typedef BooleanCircuit Circuit;

/* ============================================================================
 * Circuit complexity class descriptor
 * ============================================================================ */

/* Describes a circuit complexity class by gate restrictions and depth.
 * Used for membership testing and class hierarchy exploration. */
typedef struct {
    CircuitClass cls;        /* class enum: AC0, ACC0, TC0, NC1, P/poly */
    int          depth;      /* max depth bound, -1 = unbounded */
    int          fanin;      /* max fan-in bound, -1 = unbounded */
    int          size_bound; /* max size bound, -1 = unbounded (poly implied) */
    int          nm;         /* number of mod gates */
    int          mods[8];    /* mod gate moduli (for ACC0, TC0) */
    double       threshold;  /* threshold value for TC0 */
    int          alternations; /* number of alternation layers */
} ClassDesc;

/* ============================================================================
 * L1: Core SAT Algorithms
 * ============================================================================ */

/* Brute-force: enumerate all 2^n assignments.
 * Time: O(2^n * |C|). Space: O(n). */
int csat_brute(BooleanCircuit* c, int n_inputs);

/* DPLL with backtracking search and unit propagation.
 * Time: O(2^n) worst-case. Heuristic: try 0 before 1. */
int csat_dpll(BooleanCircuit* c, int* assign, int depth, int nv);

/* Random sampling: Monte Carlo SAT test.
 * Hits with probability = #SAT / 2^n per sample.
 * Useful for dense SAT instances. Incomplete. */
int csat_random(BooleanCircuit* c, int nv, int samples);

/* DPLL with unit propagation and pure literal elimination.
 * More efficient than basic DPLL for structured circuits. */
int csat_dpll_full(BooleanCircuit* c, int* assign, int depth, int nv);

/* Random walk (Schoning-style) for Circuit-SAT.
 * max_steps: number of flips before restart. */
int csat_random_walk(BooleanCircuit* c, int nv, int max_steps);

/* Random walk with restarts. */
int csat_random_restarts(BooleanCircuit* c, int nv, int restarts, int steps);

/* Williams speedup estimate: baseline 2^n vs 2^n/n^k.
 * Returns speedup factor. */
double csat_williams_speedup(int n, int k);

/* Simple circuit parser from text format.
 * Format: I=INPUT A=AND O=OR N=NOT X=XOR (sequential). */
BooleanCircuit* circuit_parse(const char* spec);

/* ============================================================================
 * L2: CNF Representation (Data Structure)
 * ============================================================================ */

/* CNF instance: variables 1..nv, each clause is null-terminated array.
 * Literals: positive = variable, negative = negated variable. */
typedef struct {
    int   nv;         /* number of variables */
    int   nc;         /* number of clauses */
    int   cap;        /* allocated capacity */
    int** clauses;    /* clause[i] = int array, terminated by 0 */
    int*  sizes;      /* clause[i] length (excluding terminator) */
} CNFInstance;

/* Create empty CNF with nv variables.
 * Complexity: O(1). */
CNFInstance* cnf_create(int nv);

/* Add clause to CNF. Clause is copied internally.
 * clause[] must be variable indices; terminated by 0 or size given.
 * Complexity: O(k) where k = clause length. */
void cnf_add_clause(CNFInstance* cnf, const int* clause, int size);

/* Free CNF and all clauses.
 * Complexity: O(nc * avg_clause_size). */
void cnf_free(CNFInstance* cnf);

/* Evaluate CNF under full assignment.
 * assign[v] in {0,1} for v=1..nv.
 * Returns 1 if satisfied, 0 otherwise.
 * Complexity: O(total literals). */
int cnf_evaluate(const CNFInstance* cnf, const int* assign);

/* Print CNF in DIMACS format.
 * Complexity: O(total literals). */
void cnf_print_dimacs(const CNFInstance* cnf, FILE* f);

/* Parse CNF from DIMACS file. Returns NULL on failure.
 * File: "p cnf <nv> <nc>" followed by clauses.
 * Complexity: O(file size). */
CNFInstance* cnf_parse_dimacs(const char* filename);

/* Pure literal elimination: if variable appears only positively
 * or only negatively, set it to satisfy all clauses.
 * Returns number of variables eliminated.
 * Complexity: O(total literals). */
int cnf_pure_literal_elim(CNFInstance* cnf);

/* Clause subsumption: remove clauses that are supersets of others.
 * Returns number of clauses removed.
 * Complexity: O(nc^2 * avg_clause_size). */
int cnf_subsumption_elim(CNFInstance* cnf);

/* Variable elimination (Davis-Putnam): resolve variable out.
 * Eliminates all clauses containing var or -var.
 * Returns number of clauses added (0 = pure/total elimination).
 * Complexity: O(nc^2). */
int cnf_variable_elim(CNFInstance* cnf, int var);

/* Count number of satisfied clauses.
 * Complexity: O(total literals). */
int cnf_count_satisfied(const CNFInstance* cnf, const int* assign);

/* Clause-to-variable ratio (phase transition ~4.26 for 3-SAT).
 * Complexity: O(1). */
double cnf_clause_var_ratio(const CNFInstance* cnf);

/* ============================================================================
 * L3: CNF to Circuit conversion
 * ============================================================================ */

/* Tseitin transformation: convert Boolean circuit to CNF.
 * Introduces auxiliary variable per gate. O(|C|) blowup.
 * Equisatisfiable (not equivalent). */
CNFInstance* csat_to_cnf(const BooleanCircuit* c);

/* Plaisted-Greenbaum: CNF encoding with fewer auxiliary variables.
 * Uses polarity-based naming to reduce clauses. */
CNFInstance* csat_to_cnf_plaisted(const BooleanCircuit* c);

/* ============================================================================
 * L4: CDCL SAT Solver (Conflict-Driven Clause Learning)
 * ============================================================================ */

/* Full CDCL solver with watched literals, 1-UIP conflict analysis,
 * VSIDS heuristic, and Luby restarts.
 *
 * Based on: MiniSAT (Een & Sorensson 2003), Glucose (Audemard & Simon 2009).
 * Theorem: CDCL is sound and complete for SAT. */

typedef struct {
    int   nv;              /* number of variables */
    int   nc;              /* number of clauses */
    int*  assign;          /* assignment: 0=unassigned,1=true,-1=false */
    int*  level;           /* decision level when assigned */
    int*  reason;          /* antecedent clause (index) for implied vars */
    int*  trail;           /* assignment trail */
    int   trail_sz;        /* current trail size */
    int*  trail_lim;       /* trail limit per decision level */
    int   dl;              /* current decision level */

    /* Watched literals (2 per clause) */
    int*  watches;         /* watch[2*i], watch[2*i+1] for clause i */
    int** lit_to_clauses;  /* literal -> list of clause indices */
    int*  lc_sizes;        /* size of each literal's clause list */
    int*  lc_caps;         /* capacity of each literal's clause list */

    /* VSIDS heuristic */
    double* activity;      /* variable activity score */
    double  var_inc;       /* current activity increment */
    double  var_decay;     /* decay factor per conflict */

    /* Restart policy */
    int     nconflicts;    /* total conflicts */
    int     nrestarts;     /* total restarts */
    int     restart_limit; /* next restart threshold */

    /* Phase saving */
    int*    phase;         /* saved phase: 1 or -1 */

    /* Conflict analysis scratch */
    int*    seen;          /* seen counter for each variable */
    int     seen_ctr;      /* monotonic counter for "seen" */
} CDCLSolver;

/* Create CDCL solver for nv variables.
 * Complexity: O(nv). */
CDCLSolver* cdcl_create(int nv);

/* Free CDCL solver.
 * Complexity: O(nv + nc). */
void cdcl_free(CDCLSolver* s);

/* Load CNF instance into solver.
 * Complexity: O(nc * k). */
void cdcl_load_cnf(CDCLSolver* s, const CNFInstance* cnf);

/* Main solve loop with conflict limit.
 * Returns: 1=SAT, 0=UNSAT, -1=unknown (limit reached).
 * Complexity: worst-case exponential. */
int cdcl_solve(CDCLSolver* s, int max_conflicts);

/* Boolean Constraint Propagation: deduce forced assignments.
 * Returns: -1=conflict, 0=no more propagation.
 * Complexity: O(watch list size). */
int cdcl_bcp(CDCLSolver* s);

/* Pick next unassigned variable (VSIDS heuristic).
 * Returns: 0..nv-1, or -1 if all assigned.
 * Complexity: O(nv). */
int cdcl_decide(CDCLSolver* s);

/* Analyze conflict clause via 1-UIP scheme.
 * Returns the learned clause (null-terminated malloc'd array).
 * Also computes backtrack level into *bt_level.
 * Caller must free returned clause.
 * Complexity: O(trail size). */
int* cdcl_analyze_conflict(CDCLSolver* s, int confl, int* learned_sz, int* bt_level);

/* Backtrack to given decision level.
 * Complexity: O(trail positions undone). */
void cdcl_backtrack(CDCLSolver* s, int level);

/* Get current model (only valid after SAT result).
 * model[v] = 0 or 1 for v in 0..nv-1.
 * Complexity: O(nv). */
void cdcl_get_model(const CDCLSolver* s, int* model);

/* Print solver statistics.
 * Complexity: O(1). */
void cdcl_print_stats(const CDCLSolver* s);

/* ============================================================================
 * L4: Lookahead SAT Solver
 * ============================================================================ */

/* Lookahead solver: probe each variable, detect failed literals
 * and equivalences before branching.
 *
 * Based on: Heule & van Maaren (2009) "Look-Ahead Based SAT Solving".
 * Used in: march, OKsolver, satz. */

typedef struct {
    int   nv;          /* number of variables */
    int   nc;          /* number of clauses */
    int** clauses;     /* clause database */
    int*  c_sizes;     /* clause sizes */
    int*  assign;      /* current assignment */
    int*  failed_lits; /* detected failed literals */
    int   n_failed;    /* count of failed literals */
    int*  equiv;       /* equivalence classes */
    int   n_equiv;     /* count of equivalences found */
} LookaheadSolver;

LookaheadSolver* lookahead_create(const CNFInstance* cnf);
void lookahead_free(LookaheadSolver* s);
int  lookahead_solve(LookaheadSolver* s);
int  lookahead_probe(LookaheadSolver* s, int lit);
int  lookahead_find_failed(LookaheadSolver* s);
int  lookahead_find_equiv(LookaheadSolver* s);
void lookahead_get_model(const LookaheadSolver* s, int* model);

/* ============================================================================
 * L5: Circuit Depth Analysis
 * ============================================================================ */

/* Compute exact circuit depth (longest input-to-output path).
 * Complexity: O(n_gates). Uses memoization. */
int circuit_depth_exact(const BooleanCircuit* c);

/* Compute topological ordering of gates.
 * order[] must be pre-allocated (n_gates entries).
 * Complexity: O(n_gates + n_edges). */
void circuit_topological_order(const BooleanCircuit* c, int* order);

/* Compute width at each level (number of gates at each depth).
 * depths[] must be pre-allocated (n_gates entries).
 * Returns max depth.
 * Complexity: O(n_gates). */
int circuit_level_widths(const BooleanCircuit* c, int* widths);

/* Check if circuit is an AND-OR tree (alternating layers).
 * Complexity: O(n_gates). */
int circuit_is_alternating(const BooleanCircuit* c);

/* Count fan-in distribution: AND, OR, NOT gates.
 * ands, ors, nots are output counters.
 * Complexity: O(n_gates). */
void circuit_gate_distribution(const BooleanCircuit* c, int* ands, int* ors, int* nots, int* xors);

/* ============================================================================
 * L5: Circuit Optimization
 * ============================================================================ */

/* Redundant gate elimination: remove gates whose output equals
 * one of their inputs. Returns gates removed. */
int optimize_redundant_gates(BooleanCircuit* c);

/* Constant folding: evaluate constant sub-circuits.
 * Returns gates simplified. */
int optimize_constant_fold(BooleanCircuit* c);

/* Double negation removal: NOT(NOT(x)) -> x.
 * Returns gates removed. */
int optimize_double_negation(BooleanCircuit* c);

/* Merge structurally identical gates.
 * Returns gates merged. */
int optimize_merge_duplicates(BooleanCircuit* c);

/* Idempotent elimination: AND(x,x) -> x, OR(x,x) -> x.
 * Returns gates removed. */
int optimize_idempotent(BooleanCircuit* c);

/* Absorption: AND(x,OR(x,y)) -> x, OR(x,AND(x,y)) -> x.
 * Returns gates removed. */
int optimize_absorption(BooleanCircuit* c);

/* Run all optimization passes.
 * Returns total gates removed. */
int optimize_all_passes(BooleanCircuit* c);

/* ============================================================================
 * L5: Circuit Transformations
 * ============================================================================ */

/* Push negations to inputs via De Morgan's laws.
 * Complexity: O(n_gates). */
int circuit_push_negations(BooleanCircuit* c);

/* Convert circuit to NAND-only basis.
 * AND(x,y)=NAND(NAND(x,y),NAND(x,y)), OR(x,y)=NAND(NAND(x,x),NAND(y,y)).
 * Complexity: O(n_gates). */
BooleanCircuit* circuit_to_nand_basis(const BooleanCircuit* c);

/* Convert multi-output circuit to single-output by AND of all outputs.
 * Complexity: O(n_outputs). */
void circuit_flatten_outputs(BooleanCircuit* c);

/* Substitute a gate's output: replace all occurrences of old_id with new_id.
 * Returns number of substitutions made.
 * Complexity: O(n_gates). */
int circuit_substitute(BooleanCircuit* c, int old_id, int new_id);

/* ============================================================================
 * L6: Circuit Lower Bounds
 * ============================================================================ */

/* Shannon's counting bound: most functions need Omega(2^n/n) gates.
 * Theorem (Shannon 1949): For almost all f:{0,1}^n->{0,1},
 *   CircuitSize(f) >= 2^n / n. */
double shannon_bound(int n);

/* Lupanov's upper bound: every function has circuit of size
 *   2^n/n * (1 + o(1)).
 * Theorem (Lupanov 1958): For all f:{0,1}^n->{0,1},
 *   CircuitSize(f) <= 2^n/n * (1 + O(log n / n)). */
double lupanov_bound(int n);

/* Gate elimination bound for formulas of depth d on n vars with m gates.
 * Lemma: removing a gate eliminates at most its fan-in variables.
 * For leaf-size bound: Omega(n^{1/d}) for depth-d formulas. */
double gate_elimination_bound(int n, int m, int d);

/* Random restriction bound for AC0 lower bounds.
 * Under random restriction with probability p, circuit collapses.
 * Expected leaf size bound. */
double random_restriction_bound(int n, int d, double p);

/* Hastad Switching Lemma: with prob 1-delta, a CNF/DNF becomes
 * a decision tree of depth k after restriction with prob p.
 * Parameters: n=vars, d=bottom fan-in, k=target depth.
 * Bound: circuit size >= 2^{n^{1/(d-1)}} for depth d. */
double switching_lemma_bound(int n, int d, int k);

/* Neciporuk's bound: for function with S subfunctions on disjoint
 * variable sets, size >= S - n.
 * Used for indirect addressing, element distinctness. */
double neciporuk_bound(int n, int* sub_counts, int n_sub);

/* Krapchenko's formula lower bound via communication complexity.
 * For PARITY: formula size >= n^2. */
double krapchenko_bound(int n, int parities);

/* Andreev function: nearly maximal formula size.
 * Andreev_bound(n) ~ n^{2-o(1)}. */
double andreev_bound(int n);

/* ============================================================================
 * L7: MCSP — Minimum Circuit Size Problem
 * ============================================================================ */

/* MCSP: given truth table of f, find smallest circuit computing f.
 * Meta-complexity: hardness of MCSP => circuit lower bounds.
 * Kabanets & Cai (2000), Allender et al. (2017). */

typedef struct {
    BooleanCircuit* orig;     /* original circuit */
    int             orig_size;
    int             min_size;   /* best size found */
    int*            removed;    /* boolean: gate i removed? */
    int             nr;         /* number removed */
    int             ng;         /* total gates */
    double          best_score; /* optimization metric */
} MCSPSolver;

MCSPSolver* mcsp_create(BooleanCircuit* c);
void mcsp_free(MCSPSolver* s);

/* Greedy gate removal: remove one gate at a time, check equivalence.
 * budget: max removals to attempt.
 * Returns number of gates removed. */
int mcsp_greedy_minimize(MCSPSolver* s, int budget);

/* Exact MCSP via exhaustive search (n_gates <= 16 only).
 * Returns minimized size. */
int mcsp_exact_minimize(MCSPSolver* s);

/* Kernelization: identify essential gates that cannot be removed.
 * Returns number of essential gates.
 * Complexity: O(n_gates^2). */
int mcsp_kernelize(MCSPSolver* s);

void mcsp_print_result(const MCSPSolver* s);

/* ============================================================================
 * L8: Williams' Algorithmic Method
 * ============================================================================ */

/* Williams' breakthrough: SAT algorithms => circuit lower bounds.
 * NEXP not in ACC0 (Williams 2014, JACM).
 *
 * Key idea: if Circuit-SAT for class C can be solved in
 * O(2^n / n^{omega(1)}) time, then NEXP not in C.
 *
 * This file implements the algorithmic side:
 *   ACC0-SAT in 2^{n - n^delta} time using fast matrix multiplication. */

/* Evaluate all restrictions of an ACC0 circuit using matrix mult.
 * n_inputs: number of variables.
 * Returns array of circuit outputs for all 2^n inputs.
 * Complexity: O(2^n * poly). */
int* williams_evaluate_all(BooleanCircuit* c, int n_inputs);

/* ACC0-SAT using Williams' subexponential algorithm.
 * Uses rectangular matrix multiplication for speedup.
 * Complexity: 2^{n - n^delta} for some delta > 0.
 * Returns: 1=SAT, 0=UNSAT. */
int williams_acc0_sat(BooleanCircuit* c, int n_inputs, double delta);

/* Williams derandomization: transform probabilistic circuit
 * into deterministic one with small overhead.
 * Returns new circuit (caller frees). */
BooleanCircuit* williams_derandomize(BooleanCircuit* c, int n_inputs);

/* Circuit analysis: check if circuit is in ACC0.
 * Returns 1 if circuit uses only AND, OR, NOT, MOD gates. */
int williams_is_acc0(const BooleanCircuit* c);

/* Estimate Williams speedup for given parameters.
 *   n: input size, k: polynomial degree in speedup factor
 *   gate_type: 0=AC0, 1=ACC0, 2=TC0, 3=P/poly
 * Returns: best known or conjectured runtime bound. */
double williams_speedup_estimate(int n, int k, int gate_type);

/* ============================================================================
 * L9: Benchmark & Hard Instance Generation
 * ============================================================================ */

/* Generate random k-SAT instance with n variables and m clauses.
 * Clause-to-variable ratio alpha = m/n controls hardness.
 * Phase transition: alpha ~ 4.26 for 3-SAT.
 * Complexity: O(m * k). */
CNFInstance* bench_random_ksat(int n, int m, int k, unsigned int seed);

/* Pigeonhole principle: n+1 pigeons into n holes.
 * UNSAT: each hole gets at most one pigeon.
 * Encodes exponential lower bounds for resolution.
 * Complexity: O(n^3) clauses. */
CNFInstance* bench_pigeonhole(int n);

/* Graph k-coloring SAT: vertices 1..n, colors 1..k.
 * edges[i][2] = (u,v) for each edge.
 * SAT iff graph is k-colorable.
 * Complexity: O(n*k + |E|*k) clauses. */
CNFInstance* bench_graph_coloring(int n, int edges[][2], int ne, int k);

/* Sorting network SAT: spec that circuit sorts n inputs.
 * UNSAT for random function; SAT for correct sort.
 * Complexity: O(n^2 log n) clauses. */
CNFInstance* bench_sorting_network(int n);

/* Equivalence checking: two circuits compute same function?
 * Builds miter circuit (XOR of outputs) and checks UNSAT.
 * Complexity: O(|C1| + |C2|). */
int bench_equivalent(BooleanCircuit* c1, BooleanCircuit* c2, int n_inputs);

/* ============================================================================
 * Demos and demonstrations
 * ============================================================================ */

void csat_demo(void);
void csat_core_algorithms_demo(void);
void csat_cdcl_demo(void);
void csat_lookahead_demo(void);
void csat_depth_analysis_demo(void);
void csat_optimization_demo(void);
void csat_transformations_demo(void);
void csat_cnf_tools_demo(void);
void csat_lower_bounds_demo(void);
void csat_mcsp_demo(void);
void csat_williams_demo(void);
void csat_benchmark_demo(void);
void csat_parse_demo(void);
void csat_hardness_demo(void);
void csat_reduction_demo(void);
void csat_random_demo(void);

#endif /* CSAT_H */
