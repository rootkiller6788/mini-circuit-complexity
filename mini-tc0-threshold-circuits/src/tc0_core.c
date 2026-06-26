/* tc0_core.c -- TC0 Core: Circuit Construction, Evaluation, and Analysis
 *
 * Implements fundamental operations for threshold circuits.
 * TC0 = constant-depth, poly-size circuits with MAJ/THR gates.
 *
 * Key concepts (L1-L4):
 *   L1: TC0Circuit, TC0Gate, TC0GateType — core data types
 *   L2: DAG evaluation, memoization, depth computation
 *   L3: Gate semantics (MAJ, THR, WTHR, MODp, XOR, etc.)
 *   L4: DAG verification (cycle detection), depth-size tradeoffs
 *
 * Hierarchy: AC0 c TC0 c= NC1 c= P
 * (TC0 vs NC1: open since 1989 — whether TC0 = NC1)
 *
 * References:
 *   - Vollmer (1999) "Introduction to Circuit Complexity", Ch. 2, 4
 *   - Arora & Barak (2009) "Computational Complexity: A Modern Approach", Ch. 6, 14
 *   - Barrington, Immerman, Straubing (1990) "On uniformity within NC1"
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include "tc0.h"

/* ================================================================
 * CIRCUIT LIFECYCLE
 * ================================================================ */

TC0Circuit *tc0_circuit_create(int max_gates) {
    if (max_gates <= 0) return NULL;
    TC0Circuit *c = (TC0Circuit *)malloc(sizeof(TC0Circuit));
    if (!c) return NULL;
    c->gates = (TC0Gate *)calloc((size_t)max_gates, sizeof(TC0Gate));
    if (!c->gates) { free(c); return NULL; }
    c->num_gates = max_gates;
    c->gate_count = 0;
    c->num_inputs = 0;
    c->num_outputs = 0;
    c->output_gate_ids = NULL;
    c->max_depth = -1;
    c->total_wires = 0;
    c->is_uniform = 1;
    c->name[0] = '\0';
    return c;
}

void tc0_circuit_free(TC0Circuit *c) {
    if (!c) return;
    free(c->gates);
    free(c->output_gate_ids);
    free(c);
}

void tc0_circuit_reset(TC0Circuit *c) {
    if (!c) return;
    memset(c->gates, 0, (size_t)c->num_gates * sizeof(TC0Gate));
    c->gate_count = 0;
    c->num_inputs = 0;
    c->num_outputs = 0;
    c->max_depth = -1;
    c->total_wires = 0;
    free(c->output_gate_ids);
    c->output_gate_ids = NULL;
}

/* ================================================================
 * GATE OPERATIONS
 * ================================================================ */

int tc0_add_input(TC0Circuit *c) {
    if (!c || c->gate_count >= c->num_gates) return -1;
    int id = c->gate_count++;
    TC0Gate *g = &c->gates[id];
    g->type = TC_INPUT;
    g->gate_id = id;
    g->fan_in_count = 0;
    g->fan_out_count = 0;
    g->depth = 0;
    g->is_output = 0;
    c->num_inputs++;
    return id;
}

int tc0_add_constant(TC0Circuit *c, int val) {
    if (!c || c->gate_count >= c->num_gates) return -1;
    if (val != 0 && val != 1) return -1;
    int id = c->gate_count++;
    TC0Gate *g = &c->gates[id];
    g->type = TC_CONST;
    g->gate_id = id;
    g->const_val = val;
    g->fan_in_count = 0;
    g->fan_out_count = 0;
    g->depth = 0;
    g->is_output = 0;
    return id;
}

int tc0_add_gate(TC0Circuit *c, TC0GateType type) {
    if (!c || c->gate_count >= c->num_gates) return -1;
    int id = c->gate_count++;
    TC0Gate *g = &c->gates[id];
    g->type = type;
    g->gate_id = id;
    g->fan_in_count = 0;
    g->fan_out_count = 0;
    g->thr_k = 0;
    g->thr_theta = 0.0;
    g->const_val = 0;
    g->mod_p = 2;
    g->depth = -1;
    g->is_output = 0;
    g->comp_pair = -1;
    memset(g->weights, 0, sizeof(g->weights));
    return id;
}

int tc0_add_wire(TC0Circuit *c, int from_gate, int to_gate) {
    if (!c) return -1;
    if (from_gate < 0 || from_gate >= c->gate_count) return -1;
    if (to_gate < 0 || to_gate >= c->gate_count) return -1;
    TC0Gate *src = &c->gates[from_gate];
    TC0Gate *dst = &c->gates[to_gate];
    if (src->fan_out_count >= 256) return -1;
    src->fan_out[src->fan_out_count++] = to_gate;
    if (dst->fan_in_count >= 256) return -1;
    dst->fan_in[dst->fan_in_count++] = from_gate;
    c->total_wires++;
    return 0;
}

int tc0_set_threshold(TC0Circuit *c, int gate_id, int k) {
    if (!c || gate_id < 0 || gate_id >= c->gate_count) return -1;
    if (c->gates[gate_id].type != TC_THR) return -1;
    c->gates[gate_id].thr_k = k;
    return 0;
}

int tc0_set_weighted_threshold(TC0Circuit *c, int gate_id,
                                const double *weights, int n_weights,
                                double theta) {
    if (!c || gate_id < 0 || gate_id >= c->gate_count) return -1;
    if (!weights || n_weights <= 0 || n_weights > 256) return -1;
    TC0Gate *g = &c->gates[gate_id];
    if (g->type != TC_WTHR) return -1;
    for (int i = 0; i < n_weights; i++) g->weights[i] = weights[i];
    g->thr_theta = theta;
    return 0;
}

int tc0_set_output(TC0Circuit *c, int gate_id) {
    if (!c || gate_id < 0 || gate_id >= c->gate_count) return -1;
    c->gates[gate_id].is_output = 1;
    int *new_out = (int *)realloc(c->output_gate_ids,
                                   (size_t)(c->num_outputs + 1) * sizeof(int));
    if (!new_out) return -1;
    c->output_gate_ids = new_out;
    c->output_gate_ids[c->num_outputs++] = gate_id;
    return 0;
}

/* ================================================================
 * CIRCUIT EVALUATION
 * Uses memoization (visited array) to evaluate DAG in O(size) time.
 * ================================================================ */

int tc0_evaluate_gate(const TC0Circuit *c, int gate_id,
                      const int *input_assign, int *visited) {
    if (!c || !input_assign || !visited) return 0;
    if (gate_id < 0 || gate_id >= c->gate_count) return 0;
    /* Memoization: already computed */
    if (visited[gate_id] >= 0) return visited[gate_id];

    const TC0Gate *g = &c->gates[gate_id];
    int result = 0;

    switch (g->type) {
    case TC_INPUT:
        result = (gate_id < c->num_inputs) ? input_assign[gate_id] : 0;
        break;
    case TC_CONST:
        result = g->const_val;
        break;
    case TC_NOT: {
        int in_val = tc0_evaluate_gate(c, g->fan_in[0], input_assign, visited);
        result = !in_val;
        break;
    }
    case TC_AND: {
        result = 1;
        for (int i = 0; i < g->fan_in_count; i++) {
            if (!tc0_evaluate_gate(c, g->fan_in[i], input_assign, visited)) {
                result = 0;
                break;
            }
        }
        break;
    }
    case TC_OR: {
        result = 0;
        for (int i = 0; i < g->fan_in_count; i++) {
            if (tc0_evaluate_gate(c, g->fan_in[i], input_assign, visited)) {
                result = 1;
                break;
            }
        }
        break;
    }
    case TC_MAJ: {
        /* MAJORITY: f(x) = 1 iff sum(x_i) > n/2 */
        int sum = 0, total = g->fan_in_count;
        for (int i = 0; i < total; i++)
            sum += tc0_evaluate_gate(c, g->fan_in[i], input_assign, visited);
        result = (sum > total / 2) ? 1 : 0;
        break;
    }
    case TC_THR: {
        /* THRESHOLD(k): f(x) = 1 iff sum(x_i) >= k */
        int sum = 0;
        for (int i = 0; i < g->fan_in_count; i++)
            sum += tc0_evaluate_gate(c, g->fan_in[i], input_assign, visited);
        result = (sum >= g->thr_k) ? 1 : 0;
        break;
    }
    case TC_WTHR: {
        /* Weighted threshold: f(x) = 1 iff sum(w_i * x_i) >= theta
         * This is a linear threshold function (LTF), equivalent to
         * a perceptron / artificial neuron. */
        double sum = 0.0;
        for (int i = 0; i < g->fan_in_count; i++) {
            int val = tc0_evaluate_gate(c, g->fan_in[i], input_assign, visited);
            sum += (double)val * g->weights[i];
        }
        result = (sum >= g->thr_theta) ? 1 : 0;
        break;
    }
    case TC_MODP: {
        /* MOD_p: f(x) = 1 iff sum(x_i) mod p == 0
         * MOD3 gates define ACC0; Razborov-Smolensky: MAJORITY not in ACC0 */
        int sum = 0;
        for (int i = 0; i < g->fan_in_count; i++)
            sum += tc0_evaluate_gate(c, g->fan_in[i], input_assign, visited);
        result = (sum % g->mod_p == 0) ? 1 : 0;
        break;
    }
    case TC_XOR: {
        /* XOR = PARITY: f(x) = sum(x_i) mod 2
         * PARITY in NC1 but believed NOT in TC0 */
        int parity = 0;
        for (int i = 0; i < g->fan_in_count; i++)
            parity ^= tc0_evaluate_gate(c, g->fan_in[i], input_assign, visited);
        result = parity;
        break;
    }
    case TC_NAND:
        result = 0;
        for (int i = 0; i < g->fan_in_count; i++) {
            if (!tc0_evaluate_gate(c, g->fan_in[i], input_assign, visited)) {
                result = 1;
                break;
            }
        }
        break;
    case TC_NOR:
        result = 1;
        for (int i = 0; i < g->fan_in_count; i++) {
            if (tc0_evaluate_gate(c, g->fan_in[i], input_assign, visited)) {
                result = 0;
                break;
            }
        }
        break;
    case TC_MUX: {
        int sel = tc0_evaluate_gate(c, g->fan_in[0], input_assign, visited);
        int a_val = tc0_evaluate_gate(c, g->fan_in[1], input_assign, visited);
        int b_val = tc0_evaluate_gate(c, g->fan_in[2], input_assign, visited);
        result = sel ? a_val : b_val;
        break;
    }
    case TC_COMP:
        if (g->fan_in_count >= 2) {
            int v0 = tc0_evaluate_gate(c, g->fan_in[0], input_assign, visited);
            int v1 = tc0_evaluate_gate(c, g->fan_in[1], input_assign, visited);
            result = (v0 <= v1) ? v0 : v1;
        } else {
            result = 0;
        }
        break;
    default:
        result = 0;
        break;
    }

    visited[gate_id] = result;
    return result;
}

int tc0_evaluate_circuit(const TC0Circuit *c, const int *input_assign) {
    if (!c || !input_assign) return 0;
    if (c->num_outputs == 0) return 0;

    int *visited = (int *)malloc((size_t)c->gate_count * sizeof(int));
    if (!visited) return 0;
    for (int i = 0; i < c->gate_count; i++) visited[i] = -1;

    /* Conjunctive semantics: AND of all outputs */
    int result = 1;
    for (int i = 0; i < c->num_outputs; i++) {
        int out_val = tc0_evaluate_gate(c, c->output_gate_ids[i],
                                         input_assign, visited);
        result &= out_val;
    }
    free(visited);
    return result;
}

int tc0_evaluate_all_outputs(const TC0Circuit *c, const int *input_assign,
                              int *output_vals) {
    if (!c || !input_assign || !output_vals) return -1;

    int *visited = (int *)malloc((size_t)c->gate_count * sizeof(int));
    if (!visited) return -1;
    for (int i = 0; i < c->gate_count; i++) visited[i] = -1;

    for (int i = 0; i < c->num_outputs; i++) {
        output_vals[i] = tc0_evaluate_gate(c, c->output_gate_ids[i],
                                            input_assign, visited);
    }
    free(visited);
    return 0;
}

/* ================================================================
 * CIRCUIT ANALYSIS: Depth, DAG verification, metrics
 * ================================================================ */

int tc0_compute_depth(TC0Circuit *c) {
    if (!c) return -1;
    int max_d = 0;
    for (int i = 0; i < c->gate_count; i++) {
        TC0Gate *g = &c->gates[i];
        if (g->type == TC_INPUT || g->type == TC_CONST) {
            g->depth = 0;
            continue;
        }
        int max_fan_depth = -1;
        if (g->fan_in_count > 0) {
            for (int j = 0; j < g->fan_in_count; j++) {
                int fd = c->gates[g->fan_in[j]].depth;
                if (fd > max_fan_depth) max_fan_depth = fd;
            }
            g->depth = max_fan_depth + 1;
        } else {
            g->depth = 0;
        }
        if (g->depth > max_d) max_d = g->depth;
    }
    c->max_depth = max_d;
    return max_d;
}

/* Three-color DFS for cycle detection:
 *   0 = WHITE (unvisited)
 *   1 = GRAY  (in current recursion path => back edge = cycle)
 *   2 = BLACK (fully explored) */
static int dfs_cycle_detect(const TC0Circuit *c, int gate_id, int *color) {
    color[gate_id] = 1;  /* GRAY */
    const TC0Gate *g = &c->gates[gate_id];
    for (int i = 0; i < g->fan_out_count; i++) {
        int next = g->fan_out[i];
        if (color[next] == 1) return 1;  /* back edge → cycle */
        if (color[next] == 0 && dfs_cycle_detect(c, next, color)) return 1;
    }
    color[gate_id] = 2;  /* BLACK */
    return 0;
}

int tc0_verify_dag(const TC0Circuit *c) {
    if (!c) return 0;
    int *color = (int *)calloc((size_t)c->gate_count, sizeof(int));
    if (!color) return 0;
    int has_cycle = 0;
    for (int i = 0; i < c->gate_count && !has_cycle; i++) {
        if (color[i] == 0) has_cycle = dfs_cycle_detect(c, i, color);
    }
    free(color);
    return !has_cycle;
}

void tc0_count_gate_types(const TC0Circuit *c, int *counts) {
    if (!c || !counts) return;
    memset(counts, 0, (TC_MUX + 1) * sizeof(int));
    for (int i = 0; i < c->gate_count; i++) {
        int t = (int)c->gates[i].type;
        if (t >= 0 && t <= TC_MUX) counts[t]++;
    }
}

int tc0_circuit_size(const TC0Circuit *c) {
    return c ? c->gate_count : 0;
}

int tc0_circuit_wire_count(const TC0Circuit *c) {
    return c ? c->total_wires : 0;
}

int tc0_circuit_max_fanin(const TC0Circuit *c) {
    if (!c) return 0;
    int max_f = 0;
    for (int i = 0; i < c->gate_count; i++) {
        if (c->gates[i].fan_in_count > max_f)
            max_f = c->gates[i].fan_in_count;
    }
    return max_f;
}

/* Compute the size lower bound for a function via gate-elimination.
 * For a Boolean function f on n variables:
 *   If f is in TC0 of depth d, then size >= n^c for some c depending on d.
 * This is a simplified version of the depth-reduction lemma. */
void tc0_size_bounds(int n, int depth, int *lower, int *upper) {
    /* Upper bound: naive construction with one THR gate per output bit */
    *upper = n + 1;
    /* Lower bound: at least n+1 gates (n inputs + 1 output) */
    *lower = n + 1;
    /* For depth > 1, size can be polynomial in n */
    if (depth > 1) {
        *upper = n * n;  /* O(n^2) naive upper bound */
    }
}

/* ================================================================
 * PRINTING AND DEBUGGING OUTPUT
 * ================================================================ */

static const char *gate_type_name(TC0GateType t) {
    switch (t) {
        case TC_INPUT: return "INPUT";
        case TC_CONST: return "CONST";
        case TC_NOT:   return "NOT";
        case TC_AND:   return "AND";
        case TC_OR:    return "OR";
        case TC_MAJ:   return "MAJ";
        case TC_THR:   return "THR";
        case TC_WTHR:  return "WTHR";
        case TC_MODP:  return "MODp";
        case TC_COMP:  return "COMP";
        case TC_XOR:   return "XOR";
        case TC_NAND:  return "NAND";
        case TC_NOR:   return "NOR";
        case TC_MUX:   return "MUX";
        default:       return "???";
    }
}

void tc0_print_circuit(const TC0Circuit *c) {
    if (!c) { printf("(null circuit)\n"); return; }
    printf("Circuit: %s\n", c->name[0] ? c->name : "(unnamed)");
    printf("  Gates: %d (inputs: %d, outputs: %d)\n",
           c->gate_count, c->num_inputs, c->num_outputs);
    printf("  Wires: %d, Max depth: %d\n", c->total_wires, c->max_depth);
    printf("  Output gates: ");
    for (int i = 0; i < c->num_outputs; i++)
        printf("%d ", c->output_gate_ids[i]);
    printf("\n\n");
    for (int i = 0; i < c->gate_count; i++)
        tc0_print_gate(c, i);
}

void tc0_print_gate(const TC0Circuit *c, int gate_id) {
    if (!c || gate_id < 0 || gate_id >= c->gate_count) return;
    const TC0Gate *g = &c->gates[gate_id];
    printf("  G%d [%s] depth=%d", gate_id, gate_type_name(g->type), g->depth);
    if (g->type == TC_THR)  printf(" k=%d", g->thr_k);
    if (g->type == TC_WTHR) printf(" theta=%.1f", g->thr_theta);
    if (g->type == TC_CONST) printf(" =%d", g->const_val);
    if (g->type == TC_MODP)  printf(" mod=%d", g->mod_p);
    if (g->is_output) printf(" [OUT]");
    if (g->fan_in_count > 0) {
        printf("\n    fan_in:  ");
        for (int i = 0; i < g->fan_in_count; i++)
            printf("G%d ", g->fan_in[i]);
    }
    if (g->fan_out_count > 0) {
        printf("\n    fan_out: ");
        for (int i = 0; i < g->fan_out_count; i++)
            printf("G%d ", g->fan_out[i]);
    }
    printf("\n");
}

void tc0_export_dot(const TC0Circuit *c, FILE *fp) {
    if (!c || !fp) return;
    fprintf(fp, "digraph TC0Circuit {\n");
    fprintf(fp, "  rankdir=LR;\n");
    fprintf(fp, "  label=\"%s\";\n", c->name[0] ? c->name : "TC0 Circuit");
    fprintf(fp, "  node [shape=box, style=filled, fillcolor=lightyellow];\n");
    for (int i = 0; i < c->gate_count; i++) {
        const TC0Gate *g = &c->gates[i];
        const char *color = "lightyellow";
        if (g->type == TC_INPUT) color = "lightblue";
        else if (g->type == TC_MAJ) color = "lightcoral";
        else if (g->type == TC_THR || g->type == TC_WTHR) color = "lightsalmon";
        else if (g->is_output) color = "lightgreen";
        fprintf(fp, "  G%d [label=\"%s", i, gate_type_name(g->type));
        if (g->type == TC_THR) fprintf(fp, "(k=%d)", g->thr_k);
        if (g->type == TC_CONST) fprintf(fp, "=%d", g->const_val);
        fprintf(fp, "\", fillcolor=%s];\n", color);
    }
    for (int i = 0; i < c->gate_count; i++)
        for (int j = 0; j < c->gates[i].fan_out_count; j++)
            fprintf(fp, "  G%d -> G%d;\n", i, c->gates[i].fan_out[j]);
    fprintf(fp, "}\n");
}
