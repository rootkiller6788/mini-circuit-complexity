import os
base = os.path.dirname(os.path.abspath(__file__)) if '__file__' in dir() else '.'

def W(fname, content):
    path = os.path.join(base, 'src', fname)
    mode = 'a' if os.path.exists(path) and os.path.getsize(path) > 0 else 'w'
    with open(path, mode, encoding='utf-8') as f:
        f.write(content)
    print(f'  wrote {len(content)} chars to {fname}')

content = r"""/* ===================================================================
 * L5: Circuit Minimization
 * =================================================================== */

BooleanCircuit* circuit_minimize(BooleanCircuit* c) {
    assert(c != NULL);
    BooleanCircuit* result = circuit_constant_propagate(c);
    if (!result) return NULL;
    int n = result->n_gates;
    int* equiv = (int*)malloc((size_t)n * sizeof(int));
    assert(equiv);
    for (int i = 0; i < n; i++) equiv[i] = i;
    for (int i = 0; i < n; i++) {
        if (equiv[i] != i) continue;
        const Gate* gi = &result->gates[i];
        if (gi->type == INPUT || gi->type == CONST0 || gi->type == CONST1) continue;
        for (int j = i + 1; j < n; j++) {
            if (equiv[j] != j) continue;
            const Gate* gj = &result->gates[j];
            if (gi->type != gj->type) continue;
            int same = 0;
            if (gi->type == MAJ || gi->type == THR) {
                if (gi->fanin == gj->fanin) {
                    same = 1;
                    for (int k = 0; k < gi->fanin; k++) {
                        if (equiv[gi->fanin_ids[k]] != equiv[gj->fanin_ids[k]]) { same = 0; break; }
                    }
                }
            } else {
                int in1_i = (gi->input1 >= 0) ? equiv[gi->input1] : -1;
                int in2_i = (gi->input2 >= 0) ? equiv[gi->input2] : -1;
                int in1_j = (gj->input1 >= 0) ? equiv[gj->input1] : -1;
                int in2_j = (gj->input2 >= 0) ? equiv[gj->input2] : -1;
                same = (in1_i == in1_j && in2_i == in2_j);
            }
            if (same) equiv[j] = i;
        }
    }
    int* new_id = (int*)malloc((size_t)n * sizeof(int));
    assert(new_id);
    for (int i = 0; i < n; i++) new_id[i] = -1;
    BooleanCircuit* minc = circuit_create(n + 10);
    int* order = (int*)malloc((size_t)n * sizeof(int));
    assert(order);
    circuit_topological_order(result, order);
    for (int idx = 0; idx < n; idx++) {
        int i = order[idx];
        if (equiv[i] != i && new_id[equiv[i]] >= 0) {
            new_id[i] = new_id[equiv[i]];
            continue;
        }
        const Gate* g = &result->gates[i];
        if (g->type == MAJ || g->type == THR) {
            int* fids = (int*)malloc((size_t)g->fanin * sizeof(int));
            assert(fids);
            for (int j = 0; j < g->fanin; j++) fids[j] = new_id[equiv[g->fanin_ids[j]]];
            new_id[i] = circuit_add_maj_gate(minc, g->type, g->fanin, fids);
            free(fids);
        } else {
            int a = (g->input1 >= 0) ? new_id[equiv[g->input1]] : -1;
            int b = (g->input2 >= 0) ? new_id[equiv[g->input2]] : -1;
            new_id[i] = circuit_add_gate(minc, g->type, a, b);
        }
    }
    int* outs = (int*)malloc((size_t)result->n_outputs * sizeof(int));
    assert(outs);
    for (int i = 0; i < result->n_outputs; i++) outs[i] = new_id[equiv[result->output_ids[i]]];
    circuit_set_output(minc, outs, result->n_outputs);
    free(outs); free(order); free(new_id); free(equiv);
    if (result != c) circuit_free(result);
    return minc;
}

BooleanCircuit* circuit_constant_propagate(BooleanCircuit* c) {
    assert(c != NULL);
    int n = c->n_gates;
    int* val = (int*)malloc((size_t)n * sizeof(int));
    assert(val);
    for (int i = 0; i < n; i++) val[i] = -1;
    for (int i = 0; i < n; i++) {
        if (c->gates[i].type == CONST0) val[i] = 0;
        else if (c->gates[i].type == CONST1) val[i] = 1;
    }
    int changed = 1;
    while (changed) {
        changed = 0;
        for (int i = 0; i < n; i++) {
            if (val[i] >= 0) continue;
            const Gate* g = &c->gates[i];
            int ready = 0, result = 0;
            switch (g->type) {
            case NOT:
                if (g->input1 >= 0 && val[g->input1] >= 0) { result = !val[g->input1]; ready = 1; }
                break;
            case AND:
                if (g->input1 >= 0 && g->input2 >= 0) {
                    if (val[g->input1] == 0 || val[g->input2] == 0) { result = 0; ready = 1; }
                    else if (val[g->input1] == 1 && val[g->input2] == 1) { result = 1; ready = 1; }
                }
                break;
            case OR:
                if (g->input1 >= 0 && g->input2 >= 0) {
                    if (val[g->input1] == 1 || val[g->input2] == 1) { result = 1; ready = 1; }
                    else if (val[g->input1] == 0 && val[g->input2] == 0) { result = 0; ready = 1; }
                }
                break;
            case XOR:
                if (g->input1 >= 0 && g->input2 >= 0 && val[g->input1] >= 0 && val[g->input2] >= 0) {
                    result = val[g->input1] ^ val[g->input2]; ready = 1;
                }
                break;
            case NAND:
                if (g->input1 >= 0 && g->input2 >= 0) {
                    if (val[g->input1] == 0 || val[g->input2] == 0) { result = 1; ready = 1; }
                    else if (val[g->input1] == 1 && val[g->input2] == 1) { result = 0; ready = 1; }
                }
                break;
            case NOR:
                if (g->input1 >= 0 && g->input2 >= 0) {
                    if (val[g->input1] == 1 || val[g->input2] == 1) { result = 0; ready = 1; }
                    else if (val[g->input1] == 0 && val[g->input2] == 0) { result = 1; ready = 1; }
                }
                break;
            default: break;
            }
            if (ready) { val[i] = result; changed = 1; }
        }
    }
    BooleanCircuit* cpc = circuit_create(n + 10);
    int* new_id = (int*)malloc((size_t)n * sizeof(int));
    assert(new_id);
    for (int i = 0; i < n; i++) new_id[i] = -1;
    int* order = (int*)malloc((size_t)n * sizeof(int));
    assert(order);
    circuit_topological_order(c, order);
    for (int idx = 0; idx < n; idx++) {
        int i = order[idx];
        if (val[i] >= 0) {
            new_id[i] = circuit_add_gate(cpc, val[i] ? CONST1 : CONST0, -1, -1);
            continue;
        }
        const Gate* g = &c->gates[i];
        if (g->type == MAJ || g->type == THR) {
            int* fids = (int*)malloc((size_t)g->fanin * sizeof(int));
            assert(fids);
            int valid = 1;
            for (int j = 0; j < g->fanin; j++) {
                if (new_id[g->fanin_ids[j]] < 0) { valid = 0; break; }
                fids[j] = new_id[g->fanin_ids[j]];
            }
            if (valid) new_id[i] = circuit_add_maj_gate(cpc, g->type, g->fanin, fids);
            else new_id[i] = circuit_add_gate(cpc, CONST0, -1, -1);
            free(fids);
        } else {
            int a = (g->input1 >= 0) ? new_id[g->input1] : -1;
            int b = (g->input2 >= 0) ? new_id[g->input2] : -1;
            new_id[i] = circuit_add_gate(cpc, g->type, a, b);
        }
    }
    int* outs = (int*)malloc((size_t)c->n_outputs * sizeof(int));
    assert(outs);
    for (int i = 0; i < c->n_outputs; i++) outs[i] = new_id[c->output_ids[i]];
    circuit_set_output(cpc, outs, c->n_outputs);
    free(outs); free(order); free(new_id); free(val);
    return cpc;
}
"""
W('circuit_complexity.c', content)
