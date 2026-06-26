/* circuit_complexity.c -- Circuit Complexity Measures and Classes
 *
 * Implements circuit complexity measures (density, size-depth product,
 * fan-in/fan-out statistics), complexity class membership tests
 * (AC^0, NC^1, TC^0, P/poly, etc.), lower bound estimates from major
 * theorems (Shannon, Lupanov, Nechiporuk, Hastad, Razborov-Smolensky),
 * and circuit family operations.
 *
 * Theorem (Shannon 1949): Almost all n-variable Boolean functions
 *   require circuit size Omega(2^n / n).
 * Theorem (Lupanov 1958): Every n-variable Boolean function has a
 *   circuit of size O(2^n / n).
 * Theorem (Hastad 1986): Depth-d AC^0 circuits computing PARITY
 *   require size exp(Omega(n^{1/(d-1)})).
 * Theorem (Razborov-Smolensky 1987): MOD-p requires exponential
 *   size in AC^0 with MOD-q gates.
 * Theorem (Karchmer-Wigderson 1988): Circuit depth equals the
 *   communication complexity of the K-W game.
 *
 * References: AB (2009) Ch.6,14; Vollmer (1999); Jukna (2012).
 */

#include "circuit_complexity.h"
#include "circuit_builder.h"
#include "circuit_analysis.h"
#include <assert.h>
#include <math.h>
#include <string.h>

/* ===================================================================
 * L2: Circuit Complexity Measures
 * =================================================================== */

double circuit_density(const BooleanCircuit* c) {
    assert(c != NULL);
    int n = c->n_gates;
    if (n < 2) return 0.0;
    double max_edges = (double)n * (double)(n - 1) / 2.0;
    return (double)c->n_edges / max_edges;
}

double circuit_complexity_product(const BooleanCircuit* c) {
    assert(c != NULL);
    return (double)circuit_size(c) * (double)circuit_depth(c);
}

double circuit_tradeoff_exponent(const BooleanCircuit* c) {
    assert(c != NULL);
    int sz = circuit_size(c), dp = circuit_depth(c);
    if (sz <= 1 || dp <= 1) return 0.0;
    return log((double)sz) / log((double)dp);
}

double circuit_avg_fanin(const BooleanCircuit* c) {
    assert(c != NULL);
    int sum = 0, count = 0;
    for (int i = 0; i < c->n_gates; i++) {
        const Gate* g = &c->gates[i];
        if (g->type == INPUT || g->type == CONST0 || g->type == CONST1) continue;
        if (g->type == MAJ || g->type == THR) {
            sum += g->fanin; count++;
        } else {
            int fi = (g->input1 >= 0) + (g->input2 >= 0);
            if (fi > 0) { sum += fi; count++; }
        }
    }
    return count > 0 ? (double)sum / (double)count : 0.0;
}

double circuit_avg_fanout(const BooleanCircuit* c) {
    assert(c != NULL);
    int n = c->n_gates;
    if (n == 0) return 0.0;
    int* fanout = (int*)calloc((size_t)n, sizeof(int));
    assert(fanout);
    for (int i = 0; i < n; i++) {
        const Gate* g = &c->gates[i];
        if (g->type == MAJ || g->type == THR) {
            for (int j = 0; j < g->fanin; j++) fanout[g->fanin_ids[j]]++;
        } else {
            if (g->input1 >= 0) fanout[g->input1]++;
            if (g->input2 >= 0) fanout[g->input2]++;
        }
    }
    double sum = 0.0;
    for (int i = 0; i < n; i++) sum += (double)fanout[i];
    free(fanout);
    return sum / (double)n;
}

int circuit_pathwidth(const BooleanCircuit* c) {
    assert(c != NULL);
    int n = c->n_gates;
    if (n == 0) return 0;
    int* anc = (int*)calloc((size_t)n, sizeof(int));
    assert(anc);
    int* order = (int*)malloc((size_t)n * sizeof(int));
    assert(order);
    circuit_topological_order(c, order);
    for (int idx = 0; idx < n; idx++) {
        int i = order[idx];
        anc[i] += 1;
        const Gate* g = &c->gates[i];
        if (g->type == MAJ || g->type == THR) {
            for (int j = 0; j < g->fanin; j++) {
                int ch = g->fanin_ids[j];
                if (anc[i] > anc[ch]) anc[ch] = anc[i];
            }
        } else {
            if (g->input1 >= 0 && anc[i] > anc[g->input1]) anc[g->input1] = anc[i];
            if (g->input2 >= 0 && anc[i] > anc[g->input2]) anc[g->input2] = anc[i];
        }
    }
    int max_pw = 0;
    for (int i = 0; i < c->n_outputs; i++) {
        if (anc[c->output_ids[i]] > max_pw) max_pw = anc[c->output_ids[i]];
    }
    free(order); free(anc);
    return max_pw;
}

/* ===================================================================
 * L2: Complexity Class Membership Tests
 *
 * AC^0: constant depth, unbounded fan-in, poly size
 * AC^1: O(log n) depth, unbounded fan-in, poly size
 * AC^2: O(log^2 n) depth, unbounded fan-in, poly size
 * NC^1: O(log n) depth, bounded fan-in (<=2), poly size
 * NC^2: O(log^2 n) depth, bounded fan-in (<=2), poly size
 * TC^0: constant depth, threshold/MAJ gates allowed, poly size
 * TC^1: O(log n) depth, threshold/MAJ gates, poly size
 * P/poly: poly size, any depth, any basis
 * =================================================================== */

static int has_threshold_gates(const BooleanCircuit* c) {
    for (int i = 0; i < c->n_gates; i++)
        if (c->gates[i].type == MAJ || c->gates[i].type == THR) return 1;
    return 0;
}

static int has_unbounded_fanin(const BooleanCircuit* c) {
    for (int i = 0; i < c->n_gates; i++) {
        const Gate* g = &c->gates[i];
        if (g->type == MAJ || g->type == THR) return 1;
        if (g->type != INPUT && g->type != CONST0 && g->type != CONST1 && g->type != NOT) {
            int fi = (g->input1 >= 0 ? 1 : 0) + (g->input2 >= 0 ? 1 : 0);
            if (fi > 2) return 1;
        }
    }
    return 0;
}

static int is_poly_size(const BooleanCircuit* c) {
    int n = c->n_inputs;
    if (n == 0) return 1;
    double bound = pow((double)n, 3.0) + 100.0;
    return (double)c->n_gates <= bound;
}

static int is_constant_depth(const BooleanCircuit* c) { return circuit_depth(c) <= 10; }

static int is_log_depth(const BooleanCircuit* c) {
    int n = c->n_inputs;
    if (n == 0) return circuit_depth(c) <= 5;
    double bound = 10.0 * log2((double)(n + 1));
    return (double)circuit_depth(c) <= bound;
}

static int is_log2_depth(const BooleanCircuit* c) {
    int n = c->n_inputs;
    if (n == 0) return circuit_depth(c) <= 5;
    double logn = log2((double)(n + 1));
    return (double)circuit_depth(c) <= 10.0 * logn * logn;
}

int circuit_is_AC0(const BooleanCircuit* c) {
    assert(c != NULL);
    return is_constant_depth(c) && is_poly_size(c) && !has_threshold_gates(c);
}

int circuit_is_AC1(const BooleanCircuit* c) {
    assert(c != NULL);
    return is_log_depth(c) && is_poly_size(c) && !has_threshold_gates(c) && has_unbounded_fanin(c);
}

int circuit_is_AC2(const BooleanCircuit* c) {
    assert(c != NULL);
    return is_log2_depth(c) && is_poly_size(c) && !has_threshold_gates(c);
}

int circuit_is_NC1(const BooleanCircuit* c) {
    assert(c != NULL);
    return is_log_depth(c) && is_poly_size(c) && !has_unbounded_fanin(c) && !has_threshold_gates(c);
}

int circuit_is_NC2(const BooleanCircuit* c) {
    assert(c != NULL);
    return is_log2_depth(c) && is_poly_size(c) && !has_unbounded_fanin(c) && !has_threshold_gates(c);
}

int circuit_is_TC0(const BooleanCircuit* c) {
    assert(c != NULL);
    return is_constant_depth(c) && is_poly_size(c) && has_threshold_gates(c);
}

int circuit_is_TC1(const BooleanCircuit* c) {
    assert(c != NULL);
    return is_log_depth(c) && is_poly_size(c) && has_threshold_gates(c);
}

int circuit_is_P_poly(const BooleanCircuit* c) {
    assert(c != NULL);
    return is_poly_size(c);
}

CircuitClass circuit_determine_class(const BooleanCircuit* c) {
    assert(c != NULL);
    if (circuit_is_AC0(c)) return CLASS_AC0;
    if (circuit_is_TC0(c)) return CLASS_TC0;
    if (circuit_is_NC1(c)) return CLASS_NC1;
    if (circuit_is_TC1(c)) return CLASS_TC1;
    if (circuit_is_AC1(c)) return CLASS_AC1;
    if (circuit_is_NC2(c)) return CLASS_NC2;
    if (circuit_is_AC2(c)) return CLASS_AC2;
    if (circuit_is_P_poly(c)) return CLASS_P_POLY;
    return CLASS_UNKNOWN;
}

const char* circuit_class_name(CircuitClass cls) {
    switch (cls) {
    case CLASS_AC0:    return "AC^0";
    case CLASS_AC1:    return "AC^1";
    case CLASS_AC2:    return "AC^2";
    case CLASS_NC1:    return "NC^1";
    case CLASS_NC2:    return "NC^2";
    case CLASS_TC0:    return "TC^0";
    case CLASS_TC1:    return "TC^1";
    case CLASS_P_POLY: return "P/poly";
    case CLASS_L_POLY: return "L/poly";
    case CLASS_NC:     return "NC";
    case CLASS_AC:     return "AC";
    default:           return "UNKNOWN";
    }
}

/* ===================================================================
 * L4: Fundamental Lower Bound Estimates
 *
 * Shannon (1949) counting argument:
 *   Number of n-variable Boolean functions = 2^{2^n}.
 *   Number of circuits of size s <= (c*s)^{s+n}.
 *   For s < 2^n / (c' * n), not all functions representable.
 *   => L(n) ~ 2^n / n.
 *
 * Lupanov (1958) upper bound:
 *   Every n-variable Boolean function has circuit size
 *   U(n) = 2^n / n * (1 + o(1)).
 *
 * Nechiporuk (1966): For m independent subfunctions,
 *   size >= Omega(m * log m / log n).
 *
 * Hastad (1986) Switching Lemma:
 *   PARITY not in AC^0: depth-d AC^0 requires size >=
 *   exp(Omega(n^{1/(d-1)})).
 *
 * Razborov-Smolensky (1987):
 *   MOD-p requires exponential size in AC^0 with MOD-q gates.
 *
 * Karchmer-Wigderson (1988):
 *   Depth = communication complexity of KW relation.
 * =================================================================== */

double shannon_lower_bound(int n) {
    assert(n > 0);
    /* L(n) ~ 2^n / (2*n) */
    return pow(2.0, (double)n) / (2.0 * (double)n);
}

double lupanov_upper_bound(int n) {
    assert(n > 0);
    /* U(n) = 2^n / n * (1 + log n / n) */
    double base = pow(2.0, (double)n) / (double)n;
    return base * (1.0 + log((double)n) / (double)n);
}

double lupanov_size_depth(int n, int depth) {
    assert(n > 0 && depth > 0);
    /* Size ~ 2^n / min(depth, n/log n) */
    double lim = (double)n / log2((double)(n + 1));
    double dmin = ((double)depth < lim) ? (double)depth : lim;
    return pow(2.0, (double)n) / dmin;
}

double nechiporuk_bound(int n, int m_subfunctions) {
    assert(n > 0 && m_subfunctions > 0);
    /* S >= m * log2(m) / (4 * log2(n)) */
    return (double)m_subfunctions * log2((double)m_subfunctions) /
           (4.0 * log2((double)n));
}

double hastad_parity_lower_bound(int n, int depth) {
    assert(n > 0 && depth >= 2);
    /* size >= 2^{ n^{1/(d-1)} / 20 } */
    double exponent = pow((double)n, 1.0 / (double)(depth - 1)) / 20.0;
    return pow(2.0, exponent);
}

double razborov_smolensky_bound(int n, int p, int q) {
    assert(n > 0 && p > 1 && q > 1);
    /* MOD-p requires size >= exp(Omega(n^{1/(2p)})) in AC^0[MOD-q] */
    (void)q;
    double exponent = pow((double)n, 1.0 / (2.0 * (double)p)) / 10.0;
    return pow(2.0, exponent);
}

int kw_depth_lower_bound(int n, int function_id) {
    assert(n > 0);
    /* KW game lower bounds:
     *   PARITY (0): depth >= ceil(log n)
     *   MAJORITY (1): depth >= ceil(log n)/2
     *   DISJOINTNESS (2): depth >= n/2
     *   INNER PRODUCT (3): depth >= n-1 */
    switch (function_id) {
    case 0: return (int)ceil(log2((double)n));
    case 1: return (int)ceil(log2((double)n)) / 2;
    case 2: return n / 2;
    case 3: return n - 1;
    default: return 1;
    }
}

/* ===================================================================
 * L5: Circuit Minimization
 * =================================================================== */

BooleanCircuit* circuit_minimize(BooleanCircuit* c) {
    assert(c != NULL);
    BooleanCircuit* result = circuit_constant_propagate(c);
    if (!result) return NULL;
    int n = result->n_gates;
    int* equiv = (int*)malloc((size_t)n * sizeof(int));
    assert(equiv);
    for (int i = 0; i < n; i++) equiv[i] = i;
    for (int i = 0; i < n; i++) {
        if (equiv[i] != i) continue;
        const Gate* gi = &result->gates[i];
        if (gi->type == INPUT || gi->type == CONST0 || gi->type == CONST1) continue;
        for (int j = i + 1; j < n; j++) {
            if (equiv[j] != j) continue;
            const Gate* gj = &result->gates[j];
            if (gi->type != gj->type) continue;
            int same = 0;
            if (gi->type == MAJ || gi->type == THR) {
                if (gi->fanin == gj->fanin) {
                    same = 1;
                    for (int k = 0; k < gi->fanin; k++) {
                        if (equiv[gi->fanin_ids[k]] != equiv[gj->fanin_ids[k]]) { same = 0; break; }
                    }
                }
            } else {
                int in1_i = (gi->input1 >= 0) ? equiv[gi->input1] : -1;
                int in2_i = (gi->input2 >= 0) ? equiv[gi->input2] : -1;
                int in1_j = (gj->input1 >= 0) ? equiv[gj->input1] : -1;
                int in2_j = (gj->input2 >= 0) ? equiv[gj->input2] : -1;
                same = (in1_i == in1_j && in2_i == in2_j);
            }
            if (same) equiv[j] = i;
        }
    }
    int* new_id = (int*)malloc((size_t)n * sizeof(int));
    assert(new_id);
    for (int i = 0; i < n; i++) new_id[i] = -1;
    BooleanCircuit* minc = circuit_create(n + 10);
    int* order = (int*)malloc((size_t)n * sizeof(int));
    assert(order);
    circuit_topological_order(result, order);
    for (int idx = 0; idx < n; idx++) {
        int i = order[idx];
        if (equiv[i] != i && new_id[equiv[i]] >= 0) {
            new_id[i] = new_id[equiv[i]];
            continue;
        }
        const Gate* g = &result->gates[i];
        if (g->type == MAJ || g->type == THR) {
            int* fids = (int*)malloc((size_t)g->fanin * sizeof(int));
            assert(fids);
            for (int j = 0; j < g->fanin; j++) fids[j] = new_id[equiv[g->fanin_ids[j]]];
            new_id[i] = circuit_add_maj_gate(minc, g->type, g->fanin, fids);
            free(fids);
        } else {
            int a = (g->input1 >= 0) ? new_id[equiv[g->input1]] : -1;
            int b = (g->input2 >= 0) ? new_id[equiv[g->input2]] : -1;
            new_id[i] = circuit_add_gate(minc, g->type, a, b);
        }
    }
    int* outs = (int*)malloc((size_t)result->n_outputs * sizeof(int));
    assert(outs);
    for (int i = 0; i < result->n_outputs; i++) outs[i] = new_id[equiv[result->output_ids[i]]];
    circuit_set_output(minc, outs, result->n_outputs);
    free(outs); free(order); free(new_id); free(equiv);
    if (result != c) circuit_free(result);
    return minc;
}

BooleanCircuit* circuit_constant_propagate(BooleanCircuit* c) {
    assert(c != NULL);
    int n = c->n_gates;
    int* val = (int*)malloc((size_t)n * sizeof(int));
    assert(val);
    for (int i = 0; i < n; i++) val[i] = -1;
    for (int i = 0; i < n; i++) {
        if (c->gates[i].type == CONST0) val[i] = 0;
        else if (c->gates[i].type == CONST1) val[i] = 1;
    }
    int changed = 1;
    while (changed) {
        changed = 0;
        for (int i = 0; i < n; i++) {
            if (val[i] >= 0) continue;
            const Gate* g = &c->gates[i];
            int ready = 0, result = 0;
            switch (g->type) {
            case NOT:
                if (g->input1 >= 0 && val[g->input1] >= 0) { result = !val[g->input1]; ready = 1; }
                break;
            case AND:
                if (g->input1 >= 0 && g->input2 >= 0) {
                    if (val[g->input1] == 0 || val[g->input2] == 0) { result = 0; ready = 1; }
                    else if (val[g->input1] == 1 && val[g->input2] == 1) { result = 1; ready = 1; }
                }
                break;
            case OR:
                if (g->input1 >= 0 && g->input2 >= 0) {
                    if (val[g->input1] == 1 || val[g->input2] == 1) { result = 1; ready = 1; }
                    else if (val[g->input1] == 0 && val[g->input2] == 0) { result = 0; ready = 1; }
                }
                break;
            case XOR:
                if (g->input1 >= 0 && g->input2 >= 0 && val[g->input1] >= 0 && val[g->input2] >= 0) {
                    result = val[g->input1] ^ val[g->input2]; ready = 1;
                }
                break;
            case NAND:
                if (g->input1 >= 0 && g->input2 >= 0) {
                    if (val[g->input1] == 0 || val[g->input2] == 0) { result = 1; ready = 1; }
                    else if (val[g->input1] == 1 && val[g->input2] == 1) { result = 0; ready = 1; }
                }
                break;
            case NOR:
                if (g->input1 >= 0 && g->input2 >= 0) {
                    if (val[g->input1] == 1 || val[g->input2] == 1) { result = 0; ready = 1; }
                    else if (val[g->input1] == 0 && val[g->input2] == 0) { result = 1; ready = 1; }
                }
                break;
            default: break;
            }
            if (ready) { val[i] = result; changed = 1; }
        }
    }
    BooleanCircuit* cpc = circuit_create(n + 10);
    int* new_id = (int*)malloc((size_t)n * sizeof(int));
    assert(new_id);
    for (int i = 0; i < n; i++) new_id[i] = -1;
    int* order = (int*)malloc((size_t)n * sizeof(int));
    assert(order);
    circuit_topological_order(c, order);
    for (int idx = 0; idx < n; idx++) {
        int i = order[idx];
        if (val[i] >= 0) {
            new_id[i] = circuit_add_gate(cpc, val[i] ? CONST1 : CONST0, -1, -1);
            continue;
        }
        const Gate* g = &c->gates[i];
        if (g->type == MAJ || g->type == THR) {
            int* fids = (int*)malloc((size_t)g->fanin * sizeof(int));
            assert(fids);
            int valid = 1;
            for (int j = 0; j < g->fanin; j++) {
                if (new_id[g->fanin_ids[j]] < 0) { valid = 0; break; }
                fids[j] = new_id[g->fanin_ids[j]];
            }
            if (valid) new_id[i] = circuit_add_maj_gate(cpc, g->type, g->fanin, fids);
            else new_id[i] = circuit_add_gate(cpc, CONST0, -1, -1);
            free(fids);
        } else {
            int a = (g->input1 >= 0) ? new_id[g->input1] : -1;
            int b = (g->input2 >= 0) ? new_id[g->input2] : -1;
            new_id[i] = circuit_add_gate(cpc, g->type, a, b);
        }
    }
    int* outs = (int*)malloc((size_t)c->n_outputs * sizeof(int));
    assert(outs);
    for (int i = 0; i < c->n_outputs; i++) outs[i] = new_id[c->output_ids[i]];
    circuit_set_output(cpc, outs, c->n_outputs);
    free(outs); free(order); free(new_id); free(val);
    return cpc;
}

/* ===================================================================
 * L6: Circuit Family Management
 * =================================================================== */

CircuitFamily* circuit_family_create(BooleanCircuit** circuits, int max_n,
                                      const int* sizes, const int* depths,
                                      int uniform, const char* name) {
    assert(circuits != NULL); assert(max_n > 0); assert(name != NULL);
    CircuitFamily* cf = (CircuitFamily*)malloc(sizeof(CircuitFamily));
    if (!cf) return NULL;
    cf->circuits = (BooleanCircuit**)calloc((size_t)(max_n + 1),
                                             sizeof(BooleanCircuit*));
    if (!cf->circuits) { free(cf); return NULL; }
    for (int i = 0; i <= max_n && circuits[i] != NULL; i++)
        cf->circuits[i] = circuits[i];
    cf->max_n = max_n;
    cf->sizes  = (int*)malloc((size_t)(max_n + 1) * sizeof(int));
    cf->depths = (int*)malloc((size_t)(max_n + 1) * sizeof(int));
    if (!cf->sizes || !cf->depths) {
        free(cf->sizes); free(cf->depths);
        free(cf->circuits); free(cf); return NULL;
    }
    for (int i = 0; i <= max_n; i++) {
        cf->sizes[i]  = sizes  ? sizes[i]  : 0;
        cf->depths[i] = depths ? depths[i] : 0;
    }
    cf->class_hint = CLASS_UNKNOWN;
    cf->uniform = uniform;
    cf->name = (char*)malloc(strlen(name) + 1);
    if (cf->name) strcpy(cf->name, name);
    return cf;
}

BooleanCircuit* circuit_family_get(const CircuitFamily* family, int n) {
    assert(family != NULL);
    if (n < 0 || n > family->max_n) return NULL;
    return family->circuits[n];
}

CircuitClass circuit_family_class(const CircuitFamily* family) {
    assert(family != NULL);
    return family->class_hint;
}

int circuit_family_is_uniform(const CircuitFamily* family) {
    assert(family != NULL);
    return family->uniform;
}

void circuit_family_free(CircuitFamily* family) {
    if (!family) return;
    if (family->circuits) {
        for (int i = 0; i <= family->max_n; i++)
            if (family->circuits[i])
                circuit_free(family->circuits[i]);
        free(family->circuits);
    }
    free(family->sizes);
    free(family->depths);
    free(family->name);
    free(family);
}

void circuit_family_print(const CircuitFamily* family) {
    if (!family) { printf("CircuitFamily: NULL\n"); return; }
    printf("Circuit Family: %s\n", family->name);
    printf("  max_n=%d, uniform=%d, class=%s\n",
           family->max_n, family->uniform,
           circuit_class_name(family->class_hint));
    for (int i = 0; i <= family->max_n && family->circuits[i]; i++)
        printf("  C[%d]: size=%d, depth=%d\n",
               i, family->sizes[i], family->depths[i]);
}

/* ===================================================================
 * L6: Known Circuit Families
 * =================================================================== */

CircuitFamily* circuit_family_parity(int max_n) {
    assert(max_n > 0 && max_n <= 64);
    BooleanCircuit** arr = (BooleanCircuit**)calloc((size_t)(max_n + 1),
                                                     sizeof(BooleanCircuit*));
    assert(arr);
    int* sizes  = (int*)calloc((size_t)(max_n + 1), sizeof(int));
    int* depths = (int*)calloc((size_t)(max_n + 1), sizeof(int));
    assert(sizes && depths);
    for (int n = 1; n <= max_n; n++) {
        arr[n] = circuit_parity(n);
        sizes[n]  = arr[n]->n_gates;
        depths[n] = circuit_depth(arr[n]);
    }
    CircuitFamily* cf = circuit_family_create(arr, max_n, sizes, depths,
                                               1, "PARITY");
    cf->class_hint = CLASS_NC1;
    free(sizes); free(depths); free(arr);
    return cf;
}

CircuitFamily* circuit_family_majority(int max_n) {
    assert(max_n > 0 && max_n <= 32);
    BooleanCircuit** arr = (BooleanCircuit**)calloc((size_t)(max_n + 1),
                                                     sizeof(BooleanCircuit*));
    assert(arr);
    int* sizes  = (int*)calloc((size_t)(max_n + 1), sizeof(int));
    int* depths = (int*)calloc((size_t)(max_n + 1), sizeof(int));
    assert(sizes && depths);
    for (int n = 1; n <= max_n; n++) {
        arr[n] = circuit_majority(n);
        sizes[n]  = arr[n]->n_gates;
        depths[n] = circuit_depth(arr[n]);
    }
    CircuitFamily* cf = circuit_family_create(arr, max_n, sizes, depths,
                                               1, "MAJORITY");
    cf->class_hint = CLASS_TC0;
    free(sizes); free(depths); free(arr);
    return cf;
}

CircuitFamily* circuit_family_adder(int max_n) {
    assert(max_n > 0 && max_n <= 32);
    BooleanCircuit** arr = (BooleanCircuit**)calloc((size_t)(max_n + 1),
                                                     sizeof(BooleanCircuit*));
    assert(arr);
    int* sizes  = (int*)calloc((size_t)(max_n + 1), sizeof(int));
    int* depths = (int*)calloc((size_t)(max_n + 1), sizeof(int));
    assert(sizes && depths);
    for (int n = 1; n <= max_n; n++) {
        int carry_out;
        arr[n] = circuit_adder(n, &carry_out);
        sizes[n]  = arr[n]->n_gates;
        depths[n] = circuit_depth(arr[n]);
    }
    CircuitFamily* cf = circuit_family_create(arr, max_n, sizes, depths,
                                               1, "ADDER");
    cf->class_hint = CLASS_AC0;
    free(sizes); free(depths); free(arr);
    return cf;
}

CircuitFamily* circuit_family_multiplier(int max_n) {
    assert(max_n > 0 && max_n <= 8);
    BooleanCircuit** arr = (BooleanCircuit**)calloc((size_t)(max_n + 1),
                                                     sizeof(BooleanCircuit*));
    assert(arr);
    int* sizes  = (int*)calloc((size_t)(max_n + 1), sizeof(int));
    int* depths = (int*)calloc((size_t)(max_n + 1), sizeof(int));
    assert(sizes && depths);
    for (int n = 1; n <= max_n; n++) {
        arr[n] = circuit_multiplier(n);
        sizes[n]  = arr[n]->n_gates;
        depths[n] = circuit_depth(arr[n]);
    }
    CircuitFamily* cf = circuit_family_create(arr, max_n, sizes, depths,
                                               1, "MULTIPLIER");
    cf->class_hint = CLASS_TC0;
    free(sizes); free(depths); free(arr);
    return cf;
}

CircuitFamily* circuit_family_clique(int max_n, int k) {
    assert(max_n >= 2 && k >= 2);
    BooleanCircuit** arr = (BooleanCircuit**)calloc((size_t)(max_n + 1),
                                                     sizeof(BooleanCircuit*));
    assert(arr);
    int* sizes  = (int*)calloc((size_t)(max_n + 1), sizeof(int));
    int* depths = (int*)calloc((size_t)(max_n + 1), sizeof(int));
    assert(sizes && depths);
    for (int n = k; n <= max_n; n++) {
        arr[n] = circuit_clique(n, k);
        sizes[n]  = arr[n]->n_gates;
        depths[n] = circuit_depth(arr[n]);
    }
    CircuitFamily* cf = circuit_family_create(arr, max_n, sizes, depths,
                                               0, "CLIQUE");
    cf->class_hint = CLASS_P_POLY;
    free(sizes); free(depths); free(arr);
    return cf;
}

CircuitFamily* circuit_family_threshold(int max_n, int k) {
    assert(max_n > 0 && k >= 0);
    BooleanCircuit** arr = (BooleanCircuit**)calloc((size_t)(max_n + 1),
                                                     sizeof(BooleanCircuit*));
    assert(arr);
    int* sizes  = (int*)calloc((size_t)(max_n + 1), sizeof(int));
    int* depths = (int*)calloc((size_t)(max_n + 1), sizeof(int));
    assert(sizes && depths);
    int start_n = (k > 0) ? k : 1;
    for (int n = start_n; n <= max_n; n++) {
        arr[n] = circuit_threshold(n, k);
        sizes[n]  = arr[n]->n_gates;
        depths[n] = circuit_depth(arr[n]);
    }
    CircuitFamily* cf = circuit_family_create(arr, max_n, sizes, depths,
                                               1, "THRESHOLD");
    cf->class_hint = CLASS_TC0;
    free(sizes); free(depths); free(arr);
    return cf;
}
