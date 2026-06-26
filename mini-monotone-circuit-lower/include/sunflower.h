/* sunflower.h -- Sunflower Lemma (Erdos-Rado 1960)
 *
 * A sunflower with p petals is a collection of p sets S_1,...,S_p
 * such that for all i≠j, S_i ∩ S_j = K for some fixed set K
 * (the "core" of the sunflower). The petals S_i \ K are pairwise
 * disjoint.
 *
 * Theorem (Erdos-Rado, 1960):
 *   Any family of more than (p-1)^k * k! sets, each of size k,
 *   contains a sunflower with p petals.
 *
 * This lemma is the KEY to Razborov's monotone circuit lower bounds.
 * In the method of approximations, sunflowers allow compression of
 * approximators at AND gates, keeping their size manageable.
 *
 * References:
 *   Erdos & Rado (1960), "Intersection theorems for systems of sets"
 *   Razborov (1985), "Lower bounds for the monotone complexity"
 *   Jukna (2012), "Boolean Function Complexity", Ch. 10
 *   Alweiss, Lovett, Wu, Zhang (2020), "Improved bounds for the
 *     sunflower lemma"
 */
#ifndef SUNFLOWER_H
#define SUNFLOWER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>

/* ============================================================
 * L1 Definitions: Set and Sunflower
 * ============================================================ */

/* A set represented as a 64-bit bitmask.
 * Universe size ≤ 64. For larger universes, see SetFamily below. */
typedef uint64_t SetMask;

/* A sunflower: p petals + core */
typedef struct {
    int      p;           /* Number of petals */
    int      k;           /* Size of each set (petal ∪ core) */
    SetMask  core;        /* Common intersection K */
    SetMask *petals;      /* p disjoint sets (petals = sets \ core) */
    SetMask *full_sets;   /* p full sets (petal ∪ core) */
} Sunflower;

/* ============================================================
 * L2: Set Operations (bitmask-based)
 * ============================================================ */

/* Population count (number of 1-bits) */
int setmask_popcount(SetMask s);

/* Set size (alias for popcount) */
int setmask_size(SetMask s);

/* Test if two sets are disjoint */
int setmask_disjoint(SetMask a, SetMask b);

/* Test if a is a subset of b */
int setmask_subset(SetMask a, SetMask b);

/* Create a set from a list of elements */
SetMask setmask_from_elements(const int *elements, int n_elements);

/* Print a set as {e1, e2, ...} */
void setmask_print(SetMask s);

/* ============================================================
 * L2: Sunflower Detection
 * ============================================================ */

/* Check if p sets form a sunflower. If so, sets *core = K.
 * Returns 1 if sunflower, 0 otherwise. */
int sunflower_check(const SetMask *sets, int p, int k, SetMask *core);

/* Create a sunflower from p sets and core */
Sunflower* sunflower_create(int p, int k, SetMask core, const SetMask *sets);

/* Free a sunflower */
void sunflower_free(Sunflower *sf);

/* Print a sunflower */
void sunflower_print(const Sunflower *sf);

/* ============================================================
 * L3 Mathematical Structures: Set Family Operations
 * ============================================================ */

/* Representation for families with universe > 64.
 * Each set is an array of int elements. */
typedef struct {
    int   n;           /* Universe size {0,..,n-1} */
    int   k;           /* Each set has size k */
    int   num_sets;    /* Number of sets */
    int   capacity;
    int **sets;        /* sets[i][0..k-1] = elements */
} KSetFamily;

/* Create a k-set family */
KSetFamily* ksetfamily_create(int n, int k, int capacity);

/* Add a set (copies elements) */
void ksetfamily_add(KSetFamily *kf, const int *elements);

/* Add a random k-set */
void ksetfamily_add_random(KSetFamily *kf);

/* Free */
void ksetfamily_free(KSetFamily *kf);

/* ============================================================
 * L4: Erdos-Rado Sunflower Lemma
 * ============================================================ */

/* Compute the Erdos-Rado bound: (p-1)^k * k!
 * Any family larger than this MUST contain a sunflower. */
double sunflower_bound_erdos_rado(int p, int k);

/* Compute the improved ALWZ (2020) bound:
 * (log k)^(k) * (p * log log k)^O(k)
 * This is asymptotically better but harder to compute exactly. */
double sunflower_bound_alwz(int p, int k);

/* ============================================================
 * L5: Sunflower Search Algorithms
 * ============================================================ */

/* Naive brute-force search for sunflower in k-set family.
 * Checks all (num_sets choose p) combinations.
 * If found, fills *sf and returns 1.
 * Time: O(C(num_sets, p) * p * k) */
int sunflower_find_brute(const KSetFamily *kf, int p, Sunflower *sf);

/* Greedy search: sort by core size, check most promising first.
 * Not guaranteed to find a sunflower even if one exists,
 * but much faster in practice. */
int sunflower_find_greedy(const KSetFamily *kf, int p, Sunflower *sf);

/* Sunflower with specific core: search for p petals with given core K.
 * Returns 1 if found, with petals filled in sf. */
int sunflower_find_with_core(const KSetFamily *kf, int p,
                              const int *core, int core_size,
                              Sunflower *sf);

/* ============================================================
 * L5: Sunflower Compression (for Approximators)
 * ============================================================ */

/* The key operation in Razborov's method:
 * Given a family of sets F, if |F| > bound(p,k), find a sunflower
 * and replace its p petals with the core (reducing the family size).
 * 
 * This function: given F, try to find sunflowers and compress.
 * Returns a new (potentially smaller) family.
 * The caller must free the result. */
KSetFamily* sunflower_compress(const KSetFamily *kf, int p);

/* ============================================================
 * L6: Sunflower Construction & Counter-Examples
 * ============================================================ */

/* Construct a maximal sunflower-free family of k-sets.
 * This demonstrates that the Erdos-Rado bound is tight
 * up to the k! factor. Uses the "block" construction:
 * partition [n] into k blocks of size about n/k each.
 * Take all sets with exactly one element from each block.
 * Size = (n/k)^k. No sunflower with p petals exists
 * if (n/k) < p. */
KSetFamily* sunflower_free_family_construct(int n, int k);

/* Construct an explicit sunflower for demonstration */
Sunflower* sunflower_construct_example(int p, int k);

#endif /* SUNFLOWER_H */
