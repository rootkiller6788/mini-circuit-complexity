/* circuit.c -- Boolean Circuit Model: Core Implementation
 *
 * Implements the fundamental Boolean circuit data structure and operations.
 * A Boolean circuit is a DAG (Directed Acyclic Graph) where:
 *   - Source nodes (INPUT) represent variables x_0, x_1, ..., x_{n-1}
 *   - Internal nodes represent Boolean functions (AND, OR, NOT, etc.)
 *   - Sink nodes (outputs) produce the computed values
 *
 * Theorem (circuit_evaluate): Given a BooleanCircuit C with n inputs and
 * an assignment a in {0,1}^n, circuit_evaluate(C, a) computes C(a) in
 * time O(|C|) using memoization.
 *
 * Theorem (circuit_depth): The depth of C is the length of the longest
 * directed path from any input to any output. Computable in O(|C|) time
 * via topological DP.
 *
 * References:
 *   AB (2009) Ch.6: Boolean circuits, P/poly, NC, AC
 *   Vollmer (1999) Ch.1: Basic definitions and properties
 *   Jukna (2012) Ch.1: Boolean functions and their computational models
 *   Sipser (2013) Ch.9: Circuit complexity
 */

#include "circuit.h"
#include "circuit_complexity.h"
#include <stdint.h>
#include <assert.h>

/* ===================================================================
 * Memory Management
 * =================================================================== */

BooleanCircuit* circuit_create(int max_gates) {
    assert(max_gates > 0);
    BooleanCircuit* c = (BooleanCircuit*)malloc(sizeof(BooleanCircuit));
    if (!c) return NULL;
    c->gates = (Gate*)calloc((size_t)max_gates, sizeof(Gate));
    if (!c->gates) { free(c); return NULL; }
    c->n_gates    = 0;
    c->max_gates  = max_gates;
    c->n_inputs   = 0;
    c->n_outputs  = 0;
    c->output_ids = NULL;
    c->n_edges    = 0;
    c->depth_cache = -1;
    c->basis      = 0;  /* standard basis: AND, OR, NOT */
    return c;
}

void circuit_free(BooleanCircuit* c) {
    if (!c) return;
    if (c->gates) {
        for (int i = 0; i < c->n_gates; i++) {
            free(c->gates[i].fanin_ids);
        }
        free(c->gates);
    }
    free(c->output_ids);
    free(c);
}

/* ===================================================================
 * Gate Addition
 * =================================================================== */

static void circuit_grow_if_needed(BooleanCircuit* c) {
    if (c->n_gates >= c->max_gates) {
        int new_max = c->max_gates * 2;
        Gate* new_gates = (Gate*)realloc(c->gates, (size_t)new_max * sizeof(Gate));
        if (!new_gates) return;  /* allocation failure, keep old */
        /* zero-initialize the new portion */
        memset(new_gates + c->max_gates, 0,
               (size_t)(new_max - c->max_gates) * sizeof(Gate));
        c->gates = new_gates;
        c->max_gates = new_max;
    }
}

int circuit_add_gate(BooleanCircuit* c, GateType type, int in1, int in2) {
    assert(c != NULL);
    circuit_grow_if_needed(c);
    int id = c->n_gates++;
    Gate* g = &c->gates[id];
    g->id      = id;
    g->type    = type;
    g->input1  = in1;
    g->input2  = in2;
    g->fanin   = 0;
    g->fanin_ids = NULL;
    g->value   = -1;
    g->level   = -1;
    if (type == INPUT) c->n_inputs++;
    if (in1 >= 0) c->n_edges++;
    if (in2 >= 0) c->n_edges++;
    c->depth_cache = -1;  /* invalidate depth cache */
    return id;
}

int circuit_add_maj_gate(BooleanCircuit* c, GateType type, int fanin,
                          const int* fanin_ids) {
    assert(c != NULL);
    assert(type == MAJ || type == THR);
    assert(fanin > 0);
    assert(fanin_ids != NULL);
    circuit_grow_if_needed(c);
    int id = c->n_gates++;
    Gate* g = &c->gates[id];
    g->id      = id;
    g->type    = type;
    g->input1  = -1;
    g->input2  = -1;
    g->fanin   = fanin;
    g->fanin_ids = (int*)malloc((size_t)fanin * sizeof(int));
    if (g->fanin_ids) {
        memcpy(g->fanin_ids, fanin_ids, (size_t)fanin * sizeof(int));
    }
    g->value   = -1;
    g->level   = -1;
    c->n_edges += fanin;
    c->depth_cache = -1;
    return id;
}

void circuit_set_output(BooleanCircuit* c, const int* ids, int n) {
    assert(c != NULL);
    assert(ids != NULL);
    assert(n > 0);
    free(c->output_ids);
    c->n_outputs  = n;
    c->output_ids = (int*)malloc((size_t)n * sizeof(int));
    if (c->output_ids) {
        memcpy(c->output_ids, ids, (size_t)n * sizeof(int));
    }
}

/* ===================================================================
 * Circuit Evaluation (memoized, recursive)
 * =================================================================== */

static int eval(BooleanCircuit* c, int id, const int* in) {
    Gate* g = &c->gates[id];
    if (g->value >= 0) return g->value;  /* already computed */

    int v;
    switch (g->type) {
    case INPUT:  v = in[id]; break;
    case CONST0: v = 0; break;
    case CONST1: v = 1; break;
    case NOT:    v = !eval(c, g->input1, in); break;
    case AND:    v = eval(c, g->input1, in) && eval(c, g->input2, in); break;
    case OR:     v = eval(c, g->input1, in) || eval(c, g->input2, in); break;
    case XOR:    v = eval(c, g->input1, in) ^ eval(c, g->input2, in); break;
    case NAND:   v = !(eval(c, g->input1, in) && eval(c, g->input2, in)); break;
    case NOR:    v = !(eval(c, g->input1, in) || eval(c, g->input2, in)); break;
    case MAJ: {
        int cnt = 0;
        for (int i = 0; i < g->fanin; i++)
            cnt += eval(c, g->fanin_ids[i], in);
        v = (cnt > g->fanin / 2) ? 1 : 0;
        break;
    }
    case THR: {
        int cnt = 0;
        for (int i = 0; i < g->fanin; i++)
            cnt += eval(c, g->fanin_ids[i], in);
        /* Threshold parameter stored in input1 */
        v = (cnt >= g->input1) ? 1 : 0;
        break;
    }
    default: v = 0; break;
    }
    g->value = v;
    return v;
}

int circuit_evaluate(BooleanCircuit* c, const int* inputs) {
    assert(c != NULL);
    assert(inputs != NULL);
    circuit_reset(c);
    int result = 1;
    for (int i = 0; i < c->n_outputs; i++)
        result &= eval(c, c->output_ids[i], inputs);
    return result;
}

int* circuit_evaluate_all(BooleanCircuit* c, const int* inputs, int* out_count) {
    assert(c != NULL);
    assert(inputs != NULL);
    assert(out_count != NULL);
    circuit_reset(c);
    int* results = (int*)malloc((size_t)c->n_outputs * sizeof(int));
    if (!results) { *out_count = 0; return NULL; }
    for (int i = 0; i < c->n_outputs; i++)
        results[i] = eval(c, c->output_ids[i], inputs);
    *out_count = c->n_outputs;
    return results;
}

void circuit_reset(BooleanCircuit* c) {
    assert(c != NULL);
    for (int i = 0; i < c->n_gates; i++)
        c->gates[i].value = -1;
}

int circuit_active_gates(BooleanCircuit* c, const int* inputs) {
    assert(c != NULL);
    assert(inputs != NULL);
    circuit_reset(c);
    for (int i = 0; i < c->n_outputs; i++)
        eval(c, c->output_ids[i], inputs);
    int count = 0;
    for (int i = 0; i < c->n_gates; i++)
        if (c->gates[i].value >= 0) count++;
    return count;
}

/* ===================================================================
 * Topological Order Evaluation
 * =================================================================== */

void circuit_topological_order(const BooleanCircuit* c, int* order) {
    assert(c != NULL);
    assert(order != NULL);
    int n = c->n_gates;
    int* visited = (int*)calloc((size_t)n, sizeof(int));
    int* indeg   = (int*)calloc((size_t)n, sizeof(int));
    assert(visited && indeg);

    /* Compute indegree */
    for (int i = 0; i < n; i++) {
        const Gate* g = &c->gates[i];
        if (g->type == INPUT || g->type == CONST0 || g->type == CONST1) continue;
        if (g->type == MAJ || g->type == THR) {
            indeg[i] = g->fanin;
        } else {
            if (g->input1 >= 0) indeg[i]++;
            if (g->input2 >= 0) indeg[i]++;
        }
    }

    /* Kahn's algorithm */
    int pos = 0;
    /* First, enqueue all zero-indegree gates (inputs/constants) */
    for (int i = 0; i < n; i++) {
        if (c->gates[i].type == INPUT ||
            c->gates[i].type == CONST0 ||
            c->gates[i].type == CONST1) {
            if (!visited[i]) {
                order[pos++] = i;
                visited[i] = 1;
            }
        }
    }

    while (pos < n) {
        int found = 0;
        for (int i = 0; i < n; i++) {
            if (visited[i]) continue;
            const Gate* g = &c->gates[i];
            int ready = 1;
            if (g->type == MAJ || g->type == THR) {
                for (int j = 0; j < g->fanin; j++) {
                    if (!visited[g->fanin_ids[j]]) { ready = 0; break; }
                }
            } else {
                if (g->input1 >= 0 && !visited[g->input1]) ready = 0;
                if (g->input2 >= 0 && !visited[g->input2]) ready = 0;
            }
            if (ready) {
                order[pos++] = i;
                visited[i] = 1;
                found = 1;
            }
        }
        if (!found) break;  /* cycle detected */
    }
    free(visited);
    free(indeg);
}

int circuit_evaluate_topological(BooleanCircuit* c, const int* inputs) {
    assert(c != NULL);
    assert(inputs != NULL);
    int n = c->n_gates;
    int* order = (int*)malloc((size_t)n * sizeof(int));
    assert(order);
    circuit_topological_order(c, order);

    /* Evaluate in topological order */
    int* vals = (int*)calloc((size_t)n, sizeof(int));
    assert(vals);
    for (int i = 0; i < n; i++) {
        int id = order[i];
        Gate* g = &c->gates[id];
        switch (g->type) {
        case INPUT:  vals[id] = inputs[id]; break;
        case CONST0: vals[id] = 0; break;
        case CONST1: vals[id] = 1; break;
        case NOT:    vals[id] = !vals[g->input1]; break;
        case AND:    vals[id] = vals[g->input1] && vals[g->input2]; break;
        case OR:     vals[id] = vals[g->input1] || vals[g->input2]; break;
        case XOR:    vals[id] = vals[g->input1] ^ vals[g->input2]; break;
        case NAND:   vals[id] = !(vals[g->input1] && vals[g->input2]); break;
        case NOR:    vals[id] = !(vals[g->input1] || vals[g->input2]); break;
        case MAJ: {
            int cnt = 0;
            for (int j = 0; j < g->fanin; j++) cnt += vals[g->fanin_ids[j]];
            vals[id] = (cnt > g->fanin / 2) ? 1 : 0;
            break;
        }
        case THR: {
            int cnt = 0;
            for (int j = 0; j < g->fanin; j++) cnt += vals[g->fanin_ids[j]];
            vals[id] = (cnt >= g->input1) ? 1 : 0;
            break;
        }
        default: vals[id] = 0; break;
        }
    }

    int result = 1;
    for (int i = 0; i < c->n_outputs; i++)
        result &= vals[c->output_ids[i]];

    free(order);
    free(vals);
    return result;
}

/* ===================================================================
 * Depth Computation
 * =================================================================== */

static int depth_recursive(const BooleanCircuit* c, int id, int* depth_memo) {
    if (depth_memo[id] >= 0) return depth_memo[id];
    const Gate* g = &c->gates[id];
    if (g->type == INPUT || g->type == CONST0 || g->type == CONST1) {
        depth_memo[id] = 0;
        return 0;
    }
    int max_child = 0;
    if (g->type == MAJ || g->type == THR) {
        for (int j = 0; j < g->fanin; j++) {
            int cd = depth_recursive(c, g->fanin_ids[j], depth_memo);
            if (cd > max_child) max_child = cd;
        }
    } else {
        if (g->input1 >= 0) {
            int cd = depth_recursive(c, g->input1, depth_memo);
            if (cd > max_child) max_child = cd;
        }
        if (g->input2 >= 0) {
            int cd = depth_recursive(c, g->input2, depth_memo);
            if (cd > max_child) max_child = cd;
        }
    }
    depth_memo[id] = max_child + 1;
    return depth_memo[id];
}

int circuit_depth(const BooleanCircuit* c) {
    assert(c != NULL);
    if (c->depth_cache >= 0) return c->depth_cache;
    int n = c->n_gates;
    int* depth_memo = (int*)malloc((size_t)n * sizeof(int));
    assert(depth_memo);
    for (int i = 0; i < n; i++) depth_memo[i] = -1;

    int max_depth = 0;
    for (int i = 0; i < c->n_outputs; i++) {
        int d = depth_recursive(c, c->output_ids[i], depth_memo);
        if (d > max_depth) max_depth = d;
    }
    free(depth_memo);
    /* Cast away const to cache — this is the only mutation of depth_cache */
    ((BooleanCircuit*)c)->depth_cache = max_depth;
    return max_depth;
}

/* ===================================================================
 * Size, Width, and Basic Queries
 * =================================================================== */

int circuit_size(const BooleanCircuit* c) {
    assert(c != NULL);
    return c->n_gates;
}

int circuit_input_count(const BooleanCircuit* c) {
    assert(c != NULL);
    return c->n_inputs;
}

int circuit_output_count(const BooleanCircuit* c) {
    assert(c != NULL);
    return c->n_outputs;
}

int circuit_width(const BooleanCircuit* c) {
    assert(c != NULL);
    int n = c->n_gates;
    int* levels = (int*)calloc((size_t)(n + 1), sizeof(int));
    int* level_of = (int*)malloc((size_t)n * sizeof(int));
    assert(levels && level_of);
    for (int i = 0; i < n; i++) level_of[i] = -1;

    /* Compute level of each gate via DP */
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
            int max_parent = -1;
            int all_ready = 1;
            if (g->type == MAJ || g->type == THR) {
                for (int j = 0; j < g->fanin; j++) {
                    if (level_of[g->fanin_ids[j]] < 0) { all_ready = 0; break; }
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
                level_of[i] = max_parent + 1;
                changed = 1;
            }
        }
    }

    for (int i = 0; i < n; i++)
        if (level_of[i] >= 0) levels[level_of[i]]++;

    int max_width = 0;
    for (int i = 0; i <= n; i++)
        if (levels[i] > max_width) max_width = levels[i];

    free(levels);
    free(level_of);
    return max_width;
}

const Gate* circuit_get_gate(const BooleanCircuit* c, int id) {
    assert(c != NULL);
    if (id < 0 || id >= c->n_gates) return NULL;
    return &c->gates[id];
}

int circuit_edge_count(const BooleanCircuit* c) {
    assert(c != NULL);
    return c->n_edges;
}

/* ===================================================================
 * Validation
 * =================================================================== */

int circuit_is_dag(const BooleanCircuit* c) {
    assert(c != NULL);
    int n = c->n_gates;
    int* order = (int*)malloc((size_t)n * sizeof(int));
    assert(order);
    /* Topological sort covers all gates iff DAG */
    int* visited = (int*)calloc((size_t)n, sizeof(int));
    assert(visited);
    int pos = 0;

    /* Initialize with leaf gates */
    for (int i = 0; i < n; i++) {
        if (c->gates[i].type == INPUT ||
            c->gates[i].type == CONST0 ||
            c->gates[i].type == CONST1) {
            order[pos++] = i;
            visited[i] = 1;
        }
    }

    while (pos < n) {
        int found = 0;
        for (int i = 0; i < n; i++) {
            if (visited[i]) continue;
            int ready = 1;
            const Gate* g = &c->gates[i];
            if (g->type == MAJ || g->type == THR) {
                for (int j = 0; j < g->fanin; j++)
                    if (!visited[g->fanin_ids[j]]) { ready = 0; break; }
            } else {
                if (g->input1 >= 0 && !visited[g->input1]) ready = 0;
                if (g->input2 >= 0 && !visited[g->input2]) ready = 0;
            }
            if (ready) { order[pos++] = i; visited[i] = 1; found = 1; }
        }
        if (!found) break;
    }

    int is_dag = (pos == n);
    free(order);
    free(visited);
    return is_dag;
}

int circuit_is_valid(const BooleanCircuit* c) {
    assert(c != NULL);
    /* Check output gate references */
    for (int i = 0; i < c->n_outputs; i++) {
        if (c->output_ids[i] < 0 || c->output_ids[i] >= c->n_gates)
            return 0;
    }
    /* Check gate references are in range */
    for (int i = 0; i < c->n_gates; i++) {
        const Gate* g = &c->gates[i];
        if (g->type == INPUT || g->type == CONST0 || g->type == CONST1)
            continue;
        if (g->type == MAJ || g->type == THR) {
            for (int j = 0; j < g->fanin; j++) {
                if (g->fanin_ids[j] < 0 || g->fanin_ids[j] >= c->n_gates)
                    return 0;
            }
        } else {
            if (g->input1 < -1 || g->input1 >= c->n_gates) return 0;
            if (g->input2 < -1 || g->input2 >= c->n_gates) return 0;
        }
    }
    /* Check DAG property */
    if (!circuit_is_dag(c)) return 0;
    return 1;
}

/* ===================================================================
 * Printing
 * =================================================================== */

void circuit_print(const BooleanCircuit* c) {
    if (!c) { printf("Circuit: NULL\n"); return; }
    printf("Circuit: %d gates, %d inputs, %d outputs, depth=%d, width=%d\n",
           c->n_gates, c->n_inputs, c->n_outputs,
           circuit_depth(c), circuit_width(c));
}

void circuit_print_detailed(const BooleanCircuit* c) {
    if (!c) return;
    circuit_print(c);
    printf("Basis: %s\n", c->basis == 0 ? "standard {AND,OR,NOT}" :
                           c->basis == 1 ? "NAND-only" : "NOR-only");
    printf("Edges: %d\n", c->n_edges);
    printf("Gates:\n");
    const char* type_names[] = {
        "INPUT", "AND", "OR", "NOT", "CONST0", "CONST1",
        "XOR", "NAND", "NOR", "MAJ", "THR"
    };
    for (int i = 0; i < c->n_gates; i++) {
        const Gate* g = &c->gates[i];
        printf("  [%3d] %-7s", i, type_names[g->type]);
        if (g->type == MAJ || g->type == THR) {
            printf(" fanin=%d [", g->fanin);
            for (int j = 0; j < g->fanin; j++)
                printf("%d%s", g->fanin_ids[j],
                       j < g->fanin - 1 ? "," : "");
            printf("]");
        } else {
            if (g->input1 >= 0) printf(" in1=%d", g->input1);
            if (g->input2 >= 0) printf(" in2=%d", g->input2);
        }
        printf(" val=%d lvl=%d\n", g->value, g->level);
    }
    printf("Outputs: [");
    for (int i = 0; i < c->n_outputs; i++)
        printf("%d%s", c->output_ids[i],
               i < c->n_outputs - 1 ? ", " : "");
    printf("]\n");
}

/* ===================================================================
 * Benchmarking
 * =================================================================== */

double circuit_bench_eval(BooleanCircuit* c, const int* inputs,
                           int iterations) {
    assert(c != NULL);
    assert(inputs != NULL);
    assert(iterations > 0);
    clock_t start = clock();
    for (int r = 0; r < iterations; r++) {
        circuit_reset(c);
        for (int i = 0; i < c->n_outputs; i++)
            eval(c, c->output_ids[i], inputs);
    }
    clock_t end = clock();
    double elapsed = (double)(end - start) / CLOCKS_PER_SEC;
    return elapsed / iterations * 1e6;  /* microseconds per eval */
}

/* ===================================================================
 * I/O and Serialization
 * =================================================================== */

void circuit_export_dot(const BooleanCircuit* c, const char* filename) {
    assert(c != NULL);
    assert(filename != NULL);
    FILE* f = fopen(filename, "w");
    if (!f) return;
    fprintf(f, "digraph BooleanCircuit {\n");
    fprintf(f, "  rankdir=TB;\n");
    fprintf(f, "  node [fontname=\"Courier\"];\n");

    const char* shapes[] = {
        "box",        /* INPUT */
        "ellipse",    /* AND */
        "ellipse",    /* OR */
        "invtriangle",/* NOT */
        "box",        /* CONST0 */
        "box",        /* CONST1 */
        "ellipse",    /* XOR */
        "ellipse",    /* NAND */
        "ellipse",    /* NOR */
        "trapezium",  /* MAJ */
        "trapezium"   /* THR */
    };
    const char* colors[] = {
        "blue", "red", "green", "purple", "gray", "gray",
        "orange", "pink", "cyan", "brown", "brown"
    };
    const char* labels[] = {
        "IN", "&", "|", "~", "0", "1", "^", "NAND", "NOR", "MAJ", "THR"
    };

    for (int i = 0; i < c->n_gates; i++) {
        const Gate* g = &c->gates[i];
        fprintf(f, "  n%d [shape=%s, color=%s, label=\"%s%d\"];\n",
                i, shapes[g->type], colors[g->type], labels[g->type], i);
    }
    for (int i = 0; i < c->n_gates; i++) {
        const Gate* g = &c->gates[i];
        if (g->type == INPUT || g->type == CONST0 || g->type == CONST1)
            continue;
        if (g->type == MAJ || g->type == THR) {
            for (int j = 0; j < g->fanin; j++)
                fprintf(f, "  n%d -> n%d;\n", g->fanin_ids[j], i);
        } else {
            if (g->input1 >= 0) fprintf(f, "  n%d -> n%d;\n", g->input1, i);
            if (g->input2 >= 0) fprintf(f, "  n%d -> n%d;\n", g->input2, i);
        }
    }
    for (int i = 0; i < c->n_outputs; i++)
        fprintf(f, "  n%d [shape=doublecircle, color=black];\n",
                c->output_ids[i]);

    fprintf(f, "}\n");
    fclose(f);
}

void circuit_export_stats(const BooleanCircuit* c, const char* filename) {
    assert(c != NULL);
    assert(filename != NULL);
    FILE* f = fopen(filename, "w");
    if (!f) return;

    int type_cnt[11] = {0};
    for (int i = 0; i < c->n_gates; i++)
        type_cnt[c->gates[i].type]++;

    fprintf(f, "circuit_stats:\n");
    fprintf(f, "  gates: %d\n", c->n_gates);
    fprintf(f, "  inputs: %d\n", c->n_inputs);
    fprintf(f, "  outputs: %d\n", c->n_outputs);
    fprintf(f, "  depth: %d\n", circuit_depth(c));
    fprintf(f, "  width: %d\n", circuit_width(c));
    fprintf(f, "  edges: %d\n", c->n_edges);
    fprintf(f, "  basis: %s\n",
            c->basis == 0 ? "standard" :
            c->basis == 1 ? "nand" : "nor");
    fprintf(f, "  density: %.6f\n", circuit_density(c));
    fprintf(f, "  complexity_product: %.0f\n",
            circuit_complexity_product(c));
    fprintf(f, "  gate_distribution:\n");
    const char* tn[] = {
        "INPUT", "AND", "OR", "NOT", "CONST0", "CONST1",
        "XOR", "NAND", "NOR", "MAJ", "THR"
    };
    for (int i = 0; i < 11; i++)
        if (type_cnt[i] > 0)
            fprintf(f, "    %s: %d\n", tn[i], type_cnt[i]);
    fclose(f);
}

BooleanCircuit* circuit_import_text(const char* filename) {
    assert(filename != NULL);
    FILE* f = fopen(filename, "r");
    if (!f) return NULL;

    /* First pass: count gates */
    char line[256];
    int ng = 0;
    while (fgets(line, (int)sizeof(line), f)) {
        char ch = line[0];
        if (ch == 'I' || ch == 'A' || ch == 'O' || ch == 'N' ||
            ch == 'X' || ch == '0' || ch == '1' || ch == 'M' ||
            ch == 'T')
            ng++;
    }
    if (ng == 0) { fclose(f); return NULL; }
    rewind(f);

    BooleanCircuit* c = circuit_create(ng);
    int* gate_ids = (int*)malloc((size_t)ng * sizeof(int));
    int gi = 0;

    while (fgets(line, (int)sizeof(line), f)) {
        switch (line[0]) {
        case 'I':
            gate_ids[gi++] = circuit_add_gate(c, INPUT, -1, -1);
            break;
        case '0':
            gate_ids[gi++] = circuit_add_gate(c, CONST0, -1, -1);
            break;
        case '1':
            gate_ids[gi++] = circuit_add_gate(c, CONST1, -1, -1);
            break;
        case 'A':
            if (gi >= 2)
                gate_ids[gi] = circuit_add_gate(c, AND,
                    gate_ids[gi - 1], gate_ids[gi - 2]);
            gi++;
            break;
        case 'O':
            if (gi >= 2)
                gate_ids[gi] = circuit_add_gate(c, OR,
                    gate_ids[gi - 1], gate_ids[gi - 2]);
            gi++;
            break;
        case 'N':
            if (gi >= 1)
                gate_ids[gi] = circuit_add_gate(c, NOT,
                    gate_ids[gi - 1], -1);
            gi++;
            break;
        case 'X':
            if (gi >= 2)
                gate_ids[gi] = circuit_add_gate(c, XOR,
                    gate_ids[gi - 1], gate_ids[gi - 2]);
            gi++;
            break;
        default: break;
        }
    }
    if (gi > 0) {
        int out = gate_ids[gi - 1];
        circuit_set_output(c, &out, 1);
    }
    free(gate_ids);
    fclose(f);
    return c;
}

/* Serialization format:
 *   Header: 4 bytes magic "BCKT" + 4 bytes version + 4 bytes n_gates
 *            + 4 bytes n_inputs + 4 bytes n_outputs + 4 bytes n_edges
 *   Gates:  n_gates * (4 bytes type + 4 bytes input1 + 4 bytes input2
 *                       + 4 bytes fanin + fanin * 4 bytes fanin_ids)
 *   Outputs: n_outputs * 4 bytes
 *
 * All integers are little-endian 32-bit.
 */

uint8_t* circuit_serialize(const BooleanCircuit* c, size_t* size) {
    assert(c != NULL);
    assert(size != NULL);

    /* Calculate size */
    size_t gates_data_size = 0;
    for (int i = 0; i < c->n_gates; i++) {
        const Gate* g = &c->gates[i];
        gates_data_size += 16;  /* type, in1, in2, fanin */
        if (g->type == MAJ || g->type == THR)
            gates_data_size += (size_t)g->fanin * 4;
    }
    size_t total = 24 + gates_data_size + (size_t)c->n_outputs * 4;
    uint8_t* buf = (uint8_t*)malloc(total);
    if (!buf) { *size = 0; return NULL; }
    memset(buf, 0, total);

    size_t pos = 0;
    /* Magic "BCKT" */
    memcpy(buf + pos, "BCKT", 4); pos += 4;
    /* Version 1 */
    *(int32_t*)(buf + pos) = 1; pos += 4;
    /* Counts */
    *(int32_t*)(buf + pos) = c->n_gates;   pos += 4;
    *(int32_t*)(buf + pos) = c->n_inputs;  pos += 4;
    *(int32_t*)(buf + pos) = c->n_outputs; pos += 4;
    *(int32_t*)(buf + pos) = c->n_edges;   pos += 4;

    /* Gates */
    for (int i = 0; i < c->n_gates; i++) {
        const Gate* g = &c->gates[i];
        *(int32_t*)(buf + pos) = (int)g->type; pos += 4;
        *(int32_t*)(buf + pos) = g->input1;    pos += 4;
        *(int32_t*)(buf + pos) = g->input2;    pos += 4;
        *(int32_t*)(buf + pos) = g->fanin;     pos += 4;
        if (g->type == MAJ || g->type == THR) {
            for (int j = 0; j < g->fanin; j++) {
                *(int32_t*)(buf + pos) = g->fanin_ids[j];
                pos += 4;
            }
        }
    }

    /* Outputs */
    for (int i = 0; i < c->n_outputs; i++) {
        *(int32_t*)(buf + pos) = c->output_ids[i];
        pos += 4;
    }

    *size = total;
    return buf;
}

BooleanCircuit* circuit_deserialize(const uint8_t* data, size_t size) {
    assert(data != NULL);
    if (size < 24) return NULL;
    if (memcmp(data, "BCKT", 4) != 0) return NULL;

    size_t pos = 4;
    int version = *(int32_t*)(data + pos); pos += 4;
    if (version != 1) return NULL;
    int n_gates   = *(int32_t*)(data + pos); pos += 4;
    int n_inputs  = *(int32_t*)(data + pos); pos += 4;
    int n_outputs = *(int32_t*)(data + pos); pos += 4;
    int n_edges   = *(int32_t*)(data + pos); pos += 4;

    BooleanCircuit* c = circuit_create(n_gates + 10);
    if (!c) return NULL;

    for (int i = 0; i < n_gates; i++) {
        if (pos + 16 > size) { circuit_free(c); return NULL; }
        GateType type = (GateType)(*(int32_t*)(data + pos)); pos += 4;
        int in1 = *(int32_t*)(data + pos); pos += 4;
        int in2 = *(int32_t*)(data + pos); pos += 4;
        int fan = *(int32_t*)(data + pos); pos += 4;

        int gid;
        if (type == MAJ || type == THR) {
            if (pos + (size_t)fan * 4 > size) {
                circuit_free(c); return NULL;
            }
            int* fids = (int*)malloc((size_t)fan * sizeof(int));
            for (int j = 0; j < fan; j++) {
                fids[j] = *(int32_t*)(data + pos); pos += 4;
            }
            gid = circuit_add_maj_gate(c, type, fan, fids);
            free(fids);
        } else {
            gid = circuit_add_gate(c, type, in1, in2);
        }
    }

    if (pos + (size_t)n_outputs * 4 > size) {
        circuit_free(c); return NULL;
    }
    int* outs = (int*)malloc((size_t)n_outputs * sizeof(int));
    for (int i = 0; i < n_outputs; i++) {
        outs[i] = *(int32_t*)(data + pos); pos += 4;
    }
    circuit_set_output(c, outs, n_outputs);
    free(outs);

    c->n_inputs = n_inputs;
    c->n_edges  = n_edges;
    return c;
}
