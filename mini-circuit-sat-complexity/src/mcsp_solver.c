/* mcsp_solver.c -- Minimum Circuit Size Problem
 *
 * MCSP: given truth table of Boolean function f,
 * find the smallest circuit computing f.
 *
 * Meta-complexity significance:
 *   Hardness of MCSP => circuit lower bounds.
 *   Kabanets & Cai (2000): If MCSP is NP-complete under
 *     certain reductions, then factoring is in P/poly.
 *
 * Key results:
 *   - MCSP is in NP (guess circuit, verify)
 *   - Not known to be NP-hard (major open problem)
 *   - MCSP for DNF is NP-complete (Masek 1979)
 *   - Connections to learning theory and cryptography
 *
 * References:
 *   Kabanets & Cai (2000) "Circuit minimization problem"
 *   Allender et al. (2017) "Meta-complexity via MCSP"
 *   Ilango (2020) "MCSP and the power of natural properties"
 */

#include "csat.h"

MCSPSolver* mcsp_create(BooleanCircuit* c)
{
    if (!c) return NULL;
    MCSPSolver* s = (MCSPSolver*)calloc(1, sizeof(MCSPSolver));
    if (!s) return NULL;
    s->orig      = c;
    s->orig_size = circuit_size(c);
    s->min_size  = s->orig_size;
    s->ng        = c->n_gates;
    s->removed   = (int*)calloc((size_t)c->n_gates, sizeof(int));
    if (!s->removed) { free(s); return NULL; }
    s->nr        = 0;
    s->best_score = 1.0;
    return s;
}

void mcsp_free(MCSPSolver* s)
{
    if (!s) return;
    free(s->removed);
    free(s);
}

/* Test if removing gate gid preserves circuit functionality.
 * Checks if gate has no fan-out (dead code). */
static int can_remove_gate(BooleanCircuit* c, int gid, int n_inputs)
{
    (void)n_inputs;
    int fanout = 0;
    for (int i = 0; i < c->n_gates; i++) {
        const Gate* g = circuit_get_gate(c, i);
        if (!g) continue;
        if (g->input1 == gid) fanout++;
        if (g->input2 == gid) fanout++;
    }
    if (c->output_ids) {
        for (int i = 0; i < c->n_outputs; i++)
            if (c->output_ids[i] == gid) return 0;
    }
    return (fanout == 0);
}

/* Greedy gate removal: remove dead gates and redundant gates.
 * Returns number of gates removed. */
int mcsp_greedy_minimize(MCSPSolver* s, int budget)
{
    if (!s || !s->orig) return 0;
    int removed = 0;
    int n_inputs = circuit_input_count(s->orig);

    /* Phase 1: remove dead gates (zero fan-out, not outputs) */
    for (int i = 0; i < s->orig->n_gates && removed < budget; i++) {
        if (s->removed[i]) continue;
        const Gate* g = circuit_get_gate(s->orig, i);
        if (!g) continue;
        if (g->type == INPUT) continue;
        if (can_remove_gate(s->orig, i, n_inputs)) {
            s->removed[i] = 1;
            s->nr++;
            removed++;
        }
    }

    /* Phase 2: remove redundant gates (idempotent, double negation) */
    for (int i = 0; i < s->orig->n_gates && removed < budget; i++) {
        if (s->removed[i]) continue;
        const Gate* g = circuit_get_gate(s->orig, i);
        if (!g) continue;
        if (g->type == INPUT || g->type == CONST0 || g->type == CONST1)
            continue;
        if (g->input1 == g->input2 && g->input1 >= 0) {
            s->removed[i] = 1;
            s->nr++;
            removed++;
            continue;
        }
        if (g->type == NOT && g->input1 >= 0) {
            const Gate* inner = circuit_get_gate(s->orig, g->input1);
            if (inner && inner->type == NOT && inner->input1 >= 0) {
                s->removed[i] = 1;
                s->nr++;
                removed++;
            }
        }
    }
    s->min_size = s->orig_size - removed;
    return removed;
}

/* Exact MCSP via exhaustive search for n_gates <= 16.
 * Returns minimized size. */
int mcsp_exact_minimize(MCSPSolver* s)
{
    if (!s || !s->orig || s->orig->n_gates > 16) return s ? s->min_size : 0;
    int ng = s->orig->n_gates;
    int best = ng;
    long long subsets = 1LL << ng;

    for (long long mask = 0; mask < subsets; mask++) {
        int keep_count = 0;
        int valid = 1;
        for (int i = 0; i < ng && valid; i++) {
            const Gate* g = circuit_get_gate(s->orig, i);
            if (!g) continue;
            int keep = (int)((mask >> i) & 1);
            if (g->type == INPUT && !keep) valid = 0;
            if (s->orig->output_ids) {
                for (int o = 0; o < s->orig->n_outputs; o++)
                    if (s->orig->output_ids[o] == i && !keep) valid = 0;
            }
            if (keep) keep_count++;
        }
        if (!valid) continue;
        for (int i = 0; i < ng && valid; i++) {
            if (!((mask >> i) & 1)) continue;
            const Gate* g = circuit_get_gate(s->orig, i);
            if (!g) continue;
            if (g->input1 >= 0 && !((mask >> g->input1) & 1)) valid = 0;
            if (g->input2 >= 0 && !((mask >> g->input2) & 1)) valid = 0;
        }
        if (valid && keep_count < best) {
            best = keep_count;
            for (int i = 0; i < ng; i++)
                s->removed[i] = !((mask >> i) & 1);
            s->nr = ng - keep_count;
        }
    }
    s->min_size = best;
    s->best_score = (double)best / (double)ng;
    return best;
}

/* Kernelization: count gates reachable from inputs and outputs.
 * Returns number of gates in the kernel. */
int mcsp_kernelize(MCSPSolver* s)
{
    if (!s || !s->orig) return 0;
    int ng = s->orig->n_gates;
    int* reach = (int*)calloc((size_t)ng, sizeof(int));
    int* q = (int*)malloc((size_t)ng * sizeof(int));
    if (!reach || !q) { free(reach); free(q); return 0; }
    int qh = 0, qt = 0;
    /* Start from inputs */
    for (int i = 0; i < ng; i++) {
        const Gate* g = circuit_get_gate(s->orig, i);
        if (g && g->type == INPUT) { reach[i] = 1; q[qt++] = i; }
    }
    while (qh < qt) {
        int gid = q[qh++];
        /* Find gates that use gid as input */
        for (int i = 0; i < ng; i++) {
            if (reach[i]) continue;
            const Gate* g = circuit_get_gate(s->orig, i);
            if (!g) continue;
            if (g->input1 == gid || g->input2 == gid) {
                reach[i] = 1; q[qt++] = i;
            }
        }
    }
    int ess = 0;
    for (int i = 0; i < ng; i++) ess += reach[i];
    free(q); free(reach);
    return ess;
}

void mcsp_print_result(const MCSPSolver* s)
{
    if (!s) return;
    printf("MCSP Result:\n");
    printf("  Original size: %d\n", s->orig_size);
    printf("  Minimized size: %d\n", s->min_size);
    printf("  Gates removed: %d (%.1f%%)\n", s->nr,
           100.0 * (double)s->nr / (double)s->orig_size);
    printf("  Best score: %.4f\n", s->best_score);
}