/**
 * switching_lemma.h — Hastad's Switching Lemma (1986)
 *
 * THEOREM (Switching Lemma): Let f be a k-DNF. Let ρ ~ R_p
 * (random restriction, each var kept free with prob p).
 * Then: Pr[f|ρ is not an s-CNF] ≤ (5pk)^s.
 *
 * In other words: after a random restriction, any k-DNF
 * collapses to an s-CNF with high probability (for small enough p).
 *
 * This is THE key technical lemma behind:
 *   - PARITY ∉ AC0 (Hastad 1986)
 *   - AC0 depth hierarchy (Sipser 1983)
 *   - Fourier concentration of AC0 (Linial-Mansour-Nisan 1993)
 *
 * L4: Switching Lemma (theorem statement and proof)
 * L5: Decision tree method, Razborov's proof
 * L8: Optimal parameters, Beame's refinement
 */
#ifndef SWITCHING_LEMMA_H
#define SWITCHING_LEMMA_H

#include <stdint.h>

/* ── Decision Tree ───────────────────────────────────────────── */

/** Decision tree node */
typedef struct DTNode {
    int variable;        /* variable queried at this node (-1 if leaf) */
    int leaf_value;      /* value if leaf (0 or 1) */
    struct DTNode* left; /* child when variable=0 */
    struct DTNode* right;/* child when variable=1 */
    int is_leaf;
} DTNode;

/** Create a decision tree leaf */
DTNode* dt_leaf(int value);

/** Create a decision tree node querying a variable */
DTNode* dt_node(int var, DTNode* left, DTNode* right);

/** Free entire decision tree */
void dt_free(DTNode* root);

/** Evaluate decision tree on input x */
int dt_evaluate(const DTNode* root, const int* x);

/** Depth of decision tree */
int dt_depth(const DTNode* root);

/** Number of nodes in decision tree */
int dt_size(const DTNode* root);

/** Print decision tree */
void dt_print(const DTNode* root, int indent);

/* ── Switching Lemma (Core) ──────────────────────────────────── */

/**
 * Switching Lemma Statement:
 *   k-DNF under R_p becomes s-CNF with prob ≥ 1 - (5pk)^s.
 *
 * This function checks if a restricted DNF IS representable as
 * an s-CNF (by checking the decision tree depth of each term).
 *
 * @param k     DNF term width (max literals per term)
 * @param p     Restriction probability
 * @param s     Target CNF width (max clauses)
 * @return      Upper bound on failure probability (Pr[f|ρ not s-CNF])
 */
double switching_lemma_prob_bound(int k, double p, int s);

/**
 * Check if a k-DNF restricted by r has decision tree depth ≤ s.
 * This is the constructive form: if DT depth ≤ s, then the
 * function is representable as both s-CNF and s-DNF.
 *
 * @param term_count  Number of DNF terms
 * @param k           Max term width
 * @param r           Restriction applied
 * @param s           Target depth
 * @return            1 if restricted DNF has DT depth ≤ s, 0 otherwise
 */
int switching_lemma_check(int term_count, int k,
                           const Restriction* r, int s);

/**
 * Simulate the switching lemma experimentally:
 * Generate random k-DNF, apply R_p restriction, check if
 * resulting function has small decision tree.
 *
 * @param n     Initial variables
 * @param k     DNF width
 * @param p     Restriction probability
 * @param s     Target depth
 * @param trials Number of trials
 * @return      Fraction of trials where DT depth ≤ s
 */
double switching_lemma_monte_carlo(int n, int k, double p, int s, int trials);

/* ── Switching Lemma: Parameter Selection ────────────────────── */

/**
 * Given k and acceptable failure probability ε, compute
 * the necessary restriction probability p.
 */
double switching_p_from_k(int k, double epsilon);

/**
 * Given k, p, and failure probability ε, compute the
 * necessary decision tree depth s.
 */
int switching_s_from_kp(int k, double p, double epsilon);

/**
 * Optimal parameters for Hastad's depth reduction:
 * p = n^{-1/(d-1)}, which balances surviving variables
 * with switching probability.
 */
double hastad_switching_p(int n, int depth);

/**
 * Compute how many switching rounds are needed to reduce
 * depth from d to 2.
 */
int switching_rounds_needed(int depth);

/* ── Switching Lemma: Proof Components ───────────────────────── */

/**
 * Razborov's proof (1995) uses the "bottleneck counting" method.
 * Key idea: for f|ρ to NOT be representable as s-CNF,
 * there must exist a "canonical" assignment witnessing this.
 * Count such assignments and union-bound.
 *
 * This function estimates the count bound.
 */
double razborov_bottleneck_count(int n, int k, double p, int s);

/**
 * Beame's refinement (1994): the switching lemma with
 * better constants — (Cpk)^s where C ≈ 4 instead of 5.
 */
double beame_switching_bound(int k, double p, int s);

/**
 * Compare the two bounds (Hastad vs Beame) for given parameters.
 */
void switching_compare_bounds(int k, double p, int s,
                               double* hastad_bound, double* beame_bound);

/* ── Switching Lemma: Applications ───────────────────────────── */

/**
 * Apply one switching round to reduce circuit depth by 1.
 * Input: circuit of depth d (bottom level AND/OR over inputs).
 * After R_p restriction + switching lemma: circuit becomes depth d-1.
 *
 * Returns the new circuit depth and estimates size change.
 */
void switching_round_simulate(int* n_vars, int* depth,
                               double p, double* size_factor);

#endif /* SWITCHING_LEMMA_H */
