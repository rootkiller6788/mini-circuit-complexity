/* nc_core.c -- Core Boolean Circuit Library
 *
 * A Boolean circuit is a DAG of logic gates. This module implements
 * construction, evaluation, depth computation, analysis, and classification.
 *
 * Mathematical model (L3):
 *   C = (V, E, type, I, O) — DAG with typed gates
 *
 * Gate semantics (L1):
 *   AND(v₁,...,vₖ)   = v₁ ∧ v₂ ∧ ... ∧ vₖ
 *   OR(v₁,...,vₖ)    = v₁ ∨ v₂ ∨ ... ∨ vₖ
 *   NOT(v)           = ¬v
 *   XOR(v₁,...,vₖ)   = v₁ ⊕ v₂ ⊕ ... ⊕ vₖ
 *   MAJORITY(v₁,...) = 1 iff Σvᵢ > k/2
 *   THRESHOLDₖ(v₁,..)= 1 iff Σvᵢ ≥ k
 *
 * References:
 *   Arora & Barak (2009) Ch.6: Circuit Complexity
 *   Vollmer (1999): Introduction to Circuit Complexity
 *   Pippenger (1979): On simultaneous resource bounds
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include "nc.h"

/* ---- Internal: grow circuit gate array on demand ---- */
static void grow(BoolCircuit *c) {
    if (c->num_gates >= c->capacity) {
        int ncap = c->capacity * 2;
        NCGate *ng = (NCGate *)realloc(c->gates, (size_t)ncap * sizeof(NCGate));
        assert(ng);
        for (int i = c->capacity; i < ncap; i++) {
            ng[i].type = NC_INPUT; ng[i].inputs = NULL; ng[i].num_inputs = 0;
            ng[i].input_capacity = 0; ng[i].threshold = 0;
            ng[i].input_bit = -1; ng[i].output_index = -1;
            ng[i].cached_value = -1; ng[i].topological_order = -1;
        }
        c->gates = ng; c->capacity = ncap;
    }
}

BoolCircuit *circuit_create(int icap, int mfi) {
    BoolCircuit *c = (BoolCircuit *)malloc(sizeof(BoolCircuit));
    assert(c);
    if (icap < 16) icap = 16;
    c->capacity = icap;
    c->gates = (NCGate *)calloc((size_t)icap, sizeof(NCGate));
    assert(c->gates);
    c->num_gates = 0; c->num_inputs = 0; c->num_outputs = 0;
    c->output_gates = NULL; c->max_fan_in = mfi; c->log_depth = 0.0;
    c->has_been_toposorted = 0;
    for (int i = 0; i < icap; i++) {
        c->gates[i].type = NC_INPUT; c->gates[i].inputs = NULL;
        c->gates[i].num_inputs = 0; c->gates[i].input_capacity = 0;
        c->gates[i].threshold = 0; c->gates[i].input_bit = -1;
        c->gates[i].output_index = -1; c->gates[i].cached_value = -1;
        c->gates[i].topological_order = -1;
    }
    return c;
}

void circuit_free(BoolCircuit *c) {
    if (!c) return;
    for (int i = 0; i < c->num_gates; i++) free(c->gates[i].inputs);
    free(c->gates); free(c->output_gates); free(c);
}

int circuit_add_gate(BoolCircuit *c, NCGateType t, const int *in, int nin) {
    grow(c);
    int id = c->num_gates++;
    NCGate *g = &c->gates[id];
    g->type = t; g->num_inputs = nin;
    g->input_bit = -1; g->output_index = -1;
    g->cached_value = -1; g->topological_order = -1;
    if (nin > 0) {
        g->input_capacity = (nin < 4) ? 4 : nin;
        g->inputs = (int *)malloc((size_t)g->input_capacity * sizeof(int));
        assert(g->inputs);
        memcpy(g->inputs, in, (size_t)nin * sizeof(int));
    } else { g->inputs = NULL; g->input_capacity = 0; }
    g->threshold = (t == NC_MAJORITY && nin > 0) ? nin / 2 : 0;
    return id;
}

int circuit_add_input(BoolCircuit *c, int vidx) {
    grow(c);
    int id = c->num_gates++;
    NCGate *g = &c->gates[id];
    g->type = NC_INPUT; g->inputs = NULL; g->num_inputs = 0;
    g->input_capacity = 0; g->threshold = 0;
    g->input_bit = vidx; g->output_index = -1;
    g->cached_value = -1; g->topological_order = -1;
    if (vidx >= c->num_inputs) c->num_inputs = vidx + 1;
    return id;
}

int circuit_add_output(BoolCircuit *c, int gi) {
    assert(gi >= 0 && gi < c->num_gates);
    int no = c->num_outputs++;
    c->output_gates = (int *)realloc(c->output_gates, (size_t)c->num_outputs * sizeof(int));
    assert(c->output_gates);
    c->output_gates[no] = gi;
    c->gates[gi].output_index = no;
    return no;
}

int circuit_add_constant(BoolCircuit *c, int v) {
    return circuit_add_gate(c, v ? NC_CONST_ONE : NC_CONST_ZERO, NULL, 0);
}

/* ================================================================
 * EVALUATION — Recursive with memoization, O(V+E)
 * ================================================================ */

int circuit_evaluate_gate(BoolCircuit *c, int gid, const int *iv, int *vis) {
    if (vis[gid] >= 0) return vis[gid];
    NCGate *g = &c->gates[gid]; int r = 0;
    switch (g->type) {
    case NC_INPUT:
        assert(g->input_bit >= 0 && g->input_bit < c->num_inputs);
        r = iv[g->input_bit]; break;
    case NC_CONST_ZERO: r = 0; break;
    case NC_CONST_ONE:  r = 1; break;
    case NC_NOT:
        assert(g->num_inputs == 1);
        r = !circuit_evaluate_gate(c, g->inputs[0], iv, vis); break;
    case NC_AND: r = 1;
        for (int i = 0; i < g->num_inputs; i++)
            r &= circuit_evaluate_gate(c, g->inputs[i], iv, vis); break;
    case NC_NAND: r = 1;
        for (int i = 0; i < g->num_inputs; i++)
            r &= circuit_evaluate_gate(c, g->inputs[i], iv, vis);
        r = !r; break;
    case NC_OR: r = 0;
        for (int i = 0; i < g->num_inputs; i++)
            r |= circuit_evaluate_gate(c, g->inputs[i], iv, vis); break;
    case NC_NOR: r = 0;
        for (int i = 0; i < g->num_inputs; i++)
            r |= circuit_evaluate_gate(c, g->inputs[i], iv, vis);
        r = !r; break;
    case NC_XOR:
        for (int i = 0; i < g->num_inputs; i++)
            r ^= circuit_evaluate_gate(c, g->inputs[i], iv, vis); break;
    case NC_MAJORITY: {
        int cnt = 0;
        for (int i = 0; i < g->num_inputs; i++)
            cnt += circuit_evaluate_gate(c, g->inputs[i], iv, vis);
        r = (cnt > g->num_inputs / 2) ? 1 : 0; break;
    }
    case NC_THRESHOLD: {
        int cnt = 0;
        for (int i = 0; i < g->num_inputs; i++)
            cnt += circuit_evaluate_gate(c, g->inputs[i], iv, vis);
        r = (cnt > g->threshold) ? 1 : 0; break;
    }
    case NC_MOD3: {
        int cnt = 0;
        for (int i = 0; i < g->num_inputs; i++)
            cnt += circuit_evaluate_gate(c, g->inputs[i], iv, vis);
        r = (cnt % 3 == 0) ? 1 : 0; break;
    }
    case NC_OUTPUT:
        assert(g->num_inputs == 1);
        r = circuit_evaluate_gate(c, g->inputs[0], iv, vis); break;
    default: r = 0;
    }
    vis[gid] = r; g->cached_value = r;
    return r;
}

int *circuit_evaluate(BoolCircuit *c, const int *iv) {
    int *vis = (int *)malloc((size_t)c->num_gates * sizeof(int));
    assert(vis);
    for (int i = 0; i < c->num_gates; i++) { vis[i] = -1; c->gates[i].cached_value = -1; }
    int *out = (int *)malloc((size_t)c->num_outputs * sizeof(int));
    assert(out);
    for (int i = 0; i < c->num_outputs; i++)
        out[i] = circuit_evaluate_gate(c, c->output_gates[i], iv, vis);
    free(vis);
    return out;
}

/* ================================================================
 * DEPTH COMPUTATION — Longest input-to-output path
 * ================================================================ */

int circuit_depth(BoolCircuit *c) {
    int *d = (int *)malloc((size_t)c->num_gates * sizeof(int));
    assert(d);
    for (int i = 0; i < c->num_gates; i++) d[i] = -1;
    int ch = 1;
    while (ch) {
        ch = 0;
        for (int i = 0; i < c->num_gates; i++) {
            NCGate *g = &c->gates[i];
            if (g->type == NC_INPUT || g->type == NC_CONST_ZERO || g->type == NC_CONST_ONE) {
                if (d[i] != 0) { d[i] = 0; ch = 1; } continue;
            }
            int mc = 0, ok = 1;
            for (int j = 0; j < g->num_inputs; j++) {
                int cd = d[g->inputs[j]];
                if (cd < 0) { ok = 0; break; }
                if (cd > mc) mc = cd;
            }
            if (ok && d[i] != mc + 1) { d[i] = mc + 1; ch = 1; }
        }
    }
    int md = 0;
    for (int i = 0; i < c->num_outputs; i++)
        if (d[c->output_gates[i]] > md) md = d[c->output_gates[i]];
    free(d);
    return md;
}

/* ================================================================
 * CIRCUIT ANALYSIS
 * ================================================================ */

int circuit_size(BoolCircuit *c) { return c->num_gates; }

int circuit_num_gates_of_type(BoolCircuit *c, NCGateType t) {
    int n = 0;
    for (int i = 0; i < c->num_gates; i++)
        if (c->gates[i].type == t) n++;
    return n;
}

int circuit_fan_in_sum(BoolCircuit *c) {
    int s = 0;
    for (int i = 0; i < c->num_gates; i++) s += c->gates[i].num_inputs;
    return s;
}

int circuit_is_monotone(BoolCircuit *c) {
    for (int i = 0; i < c->num_gates; i++) {
        NCGateType t = c->gates[i].type;
        if (t == NC_NOT || t == NC_NAND || t == NC_NOR || t == NC_XOR) return 0;
    }
    return 1;
}

int circuit_is_tree(BoolCircuit *c) {
    int *fo = (int *)calloc((size_t)c->num_gates, sizeof(int));
    assert(fo);
    for (int i = 0; i < c->num_gates; i++)
        for (int j = 0; j < c->gates[i].num_inputs; j++) {
            int chld = c->gates[i].inputs[j];
            fo[chld]++;
            if (fo[chld] > 1) { free(fo); return 0; }
        }
    free(fo);
    return 1;
}

int circuit_is_formula(BoolCircuit *c) { return circuit_is_tree(c); }

/* Cycle detection: 3-color DFS */
int circuit_has_cycle(BoolCircuit *c) {
    int *col = (int *)calloc((size_t)c->num_gates, sizeof(int));
    assert(col);
    for (int s = 0; s < c->num_gates; s++) {
        if (col[s] != 0) continue;
        int *stk = (int *)malloc((size_t)(c->num_gates + 1) * sizeof(int));
        int *spos = (int *)calloc((size_t)(c->num_gates + 1), sizeof(int));
        int sp = 0;
        stk[0] = s; spos[0] = 0; col[s] = 1; sp = 1;
        while (sp > 0) {
            int v = stk[sp-1], p = spos[sp-1];
            if (p >= c->gates[v].num_inputs) { col[v] = 2; sp--; continue; }
            int ch = c->gates[v].inputs[p];
            spos[sp-1] = p + 1;
            if (col[ch] == 1) { free(stk); free(spos); free(col); return 1; }
            if (col[ch] == 0) { col[ch] = 1; stk[sp] = ch; spos[sp] = 0; sp++; }
        }
        free(stk); free(spos);
    }
    free(col);
    return 0;
}

void circuit_compute_topological_order(BoolCircuit *c) {
    if (c->has_been_toposorted) return;
    int *idg = (int *)calloc((size_t)c->num_gates, sizeof(int));
    for (int i = 0; i < c->num_gates; i++)
        for (int j = 0; j < c->gates[i].num_inputs; j++)
            idg[c->gates[i].inputs[j]]++;
    int *q = (int *)malloc((size_t)c->num_gates * sizeof(int));
    int hd = 0, tl = 0;
    for (int i = 0; i < c->num_gates; i++)
        if (idg[i] == 0) q[tl++] = i;
    int ord = 0;
    while (hd < tl) {
        int v = q[hd++];
        c->gates[v].topological_order = ord++;
        for (int i = 0; i < c->num_gates; i++)
            for (int j = 0; j < c->gates[i].num_inputs; j++)
                if (c->gates[i].inputs[j] == v) {
                    idg[i]--;
                    if (idg[i] == 0) q[tl++] = i;
                }
    }
    free(idg); free(q);
    c->has_been_toposorted = 1;
}

/* ================================================================
 * PRINTING
 * ================================================================ */

void circuit_print_stats(BoolCircuit *c) {
    printf("Circuit Stats: gates=%d (in=%d, out=%d) max_fan_in=%d depth=%d wires=%d\n",
           c->num_gates, c->num_inputs, c->num_outputs,
           c->max_fan_in, circuit_depth(c), circuit_fan_in_sum(c));
    printf("  AND:%d OR:%d NOT:%d XOR:%d MAJ:%d monotone:%s tree:%s cycle:%s\n",
           circuit_num_gates_of_type(c, NC_AND),
           circuit_num_gates_of_type(c, NC_OR),
           circuit_num_gates_of_type(c, NC_NOT),
           circuit_num_gates_of_type(c, NC_XOR),
           circuit_num_gates_of_type(c, NC_MAJORITY),
           circuit_is_monotone(c) ? "yes" : "no",
           circuit_is_tree(c) ? "yes" : "no",
           circuit_has_cycle(c) ? "YES(ERR)" : "no");
}

void circuit_print_dot(BoolCircuit *c, FILE *f) {
    if (!f) return;
    static const char *col[] = {
        [NC_INPUT]="lightyellow",[NC_CONST_ZERO]="gray90",[NC_CONST_ONE]="gray70",
        [NC_NOT]="lightcoral",[NC_AND]="lightblue",[NC_OR]="lightgreen",
        [NC_NAND]="lightskyblue",[NC_NOR]="lightseagreen",[NC_XOR]="plum",
        [NC_MAJORITY]="orange",[NC_THRESHOLD]="darkorange",[NC_MOD3]="violet",
        [NC_OUTPUT]="gold"};
    static const char *nm[] = {
        [NC_INPUT]="IN",[NC_CONST_ZERO]="0",[NC_CONST_ONE]="1",
        [NC_NOT]="NOT",[NC_AND]="AND",[NC_OR]="OR",
        [NC_NAND]="NAND",[NC_NOR]="NOR",[NC_XOR]="XOR",
        [NC_MAJORITY]="MAJ",[NC_THRESHOLD]="THR",[NC_MOD3]="MOD3",
        [NC_OUTPUT]="OUT"};
    fprintf(f, "digraph C {\n  rankdir=TB;\n");
    for (int i = 0; i < c->num_gates; i++) {
        NCGate *g = &c->gates[i];
        fprintf(f, "  g%d [label=\"%s", i, nm[g->type]);
        if (g->type == NC_INPUT) fprintf(f, " x%d", g->input_bit);
        fprintf(f, "\", fillcolor=\"%s\"];\n", col[g->type]);
        for (int j = 0; j < g->num_inputs; j++)
            fprintf(f, "  g%d -> g%d;\n", g->inputs[j], i);
    }
    fprintf(f, "}\n");
}

/* ================================================================
 * CIRCUIT FAMILY
 * ================================================================ */

CircuitFamily *circuit_family_create(const char *nm, int mx) {
    CircuitFamily *cf = (CircuitFamily *)malloc(sizeof(CircuitFamily));
    assert(cf);
    cf->name = strdup(nm);
    cf->max_n = mx; cf->min_n = 1;
    cf->circuits = (BoolCircuit **)calloc((size_t)(mx + 1), sizeof(BoolCircuit *));
    assert(cf->circuits);
    return cf;
}

void circuit_family_free(CircuitFamily *cf) {
    if (!cf) return;
    for (int n = cf->min_n; n <= cf->max_n; n++)
        if (cf->circuits[n]) circuit_free(cf->circuits[n]);
    free(cf->circuits); free(cf->name); free(cf);
}

void circuit_family_set(CircuitFamily *cf, int n, BoolCircuit *c) {
    assert(n >= cf->min_n && n <= cf->max_n);
    if (cf->circuits[n]) circuit_free(cf->circuits[n]);
    cf->circuits[n] = c;
}

BoolCircuit *circuit_family_get(CircuitFamily *cf, int n) {
    if (n < cf->min_n || n > cf->max_n) return NULL;
    return cf->circuits[n];
}

int circuit_family_is_uniform(CircuitFamily *cf, UniformityType ut) {
    if (ut == UNIFORM_NONE) return 1;
    for (int n = cf->min_n; n <= cf->max_n; n++)
        if (!cf->circuits[n]) return 0;
    return 1;
}

/* ================================================================
 * NC CLASSIFICATION
 * ================================================================ */

double nc_i_depth_bound(int i, int n) {
    if (n <= 1) return 1.0;
    double l = log2((double)n), b = 1.0;
    for (int j = 0; j < i; j++) b *= l;
    return b;
}

int is_depth_nc_i(double dep, int i, int n) {
    if (n <= 1) return (dep <= 1.0);
    return (dep <= 10.0 * nc_i_depth_bound(i, n));
}

ClassBounds circuit_classify(const BoolCircuit *c, int isize) {
    ClassBounds b; memset(&b, 0, sizeof(b));
    int d = circuit_depth(c), s = circuit_size(c);
    double lg = (isize > 1) ? log2((double)isize) : 1.0;
    b.gate_types = 0;
    for (int i = 0; i < c->num_gates; i++)
        b.gate_types |= (1 << (int)c->gates[i].type);
    int hm = (b.gate_types & (1<<NC_MAJORITY)) || (b.gate_types & (1<<NC_THRESHOLD));
    int hmod = b.gate_types & (1<<NC_MOD3);
    int ub = (c->max_fan_in > 100);
    if (d <= 3 && !ub && !hm && !hmod) {
        b.class_id = CLASS_NC0; b.depth_class = 0;
        snprintf(b.description, 256, "NC^0: d=%d s=%d", d, s);
    } else if (d <= 3 && ub && !hm && !hmod) {
        b.class_id = CLASS_AC0; b.depth_class = 0;
        snprintf(b.description, 256, "AC^0: d=%d(unb) s=%d", d, s);
    } else if (d <= 3 && hm) {
        b.class_id = CLASS_TC0; b.depth_class = 0;
        snprintf(b.description, 256, "TC^0: d=%d(MAJ) s=%d", d, s);
    } else if (is_depth_nc_i((double)d, 1, isize)) {
        b.class_id = CLASS_NC1; b.depth_class = 1;
        snprintf(b.description, 256, "NC^1: d=%d O(logn)=%.1f s=%d", d, lg, s);
    } else if (is_depth_nc_i((double)d, 2, isize)) {
        b.class_id = CLASS_NC2; b.depth_class = 2;
        snprintf(b.description, 256, "NC^2: d=%d O(log^2n)=%.1f s=%d", d, lg*lg, s);
    } else if (is_depth_nc_i((double)d, 3, isize)) {
        b.class_id = CLASS_NC3; b.depth_class = 3;
        snprintf(b.description, 256, "NC^3: d=%d O(log^3n)=%.1f s=%d", d, lg*lg*lg, s);
    } else {
        b.class_id = CLASS_P; b.depth_class = -1;
        snprintf(b.description, 256, "P/poly: d=%d(>polylog) s=%d", d, s);
    }
    b.size_polynomial = (lg > 0) ? log2((double)s) / lg : 0.0;
    b.is_uniform = 1;
    return b;
}

int circuit_is_nc_i(const BoolCircuit *c, int i, int isize) {
    ClassBounds b = circuit_classify(c, isize);
    if (i == 0) return (b.class_id == CLASS_NC0 || b.class_id == CLASS_AC0 || b.class_id == CLASS_TC0);
    return (b.depth_class <= i && b.depth_class > 0);
}

ClassBounds circuit_family_classify(CircuitFamily *cf) {
    ClassBounds w; memset(&w, 0, sizeof(w)); w.class_id = CLASS_NC0;
    for (int n = cf->min_n; n <= cf->max_n; n++) {
        if (!cf->circuits[n]) continue;
        ClassBounds cb = circuit_classify(cf->circuits[n], n);
        if (cb.class_id > w.class_id || cb.depth_class > w.depth_class) w = cb;
    }
    return w;
}

void nc_hierarchy_depth_table(int mx) {
    printf("\n    NC^i Depth Bounds: O(log^i n)\n");
    printf("    n        log n    NC^0     NC^1     NC^2     NC^3\n");
    printf("    -------- -------- -------- -------- -------- --------\n");
    for (int n = 2; n <= mx; n *= 2)
        printf("    %-8d %-8.2f %-8.1f %-8.1f %-8.1f %-8.1f\n",
               n, log2((double)n), nc_i_depth_bound(0,n), nc_i_depth_bound(1,n),
               nc_i_depth_bound(2,n), nc_i_depth_bound(3,n));
}

void nc_hierarchy_class_table(void) {
    printf("\n    NC/AC/TC Hierarchy\n");
    printf("    =====================================================\n");
    printf("    Class     Depth         Size      Fan-in     Gates\n");
    printf("    --------  ------------  --------  ---------  --------\n");
    printf("    NC^0      O(1)          poly(n)   O(1)       AND/OR/NOT\n");
    printf("    AC^0      O(1)          poly(n)   unbounded  AND/OR/NOT\n");
    printf("    ACC^0     O(1)          poly(n)   unbounded  AC^0+MOD_m\n");
    printf("    TC^0      O(1)          poly(n)   unbounded  MAJORITY\n");
    printf("    NC^1      O(log n)      poly(n)   O(1)       AND/OR/NOT\n");
    printf("    L         O(log n) space -        -          logspace TM\n");
    printf("    NL        O(log n) space -        -          nondet TM\n");
    printf("    NC^2      O(log^2 n)    poly(n)   O(1)       AND/OR/NOT\n");
    printf("    NC^i      O(log^i n)    poly(n)   O(1)       AND/OR/NOT\n");
    printf("    NC        polylog(n)    poly(n)   O(1)       AND/OR/NOT\n");
    printf("    P/poly    poly(n)       poly(n)   O(1)       AND/OR/NOT\n");
    printf("\n    Known:  NC^0 < AC^0 < TC^0 <= NC^1 <= L <= NL <= NC^2 <= NC <= P\n");
    printf("    Open:   TC^0 vs NC^1?  NC^1 vs L?  NC vs P?\n");
    printf("    NL = coNL (Immerman-Szelepcsenyi 1988)\n");
}
