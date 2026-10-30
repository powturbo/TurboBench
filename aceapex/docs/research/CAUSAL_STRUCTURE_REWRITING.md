# Суть ACEAPEX: rewriting of causal structure (зерно статьи N) — 2026-06-17

Точная формулировка найдена автором (Якив, 2026-06-17):
"ACEAPEX делает не декомпрессию, а relabeling/rewiring of a dependency DAG at encode time.
По духу рядом стоят graph rewriting, SSA/IR rewriting и event sourcing."
"ACEAPEX ближе не к архивированию, а к rewriting of causal structure."

КОНТРАСТ с аналогами:
- Graph rewriting (DPO, term rewriting): работает на абстрактных структурах, НЕ на байтовых потоках.
- SSA/IR rewriting: граф известен заранее (код). ACEAPEX: произвольные данные без схемы.
- Event sourcing: causal chain читается последовательно (replay). Никто не переписывает её при записи.
ПУСТОЕ МЕСТО: rewriting causal structure AT WRITE-TIME для параллельного read-time —
нет явно ни в одной из трёх областей. Это шире, чем "GPU LZ77 decoder".

СВЯЗЬ с измеренными фактами:
- chain flattening = конкретная реализация causal rewriting (граф зависимостей → literal-источник)
- absolute offsets = следствие полного разрыва causal chain между блоками
- 0.36ms seek = observable consequence of causal independence
- Цена: literal-пул раздут (39-80%) — trade-off causal independence vs compact representation

ЗЕРНО для будущего: применить causal structure rewriting к другим доменам, где
sequential causal chain создаёт bottleneck при parallel read:
- event logs / WAL (database transaction logs, каждое событие → предыдущее)
- blockchain (каждый блок → предыдущий hash, sequential by design)
- Git DAG (commit → parent, параллельный checkout невозможен без rewiring)
НЕ спешить — это материал статьи 3-4, после публикации статьи 2.
