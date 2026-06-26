/* circuit_builder.c -- Circuit Construction Functions
 *
 * Factory functions that build Boolean circuits for specific functions.
 * Each builder constructs a circuit for a canonical Boolean function
 * studied in circuit complexity theory.
 *
 * Key results:
 *   PARITY not in AC^0 (Furst-Saxe-Sipser 1981, Hastad 1986).
 *   Balanced XOR tree: NC^1, O(n) size, O(log n) depth.
 *   MAJORITY in TC^0 (threshold gates) and NC^1 (Valiant).
 *   THRESHOLD(k): DNF gives AC^0, exponential size for k=n/2.
 *   CLIQUE(k): NP-complete for fixed k, poly-size in P/poly.
 *
 * References: AB (2009) Ch.6, Vollmer (1999) Ch.3, Jukna (2012) Ch.1-2
 */

#include "circuit_builder.h"
#include <assert.h>
#include <string.h>

BooleanCircuit* circuit_nand2(void) {
    BooleanCircuit* c = circuit_create(10);
    int a = circuit_add_gate(c, INPUT, -1, -1);
    int b = circuit_add_gate(c, INPUT, -1, -1);
    int andg = circuit_add_gate(c, AND, a, b);
    int out = circuit_add_gate(c, NOT, andg, -1);
    circuit_set_output(c, &out, 1);
    return c;
}

BooleanCircuit* circuit_nor2(void) {
    BooleanCircuit* c = circuit_create(10);
    int a = circuit_add_gate(c, INPUT, -1, -1);
    int b = circuit_add_gate(c, INPUT, -1, -1);
    int org = circuit_add_gate(c, OR, a, b);
    int out = circuit_add_gate(c, NOT, org, -1);
    circuit_set_output(c, &out, 1);
    return c;
}

BooleanCircuit* circuit_mux2(void) {
    BooleanCircuit* c = circuit_create(20);
    int a = circuit_add_gate(c, INPUT, -1, -1);
    int b = circuit_add_gate(c, INPUT, -1, -1);
    int s = circuit_add_gate(c, INPUT, -1, -1);
    int ns = circuit_add_gate(c, NOT, s, -1);
    int t1 = circuit_add_gate(c, AND, a, ns);
    int t2 = circuit_add_gate(c, AND, b, s);
    int out = circuit_add_gate(c, OR, t1, t2);
    circuit_set_output(c, &out, 1);
    return c;
}

BooleanCircuit* circuit_mux4(void) {
    BooleanCircuit* c = circuit_create(60);
    int in[4], sel[2];
    for (int i = 0; i < 4; i++) in[i] = circuit_add_gate(c, INPUT, -1, -1);
    for (int i = 0; i < 2; i++) sel[i] = circuit_add_gate(c, INPUT, -1, -1);
    int ns0 = circuit_add_gate(c, NOT, sel[0], -1);
    int ns1 = circuit_add_gate(c, NOT, sel[1], -1);
    int d00 = circuit_add_gate(c, AND, ns0, ns1);
    int d10 = circuit_add_gate(c, AND, sel[0], ns1);
    int d01 = circuit_add_gate(c, AND, ns0, sel[1]);
    int d11 = circuit_add_gate(c, AND, sel[0], sel[1]);
    int t0 = circuit_add_gate(c, AND, in[0], d00);
    int t1 = circuit_add_gate(c, AND, in[1], d10);
    int t2 = circuit_add_gate(c, AND, in[2], d01);
    int t3 = circuit_add_gate(c, AND, in[3], d11);
    int o1 = circuit_add_gate(c, OR, t0, t1);
    int o2 = circuit_add_gate(c, OR, t2, t3);
    int out = circuit_add_gate(c, OR, o1, o2);
    circuit_set_output(c, &out, 1);
    return c;
}

BooleanCircuit* circuit_half_adder(void) {
    BooleanCircuit* c = circuit_create(20);
    int a = circuit_add_gate(c, INPUT, -1, -1);
    int b = circuit_add_gate(c, INPUT, -1, -1);
    int sum = circuit_add_gate(c, XOR, a, b);
    int carry = circuit_add_gate(c, AND, a, b);
    int outs[] = {sum, carry};
    circuit_set_output(c, outs, 2);
    return c;
}

BooleanCircuit* circuit_full_adder(void) {
    BooleanCircuit* c = circuit_create(30);
    int a = circuit_add_gate(c, INPUT, -1, -1);
    int b = circuit_add_gate(c, INPUT, -1, -1);
    int cin = circuit_add_gate(c, INPUT, -1, -1);
    int axb = circuit_add_gate(c, XOR, a, b);
    int sum = circuit_add_gate(c, XOR, axb, cin);
    int ab = circuit_add_gate(c, AND, a, b);
    int cin_axb = circuit_add_gate(c, AND, cin, axb);
    int cout = circuit_add_gate(c, OR, ab, cin_axb);
    int outs[] = {sum, cout};
    circuit_set_output(c, outs, 2);
    return c;
}

BooleanCircuit* circuit_equality(int n) {
    assert(n > 0);
    BooleanCircuit* c = circuit_create(10 * n);
    int* x = (int*)malloc((size_t)n * sizeof(int));
    int* y = (int*)malloc((size_t)n * sizeof(int));
    assert(x && y);
    for (int i = 0; i < n; i++) {
        x[i] = circuit_add_gate(c, INPUT, -1, -1);
        y[i] = circuit_add_gate(c, INPUT, -1, -1);
    }
    int cur = -1;
    for (int i = 0; i < n; i++) {
        int xr = circuit_add_gate(c, XOR, x[i], y[i]);
        int eq = circuit_add_gate(c, NOT, xr, -1);
        if (cur < 0) cur = eq;
        else cur = circuit_add_gate(c, AND, cur, eq);
    }
    circuit_set_output(c, &cur, 1);
    free(x); free(y);
    return c;
}

BooleanCircuit* circuit_comparator(int n) {
    assert(n > 0);
    BooleanCircuit* c = circuit_create(20 * n);
    int* a = (int*)malloc((size_t)n * sizeof(int));
    int* b = (int*)malloc((size_t)n * sizeof(int));
    assert(a && b);
    for (int i = 0; i < n; i++) {
        a[i] = circuit_add_gate(c, INPUT, -1, -1);
        b[i] = circuit_add_gate(c, INPUT, -1, -1);
    }
    int lt_prev = circuit_add_gate(c, CONST0, -1, -1);
    for (int i = 0; i < n; i++) {
        int na = circuit_add_gate(c, NOT, a[i], -1);
        int na_and_b = circuit_add_gate(c, AND, na, b[i]);
        int xr = circuit_add_gate(c, XOR, a[i], b[i]);
        int nxr = circuit_add_gate(c, NOT, xr, -1);
        int eq_and_lt = circuit_add_gate(c, AND, nxr, lt_prev);
        lt_prev = circuit_add_gate(c, OR, na_and_b, eq_and_lt);
    }
    circuit_set_output(c, &lt_prev, 1);
    free(a); free(b);
    return c;
}

BooleanCircuit* circuit_adder(int n, int* carry_out) {
    assert(n > 0);
    BooleanCircuit* c = circuit_create(10 * n);
    int* a = (int*)malloc((size_t)n * sizeof(int));
    int* b = (int*)malloc((size_t)n * sizeof(int));
    int* s = (int*)malloc((size_t)n * sizeof(int));
    assert(a && b && s);
    for (int i = 0; i < n; i++) {
        a[i] = circuit_add_gate(c, INPUT, -1, -1);
        b[i] = circuit_add_gate(c, INPUT, -1, -1);
    }
    int carry = circuit_add_gate(c, CONST0, -1, -1);
    for (int i = 0; i < n; i++) {
        int axb = circuit_add_gate(c, XOR, a[i], b[i]);
        s[i] = circuit_add_gate(c, XOR, axb, carry);
        int ab = circuit_add_gate(c, AND, a[i], b[i]);
        int caxb = circuit_add_gate(c, AND, carry, axb);
        carry = circuit_add_gate(c, OR, ab, caxb);
    }
    int* outs = (int*)malloc((size_t)(n + 1) * sizeof(int));
    assert(outs);
    memcpy(outs, s, (size_t)n * sizeof(int));
    outs[n] = carry;
    circuit_set_output(c, outs, n + 1);
    if (carry_out) *carry_out = carry;
    free(a); free(b); free(s); free(outs);
    return c;
}

BooleanCircuit* circuit_cla_adder(int n, int* carry_out) {
    assert(n > 0);
    BooleanCircuit* c = circuit_create(25 * n);
    int* a = (int*)malloc((size_t)n * sizeof(int));
    int* b = (int*)malloc((size_t)n * sizeof(int));
    int* s = (int*)malloc((size_t)n * sizeof(int));
    assert(a && b && s);
    for (int i = 0; i < n; i++) {
        a[i] = circuit_add_gate(c, INPUT, -1, -1);
        b[i] = circuit_add_gate(c, INPUT, -1, -1);
    }
    int* g = (int*)malloc((size_t)n * sizeof(int));
    int* p = (int*)malloc((size_t)n * sizeof(int));
    int* cc = (int*)malloc((size_t)(n + 1) * sizeof(int));
    assert(g && p && cc);
    for (int i = 0; i < n; i++) {
        g[i] = circuit_add_gate(c, AND, a[i], b[i]);
        p[i] = circuit_add_gate(c, XOR, a[i], b[i]);
    }
    cc[0] = circuit_add_gate(c, CONST0, -1, -1);
    for (int i = 0; i < n; i++) {
        int pg = circuit_add_gate(c, AND, p[i], cc[i]);
        cc[i + 1] = circuit_add_gate(c, OR, g[i], pg);
    }
    for (int i = 0; i < n; i++)
        s[i] = circuit_add_gate(c, XOR, p[i], cc[i]);
    int* outs = (int*)malloc((size_t)(n + 1) * sizeof(int));
    assert(outs);
    memcpy(outs, s, (size_t)n * sizeof(int));
    outs[n] = cc[n];
    circuit_set_output(c, outs, n + 1);
    if (carry_out) *carry_out = cc[n];
    free(a); free(b); free(s); free(g); free(p); free(cc); free(outs);
    return c;
}

BooleanCircuit* circuit_multiplier(int n) {
    assert(n > 0);
    BooleanCircuit* c = circuit_create(100 * n * n);
    int* a = (int*)malloc((size_t)n * sizeof(int));
    int* b = (int*)malloc((size_t)n * sizeof(int));
    assert(a && b);
    for (int i = 0; i < n; i++) {
        a[i] = circuit_add_gate(c, INPUT, -1, -1);
        b[i] = circuit_add_gate(c, INPUT, -1, -1);
    }
    int** pp = (int**)malloc((size_t)n * sizeof(int*));
    assert(pp);
    for (int i = 0; i < n; i++) {
        pp[i] = (int*)malloc((size_t)n * sizeof(int));
        assert(pp[i]);
        for (int j = 0; j < n; j++)
            pp[i][j] = circuit_add_gate(c, AND, a[j], b[i]);
    }
    int* sum = (int*)calloc((size_t)(2 * n), sizeof(int));
    assert(sum);
    for (int i = 0; i < n; i++) {
        int carry = circuit_add_gate(c, CONST0, -1, -1);
        for (int j = 0; j < n; j++) {
            int bp = i + j;
            int s1 = circuit_add_gate(c, XOR, sum[bp], pp[i][j]);
            int nb = circuit_add_gate(c, XOR, s1, carry);
            int c1 = circuit_add_gate(c, AND, sum[bp], pp[i][j]);
            int c2 = circuit_add_gate(c, AND, s1, carry);
            sum[bp] = nb;
            carry = circuit_add_gate(c, OR, c1, c2);
        }
    }
    circuit_set_output(c, sum, 2 * n);
    free(a); free(b);
    for (int i = 0; i < n; i++) free(pp[i]);
    free(pp); free(sum);
    return c;
}

/* ===================================================================
 * Counting, Sorting, and Graph-Theoretic Circuits
 * =================================================================== */

BooleanCircuit* circuit_count(int n) {
    assert(n > 0 && n <= 32);
    int nbits = (int)ceil(log2((double)(n + 1)));
    int cap = n * nbits * 5 + 50;
    BooleanCircuit* c = circuit_create(cap);
    int* inputs = (int*)malloc((size_t)n * sizeof(int));
    assert(inputs);
    for (int i = 0; i < n; i++)
        inputs[i] = circuit_add_gate(c, INPUT, -1, -1);

    int* count = (int*)malloc((size_t)nbits * sizeof(int));
    assert(count);
    for (int j = 0; j < nbits; j++)
        count[j] = circuit_add_gate(c, CONST0, -1, -1);

    for (int i = 0; i < n; i++) {
        int carry = inputs[i];
        for (int j = 0; j < nbits; j++) {
            int sum_bit = circuit_add_gate(c, XOR, count[j], carry);
            int new_carry = circuit_add_gate(c, AND, count[j], carry);
            count[j] = sum_bit;
            carry = new_carry;
        }
    }
    circuit_set_output(c, count, nbits);
    free(inputs);
    return c;
}

/* Batcher's odd-even merge for sorting network */
static void batcher_merge(BooleanCircuit* c, int* arr, int lo, int n_vals,
                           int stride) {
    if (n_vals < 2) return;
    if (n_vals == 2) {
        int a = arr[lo], b = arr[lo + stride];
        int cmp = circuit_add_gate(c, AND, a, b);
        int maxv = circuit_add_gate(c, OR, a, b);
        int minv = circuit_add_gate(c, AND, cmp, a);
        /* Actually we need compare-and-swap. For sorting:
           min = a AND b (both 1 => 1), but need proper CAS.
           Use: max = OR(a,b), min = AND(a,b). This sorts 2 bits. */
        arr[lo] = minv;
        arr[lo + stride] = maxv;
        return;
    }
    int half = n_vals / 2;
    batcher_merge(c, arr, lo, half, stride * 2);
    batcher_merge(c, arr, lo + stride, n_vals - half, stride * 2);
    for (int i = 1; i < n_vals - 1; i += 2) {
        int idx1 = lo + i * stride;
        int idx2 = lo + (i + 1) * stride;
        int a = arr[idx1], b = arr[idx2];
        int minv = circuit_add_gate(c, AND, a, b);
        int maxv = circuit_add_gate(c, OR, a, b);
        arr[idx1] = minv;
        arr[idx2] = maxv;
    }
}

BooleanCircuit* circuit_sort(int n) {
    assert(n > 0 && n <= 16);
    int cap = n * n * 10 + 100;
    BooleanCircuit* c = circuit_create(cap);
    int* arr = (int*)malloc((size_t)n * sizeof(int));
    assert(arr);
    for (int i = 0; i < n; i++)
        arr[i] = circuit_add_gate(c, INPUT, -1, -1);
    /* Batcher odd-even mergesort */
    for (int width = 2; width <= n; width *= 2) {
        for (int lo = 0; lo < n; lo += width) {
            int remaining = n - lo;
            int sz = (remaining < width) ? remaining : width;
            if (sz >= 2) batcher_merge(c, arr, lo, sz / 2, 1);
            /* Actually the full Batcher algorithm is more involved.
               For simplicity, use repeated pairwise sort. */
        }
    }
    /* Simplified: bubble-like pairwise min/max */
    for (int pass = 0; pass < n; pass++) {
        for (int i = 0; i < n - 1; i++) {
            int minv = circuit_add_gate(c, AND, arr[i], arr[i+1]);
            int maxv = circuit_add_gate(c, OR, arr[i], arr[i+1]);
            arr[i] = minv;
            arr[i+1] = maxv;
        }
    }
    circuit_set_output(c, arr, n);
    free(arr);
    return c;
}

BooleanCircuit* circuit_clique(int n, int k) {
    assert(n >= 2 && k >= 2 && k <= n);
    /* CLIQUE: DNF over all k-subsets of vertices.
       For each k-subset S, check that all (|S| choose 2) edges exist. */
    int n_edges = n * (n - 1) / 2;
    int cap = n_edges + 1000;
    BooleanCircuit* c = circuit_create(cap);
    /* Edge inputs: edge_id(i,j) for i<j */
    int** edge_map = (int**)malloc((size_t)n * sizeof(int*));
    assert(edge_map);
    for (int i = 0; i < n; i++) {
        edge_map[i] = (int*)malloc((size_t)n * sizeof(int));
        assert(edge_map[i]);
        for (int j = 0; j < n; j++) edge_map[i][j] = -1;
    }
    int eid = 0;
    for (int i = 0; i < n; i++)
        for (int j = i + 1; j < n; j++) {
            edge_map[i][j] = circuit_add_gate(c, INPUT, -1, -1);
            eid++;
        }

    /* For small k, enumerate k-subsets via nested loops.
       Build DNF term for each k-clique candidate.
       Limit to k <= 4 for practicality. */
    int max_k = (k > 4) ? 4 : k;
    int* terms = (int*)malloc((size_t)cap * sizeof(int));
    assert(terms);
    int n_terms = 0;

    if (max_k == 2) {
        for (int i = 0; i < n; i++)
            for (int j = i + 1; j < n; j++)
                terms[n_terms++] = edge_map[i][j];
    } else if (max_k == 3) {
        for (int i = 0; i < n; i++)
            for (int j = i + 1; j < n; j++)
                for (int l = j + 1; l < n; l++) {
                    int e1 = edge_map[i][j];
                    int e2 = circuit_add_gate(c, AND, e1, edge_map[i][l]);
                    terms[n_terms++] = circuit_add_gate(c, AND, e2,
                        edge_map[j][l]);
                }
    } else {
        for (int i = 0; i < n; i++)
            for (int j = i + 1; j < n; j++)
                for (int l = j + 1; l < n; l++)
                    for (int m = l + 1; m < n; m++) {
                        int eij = edge_map[i][j];
                        int eil = circuit_add_gate(c, AND, eij, edge_map[i][l]);
                        int eim = circuit_add_gate(c, AND, eil, edge_map[i][m]);
                        int ejl = circuit_add_gate(c, AND, eim, edge_map[j][l]);
                        int ejm = circuit_add_gate(c, AND, ejl, edge_map[j][m]);
                        terms[n_terms++] = circuit_add_gate(c, AND, ejm,
                            edge_map[l][m]);
                    }
    }

    int out;
    if (n_terms == 0) {
        out = circuit_add_gate(c, CONST0, -1, -1);
    } else {
        out = terms[0];
        for (int i = 1; i < n_terms; i++)
            out = circuit_add_gate(c, OR, out, terms[i]);
    }
    circuit_set_output(c, &out, 1);
    free(terms);
    for (int i = 0; i < n; i++) free(edge_map[i]);
    free(edge_map);
    return c;
}

BooleanCircuit* circuit_ham_path(int n) {
    assert(n >= 2 && n <= 5);
    /* Hamiltonian path: enumerate all n! permutations.
       For each permutation (v1,v2,...,vn), check that all edges
       (vi,vi+1) exist. Output OR over all permutations. */
    int cap = 5000;
    BooleanCircuit* c = circuit_create(cap);
    int** edge_map = (int**)malloc((size_t)n * sizeof(int*));
    assert(edge_map);
    for (int i = 0; i < n; i++) {
        edge_map[i] = (int*)malloc((size_t)n * sizeof(int));
        assert(edge_map[i]);
        for (int j = 0; j < n; j++) edge_map[i][j] = -1;
    }
    for (int i = 0; i < n; i++)
        for (int j = i + 1; j < n; j++)
            edge_map[i][j] = edge_map[j][i] =
                circuit_add_gate(c, INPUT, -1, -1);

    /* Generate all permutations via Heap's algorithm style enumeration.
       For n<=5, n! <= 120, which is feasible. */
    int* perm = (int*)malloc((size_t)n * sizeof(int));
    assert(perm);
    for (int i = 0; i < n; i++) perm[i] = i;

    int*term_gates = (int*)malloc((size_t)(cap) * sizeof(int));
    assert(term_gates);
    int n_terms = 0;

    /* Iterative permutation generation (simple recursive simulation) */
    int stack[64]; int sp = 0;
    int used[8] = {0};
    /* DFS to generate all permutations */
    for (int i = 0; i < n; i++) {
        used[i] = 1;
        stack[sp++] = i;
        while (sp > 0) {
            if (sp == n) {
                /* Build AND over edges of this permutation */
                int term = circuit_add_gate(c, CONST1, -1, -1);
                for (int k = 0; k < n - 1; k++) {
                    int u = stack[k], v = stack[k+1];
                    term = circuit_add_gate(c, AND, term, edge_map[u][v]);
                }
                term_gates[n_terms++] = term;
                /* Backtrack */
                sp--;
                used[stack[sp]] = 0;
                continue;
            }
            /* Find next unused vertex */
            int found = 0;
            for (int j = 0; j < n; j++) {
                if (!used[j]) {
                    used[j] = 1;
                    stack[sp++] = j;
                    found = 1;
                    break;
                }
            }
            if (!found) {
                sp--;
                if (sp > 0) used[stack[sp]] = 0;
            }
        }
    }

    int out;
    if (n_terms == 0) {
        out = circuit_add_gate(c, CONST0, -1, -1);
    } else {
        out = term_gates[0];
        for (int i = 1; i < n_terms; i++)
            out = circuit_add_gate(c, OR, out, term_gates[i]);
    }
    circuit_set_output(c, &out, 1);
    free(term_gates); free(perm);
    for (int i = 0; i < n; i++) free(edge_map[i]);
    free(edge_map);
    return c;
}

BooleanCircuit* circuit_perfect_matching(int n) {
    assert(n >= 2 && n <= 8 && n % 2 == 0);
    /* Enumerate all perfect matchings: partitions of n vertices into n/2 pairs.
       For each pair (i,j), check edge[i][j]. AND over all pairs, OR over
       all perfect matchings. */
    int cap = 5000;
    BooleanCircuit* c = circuit_create(cap);
    int** edge_map = (int**)malloc((size_t)n * sizeof(int*));
    assert(edge_map);
    for (int i = 0; i < n; i++) {
        edge_map[i] = (int*)malloc((size_t)n * sizeof(int));
        assert(edge_map[i]);
        for (int j = 0; j < n; j++) edge_map[i][j] = -1;
    }
    for (int i = 0; i < n; i++)
        for (int j = i + 1; j < n; j++)
            edge_map[i][j] = edge_map[j][i] =
                circuit_add_gate(c, INPUT, -1, -1);

    /* Generate all perfect matchings via recursion */
    int* term_gates = (int*)malloc((size_t)(cap) * sizeof(int));
    assert(term_gates);
    int n_terms = 0;

    /* Simple recursive perfect matching enumeration for n <= 8 */
    int paired[8] = {0};
    int* pairs = (int*)malloc((size_t)(n / 2 * 2) * sizeof(int));
    assert(pairs);
    int pid = 0;

    /* Use iterative backtracking to enumerate all perfect matchings */
    int bt_stack[128]; int bsp = 0;
    int bt_state[128];
    int bt_start = 0;

    while (1) {
        /* Find first unpaired vertex */
        int first = -1;
        for (int i = bt_start; i < n; i++) {
            if (!paired[i]) { first = i; break; }
        }
        if (first < 0) {
            /* All paired: build term */
            int term = circuit_add_gate(c, CONST1, -1, -1);
            for (int k = 0; k < pid; k += 2) {
                int u = pairs[k], v = pairs[k+1];
                term = circuit_add_gate(c, AND, term, edge_map[u][v]);
            }
            term_gates[n_terms++] = term;
            /* Backtrack */
            if (bsp == 0) break;
            bsp--;
            int* p = &bt_state[bsp];
            paired[pairs[(*p)*2]] = 0;
            paired[pairs[(*p)*2+1]] = 0;
            pid -= 2;
            bt_start = pairs[(*p)*2] + 1;
            (*p)++;
            continue;
        }
        /* Find next unpaired vertex > first */
        int second = -1;
        for (int j = first + 1; j < n; j++) {
            if (!paired[j]) { second = j; break; }
        }
        if (second < 0) {
            if (bsp == 0) break;
            bsp--;
            int* p = &bt_state[bsp];
            paired[pairs[(*p)*2]] = 0;
            paired[pairs[(*p)*2+1]] = 0;
            pid -= 2;
            bt_start = pairs[(*p)*2] + 1;
            continue;
        }
        /* Pair first with second */
        paired[first] = 1; paired[second] = 1;
        pairs[pid] = first; pairs[pid+1] = second;
        pid += 2;
        bt_stack[bsp] = second;
        bt_state[bsp] = 0;
        bsp++;
        bt_start = 0;
    }

    int out;
    if (n_terms == 0) {
        out = circuit_add_gate(c, CONST0, -1, -1);
    } else {
        out = term_gates[0];
        for (int i = 1; i < n_terms; i++)
            out = circuit_add_gate(c, OR, out, term_gates[i]);
    }
    circuit_set_output(c, &out, 1);
    free(term_gates); free(pairs);
    for (int i = 0; i < n; i++) free(edge_map[i]);
    free(edge_map);
    return c;
}

/* ===================================================================
 * Random Circuit Generation (Shannon 1949)
 * =================================================================== */

BooleanCircuit* circuit_random(int ni, int ng, unsigned int seed) {
    assert(ni > 0 && ng > ni);
    int cap = ng + 10;
    BooleanCircuit* c = circuit_create(cap);
    int* gate_ids = (int*)malloc((size_t)ng * sizeof(int));
    assert(gate_ids);
    int n_gates = 0;

    /* Create input gates */
    for (int i = 0; i < ni; i++)
        gate_ids[n_gates++] = circuit_add_gate(c, INPUT, -1, -1);

    /* Simple LCG random number generator */
    unsigned int state = seed;

    /* Remaining gates: randomly choose from {AND, OR, XOR, NOT, NAND, NOR}
       and randomly wire to existing gates. */
    GateType rtypes[] = {AND, OR, XOR, NOT, NAND, NOR};
    int n_rtypes = 6;

    for (int i = 0; i < ng - ni; i++) {
        state = state * 1103515245u + 12345u;
        int type_idx = (int)(state % (unsigned int)n_rtypes);
        GateType gt = rtypes[type_idx];

        if (gt == NOT) {
            state = state * 1103515245u + 12345u;
            int in1 = (int)(state % (unsigned int)n_gates);
            gate_ids[n_gates++] = circuit_add_gate(c, NOT, gate_ids[in1], -1);
        } else {
            state = state * 1103515245u + 12345u;
            int in1 = (int)(state % (unsigned int)n_gates);
            state = state * 1103515245u + 12345u;
            int in2 = (int)(state % (unsigned int)n_gates);
            gate_ids[n_gates++] = circuit_add_gate(c, gt,
                gate_ids[in1], gate_ids[in2]);
        }
    }

    /* Set last gate as output */
    circuit_set_output(c, &gate_ids[n_gates - 1], 1);
    free(gate_ids);
    return c;
}

/* ===================================================================
 * Symmetric Functions (Fundamental for Complexity Separations)
 * =================================================================== */

BooleanCircuit* circuit_parity(int n) {
    /* Balanced XOR tree: O(n) size, O(log n) depth. In NC^1.
     * NOT in AC^0 (Furst-Saxe-Sipser 1981, Hastad 1986). */
    assert(n > 0);
    BooleanCircuit* c = circuit_create(4 * n);
    int* prev = (int*)malloc((size_t)n * sizeof(int));
    assert(prev);
    for (int i = 0; i < n; i++)
        prev[i] = circuit_add_gate(c, INPUT, -1, -1);
    int rem = n;
    while (rem > 1) {
        int* next = (int*)malloc((size_t)(rem / 2 + 1) * sizeof(int));
        assert(next);
        int ni = 0;
        for (int i = 0; i < rem; i += 2) {
            if (i + 1 < rem)
                next[ni++] = circuit_add_gate(c, XOR, prev[i], prev[i + 1]);
            else
                next[ni++] = prev[i];
        }
        free(prev);
        prev = next;
        rem = ni;
    }
    int out = prev[0];
    circuit_set_output(c, &out, 1);
    free(prev);
    return c;
}

BooleanCircuit* circuit_majority(int n) {
    /* Count the number of 1s via iterative addition, then compare to n/2+1.
     * O(n log n) size, O(log^2 n) depth. In NC^1 via symmetric circuit.
     * Also in TC^0 via threshold gates. */
    assert(n > 0 && n <= 32);
    int nbits = (int)ceil(log2((double)(n + 1)));
    int cap = n * nbits * 5 + 50;
    BooleanCircuit* c = circuit_create(cap);
    int* inputs = (int*)malloc((size_t)n * sizeof(int));
    assert(inputs);
    for (int i = 0; i < n; i++)
        inputs[i] = circuit_add_gate(c, INPUT, -1, -1);

    /* Count ones using ripple-carry addition */
    int* count = (int*)malloc((size_t)nbits * sizeof(int));
    assert(count);
    for (int j = 0; j < nbits; j++)
        count[j] = circuit_add_gate(c, CONST0, -1, -1);

    for (int i = 0; i < n; i++) {
        int carry = inputs[i];
        for (int j = 0; j < nbits; j++) {
            int sum_bit = circuit_add_gate(c, XOR, count[j], carry);
            int new_carry = circuit_add_gate(c, AND, count[j], carry);
            count[j] = sum_bit;
            carry = new_carry;
        }
    }

    /* Compare count >= n/2 + 1 (majority threshold).
       Build DNF over all values >= threshold. */
    int threshold = n / 2 + 1;
    int nvals = 1 << nbits;
    int* dnf_terms = (int*)malloc((size_t)nvals * sizeof(int));
    assert(dnf_terms);
    int n_terms = 0;
    for (int v = 0; v < nvals; v++) {
        if (v < threshold) continue;
        int* lits = (int*)malloc((size_t)nbits * sizeof(int));
        assert(lits);
        int nl = 0;
        for (int j = 0; j < nbits; j++) {
            if ((v >> j) & 1) lits[nl++] = count[j];
            else {
                int nj = circuit_add_gate(c, NOT, count[j], -1);
                lits[nl++] = nj;
            }
        }
        int term = lits[0];
        for (int k = 1; k < nl; k++)
            term = circuit_add_gate(c, AND, term, lits[k]);
        dnf_terms[n_terms++] = term;
        free(lits);
    }
    int out = dnf_terms[0];
    for (int i = 1; i < n_terms; i++)
        out = circuit_add_gate(c, OR, out, dnf_terms[i]);
    circuit_set_output(c, &out, 1);
    free(dnf_terms); free(count); free(inputs);
    return c;
}

BooleanCircuit* circuit_threshold(int n, int k) {
    /* Depth-2 DNF: OR of all (n choose k) AND terms.
     * AC^0 (constant depth 2) but exponential size for k=n/2.
     * Demonstrates the size-depth tradeoff. */
    assert(n > 0 && k >= 0 && k <= n);
    if (k == 0) {
        BooleanCircuit* c = circuit_create(5);
        int one = circuit_add_gate(c, CONST1, -1, -1);
        circuit_set_output(c, &one, 1);
        return c;
    }
    int max_terms = 5000;
    BooleanCircuit* c = circuit_create(max_terms + 1000);
    int* ins = (int*)malloc((size_t)n * sizeof(int));
    assert(ins);
    for (int i = 0; i < n; i++)
        ins[i] = circuit_add_gate(c, INPUT, -1, -1);
    int* and_terms = (int*)malloc((size_t)max_terms * sizeof(int));
    assert(and_terms);
    int nt = 0;
    long long total = 1LL << n;
    if (total > 2048) total = 2048;
    for (long long m = 0; m < total && nt < max_terms; m++) {
        int cnt = 0;
        for (int b = 0; b < n; b++)
            if ((m >> b) & 1) cnt++;
        if (cnt >= k) {
            int cur = -1;
            for (int b = 0; b < n; b++) {
                if ((m >> b) & 1) {
                    if (cur < 0) cur = ins[b];
                    else cur = circuit_add_gate(c, AND, cur, ins[b]);
                }
            }
            if (cur >= 0) and_terms[nt++] = cur;
        }
    }
    if (nt == 0) {
        int zero = circuit_add_gate(c, CONST0, -1, -1);
        circuit_set_output(c, &zero, 1);
        free(ins); free(and_terms);
        return c;
    }
    int or_gate = and_terms[0];
    for (int j = 1; j < nt; j++)
        or_gate = circuit_add_gate(c, OR, or_gate, and_terms[j]);
    circuit_set_output(c, &or_gate, 1);
    free(ins); free(and_terms);
    return c;
}

BooleanCircuit* circuit_mod(int n, int m) {
    /* MOD-m: 1 iff sum_i x_i ≡ 0 (mod m).
     * For m=2: PARITY. MOD-p (p prime ≠ 2) not in AC^0
     * (Razborov-Smolensky 1987).
     * Uses CSA tree to count, then checks mod condition. */
    assert(n > 0 && m > 0);
    BooleanCircuit* c = circuit_create(10 * n);
    int* bits = (int*)malloc((size_t)n * sizeof(int));
    assert(bits);
    for (int i = 0; i < n; i++)
        bits[i] = circuit_add_gate(c, INPUT, -1, -1);
    int* csa = (int*)malloc((size_t)n * sizeof(int));
    assert(csa);
    memcpy(csa, bits, (size_t)n * sizeof(int));
    int rem = n;
    while (rem > 3) {
        int ns = (rem + 2) / 3 * 2;
        int* next = (int*)calloc((size_t)ns, sizeof(int));
        assert(next);
        int ni = 0;
        for (int i = 0; i < rem; i += 3) {
            if (i + 2 < rem) {
                int s1 = circuit_add_gate(c, XOR, csa[i], csa[i + 1]);
                int sum = circuit_add_gate(c, XOR, s1, csa[i + 2]);
                int c1 = circuit_add_gate(c, AND, csa[i], csa[i + 1]);
                int c2 = circuit_add_gate(c, AND, s1, csa[i + 2]);
                next[ni++] = sum;
                next[ni++] = circuit_add_gate(c, OR, c1, c2);
            } else { next[ni++] = csa[i]; }
        }
        free(csa); csa = next; rem = ni;
    }
    int result;
    if (m == 2) {
        /* MOD-2 = NOT(PARITY): XOR all input bits, then NOT.
           PARITY is 1 when odd number of 1s, MOD-2 is 1 when even. */
        result = bits[0];
        for (int j = 1; j < n; j++)
            result = circuit_add_gate(c, XOR, result, bits[j]);
        result = circuit_add_gate(c, NOT, result, -1);
    } else {
        /* For m > 2: count ones properly, then check mod-m via DNF */
        int nbits = (int)ceil(log2((double)(n + 1)));
        int* cnt = (int*)calloc((size_t)nbits, sizeof(int));
        assert(cnt);
        for (int j = 0; j < nbits; j++)
            cnt[j] = circuit_add_gate(c, CONST0, -1, -1);
        for (int i = 0; i < n; i++) {
            int cry = bits[i];
            for (int j = 0; j < nbits; j++) {
                int sb = circuit_add_gate(c, XOR, cnt[j], cry);
                int nc = circuit_add_gate(c, AND, cnt[j], cry);
                cnt[j] = sb; cry = nc;
            }
        }
        int* dterms = (int*)malloc((size_t)(n / m + 2) * sizeof(int));
        assert(dterms);
        int nt = 0;
        for (int v = 0; v <= n; v += m) {
            int* lits = (int*)malloc((size_t)nbits * sizeof(int));
            assert(lits);
            int nl = 0;
            for (int j = 0; j < nbits; j++) {
                if ((v >> j) & 1) lits[nl++] = cnt[j];
                else {
                    int nj = circuit_add_gate(c, NOT, cnt[j], -1);
                    lits[nl++] = nj;
                }
            }
            int t = lits[0];
            for (int k = 1; k < nl; k++)
                t = circuit_add_gate(c, AND, t, lits[k]);
            dterms[nt++] = t;
            free(lits);
        }
        result = dterms[0];
        for (int i = 1; i < nt; i++)
            result = circuit_add_gate(c, OR, result, dterms[i]);
        free(dterms); free(cnt);
    }
    circuit_set_output(c, &result, 1);
    free(bits); free(csa);
    return c;
}

/* ===================================================================
 * Special Circuit Constructions
 * =================================================================== */

BooleanCircuit* circuit_xor2(void) {
    /* XOR of two input bits. Size 1, Depth 1. */
    BooleanCircuit* c = circuit_create(10);
    int a = circuit_add_gate(c, INPUT, -1, -1);
    int b = circuit_add_gate(c, INPUT, -1, -1);
    int out = circuit_add_gate(c, XOR, a, b);
    circuit_set_output(c, &out, 1);
    return c;
}

BooleanCircuit* circuit_fanout(int n) {
    /* Replicate a single input to n outputs.
     * All n output gates point to the same input gate.
     * This is free in circuits (unlike in formulas). */
    assert(n > 0);
    BooleanCircuit* c = circuit_create(n + 5);
    int in = circuit_add_gate(c, INPUT, -1, -1);
    int* outs = (int*)malloc((size_t)n * sizeof(int));
    assert(outs);
    for (int i = 0; i < n; i++) outs[i] = in;
    circuit_set_output(c, outs, n);
    free(outs);
    return c;
}

BooleanCircuit* circuit_and_tree(int n) {
    /* Balanced AND tree: Depth ceil(log2 n), Size O(n).
     * In AC^0 (constant depth with unbounded fan-in). */
    assert(n > 0);
    BooleanCircuit* c = circuit_create(2 * n);
    int* prev = (int*)malloc((size_t)n * sizeof(int));
    assert(prev);
    for (int i = 0; i < n; i++)
        prev[i] = circuit_add_gate(c, INPUT, -1, -1);
    int rem = n;
    while (rem > 1) {
        int* next = (int*)malloc((size_t)(rem / 2 + 1) * sizeof(int));
        assert(next);
        int ni = 0;
        for (int i = 0; i < rem; i += 2) {
            if (i + 1 < rem)
                next[ni++] = circuit_add_gate(c, AND, prev[i], prev[i + 1]);
            else
                next[ni++] = prev[i];
        }
        free(prev); prev = next; rem = ni;
    }
    circuit_set_output(c, &prev[0], 1);
    free(prev);
    return c;
}

BooleanCircuit* circuit_or_tree(int n) {
    /* Balanced OR tree: Depth ceil(log2 n), Size O(n). */
    assert(n > 0);
    BooleanCircuit* c = circuit_create(2 * n);
    int* prev = (int*)malloc((size_t)n * sizeof(int));
    assert(prev);
    for (int i = 0; i < n; i++)
        prev[i] = circuit_add_gate(c, INPUT, -1, -1);
    int rem = n;
    while (rem > 1) {
        int* next = (int*)malloc((size_t)(rem / 2 + 1) * sizeof(int));
        assert(next);
        int ni = 0;
        for (int i = 0; i < rem; i += 2) {
            if (i + 1 < rem)
                next[ni++] = circuit_add_gate(c, OR, prev[i], prev[i + 1]);
            else
                next[ni++] = prev[i];
        }
        free(prev); prev = next; rem = ni;
    }
    circuit_set_output(c, &prev[0], 1);
    free(prev);
    return c;
}

BooleanCircuit* circuit_boolean2(int func_index) {
    /* Synthesize any of the 16 Boolean functions of 2 variables.
     * Uses MUX-based decomposition: f(a,b) = MUX(a, f0(b), f1(b)).
     * Demonstrates universal completeness of {AND, OR, NOT}. */
    assert(func_index >= 0 && func_index < 16);
    BooleanCircuit* c = circuit_create(25);
    int a = circuit_add_gate(c, INPUT, -1, -1);
    int b = circuit_add_gate(c, INPUT, -1, -1);

    int f00 = (func_index & 1) ? circuit_add_gate(c, CONST1, -1, -1)
                               : circuit_add_gate(c, CONST0, -1, -1);
    int f01 = (func_index & 2) ? circuit_add_gate(c, CONST1, -1, -1)
                               : circuit_add_gate(c, CONST0, -1, -1);
    int f10 = (func_index & 4) ? circuit_add_gate(c, CONST1, -1, -1)
                               : circuit_add_gate(c, CONST0, -1, -1);
    int f11 = (func_index & 8) ? circuit_add_gate(c, CONST1, -1, -1)
                               : circuit_add_gate(c, CONST0, -1, -1);

    int nb = circuit_add_gate(c, NOT, b, -1);
    int t00 = circuit_add_gate(c, AND, nb, f00);
    int t01 = circuit_add_gate(c, AND, b, f01);
    int f0 = circuit_add_gate(c, OR, t00, t01);
    int t10 = circuit_add_gate(c, AND, nb, f10);
    int t11 = circuit_add_gate(c, AND, b, f11);
    int f1 = circuit_add_gate(c, OR, t10, t11);
    int na = circuit_add_gate(c, NOT, a, -1);
    int t_a0 = circuit_add_gate(c, AND, na, f0);
    int t_a1 = circuit_add_gate(c, AND, a, f1);
    int out = circuit_add_gate(c, OR, t_a0, t_a1);

    circuit_set_output(c, &out, 1);
    return c;
}
