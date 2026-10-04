# Team contribution statement (draft – every member must confirm their line)

The team agreed the work split on a group call on 1 October 2026. Each member owns two GoF patterns (design, justification, mini class diagram and a small C++ prototype with tests) plus an equal share of the PDF deliverables. All work was done on feature branches and merged into `dev` through pull requests. The team agreed that anything merged into `dev` must compile and pass its tests.

- **Matshidiso** – Composite and Iterator: the shared core (`WorkItem`, `Task`, `Stage`, `WorkflowDefinition`, `WorkflowInstance`, `DepthFirstIterator`) that every other pattern builds on. Sequence Diagram 2 and Activity Diagram 3 (scenario).
- **Rati** – State (work-item lifecycle: Created, Available, Assigned, InProgress, Completed, Rejected, Escalated, Cancelled, with a rework retry limit) and Decorator (Priority, Validation, Auditing). State Diagram. Merged every member's classes into the master UML Class Diagram.
- **Melaney** – Command and Memento (`TransitionCommand`, `CommandHistory` with undo, redo and rollback; deep `WorkItemMemento` snapshots). Created and manages the GitHub repository, issues and project board. Sequence Diagram 1. Design decisions and revision log.
- **Stephen** – Observer and Adapter (events and triggers, external system integration). Scenario description and Activity Diagram 2.
- **Lindelwe (Lee) Mabaleka** – Strategy (`WorkAllocator`, `AssignmentStrategy`, `ApprovalRequest`, `ApprovalStrategy` with role-based, least-workload, single and multi-level rules) and Visitor (`WorkItemVisitor`, progress and escalation reports over the work tree). Activity Diagram 1 (generic workflow execution, with its called activity). Assembled the final PDF and wrote this statement.

Everyone contributed at least one research reference, and every member reviewed the integrated design so that each of us can explain the whole system.

Signed (all members agree): ____________ · ____________ · ____________ · ____________ · ____________
