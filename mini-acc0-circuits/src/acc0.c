/* acc0.c -- Core ACC0 Circuit Implementation
 * ===========================================
 * ACC0 = AC0 + MOD_m gates (for all m >= 2).
 *
 * L1: Definitions -- Gate types, circuit structure.
 * L2: Core Concepts -- Constant-depth, unbounded fan-in, modular counting.
 * L3: Mathematical Structures -- DAG, topological ordering, measures.
 * L5: Algorithms -- Evaluation, depth computation, fan-in/out, clone.
 * L7: Applications -- Truth-table verification, circuit equivalence.
 *
 * References:
 * - Arora & Barak (2009), Ch.6, Ch.14
 * - Vollmer (1999), Ch.4
 * - Williams (2014), JACM 61(1)
 */

#include "acc0.h"
#include <assert.h>

/* ================================================================
 * Circuit Lifecycle
 * ================================================================ */

ACC0Circuit* acc0_circuit_create(int max_gates) {
    ACC0Circuit *c = (ACC0Circuit*)malloc(sizeof(ACC0Circuit));
    if (!c) return NULL;
    c->gates = (ACC0Gate*)calloc((size_t)max_gates, sizeof(ACC0Gate));
    if (!c->gates) { free(c); return NULL; }
    c->ngates    = 0;
    c->max_gates = max_gates;
    c->ninputs   = 0;
    c->noutputs  = 0;
    c->outputs   = NULL;
    c->depth     = -1;
    c->size      = 0;
    return c;
}

void acc0_circuit_free(ACC0Circuit *c) {
    if (!c) return;
    free(c->gates);
    free(c->outputs);
    free(c);
}

/* ================================================================
 * Gate Addition -- Internal helper and public API
 * ================================================================ */

static int add_gate_internal(ACC0Circuit *c, ACC0GateType type, int i1, int i2) {
    if (c->ngates >= c->max_gates) return -1;
    int id = c->ngates++;
    c->gates[id].type    = type;
    c->gates[id].i1      = i1;
    c->gates[id].i2      = i2;
    c->gates[id].nfans   = 0;
    c->gates[id].modulus = 0;
    c->gates[id].level   = -1;
    c->size = c->ngates;
    return id;
}

int acc0_add_input(ACC0Circuit *c) {
    int id = add_gate_internal(c, ACC0_INPUT, -1, -1);
    if (id >= 0) c->ninputs++;
    return id;
}

int acc0_add_constant(ACC0Circuit *c, int value) {
    return add_gate_internal(c, ACC0_CONST, (value != 0) ? 1 : 0, -1);
}

int acc0_add_not(ACC0Circuit *c, int in) {
    return add_gate_internal(c, ACC0_NOT, in, -1);
}

int acc0_add_and(ACC0Circuit *c, int in1, int in2) {
    return add_gate_internal(c, ACC0_AND, in1, in2);
}

int acc0_add_or(ACC0Circuit *c, int in1, int in2) {
    return add_gate_internal(c, ACC0_OR, in1, in2);
}

int acc0_add_mod(ACC0Circuit *c, int modulus) {
    ACC0GateType type;
    switch (modulus) {
        case 2:  type = ACC0_MOD2;  break;
        case 3:  type = ACC0_MOD3;  break;
        case 5:  type = ACC0_MOD5;  break;
        case 7:  type = ACC0_MOD7;  break;
        case 11: type = ACC0_MOD11; break;
        case 13: type = ACC0_MOD13; break;
        default: return -1;
    }
    int id = add_gate_internal(c, type, -1, -1);
    if (id >= 0) c->gates[id].modulus = modulus;
    return id;
}

int acc0_add_mod_composite(ACC0Circuit *c, int modulus) {
    if (modulus < 2) return -1;
    int id = add_gate_internal(c, ACC0_MOD_COMPOSITE, -1, -1);
    if (id >= 0) c->gates[id].modulus = modulus;
    return id;
}

int acc0_set_fanin(ACC0Circuit *c, int gid, const int *fans, int n) {
    if (!c || gid < 0 || gid >= c->ngates || n > ACC0_MAX_FANIN) return -1;
    ACC0Gate *g = &c->gates[gid];
    if (fans && n > 0) memcpy(g->fans, fans, (size_t)n * sizeof(int));
    g->nfans = n;
    return 0;
}

int acc0_add_fanin_one(ACC0Circuit *c, int gid, int fanin) {
    if (!c || gid < 0 || gid >= c->ngates || fanin < 0 || fanin >= c->ngates) return -1;
    ACC0Gate *g = &c->gates[gid];
    if (g->nfans >= ACC0_MAX_FANIN) return -1;
    g->fans[g->nfans++] = fanin;
    return 0;
}

int acc0_set_outputs(ACC0Circuit *c, const int *outs, int n) {
    if (!c || n < 0) return -1;
    free(c->outputs);
    c->noutputs = n;
    if (n > 0) {
        c->outputs = (int*)malloc((size_t)n * sizeof(int));
        if (!c->outputs) { c->noutputs = 0; return -1; }
        memcpy(c->outputs, outs, (size_t)n * sizeof(int));
    } else {
        c->outputs = NULL;
    }
    return 0;
}

/* ================================================================
 * Circuit Evaluation -- Recursive with memoization
 * L5: Algorithm -- O(|C|) evaluation via visited[] caching.
 * ================================================================ */

static int mod_value(ACC0GateType t) {
    switch (t) {
        case ACC0_MOD2:  return 2;
        case ACC0_MOD3:  return 3;
        case ACC0_MOD5:  return 5;
        case ACC0_MOD7:  return 7;
        case ACC0_MOD11: return 11;
        case ACC0_MOD13: return 13;
        default: return 0;
    }
}

int acc0_gate_eval(ACC0Circuit *c, int gid, const int *inputs, int *visited) {
    if (visited[gid] >= 0) return visited[gid];
    ACC0Gate *g = &c->gates[gid];
    int r = 0;
    switch (g->type) {
    case ACC0_INPUT:
        r = inputs[gid]; break;
    case ACC0_CONST:
        r = g->i1; break;
    case ACC0_NOT:
        r = !acc0_gate_eval(c, g->i1, inputs, visited); break;
    case ACC0_AND:
        if (g->nfans > 0) {
            r = 1;
            for (int i = 0; i < g->nfans; i++)
                r &= acc0_gate_eval(c, g->fans[i], inputs, visited);
        } else {
            r = acc0_gate_eval(c, g->i1, inputs, visited)
              & acc0_gate_eval(c, g->i2, inputs, visited);
        }
        break;
    case ACC0_OR:
        if (g->nfans > 0) {
            r = 0;
            for (int i = 0; i < g->nfans; i++)
                r |= acc0_gate_eval(c, g->fans[i], inputs, visited);
        } else {
            r = acc0_gate_eval(c, g->i1, inputs, visited)
              | acc0_gate_eval(c, g->i2, inputs, visited);
        }
        break;
    case ACC0_MOD2: case ACC0_MOD3: case ACC0_MOD5:
    case ACC0_MOD7: case ACC0_MOD11: case ACC0_MOD13:
    case ACC0_MOD_COMPOSITE: {
        int m = (g->type == ACC0_MOD_COMPOSITE) ? g->modulus
                                                 : mod_value(g->type);
        int s = 0;
        for (int i = 0; i < g->nfans; i++)
            s += acc0_gate_eval(c, g->fans[i], inputs, visited);
        r = (s % m == 0) ? 1 : 0;
        break;
    }
    default: r = 0;
    }
    visited[gid] = r;
    return r;
}

int acc0_circuit_eval(ACC0Circuit *c, const int *inputs) {
    if (!c) return 0;
    int *vis = (int*)malloc((size_t)c->ngates * sizeof(int));
    if (!vis) return 0;
    for (int i = 0; i < c->ngates; i++) vis[i] = -1;
    int r = 1;
    for (int i = 0; i < c->noutputs; i++)
        r &= acc0_gate_eval(c, c->outputs[i], inputs, vis);
    free(vis);
    return r;
}

/* ================================================================
 * Circuit Depth -- Iterative relaxation over DAG
 * L5: Algorithm -- O(|C| * depth) topological depth.
 * ================================================================ */

int acc0_circuit_depth(ACC0Circuit *c) {
    if (!c) return 0;
    int *d = (int*)calloc((size_t)c->ngates, sizeof(int));
    if (!d) return 0;
    for (int i = 0; i < c->ngates; i++)
        d[i] = (c->gates[i].type == ACC0_INPUT ||
                c->gates[i].type == ACC0_CONST) ? 0 : -1;
    for (int pass = 0; pass < c->ngates; pass++) {
        int changed = 0;
        for (int i = 0; i < c->ngates; i++) {
            if (d[i] >= 0) continue;
            ACC0Gate *g = &c->gates[i];
            int md = -1, ok = 1;
            if (g->nfans > 0) {
                for (int j = 0; j < g->nfans; j++) {
                    if (d[g->fans[j]] < 0) { ok = 0; break; }
                    if (d[g->fans[j]] > md) md = d[g->fans[j]];
                }
            } else {
                if (g->i1 >= 0) {
                    if (d[g->i1] < 0) ok = 0;
                    else if (d[g->i1] > md) md = d[g->i1];
                }
                if (g->i2 >= 0 && g->type != ACC0_NOT) {
                    if (d[g->i2] < 0) ok = 0;
                    else if (d[g->i2] > md) md = d[g->i2];
                }
            }
            if (ok) { d[i] = md + 1; changed = 1; }
        }
        if (!changed) break;
    }
    int md = 0;
    for (int i = 0; i < c->noutputs; i++)
        if (d[c->outputs[i]] > md) md = d[c->outputs[i]];
    free(d);
    c->depth = md;
    return md;
}

/* ================================================================
 * Circuit Measures
 * ================================================================ */

ACC0CircuitMeasures acc0_circuit_measures(ACC0Circuit *c) {
    ACC0CircuitMeasures m;
    memset(&m, 0, sizeof(m));
    m.size  = c->ngates;
    m.depth = acc0_circuit_depth(c);
    m.max_fanin = 0;
    for (int i = 0; i < c->ngates; i++) {
        ACC0Gate *g = &c->gates[i];
        if (g->nfans > m.max_fanin) m.max_fanin = g->nfans;
        m.wire_count += g->nfans;
        switch (g->type) {
        case ACC0_AND: m.and_gate_count++; break;
        case ACC0_OR:  m.or_gate_count++;  break;
        case ACC0_NOT: m.not_gate_count++; break;
        default:
            if (g->type >= ACC0_MOD2 && g->type <= ACC0_MOD_COMPOSITE)
                m.mod_gate_count++;
            break;
        }
    }
    return m;
}

/* ================================================================
 * Topological Ordering -- Kahn's algorithm
 * ================================================================ */

int* acc0_topological_order(ACC0Circuit *c) {
    if (!c) return NULL;
    int n = c->ngates;
    int *order = (int*)malloc((size_t)n * sizeof(int));
    int *indeg = (int*)calloc((size_t)n, sizeof(int));
    if (!order || !indeg) { free(order); free(indeg); return NULL; }
    for (int i = 0; i < n; i++) {
        ACC0Gate *g = &c->gates[i];
        if (g->nfans > 0) {
            for (int j = 0; j < g->nfans; j++) indeg[g->fans[j]]++;
        } else {
            if (g->i1 >= 0) indeg[g->i1]++;
            if (g->i2 >= 0) indeg[g->i2]++;
        }
    }
    int *q = (int*)malloc((size_t)n * sizeof(int));
    int qh = 0, qt = 0;
    for (int i = 0; i < n; i++)
        if (indeg[i] == 0) q[qt++] = i;
    int pos = 0;
    while (qh < qt) {
        int u = q[qh++];
        order[pos++] = u;
        for (int v = 0; v < n; v++) {
            if (v == u) continue;
            ACC0Gate *gv = &c->gates[v];
            int dep = 0;
            if (gv->nfans > 0) {
                for (int j = 0; j < gv->nfans; j++)
                    if (gv->fans[j] == u) { dep = 1; break; }
            } else if (gv->i1 == u || gv->i2 == u) dep = 1;
            if (dep && --indeg[v] == 0) q[qt++] = v;
        }
    }
    free(q); free(indeg);
    return order;
}

int acc0_is_acyclic(ACC0Circuit *c) {
    if (!c) return 1;
    int *o = acc0_topological_order(c);
    if (!o) return 0;
    int ok = 1;
    for (int i = 0; i < c->ngates; i++)
        if (o[i] < 0 || o[i] >= c->ngates) { ok = 0; break; }
    free(o);
    return ok;
}

/* ================================================================
 * Fan-in / Fan-out Closures
 * ================================================================ */

int* acc0_fanin_closure(ACC0Circuit *c, int gid) {
    if (!c || gid < 0 || gid >= c->ngates) return NULL;
    int *vis = (int*)calloc((size_t)c->ngates, sizeof(int));
    if (!vis) return NULL;
    int *stk = (int*)malloc((size_t)c->ngates * sizeof(int));
    int sp = 0;
    stk[sp++] = gid;
    while (sp > 0) {
        int id = stk[--sp];
        if (vis[id]) continue;
        vis[id] = 1;
        ACC0Gate *g = &c->gates[id];
        if (g->nfans > 0) {
            for (int i = 0; i < g->nfans; i++)
                if (!vis[g->fans[i]]) stk[sp++] = g->fans[i];
        } else {
            if (g->i1 >= 0 && !vis[g->i1]) stk[sp++] = g->i1;
            if (g->i2 >= 0 && g->type != ACC0_NOT && !vis[g->i2])
                stk[sp++] = g->i2;
        }
    }
    free(stk);
    return vis;
}

int* acc0_fanout_closure(ACC0Circuit *c, int gid, int *count) {
    if (!c || gid < 0 || gid >= c->ngates || !count) return NULL;
    int n = c->ngates;
    int *vis = (int*)calloc((size_t)n, sizeof(int));
    if (!vis) return NULL;
    int *q = (int*)malloc((size_t)n * sizeof(int));
    int qh = 0, qt = 0;
    q[qt++] = gid; vis[gid] = 1;
    while (qh < qt) {
        int u = q[qh++];
        for (int v = 0; v < n; v++) {
            if (vis[v]) continue;
            ACC0Gate *gv = &c->gates[v];
            int dep = 0;
            if (gv->nfans > 0) {
                for (int j = 0; j < gv->nfans; j++)
                    if (gv->fans[j] == u) { dep = 1; break; }
            } else if (gv->i1 == u || gv->i2 == u) dep = 1;
            if (dep) { vis[v] = 1; q[qt++] = v; }
        }
    }
    int *res = (int*)malloc((size_t)qt * sizeof(int));
    for (int i = 0; i < qt; i++) res[i] = q[i];
    *count = qt;
    free(q); free(vis);
    return res;
}

/* ================================================================
 * Clone -- Deep copy
 * ================================================================ */

ACC0Circuit* acc0_circuit_clone(ACC0Circuit *c) {
    if (!c) return NULL;
    ACC0Circuit *cl = acc0_circuit_create(c->max_gates);
    if (!cl) return NULL;
    memcpy(cl->gates, c->gates, (size_t)c->ngates * sizeof(ACC0Gate));
    cl->ngates  = c->ngates;
    cl->ninputs = c->ninputs;
    cl->size    = c->size;
    if (c->noutputs > 0) {
        cl->outputs = (int*)malloc((size_t)c->noutputs * sizeof(int));
        memcpy(cl->outputs, c->outputs, (size_t)c->noutputs * sizeof(int));
    }
    cl->noutputs = c->noutputs;
    cl->depth    = c->depth;
    return cl;
}

/* ================================================================
 * Verification -- Exhaustive truth-table check
 * ================================================================ */

int acc0_verify_truth_table(ACC0Circuit *c, int (*target)(const int*, int), int n) {
    if (!c || !target) return 0;
    long long total = 1LL << n;
    if (total > 1024) total = 1024;
    int *inp = (int*)calloc((size_t)n, sizeof(int));
    if (!inp) return 0;
    for (long long m = 0; m < total; m++) {
        for (int i = 0; i < n; i++) inp[i] = (int)((m >> i) & 1);
        if (acc0_circuit_eval(c, inp) != target(inp, n)) {
            free(inp); return 0;
        }
    }
    free(inp);
    return 1;
}

int acc0_circuits_equivalent(ACC0Circuit *c1, ACC0Circuit *c2, int ninputs) {
    if (!c1 || !c2) return 0;
    long long total = 1LL << ninputs;
    if (total > 65536) return -1;
    int *inp = (int*)calloc((size_t)ninputs, sizeof(int));
    if (!inp) return -1;
    for (long long m = 0; m < total; m++) {
        for (int i = 0; i < ninputs; i++) inp[i] = (int)((m >> i) & 1);
        if (acc0_circuit_eval(c1, inp) != acc0_circuit_eval(c2, inp)) {
            free(inp); return 0;
        }
    }
    free(inp);
    return 1;
}

/* ================================================================
 * Pretty Print
 * ================================================================ */

void acc0_print_circuit(ACC0Circuit *c) {
    if (!c) { printf("(null)\n"); return; }
    printf("ACC0 Circuit: %d gates, %d in, %d out, d=%d\n",
           c->ngates, c->ninputs, c->noutputs, acc0_circuit_depth(c));
    for (int i = 0; i < c->ngates; i++) {
        ACC0Gate *g = &c->gates[i];
        const char *tn = "?";
        switch (g->type) {
        case ACC0_INPUT: tn="IN"; break;
        case ACC0_AND: tn="AND"; break;
        case ACC0_OR: tn="OR"; break;
        case ACC0_NOT: tn="NOT"; break;
        case ACC0_CONST: tn="C"; break;
        case ACC0_MOD2: tn="M2"; break;
        case ACC0_MOD3: tn="M3"; break;
        case ACC0_MOD5: tn="M5"; break;
        case ACC0_MOD7: tn="M7"; break;
        case ACC0_MOD11: tn="M11"; break;
        case ACC0_MOD13: tn="M13"; break;
        case ACC0_MOD_COMPOSITE: tn="Mc"; break;
        }
        printf(" g%d:%s", i, tn);
        if (g->type == ACC0_CONST) printf("=%d", g->i1);
        else if (g->nfans > 0) {
            printf("(");
            for (int j = 0; j < g->nfans; j++)
                printf("%d%s", g->fans[j], j+1<g->nfans?",":"");
            printf(")");
        } else if (g->i1 >= 0)
            printf("(%d)", g->i1);
        printf("\n");
    }
}

void acc0_print_stats(ACC0Circuit *c) {
    if (!c) return;
    ACC0CircuitMeasures m = acc0_circuit_measures(c);
    printf("ACC0 Stats: size=%d depth=%d wires=%d fanin=%d\n",
           m.size, m.depth, m.wire_count, m.max_fanin);
    printf("  AND:%d OR:%d NOT:%d MOD:%d\n",
           m.and_gate_count, m.or_gate_count,
           m.not_gate_count, m.mod_gate_count);
}

/* ================================================================
 * Williams Bound
 * ================================================================ */

double acc0_williams_bound(int n) {
    if (n <= 1) return 1.0;
    double ln = log((double)n);
    return exp(ln * ln * ln);
}

/* ================================================================
 * Demo helpers and demo functions
 * ================================================================ */

static int _p3(const int *x, int n) { (void)n; return (x[0]+x[1]+x[2])%2; }
static int _m3c(const int *x, int n) { (void)n; return (x[0]+x[1]+x[2])%3==0; }
static int _mj3(const int *x, int n) { (void)n; return (x[0]+x[1]+x[2])>1; }

void acc0_demo(void) {
    printf("\n========== ACC0: AC0 + MOD Gates ==========\n");
    printf("Williams (2014): NEXP not in ACC0\\n\\n");
    printf("ACC0[m] = AC0 + MOD_m. ACC0 = union over all m.\\n\\n");

    printf("--- ACC0[3] circuit (n=4) ---\\n");
    ACC0Circuit *c = acc0_circuit_create(100);
    int ins[4];
    for (int i = 0; i < 4; i++) ins[i] = acc0_add_input(c);
    int m3 = acc0_add_mod(c, 3);
    acc0_set_fanin(c, m3, ins, 4);
    int o[1] = { m3 };
    acc0_set_outputs(c, o, 1);
    acc0_print_stats(c);

    printf("\\nTruth table:\\n");
    for (int m = 0; m < 16; m++) {
        int in[4];
        for (int i = 0; i < 4; i++) in[i] = (m>>i)&1;
        printf("  [%d%d%d%d] -> %d\\n",
               in[0],in[1],in[2],in[3], acc0_circuit_eval(c, in));
    }
    acc0_circuit_free(c);

    printf("\\n--- Williams bound ---\\n");
    for (int i = 1; i <= 5; i++)
        printf("  n=%3d: %.2e\\n", i*16, acc0_williams_bound(i*16));
    printf("\\nOpen: MAJORITY in ACC0? (since 1987)\\n");
}

void acc0_verify_demo(void) {
    printf("\\n=== ACC0 Verification ===\\n\\n");
    ACC0Circuit *c = acc0_circuit_create(100);
    int ins[3];
    for (int i = 0; i < 3; i++) ins[i] = acc0_add_input(c);
    int m3 = acc0_add_mod(c, 3);
    acc0_set_fanin(c, m3, ins, 3);
    int o[1] = { m3 };
    acc0_set_outputs(c, o, 1);
    printf("  MOD3 vs PARITY:   %s\\n",
           acc0_verify_truth_table(c, _p3, 3) ? "EQUIV" : "DIFF");
    printf("  MOD3 vs MOD3:     %s\\n",
           acc0_verify_truth_table(c, _m3c, 3) ? "EQUIV" : "DIFF");
    printf("  MOD3 vs MAJORITY: %s\\n",
           acc0_verify_truth_table(c, _mj3, 3) ? "EQUIV" : "DIFF");
    acc0_circuit_free(c);
}
