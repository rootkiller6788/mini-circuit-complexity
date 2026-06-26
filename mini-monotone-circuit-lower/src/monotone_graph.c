/* monotone_graph.c -- Graph Structures for Monotone Lower Bounds
 *
 * Implements graph operations used in Razborov's monotone circuit
 * lower bounds. Key operations: clique detection, graph generation,
 * and canonical monotone-NP graph problems.
 *
 * Theorem (Razborov 1985): CLIQUE requires super-polynomial
 * monotone circuits. The proof relies heavily on graph-theoretic
 * constructions and the sunflower lemma applied to set systems
 * derived from graph cliques.
 *
 * References:
 *   Razborov (1985), Doklady AN SSSR
 *   Alon & Boppana (1987), Combinatorica 7(1), 1-22
 *   Bollobas (1998), Modern Graph Theory
 *   Jukna (2012), Boolean Function Complexity
 *
 * Complexity:
 *   Clique detection (brute force): O(n^k * k^2) worst case
 *   Clique enumeration: O(3^{n/3}) worst case (Moon-Moser)
 *   Perfect matching (blossom algorithm): O(n^3)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <assert.h>
#include "monotone_graph.h"

/* ============================================================
 * Graph Construction
 * ============================================================ */

Graph* graph_create(int n) {
    assert(n > 0);
    Graph *g = malloc(sizeof(Graph));
    assert(g != NULL);
    g->n = n;
    g->edges = 0;
    g->adj = malloc((size_t)n * sizeof(char*));
    assert(g->adj != NULL);
    for (int i = 0; i < n; i++) {
        g->adj[i] = calloc((size_t)n, sizeof(char));
        assert(g->adj[i] != NULL);
    }
    return g;
}

Graph* graph_complete(int n) {
    Graph *g = graph_create(n);
    for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) {
            g->adj[i][j] = g->adj[j][i] = 1;
            g->edges++;
        }
    }
    return g;
}

Graph* graph_empty(int n) {
    return graph_create(n);
}

Graph* graph_random(int n, double p) {
    Graph *g = graph_create(n);
    for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) {
            if ((double)rand() / RAND_MAX < p) {
                g->adj[i][j] = g->adj[j][i] = 1;
                g->edges++;
            }
        }
    }
    return g;
}

Graph* graph_from_edges(int n, int (*edges)[2], int num_edges) {
    Graph *g = graph_create(n);
    for (int e = 0; e < num_edges; e++) {
        int u = edges[e][0], v = edges[e][1];
        if (u >= 0 && u < n && v >= 0 && v < n) {
            g->adj[u][v] = g->adj[v][u] = 1;
            g->edges++;
        }
    }
    return g;
}

Graph* graph_bipartite_complete(int a, int b) {
    int n = a + b;
    Graph *g = graph_create(n);
    for (int i = 0; i < a; i++) {
        for (int j = a; j < n; j++) {
            g->adj[i][j] = g->adj[j][i] = 1;
            g->edges++;
        }
    }
    return g;
}

Graph* graph_cycle(int n) {
    Graph *g = graph_create(n);
    for (int i = 0; i < n; i++) {
        int j = (i + 1) % n;
        g->adj[i][j] = g->adj[j][i] = 1;
        g->edges++;
    }
    return g;
}

Graph* graph_complement(const Graph *g) {
    Graph *gc = graph_create(g->n);
    for (int i = 0; i < g->n; i++) {
        for (int j = i + 1; j < g->n; j++) {
            if (!g->adj[i][j]) {
                gc->adj[i][j] = gc->adj[j][i] = 1;
                gc->edges++;
            }
        }
    }
    return gc;
}

Graph* graph_copy(const Graph *g) {
    Graph *copy = graph_create(g->n);
    for (int i = 0; i < g->n; i++) {
        memcpy(copy->adj[i], g->adj[i], (size_t)g->n * sizeof(char));
    }
    copy->edges = g->edges;
    return copy;
}

void graph_free(Graph *g) {
    if (!g) return;
    for (int i = 0; i < g->n; i++) {
        free(g->adj[i]);
    }
    free(g->adj);
    free(g);
}

/* ============================================================
 * Graph Operations
 * ============================================================ */

int graph_add_edge(Graph *g, int u, int v) {
    if (u < 0 || v < 0 || u >= g->n || v >= g->n) return 0;
    if (u == v) return 0; /* No self-loops */
    if (g->adj[u][v]) return 0; /* Already exists */
    g->adj[u][v] = g->adj[v][u] = 1;
    g->edges++;
    return 1;
}

int graph_remove_edge(Graph *g, int u, int v) {
    if (u < 0 || v < 0 || u >= g->n || v >= g->n) return 0;
    if (!g->adj[u][v]) return 0; /* Does not exist */
    g->adj[u][v] = g->adj[v][u] = 0;
    g->edges--;
    return 1;
}

int graph_has_edge(const Graph *g, int u, int v) {
    if (u < 0 || v < 0 || u >= g->n || v >= g->n) return 0;
    return g->adj[u][v];
}

int graph_edge_count(const Graph *g) {
    return g->edges;
}

int graph_degree(const Graph *g, int v) {
    if (v < 0 || v >= g->n) return 0;
    int deg = 0;
    for (int i = 0; i < g->n; i++) {
        if (g->adj[v][i]) deg++;
    }
    return deg;
}

int* graph_neighbors(const Graph *g, int v, int *count) {
    if (v < 0 || v >= g->n) {
        *count = 0;
        return NULL;
    }
    *count = graph_degree(g, v);
    if (*count == 0) return NULL;
    int *neighbors = malloc((size_t)(*count) * sizeof(int));
    int idx = 0;
    for (int i = 0; i < g->n; i++) {
        if (g->adj[v][i]) neighbors[idx++] = i;
    }
    return neighbors;
}

/* ============================================================
 * Clique Operations
 * ============================================================ */

Clique* clique_create(int n, int k) {
    Clique *c = malloc(sizeof(Clique));
    assert(c != NULL);
    c->n = n;
    c->k = k;
    c->vertices = malloc((size_t)k * sizeof(int));
    assert(c->vertices != NULL);
    for (int i = 0; i < k; i++) c->vertices[i] = 0;
    return c;
}

Clique* clique_from_vertices(int n, int k, const int *vertices) {
    Clique *c = clique_create(n, k);
    memcpy(c->vertices, vertices, (size_t)k * sizeof(int));
    return c;
}

int clique_is_valid(const Graph *g, const Clique *c) {
    if (!g || !c) return 0;
    for (int i = 0; i < c->k; i++) {
        for (int j = i + 1; j < c->k; j++) {
            if (!graph_has_edge(g, c->vertices[i], c->vertices[j])) {
                return 0;
            }
        }
    }
    return 1;
}

void clique_free(Clique *c) {
    if (!c) return;
    free(c->vertices);
    free(c);
}

void clique_print(const Clique *c) {
    printf("Clique(k=%d): {", c->k);
    for (int i = 0; i < c->k; i++) {
        if (i > 0) printf(", ");
        printf("%d", c->vertices[i]);
    }
    printf("}\n");
}

/* ============================================================
 * Clique Detection: Recursive Backtracking
 * ============================================================ */

/* Recursive backtracking clique search.
 * 'candidates': vertices that can be added to the current partial clique.
 * 'partial': vertices already in the clique.
 * 'partial_size': current size of partial clique. */
static int clique_search_rec(const Graph *g, int k,
                              int *candidates, int num_candidates,
                              int *partial, int partial_size) {
    if (partial_size == k) return 1; /* Found k-clique */

    /* If not enough candidates to reach k, prune */
    if (partial_size + num_candidates < k) return 0;

    for (int i = 0; i < num_candidates; i++) {
        int v = candidates[i];

        /* Build new candidate list: neighbors of v that are
         * also in the remaining candidates */
        int new_candidates[256]; /* Stack allocation for small n */
        int new_num = 0;
        for (int j = i + 1; j < num_candidates; j++) {
            if (graph_has_edge(g, v, candidates[j])) {
                new_candidates[new_num++] = candidates[j];
            }
        }

        partial[partial_size] = v;
        if (clique_search_rec(g, k, new_candidates, new_num,
                               partial, partial_size + 1))
            return 1;
    }
    return 0;
}

int graph_has_clique(const Graph *g, int k) {
    if (!g || k <= 0) return 0;
    if (k == 1) return g->n >= 1;
    if (k > g->n) return 0;

    int *candidates = malloc((size_t)g->n * sizeof(int));
    for (int i = 0; i < g->n; i++) candidates[i] = i;

    int *partial = malloc((size_t)k * sizeof(int));
    int result = clique_search_rec(g, k, candidates, g->n, partial, 0);

    free(candidates);
    free(partial);
    return result;
}

int graph_find_clique(const Graph *g, int k, Clique *result) {
    if (!g || k <= 0 || k > g->n || !result) return 0;
    if (k == 1) {
        result->vertices[0] = 0;
        return g->n >= 1;
    }

    int *candidates = malloc((size_t)g->n * sizeof(int));
    for (int i = 0; i < g->n; i++) candidates[i] = i;

    int *partial = malloc((size_t)k * sizeof(int));
    int found = clique_search_rec(g, k, candidates, g->n, partial, 0);

    if (found) {
        memcpy(result->vertices, partial, (size_t)k * sizeof(int));
    }

    free(candidates);
    free(partial);
    return found;
}

/* ============================================================
 * Clique Enumeration
 * ============================================================ */

typedef struct {
    Clique **cliques;
    int count;
    int capacity;
} CliqueList;

static void cliquelist_init(CliqueList *cl, int capacity) {
    cl->cliques = malloc((size_t)capacity * sizeof(Clique*));
    cl->count = 0;
    cl->capacity = capacity;
}

static void cliquelist_add(CliqueList *cl, const int *verts, int k, int n) {
    if (cl->count >= cl->capacity) {
        cl->capacity *= 2;
        cl->cliques = realloc(cl->cliques, (size_t)cl->capacity * sizeof(Clique*));
    }
    cl->cliques[cl->count] = clique_from_vertices(n, k, verts);
    cl->count++;
}

static void enumerate_cliques_rec(const Graph *g, int k,
                                   int *candidates, int num_candidates,
                                   int *partial, int partial_size,
                                   CliqueList *cl) {
    if (partial_size == k) {
        cliquelist_add(cl, partial, k, g->n);
        return;
    }

    for (int i = 0; i < num_candidates; i++) {
        int v = candidates[i];
        int new_candidates[256];
        int new_num = 0;
        for (int j = i + 1; j < num_candidates; j++) {
            if (graph_has_edge(g, v, candidates[j])) {
                new_candidates[new_num++] = candidates[j];
            }
        }
        partial[partial_size] = v;
        enumerate_cliques_rec(g, k, new_candidates, new_num,
                               partial, partial_size + 1, cl);
    }
}

int graph_enumerate_cliques(const Graph *g, int k, Clique ***cliques) {
    if (!g || k <= 0 || k > g->n) {
        *cliques = NULL;
        return 0;
    }

    CliqueList cl;
    cliquelist_init(&cl, 64);

    int *candidates = malloc((size_t)g->n * sizeof(int));
    for (int i = 0; i < g->n; i++) candidates[i] = i;
    int *partial = malloc((size_t)k * sizeof(int));

    enumerate_cliques_rec(g, k, candidates, g->n, partial, 0, &cl);

    free(candidates);
    free(partial);

    *cliques = cl.cliques;
    return cl.count;
}

/* ============================================================
 * Set Family Operations
 * ============================================================ */

SetFamily* setfamily_create(int n, int k, int capacity) {
    SetFamily *sf = malloc(sizeof(SetFamily));
    assert(sf != NULL);
    sf->n = n;
    sf->k = k;
    sf->num_sets = 0;
    sf->capacity = capacity;
    sf->sets = malloc((size_t)capacity * sizeof(uint64_t));
    assert(sf->sets != NULL);
    return sf;
}

void setfamily_add(SetFamily *sf, uint64_t set) {
    if (sf->num_sets >= sf->capacity) {
        sf->capacity *= 2;
        sf->sets = realloc(sf->sets, (size_t)sf->capacity * sizeof(uint64_t));
        assert(sf->sets != NULL);
    }
    sf->sets[sf->num_sets++] = set;
}

void setfamily_add_vertices(SetFamily *sf, const int *vertices, int k) {
    uint64_t mask = 0;
    for (int i = 0; i < k; i++) {
        mask |= (1ULL << vertices[i]);
    }
    setfamily_add(sf, mask);
}

int setfamily_contains(const SetFamily *sf, uint64_t set) {
    for (int i = 0; i < sf->num_sets; i++) {
        if (sf->sets[i] == set) return 1;
    }
    return 0;
}

void setfamily_free(SetFamily *sf) {
    if (!sf) return;
    free(sf->sets);
    free(sf);
}

/* ============================================================
 * Monotone CLIQUE Function
 * ============================================================ */

int monotone_clique_function(const Graph *g, int k) {
    return graph_has_clique(g, k);
}

/* ============================================================
 * Positive/Negative Example Generation
 * ============================================================ */

Graph* graph_generate_clique_positive(int n, int k) {
    /* Create a graph with an explicit k-clique and some extra edges */
    Graph *g = graph_create(n);

    /* Embed a k-clique on vertices 0..k-1 */
    for (int i = 0; i < k; i++) {
        for (int j = i + 1; j < k; j++) {
            g->adj[i][j] = g->adj[j][i] = 1;
            g->edges++;
        }
    }

    /* Add random extra edges for realism */
    for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) {
            if (i < k && j < k) continue; /* Already in clique */
            if ((double)rand() / RAND_MAX < 0.3) {
                if (!g->adj[i][j]) {
                    g->adj[i][j] = g->adj[j][i] = 1;
                    g->edges++;
                }
            }
        }
    }

    return g;
}

Graph* graph_generate_clique_negative(int n, int k) {
    /* Turan graph T(n, k-1): partition vertices into k-1 parts,
     * connect all vertices in different parts (no edges within parts).
     * Turan's theorem: T(n, k-1) has no k-clique and is edge-maximal
     * among graphs with no k-clique. */
    Graph *g = graph_create(n);

    /* Partition vertices evenly into k-1 parts */
    int part_size = n / (k - 1);
    int remainder = n % (k - 1);
    int *part = malloc((size_t)n * sizeof(int));
    int idx = 0;
    for (int p = 0; p < k - 1; p++) {
        int sz = part_size + (p < remainder ? 1 : 0);
        for (int j = 0; j < sz; j++) {
            part[idx++] = p;
        }
    }

    /* Connect vertices in different parts */
    for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) {
            if (part[i] != part[j]) {
                g->adj[i][j] = g->adj[j][i] = 1;
                g->edges++;
            }
        }
    }

    free(part);
    return g;
}

/* ============================================================
 * Bron-Kerbosch Clique Detection (with pivoting)
 * ============================================================ */

/* Bron-Kerbosch with pivoting: find maximum clique.
 * R = current clique, P = candidate set, X = excluded set.
 * Returns 1 if k-clique found, 0 otherwise. */
static int bron_kerbosch_pivot(const Graph *g, int k,
                                int *R, int r_size,
                                int *P, int p_size,
                                int *X, int x_size) {
    if (r_size >= k) return 1;
    if (p_size == 0) {
        /* Maximal clique found; check size */
        return (r_size >= k);
    }
    if (r_size + p_size < k) return 0; /* Cannot reach k */

    /* Choose pivot u from P ? X with max degree in P */
    int pivot = P[0];
    int max_deg = 0;
    for (int i = 0; i < p_size; i++) {
        int deg = 0;
        for (int j = 0; j < p_size; j++) {
            if (graph_has_edge(g, P[i], P[j])) deg++;
        }
        if (deg > max_deg) { max_deg = deg; pivot = P[i]; }
    }
    for (int i = 0; i < x_size; i++) {
        int deg = 0;
        for (int j = 0; j < p_size; j++) {
            if (graph_has_edge(g, X[i], P[j])) deg++;
        }
        if (deg > max_deg) { max_deg = deg; pivot = X[i]; }
    }

    /* Process vertices in P that are NOT neighbors of pivot */
    int processed[256] = {0};
    for (int i = 0; i < p_size; i++) {
        int v = P[i];
        if (graph_has_edge(g, v, pivot)) continue;
        if (processed[v]) continue;
        processed[v] = 1;

        /* R ? {v} */
        R[r_size] = v;

        /* P ? N(v) */
        int new_P[256], new_p_size = 0;
        for (int j = 0; j < p_size; j++) {
            if (P[j] != v && graph_has_edge(g, v, P[j])) {
                new_P[new_p_size++] = P[j];
            }
        }

        /* X ? N(v) */
        int new_X[256], new_x_size = 0;
        for (int j = 0; j < x_size; j++) {
            if (graph_has_edge(g, v, X[j])) {
                new_X[new_x_size++] = X[j];
            }
        }

        if (bron_kerbosch_pivot(g, k, R, r_size + 1,
                                 new_P, new_p_size,
                                 new_X, new_x_size))
            return 1;

        /* Move v from P to X */
        /* (implicitly done by the processed check + not being in new_P) */
    }
    return 0;
}

int graph_has_clique_bk(const Graph *g, int k) {
    if (!g || k <= 0 || k > g->n) return 0;

    int *R = malloc((size_t)g->n * sizeof(int));
    int *P = malloc((size_t)g->n * sizeof(int));
    int *X = malloc((size_t)g->n * sizeof(int));

    int p_size = 0;
    for (int i = 0; i < g->n; i++) P[p_size++] = i;

    int result = bron_kerbosch_pivot(g, k, R, 0, P, p_size, X, 0);

    free(R); free(P); free(X);
    return result;
}

/* ============================================================
 * Maximum Clique
 * ============================================================ */

int graph_max_clique_size(const Graph *g) {
    if (!g) return 0;
    int max_k = 0;
    for (int k = 1; k <= g->n && k <= 16; k++) {
        if (graph_has_clique(g, k)) max_k = k;
        else break;
    }
    return max_k;
}

/* ============================================================
 * Exact k-Clique Counting
 * ============================================================ */

long long graph_count_k_cliques(const Graph *g, int k) {
    if (!g || k <= 0 || k > g->n) return 0;
    Clique **cliques;
    int count = graph_enumerate_cliques(g, k, &cliques);
    for (int i = 0; i < count; i++) clique_free(cliques[i]);
    free(cliques);
    return (long long)count;
}

/* ============================================================
 * VERTEX COVER
 * ============================================================ */

int graph_has_vertex_cover(const Graph *g, int k) {
    /* VERTEX-COVER: is there a set of k vertices covering all edges?
     * Complement: if graph has a vertex cover of size k,
     * then its complement has an independent set of size n-k.
     * Independent set of size s = clique of size s in complement. */
    if (!g || k < 0) return 0;
    if (k >= g->n) return 1;
    /* For small n, brute force all subsets of size k */
    /* Using complement: vertex cover of size k iff independent set
     * of size n-k in complement graph. */
    Graph *gc = graph_complement(g);
    int result = graph_has_clique(gc, g->n - k);
    graph_free(gc);
    return result;
}

/* ============================================================
 * INDEPENDENT SET
 * ============================================================ */

int graph_has_independent_set(const Graph *g, int k) {
    /* Independent set = clique in complement graph */
    if (!g || k <= 0) return 0;
    Graph *gc = graph_complement(g);
    int result = graph_has_clique(gc, k);
    graph_free(gc);
    return result;
}

/* ============================================================
 * ST-CONNECTIVITY
 * ============================================================ */

int graph_has_path(const Graph *g, int s, int t) {
    if (!g || s < 0 || t < 0 || s >= g->n || t >= g->n) return 0;
    if (s == t) return 1;

    int *visited = calloc((size_t)g->n, sizeof(int));
    int *queue = malloc((size_t)g->n * sizeof(int));
    int front = 0, rear = 0;

    visited[s] = 1;
    queue[rear++] = s;

    while (front < rear) {
        int v = queue[front++];
        for (int u = 0; u < g->n; u++) {
            if (g->adj[v][u] && !visited[u]) {
                if (u == t) { free(visited); free(queue); return 1; }
                visited[u] = 1;
                queue[rear++] = u;
            }
        }
    }

    free(visited);
    free(queue);
    return 0;
}

/* ============================================================
 * PERFECT MATCHING (simplified: for bipartite graphs)
 * ============================================================ */

/* For general graphs, perfect matching is in P (Edmonds' blossom algorithm).
 * Here we implement for small graphs using Hall's theorem approach.
 * Simplified: brute force all perfect matchings for n <= 10. */

static int perfect_matching_rec(const Graph *g, int *matched, int depth) {
    if (depth >= g->n) return 1; /* All vertices matched */

    int v = -1;
    for (int i = 0; i < g->n; i++) {
        if (!matched[i]) { v = i; break; }
    }
    if (v < 0) return 1; /* All matched */

    for (int u = v + 1; u < g->n; u++) {
        if (!matched[u] && graph_has_edge(g, v, u)) {
            matched[v] = matched[u] = 1;
            if (perfect_matching_rec(g, matched, depth + 2)) return 1;
            matched[v] = matched[u] = 0;
        }
    }
    return 0;
}

int graph_has_perfect_matching(const Graph *g) {
    if (!g || g->n % 2 != 0) return 0;
    if (g->n > 12) {
        /* Too large for brute force; return "unknown" */
        return -1;
    }
    int *matched = calloc((size_t)g->n, sizeof(int));
    int result = perfect_matching_rec(g, matched, 0);
    free(matched);
    return result;
}

/* ============================================================
 * Printing Utilities
 * ============================================================ */

void graph_print(const Graph *g) {
    printf("Graph(n=%d, edges=%d):\n", g->n, g->edges);
    for (int i = 0; i < g->n; i++) {
        printf("  %2d: ", i);
        for (int j = 0; j < g->n; j++) {
            printf("%c ", g->adj[i][j] ? '1' : '.');
        }
        printf("\n");
    }
}

void graph_print_dot(const Graph *g, FILE *out) {
    fprintf(out, "graph G {\n");
    fprintf(out, "  node [shape=circle];\n");
    for (int i = 0; i < g->n; i++) {
        fprintf(out, "  v%d;\n", i);
    }
    for (int i = 0; i < g->n; i++) {
        for (int j = i + 1; j < g->n; j++) {
            if (g->adj[i][j]) {
                fprintf(out, "  v%d -- v%d;\n", i, j);
            }
        }
    }
    fprintf(out, "}\n");
}
