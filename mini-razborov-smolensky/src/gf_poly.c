/*============================================================================
 * gf_poly.c - GF(p) Polynomial Library (Razborov-Smolensky)
 *
 * Multilinear polynomial representation over GF(p) for Boolean inputs.
 * Since x_i in {0,1}, we have x_i^k = x_i for all k>=1, so every
 * polynomial can be written as a sum of products of distinct variables.
 *
 * This is the central algebraic tool for the Razborov-Smolensky method:
 * circuit gates are replaced by low-degree GF(p) polynomials, and the
 * degree explosion across depth yields the lower bound.
 *
 * L1: GFPoly definition, L3: GF(p) field arithmetic,
 * L4: Fermat's Little Theorem, L5: Polynomial algorithms.
 *
 * References:
 *   Lidl & Niederreiter, "Finite Fields" (1997)
 *   Arora & Barak, "Computational Complexity", Sec.14.4
 *==========================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include <time.h>
#include "gf_poly.h"

/* =========================================================================
 * GF(p) Arithmetic Helpers
 * ========================================================================= */

int gf_add(int a, int b, int p) {
    return ((a % p) + (b % p)) % p;
}

int gf_sub(int a, int b, int p) {
    int r = (a % p) - (b % p);
    return (r < 0) ? r + p : r % p;
}

int gf_mul(int a, int b, int p) {
    return ((a % p) * (b % p)) % p;
}

int gf_pow(int a, int e, int p) {
    /* Fast exponentiation: a^e mod p, O(log e) */
    a = a % p;
    if (a < 0) a += p;
    int result = 1 % p;
    int base = a;
    while (e > 0) {
        if (e & 1) result = (result * base) % p;
        base = (base * base) % p;
        e >>= 1;
    }
    return result;
}

int gf_inv(int a, int p) {
    /* Modular inverse via Fermat: a^{p-2} mod p */
    a = a % p;
    if (a < 0) a += p;
    if (a == 0) return 0;
    return gf_pow(a, p - 2, p);
}

int gf_neg(int a, int p) {
    a = a % p;
    if (a < 0) a += p;
    return (a == 0) ? 0 : p - a;
}

int gf_is_field(int p) {
    if (p < 2) return 0;
    if (p == 2 || p == 3) return 1;
    if (p % 2 == 0) return 0;
    for (int i = 3; i * i <= p; i += 2)
        if (p % i == 0) return 0;
    return 1;
}

/* =========================================================================
 * Internal helpers
 * ========================================================================= */

#define GFPOLY_INIT_CAP 64

static void gf_poly_ensure_capacity(GFPoly* pl, int needed) {
    if (pl->capacity >= needed) return;
    int new_cap = pl->capacity * 2;
    if (new_cap < needed) new_cap = needed;
    if (new_cap < GFPOLY_INIT_CAP) new_cap = GFPOLY_INIT_CAP;
    pl->coeffs = realloc(pl->coeffs, (size_t)new_cap * sizeof(int));
    pl->mons   = realloc(pl->mons,   (size_t)new_cap * sizeof(int*));
    pl->degs   = realloc(pl->degs,   (size_t)new_cap * sizeof(int));
    pl->capacity = new_cap;
    if (!pl->coeffs || !pl->mons || !pl->degs) {
        fprintf(stderr, "gf_poly_ensure_capacity: out of memory\n");
        exit(1);
    }
}

static int mon_equal(const int* a, const int* b, int n_vars) {
    for (int i = 0; i < n_vars; i++)
        if (a[i] != b[i]) return 0;
    return 1;
}

static int mon_degree(const int* exps, int n_vars) {
    int d = 0;
    for (int i = 0; i < n_vars; i++)
        if (exps[i] > 0) d++;
    return d;
}

/* =========================================================================
 * Polynomial Lifecycle (L5)
 * ========================================================================= */

GFPoly* gf_poly_create(int p, int n_vars) {
    GFPoly* pl = malloc(sizeof(GFPoly));
    if (!pl) { fprintf(stderr, "gf_poly_create: OOM\n"); exit(1); }
    pl->p       = p;
    pl->n_vars  = n_vars;
    pl->n_terms = 0;
    pl->capacity = GFPOLY_INIT_CAP;
    pl->is_normalized = 1;
    pl->coeffs = malloc((size_t)pl->capacity * sizeof(int));
    pl->mons   = malloc((size_t)pl->capacity * sizeof(int*));
    pl->degs   = malloc((size_t)pl->capacity * sizeof(int));
    if (!pl->coeffs || !pl->mons || !pl->degs) {
        fprintf(stderr, "gf_poly_create: OOM\n"); exit(1);
    }
    return pl;
}

GFPoly* gf_poly_copy(const GFPoly* src) {
    GFPoly* dst = gf_poly_create(src->p, src->n_vars);
    for (int i = 0; i < src->n_terms; i++) {
        int* exps = malloc((size_t)src->n_vars * sizeof(int));
        if (!exps) { fprintf(stderr, "gf_poly_copy: OOM\n"); exit(1); }
        memcpy(exps, src->mons[i], (size_t)src->n_vars * sizeof(int));
        gf_poly_add_mono(dst, src->coeffs[i], exps);
        free(exps);
    }
    dst->is_normalized = src->is_normalized;
    return dst;
}

void gf_poly_free(GFPoly* pl) {
    if (!pl) return;
    for (int i = 0; i < pl->n_terms; i++) free(pl->mons[i]);
    free(pl->mons);
    free(pl->coeffs);
    free(pl->degs);
    free(pl);
}

void gf_poly_reset(GFPoly* pl) {
    for (int i = 0; i < pl->n_terms; i++) free(pl->mons[i]);
    pl->n_terms = 0;
    pl->is_normalized = 1;
}

/* =========================================================================
 * Monomial Insertion (L5)
 * ========================================================================= */

void gf_poly_add_mono(GFPoly* pl, int coeff, const int* exps) {
    coeff = coeff % pl->p;
    if (coeff < 0) coeff += pl->p;
    if (coeff == 0) return;
    gf_poly_ensure_capacity(pl, pl->n_terms + 1);
    if (pl->is_normalized) {
        for (int i = 0; i < pl->n_terms; i++) {
            if (mon_equal(pl->mons[i], exps, pl->n_vars)) {
                pl->coeffs[i] = (pl->coeffs[i] + coeff) % pl->p;
                if (pl->coeffs[i] == 0) {
                    free(pl->mons[i]);
                    pl->n_terms--;
                    if (i < pl->n_terms) {
                        pl->coeffs[i] = pl->coeffs[pl->n_terms];
                        pl->mons[i]   = pl->mons[pl->n_terms];
                        pl->degs[i]   = pl->degs[pl->n_terms];
                    }
                }
                return;
            }
        }
    }
    int idx = pl->n_terms;
    pl->coeffs[idx] = coeff;
    pl->mons[idx]   = malloc((size_t)pl->n_vars * sizeof(int));
    if (!pl->mons[idx]) { fprintf(stderr, "gf_poly_add_mono: OOM\n"); exit(1); }
    memcpy(pl->mons[idx], exps, (size_t)pl->n_vars * sizeof(int));
    pl->degs[idx]   = mon_degree(exps, pl->n_vars);
    pl->n_terms++;
    pl->is_normalized = 0;
}

void gf_poly_add_mono_list(GFPoly* pl, const RSMonoList* ml) {
    for (int i = 0; i < ml->n_terms; i++)
        gf_poly_add_mono(pl, ml->terms[i].coeff, ml->terms[i].exps);
}

/* =========================================================================
 * Normalization (L5)
 * ========================================================================= */

void gf_poly_normalize(GFPoly* pl) {
    if (pl->is_normalized) return;
    if (pl->n_terms <= 1) { pl->is_normalized = 1; return; }
    for (int i = 0; i < pl->n_terms; i++) {
        if (pl->coeffs[i] == 0) continue;
        for (int j = i + 1; j < pl->n_terms; j++) {
            if (pl->coeffs[j] == 0) continue;
            if (mon_equal(pl->mons[i], pl->mons[j], pl->n_vars)) {
                pl->coeffs[i] = (pl->coeffs[i] + pl->coeffs[j]) % pl->p;
                pl->coeffs[j] = 0;
                free(pl->mons[j]);
            }
        }
    }
    int write = 0;
    for (int read = 0; read < pl->n_terms; read++) {
        if (pl->coeffs[read] != 0) {
            if (write != read) {
                pl->coeffs[write] = pl->coeffs[read];
                pl->mons[write]   = pl->mons[read];
                pl->degs[write]   = pl->degs[read];
            }
            write++;
        }
    }
    pl->n_terms = write;
    pl->is_normalized = 1;
}

void gf_poly_sort_by_deg(GFPoly* pl) {
    for (int i = 1; i < pl->n_terms; i++) {
        int c_i  = pl->coeffs[i];
        int* m_i = pl->mons[i];
        int  d_i = pl->degs[i];
        int j = i - 1;
        while (j >= 0 && pl->degs[j] > d_i) {
            pl->coeffs[j+1] = pl->coeffs[j];
            pl->mons[j+1]   = pl->mons[j];
            pl->degs[j+1]   = pl->degs[j];
            j--;
        }
        pl->coeffs[j+1] = c_i;
        pl->mons[j+1]   = m_i;
        pl->degs[j+1]   = d_i;
    }
}

/* =========================================================================
 * Evaluation (L5)
 * ========================================================================= */

int gf_poly_eval(const GFPoly* pl, const int* x) {
    int result = 0;
    for (int t = 0; t < pl->n_terms; t++) {
        if (pl->coeffs[t] == 0) continue;
        int term_val = pl->coeffs[t];
        for (int v = 0; v < pl->n_vars; v++) {
            if (pl->mons[t][v] > 0 && x[v] == 0) {
                term_val = 0;
                break;
            }
        }
        result = (result + term_val) % pl->p;
    }
    return result;
}

int* gf_poly_truth_table(const GFPoly* pl) {
    int rows = 1 << pl->n_vars;
    if (rows > (1 << 24)) {
        fprintf(stderr, "gf_poly_truth_table: n=%d too large\n", pl->n_vars);
        return NULL;
    }
    int* tt = malloc((size_t)rows * sizeof(int));
    if (!tt) { fprintf(stderr, "gf_poly_truth_table: OOM\n"); exit(1); }
    int* x = calloc((size_t)pl->n_vars, sizeof(int));
    if (!x) { free(tt); fprintf(stderr, "OOM\n"); exit(1); }
    for (int r = 0; r < rows; r++) {
        for (int v = 0; v < pl->n_vars; v++)
            x[v] = (r >> v) & 1;
        tt[r] = gf_poly_eval(pl, x);
    }
    free(x);
    return tt;
}

int gf_poly_agrees(const GFPoly* pl, int (*fn)(const int*, int),
                    int n, int* frac_ok) {
    if (pl->n_vars != n) return 0;
    int rows = 1 << n;
    int agree = 0;
    int* x = calloc((size_t)n, sizeof(int));
    if (!x) return -1;
    for (int r = 0; r < rows; r++) {
        for (int v = 0; v < n; v++) x[v] = (r >> v) & 1;
        int fval  = fn(x, n) ? 1 : 0;
        int pval  = gf_poly_eval(pl, x);
        int pbool = (pval != 0) ? 1 : 0;
        if (fval == pbool) agree++;
    }
    free(x);
    if (frac_ok) *frac_ok = agree;
    return (agree == rows);
}

/* =========================================================================
 * Degree & Query (L5)
 * ========================================================================= */

int gf_poly_degree(const GFPoly* pl) {
    int md = 0;
    for (int i = 0; i < pl->n_terms; i++)
        if (pl->coeffs[i] != 0 && pl->degs[i] > md)
            md = pl->degs[i];
    return md;
}

int gf_poly_total_degree(const GFPoly* pl) {
    return gf_poly_degree(pl);
}

double gf_poly_avg_degree(const GFPoly* pl) {
    if (pl->n_terms == 0) return 0.0;
    double sum = 0.0;
    for (int i = 0; i < pl->n_terms; i++)
        sum += (double)pl->degs[i];
    return sum / (double)pl->n_terms;
}

int gf_poly_is_zero(const GFPoly* pl) {
    return (pl->n_terms == 0);
}

int gf_poly_is_constant(const GFPoly* pl) {
    if (pl->n_terms == 0) return 1;
    if (pl->n_terms > 1) return 0;
    for (int v = 0; v < pl->n_vars; v++)
        if (pl->mons[0][v] > 0) return 0;
    return 1;
}

int gf_poly_n_terms(const GFPoly* pl) {
    return pl->n_terms;
}

int64_t gf_poly_sparsity(const GFPoly* pl) {
    return (int64_t)pl->n_terms;
}

/* =========================================================================
 * Comparison (L5)
 * ========================================================================= */

int gf_poly_equal(const GFPoly* a, const GFPoly* b) {
    if (a->p != b->p || a->n_vars != b->n_vars) return 0;
    GFPoly* an = gf_poly_copy(a);
    GFPoly* bn = gf_poly_copy(b);
    gf_poly_normalize(an);
    gf_poly_normalize(bn);
    if (an->n_terms != bn->n_terms) {
        gf_poly_free(an); gf_poly_free(bn); return 0;
    }
    for (int i = 0; i < an->n_terms; i++) {
        int found = 0;
        for (int j = 0; j < bn->n_terms; j++) {
            if (bn->coeffs[j] == 0) continue;
            if (an->coeffs[i] == bn->coeffs[j] &&
                mon_equal(an->mons[i], bn->mons[j], an->n_vars)) {
                bn->coeffs[j] = 0;
                found = 1;
                break;
            }
        }
        if (!found) { gf_poly_free(an); gf_poly_free(bn); return 0; }
    }
    gf_poly_free(an); gf_poly_free(bn);
    return 1;
}

/* =========================================================================
 * Arithmetic Operations (L3, L5)
 * ========================================================================= */

GFPoly* gf_poly_add(const GFPoly* a, const GFPoly* b) {
    int nv = (a->n_vars > b->n_vars) ? a->n_vars : b->n_vars;
    GFPoly* r = gf_poly_create(a->p, nv);
    for (int i = 0; i < a->n_terms; i++)
        gf_poly_add_mono(r, a->coeffs[i], a->mons[i]);
    for (int j = 0; j < b->n_terms; j++)
        gf_poly_add_mono(r, b->coeffs[j], b->mons[j]);
    gf_poly_normalize(r);
    return r;
}

GFPoly* gf_poly_sub(const GFPoly* a, const GFPoly* b) {
    int nv = (a->n_vars > b->n_vars) ? a->n_vars : b->n_vars;
    GFPoly* r = gf_poly_create(a->p, nv);
    for (int i = 0; i < a->n_terms; i++)
        gf_poly_add_mono(r, a->coeffs[i], a->mons[i]);
    for (int j = 0; j < b->n_terms; j++)
        gf_poly_add_mono(r, gf_neg(b->coeffs[j], a->p), b->mons[j]);
    gf_poly_normalize(r);
    return r;
}

GFPoly* gf_poly_mul(const GFPoly* a, const GFPoly* b) {
    int p  = a->p;
    int nv = (a->n_vars > b->n_vars) ? a->n_vars : b->n_vars;
    GFPoly* r = gf_poly_create(p, nv);
    for (int i = 0; i < a->n_terms; i++) {
        if (a->coeffs[i] == 0) continue;
        for (int j = 0; j < b->n_terms; j++) {
            if (b->coeffs[j] == 0) continue;
            int coeff = gf_mul(a->coeffs[i], b->coeffs[j], p);
            if (coeff == 0) continue;
            int* exps = calloc((size_t)nv, sizeof(int));
            if (!exps) { fprintf(stderr, "gf_poly_mul: OOM\n"); exit(1); }
            for (int v = 0; v < nv; v++)
                exps[v] = (a->mons[i][v] > 0 || b->mons[j][v] > 0) ? 1 : 0;
            gf_poly_add_mono(r, coeff, exps);
            free(exps);
        }
    }
    gf_poly_normalize(r);
    return r;
}

GFPoly* gf_poly_scalar_mul(const GFPoly* a, int c) {
    c = c % a->p;
    if (c < 0) c += a->p;
    GFPoly* r = gf_poly_create(a->p, a->n_vars);
    if (c == 0) return r;
    for (int i = 0; i < a->n_terms; i++) {
        int nc = gf_mul(a->coeffs[i], c, a->p);
        if (nc != 0) gf_poly_add_mono(r, nc, a->mons[i]);
    }
    r->is_normalized = a->is_normalized;
    return r;
}

GFPoly* gf_poly_pow(const GFPoly* a, int exp) {
    GFPoly* result = gf_poly_one(a->p, a->n_vars);
    GFPoly* base   = gf_poly_copy(a);
    int e = exp;
    while (e > 0) {
        if (e & 1) {
            GFPoly* tmp = gf_poly_mul(result, base);
            gf_poly_free(result);
            result = tmp;
        }
        GFPoly* tmp = gf_poly_mul(base, base);
        gf_poly_free(base);
        base = tmp;
        e >>= 1;
    }
    gf_poly_free(base);
    return result;
}

/* In-place arithmetic */

void gf_poly_iadd(GFPoly* a, const GFPoly* b) {
    for (int j = 0; j < b->n_terms; j++)
        gf_poly_add_mono(a, b->coeffs[j], b->mons[j]);
    gf_poly_normalize(a);
}

void gf_poly_isub(GFPoly* a, const GFPoly* b) {
    for (int j = 0; j < b->n_terms; j++)
        gf_poly_add_mono(a, gf_neg(b->coeffs[j], a->p), b->mons[j]);
    gf_poly_normalize(a);
}

void gf_poly_imul(GFPoly* a, const GFPoly* b) {
    GFPoly* tmp = gf_poly_mul(a, b);
    for (int i = 0; i < a->n_terms; i++) free(a->mons[i]);
    free(a->mons); free(a->coeffs); free(a->degs);
    a->coeffs       = tmp->coeffs;
    a->mons         = tmp->mons;
    a->degs         = tmp->degs;
    a->n_terms      = tmp->n_terms;
    a->capacity     = tmp->capacity;
    a->is_normalized = tmp->is_normalized;
    tmp->coeffs = NULL; tmp->mons = NULL; tmp->degs = NULL;
    tmp->n_terms = 0; tmp->capacity = 0;
    gf_poly_free(tmp);
}

void gf_poly_iscale(GFPoly* a, int c) {
    c = c % a->p;
    if (c < 0) c += a->p;
    if (c == 0) { gf_poly_reset(a); return; }
    for (int i = 0; i < a->n_terms; i++)
        a->coeffs[i] = gf_mul(a->coeffs[i], c, a->p);
}

/* =========================================================================
 * Special Polynomials (L5)
 * ========================================================================= */

GFPoly* gf_poly_constant(int p, int n_vars, int c) {
    GFPoly* pl = gf_poly_create(p, n_vars);
    int* zero_exps = calloc((size_t)n_vars, sizeof(int));
    if (!zero_exps) { fprintf(stderr, "gf_poly_constant: OOM\n"); exit(1); }
    gf_poly_add_mono(pl, c, zero_exps);
    free(zero_exps);
    pl->is_normalized = 1;
    return pl;
}

GFPoly* gf_poly_variable(int p, int n_vars, int var_idx) {
    GFPoly* pl = gf_poly_create(p, n_vars);
    int* exps = calloc((size_t)n_vars, sizeof(int));
    if (!exps) { fprintf(stderr, "gf_poly_variable: OOM\n"); exit(1); }
    if (var_idx >= 0 && var_idx < n_vars) exps[var_idx] = 1;
    gf_poly_add_mono(pl, 1, exps);
    free(exps);
    pl->is_normalized = 1;
    return pl;
}

GFPoly* gf_poly_random(int p, int n_vars, int max_deg, int max_terms,
                        unsigned int seed) {
    srand(seed);
    GFPoly* pl = gf_poly_create(p, n_vars);
    int actual_terms = 1 + (rand() % max_terms);
    for (int t = 0; t < actual_terms; t++) {
        int coeff = 1 + (rand() % (p - 1));
        int* exps = calloc((size_t)n_vars, sizeof(int));
        if (!exps) { fprintf(stderr, "gf_poly_random: OOM\n"); exit(1); }
        int target_deg = rand() % (max_deg + 1);
        int placed = 0;
        while (placed < target_deg) {
            int v = rand() % n_vars;
            if (exps[v] == 0) { exps[v] = 1; placed++; }
        }
        gf_poly_add_mono(pl, coeff, exps);
        free(exps);
    }
    gf_poly_normalize(pl);
    return pl;
}

GFPoly* gf_poly_zero(int p, int n_vars) {
    return gf_poly_create(p, n_vars);
}

GFPoly* gf_poly_one(int p, int n_vars) {
    return gf_poly_constant(p, n_vars, 1);
}

/* =========================================================================
 * Print/Debug (L5)
 * ========================================================================= */

void gf_poly_print(const GFPoly* pl) {
    gf_poly_fprint(stdout, pl);
}

void gf_poly_fprint(FILE* fp, const GFPoly* pl) {
    if (pl->n_terms == 0) { fprintf(fp, "0"); return; }
    int printed = 0;
    for (int i = 0; i < pl->n_terms; i++) {
        if (pl->coeffs[i] == 0) continue;
        if (printed > 0) fprintf(fp, " + ");
        fprintf(fp, "%d", pl->coeffs[i]);
        for (int v = 0; v < pl->n_vars; v++)
            if (pl->mons[i][v] > 0) fprintf(fp, "*x%d", v);
        printed++;
    }
    if (printed == 0) fprintf(fp, "0");
}

char* gf_poly_to_string(const GFPoly* pl) {
    static char buf[4096];
    if (pl->n_terms == 0) { snprintf(buf, sizeof(buf), "0"); return buf; }
    int pos = 0;
    int printed = 0;
    for (int i = 0; i < pl->n_terms; i++) {
        if (pl->coeffs[i] == 0) continue;
        if (printed > 0) pos += snprintf(buf + pos, sizeof(buf) - pos, " + ");
        pos += snprintf(buf + pos, sizeof(buf) - pos, "%d", pl->coeffs[i]);
        for (int v = 0; v < pl->n_vars; v++)
            if (pl->mons[i][v] > 0)
                pos += snprintf(buf + pos, sizeof(buf) - pos, "*x%d", v);
        printed++;
    }
    if (printed == 0) snprintf(buf, sizeof(buf), "0");
    return buf;
}

/* =========================================================================
 * Gate Polynomial Constructors (L5 - Razborov-Smolensky specific)
 *
 * These functions construct GF(p) polynomials corresponding to
 * Boolean gates. They are the building blocks of the
 * gate-by-gate polynomial translation central to the RS method.
 * ========================================================================= */

GFPoly* poly_and_exact(int p, const int* var_indices, int k) {
    /* AND(x_i1,...,x_ik) = product of k variables. Exact, degree k. */
    int max_var = 0;
    for (int i = 0; i < k; i++)
        if (var_indices[i] > max_var) max_var = var_indices[i];
    int n_vars = max_var + 1;
    GFPoly* pl = gf_poly_create(p, n_vars);
    int* exps = calloc((size_t)n_vars, sizeof(int));
    if (!exps) { fprintf(stderr, "poly_and_exact: OOM\n"); exit(1); }
    for (int i = 0; i < k && var_indices[i] < n_vars; i++)
        exps[var_indices[i]] = 1;
    gf_poly_add_mono(pl, 1, exps);
    free(exps);
    pl->is_normalized = 1;
    return pl;
}

GFPoly* poly_or_exact(int p, const int* var_indices, int k) {
    /* OR = NOT(AND(NOT ...)) = 1 - product(1 - x_i)
     * Expansion: OR = sum_{S nonempty} (-1)^{|S|+1} prod_{i in S} x_i
     * Exact, degree k, up to 2^k - 1 terms. */
    int max_var = 0;
    for (int i = 0; i < k; i++)
        if (var_indices[i] > max_var) max_var = var_indices[i];
    int n_vars = max_var + 1;
    GFPoly* pl = gf_poly_create(p, n_vars);
    /* Constant 1 term */
    int* zero_exps = calloc((size_t)n_vars, sizeof(int));
    if (!zero_exps) { fprintf(stderr, "poly_or_exact: OOM\n"); exit(1); }
    gf_poly_add_mono(pl, 1, zero_exps);
    free(zero_exps);
    /* For each nonempty subset S, add (-1)^{|S|+1} prod_{i in S} x_i */
    int max_subsets = 1 << k;
    if (max_subsets > 1024) max_subsets = 1024;
    for (int mask = 1; mask < max_subsets; mask++) {
        int ns = 0;
        int* exps = calloc((size_t)n_vars, sizeof(int));
        if (!exps) { fprintf(stderr, "poly_or_exact: OOM\n"); exit(1); }
        for (int i = 0; i < k; i++) {
            if (mask & (1 << i)) {
                if (var_indices[i] < n_vars) exps[var_indices[i]] = 1;
                ns++;
            }
        }
        int sign = (ns % 2 == 1) ? -1 : 1; /* (-1)^{ns+1} */
        gf_poly_add_mono(pl, sign, exps);
        free(exps);
    }
    gf_poly_normalize(pl);
    return pl;
}

GFPoly* poly_not_exact(int p, int var_idx, int n_vars) {
    /* NOT(x) = 1 - x. Exact, degree 1. */
    GFPoly* pl = gf_poly_create(p, n_vars);
    int* zero_exps = calloc((size_t)n_vars, sizeof(int));
    if (!zero_exps) { fprintf(stderr, "poly_not_exact: OOM\n"); exit(1); }
    gf_poly_add_mono(pl, 1, zero_exps);
    free(zero_exps);
    int* var_exps = calloc((size_t)n_vars, sizeof(int));
    if (!var_exps) { fprintf(stderr, "poly_not_exact: OOM\n"); exit(1); }
    if (var_idx >= 0 && var_idx < n_vars) var_exps[var_idx] = 1;
    gf_poly_add_mono(pl, -1, var_exps);
    free(var_exps);
    pl->is_normalized = 1;
    return pl;
}

GFPoly* poly_modp_exact(int p, const int* var_indices, int k) {
    /* MOD_p(x) = 1 - (sum x_i)^{p-1} mod p.
     * By Fermat: a^{p-1} = 1 iff a != 0 (mod p).
     * So (sum x_i)^{p-1} = 0 iff sum=0, =1 otherwise.
     * Thus 1 - (sum)^{p-1} = MOD_p(x). Degree = p-1 = O(1). */
    int max_var = 0;
    for (int i = 0; i < k; i++)
        if (var_indices[i] > max_var) max_var = var_indices[i];
    int n_vars = max_var + 1;
    GFPoly* S = gf_poly_create(p, n_vars);
    for (int i = 0; i < k; i++) {
        int* exps = calloc((size_t)n_vars, sizeof(int));
        if (!exps) { fprintf(stderr, "poly_modp_exact: OOM\n"); exit(1); }
        if (var_indices[i] < n_vars) exps[var_indices[i]] = 1;
        gf_poly_add_mono(S, 1, exps);
        free(exps);
    }
    gf_poly_normalize(S);
    GFPoly* S_pow = gf_poly_pow(S, p - 1);
    GFPoly* one = gf_poly_one(p, n_vars);
    GFPoly* result = gf_poly_sub(one, S_pow);
    gf_poly_free(S);
    gf_poly_free(S_pow);
    gf_poly_free(one);
    return result;
}

GFPoly* poly_and_probabilistic(int p, const int* var_indices, int k,
                                unsigned int seed) {
    /* Probabilistic AND approximation (Smolensky 1987):
     * Pick random r_i in GF(p)*, form L = sum r_i x_i, output L^{p-1}.
     * Error <= k/p per input. Degree = p-1, independent of k! */
    int max_var = 0;
    for (int i = 0; i < k; i++)
        if (var_indices[i] > max_var) max_var = var_indices[i];
    int n_vars = max_var + 1;
    GFPoly* L = gf_poly_create(p, n_vars);
    srand(seed);
    for (int i = 0; i < k; i++) {
        int r = 1 + (rand() % (p - 1));
        int* exps = calloc((size_t)n_vars, sizeof(int));
        if (!exps) { fprintf(stderr, "poly_and_probabilistic: OOM\n"); exit(1); }
        if (var_indices[i] < n_vars) exps[var_indices[i]] = 1;
        gf_poly_add_mono(L, r, exps);
        free(exps);
    }
    gf_poly_normalize(L);
    GFPoly* result = gf_poly_pow(L, p - 1);
    gf_poly_free(L);
    return result;
}

GFPoly* poly_or_probabilistic(int p, const int* var_indices, int k,
                               unsigned int seed) {
    /* OR = NOT(AND(NOT ...)): 1 - P_and. */
    int max_var = 0;
    for (int i = 0; i < k; i++)
        if (var_indices[i] > max_var) max_var = var_indices[i];
    int n_vars = max_var + 1;
    GFPoly* P_and = poly_and_probabilistic(p, var_indices, k, seed);
    GFPoly* one = gf_poly_one(p, n_vars);
    GFPoly* result = gf_poly_sub(one, P_and);
    gf_poly_free(one);
    gf_poly_free(P_and);
    return result;
}

GFPoly* poly_and_approx(int p, const int* var_indices, int k, int degree_limit) {
    /* Truncated AND: multiply at most degree_limit variables. */
    int max_var = 0;
    for (int i = 0; i < k; i++)
        if (var_indices[i] > max_var) max_var = var_indices[i];
    int n_vars = max_var + 1;
    GFPoly* pl = gf_poly_create(p, n_vars);
    int use = (k < degree_limit) ? k : degree_limit;
    int* exps = calloc((size_t)n_vars, sizeof(int));
    if (!exps) { fprintf(stderr, "poly_and_approx: OOM\n"); exit(1); }
    for (int i = 0; i < use && var_indices[i] < n_vars; i++)
        exps[var_indices[i]] = 1;
    gf_poly_add_mono(pl, 1, exps);
    free(exps);
    pl->is_normalized = 1;
    return pl;
}

GFPoly* poly_or_approx(int p, const int* var_indices, int k, int degree_limit) {
    /* Truncated OR: expand De Morgan, keep terms up to degree_limit. */
    int max_var = 0;
    for (int i = 0; i < k; i++)
        if (var_indices[i] > max_var) max_var = var_indices[i];
    int n_vars = max_var + 1;
    GFPoly* pl = gf_poly_create(p, n_vars);
    int* zero_exps = calloc((size_t)n_vars, sizeof(int));
    if (!zero_exps) { fprintf(stderr, "poly_or_approx: OOM\n"); exit(1); }
    gf_poly_add_mono(pl, 1, zero_exps);
    free(zero_exps);
    int limit = (k < 10) ? (1 << k) : 1024;
    for (int mask = 1; mask < limit; mask++) {
        int ns = 0;
        int* exps = calloc((size_t)n_vars, sizeof(int));
        if (!exps) { fprintf(stderr, "poly_or_approx: OOM\n"); exit(1); }
        for (int i = 0; i < k; i++) {
            if (mask & (1 << i)) {
                if (var_indices[i] < n_vars) exps[var_indices[i]] = 1;
                ns++;
            }
        }
        if (ns <= degree_limit) {
            int sign = (ns % 2 == 1) ? -1 : 1;
            gf_poly_add_mono(pl, sign, exps);
        }
        free(exps);
    }
    gf_poly_normalize(pl);
    return pl;
}

GFPoly* poly_modp_easy(int p, int n) {
    /* Simplified MOD_p for variables x_0..x_{n-1} */
    int* vars = malloc((size_t)n * sizeof(int));
    if (!vars) { fprintf(stderr, "poly_modp_easy: OOM\n"); exit(1); }
    for (int i = 0; i < n; i++) vars[i] = i;
    GFPoly* result = poly_modp_exact(p, vars, n);
    free(vars);
    return result;
}
