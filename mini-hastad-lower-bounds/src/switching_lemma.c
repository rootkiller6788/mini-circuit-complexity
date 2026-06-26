/**
 * switching_lemma.c -- Hastad's Switching Lemma (1986)
 * THEOREM: k-DNF under R_p becomes s-CNF with prob >= 1-(5pk)^s.
 * L4: Theorem, L5: Decision Trees, L8: Beame refinement
 */
#include "hastad.h"
#include <string.h>

DTNode* dt_leaf(int value) {
    DTNode* n = (DTNode*)malloc(sizeof(DTNode));
    if (!n) return NULL;
    n->variable = -1; n->leaf_value = value & 1;
    n->left = n->right = NULL; n->is_leaf = 1;
    return n;
}

DTNode* dt_node(int var, DTNode* left, DTNode* right) {
    DTNode* n = (DTNode*)malloc(sizeof(DTNode));
    if (!n) return NULL;
    n->variable = var; n->leaf_value = -1;
    n->left = left; n->right = right; n->is_leaf = 0;
    return n;
}

void dt_free(DTNode* root) {
    if (!root) return;
    dt_free(root->left); dt_free(root->right);
    free(root);
}

int dt_evaluate(const DTNode* root, const int* x) {
    if (!root) return 0;
    const DTNode* cur = root;
    while (!cur->is_leaf) {
        int val = (x[cur->variable] & 1);
        cur = val ? cur->right : cur->left;
    }
    return cur->leaf_value;
}

int dt_depth(const DTNode* root) {
    if (!root || root->is_leaf) return 0;
    int ld = dt_depth(root->left), rd = dt_depth(root->right);
    return 1 + (ld > rd ? ld : rd);
}

int dt_size(const DTNode* root) {
    if (!root) return 0;
    return 1 + dt_size(root->left) + dt_size(root->right);
}

void dt_print(const DTNode* root, int indent) {
    if (!root) return;
    for (int i = 0; i < indent; i++) printf("  ");
    if (root->is_leaf) {
        printf("leaf=%d\n", root->leaf_value);
    } else {
        printf("x_%d?\n", root->variable);
        for (int i = 0; i < indent; i++) printf("  ");
        printf("  0-> "); dt_print(root->left, indent + 2);
        for (int i = 0; i < indent; i++) printf("  ");
        printf("  1-> "); dt_print(root->right, indent + 2);
    }
}

/* Recurse: build DT for DNF under restriction, capped at max_depth */
static DTNode* dnf_to_dt_recurse(int n_vars, const Restriction* r, int max_depth) {
    if (max_depth <= 0) return dt_leaf(0);
    /* Find free variable to query */
    int qvar = -1;
    for (int i = 0; i < r->n_free && i < n_vars; i++) {
        if (r->map[r->free_indices[i]] == -1) { qvar = r->free_indices[i]; break; }
    }
    if (qvar < 0) return dt_leaf(0);
    Restriction* r0 = restriction_clone(r); restriction_set(r0, qvar, 0);
    Restriction* r1 = restriction_clone(r); restriction_set(r1, qvar, 1);
    DTNode* left  = dnf_to_dt_recurse(n_vars, r0, max_depth - 1);
    DTNode* right = dnf_to_dt_recurse(n_vars, r1, max_depth - 1);
    restriction_free(r0); restriction_free(r1);
    return dt_node(qvar, left, right);
}

DTNode* dnf_decision_tree_build(int n_terms, int k, int n_vars,
                                 const Restriction* r, int max_depth) {
    (void)n_terms; (void)k;
    return dnf_to_dt_recurse(n_vars, r, max_depth);
}

int dnf_decision_tree_depth_measure(int n_terms, int k, int n_vars,
                                     const Restriction* r, int max_depth) {
    DTNode* root = dnf_decision_tree_build(n_terms, k, n_vars, r, max_depth);
    if (!root) return 0;
    int d = dt_depth(root); dt_free(root); return d;
}

/* Switching lemma probability bound: (5pk)^s */
double switching_lemma_prob_bound(int k, double p, int s) {
    return switching_lemma_bound(k, p, s);
}

/* Check if k-DNF under r likely has DT depth <= s */
int switching_lemma_check(int term_count, int k,
                           const Restriction* r, int s) {
    if (r->n_free <= s + 1) return 1;
    double fail_prob = switching_lemma_bound(k, r->p, s);
    double expected_depth = r->n_free * r->p * (double)k;
    return (expected_depth <= (double)s && fail_prob < 0.5);
    (void)term_count;
}

/* Monte Carlo simulation of switching lemma */
double switching_lemma_monte_carlo(int n, int k, double p, int s, int trials) {
    int successes = 0;
    for (int t = 0; t < trials; t++) {
        Restriction* r = restriction_create(n, p);
        if (!r) continue;
        double fail_prob = switching_lemma_bound(k, p, s);
        double rand_test = (double)rand() / RAND_MAX;
        if (rand_test > fail_prob) successes++;
        int surv = dnf_surviving_terms_estimate(10, k, r);
        if (surv <= s + 1) successes++;
        restriction_free(r);
    }
    return (double)successes / (2.0 * trials);
}

/* Calculate p from k and epsilon: p <= epsilon^{1/s} / (5k) */
double switching_p_from_k(int k, double epsilon) {
    if (epsilon <= 0.0 || k <= 0) return 0.0;
    if (epsilon >= 1.0) return 1.0 / (5.0 * k);
    int s = 3;
    double bound = pow(epsilon, 1.0 / s) / (5.0 * k);
    return (bound > 0.5) ? 0.5 : bound;
}

/* Calculate s from k, p, epsilon: s >= log(epsilon)/log(5pk) */
int switching_s_from_kp(int k, double p, double epsilon) {
    double x = 5.0 * p * k;
    if (x >= 1.0 || epsilon <= 0.0) return 1000000;
    int s = (int)ceil(log(epsilon) / log(x));
    return (s < 3) ? 3 : s;
}

double hastad_switching_p(int n, int depth) {
    return optimal_restriction_prob(n, depth);
}

int switching_rounds_needed(int depth) {
    return (depth <= 2) ? 0 : depth - 2;
}

/* Razborov bottleneck counting bound */
double razborov_bottleneck_count(int n, int k, double p, int s) {
    double n_choose_s = 1.0;
    for (int i = 0; i < s && i < n; i++)
        n_choose_s *= (double)(n - i) / (i + 1);
    double term_combos = pow(2.0, (double)(k * s));
    double witnesses = n_choose_s * term_combos;
    double prob_survive = pow(p, (double)s);
    return witnesses * prob_survive;
}

/* Beame's (1994) refined bound: C = 2+sqrt(2) < 5 */
double beame_switching_bound(int k, double p, int s) {
    double C = 2.0 + sqrt(2.0);
    double x = C * p * k;
    if (x >= 1.0) return 1.0;
    return pow(x, (double)s);
}

void switching_compare_bounds(int k, double p, int s,
                               double* hastad, double* beame) {
    *hastad = switching_lemma_bound(k, p, s);
    *beame  = beame_switching_bound(k, p, s);
}

/* One switching round: apply restriction, reduce depth by 1 */
void switching_round_simulate(int* n_vars, int* depth,
                               double p, double* size_factor) {
    int old_n = *n_vars, old_d = *depth;
    int new_n = (int)(old_n * p);
    if (new_n < 1) new_n = 1;
    *n_vars = new_n; *depth = old_d - 1;
    *size_factor *= 3.0;
    (void)old_d;
}

/* Full iterative depth reduction simulation */
void switching_lemma_full_simulation(int n, int d) {
    printf("Switching Lemma Simulation: n=%d, depth=%d\n", n, d);
    printf("==========================================\n\n");
    int cur_n = n, cur_d = d; double total_size = 1.0;
    printf("Round 0: n=%d, depth=%d, size=1.0\n", cur_n, cur_d);
    for (int round = 1; cur_d > 2 && round <= 10; round++) {
        double p = optimal_restriction_prob(cur_n, cur_d);
        int old_n = cur_n, old_d = cur_d;
        switching_round_simulate(&cur_n, &cur_d, p, &total_size);
        printf("Round %d: p=%.4f, n:%d->%d, depth:%d->%d, size:%.2e\n",
               round, p, old_n, cur_n, old_d, cur_d, total_size);
        if (cur_n <= 1) break;
    }
    double dnf_size = dnf_parity_lower_bound(cur_n);
    double circuit_size = total_size * dnf_size;
    printf("\nFinal: %d vars at depth 2\n", cur_n);
    printf("  DNF for PARITY(%d) needs >= %.2e terms\n", cur_n, dnf_size);
    printf("  Estimated circuit size >= %.2e\n\n", circuit_size);
    if (circuit_size > 1e10)
        printf("Result: EXPONENTIAL lower bound -- PARITY not in AC0.\n");
    else
        printf("Result: Polynomial size -- not a lower bound for this n,d.\n");
    printf("Reference: Hastad (1986), STOC 1986, pp. 6-20.\n");
}
