/* truth_table.c -- Truth Table Implementation for Natural Proofs Barrier
 *
 * L1: Boolean function f:{0,1}^n -> {0,1} represented as array of 2^n bits.
 * L2: Core operations: create, random, evaluate, compare, boolean operations.
 * L3: Mathematical structure: vector space GF(2)^{2^n} with pointwise ops.
 *
 * Memory model:
 *   For n inputs: table_size = 2^n entries.
 *   Each entry is an int (0 or 1), stored in values[].
 *   Total size: 2^n * sizeof(int) bytes.
 *
 *   For n=10: 1024 ints = 4 KB
 *   For n=15: 32768 ints = 128 KB
 *   For n=20: 1,048,576 ints = 4 MB (upper practical limit)
 *   For n=25: 33,554,432 ints = 128 MB (feasible but heavy)
 *
 *   Use CompressedTruthTable (packed bits) for n > 20.
 *
 * Algorithm complexity:
 *   tt_create:    O(2^n) time, O(2^n) memory
 *   tt_random:    O(2^n) time
 *   tt_evaluate:  O(n) time
 *   tt_equal:     O(2^n) time
 *   tt_hamming:   O(2^n) time
 *   tt_is_monotone: O(3^n) time (naive)
 *
 * References:
 *   - Shannon (1949): truth tables as canonical representation
 *   - Wegener (1987): "The Complexity of Boolean Functions", Chapter 1
 *   - Jukna (2012): "Boolean Function Complexity", Section 1.1
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include <time.h>
#include "natural_core.h"

/* ========================================================================
 * TruthTable: Creation and Destruction
 * ======================================================================== */

TruthTable *tt_create(int n) {
    if (n < 0) return NULL;
    TruthTable *t = (TruthTable *)malloc(sizeof(TruthTable));
    if (!t) return NULL;
    t->n_inputs = n;
    t->table_size = 1LL << n;
    t->values = (int *)calloc((size_t)t->table_size, sizeof(int));
    if (!t->values) {
        free(t);
        return NULL;
    }
    t->name[0] = '\0';
    return t;
}

TruthTable *tt_create_named(int n, const char *name) {
    TruthTable *t = tt_create(n);
    if (t && name) {
        strncpy(t->name, name, 63);
        t->name[63] = '\0';
    }
    return t;
}

void tt_free(TruthTable *t) {
    if (!t) return;
    free(t->values);
    free(t);
}

TruthTable *tt_copy(const TruthTable *t) {
    if (!t) return NULL;
    TruthTable *copy = tt_create(t->n_inputs);
    if (!copy) return NULL;
    memcpy(copy->values, t->values,
           (size_t)t->table_size * sizeof(int));
    strncpy(copy->name, t->name, 63);
    copy->name[63] = '\0';
    return copy;
}

/* ========================================================================
 * TruthTable: Accessors
 * ======================================================================== */

int tt_get(const TruthTable *t, long long idx) {
    if (!t || idx < 0 || idx >= t->table_size) return 0;
    return t->values[idx];
}

void tt_set(TruthTable *t, long long idx, int val) {
    if (!t || idx < 0 || idx >= t->table_size) return;
    t->values[idx] = (val != 0) ? 1 : 0;
}

int tt_evaluate(const TruthTable *t, const int *input) {
    if (!t || !input) return 0;
    long long idx = 0;
    for (int i = t->n_inputs - 1; i >= 0; i--) {
        idx = (idx << 1) | (input[i] ? 1 : 0);
    }
    return tt_get(t, idx);
}

int tt_evaluate_int(const TruthTable *t, long long encoding) {
    if (!t) return 0;
    return tt_get(t, encoding & (t->table_size - 1));
}

/* ========================================================================
 * TruthTable: Random Generation
 *
 * Uniform distribution over all 2^{2^n} Boolean functions.
 * Each of the 2^n entries is independently 0 or 1 with prob 1/2.
 * ======================================================================== */

TruthTable *tt_random(int n) {
    TruthTable *t = tt_create(n);
    if (!t) return NULL;
    for (long long i = 0; i < t->table_size; i++) {
        t->values[i] = rand() & 1;
    }
    snprintf(t->name, 63, "random_fn[%d]", n);
    return t;
}

TruthTable *tt_random_seeded(int n, unsigned int seed) {
    srand(seed);
    return tt_random(n);
}

/* ========================================================================
 * TruthTable: Comparison
 * ======================================================================== */

int tt_equal(const TruthTable *a, const TruthTable *b) {
    if (!a && !b) return 1;
    if (!a || !b) return 0;
    if (a->n_inputs != b->n_inputs) return 0;
    for (long long i = 0; i < a->table_size; i++) {
        if (a->values[i] != b->values[i]) return 0;
    }
    return 1;
}

long long tt_hamming_distance(const TruthTable *a, const TruthTable *b) {
    if (!a || !b || a->n_inputs != b->n_inputs) return -1;
    long long dist = 0;
    for (long long i = 0; i < a->table_size; i++) {
        if (a->values[i] != b->values[i]) dist++;
    }
    return dist;
}

int tt_is_constant(const TruthTable *t) {
    if (!t || t->table_size == 0) return 1;
    int first = t->values[0];
    for (long long i = 1; i < t->table_size; i++) {
        if (t->values[i] != first) return 0;
    }
    return 1;
}

/*
 * tt_is_monotone: Check monotonicity of Boolean function.
 *
 * Definition: f is monotone if for all x, y in {0,1}^n:
 *   x <= y (bitwise) => f(x) <= f(y)
 *
 * Naive algorithm: O(3^n) by checking all pairs.
 *   For each x, check all y >= x (there are 2^{n-wt(x)} such y).
 *
 * More efficient: check only pairs differing in one bit (O(n*2^n)).
 *   If f is monotone, then f(x) = 0 => f(x|e_i) can be anything,
 *   but f(x) = 1 => f(x|e_i) must be 1.
 *   Actually the condition x<=y iff x&y=x.
 */
int tt_is_monotone(const TruthTable *t) {
    if (!t || t->n_inputs == 0) return 1;
    int n = t->n_inputs;
    long long sz = t->table_size;

    /* Check all pairs (x, y) where y = x | (1 << bit) for some bit.
     * If f is monotone, then for any x and any bit b where x_b = 0:
     *   f(x) <= f(x with bit b set to 1)
     * This is necessary and sufficient for monotonicity. */
    for (long long x = 0; x < sz; x++) {
        for (int b = 0; b < n; b++) {
            long long y = x | (1LL << b);
            if (y == x) continue;  /* bit already set */
            if (t->values[x] > t->values[y]) return 0;  /* violation */
        }
    }
    return 1;
}

/* ========================================================================
 * TruthTable: Boolean Operations (Pointwise)
 *
 * These implement the Boolean algebra on functions:
 *   (f NOT g)(x) = 1 - f(x)
 *   (f AND g)(x) = f(x) * g(x)
 *   (f OR g)(x)  = max(f(x), g(x))
 *   (f XOR g)(x) = f(x) XOR g(x)
 * ======================================================================== */

TruthTable *tt_not(const TruthTable *t) {
    if (!t) return NULL;
    TruthTable *r = tt_create(t->n_inputs);
    if (!r) return NULL;
    for (long long i = 0; i < t->table_size; i++) {
        r->values[i] = 1 - t->values[i];
    }
    snprintf(r->name, 63, "NOT(%s)", t->name);
    return r;
}

TruthTable *tt_and(const TruthTable *a, const TruthTable *b) {
    if (!a || !b || a->n_inputs != b->n_inputs) return NULL;
    TruthTable *r = tt_create(a->n_inputs);
    if (!r) return NULL;
    for (long long i = 0; i < a->table_size; i++) {
        r->values[i] = a->values[i] & b->values[i];
    }
    snprintf(r->name, 63, "(%s & %s)", a->name, b->name);
    return r;
}

TruthTable *tt_or(const TruthTable *a, const TruthTable *b) {
    if (!a || !b || a->n_inputs != b->n_inputs) return NULL;
    TruthTable *r = tt_create(a->n_inputs);
    if (!r) return NULL;
    for (long long i = 0; i < a->table_size; i++) {
        r->values[i] = a->values[i] | b->values[i];
    }
    snprintf(r->name, 63, "(%s | %s)", a->name, b->name);
    return r;
}

TruthTable *tt_xor(const TruthTable *a, const TruthTable *b) {
    if (!a || !b || a->n_inputs != b->n_inputs) return NULL;
    TruthTable *r = tt_create(a->n_inputs);
    if (!r) return NULL;
    for (long long i = 0; i < a->table_size; i++) {
        r->values[i] = a->values[i] ^ b->values[i];
    }
    snprintf(r->name, 63, "(%s XOR %s)", a->name, b->name);
    return r;
}

/* ========================================================================
 * TruthTable: Statistics
 * ======================================================================== */

long long tt_weight(const TruthTable *t) {
    if (!t) return 0;
    long long w = 0;
    for (long long i = 0; i < t->table_size; i++) {
        if (t->values[i]) w++;
    }
    return w;
}

double tt_fraction_ones(const TruthTable *t) {
    if (!t || t->table_size == 0) return 0.0;
    return (double)tt_weight(t) / (double)t->table_size;
}

double tt_count_functions(int n) {
    /* Total Boolean functions on n inputs: 2^{2^n}.
     * For n > 5, this overflows double; we cap and warn.
     * n=4: 65536, n=5: 2^32 ~ 4.3e9, n=6: 2^64 ~ 1.8e19,
     * n=7: 2^128 ~ 3.4e38, n=8: 2^256 ~ 1.2e77 (fits double),
     * n=9: 2^512 ~ 1.3e154, n=10: 2^1024 ~ overflow. */
    if (n < 0) return 0.0;
    if (n >= 10) return INFINITY;
    return pow(2.0, pow(2.0, (double)n));
}

/* ========================================================================
 * TruthTable: I/O
 * ======================================================================== */

void tt_print(const TruthTable *t, FILE *fp) {
    if (!t) { fprintf(fp, "NULL\n"); return; }
    fprintf(fp, "TruthTable: n=%d, size=%lld", t->n_inputs, t->table_size);
    if (t->name[0]) fprintf(fp, ", name=''%s''", t->name);
    fprintf(fp, "\n");

    if (t->n_inputs <= 4) {
        /* Print full truth table with input patterns */
        fprintf(fp, "  ");
        for (int i = t->n_inputs - 1; i >= 0; i--) fprintf(fp, "x%d", i);
        fprintf(fp, " | f\n");
        fprintf(fp, "  ");
        for (int i = 0; i < t->n_inputs; i++) fprintf(fp, "--");
        fprintf(fp, "+---\n");
        for (long long i = 0; i < t->table_size; i++) {
            fprintf(fp, "  ");
            for (int j = t->n_inputs - 1; j >= 0; j--)
                fprintf(fp, "%d ", (int)((i >> j) & 1));
            fprintf(fp, "| %d\n", t->values[i]);
        }
    } else {
        /* Summary for larger n */
        long long ones = tt_weight(t);
        fprintf(fp, "  Weight: %lld / %lld (%.2f%% ones)\n",
                ones, t->table_size,
                100.0 * (double)ones / (double)t->table_size);
        fprintf(fp, "  First 8 values: ");
        for (long long i = 0; i < 8 && i < t->table_size; i++)
            fprintf(fp, "%d", t->values[i]);
        fprintf(fp, "\n");
    }
}

void tt_print_brief(const TruthTable *t, FILE *fp) {
    if (!t) { fprintf(fp, "NULL\n"); return; }
    fprintf(fp, "[TT n=%d sz=%lld]", t->n_inputs, t->table_size);
    if (t->name[0]) fprintf(fp, " %s", t->name);
    fprintf(fp, "\n");
}

/* ========================================================================
 * CompressedTruthTable: Packed Bit Representation
 *
 * For n > 20, storing 2^n ints becomes impractical.
 * Packed representation: each uint64_t holds 64 truth table entries.
 * Memory: ceil(2^n / 64) * 8 bytes.
 *
 * n=20: 2^20/64 = 16384 uint64_t = 128 KB (same as int version)
 * n=25: 2^25/64 = 524288 uint64_t = 4 MB (vs 128 MB for int)
 * n=30: 2^30/64 = 16,777,216 uint64_t = 128 MB
 * ======================================================================== */

CompressedTruthTable *ctt_create(int n) {
    if (n < 0) return NULL;
    CompressedTruthTable *t = (CompressedTruthTable *)malloc(
        sizeof(CompressedTruthTable));
    if (!t) return NULL;
    t->n_inputs = n;
    t->table_size = 1LL << n;
    long long n_words = (t->table_size + 63) / 64;
    t->bits = (uint64_t *)calloc((size_t)n_words, sizeof(uint64_t));
    if (!t->bits) {
        free(t);
        return NULL;
    }
    return t;
}

void ctt_free(CompressedTruthTable *t) {
    if (!t) return;
    free(t->bits);
    free(t);
}

int ctt_get(const CompressedTruthTable *t, long long idx) {
    if (!t || idx < 0 || idx >= t->table_size) return 0;
    long long word_idx = idx / 64;
    int bit_idx = (int)(idx % 64);
    return (int)((t->bits[word_idx] >> bit_idx) & 1ULL);
}

void ctt_set(CompressedTruthTable *t, long long idx, int val) {
    if (!t || idx < 0 || idx >= t->table_size) return;
    long long word_idx = idx / 64;
    int bit_idx = (int)(idx % 64);
    if (val) {
        t->bits[word_idx] |= (1ULL << bit_idx);
    } else {
        t->bits[word_idx] &= ~(1ULL << bit_idx);
    }
}

CompressedTruthTable *ctt_random(int n) {
    CompressedTruthTable *t = ctt_create(n);
    if (!t) return NULL;
    long long n_words = (t->table_size + 63) / 64;
    for (long long i = 0; i < n_words; i++) {
        /* Generate 64 random bits efficiently */
        t->bits[i] = ((uint64_t)rand() & 0xFFFFULL)
                   | (((uint64_t)rand() & 0xFFFFULL) << 16)
                   | (((uint64_t)rand() & 0xFFFFULL) << 32)
                   | (((uint64_t)rand() & 0xFFFFULL) << 48);
    }
    return t;
}
