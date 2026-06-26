/*============================================================================
 * rs_core.c - Circuit Model Implementation (Razborov-Smolensky)
 *
 * Implements AC0 and AC0[p] circuit models: construction, evaluation,
 * and analysis. These circuits are constant-depth, unbounded-fanin,
 * polynomial-size Boolean circuits with AND, OR, NOT, and (for AC0[p])
 * MOD_p gates.
 *
 * L1: Circuit definitions, L2: Circuit complexity classes,
 * L3: Boolean circuit as DAG, L5: Circuit construction algorithms,
 * L6: Canonical problem circuits (MAJORITY, PARITY, MOD_p).
 *
 * References:
 *   Vollmer, "Introduction to Circuit Complexity" (1999)
 *   Arora & Barak, "Computational Complexity", Ch.6, Ch.14
 *==========================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include "rs_core.h"

/* =========================================================================
 * Canonical Boolean Functions (L6)
 * ========================================================================= */

int fn_majority(const int* x, int n) {
    int sum = 0;
    for (int i = 0; i < n; i++) sum += x[i];
    return sum > n / 2;
}

int fn_parity(const int* x, int n) {
    int p = 0;
    for (int i = 0; i < n; i++) p ^= x[i];
    return p;
}

int fn_modp(const int* x, int n, int p) {
    int sum = 0;
    for (int i = 0; i < n; i++) sum += x[i];
    return (sum % p == 0) ? 1 : 0;
}

int fn_threshold(const int* x, int n, int k) {
    int sum = 0;
    for (int i = 0; i < n; i++) sum += x[i];
    return sum >= k;
}

int fn_and_k(const int* x, int n, int k) {
    for (int i = 0; i < k && i < n; i++)
        if (x[i] == 0) return 0;
    return 1;
}

int fn_or_k(const int* x, int n, int k) {
    for (int i = 0; i < k && i < n; i++)
        if (x[i] == 1) return 1;
    return 0;
}

/* =========================================================================
 * Circuit Lifecycle (L5)
 * ========================================================================= */

#define RSC_INIT_CAP 256

RSCircuit* rs_circuit_create(int n_inputs) {
    RSCircuit* C = malloc(sizeof(RSCircuit));
    if (!C) { fprintf(stderr, "rs_circuit_create: OOM\n"); exit(1); }
    C->n_inputs    = n_inputs;
    C->n_gates     = 0;
    C->capacity    = RSC_INIT_CAP;
    C->output_id   = -1;
    C->depth       = 0;
    C->size        = 0;
    C->is_constant = 0;
    C->has_modp    = 0;
    C->gates = malloc((size_t)C->capacity * sizeof(RSGate*));
    if (!C->gates) { fprintf(stderr, "rs_circuit_create: OOM\n"); exit(1); }
    return C;
}

void rs_circuit_free(RSCircuit* C) {
    if (!C) return;
    for (int i = 0; i < C->n_gates; i++) {
        free(C->gates[i]->inputs);
        free(C->gates[i]);
    }
    free(C->gates);
    free(C);
}

RSCircuit* rs_circuit_copy(const RSCircuit* C) {
    RSCircuit* D = rs_circuit_create(C->n_inputs);
    for (int i = 0; i < C->n_gates; i++) {
        RSGate* g = C->gates[i];
        switch (g->type) {
            case GATE_INPUT:
                rs_circuit_add_input(D, g->input_var);
                break;
            case GATE_CONST0:
                rs_circuit_add_constant(D, 0);
                break;
            case GATE_CONST1:
                rs_circuit_add_constant(D, 1);
                break;
            case GATE_NOT:
                rs_circuit_add_not(D, g->inputs[0]);
                break;
            case GATE_AND:
                rs_circuit_add_and(D, g->inputs, g->n_inputs);
                break;
            case GATE_OR:
                rs_circuit_add_or(D, g->inputs, g->n_inputs);
                break;
            case GATE_MODP:
                rs_circuit_add_modp(D, g->modp, g->inputs, g->n_inputs);
                break;
            default:
                break;
        }
    }
    if (C->output_id >= 0)
        rs_circuit_set_output(D, C->output_id);
    return D;
}

/* =========================================================================
 * Gate Construction (L5)
 * ========================================================================= */

static void rs_circuit_ensure_capacity(RSCircuit* C) {
    if (C->n_gates < C->capacity) return;
    C->capacity *= 2;
    C->gates = realloc(C->gates, (size_t)C->capacity * sizeof(RSGate*));
    if (!C->gates) { fprintf(stderr, "rs_circuit_ensure_capacity: OOM\n"); exit(1); }
}

static RSGate* rs_gate_make(void) {
    RSGate* g = malloc(sizeof(RSGate));
    if (!g) { fprintf(stderr, "rs_gate_make: OOM\n"); exit(1); }
    g->type    = GATE_INPUT;
    g->id      = -1;
    g->modp    = 0;
    g->n_inputs = 0;
    g->inputs  = NULL;
    g->input_var = -1;
    g->depth   = 0;
    g->value   = -1;
    return g;
}

int rs_circuit_add_input(RSCircuit* C, int var_idx) {
    rs_circuit_ensure_capacity(C);
    RSGate* g = rs_gate_make();
    g->type      = GATE_INPUT;
    g->id        = C->n_gates;
    g->input_var = var_idx;
    g->depth     = 0;
    C->gates[C->n_gates] = g;
    C->n_gates++;
    return g->id;
}

int rs_circuit_add_constant(RSCircuit* C, int value) {
    rs_circuit_ensure_capacity(C);
    RSGate* g = rs_gate_make();
    g->type  = value ? GATE_CONST1 : GATE_CONST0;
    g->id    = C->n_gates;
    g->depth = 0;
    C->gates[C->n_gates] = g;
    C->n_gates++;
    C->is_constant = 1;
    return g->id;
}

int rs_circuit_add_not(RSCircuit* C, int input_id) {
    if (input_id < 0 || input_id >= C->n_gates) return -1;
    rs_circuit_ensure_capacity(C);
    RSGate* g = rs_gate_make();
    g->type     = GATE_NOT;
    g->id       = C->n_gates;
    g->n_inputs = 1;
    g->inputs   = malloc(sizeof(int));
    if (!g->inputs) { fprintf(stderr, "add_not: OOM\n"); exit(1); }
    g->inputs[0] = input_id;
    g->depth    = C->gates[input_id]->depth + 1;
    if (g->depth > C->depth) C->depth = g->depth;
    C->gates[C->n_gates] = g;
    C->n_gates++;
    C->size++;
    return g->id;
}

int rs_circuit_add_and(RSCircuit* C, const int* input_ids, int n_in) {
    rs_circuit_ensure_capacity(C);
    RSGate* g = rs_gate_make();
    g->type     = GATE_AND;
    g->id       = C->n_gates;
    g->n_inputs = n_in;
    g->inputs   = malloc((size_t)n_in * sizeof(int));
    if (!g->inputs) { fprintf(stderr, "add_and: OOM\n"); exit(1); }
    int max_d = 0;
    for (int i = 0; i < n_in; i++) {
        if (input_ids[i] < 0 || input_ids[i] >= C->n_gates) return -1;
        g->inputs[i] = input_ids[i];
        int d = C->gates[input_ids[i]]->depth;
        if (d > max_d) max_d = d;
    }
    g->depth = max_d + 1;
    if (g->depth > C->depth) C->depth = g->depth;
    C->gates[C->n_gates] = g;
    C->n_gates++;
    C->size++;
    return g->id;
}

int rs_circuit_add_or(RSCircuit* C, const int* input_ids, int n_in) {
    rs_circuit_ensure_capacity(C);
    RSGate* g = rs_gate_make();
    g->type     = GATE_OR;
    g->id       = C->n_gates;
    g->n_inputs = n_in;
    g->inputs   = malloc((size_t)n_in * sizeof(int));
    if (!g->inputs) { fprintf(stderr, "add_or: OOM\n"); exit(1); }
    int max_d = 0;
    for (int i = 0; i < n_in; i++) {
        if (input_ids[i] < 0 || input_ids[i] >= C->n_gates) return -1;
        g->inputs[i] = input_ids[i];
        int d = C->gates[input_ids[i]]->depth;
        if (d > max_d) max_d = d;
    }
    g->depth = max_d + 1;
    if (g->depth > C->depth) C->depth = g->depth;
    C->gates[C->n_gates] = g;
    C->n_gates++;
    C->size++;
    return g->id;
}

int rs_circuit_add_modp(RSCircuit* C, int p, const int* input_ids, int n_in) {
    rs_circuit_ensure_capacity(C);
    RSGate* g = rs_gate_make();
    g->type     = GATE_MODP;
    g->id       = C->n_gates;
    g->modp     = p;
    g->n_inputs = n_in;
    g->inputs   = malloc((size_t)n_in * sizeof(int));
    if (!g->inputs) { fprintf(stderr, "add_modp: OOM\n"); exit(1); }
    int max_d = 0;
    for (int i = 0; i < n_in; i++) {
        if (input_ids[i] < 0 || input_ids[i] >= C->n_gates) return -1;
        g->inputs[i] = input_ids[i];
        int d = C->gates[input_ids[i]]->depth;
        if (d > max_d) max_d = d;
    }
    g->depth = max_d + 1;
    if (g->depth > C->depth) C->depth = g->depth;
    C->gates[C->n_gates] = g;
    C->n_gates++;
    C->size++;
    C->has_modp = 1;
    return g->id;
}

void rs_circuit_set_output(RSCircuit* C, int gate_id) {
    if (gate_id >= 0 && gate_id < C->n_gates)
        C->output_id = gate_id;
}

/* =========================================================================
 * Circuit Evaluation (L5)
 * ========================================================================= */

int rs_circuit_eval(const RSCircuit* C, const int* x) {
    /* Evaluate circuit on input x (bottom-up) */
    if (C->output_id < 0) return 0;
    int* vals = malloc((size_t)C->n_gates * sizeof(int));
    if (!vals) { fprintf(stderr, "rs_circuit_eval: OOM\n"); exit(1); }

    /* Evaluate in order of increasing depth (topological order;
     * since gates are added in topological order, we can iterate
     * sequentially). */
    for (int i = 0; i < C->n_gates; i++) {
        RSGate* g = C->gates[i];
        switch (g->type) {
            case GATE_INPUT:
                vals[i] = (g->input_var >= 0 && g->input_var < C->n_inputs)
                           ? x[g->input_var] : 0;
                break;
            case GATE_CONST0:
                vals[i] = 0;
                break;
            case GATE_CONST1:
                vals[i] = 1;
                break;
            case GATE_NOT:
                vals[i] = (vals[g->inputs[0]] == 0) ? 1 : 0;
                break;
            case GATE_AND: {
                int v = 1;
                for (int j = 0; j < g->n_inputs; j++)
                    if (vals[g->inputs[j]] == 0) { v = 0; break; }
                vals[i] = v;
                break;
            }
            case GATE_OR: {
                int v = 0;
                for (int j = 0; j < g->n_inputs; j++)
                    if (vals[g->inputs[j]] == 1) { v = 1; break; }
                vals[i] = v;
                break;
            }
            case GATE_MODP: {
                int sum = 0;
                for (int j = 0; j < g->n_inputs; j++)
                    sum += vals[g->inputs[j]];
                vals[i] = (sum % g->modp == 0) ? 1 : 0;
                break;
            }
            default:
                vals[i] = 0;
                break;
        }
        g->value = vals[i];  /* cache value */
    }

    int result = vals[C->output_id];
    free(vals);
    return result;
}

void rs_circuit_reeval(RSCircuit* C, const int* x) {
    /* Re-evaluate and update cached gate values */
    rs_circuit_eval(C, x);
}

/* =========================================================================
 * Topology Queries (L5)
 * ========================================================================= */

int rs_circuit_depth(const RSCircuit* C) {
    return C->depth;
}

int rs_circuit_n_gates(const RSCircuit* C) {
    return C->n_gates;
}

int rs_circuit_size(const RSCircuit* C) {
    return (int)C->size;
}

int rs_circuit_fanin_max(const RSCircuit* C) {
    int mf = 0;
    for (int i = 0; i < C->n_gates; i++)
        if (C->gates[i]->n_inputs > mf)
            mf = C->gates[i]->n_inputs;
    return mf;
}

void rs_circuit_print(const RSCircuit* C) {
    printf("Circuit: %d inputs, %d gates, depth=%d, size=%lld\n",
           C->n_inputs, C->n_gates, C->depth, (long long)C->size);
    for (int i = 0; i < C->n_gates; i++) {
        RSGate* g = C->gates[i];
        const char* tname = "?";
        switch (g->type) {
            case GATE_INPUT:   tname = "INPUT"; break;
            case GATE_AND:     tname = "AND"; break;
            case GATE_OR:      tname = "OR"; break;
            case GATE_NOT:     tname = "NOT"; break;
            case GATE_MODP:    tname = "MOD"; break;
            case GATE_CONST0:  tname = "CONST0"; break;
            case GATE_CONST1:  tname = "CONST1"; break;
            case GATE_OUTPUT:  tname = "OUTPUT"; break;
        }
        printf("  [%d] %s depth=%d", i, tname, g->depth);
        if (g->type == GATE_INPUT)
            printf(" var=x%d", g->input_var);
        else if (g->type == GATE_MODP)
            printf(" p=%d", g->modp);
        if (g->n_inputs > 0) {
            printf(" inputs=[");
            for (int j = 0; j < g->n_inputs && j < 8; j++)
                printf("%s%d", j > 0 ? "," : "", g->inputs[j]);
            if (g->n_inputs > 8) printf(",...");
            printf("]");
        }
        printf("\n");
    }
}

void rs_circuit_print_stats(const RSCircuit* C) {
    int n_input, n_and, n_or, n_not, n_modp;
    rs_circuit_gate_counts(C, &n_input, &n_and, &n_or, &n_not, &n_modp);
    printf("Circuit stats: n_inputs=%d  gates=%d  depth=%d  size=%lld\n",
           C->n_inputs, C->n_gates, C->depth, (long long)C->size);
    printf("  input=%d and=%d or=%d not=%d modp=%d  has_modp=%d\n",
           n_input, n_and, n_or, n_not, n_modp, C->has_modp);
}

void rs_circuit_gate_counts(const RSCircuit* C,
                            int* n_input, int* n_and, int* n_or,
                            int* n_not, int* n_modp) {
    *n_input = *n_and = *n_or = *n_not = *n_modp = 0;
    for (int i = 0; i < C->n_gates; i++) {
        switch (C->gates[i]->type) {
            case GATE_INPUT:  (*n_input)++; break;
            case GATE_AND:    (*n_and)++;   break;
            case GATE_OR:     (*n_or)++;    break;
            case GATE_NOT:    (*n_not)++;   break;
            case GATE_MODP:   (*n_modp)++;  break;
            default: break;
        }
    }
}

/* =========================================================================
 * Circuit Analysis (L5)
 * ========================================================================= */

int* rs_circuit_truth_table(const RSCircuit* C) {
    int rows = 1 << C->n_inputs;
    if (rows > (1 << 20)) {
        fprintf(stderr, "rs_circuit_truth_table: n=%d too large\n", C->n_inputs);
        return NULL;
    }
    int* tt = malloc((size_t)rows * sizeof(int));
    if (!tt) { fprintf(stderr, "rs_circuit_truth_table: OOM\n"); exit(1); }
    int* x = calloc((size_t)C->n_inputs, sizeof(int));
    if (!x) { free(tt); fprintf(stderr, "OOM\n"); exit(1); }
    for (int r = 0; r < rows; r++) {
        for (int v = 0; v < C->n_inputs; v++)
            x[v] = (r >> v) & 1;
        tt[r] = rs_circuit_eval(C, x);
    }
    free(x);
    return tt;
}

int rs_circuit_checks(const RSCircuit* C, const int* tt) {
    int rows = 1 << C->n_inputs;
    int* x = calloc((size_t)C->n_inputs, sizeof(int));
    if (!x) return 0;
    for (int r = 0; r < rows; r++) {
        for (int v = 0; v < C->n_inputs; v++)
            x[v] = (r >> v) & 1;
        if (rs_circuit_eval(C, x) != tt[r]) {
            free(x); return 0;
        }
    }
    free(x);
    return 1;
}

int rs_circuit_equivalent(const RSCircuit* A, const RSCircuit* B) {
    if (A->n_inputs != B->n_inputs) return 0;
    int rows = 1 << A->n_inputs;
    if (rows > (1 << 20)) return -1;  /* too large to check */
    int* x = calloc((size_t)A->n_inputs, sizeof(int));
    if (!x) return -1;
    for (int r = 0; r < rows; r++) {
        for (int v = 0; v < A->n_inputs; v++)
            x[v] = (r >> v) & 1;
        if (rs_circuit_eval(A, x) != rs_circuit_eval(B, x)) {
            free(x); return 0;
        }
    }
    free(x);
    return 1;
}

/* =========================================================================
 * Circuit Construction Helpers (L5)
 * ========================================================================= */

RSCircuit* rs_circuit_and_all(int n) {
    /* AND(x_0, ..., x_{n-1}) — single AND gate */
    RSCircuit* C = rs_circuit_create(n);
    int* inputs = malloc((size_t)n * sizeof(int));
    if (!inputs) { fprintf(stderr, "rs_circuit_and_all: OOM\n"); exit(1); }
    for (int i = 0; i < n; i++)
        inputs[i] = rs_circuit_add_input(C, i);
    int out = rs_circuit_add_and(C, inputs, n);
    rs_circuit_set_output(C, out);
    free(inputs);
    return C;
}

RSCircuit* rs_circuit_or_all(int n) {
    /* OR(x_0, ..., x_{n-1}) — single OR gate */
    RSCircuit* C = rs_circuit_create(n);
    int* inputs = malloc((size_t)n * sizeof(int));
    if (!inputs) { fprintf(stderr, "rs_circuit_or_all: OOM\n"); exit(1); }
    for (int i = 0; i < n; i++)
        inputs[i] = rs_circuit_add_input(C, i);
    int out = rs_circuit_add_or(C, inputs, n);
    rs_circuit_set_output(C, out);
    free(inputs);
    return C;
}

RSCircuit* rs_circuit_majority_dnf(int n) {
    /* MAJORITY via DNF: OR over all subsets of size > n/2
     * Size = sum_{k=n/2+1}^n C(n,k) = ~2^{n-1}. Exponential! */
    RSCircuit* C = rs_circuit_create(n);
    /* Add input gates */
    int* input_ids = malloc((size_t)n * sizeof(int));
    if (!input_ids) { fprintf(stderr, "majority_dnf: OOM\n"); exit(1); }
    for (int i = 0; i < n; i++)
        input_ids[i] = rs_circuit_add_input(C, i);

    /* For n up to 8, enumerate all majority subsets */
    if (n > 8) {
        /* Too large for naive DNF; create placeholder */
        int out = rs_circuit_add_constant(C, 0);
        rs_circuit_set_output(C, out);
        free(input_ids);
        return C;
    }

    int threshold = n / 2 + 1;
    int max_mask = 1 << n;
    int* and_gates = malloc((size_t)max_mask * sizeof(int));
    if (!and_gates) { fprintf(stderr, "majority_dnf: OOM\n"); exit(1); }
    int n_and = 0;

    for (int mask = 0; mask < max_mask; mask++) {
        /* Count set bits */
        int cnt = 0;
        for (int b = 0; b < n; b++)
            if (mask & (1 << b)) cnt++;
        if (cnt < threshold) continue;

        /* Build AND for this subset: x_b1 AND x_b2 AND ... */
        int* and_inputs = malloc((size_t)cnt * sizeof(int));
        if (!and_inputs) { fprintf(stderr, "majority_dnf: OOM\n"); exit(1); }
        int pos = 0;
        for (int b = 0; b < n; b++)
            if (mask & (1 << b))
                and_inputs[pos++] = input_ids[b];
        and_gates[n_and] = rs_circuit_add_and(C, and_inputs, cnt);
        n_and++;
        free(and_inputs);
    }

    /* OR of all AND terms */
    int out;
    if (n_and == 0) {
        out = rs_circuit_add_constant(C, 0);
    } else {
        out = rs_circuit_add_or(C, and_gates, n_and);
    }
    rs_circuit_set_output(C, out);
    free(and_gates);
    free(input_ids);
    return C;
}

RSCircuit* rs_circuit_parity_dnf(int n) {
    /* PARITY via DNF: sum of minterms with odd weight
     * Size = 2^{n-1}. Exponential. */
    RSCircuit* C = rs_circuit_create(n);
    int* input_ids = malloc((size_t)n * sizeof(int));
    if (!input_ids) { fprintf(stderr, "parity_dnf: OOM\n"); exit(1); }
    for (int i = 0; i < n; i++)
        input_ids[i] = rs_circuit_add_input(C, i);

    if (n > 8) {
        int out = rs_circuit_add_constant(C, 0);
        rs_circuit_set_output(C, out);
        free(input_ids);
        return C;
    }

    int max_mask = 1 << n;
    int* and_gates = malloc((size_t)max_mask * sizeof(int));
    if (!and_gates) { fprintf(stderr, "parity_dnf: OOM\n"); exit(1); }
    int n_and = 0;

    for (int mask = 0; mask < max_mask; mask++) {
        int cnt = 0;
        for (int b = 0; b < n; b++)
            if (mask & (1 << b)) cnt++;
        if (cnt % 2 == 0) continue;  /* even weight -> PARITY = 0 */

        int* and_inputs = malloc((size_t)cnt * sizeof(int));
        if (!and_inputs) { fprintf(stderr, "parity_dnf: OOM\n"); exit(1); }
        int pos = 0;
        for (int b = 0; b < n; b++)
            if (mask & (1 << b))
                and_inputs[pos++] = input_ids[b];
        and_gates[n_and] = rs_circuit_add_and(C, and_inputs, cnt);
        n_and++;
        free(and_inputs);
    }

    int out = (n_and > 0) ? rs_circuit_add_or(C, and_gates, n_and)
                          : rs_circuit_add_constant(C, 0);
    rs_circuit_set_output(C, out);
    free(and_gates);
    free(input_ids);
    return C;
}

RSCircuit* rs_circuit_modp_naive(int n, int p) {
    /* MOD_p via single MOD_p gate + input gates.
     * In AC0[p], this is depth 1, size n. */
    RSCircuit* C = rs_circuit_create(n);
    int* input_ids = malloc((size_t)n * sizeof(int));
    if (!input_ids) { fprintf(stderr, "modp_naive: OOM\n"); exit(1); }
    for (int i = 0; i < n; i++)
        input_ids[i] = rs_circuit_add_input(C, i);
    int out = rs_circuit_add_modp(C, p, input_ids, n);
    rs_circuit_set_output(C, out);
    free(input_ids);
    return C;
}
