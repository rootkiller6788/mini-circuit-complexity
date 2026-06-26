#include "circuit.h"
#include "circuit_synthesis.h"
#include "circuit_analysis.h"
#include <stdio.h>
#include "circuit_complexity.h"

int main(void) {
    printf("=== Example: Logic Synthesis ===\n\n");
    printf("Synthesizing circuits from truth tables using multiple methods.\n\n");

    /* Truth table for 2-input XOR: {0,1,1,0} */
    int xor_tt[] = {0, 1, 1, 0};
    int n = 2;

    printf("Target: XOR(a,b) trivia table [0,1,1,0]\n\n");

    BooleanCircuit* dnf = circuit_synthesize_dnf(xor_tt, n);
    printf("DNF synthesis: size=%d, depth=%d\n",
           circuit_size(dnf), circuit_depth(dnf));
    circuit_free(dnf);

    BooleanCircuit* cnf = circuit_synthesize_cnf(xor_tt, n);
    printf("CNF synthesis: size=%d, depth=%d\n",
           circuit_size(cnf), circuit_depth(cnf));
    circuit_free(cnf);

    BooleanCircuit* esop = circuit_synthesize_esop(xor_tt, n);
    printf("ESOP (Reed-Muller): size=%d, depth=%d\n",
           circuit_size(esop), circuit_depth(esop));
    circuit_free(esop);

    BooleanCircuit* qm = circuit_synthesize_qm(xor_tt, n);
    printf("Quine-McCluskey (exact min): size=%d, depth=%d\n",
           circuit_size(qm), circuit_depth(qm));
    circuit_free(qm);

    BooleanCircuit* sh = circuit_synthesize_shannon(xor_tt, n);
    printf("Shannon decomposition: size=%d, depth=%d\n",
           circuit_size(sh), circuit_depth(sh));
    circuit_free(sh);

    printf("\nMultiple synthesis methods produce logically equivalent circuits\n");
    printf("with different size/depth tradeoffs.\n");
    return 0;
}
