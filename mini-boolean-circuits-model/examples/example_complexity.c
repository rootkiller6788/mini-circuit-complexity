#include "circuit.h"
#include "circuit_builder.h"
#include "circuit_complexity.h"
#include <stdio.h>
#include <math.h>

int main(void) {
    printf("=== Example: Circuit Complexity Classes ===\n\n");
    printf("Major circuit complexity classes and their separations:\n\n");

    printf("%-12s %-10s %-10s %-12s\n", "Function", "Size", "Depth", "Class");
    printf("----------------------------------------------\n");

    for (int n = 2; n <= 8; n += 2) {
        BooleanCircuit* p = circuit_parity(n);
        printf("PARITY(%d)    %-10d %-10d %-12s\n",
               n, circuit_size(p), circuit_depth(p),
               circuit_class_name(circuit_determine_class(p)));
        circuit_free(p);
    }

    printf("\nTheoretical bounds (Shannon 1949, Lupanov 1958):\n");
    for (int n = 2; n <= 8; n++) {
        printf("  n=%d: Shannon LB=%.0f, Lupanov UB=%.0f\n",
               n, shannon_lower_bound(n), lupanov_upper_bound(n));
    }

    printf("\nHastad (1986): PARITY requires exp(Omega(n^{1/(d-1)})) size\n");
    printf("  in depth-d AC^0 circuits. For n=8, d=3: %.0f\n",
           hastad_parity_lower_bound(8, 3));

    printf("\nNechiporuk (1966): independent subfunctions give\n");
    printf("  Omega(n^2 / log^2 n) lower bounds for certain functions.\n");

    printf("\nCircuit Family demo:\n");
    CircuitFamily* cf = circuit_family_parity(4);
    circuit_family_print(cf);
    circuit_family_free(cf);

    return 0;
}
