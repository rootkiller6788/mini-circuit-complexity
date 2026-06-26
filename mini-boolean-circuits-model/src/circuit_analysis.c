/* circuit_analysis.c -- Circuit Analysis, Verification, and Transformation
 *
 * Implements analysis tools for Boolean circuits: gate-level statistics,
 * equivalence/satisfiability checking, and structural transformations.
 *
 * Theorem (CIRCUIT-SAT is NP-complete): circuit_is_satisfiable decides
 * CIRCUIT-SAT via brute force for n_i <= 16.
 *
 * Theorem (functional completeness): {NAND} and {NOR} are functionally
 * complete bases. Any Boolean circuit can be transformed to NAND-only
 * or NOR-only with at most 3x size blowup and 2x depth blowup.
 *
 * Theorem (circuit balancing): Any circuit with associative gate chains
 * (AND, OR, XOR) can be balanced to O(log n) depth with O(n) size.
 *
 * References:
 *   AB (2009) Ch.6; Vollmer (1999) Ch.1,3; Jukna (2012) Ch.4;
 *   Sipser (2013) Ch.9; Stanford CS358.
 */

#include "circuit_analysis.h"
#include <assert.h>
#include <math.h>

/* ===================================================================
 * Gate-Level Statistics
 * =================================================================== */

void circuit_count_gates(const BooleanCircuit* c, int gates_by_type[11]) {
    assert(c != NULL); assert(gates_by_type != NULL);
    for (int t = 0; t < 11; t++) gates_by_type[t] = 0;
    for (int i = 0; i < c->n_gates; i++)
        gates_by_type[c->gates[i].type]++;
}

void circuit_fanin_stats(const BooleanCircuit* c,
                          int* min_fi, int* max_fi, double* avg_fi) {
    assert(c != NULL); assert(min_fi && max_fi && avg_fi);
    *min_fi = 0x7FFFFFFF; *max_fi = 0;
    double sum = 0.0; int count = 0;
    for (int i = 0; i < c->n_gates; i++) {
        const Gate* g = &c->gates[i];
        if (g->type == INPUT || g->type == CONST0 || g->type == CONST1)
            continue;
        int fi = 0;
        if (g->type == MAJ || g->type == THR) {
            fi = g->fanin;
        } else {
            if (g->input1 >= 0) fi++;
            if (g->input2 >= 0) fi++;
        }
        if (fi == 0) continue;
        count++;
        if (fi < *min_fi) *min_fi = fi;
        if (fi > *max_fi) *max_fi = fi;
        sum += (double)fi;
    }
    if (count == 0) { *min_fi = 0; *max_fi = 0; *avg_fi = 0.0; }
    else { *avg_fi = sum / (double)count; }
}

void circuit_fanout_stats(const BooleanCircuit* c,
                           int* min_fo, int* max_fo, double* avg_fo) {
    assert(c != NULL); assert(min_fo && max_fo && avg_fo);
    int n = c->n_gates;
    int* fanout = (int*)calloc((size_t)n, sizeof(int));
    assert(fanout);
    for (int i = 0; i < n; i++) {
        const Gate* g = &c->gates[i];
        if (g->type == MAJ || g->type == THR) {
            for (int j = 0; j < g->fanin; j++)
                fanout[g->fanin_ids[j]]++;
        } else {
            if (g->input1 >= 0) fanout[g->input1]++;
            if (g->input2 >= 0) fanout[g->input2]++;
        }
    }
    *min_fo = (n > 0) ? fanout[0] : 0; *max_fo = 0;
    double sum = 0.0;
    for (int i = 0; i < n; i++) {
        if (fanout[i] < *min_fo) *min_fo = fanout[i];
        if (fanout[i] > *max_fo) *max_fo = fanout[i];
        sum += (double)fanout[i];
    }
    *avg_fo = (n > 0) ? sum / (double)n : 0.0;
    free(fanout);
}

void circuit_type_distribution(const BooleanCircuit* c, double pct[11]) {
    assert(c != NULL); assert(pct != NULL);
    int cnt[11]; circuit_count_gates(c, cnt);
    double total = (double)c->n_gates;
    if (total == 0.0) {
        for (int i = 0; i < 11; i++) pct[i] = 0.0;
    } else {
        for (int i = 0; i < 11; i++)
            pct[i] = (double)cnt[i] / total * 100.0;
    }
}

int* circuit_level_widths(const BooleanCircuit* c, int* n_levels) {
    assert(c != NULL); assert(n_levels != NULL);
    int n = c->n_gates;
    int* level_of = (int*)malloc((size_t)n * sizeof(int));
    int* levels = (int*)calloc((size_t)(n + 1), sizeof(int));
    assert(level_of && levels);
    for (int i = 0; i < n; i++) level_of[i] = -1;
    for (int i = 0; i < n; i++) {
        const Gate* g = &c->gates[i];
        if (g->type == INPUT || g->type == CONST0 || g->type == CONST1)
            level_of[i] = 0;
    }
    int changed = 1;
    while (changed) {
        changed = 0;
        for (int i = 0; i < n; i++) {
            if (level_of[i] >= 0) continue;
            const Gate* g = &c->gates[i];
            int max_parent = -1, all_ready = 1;
            if (g->type == MAJ || g->type == THR) {
                for (int j = 0; j < g->fanin; j++) {
                    if (level_of[g->fanin_ids[j]] < 0) {all_ready=0; break;}
                    if (level_of[g->fanin_ids[j]] > max_parent)
                        max_parent = level_of[g->fanin_ids[j]];
                }
            } else {
                if (g->input1 >= 0) {
                    if (level_of[g->input1] < 0) all_ready = 0;
                    else if (level_of[g->input1] > max_parent)
                        max_parent = level_of[g->input1];
                }
                if (g->input2 >= 0) {
                    if (level_of[g->input2] < 0) all_ready = 0;
                    else if (level_of[g->input2] > max_parent)
                        max_parent = level_of[g->input2];
                }
            }
            if (all_ready && max_parent >= 0) {
                level_of[i] = max_parent + 1; changed = 1;
            }
        }
    }
    for (int i = 0; i < n; i++)
        if (level_of[i] >= 0) levels[level_of[i]]++;
    int max_lvl = 0;
    for (int i = 0; i <= n; i++)
        if (levels[i] > 0) max_lvl = i;
    int* result = (int*)malloc((size_t)(max_lvl + 1) * sizeof(int));
    if (result) {
        for (int i = 0; i <= max_lvl; i++) result[i] = levels[i];
        *n_levels = max_lvl + 1;
    } else { *n_levels = 0; }
    free(level_of); free(levels);
    return result;
}
/* ===================================================================
 * Circuit Verification via Brute Force
 * =================================================================== */

int circuits_equivalent(const BooleanCircuit* a, const BooleanCircuit* b,
                         int ni) {
    assert(a && b); assert(ni > 0 && ni <= 16);
    long long total = 1LL << ni;
    int* inputs = (int*)calloc((size_t)ni, sizeof(int));
    assert(inputs);
    for (long long t = 0; t < total; t++) {
        for (int j = 0; j < ni; j++) inputs[j] = (int)((t >> j) & 1);
        int va = circuit_evaluate((BooleanCircuit*)a, inputs);
        int vb = circuit_evaluate((BooleanCircuit*)b, inputs);
        if (va != vb) { free(inputs); return 0; }
    }
    free(inputs); return 1;
}

int circuit_is_tautology(const BooleanCircuit* c, int ni) {
    assert(c != NULL); assert(ni > 0 && ni <= 16);
    long long total = 1LL << ni;
    int* inputs = (int*)calloc((size_t)ni, sizeof(int));
    assert(inputs);
    for (long long t = 0; t < total; t++) {
        for (int j = 0; j < ni; j++) inputs[j] = (int)((t >> j) & 1);
        if (!circuit_evaluate((BooleanCircuit*)c, inputs)) {
            free(inputs); return 0;
        }
    }
    free(inputs); return 1;
}

int circuit_is_satisfiable(const BooleanCircuit* c, int ni) {
    assert(c != NULL); assert(ni > 0 && ni <= 16);
    long long total = 1LL << ni;
    int* inputs = (int*)calloc((size_t)ni, sizeof(int));
    assert(inputs);
    for (long long t = 0; t < total; t++) {
        for (int j = 0; j < ni; j++) inputs[j] = (int)((t >> j) & 1);
        if (circuit_evaluate((BooleanCircuit*)c, inputs)) {
            free(inputs); return 1;
        }
    }
    free(inputs); return 0;
}

long long circuit_count_sat_assignments(const BooleanCircuit* c, int ni) {
    assert(c != NULL); assert(ni > 0 && ni <= 16);
    long long total = 1LL << ni, count = 0;
    int* inputs = (int*)calloc((size_t)ni, sizeof(int));
    assert(inputs);
    for (long long t = 0; t < total; t++) {
        for (int j = 0; j < ni; j++) inputs[j] = (int)((t >> j) & 1);
        if (circuit_evaluate((BooleanCircuit*)c, inputs)) count++;
    }
    free(inputs); return count;
}

int circuit_verify_truth_table(const BooleanCircuit* c, int ni,
                                const int* truth) {
    assert(c && truth); assert(ni > 0 && ni <= 16);
    long long total = 1LL << ni;
    int* inputs = (int*)calloc((size_t)ni, sizeof(int));
    assert(inputs);
    for (long long t = 0; t < total; t++) {
        for (int j = 0; j < ni; j++) inputs[j] = (int)((t >> j) & 1);
        int out = circuit_evaluate((BooleanCircuit*)c, inputs);
        if (out != truth[t]) { free(inputs); return 0; }
    }
    free(inputs); return 1;
}

/* ===================================================================
 * Circuit Comparison
 * =================================================================== */

int circuit_compare_size(const BooleanCircuit* a, const BooleanCircuit* b) {
    assert(a && b);
    if (a->n_gates < b->n_gates) return -1;
    if (a->n_gates > b->n_gates) return 1;
    return 0;
}

int circuit_compare_depth(const BooleanCircuit* a, const BooleanCircuit* b) {
    assert(a && b);
    int da = circuit_depth(a), db = circuit_depth(b);
    if (da < db) return -1;
    if (da > db) return 1;
    return 0;
}

double circuit_size_ratio(const BooleanCircuit* a, const BooleanCircuit* b) {
    assert(a && b);
    if (b->n_gates == 0) return INFINITY;
    return (double)a->n_gates / (double)b->n_gates;
}

int circuit_compare_complexity(const BooleanCircuit* a,
                                const BooleanCircuit* b) {
    assert(a && b);
    double pa = (double)a->n_gates * (double)circuit_depth(a);
    double pb = (double)b->n_gates * (double)circuit_depth(b);
    if (pa < pb) return -1;
    if (pa > pb) return 1;
    return 0;
}

/* ===================================================================
 * A helper: synthesize truth table from circuit (brute force)
 * =================================================================== */

static int* circuit_extract_truth_table(const BooleanCircuit* c,
                                         int ni, int* n_rows) {
    long long total = 1LL << ni;
    int* truth = (int*)malloc((size_t)total * sizeof(int));
    if (!truth) { *n_rows = 0; return NULL; }
    int* inputs = (int*)calloc((size_t)ni, sizeof(int));
    assert(inputs);
    for (long long t = 0; t < total; t++) {
        for (int j = 0; j < ni; j++) inputs[j] = (int)((t >> j) & 1);
        truth[t] = circuit_evaluate((BooleanCircuit*)c, inputs);
    }
    free(inputs);
    *n_rows = (int)total;
    return truth;
}

/* ===================================================================
 * Circuit Transformations: DNF and CNF
 * =================================================================== */

BooleanCircuit* circuit_to_dnf(const BooleanCircuit* c) {
    assert(c != NULL);
    int ni = c->n_inputs;
    assert(ni > 0 && ni <= 12);  /* exponential blowup guard */
    int n_rows = 0;
    int* truth = circuit_extract_truth_table(c, ni, &n_rows);
    if (!truth) return NULL;

    /* Count satisfying assignments */
    int n_sat = 0;
    for (int t = 0; t < n_rows; t++)
        if (truth[t]) n_sat++;

    int cap = ni + n_sat * (ni * 2 + 2) + 10;
    BooleanCircuit* dnf = circuit_create(cap > 10 ? cap : 10);

    int in_ids[16];
    for (int j = 0; j < ni; j++)
        in_ids[j] = circuit_add_gate(dnf, INPUT, -1, -1);

    int* term_ids = (int*)malloc((size_t)(n_sat + 1) * sizeof(int));
    assert(term_ids);
    int n_terms = 0;

    for (int t = 0; t < n_rows; t++) {
        if (!truth[t]) continue;
        int* lit_ids = (int*)malloc((size_t)ni * sizeof(int));
        assert(lit_ids);
        int n_lits = 0;
        for (int j = 0; j < ni; j++) {
            int bit = (t >> j) & 1;
            if (bit == 1)
                lit_ids[n_lits++] = in_ids[j];
            else {
                int ng = circuit_add_gate(dnf, NOT, in_ids[j], -1);
                lit_ids[n_lits++] = ng;
            }
        }
        int and_tree = lit_ids[0];
        for (int j = 1; j < n_lits; j++)
            and_tree = circuit_add_gate(dnf, AND, and_tree, lit_ids[j]);
        term_ids[n_terms++] = and_tree;
        free(lit_ids);
    }

    if (n_terms == 0) {
        int c0 = circuit_add_gate(dnf, CONST0, -1, -1);
        circuit_set_output(dnf, &c0, 1);
    } else if (n_terms == 1) {
        circuit_set_output(dnf, &term_ids[0], 1);
    } else {
        int or_tree = term_ids[0];
        for (int i = 1; i < n_terms; i++)
            or_tree = circuit_add_gate(dnf, OR, or_tree, term_ids[i]);
        circuit_set_output(dnf, &or_tree, 1);
    }
    free(term_ids); free(truth);
    return dnf;
}

BooleanCircuit* circuit_to_cnf(const BooleanCircuit* c) {
    assert(c != NULL);
    int ni = c->n_inputs;
    assert(ni > 0 && ni <= 12);
    int n_rows = 0;
    int* truth = circuit_extract_truth_table(c, ni, &n_rows);
    if (!truth) return NULL;

    int n_unsat = 0;
    for (int t = 0; t < n_rows; t++)
        if (!truth[t]) n_unsat++;

    int cap = ni + n_unsat * (ni * 2 + 2) + 10;
    BooleanCircuit* cnf = circuit_create(cap > 10 ? cap : 10);

    int in_ids[16];
    for (int j = 0; j < ni; j++)
        in_ids[j] = circuit_add_gate(cnf, INPUT, -1, -1);

    int* clause_ids = (int*)malloc((size_t)(n_unsat + 1) * sizeof(int));
    assert(clause_ids);
    int n_clauses = 0;

    for (int t = 0; t < n_rows; t++) {
        if (truth[t]) continue;
        int* lit_ids = (int*)malloc((size_t)ni * sizeof(int));
        assert(lit_ids);
        int n_lits = 0;
        for (int j = 0; j < ni; j++) {
            int bit = (t >> j) & 1;
            if (bit == 0)
                lit_ids[n_lits++] = in_ids[j];
            else {
                int ng = circuit_add_gate(cnf, NOT, in_ids[j], -1);
                lit_ids[n_lits++] = ng;
            }
        }
        int or_tree = lit_ids[0];
        for (int j = 1; j < n_lits; j++)
            or_tree = circuit_add_gate(cnf, OR, or_tree, lit_ids[j]);
        clause_ids[n_clauses++] = or_tree;
        free(lit_ids);
    }

    if (n_clauses == 0) {
        int c1 = circuit_add_gate(cnf, CONST1, -1, -1);
        circuit_set_output(cnf, &c1, 1);
    } else if (n_clauses == 1) {
        circuit_set_output(cnf, &clause_ids[0], 1);
    } else {
        int and_tree = clause_ids[0];
        for (int i = 1; i < n_clauses; i++)
            and_tree = circuit_add_gate(cnf, AND, and_tree, clause_ids[i]);
        circuit_set_output(cnf, &and_tree, 1);
    }
    free(clause_ids); free(truth);
    return cnf;
}

/* ===================================================================
 * Circuit Transformations: Basis Conversion
 * =================================================================== */

BooleanCircuit* circuit_to_nand_only(const BooleanCircuit* c) {
    assert(c != NULL);
    int cap = c->n_gates * 5 + 10;
    BooleanCircuit* ncirc = circuit_create(cap);
    int* mapping = (int*)malloc((size_t)c->n_gates * sizeof(int));
    assert(mapping);
    for (int i = 0; i < c->n_gates; i++) mapping[i] = -1;
    int* order = (int*)malloc((size_t)c->n_gates * sizeof(int));
    assert(order);
    circuit_topological_order(c, order);

    for (int idx = 0; idx < c->n_gates; idx++) {
        int i = order[idx];
        const Gate* g = &c->gates[i];
        int a, b;
        switch (g->type) {
        case INPUT:
            mapping[i] = circuit_add_gate(ncirc, INPUT, -1, -1); break;
        case CONST0:
            mapping[i] = circuit_add_gate(ncirc, CONST0, -1, -1); break;
        case CONST1:
            mapping[i] = circuit_add_gate(ncirc, CONST1, -1, -1); break;
        case NOT:
            a = mapping[g->input1];
            mapping[i] = circuit_add_gate(ncirc, NAND, a, a); break;
        case AND:
            a = mapping[g->input1]; b = mapping[g->input2];
            { int ng = circuit_add_gate(ncirc, NAND, a, b);
              mapping[i] = circuit_add_gate(ncirc, NAND, ng, ng); }
            break;
        case OR:
            a = mapping[g->input1]; b = mapping[g->input2];
            { int na = circuit_add_gate(ncirc, NAND, a, a);
              int nb = circuit_add_gate(ncirc, NAND, b, b);
              mapping[i] = circuit_add_gate(ncirc, NAND, na, nb); }
            break;
        case XOR:
            a = mapping[g->input1]; b = mapping[g->input2];
            { int n1 = circuit_add_gate(ncirc, NAND, a, b);
              int n2 = circuit_add_gate(ncirc, NAND, a, n1);
              int n3 = circuit_add_gate(ncirc, NAND, b, n1);
              mapping[i] = circuit_add_gate(ncirc, NAND, n2, n3); }
            break;
        case NAND:
            a = mapping[g->input1]; b = mapping[g->input2];
            mapping[i] = circuit_add_gate(ncirc, NAND, a, b); break;
        case NOR:
            a = mapping[g->input1]; b = mapping[g->input2];
            { int na = circuit_add_gate(ncirc, NAND, a, a);
              int nb = circuit_add_gate(ncirc, NAND, b, b);
              int nand_ab = circuit_add_gate(ncirc, NAND, na, nb);
              mapping[i] = circuit_add_gate(ncirc, NAND, nand_ab, nand_ab); }
            break;
        case MAJ: case THR:
            if (g->fanin <= 2) {
                a = mapping[g->fanin_ids[0]];
                b = (g->fanin > 1) ? mapping[g->fanin_ids[1]] : a;
                mapping[i] = circuit_add_gate(ncirc, NAND, a, b);
            } else {
                int acc = mapping[g->fanin_ids[0]];
                for (int j = 1; j < g->fanin; j++) {
                    int nb = mapping[g->fanin_ids[j]];
                    int n1 = circuit_add_gate(ncirc, NAND, acc, nb);
                    acc = circuit_add_gate(ncirc, NAND, n1, n1);
                }
                mapping[i] = acc;
            }
            break;
        default:
            mapping[i] = circuit_add_gate(ncirc, CONST0, -1, -1); break;
        }
    }

    int* outs = (int*)malloc((size_t)c->n_outputs * sizeof(int));
    assert(outs);
    for (int i = 0; i < c->n_outputs; i++)
        outs[i] = mapping[c->output_ids[i]];
    circuit_set_output(ncirc, outs, c->n_outputs);
    ncirc->basis = 1;
    free(outs); free(order); free(mapping);
    return ncirc;
}

BooleanCircuit* circuit_to_nor_only(const BooleanCircuit* c) {
    assert(c != NULL);
    int cap = c->n_gates * 5 + 10;
    BooleanCircuit* ncirc = circuit_create(cap);
    int* mapping = (int*)malloc((size_t)c->n_gates * sizeof(int));
    assert(mapping);
    for (int i = 0; i < c->n_gates; i++) mapping[i] = -1;
    int* order = (int*)malloc((size_t)c->n_gates * sizeof(int));
    assert(order);
    circuit_topological_order(c, order);

    for (int idx = 0; idx < c->n_gates; idx++) {
        int i = order[idx];
        const Gate* g = &c->gates[i];
        int a, b;
        switch (g->type) {
        case INPUT:
            mapping[i] = circuit_add_gate(ncirc, INPUT, -1, -1); break;
        case CONST0:
            mapping[i] = circuit_add_gate(ncirc, CONST0, -1, -1); break;
        case CONST1:
            mapping[i] = circuit_add_gate(ncirc, CONST1, -1, -1); break;
        case NOT:
            a = mapping[g->input1];
            mapping[i] = circuit_add_gate(ncirc, NOR, a, a); break;
        case OR:
            a = mapping[g->input1]; b = mapping[g->input2];
            { int n1 = circuit_add_gate(ncirc, NOR, a, b);
              mapping[i] = circuit_add_gate(ncirc, NOR, n1, n1); }
            break;
        case AND:
            a = mapping[g->input1]; b = mapping[g->input2];
            { int na = circuit_add_gate(ncirc, NOR, a, a);
              int nb = circuit_add_gate(ncirc, NOR, b, b);
              mapping[i] = circuit_add_gate(ncirc, NOR, na, nb); }
            break;
        case NOR:
            a = mapping[g->input1]; b = mapping[g->input2];
            mapping[i] = circuit_add_gate(ncirc, NOR, a, b); break;
        case NAND:
            a = mapping[g->input1]; b = mapping[g->input2];
            { int na = circuit_add_gate(ncirc, NOR, a, a);
              int nb = circuit_add_gate(ncirc, NOR, b, b);
              int nand_ab = circuit_add_gate(ncirc, NOR, na, nb);
              mapping[i] = circuit_add_gate(ncirc, NOR, nand_ab, nand_ab); }
            break;
        case XOR:
            a = mapping[g->input1]; b = mapping[g->input2];
            { int n1 = circuit_add_gate(ncirc, NOR, a, b);
              int n2 = circuit_add_gate(ncirc, NOR, a, n1);
              int n3 = circuit_add_gate(ncirc, NOR, b, n1);
              mapping[i] = circuit_add_gate(ncirc, NOR, n2, n3); }
            break;
        default:
            mapping[i] = circuit_add_gate(ncirc, CONST0, -1, -1); break;
        }
    }

    int* outs = (int*)malloc((size_t)c->n_outputs * sizeof(int));
    assert(outs);
    for (int i = 0; i < c->n_outputs; i++)
        outs[i] = mapping[c->output_ids[i]];
    circuit_set_output(ncirc, outs, c->n_outputs);
    ncirc->basis = 2;
    free(outs); free(order); free(mapping);
    return ncirc;
}

BooleanCircuit* circuit_to_fanin2(const BooleanCircuit* c) {
    assert(c != NULL);
    int cap = c->n_gates * 3 + 10;
    BooleanCircuit* f2 = circuit_create(cap);
    int* mapping = (int*)malloc((size_t)c->n_gates * sizeof(int));
    assert(mapping);
    for (int i = 0; i < c->n_gates; i++) mapping[i] = -1;
    int* order = (int*)malloc((size_t)c->n_gates * sizeof(int));
    assert(order);
    circuit_topological_order(c, order);

    for (int idx = 0; idx < c->n_gates; idx++) {
        int i = order[idx];
        const Gate* g = &c->gates[i];
        if (g->type == MAJ || g->type == THR) {
            if (g->fanin <= 2) {
                int a = mapping[g->fanin_ids[0]];
                int b = (g->fanin > 1) ? mapping[g->fanin_ids[1]] : a;
                mapping[i] = circuit_add_gate(f2, AND, a, b);
            } else {
                int acc = mapping[g->fanin_ids[0]];
                for (int j = 1; j < g->fanin; j++)
                    acc = circuit_add_gate(f2, AND, acc,
                                           mapping[g->fanin_ids[j]]);
                mapping[i] = acc;
            }
        } else {
            int a = (g->input1 >= 0) ? mapping[g->input1] : -1;
            int b = (g->input2 >= 0) ? mapping[g->input2] : -1;
            mapping[i] = circuit_add_gate(f2, g->type, a, b);
        }
    }

    int* outs = (int*)malloc((size_t)c->n_outputs * sizeof(int));
    assert(outs);
    for (int i = 0; i < c->n_outputs; i++)
        outs[i] = mapping[c->output_ids[i]];
    circuit_set_output(f2, outs, c->n_outputs);
    free(outs); free(order); free(mapping);
    return f2;
}

/* ===================================================================
 * Circuit Transformations: Redundancy, Composition, Balancing
 * =================================================================== */

BooleanCircuit* circuit_remove_redundant(const BooleanCircuit* c) {
    assert(c != NULL);
    int n = c->n_gates;
    int* reachable = (int*)calloc((size_t)n, sizeof(int));
    int* queue = (int*)malloc((size_t)n * sizeof(int));
    assert(reachable && queue);
    int qh = 0, qt = 0;

    for (int i = 0; i < c->n_outputs; i++) {
        int oid = c->output_ids[i];
        if (!reachable[oid]) { queue[qt++] = oid; reachable[oid] = 1; }
    }
    while (qh < qt) {
        int gid = queue[qh++];
        const Gate* g = &c->gates[gid];
        if (g->type == MAJ || g->type == THR) {
            for (int j = 0; j < g->fanin; j++) {
                int pid = g->fanin_ids[j];
                if (!reachable[pid]) { queue[qt++] = pid; reachable[pid] = 1; }
            }
        } else {
            if (g->input1 >= 0 && !reachable[g->input1])
                { queue[qt++] = g->input1; reachable[g->input1] = 1; }
            if (g->input2 >= 0 && !reachable[g->input2])
                { queue[qt++] = g->input2; reachable[g->input2] = 1; }
        }
    }

    BooleanCircuit* clean = circuit_create(n + 1);
    int* new_id = (int*)malloc((size_t)n * sizeof(int));
    assert(new_id);
    for (int i = 0; i < n; i++) new_id[i] = -1;
    int* order = (int*)malloc((size_t)n * sizeof(int));
    assert(order);
    circuit_topological_order(c, order);

    for (int idx = 0; idx < n; idx++) {
        int i = order[idx];
        if (!reachable[i]) continue;
        const Gate* g = &c->gates[i];
        if (g->type == MAJ || g->type == THR) {
            int* fids = (int*)malloc((size_t)g->fanin * sizeof(int));
            assert(fids);
            for (int j = 0; j < g->fanin; j++)
                fids[j] = new_id[g->fanin_ids[j]];
            new_id[i] = circuit_add_maj_gate(clean, g->type, g->fanin, fids);
            free(fids);
        } else {
            int a = (g->input1 >= 0) ? new_id[g->input1] : -1;
            int b = (g->input2 >= 0) ? new_id[g->input2] : -1;
            new_id[i] = circuit_add_gate(clean, g->type, a, b);
        }
    }

    int* outs = (int*)malloc((size_t)c->n_outputs * sizeof(int));
    assert(outs);
    for (int i = 0; i < c->n_outputs; i++)
        outs[i] = new_id[c->output_ids[i]];
    circuit_set_output(clean, outs, c->n_outputs);
    free(outs); free(order); free(new_id); free(queue); free(reachable);
    return clean;
}

BooleanCircuit* circuit_compose(const BooleanCircuit* c1,
                                const BooleanCircuit* c2) {
    assert(c1 && c2);
    int needed = (c2->n_inputs < c1->n_outputs) ? c2->n_inputs : c1->n_outputs;
    int cap = c1->n_gates + c2->n_gates + 10;
    BooleanCircuit* composed = circuit_create(cap);

    /* Copy c1 */
    int* map1 = (int*)malloc((size_t)c1->n_gates * sizeof(int));
    assert(map1);
    int* order1 = (int*)malloc((size_t)c1->n_gates * sizeof(int));
    assert(order1);
    circuit_topological_order(c1, order1);
    for (int idx = 0; idx < c1->n_gates; idx++) {
        int i = order1[idx];
        const Gate* g = &c1->gates[i];
        if (g->type == MAJ || g->type == THR) {
            int* fids = (int*)malloc((size_t)g->fanin * sizeof(int));
            assert(fids);
            for (int j = 0; j < g->fanin; j++) fids[j] = map1[g->fanin_ids[j]];
            map1[i] = circuit_add_maj_gate(composed, g->type, g->fanin, fids);
            free(fids);
        } else {
            int a = (g->input1 >= 0) ? map1[g->input1] : -1;
            int b = (g->input2 >= 0) ? map1[g->input2] : -1;
            map1[i] = circuit_add_gate(composed, g->type, a, b);
        }
    }

    /* Copy c2, wiring its INPUTs to c1's outputs */
    int* map2 = (int*)malloc((size_t)c2->n_gates * sizeof(int));
    assert(map2);
    for (int i = 0; i < c2->n_gates; i++) map2[i] = -1;
    int* order2 = (int*)malloc((size_t)c2->n_gates * sizeof(int));
    assert(order2);
    circuit_topological_order(c2, order2);
    int c2_in_ctr = 0;
    for (int idx = 0; idx < c2->n_gates; idx++) {
        int i = order2[idx];
        const Gate* g = &c2->gates[i];
        if (g->type == INPUT) {
            if (c2_in_ctr < needed)
                map2[i] = map1[c1->output_ids[c2_in_ctr++]];
            else
                map2[i] = circuit_add_gate(composed, CONST0, -1, -1);
        } else if (g->type == MAJ || g->type == THR) {
            int* fids = (int*)malloc((size_t)g->fanin * sizeof(int));
            assert(fids);
            for (int j = 0; j < g->fanin; j++) fids[j] = map2[g->fanin_ids[j]];
            map2[i] = circuit_add_maj_gate(composed, g->type, g->fanin, fids);
            free(fids);
        } else {
            int a = (g->input1 >= 0) ? map2[g->input1] : -1;
            int b = (g->input2 >= 0) ? map2[g->input2] : -1;
            map2[i] = circuit_add_gate(composed, g->type, a, b);
        }
    }

    int* outs = (int*)malloc((size_t)c2->n_outputs * sizeof(int));
    assert(outs);
    for (int i = 0; i < c2->n_outputs; i++)
        outs[i] = map2[c2->output_ids[i]];
    circuit_set_output(composed, outs, c2->n_outputs);
    free(outs); free(order2); free(map2); free(order1); free(map1);
    return composed;
}

BooleanCircuit* circuit_balance(const BooleanCircuit* c) {
    assert(c != NULL);
    int cap = c->n_gates * 3 + 10;
    BooleanCircuit* bc = circuit_create(cap);
    int* mapping = (int*)malloc((size_t)c->n_gates * sizeof(int));
    assert(mapping);
    for (int i = 0; i < c->n_gates; i++) mapping[i] = -1;
    int* order = (int*)malloc((size_t)c->n_gates * sizeof(int));
    assert(order);
    circuit_topological_order(c, order);

    for (int idx = 0; idx < c->n_gates; idx++) {
        int i = order[idx];
        const Gate* g = &c->gates[i];

        if ((g->type == AND || g->type == OR || g->type == XOR) &&
            g->input1 >= 0 && g->input2 >= 0) {
            /* Collect leaf operands of same-type chain */
            int* collect = (int*)malloc((size_t)c->n_gates * sizeof(int));
            assert(collect);
            int nc = 0;
            int stack[1024]; int sp = 0;
            if (g->input1 >= 0) stack[sp++] = g->input1;
            if (g->input2 >= 0) stack[sp++] = g->input2;
            while (sp > 0 && nc < c->n_gates) {
                int cid = stack[--sp];
                const Gate* cg = &c->gates[cid];
                if (cg->type == g->type && cg->input1 >= 0 &&
                    cg->input2 >= 0) {
                    stack[sp++] = cg->input1;
                    stack[sp++] = cg->input2;
                } else {
                    collect[nc++] = mapping[cid];
                }
            }
            if (nc <= 1) {
                mapping[i] = (nc == 1) ? collect[0] :
                    circuit_add_gate(bc, CONST0, -1, -1);
            } else {
                /* Build balanced tree */
                int* cur = collect; int cur_n = nc;
                while (cur_n > 1) {
                    int next_n = (cur_n + 1) / 2;
                    int* next_lvl = (int*)malloc((size_t)next_n * sizeof(int));
                    assert(next_lvl);
                    for (int k = 0; k < cur_n / 2; k++)
                        next_lvl[k] = circuit_add_gate(bc, g->type,
                            cur[2*k], cur[2*k+1]);
                    if (cur_n % 2 == 1) next_lvl[next_n-1] = cur[cur_n-1];
                    if (cur != collect) free(cur);
                    cur = next_lvl; cur_n = next_n;
                }
                mapping[i] = cur[0];
                if (cur != collect) free(cur);
            }
            free(collect);
        } else if (g->type == MAJ || g->type == THR) {
            int* fids = (int*)malloc((size_t)g->fanin * sizeof(int));
            assert(fids);
            for (int j = 0; j < g->fanin; j++)
                fids[j] = mapping[g->fanin_ids[j]];
            mapping[i] = circuit_add_maj_gate(bc, g->type, g->fanin, fids);
            free(fids);
        } else {
            int a = (g->input1 >= 0) ? mapping[g->input1] : -1;
            int b = (g->input2 >= 0) ? mapping[g->input2] : -1;
            mapping[i] = circuit_add_gate(bc, g->type, a, b);
        }
    }

    int* outs = (int*)malloc((size_t)c->n_outputs * sizeof(int));
    assert(outs);
    for (int i = 0; i < c->n_outputs; i++)
        outs[i] = mapping[c->output_ids[i]];
    circuit_set_output(bc, outs, c->n_outputs);
    free(outs); free(order); free(mapping);
    return bc;
}

BooleanCircuit* circuit_negate(const BooleanCircuit* c) {
    assert(c != NULL);
    BooleanCircuit* nc = circuit_create(c->n_gates + 10);
    int* mapping = (int*)malloc((size_t)c->n_gates * sizeof(int));
    assert(mapping);
    int* order = (int*)malloc((size_t)c->n_gates * sizeof(int));
    assert(order);
    circuit_topological_order(c, order);

    for (int idx = 0; idx < c->n_gates; idx++) {
        int i = order[idx];
        const Gate* g = &c->gates[i];
        if (g->type == MAJ || g->type == THR) {
            int* fids = (int*)malloc((size_t)g->fanin * sizeof(int));
            assert(fids);
            for (int j = 0; j < g->fanin; j++)
                fids[j] = mapping[g->fanin_ids[j]];
            mapping[i] = circuit_add_maj_gate(nc, g->type, g->fanin, fids);
            free(fids);
        } else {
            int a = (g->input1 >= 0) ? mapping[g->input1] : -1;
            int b = (g->input2 >= 0) ? mapping[g->input2] : -1;
            mapping[i] = circuit_add_gate(nc, g->type, a, b);
        }
    }

    int not_out = circuit_add_gate(nc, NOT, mapping[c->output_ids[0]], -1);
    int outs[16];
    int nout = (c->n_outputs < 16) ? c->n_outputs : 16;
    outs[0] = not_out;
    for (int i = 1; i < nout; i++)
        outs[i] = mapping[c->output_ids[i]];
    circuit_set_output(nc, outs, nout);
    free(order); free(mapping);
    return nc;
}

BooleanCircuit* circuit_replicate(const BooleanCircuit* c, int n) {
    assert(c != NULL); assert(n > 0);
    int cap = c->n_gates * n + n * 10;
    BooleanCircuit* rc = circuit_create(cap);
    int* all_outs = (int*)malloc((size_t)(c->n_outputs * n) * sizeof(int));
    assert(all_outs);
    int out_pos = 0;

    for (int rep = 0; rep < n; rep++) {
        int* mapping = (int*)malloc((size_t)c->n_gates * sizeof(int));
        assert(mapping);
        for (int i = 0; i < c->n_gates; i++) mapping[i] = -1;
        int* order = (int*)malloc((size_t)c->n_gates * sizeof(int));
        assert(order);
        circuit_topological_order(c, order);

        for (int idx = 0; idx < c->n_gates; idx++) {
            int i = order[idx];
            const Gate* g = &c->gates[i];
            if (g->type == MAJ || g->type == THR) {
                int* fids = (int*)malloc((size_t)g->fanin * sizeof(int));
                assert(fids);
                for (int j = 0; j < g->fanin; j++)
                    fids[j] = mapping[g->fanin_ids[j]];
                mapping[i] = circuit_add_maj_gate(rc, g->type, g->fanin, fids);
                free(fids);
            } else {
                int a = (g->input1 >= 0) ? mapping[g->input1] : -1;
                int b = (g->input2 >= 0) ? mapping[g->input2] : -1;
                mapping[i] = circuit_add_gate(rc, g->type, a, b);
            }
        }
        for (int i = 0; i < c->n_outputs; i++)
            all_outs[out_pos++] = mapping[c->output_ids[i]];
        free(order); free(mapping);
    }

    circuit_set_output(rc, all_outs, out_pos);
    free(all_outs);
    return rc;
}
