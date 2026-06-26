/* circuit_depth_analysis.c -- Circuit Depth and Structure Analysis
 *
 * Analyzes the structural properties of Boolean circuits:
 * depth, width, topological ordering, alternating layers,
 * gate distribution, and critical path identification.
 *
 * Depth is crucial for circuit complexity classes:
 *   AC0: constant depth
 *   NC1: O(log n) depth
 *   NC: polylog depth
 *
 * References:
 *   Vollmer (1999) "Introduction to Circuit Complexity" Ch.4
 *   Jukna (2012) "Boolean Function Complexity" Ch.3
 */

#include "csat.h"

/* Compute exact circuit depth using topological traversal.
 * Depth = length of longest path from any input to any output.
 * Complexity: O(n_gates). */
int circuit_depth_exact(const BooleanCircuit* c)
{
    if (!c || c->n_gates <= 0) return 0;
    int* depths = (int*)malloc((size_t)c->n_gates * sizeof(int));
    if (!depths) return 0;
    /* Initialize: inputs at depth 0 */
    for (int i = 0; i < c->n_gates; i++) {
        const Gate* g = circuit_get_gate(c, i);
        if (!g) { depths[i] = 0; continue; }
        if (g->type == INPUT || g->type == CONST0 || g->type == CONST1)
            depths[i] = 0;
        else
            depths[i] = -1;  /* uncomputed */
    }
    /* Forward pass: compute depth for each gate */
    int changed = 1;
    while (changed) {
        changed = 0;
        for (int i = 0; i < c->n_gates; i++) {
            const Gate* g = circuit_get_gate(c, i);
            if (!g || depths[i] >= 0) continue;
            int d1 = -1, d2 = -1;
            if (g->input1 >= 0 && g->input1 < c->n_gates)
                d1 = depths[g->input1];
            if (g->input2 >= 0 && g->input2 < c->n_gates)
                d2 = depths[g->input2];
            /* For fan-in gates, consider all fan-in depths */
            if (g->fanin > 0 && g->fanin_ids) {
                for (int j = 0; j < g->fanin; j++) {
                    int fid = g->fanin_ids[j];
                    if (fid >= 0 && fid < c->n_gates && depths[fid] > d1)
                        d1 = depths[fid];
                }
            }
            if (d1 >= 0 || d2 >= 0) {
                int maxd = d1 > d2 ? d1 : d2;
                if (maxd < 0) maxd = d1 >= 0 ? d1 : d2;
                depths[i] = maxd + 1;
                changed = 1;
            }
        }
    }
    /* Find max depth among outputs */
    int max_depth = 0;
    for (int i = 0; i < c->n_gates; i++) {
        if (depths[i] > max_depth) max_depth = depths[i];
    }
    /* Also check specifically output gates */
    if (c->output_ids) {
        for (int i = 0; i < c->n_outputs; i++) {
            int oid = c->output_ids[i];
            if (oid >= 0 && oid < c->n_gates && depths[oid] > max_depth)
                max_depth = depths[oid];
        }
    }
    free(depths);
    return max_depth;
}

/* Depth-exact analysis uses circuit_depth_exact() above.
 * circuit_topological_order() and circuit_level_widths()
 * are provided by the mini-boolean-circuits-model module. */

/* Check if circuit has alternating AND/OR layers.
 * A circuit is alternating if no two adjacent gates on any path
 * are of the same type (AND/OR). Useful for AC0 analysis.
 * Complexity: O(n_gates). */
int circuit_is_alternating(const BooleanCircuit* c)
{
    if (!c) return 0;
    for (int i = 0; i < c->n_gates; i++) {
        const Gate* g = circuit_get_gate(c, i);
        if (!g) continue;
        if (g->type != AND && g->type != OR) continue;
        /* Check each input */
        for (int j = 0; j < 2; j++) {
            int inp = (j == 0) ? g->input1 : g->input2;
            if (inp < 0 || inp >= c->n_gates) continue;
            const Gate* ig = circuit_get_gate(c, inp);
            if (!ig) continue;
            if ((g->type == AND && ig->type == AND) ||
                (g->type == OR && ig->type == OR))
                return 0;  /* consecutive same type */
        }
    }
    return 1;
}

/* Count gate type distribution.
 * ands, ors, nots, xors are output counters.
 * Complexity: O(n_gates). */
void circuit_gate_distribution(const BooleanCircuit* c,
    int* ands, int* ors, int* nots, int* xors)
{
    if (ands)  *ands  = 0;
    if (ors)   *ors   = 0;
    if (nots)  *nots  = 0;
    if (xors)  *xors  = 0;
    if (!c) return;
    for (int i = 0; i < c->n_gates; i++) {
        const Gate* g = circuit_get_gate(c, i);
        if (!g) continue;
        switch (g->type) {
            case AND:  if (ands) (*ands)++; break;
            case OR:   if (ors)  (*ors)++;  break;
            case NOT:  if (nots) (*nots)++; break;
            case XOR:  if (xors) (*xors)++; break;
            default: break;
        }
    }
}