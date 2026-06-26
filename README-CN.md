# Mini Circuit Complexity（迷你电路复杂性）

**从零开始、零依赖的 C 语言实现**，涵盖电路复杂性理论——将布尔电路作为计算模型，研究下界证明技术以及电路类层次结构（AC⁰, ACC⁰, TC⁰, NC, P/poly）。每个模块对应 MIT、斯坦福、伯克利的博士级课程，将里程碑定理（Håstad 开关引理、Razborov-Smolensky 多项式方法、自然证明屏障）转化为可运行的 C 代码。

## 子模块总览

| 子模块 | 主题 | 参考课程 |
|-----------|--------|-------------|
| [mini-ac0-nc-poly-hierarchy](mini-ac0-nc-poly-hierarchy/) | AC⁰/NC¹/TC⁰/P/poly 层次结构、分离性、深度-规模权衡、完全问题 | Stanford CS358, MIT 6.841 |
| [mini-acc0-circuits](mini-acc0-circuits/) | ACC⁰ = AC⁰ + MODₘ 门、多项式方法、Williams 的 NEXP ∉ ACC⁰ 突破 | Stanford CS358, MIT 6.841 |
| [mini-boolean-circuits-model](mini-boolean-circuits-model/) | 布尔电路 DAG 模型、门类型、扇入/扇出、电路分析与综合 | MIT 6.841, UC Berkeley CS278 |
| [mini-circuit-sat-complexity](mini-circuit-sat-complexity/) | Circuit-SAT（首个 NP 完全问题）、Williams 的 SAT↔下界程序、电路类描述符 | MIT 6.841, Stanford CS358 |
| [mini-hastad-lower-bounds](mini-hastad-lower-bounds/) | Håstad（1986）最优 AC⁰ 下界、PARITY ∉ AC⁰、exp(Ω(n^(1/(d-1)))) 规模下界 | Stanford CS358, MIT 6.845 |
| [mini-monotone-circuit-lower](mini-monotone-circuit-lower/) | Razborov 单调电路下界、CLIQUE 超多项式下界、向日葵引理、逼近方法 | MIT 6.841, UC Berkeley CS278 |
| [mini-natural-proofs-barrier](mini-natural-proofs-barrier/) | Razborov-Rudich 屏障（1994）、三大屏障：相对化、自然证明、代数化 | Stanford CS358, MIT 6.845 |
| [mini-nc-hierarchy](mini-nc-hierarchy/) | Nick 类 NCⁱ = 多项式规模对数多项式深度、NC¹ 完全问题（PARITY、公式求值）、NC² 完全问题（行列式、上下文无关语言） | MIT 6.841, UC Berkeley CS278 |
| [mini-razborov-smolensky](mini-razborov-smolensky/) | Razborov-Smolensky 多项式方法、MAJORITY ∉ AC⁰[p]（素数 p≠2）、GF(p) 多项式逼近 | Stanford CS358, MIT 6.841 |
| [mini-switching-lemma](mini-switching-lemma/) | Håstad 开关引理（1994 年 Gödel 奖）、随机限制下的 DNF→CNF 转换、深度压缩 | Stanford CS358, MIT 6.845 |
| [mini-tc0-threshold-circuits](mini-tc0-threshold-circuits/) | TC⁰ 阈值电路、MAJORITY 门、排序/乘法/除法在 TC⁰ 中、TC⁰ vs NC¹ 开放问题 | Stanford CS358, MIT 6.841 |

## 设计理念

- **零外部依赖** — 纯 C（C99/C11），仅使用 `libc` 和 `libm`
- **模块自包含** — 每个目录自带 `Makefile`、`include/`、`src/`、`examples/`、`demos/`、`tests/`
- **理论到代码的映射** — 每个模块包含 `docs/` 目录，内有课程对齐说明和定理引用
- **里程碑定理可执行** — Håstad 开关引理、Razborov-Smolensky 多项式方法、Razborov-Rudich 自然证明屏障均可运行验证，而非仅作陈述

## 构建方式

每个模块相互独立。进入模块目录后运行：

```bash
cd mini-ac0-nc-poly-hierarchy
make all    # 构建全部
make test   # 运行测试
```

需要 **GCC** 和 **GNU Make**。

## 项目结构

```
mini-circuit-complexity/
├── mini-ac0-nc-poly-hierarchy/   # AC⁰/NC¹/TC⁰/P/poly 层次结构、分离性、深度-规模曲线
├── mini-acc0-circuits/           # ACC⁰ = AC⁰ + MODₘ 门、多项式方法、Williams 定理
├── mini-boolean-circuits-model/  # 布尔电路 DAG、门类型、电路分析、电路综合
├── mini-circuit-sat-complexity/  # Circuit-SAT NP 完全性、Williams 程序
├── mini-hastad-lower-bounds/     # Håstad 最优 AC⁰ 下界、PARITY ∉ AC⁰
├── mini-monotone-circuit-lower/  # Razborov 单调电路下界、CLIQUE、向日葵引理
├── mini-natural-proofs-barrier/  # Razborov-Rudich 屏障、相对化、代数化
├── mini-nc-hierarchy/            # Nick 类、NC¹ 完全问题、NC² 完全问题
├── mini-razborov-smolensky/      # 多项式方法、MAJORITY ∉ AC⁰[p]、GF(p) 逼近
├── mini-switching-lemma/         # Håstad 开关引理、随机限制、深度压缩
└── mini-tc0-threshold-circuits/  # TC⁰ 阈值电路、MAJORITY、TC⁰ vs NC¹ 开放问题
```

## 许可证

MIT
