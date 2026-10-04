# Pattern justifications: Strategy and Visitor (Person 5 – Lee)

Subsystem: **routing, assignment and approval rules** (Strategy) and **monitoring and reporting** (Visitor).
Mini class diagram: `docs/diagrams/lee/mini-uml-visitor-strategy.jpg`
Prototype: `src/` (see "Code" at the end), tests: `make test_vs`

---

## 1. Strategy – assignment and approval rules

### Design problem
TIVIDY is generic, so it cannot hard-code *who* gets a piece of work or *how* work is approved. One organisation gives a task to whoever has the right role (e.g. only a Manager may approve a budget), another gives it to whoever is least busy, and another may add its own rule later. Approval varies the same way: some work needs one approver's sign-off, other work must pass several levels, and one "no" sends it back. If these rules were written inside `WorkItem`, every new organisation would mean editing the core work-item classes and adding another branch to an `if/else` chain (forbidden by rule 6 of the brief).

Workflow research names these as separate, recurring "resource patterns": role-based distribution and shortest-queue (least workload) distribution appear across workflow products as configurable allocation rules rather than engine code (Russell et al., 2005).

### GoF participants

| GoF role | Assignment rules | Approval rules |
|---|---|---|
| **Strategy** | `AssignmentStrategy` – `choose(item, candidates)` | `ApprovalStrategy` – `decide(votes)` |
| **ConcreteStrategy** | `RoleBasedAssignment`, `LeastWorkloadAssignment` | `SingleApproval`, `MultiLevelApproval` |
| **Context** | `WorkAllocator` | `ApprovalRequest` |
| Supporting class | `Participant` (name, role, active task count) | `Decision` enum (PENDING, APPROVED, REJECTED) |

### How the participants collaborate
1. A `WorkAllocator` is created with one `AssignmentStrategy` (it owns it) and a list of `Participant`s.
2. When a work item becomes Available, the engine calls `allocator.allocate(item)`.
3. The allocator asks its strategy `choose(item, participants)`. The strategy only *picks* a person; it changes nothing.
4. If someone was chosen, the allocator calls `item.assign()`. The **State** pattern (Rati) decides whether that transition is legal. Only if it succeeds does the allocator record the assignee and increase that participant's workload.
5. The rule can be swapped at runtime with `setStrategy()`, e.g. when nobody suitable exists the engine escalates and switches to `RoleBasedAssignment("Manager")` (Activity Diagram 1b).
6. When work is submitted, an `ApprovalRequest` is created for the item with an `ApprovalStrategy`. Each `vote()` is passed to `strategy->decide(votes)`. On APPROVED the request calls `item->complete()`; on REJECTED it calls `item->reject()`; if the deadline passes while still PENDING, `deadlinePassed()` calls `item->escalate()`. All three go through State, and through any Decorators (e.g. `ValidationDecorator` can still block `complete()`).

### What varies
- The rule for choosing a participant (role, workload, or a future data-based rule).
- The rule for turning approvers' votes into a decision (one approver, N levels, or a future majority vote).

Adding a new rule = one new subclass. `WorkItem`, the State classes, `WorkAllocator` and `ApprovalRequest` do not change.

### Why Strategy is appropriate
The brief says assignment and approval "should not require the fundamental work-item classes to be rewritten each time the process changes". These are *families of interchangeable algorithms* with the same inputs and output, chosen per organisation or per workflow definition. That is exactly Strategy's intent. State would be the wrong fit: the rule does not change because the work item changes state, it changes because a *different organisation* configured the workflow differently. Keeping the rule outside the work item also keeps the work item free of people data, so the same definition can be reused by organisations with different staff.

---

## 2. Visitor – monitoring and reporting

### Design problem
A running workflow is a tree of `Stage`s and `Task`s (Composite – Tshidi). Managers need different views of that tree: how far along the instance is, which tasks are stuck (escalated or rejected) and in which stage, and later workload or overdue reports. The brief warns against "placing every possible reporting operation inside the core model". If each report were a method on `WorkItem`, every new report would mean editing `WorkItem`, `Task`, `Stage` and every decorator.

### GoF participants

| GoF role | TIVIDY class |
|---|---|
| **Visitor** | `WorkItemVisitor` – `visitTask(Task&)`, `visitStage(Stage&)` |
| **ConcreteVisitor** | `ProgressReportVisitor` (task counts per state, % complete), `EscalationReportVisitor` (escalated/rejected tasks and their stage) |
| **Element** | `WorkItem` – `accept(WorkItemVisitor&)` (pure virtual) |
| **ConcreteElement** | `Task`, `Stage` |
| **ObjectStructure** | The work tree of a `WorkflowInstance`, walked by `DepthFirstIterator` |

### How the participants collaborate
1. A report is created, e.g. `ProgressReportVisitor report;`
2. `report.visitAll(*instance.getRoot())` asks the root for an iterator (**Iterator** – Tshidi) and, for each item, calls `item->accept(report)`.
3. Each item calls back the method for its own kind: a `Task` calls `visitTask(*this)`, a `Stage` calls `visitStage(*this)` (double dispatch). No type checks or casts are needed.
4. A decorated item (**Decorator** – Rati) forwards `accept()` to the item it wraps, so a prioritised, audited task is still reported as a task.
5. The visitor accumulates its results (counts, flagged items) and the caller reads them afterwards (`summary()`, `getFlagged()`).

`Stage::accept()` deliberately does *not* recurse into its children. The Iterator already owns traversal, so the two patterns do not both walk the tree.

### What varies
The set of reports. A new report (e.g. workload per participant, overdue items) is one new `ConcreteVisitor`; no element class changes. The element hierarchy itself (Task, Stage) is stable, which is the condition under which Visitor is the right choice.

### Why Visitor is appropriate
The tree's *kinds* of item rarely change, but the *operations* run over it keep growing as monitoring needs grow. Visitor puts each report's logic in one class instead of scattering it across Task and Stage, and it treats tasks and stages differently (tasks count toward progress, stages provide grouping) without manual type checks. It also collaborates naturally with the patterns already in the design: Composite provides the structure, Iterator provides traversal, and Decorator stays transparent.

Trade-off we accept: adding a new kind of work item (a new ConcreteElement) would require a new `visitX()` method on every visitor. That is acceptable because the work-item kinds are fixed by the Composite design, while reports are the part expected to grow.

---

## Changes to shared code (additive only)
To support Visitor, one method was added to existing classes. No existing lines were changed:
- `WorkItem.h`: `virtual void accept(WorkItemVisitor& visitor) = 0;`
- `Task`, `Stage`: `accept()` calls `visitTask` / `visitStage`
- `WorkItemDecorator`: `accept()` forwards to the wrapped item

Strategy needed **no** changes to existing classes. It uses the existing `assign()`, `complete()`, `reject()` and `escalate()`.

## Code
New: `WorkItemVisitor`, `ProgressReportVisitor`, `EscalationReportVisitor`, `Participant`, `AssignmentStrategy`, `RoleBasedAssignment`, `LeastWorkloadAssignment`, `WorkAllocator`, `ApprovalStrategy`, `SingleApproval`, `MultiLevelApproval`, `ApprovalRequest`, `visitor_strategy_test.cpp`.
Run: `cd src && make clean && make test_vs` (24 checks). `make run` and `make test_cm` still pass.

## References
- Gamma, E., Helm, R., Johnson, R. and Vlissides, J. (1994) *Design Patterns: Elements of Reusable Object-Oriented Software*. Addison-Wesley. (Strategy, pp. 315–323; Visitor, pp. 331–344.)
- Russell, N., van der Aalst, W.M.P., ter Hofstede, A.H.M. and Edmond, D. (2005) "Workflow Resource Patterns: Identification, Representation and Tool Support". In *Advanced Information Systems Engineering (CAiSE 2005)*, LNCS 3520, Springer, pp. 216–232. (Role-based distribution and shortest-queue allocation as configurable rules.)
