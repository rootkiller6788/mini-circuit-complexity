/* monotone_graph.h -- Graph Structures for Monotone Lower Bounds
 *
 * Razborov's lower bound for CLIQUE requires graph operations
 * and clique detection. This module provides the graph data
 * structures and algorithms used throughout the monotone
 * circuit lower bound proofs.
 *
 * Key structures:
 *   Graph: simple undirected graph (adjacency matrix)
 *   Clique: a set of k mutually adjacent vertices
 *
 * References:
 *   Razborov (1985) - Doklady AN SSSR
 *   Alon & Boppana (1987) - Combinatorica
 *   Bollobas (1998) - Modern Graph Theory
 */
#ifndef MONOTONE_GRAPH_H
#define MONOTONE_GRAPH_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

/* ============================================================
 * L1 Definitions: Graph
 * ============================================================ */

/* Simple undirected graph represented by adjacency matrix */
typedef struct {
    int    n;         /* Number of vertices */
    char **adj;       /* n x n adjacency matrix (symmetric) */
    int    edges;     /* Number of edges */
} Graph;

/* ============================================================
 * L1 Definitions: Clique
 * ============================================================ */

/* A k-clique: set of k vertices that are all pairwise adjacent */
typedef struct {
    int  k;           /* Clique size */
    int *vertices;    /* Vertex indices [0..k-1] */
    int  n;           /* Parent graph size (for bounds) */
} Clique;

/* ============================================================
 * L2 Core Concepts: Graph Construction
 * ============================================================ */

/* Create graph with n vertices, no edges */
Graph* graph_create(int n);

/* Create a complete graph K_n */
Graph* graph_complete(int n);

/* Create an empty graph E_n */
Graph* graph_empty(int n);

/* Create Erdos-Renyi random graph G(n,p) */
Graph* graph_random(int n, double p);

/* Create graph from edge list. edges[i][0]-edges[i][1] are edges.
 * num_edges: number of edges in the list. */
Graph* graph_from_edges(int n, int (*edges)[2], int num_edges);

/* Create a bipartite graph K_{a,b} (two parts, all cross edges) */
Graph* graph_bipartite_complete(int a, int b);

/* Create a cycle C_n */
Graph* graph_cycle(int n);

/* Create complement graph (edge iff not edge in original, excluding loops) */
Graph* graph_complement(const Graph *g);

/* Deep copy a graph */
Graph* graph_copy(const Graph *g);

/* Free graph memory */
void graph_free(Graph *g);

/* ============================================================
 * L2: Graph Operations
 * ============================================================ */

/* Add/remove edge between u and v. Returns 1 on success. */
int graph_add_edge(Graph *g, int u, int v);
int graph_remove_edge(Graph *g, int u, int v);

/* Check if edge exists */
int graph_has_edge(const Graph *g, int u, int v);

/* Count total edges */
int graph_edge_count(const Graph *g);

/* Get degree of vertex v */
int graph_degree(const Graph *g, int v);

/* Get neighbors of vertex v as array. *count set to degree. 
 * Caller must free. */
int* graph_neighbors(const Graph *g, int v, int *count);

/* ============================================================
 * L3 Mathematical Structures: Clique Operations
 * ============================================================ */

/* Create a clique structure */
Clique* clique_create(int n, int k);

/* Create a clique from vertex list */
Clique* clique_from_vertices(int n, int k, const int *vertices);

/* Check if a set of vertices forms a clique in graph g */
int clique_is_valid(const Graph *g, const Clique *c);

/* Check if graph contains any k-clique (brute force).
 * If found, fills *found (if non-NULL). */
int graph_has_clique(const Graph *g, int k);

/* Find a specific k-clique. Returns 1 if found, fills clique. */
int graph_find_clique(const Graph *g, int k, Clique *result);

/* Enumerate all k-cliques. Returns count, *cliques set to array. */
int graph_enumerate_cliques(const Graph *g, int k, Clique ***cliques);

/* ============================================================
 * L3: Set System / Family of Sets
 * ============================================================ */

/* A family of k-subsets of [n] = {0, 1, ..., n-1} */
typedef struct {
    int           n;          /* Universe size */
    int           k;          /* Each set has size k */
    int           num_sets;   /* Number of sets in family */
    int           capacity;   /* Allocated capacity */
    uint64_t     *sets;       /* Each set as bitmask */
} SetFamily;

/* Create set family */
SetFamily* setfamily_create(int n, int k, int capacity);

/* Add a set (as bitmask) */
void setfamily_add(SetFamily *sf, uint64_t set);

/* Add a set from vertex list */
void setfamily_add_vertices(SetFamily *sf, const int *vertices, int k);

/* Check if set belongs to family */
int setfamily_contains(const SetFamily *sf, uint64_t set);

/* Free */
void setfamily_free(SetFamily *sf);

/* ============================================================
 * L4: Monotone Graph Properties
 * ============================================================ */

/* The CLIQUE function: CLIQUE_{n,k}(G) = 1 iff G has a k-clique.
 * This is the canonical monotone-NP-complete function. */
int monotone_clique_function(const Graph *g, int k);

/* Generate a positive example for CLIQUE_{n,k}: 
 * a graph that HAS a k-clique. */
Graph* graph_generate_clique_positive(int n, int k);

/* Generate a negative example for CLIQUE_{n,k}:
 * a graph that does NOT have a k-clique.
 * Strategy: (k-1)-partite complete graph = Turan graph T(n, k-1) */
Graph* graph_generate_clique_negative(int n, int k);

/* ============================================================
 * L5: Clique Detection Algorithms
 * ============================================================ */

/* Improved clique detection using Bron-Kerbosch (with pivoting).
 * Works for any k, not just k<=4. */
int graph_has_clique_bk(const Graph *g, int k);

/* Maximum clique: find largest k such that g has a k-clique */
int graph_max_clique_size(const Graph *g);

/* Exact k-clique count (not just existence) */
long long graph_count_k_cliques(const Graph *g, int k);

/* ============================================================
 * L6: Canonical Monotone Problems on Graphs
 * ============================================================ */

/* VERTEX COVER: does g have a vertex cover of size ≤ k?
 * Equivalent to: is there a set S of k vertices such that
 * every edge touches at least one vertex in S?
 * VERTEX-COVER is monotone-NP-complete. */
int graph_has_vertex_cover(const Graph *g, int k);

/* INDEPENDENT SET: does g have an independent set of size ≥ k?
 * Complement of CLIQUE in complement graph. */
int graph_has_independent_set(const Graph *g, int k);

/* ST-CONNECTIVITY: is there a path from s to t?
 * Monotone-NC-complete. */
int graph_has_path(const Graph *g, int s, int t);

/* PERFECT MATCHING: does g have a perfect matching?
 * Monotone-P-complete. */
int graph_has_perfect_matching(const Graph *g);

/* ============================================================
 * L7: Graph Utilities
 * ============================================================ */

/* Print graph as adjacency matrix */
void graph_print(const Graph *g);

/* Print graph in Graphviz DOT format */
void graph_print_dot(const Graph *g, FILE *out);

/* Print clique */
void clique_print(const Clique *c);

/* Free clique */
void clique_free(Clique *c);

#endif /* MONOTONE_GRAPH_H */
