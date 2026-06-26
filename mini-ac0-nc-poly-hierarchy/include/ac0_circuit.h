/* ac0_circuit.h — AC⁰-Specific Circuit Definitions
 * =====================================================================
 * AC⁰: constant-depth (O(1)), unbounded fan-in AND/OR/NOT,
 *      polynomial-size circuit families.
 *
 * Key properties:
 *   - Depth is a constant independent of input size n
 *   - Gates can have fan-in up to poly(n)
 *   - NOT gates may appear anywhere (unlike monotone circuits)
 *   - Cannot compute PARITY (FSS/Ajtai/Håstad)
 *   - Can compute: AND, OR, THRESHOLD-k for k=O(1) via DNF
 *   - Cannot compute MAJORITY (Razborov-Smolensky for AC⁰[p!=2])
 *
 * AC⁰ depth hierarchy: AC⁰[d] ⊊ AC⁰[d+1] for all d ≥ 0.
 * Proof uses Sipser functions S_d (Håstad 1986).
 *
 * References:
 *   AB §6.1-6.3, §14.1
 *   Furst, Saxe, Sipser (1981) — PARITY ∉ AC⁰
 *   Håstad (1986) — Switching Lemma, depth hierarchy
 */
#ifndef AC0_CIRCUIT_H
#define AC0_CIRCUIT_H

#include "ac0nc.h"

/* ─── AC⁰ Depth Constraints ─────────────────────────────────────── */
/* AC⁰ requires CONSTANT depth. We define the practical bound as
 * a small constant (depth ≤ 10 for all n). In theory, depth
 * cannot grow with n. */
#define AC0_MAX_DEPTH       16

/* ─── AC⁰-Specific Gate Restrictions ───────────────────────────── */
/* Gates allowed in pure AC⁰: INPUT, AND, OR, NOT, CONST.
 * (Fan-in 2 gates are a special case of unbounded fan-in with nf=2.) */
int ac0_is_allowed_gate(AC0GateType t);

/* ─── AC⁰ Depth-d Circuit ───────────────────────────────────────── */
/* An AC⁰[d] circuit has depth exactly d (alternating AND/OR layers).
 * By convention, we push negations to inputs via De Morgan. */
typedef struct {
    AC0Circuit *circuit;
    int         depth_bound;
} AC0DepthCircuit;

AC0DepthCircuit* ac0_depth_circuit_create(int depth, int n_inputs);
void             ac0_depth_circuit_free(AC0DepthCircuit *c);

/* Build a depth-d AC⁰ circuit for THRESHOLD-k via DNF or CNF.
 * DNF = OR-of-ANDs (depth 2), CNF = AND-of-ORs (depth 2).
 * For depth > 2: alternating layers of AND/OR with fan-in reduction. */
AC0DepthCircuit* ac0_depth_d_build_threshold(int n, int k, int depth);

/* ─── DNF (Disjunctive Normal Form) ──────────────────────────────── */
/* DNF = OR of AND terms. Each AND term is a conjunction of literals.
 * Depth = 2. Size = number of terms × n. */
typedef struct {
    int  **terms;      /* terms[t][i] ∈ {0, 1, -1} = {neg, pos, absent} */
    int   *term_sizes;  /* number of literals in each term */
    int    n_terms;
    int    n_vars;
} DNF;

DNF*        dnf_create(int n_vars, int n_terms);
void        dnf_free(DNF *d);
void        dnf_set_literal(DNF *d, int term, int var, int polarity);
int         dnf_evaluate(DNF *d, const int *assignment);
AC0Circuit* dnf_to_circuit(DNF *d);
DNF*        circuit_to_dnf(AC0Circuit *c);
void        dnf_print(DNF *d);

/* ─── CNF (Conjunctive Normal Form) ──────────────────────────────── */
/* CNF = AND of OR clauses. Dual of DNF. */
typedef struct {
    int  **clauses;
    int   *clause_sizes;
    int    n_clauses;
    int    n_vars;
} CNF;

CNF*        cnf_create(int n_vars, int n_clauses);
void        cnf_free(CNF *c);
void        cnf_set_literal(CNF *c, int clause, int var, int polarity);
int         cnf_evaluate(CNF *c, const int *assignment);
AC0Circuit* cnf_to_circuit(CNF *c);
CNF*        circuit_to_cnf(AC0Circuit *c);
void        cnf_print(CNF *c);

/* ─── Sipser Functions ───────────────────────────────────────────── */
/* S_d: {0,1}^n → {0,1}. S_d ∈ AC⁰[d+1] but S_d ∉ AC⁰[d].
 * These functions separate levels of the AC⁰ depth hierarchy. */
typedef struct {
    int n;     /* number of inputs */
    int d;     /* depth parameter */
} SipserFunction;

int    sipser_evaluate(SipserFunction *sf, const int *x);
double sipser_ac0_lower_bound(int n, int d);

/* ─── AC⁰ Completeness ──────────────────────────────────────────── */
/* There are NO known natural AC⁰-complete problems under AC⁰ reductions.
 * The class AC⁰ is not closed under many natural reductions,
 * making the concept of "AC⁰-complete" subtle.
 *
 * However, PARITY serves as a natural AC⁰-lower-bound:
 * PARITY ∉ AC⁰, but PARITY ∈ AC⁰[2] ⊆ NC¹. */

/* ─── AC⁰[p]: AC⁰ with MOD_p gates ──────────────────────────────── */
/* AC⁰[p] extends AC⁰ with MOD_p gates (output 1 iff sum ≡ 0 mod p).
 * Key results:
 *   AC⁰[p] = AC⁰[q] iff p and q have same prime factors (mod p).
 *   MAJORITY ∉ AC⁰[p] for p ≠ 2 (Razborov-Smolensky 1987). */
int ac0_modp_can_compute_parity(int p);
int ac0_modp_can_compute_majority(int p);

#endif /* AC0_CIRCUIT_H */
