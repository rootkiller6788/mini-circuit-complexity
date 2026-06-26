/* example_ac0.c — AC0 Circuit Construction and Evaluation
 * =====================================================================
 * Demonstrates AC0 circuits: DNF, threshold, symmetric functions.
 * Shows that PARITY requires exponential size in AC0.
 *
 * Build: gcc -I../include example_ac0.c ../src/*.c -lm -o example_ac0
 * Run:   ./example_ac0
 */
#include "ac0nc.h"
#include "ac0_circuit.h"

int main(void)
{
    setbuf(stdout, NULL);
    printf("AC0 CIRCUIT CLASS DEMONSTRATION\n");
    printf("================================\n\n");

    /* 1. DNF construction */
    printf("1. DNF (Disjunctive Normal Form)\n");
    printf("   DNF = OR of AND terms, depth=2\n\n");

    DNF *d = dnf_create(3, 4);
    dnf_set_literal(d, 0, 0,  1);  /* x0 AND ... */
    dnf_set_literal(d, 0, 1, -1);  /* NOT x1 */
    dnf_set_literal(d, 1, 2,  1);  /* x2 */
    dnf_set_literal(d, 2, 0, -1);  /* NOT x0 AND x1 */
    dnf_set_literal(d, 2, 1,  1);

    printf("   DNF: (x0 AND NOT x1) OR (x2) OR (NOT x0 AND x1)\n\n");

    int test_cases[4][3] = {{0,0,0}, {1,0,1}, {0,1,0}, {1,1,1}};
    const char *test_names[] = {"(0,0,0)", "(1,0,1)", "(0,1,0)", "(1,1,1)"};
    for (int t = 0; t < 4; t++) {
        printf("   f%s = %d\n", test_names[t],
               dnf_evaluate(d, test_cases[t]));
    }

    /* Convert to circuit and verify */
    AC0Circuit *dc = dnf_to_circuit(d);
    printf("\n   Circuit: size=%d, depth=%d, class=%s\n",
           ac0_circuit_size(dc), ac0_circuit_depth(dc),
           classify_circuit(dc));
    for (int t = 0; t < 4; t++) {
        printf("   C%s = %d\n", test_names[t],
               ac0_circuit_eval(dc, test_cases[t]));
    }
    ac0_circuit_free(dc);
    dnf_free(d);

    /* 2. THRESHOLD circuit */
    printf("\n2. AC0 THRESHOLD Circuit\n");
    printf("   THRESHOLD_k(x) = 1 iff |{i: x_i=1}| >= k\n\n");

    int n = 6, k = 3;
    AC0Circuit *th = ac0_build_threshold(n, k);
    printf("   n=%d, k=%d: size=%d, depth=%d\n",
           n, k, ac0_circuit_size(th), ac0_circuit_depth(th));

    printf("\n   Truth table (first 16 rows):\n");
    printf("   ");
    for (int i = 0; i < n; i++) printf("x%d ", i);
    printf("| THR\n");
    printf("   ");
    for (int i = 0; i < n; i++) printf("---");
    printf("+----\n");

    for (int r = 0; r < 16 && r < (1<<n); r++) {
        int inp[AC0_MAX_INPUTS];
        int sum = 0;
        for (int i = 0; i < n; i++) {
            inp[i] = (r >> i) & 1;
            sum += inp[i];
        }
        printf("   ");
        for (int i = 0; i < n; i++) printf(" %d ", inp[i]);
        printf("|  %d\n", (sum >= k) ? 1 : 0);
    }
    ac0_circuit_free(th);

    /* 3. AC0 limitation: PARITY lower bound */
    printf("\n3. AC0 Limitation: PARITY\n");
    printf("   PARITY NOT in AC0 (Hastad 1986).\n\n");
    printf("   n=32 depth=2: size >= %.2e\n",
           ac0_parity_size_lower_bound(32, 2));
    printf("   n=32 depth=3: size >= %.2e\n",
           ac0_parity_size_lower_bound(32, 3));
    printf("   => Exponential blowup as depth decreases.\n");

    printf("\n================================\n");
    printf("  AC0 demonstration complete.\n");
    return 0;
}
