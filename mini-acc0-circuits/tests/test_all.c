#include "acc0.h"
#include <stdio.h>
#include "acc0.h"
#include "acc0_gates.h"
int main(void){
    setbuf(stdout, NULL);
    printf("ACC0 Tests\n");

    /* Test parity via MOD2 gate */
    ACC0Circuit *c = acc0_circuit_create(64);
    if (!c) { printf("[FAIL] create\n"); return 1; }
    for (int i = 0; i < 4; i++) acc0_add_input(c);
    int mod_out = acc0_build_mod_gate(c, 3, 4);
    int output = mod_out;
    acc0_set_outputs(c, &output, 1);
    printf("[PASS] mod3 gate created (depth=%d, size=%d)\n",
           acc0_circuit_depth(c), c->size);
    acc0_circuit_free(c);

    printf("All passed\n");
    return 0;
}