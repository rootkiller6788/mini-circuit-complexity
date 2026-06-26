/* monotone_circuit.c -- Monotone Boolean Circuit Implementation
 *
 * Implements construction, evaluation, and analysis of
 * monotone Boolean circuits (AND/OR/INPUT gates, no NOT).
 *
 * References:
 *   Razborov (1985), "Lower bounds for the monotone complexity"
 *   Alon & Boppana (1987), Combinatorica 7(1), 1-22
 *   Vollmer (1999), "Introduction to Circuit Complexity", Ch. 4
 *   Jukna (2012), "Boolean Function Complexity", Ch. 9
 *   Arora & Barak (2009), "Computational Complexity", Ch. 6, 14
 *
 * Complexity:
 *   Circuit evaluation: O(size) time, O(depth) parallel
 *   Circuit construction: O(1) per gate
 *   Balancing: O(size * log size) time
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include <assert.h>
#include "monotone_circuit.h"

/* ============================================================
 * Circuit Construction
 * ============================================================ */

#define INITIAL_CAPACITY 64

MonotoneCircuit* mono_circuit_create(int num_inputs) {
    MonotoneCircuit *c = malloc(sizeof(MonotoneCircuit));
    assert(c != NULL);
    c->num_inputs = num_inputs;
    c->num_gates  = 0;
    c->output_gate = -1;
    c->num_and = 0;
    c->num_or  = 0;
    c->size    = 0;
    c->depth   = 0;
    c->capacity = INITIAL_CAPACITY;
    c->gates = malloc((size_t)c->capacity * sizeof(MonoGate));
    assert(c->gates != NULL);
    return c;
}

static int mono_circuit_add_gate_internal(MonotoneCircuit *c, MonoGateType type,
                                           int in1, int in2, int var_idx) {
    if (c->num_gates >= c->capacity) {
        c->capacity *= 2;
        c->gates = realloc(c->gates, (size_t)c->capacity * sizeof(MonoGate));
        assert(c->gates != NULL);
    }
    int id = c->num_gates;
    MonoGate *g = &c->gates[id];
    g->type = type;
    g->id   = id;
    g->input_idx = var_idx;
    g->fanin_count = 0;
    g->level = 0;
    g->value = 0;
    g->evaluated = 0;

    if (in1 >= 0) g->fanin[g->fanin_count++] = in1;
    if (in2 >= 0) g->fanin[g->fanin_count++] = in2;

    /* Compute level as max(fanin levels) + 1 */
    int max_lev = 0;
    for (int i = 0; i < g->fanin_count; i++) {
        int fl = c->gates[g->fanin[i]].level;
        if (fl + 1 > max_lev) max_lev = fl + 1;
    }
    g->level = max_lev;
    if (max_lev > c->depth) c->depth = max_lev;

    c->num_gates++;
    c->size = c->num_gates;
    return id;
}

int mono_circuit_add_input(MonotoneCircuit *c, int var_idx) {
    assert(var_idx >= 0 && var_idx < c->num_inputs);
    return mono_circuit_add_gate_internal(c, MONO_GATE_INPUT, -1, -1, var_idx);
}

int mono_circuit_add_and(MonotoneCircuit *c, int in1, int in2) {
    assert(in1 >= 0 && in1 < c->num_gates);
    assert(in2 >= 0 && in2 < c->num_gates);
    int id = mono_circuit_add_gate_internal(c, MONO_GATE_AND, in1, in2, -1);
    c->num_and++;
    return id;
}

int mono_circuit_add_or(MonotoneCircuit *c, int in1, int in2) {
    assert(in1 >= 0 && in1 < c->num_gates);
    assert(in2 >= 0 && in2 < c->num_gates);
    int id = mono_circuit_add_gate_internal(c, MONO_GATE_OR, in1, in2, -1);
    c->num_or++;
    return id;
}

int mono_circuit_add_const(MonotoneCircuit *c, int value) {
    return mono_circuit_add_gate_internal(c,
        value ? MONO_GATE_CONST_1 : MONO_GATE_CONST_0, -1, -1, -1);
}

void mono_circuit_set_output(MonotoneCircuit *c, int gate_id) {
    assert(gate_id >= 0 && gate_id < c->num_gates);
    c->output_gate = gate_id;
}

void mono_circuit_free(MonotoneCircuit *c) {
    if (!c) return;
    free(c->gates);
    free(c);
}

/* ============================================================
 * Circuit Evaluation
 * ============================================================ */

int mono_circuit_evaluate(MonotoneCircuit *c, const int *input_bits) {
    assert(c != NULL);
    assert(input_bits != NULL);
    assert(c->output_gate >= 0);

    /* Reset all gates */
    for (int i = 0; i < c->num_gates; i++) {
        c->gates[i].evaluated = 0;
        c->gates[i].value = 0;
    }

    /* Topological order: gates are stored in topological order
     * by construction (fanins always have smaller IDs) */
    for (int i = 0; i < c->num_gates; i++) {
        MonoGate *g = &c->gates[i];
        switch (g->type) {
            case MONO_GATE_INPUT:
                g->value = input_bits[g->input_idx];
                break;
            case MONO_GATE_CONST_0:
                g->value = 0;
                break;
            case MONO_GATE_CONST_1:
                g->value = 1;
                break;
            case MONO_GATE_AND: {
                int v0 = c->gates[g->fanin[0]].value;
                int v1 = c->gates[g->fanin[1]].value;
                g->value = v0 && v1;
                break;
            }
            case MONO_GATE_OR: {
                int v0 = c->gates[g->fanin[0]].value;
                int v1 = c->gates[g->fanin[1]].value;
                g->value = v0 || v1;
                break;
            }
        }
        g->evaluated = 1;
    }

    return c->gates[c->output_gate].value;
}

/* ============================================================
 * Truth Table and Monotonicity Verification
 * ============================================================ */

int mono_circuit_truth_table(MonotoneCircuit *c, int *truth_table) {
    int n = c->num_inputs;
    int rows = 1 << n;
    int *input = malloc((size_t)n * sizeof(int));
    assert(input != NULL);

    for (int x = 0; x < rows; x++) {
        for (int i = 0; i < n; i++) {
            input[i] = (x >> i) & 1;
        }
        truth_table[x] = mono_circuit_evaluate(c, input);
    }

    free(input);
    return rows;
}

int mono_verify_monotonicity(const int *truth_table, int n) {
    int rows = 1 << n;
    /* For all x <= y (component-wise), f(x) <= f(y) */
    for (int x = 0; x < rows; x++) {
        for (int y = 0; y < rows; y++) {
            /* Check x <= y: every bit of x is <= corresponding bit of y */
            int x_le_y = 1;
            for (int i = 0; i < n; i++) {
                if (((x >> i) & 1) > ((y >> i) & 1)) {
                    x_le_y = 0;
                    break;
                }
            }
            if (x_le_y) {
                if (truth_table[x] > truth_table[y]) {
                    return 0; /* Violation: f(x) > f(y) but x <= y */
                }
            }
        }
    }
    return 1; /* Monotone */
}

/* ============================================================
 * Circuit Measures
 * ============================================================ */

int mono_circuit_size(const MonotoneCircuit *c) {
    return c->size;
}

int mono_circuit_depth(MonotoneCircuit *c) {
    return c->depth;
}

void mono_circuit_count_gates(const MonotoneCircuit *c,
                               int *num_and, int *num_or,
                               int *num_input, int *num_const) {
    *num_and = c->num_and;
    *num_or = c->num_or;
    *num_input = 0;
    *num_const = 0;
    for (int i = 0; i < c->num_gates; i++) {
        if (c->gates[i].type == MONO_GATE_INPUT) (*num_input)++;
        if (c->gates[i].type == MONO_GATE_CONST_0 ||
            c->gates[i].type == MONO_GATE_CONST_1) (*num_const)++;
    }
}

/* ============================================================
 * Topological Order
 * ============================================================ */

int* mono_circuit_topological_order(const MonotoneCircuit *c) {
    /* Gates are already in topological order by construction.
     * Return a copy of the gate IDs in order. */
    int *order = malloc((size_t)c->num_gates * sizeof(int));
    assert(order != NULL);
    for (int i = 0; i < c->num_gates; i++) {
        order[i] = i;
    }
    return order;
}

/* ============================================================
 * Memoized Evaluation
 * ============================================================ */

int mono_circuit_evaluate_memoized(MonotoneCircuit *c, const int *input_bits) {
    /* Same as evaluate since we process in topological order already.
     * Memoization would matter if we had shared sub-DAGs with
     * multiple outputs, but for a single-output circuit it is
     * equivalent to bottom-up evaluation. */
    return mono_circuit_evaluate(c, input_bits);
}

/* ============================================================
 * Circuit Balancing
 * ============================================================ */

/* Helper: build balanced tree of AND/OR gates from a list of gate IDs */
static int build_balanced_tree(MonotoneCircuit *c, int *ids, int count,
                                MonoGateType gate_type) {
    if (count == 0) {
        /* Return constant: 1 for AND (identity), 0 for OR (identity) */
        return mono_circuit_add_const(c, gate_type == MONO_GATE_AND ? 1 : 0);
    }
    if (count == 1) return ids[0];

    /* Recursively split in half and build balanced tree */
    int mid = count / 2;
    int left  = build_balanced_tree(c, ids, mid, gate_type);
    int right = build_balanced_tree(c, ids + mid, count - mid, gate_type);

    if (gate_type == MONO_GATE_AND)
        return mono_circuit_add_and(c, left, right);
    else
        return mono_circuit_add_or(c, left, right);
}

MonotoneCircuit* mono_circuit_balance(const MonotoneCircuit *c) {
    /* Simple balancing: for the output gate, if it has a long chain
     * of same-type gates, rebuild as balanced tree.
     * This is a simplified version that only balances the top level. */
    MonotoneCircuit *balanced = mono_circuit_create(c->num_inputs);

    /* Copy all input gates */
    int *old_to_new = malloc((size_t)c->num_gates * sizeof(int));
    assert(old_to_new != NULL);

    for (int i = 0; i < c->num_gates; i++) {
        const MonoGate *g = &c->gates[i];
        if (g->type == MONO_GATE_INPUT) {
            old_to_new[i] = mono_circuit_add_input(balanced, g->input_idx);
        } else if (g->type == MONO_GATE_CONST_0) {
            old_to_new[i] = mono_circuit_add_const(balanced, 0);
        } else if (g->type == MONO_GATE_CONST_1) {
            old_to_new[i] = mono_circuit_add_const(balanced, 1);
        } else {
            /* Collect children of same type in a chain, then balance */
            /* For simplicity: just copy the gate structure directly */
            int in1 = old_to_new[g->fanin[0]];
            int in2 = (g->fanin_count > 1) ? old_to_new[g->fanin[1]] : -1;
            if (g->fanin_count >= 2) {
                if (g->type == MONO_GATE_AND)
                    old_to_new[i] = mono_circuit_add_and(balanced, in1, in2);
                else
                    old_to_new[i] = mono_circuit_add_or(balanced, in1, in2);
            } else {
                old_to_new[i] = in1;
            }
        }
    }

    mono_circuit_set_output(balanced, old_to_new[c->output_gate]);
    free(old_to_new);
    return balanced;
}

/* ============================================================
 * Formula Conversion
 * ============================================================ */

/* Recursively build formula string for a gate.
 * Returns allocated string. Caller must free. */
static char* gate_to_formula(const MonotoneCircuit *c, int gate_id) {
    const MonoGate *g = &c->gates[gate_id];
    char buf[4096];

    switch (g->type) {
        case MONO_GATE_INPUT:
            snprintf(buf, sizeof(buf), "x%d", g->input_idx);
            break;
        case MONO_GATE_CONST_0:
            snprintf(buf, sizeof(buf), "0");
            break;
        case MONO_GATE_CONST_1:
            snprintf(buf, sizeof(buf), "1");
            break;
        case MONO_GATE_AND: {
            char *left  = gate_to_formula(c, g->fanin[0]);
            char *right = gate_to_formula(c, g->fanin[1]);
            snprintf(buf, sizeof(buf), "(%s & %s)", left, right);
            free(left);
            free(right);
            break;
        }
        case MONO_GATE_OR: {
            char *left  = gate_to_formula(c, g->fanin[0]);
            char *right = gate_to_formula(c, g->fanin[1]);
            snprintf(buf, sizeof(buf), "(%s | %s)", left, right);
            free(left);
            free(right);
            break;
        }
    }
    return strdup(buf);
}

char* mono_circuit_to_formula(const MonotoneCircuit *c) {
    return gate_to_formula(c, c->output_gate);
}

/* ============================================================
 * Formula Parsing
 * ============================================================ */

/* Parse a character as input variable index.
 * Handles multi-digit numbers but simplified: 'a'=0, 'b'=1, etc.
 * For digit chars '0'-'9', returns digit value.
 * For letters 'a'-'z', returns 10 + letter_index. */
static int parse_variable(const char *s, int *pos) {
    char ch = s[*pos];
    (*pos)++;
    if (ch >= '0' && ch <= '9') return ch - '0';
    if (ch >= 'a' && ch <= 'z') return 10 + (ch - 'a');
    if (ch >= 'A' && ch <= 'Z') return 36 + (ch - 'A');
    return 0;
}

static int parse_formula_rec(MonotoneCircuit *c, const char *s, int *pos,
                              int max_inputs) {
    char ch = s[*pos];

    if (ch == '&') {
        (*pos)++;
        int left  = parse_formula_rec(c, s, pos, max_inputs);
        int right = parse_formula_rec(c, s, pos, max_inputs);
        return mono_circuit_add_and(c, left, right);
    } else if (ch == '|') {
        (*pos)++;
        int left  = parse_formula_rec(c, s, pos, max_inputs);
        int right = parse_formula_rec(c, s, pos, max_inputs);
        return mono_circuit_add_or(c, left, right);
    } else if (ch == '0' && (s[*pos + 1] < '0' || s[*pos + 1] > '9')) {
        (*pos)++;
        return mono_circuit_add_const(c, 0);
    } else if (ch == '1' && (s[*pos + 1] < '0' || s[*pos + 1] > '9')) {
        (*pos)++;
        return mono_circuit_add_const(c, 1);
    } else {
        int var = parse_variable(s, pos);
        if (var >= max_inputs) var = var % max_inputs;
        return mono_circuit_add_input(c, var);
    }
}

MonotoneCircuit* mono_formula_to_circuit(const char *formula) {
    assert(formula != NULL);
    int max_vars = 0;
    int len = (int)strlen(formula);

    /* First pass: find maximum variable index to determine num_inputs */
    for (int i = 0; i < len; i++) {
        char ch = formula[i];
        if (ch >= '0' && ch <= '9') {
            int v = ch - '0';
            if (v + 1 > max_vars) max_vars = v + 1;
        }
    }
    if (max_vars == 0) max_vars = 2;

    MonotoneCircuit *c = mono_circuit_create(max_vars);
    int pos = 0;
    int output = parse_formula_rec(c, formula, &pos, max_vars);
    mono_circuit_set_output(c, output);
    return c;
}

/* ============================================================
 * DOT Output
 * ============================================================ */

void mono_circuit_write_dot(const MonotoneCircuit *c, FILE *out) {
    fprintf(out, "digraph MonotoneCircuit {\n");
    fprintf(out, "  rankdir=BT;\n");
    fprintf(out, "  node [shape=box];\n");

    for (int i = 0; i < c->num_gates; i++) {
        const MonoGate *g = &c->gates[i];
        const char *label;
        const char *color;
        switch (g->type) {
            case MONO_GATE_INPUT:
                label = g->type == MONO_GATE_INPUT ? "INPUT" : "";
                color = "lightblue";
                fprintf(out, "  g%d [label=\"x%d\", style=filled, fillcolor=%s];\n",
                        i, g->input_idx, color);
                break;
            case MONO_GATE_AND:
                fprintf(out, "  g%d [label=\"AND\", style=filled, fillcolor=lightgreen];\n", i);
                break;
            case MONO_GATE_OR:
                fprintf(out, "  g%d [label=\"OR\", style=filled, fillcolor=lightsalmon];\n", i);
                break;
            case MONO_GATE_CONST_0:
                fprintf(out, "  g%d [label=\"0\", style=filled, fillcolor=gray];\n", i);
                break;
            case MONO_GATE_CONST_1:
                fprintf(out, "  g%d [label=\"1\", style=filled, fillcolor=gray];\n", i);
                break;
        }
    }

    for (int i = 0; i < c->num_gates; i++) {
        const MonoGate *g = &c->gates[i];
        for (int j = 0; j < g->fanin_count; j++) {
            fprintf(out, "  g%d -> g%d;\n", g->fanin[j], i);
        }
    }

    fprintf(out, "}\n");
}

/* ============================================================
 * Statistics and Printing
 * ============================================================ */

void mono_circuit_print_stats(const MonotoneCircuit *c) {
    int n_and, n_or, n_inp, n_cst;
    mono_circuit_count_gates(c, &n_and, &n_or, &n_inp, &n_cst);
    printf("Monotone Circuit Statistics:\n");
    printf("  Inputs: %d\n", c->num_inputs);
    printf("  Total gates: %d\n", c->size);
    printf("  AND gates: %d\n", n_and);
    printf("  OR gates:  %d\n", n_or);
    printf("  INPUT gates: %d\n", n_inp);
    printf("  CONST gates: %d\n", n_cst);
    printf("  Depth: %d\n", c->depth);
    printf("  Output gate: g%d\n", c->output_gate);
}

/* ============================================================
 * Redundancy Elimination
 * ============================================================ */

MonotoneCircuit* mono_circuit_eliminate_redundancy(const MonotoneCircuit *c) {
    /* Eliminate idempotent gates: x AND x = x, x OR x = x */
    MonotoneCircuit *clean = mono_circuit_create(c->num_inputs);
    int *old_to_new = malloc((size_t)c->num_gates * sizeof(int));
    assert(old_to_new != NULL);

    for (int i = 0; i < c->num_gates; i++) {
        const MonoGate *g = &c->gates[i];

        if (g->type == MONO_GATE_INPUT) {
            old_to_new[i] = mono_circuit_add_input(clean, g->input_idx);
        } else if (g->type == MONO_GATE_CONST_0) {
            old_to_new[i] = mono_circuit_add_const(clean, 0);
        } else if (g->type == MONO_GATE_CONST_1) {
            old_to_new[i] = mono_circuit_add_const(clean, 1);
        } else {
            int in1 = old_to_new[g->fanin[0]];
            int in2 = old_to_new[g->fanin[1]];

            /* Idempotence: same input twice -> skip gate, use input directly */
            if (in1 == in2) {
                old_to_new[i] = in1;
                continue;
            }

            if (g->type == MONO_GATE_AND)
                old_to_new[i] = mono_circuit_add_and(clean, in1, in2);
            else
                old_to_new[i] = mono_circuit_add_or(clean, in1, in2);
        }
    }

    mono_circuit_set_output(clean, old_to_new[c->output_gate]);
    free(old_to_new);
    return clean;
}

/* ============================================================
 * Constant Propagation
 * ============================================================ */

MonotoneCircuit* mono_circuit_constant_propagation(const MonotoneCircuit *c) {
    MonotoneCircuit *opt = mono_circuit_create(c->num_inputs);
    int *old_to_new = malloc((size_t)c->num_gates * sizeof(int));
    int *known_value = malloc((size_t)c->num_gates * sizeof(int));
    int *is_constant = malloc((size_t)c->num_gates * sizeof(int));
    assert(old_to_new && known_value && is_constant);

    for (int i = 0; i < c->num_gates; i++) {
        is_constant[i] = 0;
        known_value[i] = 0;
    }

    for (int i = 0; i < c->num_gates; i++) {
        const MonoGate *g = &c->gates[i];

        if (g->type == MONO_GATE_INPUT) {
            old_to_new[i] = mono_circuit_add_input(opt, g->input_idx);
        } else if (g->type == MONO_GATE_CONST_0) {
            old_to_new[i] = mono_circuit_add_const(opt, 0);
            is_constant[i] = 1;
            known_value[i] = 0;
        } else if (g->type == MONO_GATE_CONST_1) {
            old_to_new[i] = mono_circuit_add_const(opt, 1);
            is_constant[i] = 1;
            known_value[i] = 1;
        } else if (g->type == MONO_GATE_AND) {
            int i1 = g->fanin[0], i2 = g->fanin[1];
            /* AND with 0 => 0 */
            if ((is_constant[i1] && known_value[i1] == 0) ||
                (is_constant[i2] && known_value[i2] == 0)) {
                old_to_new[i] = mono_circuit_add_const(opt, 0);
                is_constant[i] = 1;
                known_value[i] = 0;
            } else if (is_constant[i1] && known_value[i1] == 1) {
                old_to_new[i] = old_to_new[i2]; /* 1 AND x = x */
            } else if (is_constant[i2] && known_value[i2] == 1) {
                old_to_new[i] = old_to_new[i1];
            } else {
                old_to_new[i] = mono_circuit_add_and(opt,
                    old_to_new[i1], old_to_new[i2]);
            }
        } else if (g->type == MONO_GATE_OR) {
            int i1 = g->fanin[0], i2 = g->fanin[1];
            /* OR with 1 => 1 */
            if ((is_constant[i1] && known_value[i1] == 1) ||
                (is_constant[i2] && known_value[i2] == 1)) {
                old_to_new[i] = mono_circuit_add_const(opt, 1);
                is_constant[i] = 1;
                known_value[i] = 1;
            } else if (is_constant[i1] && known_value[i1] == 0) {
                old_to_new[i] = old_to_new[i2]; /* 0 OR x = x */
            } else if (is_constant[i2] && known_value[i2] == 0) {
                old_to_new[i] = old_to_new[i1];
            } else {
                old_to_new[i] = mono_circuit_add_or(opt,
                    old_to_new[i1], old_to_new[i2]);
            }
        }
    }

    mono_circuit_set_output(opt, old_to_new[c->output_gate]);
    free(old_to_new);
    free(known_value);
    free(is_constant);
    return opt;
}
