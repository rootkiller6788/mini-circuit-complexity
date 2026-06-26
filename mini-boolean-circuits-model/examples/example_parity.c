#include "circuit.h"
#include "circuit_builder.h"
#include "circuit_analysis.h"
#include <stdio.h>
#include "circuit_complexity.h"
#include <stdlib.h>

int main(void) {
    printf("=== Example: PARITY Circuit ===\n\n");
    printf("PARITY(x0,...,xn-1) = XOR of all n inputs.\n");
    printf("Not in AC^0 (Furst-Saxe-Sipser 1981, Hastad 1986).\n");
    printf("In NC^1 via balanced XOR tree of depth O(log n).\n\n");

    for (int n = 2; n <= 8; n++) {
        BooleanCircuit* p = circuit_parity(n);
        printf("PARITY(%d): size=%d, depth=%d, class=%s\n",
               n, circuit_size(p), circuit_depth(p),
               circuit_class_name(circuit_determine_class(p)));

        /* Verify on a few inputs */
        int* inputs = (int*)calloc((size_t)n, sizeof(int));
        inputs[0] = 1; /* single 1 = odd parity */
        int out = circuit_evaluate(p, inputs);
        printf("  P(1,0,...,0) = %d (expected 1)\n", out);

        inputs[0] = 1; inputs[1] = 1; /* two 1s = even parity */
        out = circuit_evaluate(p, inputs);
        printf("  P(1,1,0,...,0) = %d (expected 0)\n", out);

        circuit_free(p);
        free(inputs);
    }
    printf("\nPARITY is the canonical example separating AC^0 from NC^1.\n");
    return 0;
}
