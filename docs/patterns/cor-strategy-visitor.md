# GoF participant mapping: Chain of Responsibility, Strategy and Visitor (Lee)

Final project version. The Practical 6 write-up is kept in
`docs/diagrams/visitor-strategy.md` as design history.

Tests for everything below: `cd src && make test_chain` (doctest) and `make test_vs`.

---

## Chain of Responsibility – who takes a task when a worker declines or does not respond

| GoF role | TIVIDY class | What it does |
|---|---|---|
| **Handler** | `AssignmentHandler` | Holds the next level (`next`, `setNext()`, `getNext()`). `handle(item, candidates)` picks a worker or calls `passOn()`. `covers(worker)` and `levelOf(worker)` tell which level a worker is in. |
| **ConcreteHandler** | `LevelHandler` | One level of workers with the same role, for example General Assistant, Technical Assistant, Engineer. Takes the task if its priority is not above `maxPriority` and the level has a worker. Uses its own `AssignmentStrategy` to pick which worker. Otherwise passes the task on. |
| **ConcreteHandler** | `ManagerHandler` | The last link. Always gives the task to the manager, so no task is left without an owner. |
| **Client** | `WorkAllocator` | Only knows the first level (`setChain()`). `allocate()` sends the task into the chain. `escalate()` (no response) and `reassign()` (declined) send it to the level above the one that has it now. It remembers the level in `holder`. |
| Used by | `ReportScreen` | The escalation screen calls `escalateTask()` / `reassignTask()` and prints who has the task now. |

How it runs:
1. A task becomes Available. `WorkAllocator::allocate()` calls `chain->handle(task, participants)`.
2. Each level checks the task's priority and whether it has a worker. If yes, its strategy picks the worker. If no, it calls `passOn()` and the next level tries.
3. The worker declines (`Participant::decline()`), or does not respond (`WorkAllocator::escalate()` moves the task Assigned → Escalated).
4. `reassign()` / `escalate()` looks up the level that had the task and calls `handle()` on the next level up.
5. If the manager also declines, there is no level above, so the task stays Available and the screen says so.

Industry differences live only in how the chain is built (roles and priorities), not in these classes. The insurance or university industry builds its own levels with the same three classes.

---

## Strategy – how one worker is picked (rewritten)

| GoF role | TIVIDY class | What it does |
|---|---|---|
| **Strategy** | `AssignmentStrategy` | `choose(item, candidates)` returns one worker, or nullptr. It changes nothing. |
| **ConcreteStrategy** | `RoleBasedAssignment` | Picks the first worker with a given role. |
| **ConcreteStrategy** | `LeastWorkloadAssignment` | Picks the worker with the fewest active tasks. |
| **Context** | `LevelHandler` | Each level owns one strategy and uses it on its own workers. Two levels can pick workers in different ways. |
| **Context** | `WorkAllocator` | When no chain is set, the allocator uses its own strategy on everyone. |
| **Strategy** | `ApprovalStrategy` | `decide(votes)` returns PENDING, APPROVED or REJECTED. |
| **ConcreteStrategy** | `SingleApproval`, `MultiLevelApproval` | One approver, or several levels where one "no" rejects. |
| **Context** | `ApprovalRequest` | Collects votes and calls `complete()`, `reject()` or `escalate()` on the item. |

What changed since Practical 6: escalation used to swap the allocator's strategy to `RoleBasedAssignment("Manager")`. Now the Chain of Responsibility decides which **level** takes the task, and Strategy only decides which **worker** in that level. The two patterns split the job.

---

## Visitor – reports over the workflow tree

| GoF role | TIVIDY class | What it does |
|---|---|---|
| **Visitor** | `WorkItemVisitor` | `visitTask(Task&)`, `visitStage(Stage&)`, and `visitAll(root)` which walks the tree with the Iterator. |
| **ConcreteVisitor** | `ProgressReportVisitor` | Counts tasks per state, % complete overall, and per stage (`stageSummary()`). `reset()` lets the same report run again after every action. Checks Completed with `CompletedState::instance()`, not by comparing names. |
| **ConcreteVisitor** | `EscalationReportVisitor` | Lists escalated and rejected tasks and the stage they are in. Checks the state objects `EscalatedState::instance()` and `RejectedState::instance()`. |
| **Element** | `WorkItem` | `accept(WorkItemVisitor&)` (pure virtual). |
| **ConcreteElement** | `Task`, `Stage` | `accept()` calls `visitTask(*this)` / `visitStage(*this)` (double dispatch). `Stage::accept()` does not go into its children; the Iterator does the walking. |
| **ObjectStructure** | the tree of a `WorkflowInstance` | Walked by `DepthFirstIterator`. Decorators forward `accept()` to the item they wrap. |
| Used by | `ReportScreen` | `showProgress()` and `showEscalations()` print the reports in the TUI. |
