/**
 * circuit_model.h — AC0 Circuit Model for Hastad Lower Bounds
 *
 * Models constant-depth unbounded fan-in AND/OR/NOT circuits.
 * AC0 = union over constant d of depth-d poly-size circuits.
 *
 * L1: AC0 circuit family definition
 * L3: Circuit as labeled DAG, gate types, evaluation
 * L6: PARITY/MOD evaluation on circuits
 */
#ifndef CIRCUIT_MODEL_H
#define CIRCUIT_MODEL_H

#include <stdint.h>
#include "hastad.h"

/* ── Gate Structure ──────────────────────────────────────────── */

typedef struct Gate {
    int id;              /* unique gate ID */
    GateType type;       /* gate type */
    int n_inputs;        /* fan-in */
    int* inputs;         /* array of gate IDs feeding this gate */
    int output_value;    /* evaluated value (-1=unevaluated, 0, 1) */
    int depth;           /* depth from inputs (0 for input gates) */
    int is_output;       /* 1 if this is the output gate */
} Gate;

/* ── Circuit Structure ───────────────────────────────────────── */

typedef struct {
    int n_vars;          /* number of input variables */
    int n_gates;         /* total number of gates */
    int max_depth;       /* maximum gate depth */
    double size;         /* circuit size (number of gates) */
    Gate* gates;         /* array of all gates */
    int* input_gate_ids; /* gate IDs for input variables */
    int output_gate_id;  /* output gate ID */
} AC0Circuit;

/* ── DNF/CNF Representation ──────────────────────────────────── */

typedef struct {
    int n_vars;          /* variables */
    int n_terms;         /* number of terms */
    int term_width;      /* max literals per term */
    int** terms;         /* terms[n_terms][n_vars]: -1=absent, 0=neg, 1=pos */
    int is_dnf;          /* 1=DNF, 0=CNF */
} DNF_CNF;

/* ── Circuit Construction ────────────────────────────────────── */

/** Create an AC0 circuit with given parameters */
AC0Circuit* circuit_create(int n_vars, int max_gates);

/** Free circuit memory */
void circuit_free(AC0Circuit* c);

/** Add a gate to the circuit, return gate ID */
int circuit_add_gate(AC0Circuit* c, GateType type, int n_inputs, const int* inputs);

/** Set the output gate */
void circuit_set_output(AC0Circuit* c, int gate_id);

/** Build a PARITY circuit of given depth (upper bound construction) */
AC0Circuit* circuit_build_parity(int n, int depth);

/** Build a MAJORITY circuit (exponential size for depth d) */
AC0Circuit* circuit_build_majority(int n, int depth);

/** Build an AND-of-ORs DNF circuit */
AC0Circuit* circuit_build_dnf(int n_vars, const DNF_CNF* dnf);

/* ── Circuit Evaluation ──────────────────────────────────────── */

/** Evaluate circuit on input x (array of n_vars 0/1 values) */
int circuit_evaluate(const AC0Circuit* c, const int* x);

/** Evaluate and return the value at each gate (for debugging) */
int* circuit_evaluate_all_gates(const AC0Circuit* c, const int* x);

/** Count the number of satisfying assignments (2^n possible) */
int64_t circuit_count_sat(const AC0Circuit* c);

/* ── Circuit Properties ──────────────────────────────────────── */

/** Get circuit depth */
int circuit_depth(const AC0Circuit* c);

/** Get circuit size (total gates) */
int circuit_size(const AC0Circuit* c);

/** Max fan-in */
int circuit_max_fanin(const AC0Circuit* c);

/** Check if circuit is layered (gates at depth k only connect to depth k-1) */
int circuit_is_layered(const AC0Circuit* c);

/** Print circuit statistics */
void circuit_print_stats(const AC0Circuit* c);

/* ── DNF/CNF Operations ──────────────────────────────────────── */

/** Create a DNF/CNF structure */
DNF_CNF* dnf_cnf_create(int n_vars, int max_terms, int term_width, int is_dnf);

/** Free DNF/CNF memory */
void dnf_cnf_free(DNF_CNF* dc);

/** Add a term to DNF/CNF */
void dnf_cnf_add_term(DNF_CNF* dc, const int* literals);

/** Evaluate DNF/CNF on input */
int dnf_cnf_evaluate(const DNF_CNF* dc, const int* x);

/** Convert between DNF and CNF via De Morgan (size may blow up) */
DNF_CNF* dnf_to_cnf(const DNF_CNF* dnf);

/** Compute the number of terms for PARITY-n in DNF (always 2^{n-1}) */
int64_t parity_dnf_term_count(int n);

/** Check if two DNF/CNF formulas represent the same function */
int dnf_cnf_equivalent(const DNF_CNF* a, const DNF_CNF* b);

/* ── Circuit Under Restriction ───────────────────────────────── */

/** Apply restriction to circuit: fix variables to 0/1, simplify */
AC0Circuit* circuit_restrict(const AC0Circuit* c, const int* restr, int n);

/** Convert circuit under restriction to DNF/CNF */
DNF_CNF* circuit_to_dnf_after_restriction(const AC0Circuit* c,
                                           const int* restr, int n);

#endif /* CIRCUIT_MODEL_H */
