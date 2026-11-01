# TIVIDY web API

The web interface is a thin layer over the C++ engine. `src/web_main.cpp` moves JSON
between HTTP and `TividyService` (a Facade, `src/TividyService.h`); the service drives the
real engine classes. The page in `web/index.html` only calls the endpoints below.

Run it with `cd src && make run_web`, then open <http://localhost:8080/>
(`./tividy_web --port 9000 --web ../web` to change the port or the front-end folder).

Every response is JSON. Any number of cases can be open at once, and none is ever closed
behind the user's back. Each is an independent `WorkflowInstance` with its own data, people
and history; starting a case makes it the current one, and `POST /api/case` switches back.

**Saving.** The server writes every change (cases, workflows, actions, undo/redo, case
switches) to `tividy-log.jsonl` in the folder it runs from, and on start replays that log
through the same calls, so the engine rebuilds exactly the same state.
`./tividy_web --fresh` starts empty, `--no-save` turns saving off, `--log FILE` picks the file.

**Workspaces.** Every workflow belongs to an organisation and a workspace
(Organisation -> Workspace -> Workflow -> Case). The ready-made ones are
"Pretoria Software Co. / Customer Support" and "University of Pretoria / Admissions Office";
`/api/workflow` takes optional `organisation` and `workspace` fields.

| Method | Endpoint | Purpose |
|---|---|---|
| GET | `/api/industries` | The industries (two built in, plus any built in the app), with stages, escalation levels and sample cases |
| POST | `/api/industry` | Load an industry and start a case |
| POST | `/api/workflow` | Define a new workflow (levels, people, stages) and start a case on it |
| GET | `/api/state` | Full snapshot of the running case |
| POST | `/api/action` | `allocate`, `start`, `complete`, `approve`, `reject`, `decline`, `escalate`, `cancel` on a task |
| POST | `/api/case` | `{ "index": 0 }`: make that open case the current one |
| GET | `/api/overview` | Every open case, every task, the staff, the workspaces and the activity log (dashboard, My work, alerts, history, reports) |
| POST | `/api/reset` | Close every case and forget workflows built in the app ("Start fresh") |
| POST | `/api/undo`, `/api/redo` | Undo or redo the last action |
| POST | `/api/rollback` | Undo everything after a history marker |
| GET | `/api/report/progress`, `/api/report/escalations` | Text from the Visitor reports |

## Responses that change the case

`/api/industry`, `/api/workflow`, `/api/action`, `/api/undo`, `/api/redo` and `/api/rollback` all answer with

```json
{ "ok": true, "message": "Task S1-T1 allocated to Thabo Nkosi.",
  "events": ["S1-T1 (Log support case): Available -> Assigned"],
  "state": { "...": "full snapshot, see below" } }
```

`ok: false` means the engine refused the request (for example `Cannot complete S1-T1
(state: Assigned).`). The state in that response is unchanged and nothing is added to the
history. Malformed requests get HTTP 400 with the same shape.

`events` are the state changes the request caused, including the work the engine opened
by itself (finishing a task opens the next one). They are computed by comparing the
tree before and after, so undo, redo and rollback report them too.

## POST /api/industry

```json
{ "industryId": 2, "caseIndex": 0, "data": { "score": 65 } }
```

`caseIndex` picks a sample case (default 0). `data` is optional and replaces values of
that case. `/api/industries` lists each industry's `decisionFields`, the case details its
decision stages look at: `severity` in Software Customer Support (`critical` gets an
incident report, anything else a satisfaction survey) and `score` in University
Admissions (70 or more: offer, 55 or more: waiting list, otherwise rejection).
Starting the same sample twice gives the second case its own number (`ADM-2001-2`).

## POST /api/workflow

Makes the engine generic from the GUI: any process can be described without new code.

```json
{ "name": "Hospital admission",
  "levels": ["Nurse", "Doctor", "Ward Manager"],
  "people": [ { "name": "Thandi Mokoena", "role": "Nurse", "capacity": 3 },
              { "name": "Dr Priya Naidoo", "role": "Doctor", "capacity": 2 },
              { "name": "Mrs Elsa Venter", "role": "Ward Manager", "capacity": 5 } ],
  "stages": [ { "name": "Triage", "mode": "sequential",
                "tasks": [ { "name": "Take vital signs", "role": "Nurse" } ] },
              { "name": "Tests", "mode": "parallel",
                "tasks": [ { "name": "Blood test", "role": "Nurse" },
                           { "name": "Review X-ray", "role": "Doctor" } ] } ] }
```

* `levels` is the escalation chain, lowest first; the last level is the manager.
* `people` is optional. Without it, two people are made up for every level and one for
  the manager level ("Assessor 1", "Assessor 2", "Claims Manager"). If given, every
  person's `role` must be a level, every level needs a person, and `capacity` is 1 to 20
  (default 3). Tasks never name a person: the escalation chain picks one.
* `mode` is `sequential` (default), `parallel` or `choice` (a decision: one branch runs).
  Up to 10 stages, 10 tasks per stage, 20 people; names must be unique.
* A decision needs `"decision": { "field": "amount", "value": 60000 }` (the case detail it
  looks at and its value for the first case). Each branch has
  `"when": { "op": ">=", "value": "50000" }` (ops `== != < <= > >=`); the first match
  runs, and exactly one branch has no `when` (the "otherwise" branch).
* `"signoffs": 1` (0 to 3) makes a task need approvals instead of Complete.
* The spec goes through the same `WorkflowDirector` and builders as the built-in
  industries. A task's priority is its level (1 = lowest), so the chain offers it to
  that level first.
* The answer is the usual change response plus `industryId`. A bad spec gets
  `ok: false` with a plain message (for example `Give the workflow a name.`) and nothing
  is added.

## POST /api/action

```json
{ "action": "allocate", "taskId": "S1-T1", "participantName": "Ayesha Patel", "caseId": "SC-1001" }
```

`caseId` is optional. With it, the action runs on that open case (used by "My work", which
lists tasks of every case) and the current case stays the same; the reply's `state` is still
the current case.

`participantName` is optional. The page never sends it with `allocate`: the escalation
chain (Chain of Responsibility) always picks the person. It is kept for the tests. With
the other actions it must match the person who holds the task.

* `allocate` on new work starts at the lowest escalation level. On work someone already
  held (after a decline) it moves up one level.
* `decline` gives the work back; from `InProgress` it becomes `Rejected` and may be reworked
  only `maxRetries` times.
* `decline` and `reject` pass the work straight up the escalation chain in the same
  step (one Undo takes back both). If nobody higher is free it goes back to the start of
  the chain; if nobody at all is free it waits.
* `approve` / `reject` are for tasks that need sign-offs (`signoffs` in the state) once
  they are `InProgress`. `ApprovalRequest` with `SingleApproval` or `MultiLevelApproval`
  (Strategy) decides; `ApprovalCommand` (Command) records each vote so it can be undone.
  `complete` is refused on these tasks.
* `escalate` hands the task to the next level up.
* `cancel` cancels the task, which fails the whole case. It is refused on work that has
  not opened yet (`Created`).
* At the last level (the manager) `decline` and `escalate` are refused, because there is
  nobody higher to pass the work to. The manager completes it or cancels the case. This
  is what stops the chain from freezing at the top.

Notifications (Observer + Adapter): every task is watched by a `ParticipantObserver` for
the manager. Escalations are sent by SMS (`SmsConnector` adapting `SmsGateway`); work sent
back or cancelled is sent by email (`EmailConnector` adapting `MailService`). Each one
also appears in `events` as `SMS to <manager>: ...` / `Email to <manager>: ...`.

## State snapshot

```json
{
  "loaded": true, "industry": "Software Customer Support", "industryId": 1,
  "instanceId": "SC-1001", "status": "Running", "failReason": "",
  "custom": false,
  "levels": ["General Assistant", "Technical Assistant", "Engineer", "Support Manager"],
  "caseData": { "customer": "Mamelodi Pharmacy", "...": "..." },
  "stages": [ { "id": "S1", "name": "Intake", "state": "InProgress", "mode": "sequential", "tasks": [
      { "id": "S1-T1", "name": "Log support case", "state": "Assigned",
        "assignedTo": "Thabo Nkosi", "requiredRole": "General Assistant", "priority": 1,
        "retryCount": 0, "maxRetries": 3, "skipped": false, "lastLevel": false,
        "signoffs": { "needed": 1, "given": 0 }, "when": "severity == critical",
        "validActions": ["start", "decline", "escalate", "cancel"] } ] } ],
  "participants": [ { "name": "Thabo Nkosi", "role": "General Assistant",
        "workload": 1, "capacity": 3, "available": true, "worklist": ["S1-T1"] } ],
  "history": { "canUndo": true, "canRedo": false, "undoCount": 1, "redoCount": 0,
        "marker": 1, "lastCommand": "Allocate: Log support case",
        "done": ["Allocate: Log support case"], "undone": [] },
  "notifications": [ { "channel": "SMS", "to": "Naledi Khumalo",
        "text": "'Log support case' (S1-T1) was escalated and needs someone more senior." } ],
  "openCases": [ { "index": 0, "id": "SC-1001", "industry": "Software Customer Support",
        "status": "Running", "current": true } ]
}
```

* `state` uses the engine's own names: `Created`, `Available`, `Assigned`, `InProgress`,
  `Completed`, `Rejected`, `Escalated`, `Cancelled`. Stages show `Created`, `InProgress`
  or `Completed`.
* `validActions` is answered by the engine itself: each action is tried on a throw-away
  copy of the task in its current state, so it can never disagree with the State pattern.
  It is empty for finished work and once the case has `Completed` or `Failed`.
* `skipped` is true for a branch of a choice stage that a finished stage did not take.
* `mode` is `sequential`, `parallel` or `choice` (a decision stage).
* `signoffs` appears only on tasks that need approvals; `when` only on decision branches
  (a branch with no `when` in a `choice` stage is the "otherwise" branch).
* `lastLevel` is true when the task is held at the top of the chain.
* `failReason` names the cancelled task once the case has `Failed`.
* `levels` is the escalation chain, lowest first; `custom` is true for a workflow built in
  the app.
* `history.marker` is the number to send to `/api/rollback` to return to this point.
* Before any industry is loaded, `/api/state` returns `{ "loaded": false, "industry": null }`.

## GET /api/overview

```json
{ "workspaces": [ { "organisation": "...", "workspace": "...", "workflows": ["..."] } ],
  "cases": [ { "index": 0, "number": 1, "id": "SC-1001", "industry": "...", "status": "Running",
               "progress": { "done": 2, "total": 9 }, "awaiting": 1, "pendingApprovals": 0,
               "active": 1, "stage": "Intake", "outcomes": [], "current": true, "...": "..." } ],
  "tasks": [ { "caseId": "SC-1001", "stage": "Intake", "stageMode": "sequential", "level": "General Assistant",
               "...": "the same fields as a task in /api/state" } ],
  "people": [ { "name": "Thabo Nkosi", "role": "...", "workflow": "...", "workload": 1,
                "completed": 3, "holding": [ { "caseId": "SC-1001", "taskId": "S1-T2" } ] } ],
  "activity": [ { "time": "21:04:11", "at": 1793300651, "caseId": "SC-1001", "kind": "assigned",
                  "text": "Task S1-T1 allocated to Thabo Nkosi.", "who": "Thabo Nkosi" } ],
  "currentCase": "SC-1001" }
```

Activity kinds: `case-opened`, `workflow-created`, `assigned`, `started`, `completed`,
`approval` (one sign-off of several), `approved`, `approval-pending`, `sentback`, `escalated`,
`cancelled`, `ready` (work that just opened), `case-completed`, `case-failed`, `sms`, `email`,
`undo`. Tasks that have not opened carry `waitingFor`, e.g. "the 'Intake' stage to finish".

## Reports

`{ "reportType": "progress", "content": "=== Progress: ... ===\n..." }`, meant for a `<pre>`.

## How the web layer maps onto the engine

| API | Engine |
|---|---|
| approve, reject | `ApprovalRequest` + `SingleApproval` / `MultiLevelApproval` (Strategy), `ApprovalCommand` (Command) |
| decline + pass on | one undoable step: `WorkerCommand` then `AllocationCommand` (reassign up the chain) |
| create workflow | `readCustomIndustry` checks the spec, then the same path as load industry |
| notifications | `ParticipantObserver` (Observer) sending through `EmailConnector` / `SmsConnector` (Adapter) |
| load industry | `DemoData` spec, `WorkflowDirector` + `DefinitionBuilder` (Builder), `WorkflowInstance` (clone of the definition), `WorkAllocator` with a `LevelHandler` chain (Chain of Responsibility, Strategy) |
| action | `AllocationCommand` / `WorkerCommand` / `TransitionCommand` wrapped in `ProgressCommand`, run by `CommandHistory` (Command) |
| validActions | `WorkItem::clone` + `restore` (Memento) tried against the State classes |
| undo, redo, rollback | `CommandHistory`, `WorkItemMemento` |
| reports | `ReportScreen` with `ProgressReportVisitor`, `EscalationReportVisitor` (Visitor) |

Engine additions made for the web layer: `ProgressCommand` (undo also reverses the
stages an action opened), `WorkAllocator::allocateTo` (manual assignment),
`WorkflowInstance::recomputeStatus` (a case runs again after an undo).

On Windows (MSYS2) the Makefile links `-lws2_32` for the web server automatically.
