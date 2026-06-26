/* circuit_app.c -- Boolean Circuit Applications
 *
 * L7: SAT solving, formal verification, OWF, benchmarking.
 */

#include "circuit.h"
#include "circuit_analysis.h"
#include "circuit_complexity.h"
#include "circuit_builder.h"
#include "circuit_synthesis.h"
#include <assert.h>
#include <string.h>

typedef struct { int* a; int nv, nd, nb; } DPLLSolver;

static DPLLSolver* dpll_create(int nv) {
    DPLLSolver* s = (DPLLSolver*)malloc(sizeof(DPLLSolver));
    assert(s);
    s->nv = nv; s->nd = 0; s->nb = 0;
    s->a = (int*)calloc((size_t)nv, sizeof(int));
    assert(s->a);
    return s;
}

static void dpll_free(DPLLSolver* s) {
    if (s) { free(s->a); free(s); }
}

static int dpll_eval(BooleanCircuit* c, const DPLLSolver* s) {
    int* in = (int*)malloc((size_t)s->nv * sizeof(int));
    assert(in);
    for (int i = 0; i < s->nv; i++)
        in[i] = (s->a[i] == 2) ? 1 : 0;
    int r = circuit_evaluate(c, in);
    free(in);
    return r;
}

static int dpll_rec(BooleanCircuit* c, DPLLSolver* s, int var) {
    if (var >= s->nv) return dpll_eval(c, s);
    s->a[var] = 1; s->nd++;
    if (dpll_rec(c, s, var + 1)) return 1;
    s->a[var] = 2; s->nb++;
    if (dpll_rec(c, s, var + 1)) return 1;
    s->a[var] = 0;
    return 0;
}

int circuit_sat_solve(BooleanCircuit* c, int* out) {
    assert(c && out);
    int nv = c->n_inputs;
    if (nv > 16) return -1;
    DPLLSolver* s = dpll_create(nv);
    int f = dpll_rec(c, s, 0);
    if (f) {
        for (int i = 0; i < nv; i++)
            out[i] = (s->a[i] == 2) ? 1 : 0;
    }
    dpll_free(s);
    return f;
}

typedef struct { int nc, ne, nd; long long fce; int* cei; } VerifRes;

VerifRes* verify_equiv(BooleanCircuit* a, BooleanCircuit* b, int ni) {
    assert(a && b && ni > 0 && ni <= 16);
    VerifRes* v = (VerifRes*)malloc(sizeof(VerifRes));
    assert(v);
    v->nc = 0; v->ne = 0; v->nd = 0; v->fce = -1;
    v->cei = (int*)calloc((size_t)ni, sizeof(int));
    assert(v->cei);
    long long tot = 1LL << ni;
    int* in = (int*)calloc((size_t)ni, sizeof(int));
    assert(in);
    for (long long t = 0; t < tot; t++) {
        v->nc++;
        for (int j = 0; j < ni; j++)
            in[j] = (int)((t >> j) & 1);
        if (circuit_evaluate(a, in) == circuit_evaluate(b, in))
            v->ne++;
        else {
            v->nd++;
            if (v->fce < 0) {
                v->fce = t;
                for (int j = 0; j < ni; j++)
                    v->cei[j] = in[j];
            }
        }
    }
    free(in);
    return v;
}

void verify_free(VerifRes* v) { if (v) { free(v->cei); free(v); } }

void verify_print(const VerifRes* v) {
    if (!v) return;
    printf("Verify: %d checked, %d eq, %d diff\n", v->nc, v->ne, v->nd);
    if (v->fce >= 0)
        printf("  First diff at input %lld\n", v->fce);
}

BooleanCircuit* circuit_owf_multiply(int n) {
    assert(n > 0 && n <= 8);
    BooleanCircuit* c = circuit_create(n * n * 20 + 200);
    int* a = (int*)malloc((size_t)n * sizeof(int)); assert(a);
    int* b = (int*)malloc((size_t)n * sizeof(int)); assert(b);
    for (int i = 0; i < n; i++) {
        a[i] = circuit_add_gate(c, INPUT, -1, -1);
        b[i] = circuit_add_gate(c, INPUT, -1, -1);
    }
    int** pp = (int**)malloc((size_t)n * sizeof(int*)); assert(pp);
    for (int i = 0; i < n; i++) {
        pp[i] = (int*)malloc((size_t)n * sizeof(int)); assert(pp[i]);
        for (int j = 0; j < n; j++)
            pp[i][j] = circuit_add_gate(c, AND, a[i], b[j]);
    }
    int* acc = (int*)calloc((size_t)(2 * n), sizeof(int)); assert(acc);
    for (int i = 0; i < 2 * n; i++)
        acc[i] = circuit_add_gate(c, CONST0, -1, -1);
    for (int i = 0; i < n; i++) {
        int cy = circuit_add_gate(c, CONST0, -1, -1);
        for (int j = 0; j < n; j++) {
            int pos = i + j; if (pos >= 2 * n) break;
            int s1 = circuit_add_gate(c, XOR, acc[pos], pp[i][j]);
            int nb = circuit_add_gate(c, XOR, s1, cy);
            int c1 = circuit_add_gate(c, AND, acc[pos], pp[i][j]);
            int c2 = circuit_add_gate(c, AND, acc[pos], cy);
            int c3 = circuit_add_gate(c, AND, pp[i][j], cy);
            int c12 = circuit_add_gate(c, OR, c1, c2);
            cy = circuit_add_gate(c, OR, c12, c3);
            acc[pos] = nb;
        }
    }
    circuit_set_output(c, acc, 2 * n);
    for (int i = 0; i < n; i++) free(pp[i]);
    free(pp); free(acc); free(b); free(a);
    return c;
}

void circuit_complexity_benchmark(const BooleanCircuit* c) {
    assert(c);
    printf("=== Complexity Benchmark ===\n");
    printf("Size: %d  Depth: %d  Width: %d\n",
           circuit_size(c), circuit_depth(c), circuit_width(c));
    printf("Inputs: %d  Outputs: %d  Edges: %d\n",
           circuit_input_count(c), circuit_output_count(c),
           circuit_edge_count(c));
    printf("Density: %.6f  Size*Depth: %.0f\n",
           circuit_density(c), circuit_complexity_product(c));
    printf("Avg fan-in: %.2f  Avg fan-out: %.2f\n",
           circuit_avg_fanin(c), circuit_avg_fanout(c));
    printf("Is DAG: %s  Class: %s\n",
           circuit_is_dag(c) ? "yes" : "no",
           circuit_class_name(circuit_determine_class(c)));
    int n = circuit_input_count(c);
    if (n > 0 && n <= 8) {
        long long ns = circuit_count_sat_assignments(
            (BooleanCircuit*)c, n);
        printf("SAT: %lld / %lld\n", ns, 1LL << n);
    }
    if (n > 0)
        printf("Shannon LB(n=%d): %.0f\n", n, shannon_lower_bound(n));
    printf("====================================\n");
}

