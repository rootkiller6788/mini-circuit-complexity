/* circuit_model.c -- Boolean Circuit DAG Model
 *
 * L1: BooleanCircuit: DAG with AND/OR/NOT gates.
 * L2: Circuit evaluation, depth computation, gate counting.
 * L3: Graph theory: DAG topology, levels, topological ordering.
 *
 * Circuit definition:
 *   A Boolean circuit C on n inputs is a directed acyclic graph where:
 *     - There are n input nodes labeled x_0, ..., x_{n-1} (no incoming edges)
 *     - Internal nodes are gates: AND, OR, NOT (with 2, 2, 1 inputs resp.)
 *     - There is one designated output node
 *   The circuit computes a function f_C: {0,1}^n -> {0,1}.
 *
 * Circuit size: number of non-input gates.
 * Circuit depth: longest path from input to output.
 * Circuit width: maximum gates at any topological level.
 *
 * Circuit families:
 *   A family {C_n}_{n=1}^{\infty} where C_n has n inputs.
 *   C_n has size S(n) and depth D(n).
 *
 * Complexity classes (by resource bounds on families):
 *   SIZE[S(n)] = {languages decided by circuit families of size O(S(n))}
 *   DEPTH[D(n)] = languages with depth O(D(n))
 *   P/poly = union_{k} SIZE[n^k]
 *   NC1 = DEPTH[O(log n)] with bounded fan-in
 *   AC0 = DEPTH[O(1)] with unbounded fan-in
 *   TC0 = DEPTH[O(1)] with threshold gates
 *
 * References:
 *   Vollmer (1999): "Introduction to Circuit Complexity"
 *   Arora-Barak (2009), Chapter 6
 *   Jukna (2012), Chapter 1
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "natural_core.h"

/* ========================================================================
 * Circuit Construction
 * ======================================================================== */

BooleanCircuit *circ_create(int n_inputs, int initial_capacity) {
    if (n_inputs < 0) return NULL;
    if (initial_capacity < n_inputs + 4) initial_capacity = n_inputs + 4;

    BooleanCircuit *c = (BooleanCircuit *)malloc(sizeof(BooleanCircuit));
    if (!c) return NULL;

    c->n_inputs = n_inputs;
    c->n_outputs = 1;
    c->n_gates = 0;
    c->capacity = initial_capacity;
    c->output_gate = -1;
    c->output_gates = NULL;
    c->size = 0;
    c->depth = 0;
    c->class_name[0] = '\0';
    c->is_uniform = 0;

    c->gates = (CircuitGate *)malloc(
        (size_t)initial_capacity * sizeof(CircuitGate));
    if (!c->gates) {
        free(c);
        return NULL;
    }

    /* Create input gates */
    for (int i = 0; i < n_inputs; i++) {
        CircuitGate *g = &c->gates[c->n_gates++];
        g->type = GATE_INPUT;
        g->gate_id = i;
        g->fanin_count = 0;
        g->fanin = NULL;
        g->value = 0;
        g->level = 0;
        snprintf(g->label, 31, "x_%d", i);
    }

    return c;
}

void circ_free(BooleanCircuit *c) {
    if (!c) return;
    for (int i = 0; i < c->n_gates; i++) {
        free(c->gates[i].fanin);
    }
    free(c->gates);
    free(c->output_gates);
    free(c);
}

int circ_add_gate(BooleanCircuit *c, GateType type,
                  const int *fanin_ids, int n_fanin) {
    if (!c) return -1;

    /* Expand capacity if needed */
    if (c->n_gates >= c->capacity) {
        int new_cap = c->capacity * 2;
        CircuitGate *new_gates = (CircuitGate *)realloc(
            c->gates, (size_t)new_cap * sizeof(CircuitGate));
        if (!new_gates) return -1;
        c->gates = new_gates;
        c->capacity = new_cap;
    }

    int id = c->n_gates;
    CircuitGate *g = &c->gates[id];
    g->type = type;
    g->gate_id = id;
    g->fanin_count = n_fanin;
    g->fanin = NULL;
    g->value = 0;
    g->level = -1;  /* not computed yet */

    /* Set label */
    switch (type) {
        case GATE_AND:    snprintf(g->label, 31, "AND_%d", id); break;
        case GATE_OR:     snprintf(g->label, 31, "OR_%d", id); break;
        case GATE_NOT:    snprintf(g->label, 31, "NOT_%d", id); break;
        case GATE_XOR:    snprintf(g->label, 31, "XOR_%d", id); break;
        case GATE_MAJ:    snprintf(g->label, 31, "MAJ_%d", id); break;
        case GATE_CONST0: snprintf(g->label, 31, "CONST0"); break;
        case GATE_CONST1: snprintf(g->label, 31, "CONST1"); break;
        default:          snprintf(g->label, 31, "G_%d", id); break;
    }

    /* Copy fan-in list */
    if (n_fanin > 0 && fanin_ids) {
        g->fanin = (int *)malloc((size_t)n_fanin * sizeof(int));
        if (!g->fanin) {
            /* Leave as NULL — gate will always evaluate to 0 */
            g->fanin_count = 0;
        } else {
            memcpy(g->fanin, fanin_ids, (size_t)n_fanin * sizeof(int));
        }
    }

    c->n_gates++;
    if (type != GATE_INPUT) c->size++;
    return id;
}

int circ_add_input(BooleanCircuit *c, int var_index) {
    if (!c || var_index < 0) return -1;
    /* Input gates already created in circ_create.
     * If var_index >= n_inputs, we need to add more. */
    if (var_index < c->n_inputs) return var_index;

    /* Adding extra input beyond initial n_inputs — unusual but supported */
    return circ_add_gate(c, GATE_INPUT, NULL, 0);
}

void circ_set_output(BooleanCircuit *c, int gate_id) {
    if (!c || gate_id < 0 || gate_id >= c->n_gates) return;
    c->output_gate = gate_id;
}

/* ========================================================================
 * Circuit Evaluation
 * ======================================================================== */

/*
 * circ_evaluate_rec: Recursive circuit evaluation.
 *   Avoids recomputation by checking if gate->value is already set.
 *   Uses a sentinel to detect cycles (shouldn't happen in valid circuit).
 */
static int circ_evaluate_rec(BooleanCircuit *c, int gate_id,
                              const int *input, int *visited) {
    if (gate_id < 0 || gate_id >= c->n_gates) return 0;

    /* Cycle detection */
    if (visited[gate_id]) return c->gates[gate_id].value;
    visited[gate_id] = 1;

    CircuitGate *g = &c->gates[gate_id];

    switch (g->type) {
        case GATE_INPUT:
            g->value = input[gate_id];
            return g->value;

        case GATE_CONST0:
            g->value = 0;
            return 0;

        case GATE_CONST1:
            g->value = 1;
            return 1;

        case GATE_NOT:
            if (g->fanin_count >= 1) {
                int in_val = circ_evaluate_rec(c, g->fanin[0], input, visited);
                g->value = 1 - in_val;
            } else {
                g->value = 0;
            }
            return g->value;

        case GATE_AND:
            {
                int val = 1;
                for (int i = 0; i < g->fanin_count; i++) {
                    if (circ_evaluate_rec(c, g->fanin[i], input, visited) == 0) {
                        val = 0;
                        break;
                    }
                }
                g->value = val;
            }
            return g->value;

        case GATE_OR:
            {
                int val = 0;
                for (int i = 0; i < g->fanin_count; i++) {
                    if (circ_evaluate_rec(c, g->fanin[i], input, visited) == 1) {
                        val = 1;
                        break;
                    }
                }
                g->value = val;
            }
            return g->value;

        case GATE_XOR:
            {
                int val = 0;
                for (int i = 0; i < g->fanin_count; i++) {
                    val ^= circ_evaluate_rec(c, g->fanin[i], input, visited);
                }
                g->value = val;
            }
            return g->value;

        case GATE_MAJ:
            {
                int count = 0;
                for (int i = 0; i < g->fanin_count; i++) {
                    count += circ_evaluate_rec(c, g->fanin[i], input, visited);
                }
                g->value = (count > g->fanin_count / 2) ? 1 : 0;
            }
            return g->value;

        case GATE_MODm:
            {
                int count = 0;
                for (int i = 0; i < g->fanin_count; i++) {
                    count += circ_evaluate_rec(c, g->fanin[i], input, visited);
                }
                /* MOD-2 by default; mod value stored in fanin_count */
                g->value = (count % 2 == 0) ? 0 : 1;
            }
            return g->value;

        default:
            g->value = 0;
            return 0;
    }
}

int circ_evaluate(BooleanCircuit *c, const int *input) {
    if (!c || !input) return 0;
    if (c->output_gate < 0) return 0;

    /* Allocate visited array on stack for small circuits */
    int *visited = (int *)calloc((size_t)c->n_gates, sizeof(int));
    if (!visited) return 0;

    /* Reset all gate values */
    for (int i = 0; i < c->n_gates; i++) {
        c->gates[i].value = -1;
    }

    int result = circ_evaluate_rec(c, c->output_gate, input, visited);
    free(visited);
    return result;
}

TruthTable *circ_evaluate_tt(BooleanCircuit *c) {
    if (!c) return NULL;
    TruthTable *tt = tt_create(c->n_inputs);
    if (!tt) return NULL;

    int *input = (int *)calloc((size_t)c->n_inputs, sizeof(int));
    if (!input) { tt_free(tt); return NULL; }

    for (long long i = 0; i < tt->table_size; i++) {
        /* Decode i into input bits */
        for (int j = 0; j < c->n_inputs; j++) {
            input[j] = (int)((i >> j) & 1);
        }
        tt->values[i] = circ_evaluate(c, input);
    }

    free(input);
    snprintf(tt->name, 63, "circuit_fn[%d]", c->n_inputs);
    return tt;
}

/* ========================================================================
 * Circuit Analysis
 * ======================================================================== */

void circ_compute_depth(BooleanCircuit *c) {
    if (!c) return;

    /* Initialize levels */
    for (int i = 0; i < c->n_gates; i++) {
        if (c->gates[i].type == GATE_INPUT) {
            c->gates[i].level = 0;
        } else {
            c->gates[i].level = -1;
        }
    }

    /* Topological computation of levels.
     * Since gates are added in topological order, a single pass suffices. */
    int changed = 1;
    while (changed) {
        changed = 0;
        for (int i = 0; i < c->n_gates; i++) {
            CircuitGate *g = &c->gates[i];
            if (g->level >= 0) continue;  /* already computed */

            int max_fanin_level = -1;
            int all_ready = 1;
            for (int j = 0; j < g->fanin_count; j++) {
                int fi = g->fanin[j];
                if (fi >= 0 && fi < c->n_gates) {
                    if (c->gates[fi].level < 0) {
                        all_ready = 0;
                        break;
                    }
                    if (c->gates[fi].level > max_fanin_level) {
                        max_fanin_level = c->gates[fi].level;
                    }
                }
            }
            if (all_ready) {
                g->level = max_fanin_level + 1;
                changed = 1;
            }
        }
    }

    /* Maximum level = depth */
    c->depth = 0;
    for (int i = 0; i < c->n_gates; i++) {
        if (c->gates[i].level > c->depth)
            c->depth = c->gates[i].level;
    }
}

int *circ_count_gates_by_type(BooleanCircuit *c) {
    if (!c) return NULL;
    int *counts = (int *)calloc(10, sizeof(int));  /* GATE_OUTPUT+1 = 10 */
    if (!counts) return NULL;

    for (int i = 0; i < c->n_gates; i++) {
        GateType t = c->gates[i].type;
        if (t <= GATE_OUTPUT) counts[t]++;
    }
    return counts;
}

void circ_print(BooleanCircuit *c, FILE *fp) {
    if (!c) { fprintf(fp, "NULL circuit\n"); return; }
    if (!fp) fp = stdout;

    fprintf(fp, "BooleanCircuit: %d inputs, %d gates, %d outputs\n",
            c->n_inputs, c->n_gates, c->n_outputs);
    fprintf(fp, "  Size: %d (non-input gates), Depth: %d\n",
            c->size, c->depth);
    if (c->class_name[0])
        fprintf(fp, "  Class: %s\n", c->class_name);

    if (c->n_gates <= 30) {
        fprintf(fp, "\n  Gates:\n");
        for (int i = 0; i < c->n_gates; i++) {
            CircuitGate *g = &c->gates[i];
            fprintf(fp, "  [%2d] %-8s fanin=[", g->gate_id, g->label);
            for (int j = 0; j < g->fanin_count; j++) {
                fprintf(fp, "%d%s", g->fanin[j],
                        (j + 1 < g->fanin_count) ? "," : "");
            }
            fprintf(fp, "] level=%d", g->level);
            if (i == c->output_gate) fprintf(fp, " ***OUTPUT***");
            fprintf(fp, "\n");
        }
    } else {
        fprintf(fp, "  (gate list suppressed — %d gates total)\n", c->n_gates);
    }
}

void circ_print_stats(BooleanCircuit *c, FILE *fp) {
    if (!c) { fprintf(fp, "NULL circuit\n"); return; }
    if (!fp) fp = stdout;

    int *counts = circ_count_gates_by_type(c);
    fprintf(fp, "Circuit Stats:\n");
    fprintf(fp, "  Inputs:    %d\n", c->n_inputs);
    fprintf(fp, "  Gates:     %d\n", c->n_gates);
    fprintf(fp, "  Size:      %d\n", c->size);
    fprintf(fp, "  Depth:     %d\n", c->depth);
    if (counts) {
        fprintf(fp, "  AND: %d, OR: %d, NOT: %d, XOR: %d, MAJ: %d\n",
                counts[GATE_AND], counts[GATE_OR], counts[GATE_NOT],
                counts[GATE_XOR], counts[GATE_MAJ]);
        free(counts);
    }
}

/* ========================================================================
 * CircuitClass Helpers
 * ======================================================================== */

const char *circuit_class_name(CircuitClass cc) {
    switch (cc) {
        case CLASS_AC0:      return "AC0";
        case CLASS_AC0_p:    return "AC0[p]";
        case CLASS_ACC0:     return "ACC0";
        case CLASS_TC0:      return "TC0";
        case CLASS_NC1:      return "NC1";
        case CLASS_NC:       return "NC";
        case CLASS_P_poly:   return "P/poly";
        case CLASS_SIZE_S:   return "SIZE[S]";
        case CLASS_MONOTONE: return "Monotone";
        case CLASS_GENERAL:  return "All circuits";
        default:             return "Unknown";
    }
}

const char *circuit_class_description(CircuitClass cc) {
    switch (cc) {
        case CLASS_AC0:
            return "Constant-depth circuits with unbounded fan-in AND/OR/NOT gates. "
                   "Cannot compute PARITY (FSS/Ajtai/Hastad).";
        case CLASS_AC0_p:
            return "AC0 extended with MOD-p gates (p prime). "
                   "Cannot compute MOD-q for q != p^k (Razborov-Smolensky).";
        case CLASS_ACC0:
            return "AC0 with MOD-m gates for any fixed m. "
                   "Conjectured not to contain MAJORITY (open problem).";
        case CLASS_TC0:
            return "Constant-depth circuits with threshold (MAJORITY) gates. "
                   "Contains addition, multiplication, sorting.";
        case CLASS_NC1:
            return "Log-depth circuits with bounded fan-in. "
                   "Equivalent to polynomial-size Boolean formulas. "
                   "Contains TC0, contained in P/poly.";
        case CLASS_NC:
            return "Nick's Class: polylog-depth, poly-size with bounded fan-in. "
                   "The class of efficiently parallelizable problems.";
        case CLASS_P_poly:
            return "Polynomial-size circuit families (non-uniform P). "
                   "Contains NC, contained in NP/poly. "
                   "Natural proofs barrier targets separation from NP.";
        case CLASS_SIZE_S:
            return "Circuits of size at most S(n) for a specific function S. "
                   "Shannon: most functions require S = Omega(2^n/n).";
        case CLASS_MONOTONE:
            return "Monotone circuits (no NOT gates). "
                   "Cannot compute CLIQUE (Razborov 1985).";
        case CLASS_GENERAL:
            return "All Boolean circuits (no resource bounds). "
                   "Every function is computable by some circuit.";
        default:
            return "Unknown circuit class.";
    }
}
