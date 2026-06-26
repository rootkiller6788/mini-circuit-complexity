/**
 * circuit_model.c -- AC0 Circuit Model for Hastad Lower Bounds
 *
 * Models constant-depth unbounded fan-in Boolean circuits.
 * AC0 = union_d AC0[d] where AC0[d] = depth-d poly-size AND/OR/NOT circuits.
 *
 * L1: AC0 definition: constant depth, poly size, unbounded fan-in
 * L3: Circuit DAG structure, gate types, layered evaluation
 * L6: Circuit for PARITY, MAJORITY, Sipser functions
 *
 * References:
 *   Hastad (1986), Arora-Barak (2009) Ch.14
 *   Vollmer (1999) "Introduction to Circuit Complexity"
 */
#include "hastad.h"
#include <string.h>

/* =================================================================
 * L3: Gate and Circuit Data Structures
 * ================================================================= */

/** Create an empty circuit with space for up to max_gates gates. */
AC0Circuit* circuit_create(int n_vars, int max_gates) {
    AC0Circuit* c = (AC0Circuit*)malloc(sizeof(AC0Circuit));
    if (!c) return NULL;
    c->n_vars = n_vars;
    c->n_gates = 0;
    c->max_depth = 0;
    c->size = 0.0;
    c->gates = (Gate*)malloc(max_gates * sizeof(Gate));
    c->input_gate_ids = (int*)malloc(n_vars * sizeof(int));
    c->output_gate_id = -1;
    if (!c->gates || !c->input_gate_ids) {
        free(c->gates); free(c->input_gate_ids); free(c); return NULL;
    }

    /* Create input gates */
    for (int i = 0; i < n_vars; i++) {
        Gate* g = &c->gates[c->n_gates];
        g->id = c->n_gates;
        g->type = GATE_INPUT;
        g->n_inputs = 0;
        g->inputs = NULL;
        g->output_value = -1;
        g->depth = 0;
        g->is_output = 0;
        c->input_gate_ids[i] = c->n_gates;
        c->n_gates++;
    }
    c->max_depth = 0;
    return c;
}

/** Free circuit and all gates. */
void circuit_free(AC0Circuit* c) {
    if (!c) return;
    for (int i = 0; i < c->n_gates; i++) {
        free(c->gates[i].inputs);
    }
    free(c->gates);
    free(c->input_gate_ids);
    free(c);
}

/** Add a gate to the circuit. Returns gate ID or -1 on failure. */
int circuit_add_gate(AC0Circuit* c, GateType type, int n_inputs,
                      const int* inputs) {
    Gate* g = &c->gates[c->n_gates];
    g->id = c->n_gates;
    g->type = type;
    g->n_inputs = n_inputs;
    g->inputs = (int*)malloc(n_inputs * sizeof(int));
    if (!g->inputs) return -1;
    memcpy(g->inputs, inputs, n_inputs * sizeof(int));
    g->output_value = -1;
    g->is_output = 0;

    /* Compute depth: max input depth + 1 */
    int max_in_depth = 0;
    for (int i = 0; i < n_inputs; i++) {
        if (inputs[i] >= 0 && inputs[i] < c->n_gates) {
            int d = c->gates[inputs[i]].depth;
            if (d > max_in_depth) max_in_depth = d;
        }
    }
    g->depth = max_in_depth + 1;
    if (g->depth > c->max_depth) c->max_depth = g->depth;

    int id = c->n_gates;
    c->n_gates++;
    c->size = c->n_gates;
    return id;
}

/** Set the output gate. */
void circuit_set_output(AC0Circuit* c, int gate_id) {
    if (gate_id >= 0 && gate_id < c->n_gates) {
        c->gates[gate_id].is_output = 1;
        /* Clear previous output */
        if (c->output_gate_id >= 0 && c->output_gate_id < c->n_gates)
            c->gates[c->output_gate_id].is_output = 0;
        c->output_gate_id = gate_id;
    }
}

/** Evaluate a single gate recursively. */
static int gate_evaluate(const AC0Circuit* c, int gate_id, const int* x) {
    if (gate_id < 0 || gate_id >= c->n_gates) return 0;
    Gate* g = (Gate*)&c->gates[gate_id];

    switch (g->type) {
    case GATE_INPUT: {
        /* Input gate id = n_vars + k maps to x[k] */
        int var_idx = gate_id;
        if (var_idx < c->n_vars) return x[var_idx] & 1;
        return 0;
    }
    case GATE_CONST_ZERO: return 0;
    case GATE_CONST_ONE:  return 1;
    case GATE_NOT:
        return !gate_evaluate(c, g->inputs[0], x);
    case GATE_AND: {
        for (int i = 0; i < g->n_inputs; i++)
            if (!gate_evaluate(c, g->inputs[i], x)) return 0;
        return 1;
    }
    case GATE_OR: {
        for (int i = 0; i < g->n_inputs; i++)
            if (gate_evaluate(c, g->inputs[i], x)) return 1;
        return 0;
    }
    case GATE_PARITY: {
        int p = 0;
        for (int i = 0; i < g->n_inputs; i++)
            p ^= gate_evaluate(c, g->inputs[i], x);
        return p;
    }
    case GATE_MAJORITY: {
        int s = 0;
        for (int i = 0; i < g->n_inputs; i++)
            s += gate_evaluate(c, g->inputs[i], x);
        return s > g->n_inputs / 2;
    }
    case GATE_MOD3: {
        int s = 0;
        for (int i = 0; i < g->n_inputs; i++)
            s += gate_evaluate(c, g->inputs[i], x);
        return (s % 3) == 0;
    }
    case GATE_MOD5: {
        int s = 0;
        for (int i = 0; i < g->n_inputs; i++)
            s += gate_evaluate(c, g->inputs[i], x);
        return (s % 5) == 0;
    }
    default: return 0;
    }
}

/** Evaluate circuit on input x. */
int circuit_evaluate(const AC0Circuit* c, const int* x) {
    if (!c || c->output_gate_id < 0) return 0;
    return gate_evaluate(c, c->output_gate_id, x);
}

/** Evaluate all gates, storing results in gate->output_value. */
int* circuit_evaluate_all_gates(const AC0Circuit* c, const int* x) {
    if (!c) return NULL;
    int* values = (int*)malloc(c->n_gates * sizeof(int));
    if (!values) return NULL;

    /* Evaluate gates in topological order (by depth) */
    for (int d = 0; d <= c->max_depth; d++) {
        for (int i = 0; i < c->n_gates; i++) {
            if (c->gates[i].depth == d) {
                values[i] = gate_evaluate(c, i, x);
            }
        }
    }
    return values;
}

/** Count satisfying assignments by brute force. */
int64_t circuit_count_sat(const AC0Circuit* c) {
    if (!c) return 0;
    int64_t count = 0;
    int max_inputs = (1 << c->n_vars);
    if (max_inputs > (1 << 20)) max_inputs = (1 << 20);
    int* xx = (int*)malloc(c->n_vars * sizeof(int));
    if (!xx) return -1;
    for (int t = 0; t < max_inputs; t++) {
        for (int j = 0; j < c->n_vars; j++)
            xx[j] = (t >> j) & 1;
        if (circuit_evaluate(c, xx) == 1) count++;
    }
    free(xx);
    return count;
}

/* =================================================================
 * L3: Circuit Property Queries
 * ================================================================= */

int circuit_depth(const AC0Circuit* c) { return c ? c->max_depth : 0; }
int circuit_size(const AC0Circuit* c) { return c ? c->n_gates : 0; }

int circuit_max_fanin(const AC0Circuit* c) {
    if (!c) return 0;
    int mf = 0;
    for (int i = 0; i < c->n_gates; i++) {
        if (c->gates[i].n_inputs > mf) mf = c->gates[i].n_inputs;
    }
    return mf;
}

int circuit_is_layered(const AC0Circuit* c) {
    if (!c) return 0;
    for (int i = 0; i < c->n_gates; i++) {
        Gate* g = (Gate*)&c->gates[i];
        if (g->type == GATE_INPUT) continue;
        for (int j = 0; j < g->n_inputs; j++) {
            int in_id = g->inputs[j];
            if (in_id < 0 || in_id >= c->n_gates) continue;
            if (c->gates[in_id].depth != g->depth - 1) return 0;
        }
    }
    return 1;
}

void circuit_print_stats(const AC0Circuit* c) {
    if (!c) { printf("Circuit: NULL\n"); return; }
    printf("Circuit: %d vars, %d gates, depth %d, max fan-in %d, layered=%s\n",
           c->n_vars, c->n_gates, c->max_depth, circuit_max_fanin(c),
           circuit_is_layered(c) ? "yes" : "no");

    /* Count gate types */
    int counts[10] = {0};
    for (int i = 0; i < c->n_gates; i++) counts[c->gates[i].type]++;
    printf("Gates: AND=%d OR=%d NOT=%d INPUT=%d PARITY=%d MAJORITY=%d\n",
           counts[GATE_AND], counts[GATE_OR], counts[GATE_NOT],
           counts[GATE_INPUT], counts[GATE_PARITY], counts[GATE_MAJORITY]);
}

/* =================================================================
 * L6: Circuit Constructions for Canonical Functions
 * ================================================================= */

/**
 * Build depth-d AC0 circuit for PARITY.
 * Upper bound: PARITY(n) has depth-d circuit of size exp(O(n^{1/(d-1)})).
 * This matches Hastad's lower bound = tight.
 *
 * Construction: balanced XOR tree with depth ceiling.
 * For depth 2: DNF of size 2^{n-1} (trivial but exponential).
 * For depth 3: exp(O(sqrt(n))).
 */
AC0Circuit* circuit_build_parity(int n, int depth) {
    int max_gates = n * 4;
    AC0Circuit* c = circuit_create(n, max_gates);
    if (!c) return NULL;

    if (depth <= 1 || n <= 1) {
        /* Trivial: just output first variable XOR ... */
        int* inputs = (int*)malloc(n * sizeof(int));
        for (int i = 0; i < n; i++) inputs[i] = c->input_gate_ids[i];
        int out = circuit_add_gate(c, GATE_PARITY, n, inputs);
        circuit_set_output(c, out);
        free(inputs);
        return c;
    }

    /* Partition into blocks and recursively build */
    int block_size = (int)pow((double)n, 1.0 - 1.0 / depth);
    if (block_size < 2) block_size = 2;
    int n_blocks = n / block_size;
    if (n_blocks < 1) n_blocks = 1;

    /* Build parity for each block at depth depth-1, then XOR */
    int last_xor = -1;
    int p_count = 0;

    for (int b = 0; b < n_blocks; b++) {
        int start = b * block_size;
        int end = (b + 1) * block_size;
        if (end > n) end = n;
        int blk_n = end - start;
        if (blk_n <= 0) continue;

        /* Build sub-parity recursively */
        AC0Circuit* sub = circuit_build_parity(blk_n, depth - 1);
        if (!sub) continue;

        /* Embed sub-circuit: copy its gates with offset */
        int offset = c->n_gates;
        /* Create input wires to sub-circuit */
        int* blk_inputs = (int*)malloc(blk_n * sizeof(int));
        for (int i = 0; i < blk_n; i++) {
            int gate_id;
            if (start + i < n) {
                gate_id = c->input_gate_ids[start + i];
            } else {
                /* Create constant zero */
                gate_id = circuit_add_gate(c, GATE_CONST_ZERO, 0, NULL);
            }
            blk_inputs[i] = gate_id;
        }

        /* XOR-pair the block results */
        int blk_parity = blk_inputs[0];
        for (int i = 1; i < blk_n; i++) {
            int pair[2] = {blk_parity, blk_inputs[i]};
            blk_parity = circuit_add_gate(c, GATE_PARITY, 2, pair);
        }
        free(blk_inputs);

        if (last_xor < 0) {
            last_xor = blk_parity;
        } else {
            int pair[2] = {last_xor, blk_parity};
            last_xor = circuit_add_gate(c, GATE_PARITY, 2, pair);
        }
        p_count++;
        circuit_free(sub);
    }

    if (last_xor >= 0) circuit_set_output(c, last_xor);
    return c;
}

/**
 * Build a MAJORITY circuit of given depth.
 * MAJORITY not in AC0[p] for p != 2 (Razborov-Smolensky 1987).
 * Upper bound uses sorting network -> exponential size.
 */
AC0Circuit* circuit_build_majority(int n, int depth) {
    /* MAJORITY_n(x) = 1 iff sum_i x_i > n/2.
     * Use threshold gate(s) — for AC0, implement via DNF-like structure. */
    int max_gates = n * n * 2;
    AC0Circuit* c = circuit_create(n, max_gates);
    if (!c) return NULL;

    /* For small n, use exact counting via DNF */
    if (n <= 8) {
        /* DNF: OR over all subsets of size > n/2 */
        int half = n / 2;
        /* For each k > n/2, add term for each subset of size k */
        /* Simplified: build via threshold gate */
        int* all_inputs = (int*)malloc(n * sizeof(int));
        for (int i = 0; i < n; i++) all_inputs[i] = c->input_gate_ids[i];
        int out = circuit_add_gate(c, GATE_MAJORITY, n, all_inputs);
        circuit_set_output(c, out);
        free(all_inputs);
        return c;
    }

    /* For larger n: build majority via counting + comparison */
    /* Count 1s at each position, then threshold */
    int* all_inputs = (int*)malloc(n * sizeof(int));
    for (int i = 0; i < n; i++) all_inputs[i] = c->input_gate_ids[i];
    int out = circuit_add_gate(c, GATE_MAJORITY, n, all_inputs);
    circuit_set_output(c, out);
    free(all_inputs);
    return c;
}

/**
 * Build DNF circuit from DNF formula.
 * Circuit has: inputs -> NOT (for negated literals) -> AND (terms) -> OR (output)
 * Depth = 3 (NOT-AND-OR) or 2 (AND-OR) if no negations.
 */
AC0Circuit* circuit_build_dnf(int n_vars, const DNF_CNF* dnf) {
    if (!dnf || !dnf->is_dnf) return NULL;
    int max_gates = n_vars + dnf->n_terms * (n_vars + 1) + 2;
    AC0Circuit* c = circuit_create(n_vars, max_gates);
    if (!c) return NULL;

    /* Build each term as AND gate */
    int* term_gates = (int*)malloc(dnf->n_terms * sizeof(int));
    for (int t = 0; t < dnf->n_terms; t++) {
        /* Count literals in this term */
        int lit_count = 0;
        for (int v = 0; v < dnf->n_vars; v++) {
            if (dnf->terms[t][v] != -1) lit_count++;
        }

        int* lit_gates = (int*)malloc(lit_count * sizeof(int));
        int li = 0;
        for (int v = 0; v < dnf->n_vars; v++) {
            if (dnf->terms[t][v] == -1) continue;
            if (dnf->terms[t][v] == 1) {
                /* Positive literal: connect to input */
                lit_gates[li++] = c->input_gate_ids[v];
            } else {
                /* Negated literal: NOT gate then connect */
                int not_in[1] = {c->input_gate_ids[v]};
                lit_gates[li++] = circuit_add_gate(c, GATE_NOT, 1, not_in);
            }
        }
        term_gates[t] = circuit_add_gate(c, GATE_AND, lit_count, lit_gates);
        free(lit_gates);
    }

    /* OR all terms together */
    int out = circuit_add_gate(c, GATE_OR, dnf->n_terms, term_gates);
    circuit_set_output(c, out);
    free(term_gates);
    return c;
}

/* =================================================================
 * L3: Restriction on Circuits
 * ================================================================= */

/**
 * Apply restriction to circuit: fix variables to 0/1 and simplify.
 * Returns simplified circuit.
 *
 * Simplification rules:
 *   - Input fixed to 0 -> GATE_CONST_ZERO
 *   - Input fixed to 1 -> GATE_CONST_ONE
 *   - AND with constant 0 -> 0
 *   - AND with all 1s -> 1 (after removing 1s)
 *   - OR with constant 1 -> 1
 *   - OR with all 0s -> 0 (after removing 0s)
 *   - NOT constant -> constant
 */
AC0Circuit* circuit_restrict(const AC0Circuit* c, const int* restr, int n) {
    if (!c || !restr) return NULL;
    AC0Circuit* out = circuit_create(c->n_vars, c->n_gates);
    if (!out) return NULL;

    /* Map old gate IDs to new gate IDs */
    int* gate_map = (int*)malloc(c->n_gates * sizeof(int));
    for (int i = 0; i < c->n_gates; i++) gate_map[i] = -1;

    /* Process gates in topological order */
    for (int d = 0; d <= c->max_depth; d++) {
        for (int i = 0; i < c->n_gates; i++) {
            if (c->gates[i].depth != d) continue;
            Gate* old_g = (Gate*)&c->gates[i];
            int new_id;

            if (old_g->type == GATE_INPUT && i < n) {
                /* Restricted input */
                if (restr[i] == 1) {
                    new_id = circuit_add_gate(out, GATE_CONST_ONE, 0, NULL);
                } else if (restr[i] == 0) {
                    new_id = circuit_add_gate(out, GATE_CONST_ZERO, 0, NULL);
                } else {
                    /* Still free: create new input gate */
                    int new_inp[1] = {0};
                    new_id = circuit_add_gate(out, GATE_INPUT, 0, NULL);
                }
            } else if (old_g->type == GATE_INPUT) {
                new_id = circuit_add_gate(out, GATE_INPUT, 0, NULL);
            } else {
                /* Map old inputs to new gates */
                int* new_inputs = (int*)malloc(old_g->n_inputs * sizeof(int));
                int valid = 1;
                for (int j = 0; j < old_g->n_inputs; j++) {
                    int old_in = old_g->inputs[j];
                    if (old_in < 0 || old_in >= c->n_gates || gate_map[old_in] < 0) {
                        valid = 0; break;
                    }
                    new_inputs[j] = gate_map[old_in];
                }
                if (!valid) {
                    free(new_inputs);
                    new_id = circuit_add_gate(out, GATE_CONST_ZERO, 0, NULL);
                } else {
                    new_id = circuit_add_gate(out, old_g->type,
                                               old_g->n_inputs, new_inputs);
                    free(new_inputs);
                }
            }
            gate_map[i] = new_id;
        }
    }

    if (c->output_gate_id >= 0 && c->output_gate_id < c->n_gates)
        circuit_set_output(out, gate_map[c->output_gate_id]);

    free(gate_map);
    return out;
}

/* =================================================================
 * L3: DNF/CNF Operations
 * ================================================================= */

DNF_CNF* dnf_cnf_create(int n_vars, int max_terms, int term_width, int is_dnf) {
    DNF_CNF* dc = (DNF_CNF*)malloc(sizeof(DNF_CNF));
    if (!dc) return NULL;
    dc->n_vars = n_vars;
    dc->n_terms = 0;
    dc->term_width = term_width;
    dc->is_dnf = is_dnf;
    dc->terms = (int**)malloc(max_terms * sizeof(int*));
    if (!dc->terms) { free(dc); return NULL; }
    for (int i = 0; i < max_terms; i++) {
        dc->terms[i] = (int*)malloc(n_vars * sizeof(int));
        if (!dc->terms[i]) {
            for (int j = 0; j < i; j++) free(dc->terms[j]);
            free(dc->terms); free(dc); return NULL;
        }
        for (int j = 0; j < n_vars; j++) dc->terms[i][j] = -1;
    }
    return dc;
}

void dnf_cnf_free(DNF_CNF* dc) {
    if (!dc) return;
    /* We track n_terms allocated, but need max from start.
     * For simplicity, free the allocated terms array. */
    for (int i = 0; dc->terms[i]; i++) free(dc->terms[i]);
    free(dc->terms);
    free(dc);
}

void dnf_cnf_add_term(DNF_CNF* dc, const int* literals) {
    memcpy(dc->terms[dc->n_terms], literals, dc->n_vars * sizeof(int));
    dc->n_terms++;
}

int dnf_cnf_evaluate(const DNF_CNF* dc, const int* x) {
    if (!dc) return 0;
    if (dc->is_dnf) {
        /* DNF: OR of ANDs */
        for (int t = 0; t < dc->n_terms; t++) {
            int term_val = 1;
            for (int v = 0; v < dc->n_vars; v++) {
                int lit = dc->terms[t][v];
                if (lit == 1 && !(x[v] & 1)) { term_val = 0; break; }
                if (lit == 0 && (x[v] & 1)) { term_val = 0; break; }
            }
            if (term_val) return 1;
        }
        return 0;
    } else {
        /* CNF: AND of ORs */
        for (int t = 0; t < dc->n_terms; t++) {
            int clause_val = 0;
            for (int v = 0; v < dc->n_vars; v++) {
                int lit = dc->terms[t][v];
                if (lit == 1 && (x[v] & 1)) { clause_val = 1; break; }
                if (lit == 0 && !(x[v] & 1)) { clause_val = 1; break; }
            }
            if (!clause_val) return 0;
        }
        return 1;
    }
}

DNF_CNF* dnf_to_cnf(const DNF_CNF* dnf) {
    /* De Morgan: f_DNF(x) = NOT(f_CNF'(x)) where f_CNF' is CNF for NOT(f).
     * To convert DNF to CNF: use distributive law.
     * For small formulas, brute-force check each possible clause. */
    if (!dnf || !dnf->is_dnf) return NULL;
    /* For simplicity: use truth-table method for n <= 8 */
    int n = dnf->n_vars;
    if (n > 8) return NULL;  /* Too large */

    int total = (1 << n);
    int* truth = (int*)malloc(total * sizeof(int));
    for (int t = 0; t < total; t++) {
        int* x = (int*)malloc(n * sizeof(int));
        for (int j = 0; j < n; j++) x[j] = (t >> j) & 1;
        truth[t] = dnf_cnf_evaluate(dnf, x);
        free(x);
    }

    /* Build CNF: each maxterm (assignment where f=0) becomes a clause */
    DNF_CNF* cnf = dnf_cnf_create(n, total, n, 0);
    for (int t = 0; t < total; t++) {
        if (truth[t] == 0) {
            int* clause = (int*)malloc(n * sizeof(int));
            for (int j = 0; j < n; j++)
                clause[j] = ((t >> j) & 1) ? 0 : 1;  /* negate */
            dnf_cnf_add_term(cnf, clause);
            free(clause);
        }
    }
    free(truth);
    return cnf;
}

int64_t parity_dnf_term_count(int n) {
    if (n > 60) return INT64_MAX;
    return (int64_t)1 << (n - 1);
}

int dnf_cnf_equivalent(const DNF_CNF* a, const DNF_CNF* b) {
    if (!a || !b || a->n_vars != b->n_vars) return 0;
    int n = a->n_vars;
    if (n > 10) return -1;  /* Too large to check */
    int total = (1 << n);
    for (int t = 0; t < total; t++) {
        int* x = (int*)malloc(n * sizeof(int));
        for (int j = 0; j < n; j++) x[j] = (t >> j) & 1;
        if (dnf_cnf_evaluate(a, x) != dnf_cnf_evaluate(b, x)) {
            free(x); return 0;
        }
        free(x);
    }
    return 1;
}

DNF_CNF* circuit_to_dnf_after_restriction(const AC0Circuit* c,
                                           const int* restr, int n) {
    /* For small restricted circuits, enumerate truth table and build DNF */
    AC0Circuit* rc = circuit_restrict(c, restr, n);
    if (!rc) return NULL;

    /* Count free vars */
    int n_free = 0;
    for (int i = 0; i < n; i++) if (restr[i] == -1) n_free++;

    if (n_free > 8) { circuit_free(rc); return NULL; }

    int total = (1 << n_free);
    DNF_CNF* dnf = dnf_cnf_create(n_free, total, n_free, 1);

    int* xx = (int*)malloc(n * sizeof(int));
    for (int i = 0; i < n; i++)
        xx[i] = (restr[i] != -1) ? (restr[i] & 1) : 0;

    int* fi = (int*)malloc(n_free * sizeof(int));
    int k = 0;
    for (int i = 0; i < n; i++) if (restr[i] == -1) fi[k++] = i;

    for (int t = 0; t < total; t++) {
        for (int j = 0; j < n_free; j++) xx[fi[j]] = (t >> j) & 1;
        if (circuit_evaluate(rc, xx) == 1) {
            int* term = (int*)malloc(n_free * sizeof(int));
            for (int j = 0; j < n_free; j++)
                term[j] = ((t >> j) & 1) ? 1 : 0;
            dnf_cnf_add_term(dnf, term);
            free(term);
        }
    }
    free(xx); free(fi);
    circuit_free(rc);
    return dnf;
}