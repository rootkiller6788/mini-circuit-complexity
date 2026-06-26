/* nc_matrix.c -- Parallel Matrix Operations (NC^2)
 *
 * Matrix multiplication: O(log n) depth, O(n^3) size.
 * Matrix powering: O(log k * log n) depth — NC^2.
 * Transitive Closure: O(log^2 n) depth — NC^2.
 * This construction proves NL subseteq NC^2.
 *
 * Reference:
 *   Csanky (1976) Fast parallel matrix inversion
 *   Berkowitz (1984) Computing the determinant in small parallel time
 *   Borodin-Cook-Dymond-Ruzzo-Tompa (1989) NL in NC^2
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include "nc.h"

void matrix_multiply_sequential(const double *A, const double *B,
                                 double *C, int n) {
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++) {
            double sum = 0.0;
            for (int k = 0; k < n; k++)
                sum += A[i * n + k] * B[k * n + j];
            C[i * n + j] = sum;
        }
}

static double tree_sum(const double *arr, int n) {
    if (n <= 0) return 0.0;
    if (n == 1) return arr[0];
    double *work = (double *)malloc((size_t)n * sizeof(double));
    assert(work);
    memcpy(work, arr, (size_t)n * sizeof(double));
    int rem = n;
    while (rem > 1) {
        for (int i = 0; i < rem / 2; i++)
            work[i] = work[2*i] + work[2*i+1];
        if (rem % 2 == 1) work[rem/2] = work[rem-1];
        rem = (rem + 1) / 2;
    }
    double result = work[0]; free(work);
    return result;
}

void matrix_multiply_nc(const double *A, const double *B,
                         double *C, int n) {
    double *prods = (double *)malloc((size_t)n * sizeof(double));
    assert(prods);
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++) {
            for (int k = 0; k < n; k++)
                prods[k] = A[i*n + k] * B[k*n + j];
            C[i*n + j] = tree_sum(prods, n);
        }
    free(prods);
}

void matrix_multiply_strassen(const double *A, const double *B,
                               double *C, int n) {
    int p2 = 1;
    while (p2 < n) p2 <<= 1;
    if (p2 > n) {
        double *Ap = (double *)calloc((size_t)(p2*p2), sizeof(double));
        double *Bp = (double *)calloc((size_t)(p2*p2), sizeof(double));
        double *Cp = (double *)calloc((size_t)(p2*p2), sizeof(double));
        assert(Ap && Bp && Cp);
        for (int i = 0; i < n; i++) {
            memcpy(Ap + i*p2, A + i*n, (size_t)n * sizeof(double));
            memcpy(Bp + i*p2, B + i*n, (size_t)n * sizeof(double));
        }
        matrix_multiply_nc(Ap, Bp, Cp, p2);
        for (int i = 0; i < n; i++)
            memcpy(C + i*n, Cp + i*p2, (size_t)n * sizeof(double));
        free(Ap); free(Bp); free(Cp);
    } else {
        matrix_multiply_nc(A, B, C, n);
    }
}

void matrix_power_nc(const double *A, int n, int k, double *result) {
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++)
            result[i*n + j] = (i == j) ? 1.0 : 0.0;
    double *base = (double *)malloc((size_t)(n*n) * sizeof(double));
    double *temp = (double *)malloc((size_t)(n*n) * sizeof(double));
    assert(base && temp);
    memcpy(base, A, (size_t)(n*n) * sizeof(double));
    int exp = k;
    while (exp > 0) {
        if (exp & 1) {
            matrix_multiply_nc(result, base, temp, n);
            memcpy(result, temp, (size_t)(n*n) * sizeof(double));
        }
        matrix_multiply_nc(base, base, temp, n);
        memcpy(base, temp, (size_t)(n*n) * sizeof(double));
        exp >>= 1;
    }
    free(base); free(temp);
}

void matrix_power_modular(const int *A, int n, int k, int mod, int *result) {
    double *Ad = (double *)malloc((size_t)(n*n) * sizeof(double));
    double *Rd = (double *)malloc((size_t)(n*n) * sizeof(double));
    assert(Ad && Rd);
    for (int i = 0; i < n*n; i++) Ad[i] = (double)A[i];
    matrix_power_nc(Ad, n, k, Rd);
    for (int i = 0; i < n*n; i++) result[i] = ((int)Rd[i]) % mod;
    free(Ad); free(Rd);
}

/* Transitive closure via Boolean matrix powering.
 * A* = (I OR A)^n using Boolean semiring (OR=+, AND=x).
 * Depth: O(log^2 n) — log n squarings, each O(log n) depth.
 * This proves NL subseteq NC^2 (Borodin et al. 1989). */
void transitive_closure_nc(const int *adj, int n, int *closure) {
    int *cur = (int *)malloc((size_t)(n*n) * sizeof(int));
    int *nxt = (int *)malloc((size_t)(n*n) * sizeof(int));
    assert(cur && nxt);
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++)
            cur[i*n + j] = (i == j) || adj[i*n + j];
    int steps = (int)ceil(log2((double)n));
    for (int s = 0; s < steps; s++) {
        for (int i = 0; i < n; i++)
            for (int j = 0; j < n; j++) {
                int val = 0;
                for (int k = 0; k < n; k++)
                    val |= cur[i*n + k] && cur[k*n + j];
                nxt[i*n + j] = val;
            }
        memcpy(cur, nxt, (size_t)(n*n) * sizeof(int));
    }
    memcpy(closure, cur, (size_t)(n*n) * sizeof(int));
    free(cur); free(nxt);
}

void all_pairs_shortest_path_nc(const int *adj, int n, int *dist) {
    int *cur = (int *)malloc((size_t)(n*n) * sizeof(int));
    int *nxt = (int *)malloc((size_t)(n*n) * sizeof(int));
    assert(cur && nxt);
    const int INF = 1000000000;
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++) {
            if (i == j) cur[i*n + j] = 0;
            else if (adj[i*n + j]) cur[i*n + j] = adj[i*n + j];
            else cur[i*n + j] = INF;
        }
    int steps = (int)ceil(log2((double)n));
    for (int s = 0; s < steps; s++) {
        for (int i = 0; i < n; i++)
            for (int j = 0; j < n; j++) {
                int min_d = cur[i*n + j];
                for (int k = 0; k < n; k++) {
                    int d = cur[i*n + k] + cur[k*n + j];
                    if (d < min_d) min_d = d;
                }
                nxt[i*n + j] = min_d;
            }
        memcpy(cur, nxt, (size_t)(n*n) * sizeof(int));
    }
    memcpy(dist, cur, (size_t)(n*n) * sizeof(int));
    free(cur); free(nxt);
}

int graph_is_connected(const int *adj, int n) {
    int *closure = (int *)malloc((size_t)(n*n) * sizeof(int));
    assert(closure);
    transitive_closure_nc(adj, n, closure);
    int connected = 1;
    for (int i = 0; i < n && connected; i++)
        for (int j = 0; j < n && connected; j++)
            if (!closure[i*n + j]) connected = 0;
    free(closure);
    return connected;
}

void connected_components_nc(const int *adj, int n, int *component) {
    int *closure = (int *)malloc((size_t)(n*n) * sizeof(int));
    assert(closure);
    transitive_closure_nc(adj, n, closure);
    int *visited = (int *)calloc((size_t)n, sizeof(int));
    int comp_id = 0;
    for (int i = 0; i < n; i++) {
        if (visited[i]) continue;
        comp_id++;
        for (int j = 0; j < n; j++)
            if (closure[i*n + j]) {
                component[j] = comp_id;
                visited[j] = 1;
            }
    }
    free(closure); free(visited);
}

static void pmd(const double *M, int n, const char *label) {
    printf("  %s:\n", label);
    for (int i = 0; i < n && i < 6; i++) {
        printf("    ");
        for (int j = 0; j < n && j < 6; j++)
            printf("%6.1f ", M[i*n + j]);
        if (n > 6) printf("...");
        printf("\n");
    }
    if (n > 6) printf("    ...\n");
}

static void pmi(const int *M, int n, const char *label) {
    printf("  %s:\n", label);
    for (int i = 0; i < n && i < 6; i++) {
        printf("    ");
        for (int j = 0; j < n && j < 6; j++)
            printf("%2d ", M[i*n + j]);
        if (n > 6) printf("...");
        printf("\n");
    }
    if (n > 6) printf("    ...\n");
}

/* ================================================================
 * DEMOS
 * ================================================================ */

void matrix_mult_demo(void) {
    printf("\n================================================================\n");
    printf("  MATRIX MULTIPLICATION IN NC\n");
    printf("================================================================\n\n");

    int n = 4;
    double A2[16] = {1,0,0,1, 0,1,1,0, 1,0,1,0, 0,1,0,1};
    double B2[16] = {1,1,0,0, 0,1,1,0, 0,0,1,1, 1,0,0,1};
    double C_seq[16], C_nc[16];
    matrix_multiply_sequential(A2, B2, C_seq, n);
    matrix_multiply_nc(A2, B2, C_nc, n);
    pmd(A2, n, "A");
    pmd(B2, n, "B");
    pmd(C_seq, n, "A x B (seq)");

    double P4[16];
    matrix_power_nc(A2, n, 4, P4);
    pmd(P4, n, "A^4");

    printf("\n--- Transitive Closure (Graph Reachability in NC^2) ---\n");
    int adj[25] = {0,1,0,0,0, 0,0,1,0,0, 0,0,0,1,0, 0,0,0,0,1, 0,0,0,0,0};
    int closure[25];
    transitive_closure_nc(adj, 5, closure);
    pmi(adj, 5, "Adjacency (path length 5)");
    pmi(closure, 5, "Reachability");

    int comp[5];
    connected_components_nc(adj, 5, comp);
    printf("  Components: ");
    for (int i = 0; i < 5; i++) printf("%d ", comp[i]);
    printf("\n");

    printf("\n--- NC Complexity ---\n");
    printf("  Matrix multiply: O(log n) depth, O(n^3) size (NC^1)\n");
    printf("  Matrix power:    O(log k * log n) depth (NC^2)\n");
    printf("  Transitive closure: O(log^2 n) depth (NC^2)\n");
    printf("  NL in NC^2 via transitive closure (Borodin et al. 1989)\n");
}

void graph_reachability_demo(void) {
    printf("\n================================================================\n");
    printf("  GRAPH REACHABILITY IN NC^2\n");
    printf("================================================================\n\n");

    int adj[16] = {0,1,1,0, 0,0,0,1, 0,0,0,1, 0,0,0,0};
    int closure[16], dist[16];
    transitive_closure_nc(adj, 4, closure);
    all_pairs_shortest_path_nc(adj, 4, dist);
    pmi(adj, 4, "Adjacency (4-vertex DAG)");
    pmi(closure, 4, "Reachability (transitive closure)");
    pmi(dist, 4, "Shortest paths (all-pairs)");
    printf("  Connected: %s\n", graph_is_connected(adj,4) ? "yes" : "no");

    /* A second example: cycle graph */
    printf("\n--- Second Example: Cycle ---\n");
    int adj2[9] = {0,1,0, 0,0,1, 1,0,0};
    int closure2[9];
    transitive_closure_nc(adj2, 3, closure2);
    pmi(adj2, 3, "Adjacency (directed 3-cycle)");
    pmi(closure2, 3, "Reachability (all reachable)");

    printf("\n--- Theoretical Significance ---\n");
    printf("  Reachability is NL-complete.\n");
    printf("  Transitive closure in NC^2 proves NL subseteq NC^2.\n");
    printf("  NL = coNL (Immerman-Szelepcsenyi 1988).\n");
    printf("  L subseteq NL subseteq NC^2 subseteq NC subseteq P.\n");
    printf("  Whether NL = NC^2 is open.\n");
}