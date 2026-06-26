/* tc0_threshold.h -- Threshold Function Theory (L3-L4)
 *
 * Linear threshold functions, perceptron learning, symmetric functions,
 * and the mathematical structures of threshold circuits.
 */

#ifndef TC0_THRESHOLD_H
#define TC0_THRESHOLD_H

#include "tc0.h"

/* ================================================================
 * LINEAR THRESHOLD FUNCTIONS
 * ================================================================ */

/* Create/free a linear threshold function */
LinearThresholdFunc *ltf_create(int num_vars, const int *weights, int theta);
void ltf_free(LinearThresholdFunc *ltf);

/* Evaluate LTF on input */
int ltf_evaluate(const LinearThresholdFunc *ltf, const int *x);

/* Construct MAJORITY and THRESHOLD(k) as LTFs */
LinearThresholdFunc *ltf_majority(int n);
LinearThresholdFunc *ltf_threshold(int n, int k);

/* Convert an LTF to a TC0Circuit with a single THR/WTHR gate */
TC0Circuit *ltf_to_circuit(const LinearThresholdFunc *ltf);

/* ================================================================
 * PERCEPTRON LEARNING ALGORITHM (Rosenblatt 1958, Novikoff 1962)
 * ================================================================ */

/* Learn a linear separator for labeled examples.
 * Input: examples (n_examples × n_vars in row-major), labels (0/1).
 * Output: weights[n_vars], theta.
 * Returns 1 if separable, 0 if non-separable (or max iterations reached). */
int perceptron_learn(const int *examples, const int *labels,
                     int n_examples, int n_vars, int max_iterations,
                     int *out_weights, int *out_theta);

/* ================================================================
 * BOOLEAN FUNCTION PROPERTIES
 * ================================================================ */

/* Create/free a Boolean function from its truth table */
BoolFunc *boolfunc_create(const char *name, int num_vars);
void boolfunc_free(BoolFunc *f);
void boolfunc_set(BoolFunc *f, int assignment, int value);
void boolfunc_print(const BoolFunc *f);

/* Construct standard Boolean functions */
BoolFunc *boolfunc_majority(int n);
BoolFunc *boolfunc_parity(int n);

/* Check Boolean function properties:
 *   symmetric:   f depends only on Hamming weight of input
 *   monotone:     x <= y (bitwise) implies f(x) <= f(y)
 *   self-dual:    f(not(x)) = not(f(x))
 *   linear-sep:   f is a linear threshold function (single gate) */
int tc0_is_symmetric(const BoolFunc *f);
int tc0_is_monotone(const BoolFunc *f);
int tc0_is_self_dual(const BoolFunc *f);
int tc0_is_linear_separable(const BoolFunc *f);

/* Compute variable influences (for lower bound analysis) */
double tc0_variable_influence(const BoolFunc *f, int var_idx);
double tc0_total_influence(const BoolFunc *f);

/* Compute PTF degree of a Boolean function */
int tc0_compute_ptf_degree(const BoolFunc *f);

#endif /* TC0_THRESHOLD_H */
