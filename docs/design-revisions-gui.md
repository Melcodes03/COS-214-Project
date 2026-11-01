# Design revisions: final GUI round

Four changes made after the GUI review. Each follows the report's format: the problem,
what the design did before, what changed, why, and which diagrams to update.

---

## Revision A: sign-offs come from a higher level

**Problem.** A sign-off was recorded as the holder's own vote, so the person who did the work
approved it. Other people could only approve from the "Everyone" view, with no name attached.

**Before.** `doSignoff()` checked that the caller held the task and voted as the holder.

**Change.** A new Strategy, `ApproverPolicy`, decides who may sign:
- `HigherLevelApprover` (the default): a different person at a higher level. Work done at the
  manager level may be signed by the manager.
- `AnyOtherPersonApprover` and `HolderMayApprove` are the alternatives.

`SingleApproval` / `MultiLevelApproval` still count the votes. `SignoffCommand` records who
signed, and Undo removes that vote. Each person signs a round once. Viewing as the manager
(Naledi Khumalo, Busisiwe Moloi) gives her the manager buttons under her own name.

**Data change.**
- "Verify fix" needs 2 sign-offs (Engineer + Support Manager), so MultiLevelApproval is used.
- "Issue offer of admission" is prepared by a Faculty Reviewer and signed by the Registrar.

**Why.** An approval is only meaningful if someone other than the worker gives it. Keeping it a
Strategy makes the rule a one-line swap.

**Diagrams.**
- Class diagram: `ApproverPolicy` and its 3 ConcreteStrategies, used by `TividyService`.
- Strategy pattern diagram.
- State diagram: no change.

---

## Revision B: the Reviewer at the end of the escalation chain

**Problem.** Chain of Responsibility guarantees nothing about being handled. `ManagerHandler`
gave work to the manager even when she was full, and stuck work was never looked at again.

**Before.** General -> Technical -> Engineer -> Manager (always takes it).

**Change.** General -> Technical -> Engineer -> Manager -> **Reviewer**.
- The manager takes work only when she has room. Otherwise `ReviewHandler` applies its
  `ReviewAction` (Strategy, default `FlagForReview`): the work waits, marked "Under review".
- A review sweep re-offers flagged work, and work unchanged for 6 steps, from the bottom of the
  chain. It is found by `StalledWorkVisitor` (Visitor), which walks the tree with the Iterator.
- New "Stalled work" report and "Review stalled work" button.
- The manager is still the last level a person can pass work to.

**Why.** Nothing can fall off the end of the chain unseen, and nothing piles onto a full manager.

**Diagrams.**
- Class diagram: `ReviewHandler`, `ReviewAction` (+2), `StalledWorkVisitor`.
- CoR sequence diagram: add the Reviewer.
- Activity Diagram 1b ("Process work item"): add "everyone full -> under review -> re-offered".

---

## Revision C: default scenarios and Reset to defaults

**Problem.** The first look depended on whatever was saved from earlier testing, and
"Start fresh" left an empty screen. A demo could not be repeated from the same start.

**Before.** Startup only replayed the save file.

**Change.**
- With nothing open, the server opens the default scenarios, like a `main()`: 3 support cases
  and 3 admissions applications, untouched. They are built through the same
  `WorkflowDirector` / Builder, and each case is a clone (Prototype) of its definition.
- "Reset to defaults" returns to exactly that start. "Clear everything" empties it.

**Why.** It gives a clean, repeatable demo start.

**Diagrams.** None. This is a Facade addition (`TividyService::loadDefaults / resetToDefaults`).

---

## Revision D: Run all tickets

**Problem.** Showing many cases at once needed hundreds of clicks. The engine had no idea of a
desk's capacity: no running slots, no waiting queue.

**Before.** Cases only moved when someone clicked.

**Change.**
- `BatchRunner` (Mediator) opens N tickets, keeps `slots` running and the rest waiting.
- Each tick it gives every running ticket one step. `RunPolicy` (Strategy) picks the step:
  - `HappyPathPolicy`: approve, complete, start, assign.
  - `WithDeclinesPolicy`: every other task is sent back once.
- A finished ticket frees its slot, and the next waiting one starts.
- A ticket stuck for 3 ticks goes to the Reviewer.
- The cases never know about each other or the runner: they are reached through the
  `CaseDesk` interface, which `TividyService` implements.
- Every step uses the same call as a button press, so it is a real Command in the case's
  history. The runner can only pick actions the engine allows; it never sets a task to
  Completed directly.
- The page drives time with ticks (no thread), so a run is deterministic, testable and saved.
- The GUI has a run bar (Pause, Resume, Finish instantly, Stop) and "Waiting" / "Auto-running"
  chips. Any manual click pauses the run. A run summary (completed, failed, escalations,
  reworks, longest wait) shows on the Dashboard and in Reports.

**Why.** It demonstrates capacity, escalation, rework and independent instances at scale
without hand-clicking, while staying honest to the engine.

**Diagrams.**
- Class diagram: `BatchRunner`, `CaseDesk`, `RunPolicy` (+2).
- New sequence diagram "Run all tickets": page -> TividyService.tick -> BatchRunner ->
  CaseDesk.step -> actHere -> Command.

---

## Pattern count after these changes

The new roles are **Mediator** (`BatchRunner`) and 3 new Strategy families
(`ApproverPolicy`, `ReviewAction`, `RunPolicy`). Chain of Responsibility gains a new handler
(`ReviewHandler`), and Visitor a new visitor (`StalledWorkVisitor`). Update the GoF mapping
table and the colour legend accordingly.

## Tests added

`src/web_service_test.cpp` (doctest):
- **Sign-offs:** a worker cannot approve their own work; a higher level can; nobody signs twice;
  the manager console signs as the manager; undo removes the vote.
- **Reviewer:** stuck work is flagged and re-offered with no loop.
- **Defaults:** the 6 untouched cases; reset gives the identical start.
- **Run all tickets:** 8 running / 7 waiting, a waiting ticket is promoted, all 15 end Completed
  with real sign-offs and no deadlock, the run with declines finishes, Stop, bad input, and a
  run replayed exactly after a restart.
