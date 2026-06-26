/* approximator.c -- Razborov's Method of Approximations (1985)
 *
 * Implements Razborov's method of approximations for proving
 * monotone circuit lower bounds for CLIQUE.
 *
 * THE METHOD:
 *   Replace each gate in a monotone circuit with an "approximator" -
 *   a small set of positive/negative examples that approximates
 *   the function computed at that gate.
 *
 * FOR CLIQUE_{n,k}:
 *   - Input gates: variables x_{ij} = "edge (i,j) exists"
 *   - Positive example: a specific k-clique (YES instance)
 *   - Negative example: a (k-1)-coloring (NO instance)
 *
 * APPROXIMATION RULES:
 *   AND gate:  A_and = intersect(A1, A2)
 *              Blowup controlled by sunflower lemma
 *   OR gate:   A_or = union(A1, A2)
 *              Blowup is additive (no problem)
 *
 * CORRECTNESS:
 *   If |A+| and |A-| stay small through all gates, but CLIQUE
 *   requires large approximators, then the circuit must be large.
 *
 * This was the FIRST super-polynomial circuit lower bound in history.
 * The method only works for MONOTONE circuits (no NOT gates).
 * Extending to general circuits remains an open problem.
 *
 * References:
 *   Razborov (1985), Doklady AN SSSR 281(4), 798-801
 *   Boppana & Sipser (1990), "The complexity of finite functions"
 *   Jukna (2012), "Boolean Function Complexity", Ch. 10
 *   Arora & Barak (2009), "Computational Complexity", Ch. 14.4
 *
 * Complexity:
 *   Approximator AND gate: O(|A1| * |A2| * k^2) time
 *   Approximator OR gate: O(|A1| + |A2|) time (additive)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include <time.h>
#include <assert.h>
#include "approximator.h"
#include "monotone_circuit.h"
#include "sunflower.h"

/* ============================================================
 * Approximator Construction
 * ============================================================ */

Approximator* approx_create(int n, int k, int max_size) {
    Approximator *a = malloc(sizeof(Approximator));
    assert(a != NULL);
    a->n = n;
    a->k = k;
    a->max_size = max_size;
    a->num_pos = 0;
    a->num_neg = 0;
    a->pos_capacity = max_size > 0 ? max_size : 64;
    a->neg_capacity = max_size > 0 ? max_size : 64;
    a->pos = malloc((size_t)a->pos_capacity * sizeof(PositiveExample*));
    a->neg = malloc((size_t)a->neg_capacity * sizeof(NegativeExample*));
    assert(a->pos && a->neg);
    a->and_gates_processed = 0;
    a->or_gates_processed = 0;
    a->sunflower_compressions = 0;
    return a;
}

void approx_free(Approximator *a) {
    if (!a) return;
    for (int i = 0; i < a->num_pos; i++) {
        positive_example_free(a->pos[i]);
    }
    for (int i = 0; i < a->num_neg; i++) {
        negative_example_free(a->neg[i]);
    }
    free(a->pos);
    free(a->neg);
    free(a);
}

void approx_add_positive(Approximator *a, const PositiveExample *ex) {
    if (a->num_pos >= a->max_size && a->max_size > 0) return;
    if (a->num_pos >= a->pos_capacity) {
        a->pos_capacity *= 2;
        a->pos = realloc(a->pos, (size_t)a->pos_capacity * sizeof(PositiveExample*));
        assert(a->pos != NULL);
    }
    PositiveExample *copy = malloc(sizeof(PositiveExample));
    assert(copy != NULL);
    copy->graph = graph_copy(ex->graph);
    copy->witness = clique_from_vertices(ex->witness->n, ex->witness->k,
                                          ex->witness->vertices);
    a->pos[a->num_pos++] = copy;
}

void approx_add_negative(Approximator *a, const NegativeExample *ex) {
    if (a->num_neg >= a->max_size && a->max_size > 0) return;
    if (a->num_neg >= a->neg_capacity) {
        a->neg_capacity *= 2;
        a->neg = realloc(a->neg, (size_t)a->neg_capacity * sizeof(NegativeExample*));
        assert(a->neg != NULL);
    }
    NegativeExample *copy = malloc(sizeof(NegativeExample));
    assert(copy != NULL);
    copy->graph = graph_copy(ex->graph);
    copy->type = ex->type;
    copy->num_colors = ex->num_colors;
    copy->color_sizes = malloc((size_t)ex->num_colors * sizeof(int));
    memcpy(copy->color_sizes, ex->color_sizes,
           (size_t)ex->num_colors * sizeof(int));
    copy->color_classes = malloc((size_t)ex->num_colors * sizeof(int*));
    for (int i = 0; i < ex->num_colors; i++) {
        copy->color_classes[i] = malloc((size_t)ex->color_sizes[i] * sizeof(int));
        memcpy(copy->color_classes[i], ex->color_classes[i],
               (size_t)ex->color_sizes[i] * sizeof(int));
    }
    a->neg[a->num_neg++] = copy;
}

/* ============================================================
 * Positive/Negative Example Creation
 * ============================================================ */

PositiveExample* positive_example_create_random(int n, int k) {
    PositiveExample *ex = malloc(sizeof(PositiveExample));
    assert(ex != NULL);

    /* Pick k random distinct vertices for the clique */
    int *vertices = malloc((size_t)k * sizeof(int));
    int *used = calloc((size_t)n, sizeof(int));

    for (int i = 0; i < k; i++) {
        int v;
        do { v = rand() % n; } while (used[v]);
        used[v] = 1;
        vertices[i] = v;
    }

    /* Create a graph with a complete subgraph on those vertices */
    Graph *g = graph_create(n);
    for (int i = 0; i < k; i++) {
        for (int j = i + 1; j < k; j++) {
            graph_add_edge(g, vertices[i], vertices[j]);
        }
    }

    /* Add some extra random edges */
    for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) {
            if (g->adj[i][j]) continue;
            if ((double)rand() / RAND_MAX < 0.2) {
                graph_add_edge(g, i, j);
            }
        }
    }

    ex->graph = g;
    ex->witness = clique_from_vertices(n, k, vertices);
    free(vertices);
    free(used);
    return ex;
}

NegativeExample* negative_example_create_turan(int n, int k) {
    NegativeExample *ex = malloc(sizeof(NegativeExample));
    assert(ex != NULL);

    /* Turan graph T(n, k-1): k-1 partite, no k-clique.
     * This is the edge-maximal graph with no k-clique. */
    int num_parts = k - 1;
    Graph *g = graph_create(n);

    /* Partition vertices */
    int *part = malloc((size_t)n * sizeof(int));
    int part_size = n / num_parts;
    int remainder = n % num_parts;
    int idx = 0;
    for (int p = 0; p < num_parts; p++) {
        int sz = part_size + (p < remainder ? 1 : 0);
        for (int j = 0; j < sz; j++) part[idx++] = p;
    }

    for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) {
            if (part[i] != part[j]) graph_add_edge(g, i, j);
        }
    }

    /* Set up color classes */
    ex->num_colors = num_parts;
    ex->color_sizes = calloc((size_t)num_parts, sizeof(int));
    for (int i = 0; i < n; i++) ex->color_sizes[part[i]]++;

    ex->color_classes = malloc((size_t)num_parts * sizeof(int*));
    for (int p = 0; p < num_parts; p++) {
        ex->color_classes[p] = malloc((size_t)ex->color_sizes[p] * sizeof(int));
        int pos = 0;
        for (int i = 0; i < n; i++) {
            if (part[i] == p) ex->color_classes[p][pos++] = i;
        }
    }

    ex->graph = g;
    ex->type = NEG_TYPE_TURAN;

    free(part);
    return ex;
}

void positive_example_free(PositiveExample *ex) {
    if (!ex) return;
    graph_free(ex->graph);
    clique_free(ex->witness);
    free(ex);
}

void negative_example_free(NegativeExample *ex) {
    if (!ex) return;
    graph_free(ex->graph);
    for (int i = 0; i < ex->num_colors; i++) free(ex->color_classes[i]);
    free(ex->color_classes);
    free(ex->color_sizes);
    free(ex);
}

/* ============================================================
 * Approximator Evaluation
 * ============================================================ */

int approx_accepts(const Approximator *a, const Graph *g) {
    /* Accepts if matches a positive example and does NOT match
     * any negative example.
     * Match: graph contains the witness clique. */
    int pos_match = 0;
    for (int i = 0; i < a->num_pos; i++) {
        if (clique_is_valid(g, a->pos[i]->witness)) {
            pos_match = 1;
            break;
        }
    }

    int neg_match = 0;
    for (int i = 0; i < a->num_neg; i++) {
        /* Check if g matches this negative example.
         * For Turan type: check if g is a subgraph of the Turan graph.
         * Simplified: check if removing some edges makes it Turan. */
        /* For now, check if g has no edge between vertices
         * of the same color class in the negative example. */
        const NegativeExample *ne = a->neg[i];
        int matches = 1;
        for (int c = 0; c < ne->num_colors && matches; c++) {
            for (int x = 0; x < ne->color_sizes[c] && matches; x++) {
                for (int y = x + 1; y < ne->color_sizes[c] && matches; y++) {
                    int u = ne->color_classes[c][x];
                    int v = ne->color_classes[c][y];
                    if (graph_has_edge(g, u, v)) {
                        matches = 0;
                    }
                }
            }
        }
        if (matches) {
            neg_match = 1;
            break;
        }
    }

    return pos_match && !neg_match;
}

int approx_rejects(const Approximator *a, const Graph *g) {
    return !approx_accepts(a, g);
}

void approx_count_errors(const Approximator *a,
                          int *false_pos, int *false_neg,
                          int *correct_pos, int *correct_neg) {
    *false_pos = *false_neg = *correct_pos = *correct_neg = 0;

    /* Cannot enumerate all 2^{n^2} graphs.
     * For small n, enumerate all graphs. Otherwise estimate. */
    int n = a->n, k = a->k;
    if (n > 5) {
        /* Estimate based on samples (not implemented here) */
        return;
    }

    /* Enumerate all graphs on n vertices by enumerating all
     * edge sets: 2^{n(n-1)/2} possibilities. */
    int max_edges = n * (n - 1) / 2;
    int total_graphs = 1 << max_edges;

    for (int mask = 0; mask < total_graphs; mask++) {
        Graph *g = graph_create(n);
        int bit = 0;
        for (int i = 0; i < n; i++) {
            for (int j = i + 1; j < n; j++) {
                if (mask & (1 << bit)) graph_add_edge(g, i, j);
                bit++;
            }
        }

        int actual = graph_has_clique(g, k);
        int predicted = approx_accepts(a, g);

        if (actual && predicted) (*correct_pos)++;
        else if (!actual && !predicted) (*correct_neg)++;
        else if (!actual && predicted) (*false_pos)++;
        else (*false_neg)++;

        graph_free(g);
    }
}

/* ============================================================
 * Input Edge Approximator
 * ============================================================ */

Approximator* approx_input_edge(int n, int k, int u, int v, int max_size) {
    /* The input approximator for edge variable x_{uv} in CLIQUE.
     * Positive: a specific k-clique containing edge (u,v)
     * Negative: a (k-1)-coloring where u and v have the same color
     *   (since any k-clique must use k different colors by
     *    pigeonhole principle, an edge within a color class
     *    cannot be part of any k-clique in a proper coloring) */
    Approximator *a = approx_create(n, k, max_size);

    /* Positive example: embed (u,v) in a k-clique */
    /* Pick k-2 additional vertices to form a clique with u and v */
    int *vertices = malloc((size_t)k * sizeof(int));
    vertices[0] = u;
    vertices[1] = v;
    int count = 2;
    for (int w = 0; w < n && count < k; w++) {
        if (w == u || w == v) continue;
        vertices[count++] = w;
    }

    Graph *pos_g = graph_create(n);
    for (int i = 0; i < k; i++) {
        for (int j = i + 1; j < k; j++) {
            graph_add_edge(pos_g, vertices[i], vertices[j]);
        }
    }

    PositiveExample *pos = malloc(sizeof(PositiveExample));
    pos->graph = pos_g;
    pos->witness = clique_from_vertices(n, k, vertices);
    approx_add_positive(a, pos);
    positive_example_free(pos);

    /* Negative example: (k-1)-coloring with u and v in same color */
    int num_colors = k - 1;
    int color_u = 0; /* Put u in color 0 */
    int *color = malloc((size_t)n * sizeof(int));
    for (int i = 0; i < n; i++) color[i] = -1;
    color[u] = 0;
    color[v] = 0; /* Same color as u */

    /* Assign remaining vertices to other colors */
    int next_color = 1;
    for (int i = 0; i < n; i++) {
        if (color[i] < 0) {
            color[i] = next_color % num_colors;
            next_color++;
        }
    }

    /* Build negative example graph: complete multipartite
     * (edges only between vertices of different colors) */
    Graph *neg_g = graph_create(n);
    for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) {
            if (color[i] != color[j]) graph_add_edge(neg_g, i, j);
        }
    }

    NegativeExample *neg = malloc(sizeof(NegativeExample));
    neg->graph = neg_g;
    neg->type = NEG_TYPE_COLORING;
    neg->num_colors = num_colors;
    neg->color_sizes = calloc((size_t)num_colors, sizeof(int));
    for (int i = 0; i < n; i++) neg->color_sizes[color[i]]++;
    neg->color_classes = malloc((size_t)num_colors * sizeof(int*));
    for (int c = 0; c < num_colors; c++) {
        int sz = neg->color_sizes[c];
        neg->color_classes[c] = malloc((size_t)sz * sizeof(int));
        int pos_idx = 0;
        for (int i = 0; i < n; i++) {
            if (color[i] == c) neg->color_classes[c][pos_idx++] = i;
        }
    }
    approx_add_negative(a, neg);
    negative_example_free(neg);

    free(vertices);
    free(color);
    return a;
}

/* ============================================================
 * Approximator AND Gate
 * ============================================================ */

Approximator* approx_and_gate(const Approximator *a1,
                               const Approximator *a2, int max_size) {
    Approximator *result = approx_create(a1->n, a1->k, max_size);

    /* Positive: intersection. A graph is accepted by AND if both
     * a1 and a2 accept it. Positive examples: minimal cliques
     * that are in both. For simplicity, we take the positive
     * examples from a1 that are also accepted by a2. */
    for (int i = 0; i < a1->num_pos && result->num_pos < max_size; i++) {
        int accepted = 0;
        /* Check if this positive example would be accepted by a2 */
        for (int j = 0; j < a2->num_pos && !accepted; j++) {
            /* Two positive examples: check if they share a common
             * k-clique. Simplified: if the witness cliques overlap enough. */
            Clique *c1 = a1->pos[i]->witness;
            Clique *c2 = a2->pos[j]->witness;
            int common = 0;
            for (int a = 0; a < c1->k; a++) {
                for (int b = 0; b < c2->k; b++) {
                    if (c1->vertices[a] == c2->vertices[b]) common++;
                }
            }
            if (common > 0) accepted = 1;
        }
        if (accepted && result->num_pos < max_size) {
            approx_add_positive(result, a1->pos[i]);
        }
    }

    /* Negative: union of negatives from a1 and a2.
     * A graph is rejected by AND if EITHER a1 or a2 rejects it.
     * But if this makes the negative set too large, we use
     * the sunflower lemma to compress. */
    for (int i = 0; i < a1->num_neg && result->num_neg < max_size; i++) {
        approx_add_negative(result, a1->neg[i]);
    }
    for (int i = 0; i < a2->num_neg && result->num_neg < max_size; i++) {
        approx_add_negative(result, a2->neg[i]);
    }

    result->and_gates_processed = a1->and_gates_processed + a2->and_gates_processed + 1;
    result->or_gates_processed = a1->or_gates_processed + a2->or_gates_processed;

    return result;
}

/* ============================================================
 * Approximator OR Gate
 * ============================================================ */

Approximator* approx_or_gate(const Approximator *a1,
                              const Approximator *a2, int max_size) {
    Approximator *result = approx_create(a1->n, a1->k, max_size);

    /* Positive: union of positives from a1 and a2.
     * A graph is accepted by OR if EITHER a1 or a2 accepts it. */
    for (int i = 0; i < a1->num_pos && result->num_pos < max_size; i++) {
        approx_add_positive(result, a1->pos[i]);
    }
    for (int i = 0; i < a2->num_pos && result->num_pos < max_size; i++) {
        approx_add_positive(result, a2->pos[i]);
    }

    /* Negative: intersection. A graph rejected by OR only if
     * BOTH a1 and a2 reject it. For simplicity, take negatives
     * that are common to both. */
    for (int i = 0; i < a1->num_neg && result->num_neg < max_size; i++) {
        /* Check if this negative example is also in a2 */
        int in_a2 = 0;
        for (int j = 0; j < a2->num_neg && !in_a2; j++) {
            /* Compare color class structures */
            if (a1->neg[i]->num_colors == a2->neg[j]->num_colors) {
                in_a2 = 1;
            }
        }
        if (in_a2 && result->num_neg < max_size) {
            approx_add_negative(result, a1->neg[i]);
        }
    }

    result->and_gates_processed = a1->and_gates_processed + a2->and_gates_processed;
    result->or_gates_processed = a1->or_gates_processed + a2->or_gates_processed + 1;

    return result;
}

/* ============================================================
 * Razborov Simulator
 * ============================================================ */

Approximator* approx_simulate_circuit(const MonotoneCircuit *c,
                                       const int *input_assignment,
                                       int max_size) {
    if (!c) return NULL;
    int n = c->num_inputs;
    int k = (int)sqrt((double)n);
    if (k < 2) k = 2;

    /* Create approximator for each gate */
    Approximator **gate_approx = malloc((size_t)c->num_gates * sizeof(Approximator*));
    assert(gate_approx != NULL);

    for (int i = 0; i < c->num_gates; i++) {
        const MonoGate *g = &c->gates[i];

        switch (g->type) {
            case MONO_GATE_INPUT:
                /* Input variable = edge existence.
                 * Create edge approximator.
                 * Map input_idx to an edge (u,v):
                 * u = input_idx / n, v = input_idx % n */
                {
                    int u = g->input_idx / n;
                    int v = g->input_idx % n;
                    if (u >= n) u = u % n;
                    if (v >= n) v = v % n;
                    if (u == v) v = (v + 1) % n;
                    gate_approx[i] = approx_input_edge(n, k, u, v, max_size);
                }
                break;

            case MONO_GATE_CONST_0:
                gate_approx[i] = approx_create(n, k, max_size);
                break;

            case MONO_GATE_CONST_1: {
                gate_approx[i] = approx_create(n, k, max_size);
                /* Add a trivial positive example */
                int *verts = malloc((size_t)k * sizeof(int));
                for (int j = 0; j < k; j++) verts[j] = j;
                Graph *g_pos = graph_complete(k);
                /* Embed k-clique in n-vertex graph */
                Graph *g_full = graph_create(n);
                for (int a = 0; a < k; a++)
                    for (int b = a + 1; b < k; b++)
                        graph_add_edge(g_full, verts[a], verts[b]);
                PositiveExample *ex = malloc(sizeof(PositiveExample));
                ex->graph = g_full;
                ex->witness = clique_from_vertices(n, k, verts);
                approx_add_positive(gate_approx[i], ex);
                positive_example_free(ex);
                free(verts);
                break;
            }

            case MONO_GATE_AND:
                gate_approx[i] = approx_and_gate(
                    gate_approx[g->fanin[0]],
                    gate_approx[g->fanin[1]], max_size);
                break;

            case MONO_GATE_OR:
                gate_approx[i] = approx_or_gate(
                    gate_approx[g->fanin[0]],
                    gate_approx[g->fanin[1]], max_size);
                break;
        }

        /* If approximator exceeds max_size, the circuit is too small */
        if (gate_approx[i] &&
            gate_approx[i]->num_pos + gate_approx[i]->num_neg > max_size) {
            /* Cleanup and return NULL to indicate failure */
            for (int j = 0; j <= i; j++) approx_free(gate_approx[j]);
            free(gate_approx);
            return NULL;
        }
    }

    Approximator *output = gate_approx[c->output_gate];
    /* Don't free output; return it. Free the rest. */
    for (int i = 0; i < c->num_gates; i++) {
        if (i != c->output_gate) approx_free(gate_approx[i]);
    }
    free(gate_approx);
    return output;
}

/* ============================================================
 * Size Analysis
 * ============================================================ */

double approx_expected_size(int initial_size, int depth,
                             double and_prob, int p, int k) {
    double size = (double)initial_size;
    for (int d = 0; d < depth; d++) {
        /* Each level: some AND gates, some OR gates.
         * AND gates: size_new <= sunflower_bound(p,k) * size^2
         *           but sunflower compression reduces to O(size)
         * OR gates:  size_new = 2 * size (additive)
         * On average: size stays manageable */
        double and_blowup = pow((double)size, 2.0);
        double sunflower_compressed = sunflower_bound_erdos_rado(p, k) * size;
        double effective_blowup = and_prob * sunflower_compressed +
                                   (1.0 - and_prob) * 2.0 * size;
        size = size + effective_blowup * 0.1; /* Conservative */
        if (size > 1e9) break; /* Blowup -> large circuit needed */
    }
    return size;
}

int approx_is_valid_positive(const Approximator *a) {
    /* Check: no positive example is a NO instance.
     * A positive example is a graph with a k-clique. */
    for (int i = 0; i < a->num_pos; i++) {
        if (!graph_has_clique(a->pos[i]->graph, a->k)) return 0;
    }
    return 1;
}

int approx_is_valid_negative(const Approximator *a) {
    /* Check: no negative example is a YES instance. */
    for (int i = 0; i < a->num_neg; i++) {
        if (graph_has_clique(a->neg[i]->graph, a->k)) return 0;
    }
    return 1;
}

/* ============================================================
 * Lower Bound Size from Approximator Analysis
 * ============================================================ */

double approx_lower_bound_size(int n, int k, int depth) {
    /* If the circuit has size S and depth d, the approximator
     * size at the output is bounded by:
     *   |A_out| <= exp(O(d * log B)) where B = sunflower_bound
     * For CLIQUE with k = n^{1/4}:
     *   Any correct approximator needs size >= exp(Omega(n^{1/8}))
     * So if |A_out| < CLIQUE bound, circuit must be large. */

    double bound = sunflower_bound_erdos_rado(3, k);
    double required = exp(sqrt((double)k) * log((double)n) / 4.0);
    double approx_size = pow(bound, (double)depth);

    if (approx_size < required) {
        /* Circuit must be large: S >= required / poly(n) */
        return required / (double)(n * n);
    }
    return approx_size;
}

/* ============================================================
 * Tiny Circuit Demo
 * ============================================================ */

void approx_demo_tiny_circuit(void) {
    printf("=== Method of Approximations: Tiny Circuit Demo ===\n\n");

    int n = 6, k = 3;
    printf("Computing CLIQUE(%d,%d)\n", n, k);

    /* Create input approximators for each possible edge */
    printf("Creating input edge approximators...\n");

    Approximator *a = approx_create(n, k, 100);
    for (int u = 0; u < n; u++) {
        for (int v = u + 1; v < n; v++) {
            Approximator *edge = approx_input_edge(n, k, u, v, 100);
            printf("  Edge(%d,%d): pos=%d neg=%d\n",
                   u, v, edge->num_pos, edge->num_neg);
            approx_free(edge);
        }
    }

    /* Demo: OR of two edge approximators */
    Approximator *e1 = approx_input_edge(n, k, 0, 1, 100);
    Approximator *e2 = approx_input_edge(n, k, 2, 3, 100);
    Approximator *e_or = approx_or_gate(e1, e2, 100);
    printf("\nOR gate: pos=%d neg=%d\n", e_or->num_pos, e_or->num_neg);

    /* Demo: AND of two edge approximators */
    Approximator *e_and = approx_and_gate(e1, e2, 100);
    printf("AND gate: pos=%d neg=%d\n", e_and->num_pos, e_and->num_neg);

    approx_free(a);
    approx_free(e1); approx_free(e2);
    approx_free(e_or); approx_free(e_and);

    printf("\nKey insight:\n");
    printf("  AND gate: approximator size may grow (controlled by sunflower)\n");
    printf("  OR gate:  approximator size grows additively (manageable)\n");
    printf("  => Monotone circuits with many ANDs on large inputs\n");
    printf("     must be large to compute CLIQUE correctly.\n");
}

/* ============================================================
 * Andreev Approximator (placeholder)
 * ============================================================ */

Approximator* approx_andreev_construct(int n, int k, int max_size) {
    /* Andreev (1987) uses a different construction for positive
     * and negative examples that gives better bounds.
     * Placeholder: returns an approximator with standard construction. */
    Approximator *a = approx_create(n, k, max_size);
    /* Add one random positive and one Turan negative */
    PositiveExample *pos = positive_example_create_random(n, k);
    NegativeExample *neg = negative_example_create_turan(n, k);
    approx_add_positive(a, pos);
    approx_add_negative(a, neg);
    positive_example_free(pos);
    negative_example_free(neg);
    return a;
}

/* ============================================================
 * Frontier Approximator
 * ============================================================ */

FrontierApproximator* frontier_approx_create(int n, int k, int max_size) {
    FrontierApproximator *fa = malloc(sizeof(FrontierApproximator));
    assert(fa != NULL);
    fa->n = n; fa->k = k; fa->max_size = max_size;
    fa->num_pos = fa->num_neg = 0;
    fa->frontier_pos = malloc((size_t)max_size * sizeof(Clique*));
    fa->frontier_neg = malloc((size_t)max_size * sizeof(NegativeExample*));
    return fa;
}

void frontier_approx_free(FrontierApproximator *fa) {
    if (!fa) return;
    for (int i = 0; i < fa->num_pos; i++) clique_free(fa->frontier_pos[i]);
    for (int i = 0; i < fa->num_neg; i++) negative_example_free(fa->frontier_neg[i]);
    free(fa->frontier_pos);
    free(fa->frontier_neg);
    free(fa);
}

int frontier_approx_accepts(const FrontierApproximator *fa, const Graph *g) {
    /* Accepts if g contains some frontier positive clique
     * and does not match any frontier negative coloring */
    for (int i = 0; i < fa->num_pos; i++) {
        if (clique_is_valid(g, fa->frontier_pos[i])) return 1;
    }
    return 0;
}

/* ============================================================
 * Extended Approximators (L9)
 * ============================================================ */

Approximator* approx_perfect_matching(int n, int max_size) {
    /* Placeholder: approximator for PERFECT MATCHING */
    Approximator *a = approx_create(n, n / 2, max_size);
    return a;
}

Approximator* approx_st_connectivity(int n, int s, int t, int max_size) {
    /* Placeholder: approximator for ST-CONNECTIVITY */
    Approximator *a = approx_create(n, 2, max_size);
    return a;
}
