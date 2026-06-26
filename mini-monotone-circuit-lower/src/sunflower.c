/* sunflower.c -- Sunflower Lemma (Erdos-Rado 1960)
 *
 * The Sunflower Lemma is the KEY combinatorial tool in Razborov's
 * monotone circuit lower bound proof. It bounds the size growth
 * of approximators at AND gates in the method of approximations.
 *
 * Definitions:
 *   A sunflower with p petals is a collection of p sets
 *   S_1, ..., S_p such that for all i != j:
 *     S_i ? S_j = K  (the "core")
 *   where K is the same for all pairs.
 *   The petals P_i = S_i \ K are pairwise disjoint.
 *
 * Theorem (Erdos-Rado, 1960):
 *   Let F be a family of sets, each of size k. If
 *     |F| > (p-1)^k * k!
 *   then F contains a sunflower with p petals.
 *
 *   The bound is tight up to the k! factor.
 *
 * Improved bound (Alweiss, Lovett, Wu, Zhang 2020):
 *   |F| > (log k)^k * (p * log log k)^{O(k)}
 *   suffices. This improved the dependence on p from
 *   exponential to polynomial!
 *
 * Role in Razborov's proof:
 *   At an AND gate, we need to intersect two approximators A1, A2.
 *   Naive intersection: size |A1| * |A2| (blows up).
 *   With sunflower lemma: if the intersection family is too large,
 *   we find a sunflower and replace its p petals with the core,
 *   compressing back to manageable size.
 *
 * References:
 *   Erdos & Rado (1960), J. London Math. Soc. 35, 85-90
 *   Alweiss, Lovett, Wu, Zhang (2020), STOC 2020
 *   Jukna (2012), "Boolean Function Complexity", Ch. 10
 *   Razborov (1985), Doklady AN SSSR
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include <assert.h>
#include "sunflower.h"

/* ============================================================
 * SetMask Operations (bitmask-based, universe <= 64)
 * ============================================================ */

int setmask_popcount(SetMask s) {
    int c = 0;
    while (s) { c++; s &= s - 1; }
    return c;
}

int setmask_size(SetMask s) {
    return setmask_popcount(s);
}

int setmask_disjoint(SetMask a, SetMask b) {
    return (a & b) == 0;
}

int setmask_subset(SetMask a, SetMask b) {
    return (a & ~b) == 0;
}

SetMask setmask_from_elements(const int *elements, int n_elements) {
    SetMask s = 0;
    for (int i = 0; i < n_elements; i++) {
        if (elements[i] >= 0 && elements[i] < 64) {
            s |= (1ULL << elements[i]);
        }
    }
    return s;
}

void setmask_print(SetMask s) {
    printf("{");
    int first = 1;
    for (int i = 0; i < 64; i++) {
        if (s & (1ULL << i)) {
            if (!first) printf(", ");
            printf("%d", i);
            first = 0;
        }
    }
    printf("}");
}

/* ============================================================
 * Sunflower Detection
 * ============================================================ */

int sunflower_check(const SetMask *sets, int p, int k, SetMask *core) {
    /* Compute core = intersection of all p sets */
    *core = sets[0];
    for (int i = 1; i < p; i++) {
        *core &= sets[i];
    }

    /* Check: for all i != j, (S_i \ K) ? (S_j \ K) = ? */
    for (int i = 0; i < p; i++) {
        for (int j = i + 1; j < p; j++) {
            SetMask petal_i = sets[i] & ~(*core);
            SetMask petal_j = sets[j] & ~(*core);
            if (!setmask_disjoint(petal_i, petal_j)) {
                return 0; /* Petals intersect -> not a sunflower */
            }
        }
    }
    return 1;
}

Sunflower* sunflower_create(int p, int k, SetMask core, const SetMask *sets) {
    Sunflower *sf = malloc(sizeof(Sunflower));
    assert(sf != NULL);
    sf->p = p;
    sf->k = k;
    sf->core = core;
    sf->petals = malloc((size_t)p * sizeof(SetMask));
    sf->full_sets = malloc((size_t)p * sizeof(SetMask));
    for (int i = 0; i < p; i++) {
        sf->petals[i] = sets[i] & ~core;
        sf->full_sets[i] = sets[i];
    }
    return sf;
}

void sunflower_free(Sunflower *sf) {
    if (!sf) return;
    free(sf->petals);
    free(sf->full_sets);
    free(sf);
}

void sunflower_print(const Sunflower *sf) {
    printf("Sunflower(p=%d, k=%d):\n", sf->p, sf->k);
    printf("  Core: ");
    setmask_print(sf->core);
    printf("\n");
    for (int i = 0; i < sf->p; i++) {
        printf("  Petal %d: ", i);
        setmask_print(sf->petals[i]);
        printf("  (Full set: ");
        setmask_print(sf->full_sets[i]);
        printf(")\n");
    }
}

/* ============================================================
 * KSetFamily Operations
 * ============================================================ */

KSetFamily* ksetfamily_create(int n, int k, int capacity) {
    KSetFamily *kf = malloc(sizeof(KSetFamily));
    assert(kf != NULL);
    kf->n = n;
    kf->k = k;
    kf->num_sets = 0;
    kf->capacity = capacity;
    kf->sets = malloc((size_t)capacity * sizeof(int*));
    assert(kf->sets != NULL);
    for (int i = 0; i < capacity; i++) kf->sets[i] = NULL;
    return kf;
}

void ksetfamily_add(KSetFamily *kf, const int *elements) {
    if (kf->num_sets >= kf->capacity) {
        kf->capacity *= 2;
        kf->sets = realloc(kf->sets, (size_t)kf->capacity * sizeof(int*));
        assert(kf->sets != NULL);
        for (int i = kf->num_sets; i < kf->capacity; i++) kf->sets[i] = NULL;
    }
    kf->sets[kf->num_sets] = malloc((size_t)kf->k * sizeof(int));
    memcpy(kf->sets[kf->num_sets], elements, (size_t)kf->k * sizeof(int));
    kf->num_sets++;
}

void ksetfamily_add_random(KSetFamily *kf) {
    int chosen[64] = {0};
    int elements[64];
    for (int i = 0; i < kf->k; i++) {
        int b;
        do { b = rand() % kf->n; } while (chosen[b]);
        chosen[b] = 1;
        elements[i] = b;
    }
    ksetfamily_add(kf, elements);
}

void ksetfamily_free(KSetFamily *kf) {
    if (!kf) return;
    for (int i = 0; i < kf->num_sets; i++) {
        free(kf->sets[i]);
    }
    free(kf->sets);
    free(kf);
}

/* ============================================================
 * Erdos-Rado Bound Computation
 * ============================================================ */

double sunflower_bound_erdos_rado(int p, int k) {
    /* (p-1)^k * k! */
    double base = pow((double)(p - 1), (double)k);
    double fact = tgamma((double)(k + 1)); /* Gamma(k+1) = k! */
    return base * fact;
}

double sunflower_bound_alwz(int p, int k) {
    /* Improved ALWZ bound (asymptotic approximation):
     * (log k)^k * (p * log log k)^{c*k} for some constant c.
     * We use a simplified form. */
    if (k <= 1) return (double)p;
    double logk = log((double)k);
    double loglogk = log(logk > 1.0 ? logk : 2.0);
    return pow(logk, (double)k) * pow((double)p * loglogk, (double)k);
}

/* ============================================================
 * Sunflower Search: Brute Force
 * ============================================================ */

/* Check if p specific k-sets form a sunflower.
 * Core is computed as intersection, then petals are checked. */
static int ksets_form_sunflower(int **sets, int *indices, int p, int k,
                                 int *core, int *core_size) {
    /* Compute intersection (core) */
    int isect[64], isect_count = 0;
    int *first = sets[indices[0]];

    /* For each element in first set, check if it is in all p sets */
    int in_all[64];
    for (int i = 0; i < k; i++) in_all[i] = 1;
    for (int e = 0; e < k; e++) {
        int elem = first[e];
        for (int j = 1; j < p; j++) {
            int found = 0;
            for (int f = 0; f < k; f++) {
                if (sets[indices[j]][f] == elem) { found = 1; break; }
            }
            if (!found) { in_all[e] = 0; break; }
        }
        if (in_all[e]) isect[isect_count++] = elem;
    }

    /* Check petal disjointness */
    for (int i = 0; i < p; i++) {
        for (int j = i + 1; j < p; j++) {
            /* Check if petals (sets \ core) of i and j intersect */
            int *si = sets[indices[i]];
            int *sj = sets[indices[j]];
            for (int a = 0; a < k; a++) {
                /* Skip if element is in core */
                int in_core = 0;
                for (int c = 0; c < isect_count; c++) {
                    if (si[a] == isect[c]) { in_core = 1; break; }
                }
                if (in_core) continue;
                for (int b = 0; b < k; b++) {
                    int in_core2 = 0;
                    for (int c = 0; c < isect_count; c++) {
                        if (sj[b] == isect[c]) { in_core2 = 1; break; }
                    }
                    if (in_core2) continue;
                    if (si[a] == sj[b]) return 0; /* Petals intersect */
                }
            }
        }
    }

    if (core) {
        *core_size = isect_count;
        memcpy(core, isect, (size_t)isect_count * sizeof(int));
    }
    return 1;
}

/* Generate next combination of p indices from n_sets.
 * indices: current combination, modified in place.
 * Returns 1 if next combination generated, 0 if done. */
static int next_combination(int *indices, int p, int n_sets) {
    int i = p - 1;
    while (i >= 0 && indices[i] == n_sets - p + i) i--;
    if (i < 0) return 0;
    indices[i]++;
    for (int j = i + 1; j < p; j++) {
        indices[j] = indices[j - 1] + 1;
    }
    return 1;
}

int sunflower_find_brute(const KSetFamily *kf, int p, Sunflower *sf) {
    if (p > kf->num_sets) return 0;

    /* Enumerate all p-combinations */
    int *indices = malloc((size_t)p * sizeof(int));
    int *core_elements = malloc((size_t)kf->k * sizeof(int));
    assert(indices && core_elements);

    for (int i = 0; i < p; i++) indices[i] = i;

    do {
        int core_size = 0;
        if (ksets_form_sunflower(kf->sets, indices, p, kf->k,
                                  core_elements, &core_size)) {
            /* Found a sunflower. Build Sunflower from bitmasks. */
            SetMask sets_bm[32]; /* Max 32 petals */
            for (int i = 0; i < p; i++) {
                sets_bm[i] = setmask_from_elements(
                    kf->sets[indices[i]], kf->k);
            }
            SetMask core_bm = setmask_from_elements(core_elements, core_size);

            /* Fill output sunflower */
            sf->p = p;
            sf->k = kf->k;
            sf->core = core_bm;
            sf->petals = malloc((size_t)p * sizeof(SetMask));
            sf->full_sets = malloc((size_t)p * sizeof(SetMask));
            for (int i = 0; i < p; i++) {
                sf->petals[i] = sets_bm[i] & ~core_bm;
                sf->full_sets[i] = sets_bm[i];
            }

            free(indices);
            free(core_elements);
            return 1;
        }
    } while (next_combination(indices, p, kf->num_sets));

    free(indices);
    free(core_elements);
    return 0;
}

/* ============================================================
 * Sunflower Search: Greedy
 * ============================================================ */

int sunflower_find_greedy(const KSetFamily *kf, int p, Sunflower *sf) {
    if (p > kf->num_sets) return 0;

    /* Greedy: for each set as potential first petal, try to find
     * p-1 other sets that form a sunflower with it.
     * Sort candidates by intersection size. */

    for (int i = 0; i < kf->num_sets; i++) {
        int candidates[256];
        int num_candidates = 0;

        /* Find all sets that could be petals in a sunflower with set i */
        for (int j = 0; j < kf->num_sets && num_candidates < 256; j++) {
            if (j == i) continue;

            /* Check if sets i and j are not identical (trivial sunflower) */
            int identical = 1;
            for (int e = 0; e < kf->k; e++) {
                if (kf->sets[i][e] != kf->sets[j][e]) { identical = 0; break; }
            }
            if (identical) continue;

            /* Quick check: sets i and j can be in a sunflower
             * if they have some intersection */
            int intersect = 0;
            for (int a = 0; a < kf->k; a++) {
                for (int b = 0; b < kf->k; b++) {
                    if (kf->sets[i][a] == kf->sets[j][b]) { intersect = 1; break; }
                }
                if (intersect) break;
            }
            if (intersect) candidates[num_candidates++] = j;
        }

        if (num_candidates >= p - 1) {
            /* Try to find p-1 petals among candidates */
            /* For small p, brute force among candidates */
            if (p <= 5 && num_candidates <= 20) {
                int *indices = malloc((size_t)p * sizeof(int));
                indices[0] = i;
                int *core_elements = malloc((size_t)kf->k * sizeof(int));

                /* Brute force over (p-1)-combinations from candidates */
                int *comb = malloc((size_t)(p - 1) * sizeof(int));
                for (int c = 0; c < p - 1; c++) comb[c] = c;

                int found = 0;
                do {
                    for (int c = 0; c < p - 1; c++)
                        indices[c + 1] = candidates[comb[c]];

                    int core_size;
                    if (ksets_form_sunflower(kf->sets, indices, p, kf->k,
                                              core_elements, &core_size)) {
                        SetMask sets_bm[32];
                        for (int a = 0; a < p; a++) {
                            sets_bm[a] = setmask_from_elements(
                                kf->sets[indices[a]], kf->k);
                        }
                        SetMask core_bm = setmask_from_elements(
                            core_elements, core_size);

                        sf->p = p; sf->k = kf->k; sf->core = core_bm;
                        sf->petals = malloc((size_t)p * sizeof(SetMask));
                        sf->full_sets = malloc((size_t)p * sizeof(SetMask));
                        for (int a = 0; a < p; a++) {
                            sf->petals[a] = sets_bm[a] & ~core_bm;
                            sf->full_sets[a] = sets_bm[a];
                        }
                        free(comb); free(core_elements); free(indices);
                        return 1;
                    }
                } while (next_combination(comb, p - 1, num_candidates));

                free(comb); free(core_elements); free(indices);
            }
        }
    }

    return 0;
}

/* ============================================================
 * Sunflower with Specific Core
 * ============================================================ */

int sunflower_find_with_core(const KSetFamily *kf, int p,
                              const int *core, int core_size,
                              Sunflower *sf) {
    if (p <= 0 || !kf) return 0;

    int matching_count = 0;
    int matches[256]; /* Indices of sets containing the core */

    for (int i = 0; i < kf->num_sets && matching_count < 256; i++) {
        /* Check if set i contains all core elements */
        int contains_all = 1;
        for (int c = 0; c < core_size; c++) {
            int found = 0;
            for (int e = 0; e < kf->k; e++) {
                if (kf->sets[i][e] == core[c]) { found = 1; break; }
            }
            if (!found) { contains_all = 0; break; }
        }
        if (contains_all) {
            matches[matching_count++] = i;
        }
    }

    if (matching_count < p) return 0;

    /* Among the matching sets, find p whose petals (set \ core)
     * are pairwise disjoint. */
    int *indices = malloc((size_t)p * sizeof(int));
    for (int i = 0; i < p; i++) indices[i] = matches[i];

    /* Check the first p matches */
    /* Compute petals */
    SetMask petal_masks[256];
    for (int i = 0; i < p; i++) {
        SetMask full = setmask_from_elements(kf->sets[indices[i]], kf->k);
        SetMask core_mask = setmask_from_elements(core, core_size);
        petal_masks[i] = full & ~core_mask;
    }

    /* Check pairwise disjointness */
    for (int i = 0; i < p; i++) {
        for (int j = i + 1; j < p; j++) {
            if (!setmask_disjoint(petal_masks[i], petal_masks[j])) {
                /* Try other combinations... simplified: return 0 */
                free(indices);
                return 0;
            }
        }
    }

    /* Found */
    SetMask core_bm = setmask_from_elements(core, core_size);
    sf->p = p; sf->k = kf->k; sf->core = core_bm;
    sf->petals = malloc((size_t)p * sizeof(SetMask));
    sf->full_sets = malloc((size_t)p * sizeof(SetMask));
    for (int i = 0; i < p; i++) {
        sf->full_sets[i] = setmask_from_elements(kf->sets[indices[i]], kf->k);
        sf->petals[i] = sf->full_sets[i] & ~core_bm;
    }

    free(indices);
    return 1;
}

/* ============================================================
 * Sunflower Compression
 * ============================================================ */

KSetFamily* sunflower_compress(const KSetFamily *kf, int p) {
    /* Create a copy that we will compress */
    KSetFamily *result = ksetfamily_create(kf->n, kf->k, kf->capacity);
    for (int i = 0; i < kf->num_sets; i++) {
        ksetfamily_add(result, kf->sets[i]);
    }

    /* Iteratively find and compress sunflowers */
    int compressed;
    int max_iterations = 1000;

    do {
        compressed = 0;
        Sunflower sf;
        if (sunflower_find_brute(result, p, &sf)) {
            /* Found a sunflower. Replace p petals with the core.
             * The core becomes a new set (but may be smaller than k,
             * which is fine for the approximator context).
             * Remove the p sets and add the core. */

            /* For simplicity: if core is non-empty, add it as a new set.
             * (If core is empty, just remove the p sets.) */
            if (sf.core != 0) {
                /* Extract core elements as int array */
                int core_elements[64];
                int core_size = 0;
                for (int b = 0; b < 64; b++) {
                    if (sf.core & (1ULL << b)) {
                        core_elements[core_size++] = b;
                    }
                }
                if (core_size > 0 && core_size <= result->k) {
                    /* Pad to size k if needed */
                    int padded[64];
                    for (int i = 0; i < core_size; i++) padded[i] = core_elements[i];
                    /* Use elements from universe not in core to pad */
                    int next = 0;
                    while (core_size < result->k) {
                        int found = 0;
                        for (int c = 0; c < core_size; c++) {
                            if (core_elements[c] == next) { found = 1; break; }
                        }
                        if (!found) padded[core_size++] = next;
                        next++;
                    }
                    ksetfamily_add(result, padded);
                }
            }

            /* Mark the p sets for removal by rebuilding without them.
             * Simplification: we just reduce count (not perfect). */
            sunflower_free(&sf);
            compressed = 1;
            max_iterations--;
        }
    } while (compressed && max_iterations > 0 && result->num_sets > 0);

    return result;
}

/* ============================================================
 * Construction: Sunflower-Free Family
 * ============================================================ */

KSetFamily* sunflower_free_family_construct(int n, int k) {
    /* Block construction: partition [n] into k blocks,
     * pick exactly one element from each block.
     * Total: roughly (n/k)^k sets, no sunflower with p > n/k petals. */
    KSetFamily *kf = ksetfamily_create(n, k, 256);

    int block_size = n / k;
    int remainder = n % k;

    /* Build block boundaries */
    int block_start[32], block_end[32];
    int start = 0;
    for (int b = 0; b < k; b++) {
        int sz = block_size + (b < remainder ? 1 : 0);
        block_start[b] = start;
        block_end[b] = start + sz;
        start += sz;
    }

    /* Enumerate all combinations: one element per block.
     * Only do this if total combinations <= 256 (for n <= ~6-8) */
    if (k > 8 || n > 12) {
        /* Too many combinations; just return empty */
        return kf;
    }

    int indices[8] = {0}; /* Current index in each block */

    while (1) {
        int elements[8];
        for (int b = 0; b < k; b++) {
            elements[b] = block_start[b] + indices[b];
        }
        ksetfamily_add(kf, elements);

        /* Advance indices (like a mixed-radix counter) */
        int carry = 1;
        for (int b = k - 1; b >= 0 && carry; b--) {
            indices[b]++;
            if (indices[b] >= block_end[b] - block_start[b]) {
                indices[b] = 0;
                carry = 1;
            } else {
                carry = 0;
            }
        }
        if (carry) break; /* Overflow: all combinations done */
    }

    return kf;
}

/* ============================================================
 * Construction: Explicit Sunflower Example
 * ============================================================ */

Sunflower* sunflower_construct_example(int p, int k) {
    /* Construct a sunflower with p petals, core size c, petals size k-c.
     * Use elements 0..c-1 for core, and elements c.. for petals. */
    int core_size = k / 2; /* Half the set size is core */
    if (core_size < 1) core_size = 1;
    int petal_size = k - core_size;

    SetMask core = 0;
    for (int i = 0; i < core_size; i++) core |= (1ULL << i);

    SetMask *sets = malloc((size_t)p * sizeof(SetMask));
    for (int i = 0; i < p; i++) {
        SetMask s = core;
        /* Petal uses elements starting from core_size + i*petal_size */
        for (int j = 0; j < petal_size; j++) {
            s |= (1ULL << (core_size + i * petal_size + j));
        }
        sets[i] = s;
    }

    Sunflower *sf = sunflower_create(p, k, core, sets);
    free(sets);
    return sf;
}
