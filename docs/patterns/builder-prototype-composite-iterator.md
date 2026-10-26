# Builder, Prototype, Composite and Iterator (owner: Matshidiso)

## Builder (Creational)

**Problem.** A process used to be made by hand: create a stage, add a task, wrap
it in decorators, add it to the stage, repeat. The code was long, easy to get
wrong (a task added to the wrong stage, a decorator forgotten) and had to be
rewritten for every industry.

| GoF role | TIVIDY class |
|---|---|
| Builder | `WorkflowBuilder` |
| ConcreteBuilder | `DefinitionBuilder` (builds a `WorkflowDefinition`), `OutlineBuilder` (builds a text outline) |
| Director | `WorkflowDirector`, `SupportDirector` (industry 1), `AdmissionsDirector` (industry 2) |
| Product | `WorkflowDefinition` and its `Stage`/`Task` tree; the outline string |

**Collaboration.** The TUI picks a Director for the chosen industry and a
builder, then calls `director.construct(builder)`. The Director sends steps
(`beginStage`, `addTask`, `withPriority`, `withValidation`, `withAudit`,
`endStage`). The client asks the builder for the result with `getResult()`.

**What varies.** The industry recipe (a new Director per industry) and the
product (a new ConcreteBuilder per kind of output). Neither change touches the other.

**Why Builder.** A workflow is a complex object made of parts in a set order.
Builder keeps the construction steps the same while the recipe and the product
change, so adding the second industry only needs one new Director.

## Prototype (Creational)

**Problem.** Every running case needs its own copy of the process tree. If two
cases shared one tree, completing a task in one case would complete it in all.

| GoF role | TIVIDY class |
|---|---|
| Prototype | `WorkItem` (`clone()`) |
| ConcretePrototype | `Task`, `Stage`, `PriorityDecorator`, `ValidationDecorator`, `AuditingDecorator` |
| Client | `WorkflowInstance` |

**Collaboration.** The `WorkflowInstance` constructor calls `clone()` on the
definition's root. `Stage::clone()` clones each child, and each decorator
clones the item it wraps, so the whole tree is copied. The copy starts in
Created with no retries used and no observers.

**What varies.** The kind of item being copied. The instance never needs to
know whether it copies a task, a stage or a decorated item.

**Why Prototype.** The definition is built once (by the Builder) and then
copied cheaply for every case, which gives instance isolation.

## Composite (Structural)

| GoF role | TIVIDY class |
|---|---|
| Component | `WorkItem` |
| Leaf | `Task` |
| Composite | `Stage` |
| Client | `WorkflowInstance`, visitors, the TUI |

A process is a tree: stages hold tasks and other stages. Clients treat a single
task and a whole stage the same way through `WorkItem`.

## Iterator (Behavioural)

| GoF role | TIVIDY class |
|---|---|
| Iterator | `WorkItemIterator` |
| ConcreteIterator | `DepthFirstIterator` |
| Aggregate | `WorkItem` (`createIterator()`) |
| ConcreteAggregate | `Stage`, `Task` |

Reports and the TUI walk the whole tree without knowing how it is stored.
`DepthFirstIterator` visits a stage before its children.

## Tests
`make test_builder` and `make test_composite` (doctest).
