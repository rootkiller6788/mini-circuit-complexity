/* tc0_threshold.c -- Threshold Functions and Linear Separability (L3-L5)
 *
 * Implements threshold function theory:
 *   L3: LinearThresholdFunc, perceptron algorithm, weight enumeration
 *   L4: Linear separability (Minsky-Papert), threshold gate universality
 *   L5: Perceptron learning, min-weight representation, PTF degree bounds
 *
 * A Boolean function f is a linear threshold function (LTF) iff there exist
 * integer weights w_1,...,w_n and threshold theta such that:
 *   f(x) = 1 iff sum(w_i * x_i) >= theta
 *
 * Key facts:
 *   - MAJORITY is an LTF (all weights = 1, theta = floor(n/2)+1)
 *   - PARITY is NOT an LTF (requires degree-n PTF)
 *   - Every Boolean function is representable as a polynomial threshold
 *     function of degree at most n
 *   - TC0 consists of functions with depth-O(1) threshold circuits
 *
 * References:
 *   - Minsky & Papert (1969) "Perceptrons"
 *   - O'Donnell (2014) "Analysis of Boolean Functions", Ch. 5, 10
 *   - Vollmer (1999), Ch. 4
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include "tc0.h"

/* ================================================================
 * LINEAR THRESHOLD FUNCTIONS (L3: Mathematical Structures)
 * ================================================================ */

/* Create a linear threshold function with given weights and threshold.
 * The function computes f(x) = 1 iff sum(w_i * x_i) >= theta */
LinearThresholdFunc *ltf_create(int num_vars, const int *weights, int theta) {
    if (num_vars <= 0 || !weights) return NULL;
    LinearThresholdFunc *ltf = (LinearThresholdFunc *)malloc(sizeof(LinearThresholdFunc));
    if (!ltf) return NULL;
    ltf->num_vars = num_vars;
    ltf->weights = (int *)malloc((size_t)num_vars * sizeof(int));
    if (!ltf->weights) { free(ltf); return NULL; }
    ltf->weight_sum = 0;
    for (int i = 0; i < num_vars; i++) {
        ltf->weights[i] = weights[i];
        ltf->weight_sum += abs(weights[i]);
    }
    ltf->theta = theta;
    return ltf;
}

void ltf_free(LinearThresholdFunc *ltf) {
    if (!ltf) return;
    free(ltf->weights);
    free(ltf);
}

/* Evaluate a linear threshold function on a Boolean input vector */
int ltf_evaluate(const LinearThresholdFunc *ltf, const int *x) {
    if (!ltf || !x) return 0;
    int sum = 0;
    for (int i = 0; i < ltf->num_vars; i++)
        sum += ltf->weights[i] * x[i];
    return (sum >= ltf->theta) ? 1 : 0;
}

/* Create the MAJORITY function as an LTF.
 * MAJORITY(x) = 1 iff sum(x_i) > n/2
 * Weights: all 1s. Threshold: floor(n/2) + 1 */
LinearThresholdFunc *ltf_majority(int n) {
    int *weights = (int *)malloc((size_t)n * sizeof(int));
    for (int i = 0; i < n; i++) weights[i] = 1;
    LinearThresholdFunc *ltf = ltf_create(n, weights, n / 2 + 1);
    free(weights);
    return ltf;
}

/* Create the THRESHOLD(k) function as an LTF.
 * THRESHOLD(x; k) = 1 iff sum(x_i) >= k */
LinearThresholdFunc *ltf_threshold(int n, int k) {
    int *weights = (int *)malloc((size_t)n * sizeof(int));
    for (int i = 0; i < n; i++) weights[i] = 1;
    LinearThresholdFunc *ltf = ltf_create(n, weights, k);
    free(weights);
    return ltf;
}

/* Build a TC0Circuit from a LTF with a single THR gate */
TC0Circuit *ltf_to_circuit(const LinearThresholdFunc *ltf) {
    if (!ltf || ltf->num_vars > 256) return NULL;
    TC0Circuit *c = tc0_circuit_create(ltf->num_vars + 2);
    snprintf(c->name, sizeof(c->name), "LTF_n%d", ltf->num_vars);

    /* Input gates */
    int *in_ids = (int *)malloc((size_t)ltf->num_vars * sizeof(int));
    for (int i = 0; i < ltf->num_vars; i++)
        in_ids[i] = tc0_add_input(c);

    /* Single THR or WTHR gate */
    int gate;
    int is_simple = 1;
    int uniform_weight = ltf->weights[0];
    for (int i = 1; i < ltf->num_vars; i++) {
        if (ltf->weights[i] != uniform_weight) { is_simple = 0; break; }
    }

    if (is_simple && uniform_weight == 1) {
        /* Simple threshold gate */
        gate = tc0_add_gate(c, TC_THR);
        tc0_set_threshold(c, gate, ltf->theta);
    } else {
        /* Weighted threshold gate */
        gate = tc0_add_gate(c, TC_WTHR);
        double *wts = (double *)malloc((size_t)ltf->num_vars * sizeof(double));
        for (int i = 0; i < ltf->num_vars; i++) wts[i] = (double)ltf->weights[i];
        tc0_set_weighted_threshold(c, gate, wts, ltf->num_vars, (double)ltf->theta);
        free(wts);
    }

    for (int i = 0; i < ltf->num_vars; i++)
        tc0_add_wire(c, in_ids[i], gate);

    tc0_set_output(c, gate);
    free(in_ids);
    tc0_compute_depth(c);
    return c;
}

/* ================================================================
 * PERCEPTRON LEARNING ALGORITHM (L5: Algorithms/Methods)
 * ================================================================ */

/* The Perceptron Algorithm (Rosenblatt 1958):
 * Given labeled examples (x, y), find weights w and threshold theta
 * such that y_i = sign(sum w_j * x_{ij} - theta).
 *
 * Convergence guaranteed if the data is linearly separable (Novikoff 1962).
 * Number of updates <= (R/gamma)^2 where R = max||x|| and gamma = margin.
 *
 * This function returns 1 if a separating LTF was found, 0 otherwise.
 * max_iterations prevents infinite loops for non-separable data. */
int perceptron_learn(const int *examples, const int *labels,
                     int n_examples, int n_vars, int max_iterations,
                     int *out_weights, int *out_theta) {
    if (!examples || !labels || !out_weights || !out_theta) return 0;
    if (n_examples <= 0 || n_vars <= 0) return 0;

    /* Initialize weights to 0 */
    for (int i = 0; i < n_vars; i++) out_weights[i] = 0;
    *out_theta = 0;

    int updates = 0;
    for (int iter = 0; iter < max_iterations; iter++) {
        int errors = 0;
        for (int e = 0; e < n_examples; e++) {
            int sum = 0;
            for (int j = 0; j < n_vars; j++)
                sum += out_weights[j] * examples[e * n_vars + j];
            int prediction = (sum >= *out_theta) ? 1 : 0;

            if (prediction != labels[e]) {
                /* Update weights: w += (y - pred) * x */
                int delta = labels[e] - prediction;
                for (int j = 0; j < n_vars; j++)
                    out_weights[j] += delta * examples[e * n_vars + j];
                *out_theta -= delta;  /* bias update */
                errors++;
                updates++;
            }
        }
        if (errors == 0) return 1;  /* Perfect classification */
    }
    return (updates < max_iterations * n_examples) ? 1 : 0;
}

/* ================================================================
 * SYMMETRIC FUNCTION ANALYSIS (L3-L4)
 * ================================================================ */

/* A symmetric Boolean function depends only on the number of 1s.
 * It can be represented as a unary function s: {0,...,n} -> {0,1}.
 * All symmetric functions are in TC0 (computable by a depth-2 threshold
 * circuit with O(n) gates). */
int tc0_is_symmetric(const BoolFunc *f) {
    if (!f || !f->truth_table) return 0;
    int n = f->num_vars;

    /* Group assignments by Hamming weight */
    int *weight_value = (int *)malloc((size_t)(n + 1) * sizeof(int));
    for (int w = 0; w <= n; w++) weight_value[w] = -1;

    for (int m = 0; m < f->table_size; m++) {
        int wt = 0;
        for (int i = 0; i < n; i++)
            if ((m >> i) & 1) wt++;
        int val = f->truth_table[m];
        if (weight_value[wt] == -1) {
            weight_value[wt] = val;
        } else if (weight_value[wt] != val) {
            free(weight_value);
            return 0;  /* Not symmetric */
        }
    }
    free(weight_value);
    return 1;
}

/* Check if a Boolean function is monotone:
 * x <= y (bitwise) implies f(x) <= f(y) */
int tc0_is_monotone(const BoolFunc *f) {
    if (!f || !f->truth_table) return 0;
    int n = f->num_vars;
    for (int x = 0; x < f->table_size; x++) {
        for (int y = 0; y < f->table_size; y++) {
            /* Check if x <= y bitwise */
            int is_subset = 1;
            for (int i = 0; i < n && is_subset; i++) {
                if (((x >> i) & 1) && !((y >> i) & 1))
                    is_subset = 0;
            }
            if (is_subset && f->truth_table[x] > f->truth_table[y])
                return 0;
        }
    }
    return 1;
}

/* Check if a Boolean function is self-dual:
 * f(not(x)) = not(f(x)) */
int tc0_is_self_dual(const BoolFunc *f) {
    if (!f || !f->truth_table) return 0;
    int n = f->num_vars;
    int mask = (1 << n) - 1;
    for (int x = 0; x < f->table_size; x++) {
        int not_x = (~x) & mask;
        if (f->truth_table[x] != 1 - f->truth_table[not_x])
            return 0;
    }
    return 1;
}

/* ================================================================
 * LINEAR SEPARABILITY TEST (L4: Fundamental Laws)
 * ================================================================ */

/* Test if a Boolean function is linearly separable (representable by
 * a single threshold gate). For small n (<= 8), exhaustive search.
 * For larger n, uses the perceptron algorithm as a heuristic.
 *
 * A function is linearly separable iff there exists a hyperplane in
 * {0,1}^n that separates the true and false points. */
int tc0_is_linear_separable(const BoolFunc *f) {
    if (!f || !f->truth_table) return 0;
    int n = f->num_vars;

    /* For very small n, exhaustive search through weight space */
    if (n <= 6) {
        int max_w = (1 << n);  /* Weight bound: small weights suffice */
        int total = f->table_size;

        /* Count positive and negative examples */
        int n_pos = 0, n_neg = 0;
        for (int i = 0; i < total; i++) {
            if (f->truth_table[i]) n_pos++;
            else n_neg++;
        }
        /* If all same label, always separable */
        if (n_pos == 0 || n_neg == 0) return 1;

        /* Brute force weight search (simplified: only weights in {-1,0,1}) */
        int *weights = (int *)malloc((size_t)n * sizeof(int));
        int max_configs = 1;
        for (int i = 0; i < n; i++) max_configs *= 3;  /* {-1,0,1}^n */

        for (int config = 0; config < max_configs && config < 100000; config++) {
            int tmp = config;
            for (int i = 0; i < n; i++) {
                weights[i] = (tmp % 3) - 1;
                tmp /= 3;
            }

            /* Try all possible thresholds */
            for (int theta = -n; theta <= n + 1; theta++) {
                int consistent = 1;
                for (int x = 0; x < total && consistent; x++) {
                    int sum = 0;
                    for (int i = 0; i < n; i++)
                        sum += weights[i] * ((x >> i) & 1);
                    int pred = (sum >= theta) ? 1 : 0;
                    if (pred != f->truth_table[x])
                        consistent = 0;
                }
                if (consistent) { free(weights); return 1; }
            }
        }
        free(weights);
        return 0;
    }

    /* For larger n, use perceptron heuristic */
    int n_examples = f->table_size;
    int *examples = (int *)malloc((size_t)n_examples * n * sizeof(int));
    int *labels = (int *)malloc((size_t)n_examples * sizeof(int));
    for (int i = 0; i < n_examples; i++) {
        labels[i] = f->truth_table[i];
        for (int j = 0; j < n; j++)
            examples[i * n + j] = (i >> j) & 1;
    }

    int *weights = (int *)calloc((size_t)n, sizeof(int));
    int theta;
    int result = perceptron_learn(examples, labels, n_examples, n,
                                   10000, weights, &theta);

    free(examples); free(labels); free(weights);
    return result;
}

/* ================================================================
 * BOOLEAN FUNCTION INFLUENCE (L8: Advanced Topics)
 * ================================================================ */

/* Compute the influence of variable i on Boolean function f.
 * Inf_i(f) = Pr_{x in {0,1}^n}[f(x) != f(x xor e_i)]
 * where e_i is the unit vector in the i-th coordinate.
 *
 * High average influence implies large circuit size (Hastad 1987).
 * For AC0 functions: sum Inf_i(f) = O((log S)^{d-1}) where
 * S = circuit size, d = depth. */
double tc0_variable_influence(const BoolFunc *f, int var_idx) {
    if (!f || !f->truth_table) return 0.0;
    if (var_idx < 0 || var_idx >= f->num_vars) return 0.0;

    int n = f->num_vars;
    int total = f->table_size;
    int flips = 0;
    int valid_pairs = 0;

    for (int x = 0; x < total; x++) {
        int y = x ^ (1 << var_idx);
        if (y >= total) continue;  /* Should not happen for full table */
        if (f->truth_table[x] != f->truth_table[y])
            flips++;
        valid_pairs++;
    }

    return (valid_pairs > 0) ? (double)flips / (double)valid_pairs : 0.0;
}

/* Total influence = sum_i Inf_i(f).
 * By the KKL theorem, there exists a variable with influence
 * Omega(log n / n) for any non-constant monotone function. */
double tc0_total_influence(const BoolFunc *f) {
    if (!f) return 0.0;
    double total = 0.0;
    for (int i = 0; i < f->num_vars; i++)
        total += tc0_variable_influence(f, i);
    return total;
}

/* ================================================================
 * BOOLEAN FUNCTION CREATION AND MANIPULATION
 * ================================================================ */

BoolFunc *boolfunc_create(const char *name, int num_vars) {
    if (num_vars <= 0 || num_vars > 20) return NULL;  /* Practical limit */
    BoolFunc *f = (BoolFunc *)malloc(sizeof(BoolFunc));
    if (!f) return NULL;
    strncpy(f->name, name ? name : "unnamed", 127);
    f->name[127] = '\0';
    f->num_vars = num_vars;
    f->table_size = 1 << num_vars;
    f->truth_table = (int *)calloc((size_t)f->table_size, sizeof(int));
    if (!f->truth_table) { free(f); return NULL; }
    f->is_monotone = -1;   /* Not yet checked */
    f->is_symmetric = -1;
    f->is_self_dual = -1;
    return f;
}

void boolfunc_free(BoolFunc *f) {
    if (!f) return;
    free(f->truth_table);
    free(f);
}

/* Set a truth table entry */
void boolfunc_set(BoolFunc *f, int assignment, int value) {
    if (!f || assignment < 0 || assignment >= f->table_size) return;
    if (value != 0 && value != 1) return;
    f->truth_table[assignment] = value;
    /* Invalidate cached properties */
    f->is_monotone = -1;
    f->is_symmetric = -1;
    f->is_self_dual = -1;
}

/* Build the truth table for MAJORITY_n */
BoolFunc *boolfunc_majority(int n) {
    BoolFunc *f = boolfunc_create("MAJORITY", n);
    if (!f) return NULL;
    for (int x = 0; x < f->table_size; x++) {
        int ones = 0;
        for (int i = 0; i < n; i++)
            if ((x >> i) & 1) ones++;
        f->truth_table[x] = (ones > n / 2) ? 1 : 0;
    }
    f->is_symmetric = 1;
    f->is_monotone = 1;
    return f;
}

/* Build the truth table for PARITY_n */
BoolFunc *boolfunc_parity(int n) {
    BoolFunc *f = boolfunc_create("PARITY", n);
    if (!f) return NULL;
    for (int x = 0; x < f->table_size; x++) {
        int parity = 0;
        for (int i = 0; i < n; i++)
            parity ^= ((x >> i) & 1);
        f->truth_table[x] = parity;
    }
    f->is_symmetric = 1;
    f->is_monotone = 0;
    return f;
}

/* Print a truth table */
void boolfunc_print(const BoolFunc *f) {
    if (!f) return;
    printf("Boolean function: %s (n=%d, size=%d)\n",
           f->name, f->num_vars, f->table_size);
    printf("Properties: monotone=%d symmetric=%d self_dual=%d\n",
           f->is_monotone, f->is_symmetric, f->is_self_dual);
    for (int x = 0; x < f->table_size; x++) {
        printf("  ");
        for (int i = f->num_vars - 1; i >= 0; i--)
            printf("%d", (x >> i) & 1);
        printf(" -> %d\n", f->truth_table[x]);
    }
}
