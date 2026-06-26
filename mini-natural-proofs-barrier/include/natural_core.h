#ifndef NATURAL_CORE_H
#define NATURAL_CORE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>

/*
 * natural_core.h -- Core Definitions for Natural Proofs Barrier
 *
 * L1: Core Definitions
 *   - TruthTable: Boolean function f:{0,1}^n -> {0,1}
 *   - NaturalProperty: constructive + large + useful
 *   - BooleanCircuit: DAG model
 *   - CircuitClass: AC0, TC0, NC1, P/poly, etc.
 *   - PseudoRandomFunction: PRF model for RR contradiction
 *
 * L2: Core Concepts
 *   - Constructiveness: decidable in 2^{O(n)} from truth table
 *   - Largeness: holds for >= 2^{-O(n)} fraction of functions
 *   - Usefulness: implies circuit lower bound
 *   - Shannon counting: 2^{2^n} functions vs 2^{O(S log S)} circuits
 *
 * L3: Mathematical Structures
 *   - TruthTable as vector in {0,1}^{2^n}
 *   - Circuit as DAG with AND/OR/NOT gates
 *   - Uniform distribution over all Boolean functions
 *
 * References:
 *   Razborov-Rudich (1997): "Natural Proofs", JCSS 55(1):24-35
 *   Arora-Barak (2009), Chapter 23
 *   Shannon (1949): BSTJ 28(1):59-98
 *   HILL (1999): "Pseudorandom Generators from One-Way Functions"
 */

/* ========================================================================
 * TruthTable: Boolean function f:{0,1}^n -> {0,1}
 * 2^n entries, each 0 or 1. Total functions: 2^{2^n}
 * ======================================================================== */
typedef struct {
    int        n_inputs;
    int       *values;
    long long  table_size;
    char       name[64];
} TruthTable;

typedef struct {
    int        n_inputs;
    uint64_t  *bits;
    long long  table_size;
} CompressedTruthTable;

TruthTable *tt_create(int n);
TruthTable *tt_create_named(int n, const char *name);
void        tt_free(TruthTable *t);
TruthTable *tt_copy(const TruthTable *t);

int         tt_get(const TruthTable *t, long long idx);
void        tt_set(TruthTable *t, long long idx, int val);
int         tt_evaluate(const TruthTable *t, const int *input);
int         tt_evaluate_int(const TruthTable *t, long long encoding);

TruthTable *tt_random(int n);
TruthTable *tt_random_seeded(int n, unsigned int seed);

int         tt_equal(const TruthTable *a, const TruthTable *b);
long long   tt_hamming_distance(const TruthTable *a, const TruthTable *b);
int         tt_is_constant(const TruthTable *t);
int         tt_is_monotone(const TruthTable *t);

TruthTable *tt_not(const TruthTable *t);
TruthTable *tt_and(const TruthTable *a, const TruthTable *b);
TruthTable *tt_or(const TruthTable *a, const TruthTable *b);
TruthTable *tt_xor(const TruthTable *a, const TruthTable *b);

long long   tt_weight(const TruthTable *t);
double      tt_fraction_ones(const TruthTable *t);
double      tt_count_functions(int n);

void        tt_print(const TruthTable *t, FILE *fp);
void        tt_print_brief(const TruthTable *t, FILE *fp);

CompressedTruthTable *ctt_create(int n);
void                  ctt_free(CompressedTruthTable *t);
int                   ctt_get(const CompressedTruthTable *t, long long idx);
void                  ctt_set(CompressedTruthTable *t, long long idx, int val);
CompressedTruthTable *ctt_random(int n);

/* ========================================================================
 * NaturalProperty: Property P of Boolean functions (Razborov-Rudich 1997)
 *
 * Three criteria (all must hold):
 *   1. CONSTRUCTIVE: P(f) decidable in time 2^{O(n)} given truth table
 *   2. LARGE: Pr_{random f}[P(f)] >= 2^{-O(n)}
 *   3. USEFUL: P(f) => f notin C (C is target circuit class)
 *
 * THEOREM (RR97): If OWF exist, no natural proof can show NP notin P/poly.
 * ======================================================================== */
typedef struct {
    int    constructive;
    int    large;
    int    useful;
    double constructiveness_bound;
    double largeness_exponent;
    int    target_circuit_size;
    int    is_natural;
} NaturalProperty;

typedef enum {
    NP_CONSTRUCTIVE = 0,
    NP_LARGE        = 1,
    NP_USEFUL       = 2,
    NP_NUM_CRITERIA = 3
} NP_Criterion;

typedef struct {
    NaturalProperty criteria;
    const char     *property_name;
    const char     *circuit_class;
    int             n_inputs;
    int             size_bound;
    double          computational_cost;
    double          fraction_satisfying;
    int             implies_lower_bound;
    char            verdict[256];
} NP_Verdict;

/* ========================================================================
 * BooleanCircuit: DAG model with gates
 * ======================================================================== */
typedef enum {
    GATE_INPUT  = 0,
    GATE_AND    = 1,
    GATE_OR     = 2,
    GATE_NOT    = 3,
    GATE_CONST0 = 4,
    GATE_CONST1 = 5,
    GATE_XOR    = 6,
    GATE_MAJ    = 7,
    GATE_MODm   = 8,
    GATE_OUTPUT = 9
} GateType;

typedef struct {
    GateType type;
    int      gate_id;
    int      fanin_count;
    int     *fanin;
    int      value;
    int      level;
    char     label[32];
} CircuitGate;

typedef struct {
    int          n_inputs;
    int          n_outputs;
    int          n_gates;
    int          capacity;
    CircuitGate *gates;
    int          output_gate;
    int         *output_gates;
    int          size;
    int          depth;
    char         class_name[32];
    int          is_uniform;
} BooleanCircuit;

BooleanCircuit *circ_create(int n_inputs, int initial_capacity);
void            circ_free(BooleanCircuit *c);
int             circ_add_gate(BooleanCircuit *c, GateType type,
                              const int *fanin_ids, int n_fanin);
int             circ_add_input(BooleanCircuit *c, int var_index);
void            circ_set_output(BooleanCircuit *c, int gate_id);
int             circ_evaluate(BooleanCircuit *c, const int *input);
TruthTable     *circ_evaluate_tt(BooleanCircuit *c);
void            circ_compute_depth(BooleanCircuit *c);
int            *circ_count_gates_by_type(BooleanCircuit *c);
void            circ_print(BooleanCircuit *c, FILE *fp);
void            circ_print_stats(BooleanCircuit *c, FILE *fp);

/* ========================================================================
 * CircuitClass: Standard circuit complexity hierarchy
 *   AC0 subset AC0[p] subset ACC0 subset TC0 subset NC1 subset P/poly
 * ======================================================================== */
typedef enum {
    CLASS_AC0      = 0,
    CLASS_AC0_p    = 1,
    CLASS_ACC0     = 2,
    CLASS_TC0      = 3,
    CLASS_NC1      = 4,
    CLASS_NC       = 5,
    CLASS_P_poly   = 6,
    CLASS_SIZE_S   = 7,
    CLASS_MONOTONE = 8,
    CLASS_GENERAL  = 9
} CircuitClass;

const char *circuit_class_name(CircuitClass cc);
const char *circuit_class_description(CircuitClass cc);

/* ========================================================================
 * PseudoRandomFunction: PRF model for RR contradiction
 *
 * HILL (1999): OWF => PRF exist.
 *
 * Contradiction: natural P + OWF => PRF has P (large) + PRF easy (P/poly)
 *                 but P is useful => PRF hard => CONTRADICTION.
 * ======================================================================== */
typedef struct {
    int         key_length;
    int         input_length;
    int         output_length;
    TruthTable *function;
    int         is_prf;
    double      advantage;
    long long   seed;
} PseudoRandomFunction;

PseudoRandomFunction *prf_create(int n_inputs, int key_length);
PseudoRandomFunction *prf_create_from_tt(TruthTable *tt, int key_length);
void                  prf_free(PseudoRandomFunction *prf);
int                   prf_evaluate(PseudoRandomFunction *prf, long long input);
const TruthTable     *prf_get_truth_table(const PseudoRandomFunction *prf);

#endif /* NATURAL_CORE_H */
