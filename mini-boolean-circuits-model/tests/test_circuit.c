/* test_circuit.c -- Comprehensive tests for mini-boolean-circuits-model */
#include "circuit.h"
#include "circuit_analysis.h"
#include "circuit_builder.h"
#include "circuit_complexity.h"
#include "circuit_synthesis.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>

static int passed = 0, failed = 0;
#define T(n,e) do{if(e)passed++;else{failed++;printf("  FAIL: %s\n",n);}}while(0)

int main(void) {
    setbuf(stdout, NULL);
    printf("===== Boolean Circuit Model Test Suite =====\n");

    /* L1: Core Operations */
    printf("\n--- L1: Core Operations ---\n");
    BooleanCircuit* c = circuit_create(20);
    T("create", c != NULL);
    int a = circuit_add_gate(c, INPUT, -1, -1);
    int b = circuit_add_gate(c, INPUT, -1, -1);
    T("input count", circuit_input_count(c) == 2);
    int ag = circuit_add_gate(c, AND, a, b);
    int og = circuit_add_gate(c, NOT, ag, -1);
    circuit_set_output(c, &og, 1);
    T("nand(0,0)=1", circuit_evaluate(c, (int[]){0,0}) == 1);
    T("nand(1,1)=0", circuit_evaluate(c, (int[]){1,1}) == 0);
    T("depth==2", circuit_depth(c) == 2);
    T("is_dag", circuit_is_dag(c) == 1);
    T("is_valid", circuit_is_valid(c) == 1);
    circuit_free(c);

    /* L5: Builders - PARITY */
    printf("\n--- L5: Builders ---\n");
    BooleanCircuit* p3 = circuit_parity(3);
    T("parity(000)=0", circuit_evaluate(p3, (int[]){0,0,0}) == 0);
    T("parity(001)=1", circuit_evaluate(p3, (int[]){1,0,0}) == 1);
    T("parity(111)=1", circuit_evaluate(p3, (int[]){1,1,1}) == 1);
    circuit_free(p3);

    /* AND/OR trees */
    BooleanCircuit* at = circuit_and_tree(4);
    T("and(1111)=1", circuit_evaluate(at, (int[]){1,1,1,1}) == 1);
    T("and(1110)=0", circuit_evaluate(at, (int[]){1,1,1,0}) == 0);
    circuit_free(at);
    BooleanCircuit* ot = circuit_or_tree(4);
    T("or(0000)=0", circuit_evaluate(ot, (int[]){0,0,0,0}) == 0);
    T("or(0001)=1", circuit_evaluate(ot, (int[]){0,0,0,1}) == 1);
    circuit_free(ot);

    /* Half adder */
    BooleanCircuit* ha = circuit_half_adder();
    int* ha_out = circuit_evaluate_all(ha, (int[]){0,0}, &(int){0});
    T("ha sum(0,0)=0", ha_out[0] == 0);
    T("ha carry(0,0)=0", ha_out[1] == 0);
    free(ha_out); circuit_free(ha);

    /* Full adder */
    BooleanCircuit* fa = circuit_full_adder();
    int* fa_out = circuit_evaluate_all(fa, (int[]){1,1,1}, &(int){0});
    T("fa sum(1,1,1)=1", fa_out[0] == 1);
    T("fa carry(1,1,1)=1", fa_out[1] == 1);
    free(fa_out); circuit_free(fa);

    /* MAJORITY */
    BooleanCircuit* maj = circuit_majority(3);
    T("maj(011)=1", circuit_evaluate(maj, (int[]){0,1,1}) == 1);
    T("maj(001)=0", circuit_evaluate(maj, (int[]){1,0,0}) == 0);
    T("maj(000)=0", circuit_evaluate(maj, (int[]){0,0,0}) == 0);
    T("maj(111)=1", circuit_evaluate(maj, (int[]){1,1,1}) == 1);
    circuit_free(maj);

    /* Equality: inputs interleaved x0,y0,x1,y1,x2,y2 */
    BooleanCircuit* eq = circuit_equality(3);
    /* x=(1,0,1), y=(1,0,1) => eq=1 */
    T("eq match", circuit_evaluate(eq, (int[]){1,1, 0,0, 1,1}) == 1);
    /* x=(1,0,1), y=(0,1,0) => eq=0 */
    T("eq mismatch", circuit_evaluate(eq, (int[]){1,0, 0,1, 1,0}) == 0);
    circuit_free(eq);

    /* MUX2: gate order is a,b,s. (a=0,b=1,s=0) -> selects a -> 0 */
    BooleanCircuit* mux = circuit_mux2();
    T("mux(a=0,b=1,s=0)=0", circuit_evaluate(mux, (int[]){0,1,0}) == 0);
    T("mux(a=0,b=1,s=1)=1", circuit_evaluate(mux, (int[]){0,1,1}) == 1);
    T("mux(a=1,b=0,s=0)=1", circuit_evaluate(mux, (int[]){1,0,0}) == 1);
    circuit_free(mux);

    /* MOD-2 (PARITY) check */
    BooleanCircuit* mod2 = circuit_mod(4, 2);
    T("mod2(0000)=1", circuit_evaluate(mod2, (int[]){0,0,0,0}) == 1);
    T("mod2(1000)=0", circuit_evaluate(mod2, (int[]){1,0,0,0}) == 0);
    T("mod2(1100)=1", circuit_evaluate(mod2, (int[]){1,1,0,0}) == 1);
    circuit_free(mod2);

    /* Adder: interleaved a[i],b[i]. a=01(1), b=01(1) -> sum=10(2) */
    int carry;
    BooleanCircuit* ad2 = circuit_adder(2, &carry);
    int* ad_out = circuit_evaluate_all(ad2, (int[]){1,1,0,0}, &(int){0});
    T("adder sum0=0", ad_out[0] == 0);
    T("adder sum1=1", ad_out[1] == 1);
    T("adder carry=0", ad_out[2] == 0);
    free(ad_out);
    circuit_free(ad2);

    /* L5: Analysis */
    printf("\n--- L5: Analysis ---\n");
    BooleanCircuit* p4 = circuit_parity(4);
    int gc[11]; circuit_count_gates(p4, gc);
    T("has inputs", gc[INPUT] == 4);
    T("parity SAT", circuit_is_satisfiable(p4, 4) == 1);
    T("parity not taut", circuit_is_tautology(p4, 4) == 0);
    T("sat count 8", circuit_count_sat_assignments(p4, 4) == 8);
    BooleanCircuit* p4b = circuit_parity(4);
    T("equivalent", circuits_equivalent(p4, p4b, 4) == 1);
    circuit_free(p4b);

    /* NAND-only conversion */
    BooleanCircuit* nc = circuit_to_nand_only(p4);
    T("nand-equiv", circuits_equivalent(p4, nc, 4) == 1);
    circuit_free(nc);
    circuit_free(p4);

    /* L2+L4: Complexity */
    printf("\n--- L2+L4: Complexity ---\n");
    BooleanCircuit* p6 = circuit_parity(6);
    double dens = circuit_density(p6);
    T("density>=0", dens >= 0.0 && dens <= 1.0);
    T("complexity product>0", circuit_complexity_product(p6) > 0.0);
    const char* cn = circuit_class_name(circuit_determine_class(p6));
    T("class name", cn != NULL);
    T("shannon>0", shannon_lower_bound(10) > 0.0);
    T("lupanov>0", lupanov_upper_bound(10) > 0.0);
    T("hastad>0", hastad_parity_lower_bound(10, 3) > 0.0);
    T("kw parity=3", kw_depth_lower_bound(8, 0) == 3);
    CircuitFamily* pf = circuit_family_parity(4);
    T("family created", pf != NULL);
    T("family class NC1", circuit_family_class(pf) == CLASS_NC1);
    T("family uniform", circuit_family_is_uniform(pf) == 1);
    circuit_family_free(pf);
    circuit_free(p6);

    /* L5: Synthesis */
    printf("\n--- L5: Synthesis ---\n");
    int xor_truth[] = {0,1,1,0};
    BooleanCircuit* dnf_c = circuit_synthesize_dnf(xor_truth, 2);
    T("dnf synth", dnf_c != NULL);
    T("dnf xor(0,1)=1", circuit_evaluate(dnf_c, (int[]){0,1}) == 1);
    T("dnf xor(1,1)=0", circuit_evaluate(dnf_c, (int[]){1,1}) == 0);
    circuit_free(dnf_c);
    BooleanCircuit* cnf_c = circuit_synthesize_cnf(xor_truth, 2);
    T("cnf synth", cnf_c != NULL);
    T("cnf xor(1,1)=0", circuit_evaluate(cnf_c, (int[]){1,1}) == 0);
    circuit_free(cnf_c);
    BooleanCircuit* esop_c = circuit_synthesize_esop(xor_truth, 2);
    T("esop synth", esop_c != NULL);
    T("esop xor(0,1)=1", circuit_evaluate(esop_c, (int[]){0,1}) == 1);
    circuit_free(esop_c);
    BooleanCircuit* qm_c = circuit_synthesize_qm(xor_truth, 2);
    T("qm synth", qm_c != NULL);
    circuit_free(qm_c);

    /* L3: Serialization */
    printf("\n--- L3: Serialization ---\n");
    BooleanCircuit* sp = circuit_parity(4);
    size_t sz = 0;
    uint8_t* data = circuit_serialize(sp, &sz);
    T("serialize non-null", data != NULL);
    BooleanCircuit* dc = circuit_deserialize(data, sz);
    T("round-trip equiv", circuits_equivalent(sp, dc, 4) == 1);
    free(data); circuit_free(dc); circuit_free(sp);

    printf("\n===== Results: %d passed, %d failed =====\n", passed, failed);
    return (failed > 0) ? 1 : 0;
}