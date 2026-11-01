# TIVIDY – Workflow Management System

COS 214 Final Project, University of Pretoria, 2026.

TIVIDY is one generic workflow engine written in C++11. The engine knows about
work items, stages, people, states and history, but nothing about any single
business. Each industry gives it its own roles, rules, data and order of steps.

---

## 1. What you need

| Tool | Version | Notes |
|---|---|---|
| g++ | 7 or newer (C++11) | Windows: MSYS2 / MinGW-w64 |
| make | GNU Make | Windows: use `mingw32-make` instead of `make` |
| valgrind | any | optional, Linux only, for memory checks |

The test framework, [doctest](https://github.com/doctest/doctest) v2.4.11
(MIT licence), is included as `src/doctest.h`, so there is nothing else to install.

## 2. Build

```bash
cd src
make clean     # do this after pulling: the Makefile does not track header changes
make
```

All code is built with `-std=c++11 -Wall -Wextra -pedantic`.

## 3. Run

TIVIDY has two interfaces. Both drive the same C++ engine.

**Web GUI (recommended for the demo)**

```bash
cd src
make run_web          # then open http://localhost:8080 in a browser
```

`./tividy_web --port 9000 --web ../web` changes the port. Pick a workflow and a sample
case (or build your own for any organisation), then press Assign, Start, Complete,
Approve / Reject, Decline, Escalate or Cancel on the tasks.

- **Dashboard, Cases, My work, Alerts, History, Reports** cover every open case; the
  **Case** page shows one case in detail.
- **Workspace** (top bar) switches between organisations' areas, e.g. an airport's
  Passenger Services; **View as** shows one person's work; **Simple / Advanced** hides or
  shows charts and expert options.
- It starts with the **default scenarios** (3 support cases, 3 admissions applications,
  untouched). History → **Reset to defaults** returns to exactly that start.
- **Run all tickets** (My work) opens e.g. 15 tickets, runs 8 at a time with the rest
  waiting, and works them through the engine step by step (Pause, Resume, Finish instantly).
- **Sign-offs** come from someone above the person who did the work; viewing as the
  manager (e.g. Naledi Khumalo) gives her manager buttons under her own name.
- Everything is saved to `src/tividy-log.jsonl` and restored on restart
  (`./tividy_web --fresh` starts again from the defaults).

The REST API is in `docs/web-api.md`.

**Text interface (TUI)**

```bash
cd src
make run_tui
```

## 4. Tests

Run everything with `make test_all` (stops at the first failure).

| Command | What it tests | Framework |
|---|---|---|
| `make test_web` | The web service: actions, chain, last level, cancel, sign-offs, decisions, several cases, building a workflow | doctest |
| `make test_workflow` | advance(): sequential, parallel and choice stages, the cancel rule, instance isolation | doctest |
| `make test_state` | Work-item lifecycle (State) and the decorators | doctest |
| `make test_tui` | The text interface, scripted | assert program |
| `make test_chain` | Chain of Responsibility, WorkAllocator escalation, progress and escalation reports, report screens | doctest |
| `make test_builder` | Builder, Directors, Prototype (clone and instance isolation), Industry 1 data | doctest |
| `make test_composite` | Composite and Iterator | doctest |
| `make run` | Composite, Iterator, State, Decorator, instance isolation | assert program |
| `make test_cm` | Command, Memento, worker commands, undo/redo, history | assert program |
| `make test_vs` | Visitor and Strategy | assert program |
| `make test_observer` | Observer | assert program |
| `make test_adapter` | Email and SMS adapters | assert program |

Memory check (Linux): `valgrind --leak-check=full ./chain_test`

Useful doctest options: `./chain_test --list-test-cases`, `./chain_test -tc="*declined*"`,
`./chain_test -s` (show every check).

## 5. The two industries

### Industry 1: Software customer support
A software company's support desk. A customer's support case is logged,
investigated and resolved by General Assistants, Technical Assistants and
Engineers, with a Support Manager as the last level.
- Completion path: the case is logged, investigated, fixed (the fix needs one
  sign-off), closed and followed up.
- Decision: Follow-up checks `severity`: a critical case gets an incident report,
  any other case a satisfaction survey.
- Exception paths: a worker declines or the fix is rejected, so the work moves up
  the escalation chain (at most 3 reworks); a cancelled task ends the whole workflow.

Five stages: Intake, Investigation, Resolution, Closure and Follow-up. Each task's
priority matches its escalation level (1 General Assistant, 2 Technical Assistant,
3 Engineer, 4 Support Manager).

### Industry 2: University admissions
A university admissions office. An application is registered, its documents and
identity are checked at the same time, it is assessed and scored, and the score
decides the outcome. Levels: Admissions Clerk, Verification Officer, Faculty
Reviewer, Registrar.
- Decision: score 70 or more gets an offer (needs two sign-offs), 55 or more the
  waiting list, otherwise a rejection notice. Only an offer leads to enrolment.
- Exception paths: as above (decline, rejection, escalation, cancel).

Both industries are plain data in `src/DemoData.cpp` (a `WorkflowSpec` per
industry). The one generic `WorkflowDirector` turns any spec into a workflow, so a
new industry needs no new engine code; the web GUI's "Build your own workflow" form
does exactly that at run time.

## 6. Design patterns

| Category | Pattern | Main classes |
|---|---|---|
| Creational | Builder | `WorkflowBuilder`, `DefinitionBuilder`, `OutlineBuilder`, `WorkflowDirector` (reads a `WorkflowSpec`) |
| Creational | Prototype | `WorkItem::clone()`, `WorkflowInstance` |
| Structural | Composite | `WorkItem`, `Task`, `Stage` |
| Structural | Decorator | `PriorityDecorator`, `ValidationDecorator`, `AuditingDecorator` |
| Structural | Adapter | `CommunicationAdapter`, `EmailConnector`, `SmsConnector` |
| Structural | Facade | `TividyService` (the one class the web server talks to) |
| Behavioural | Iterator | `WorkItemIterator`, `DepthFirstIterator` |
| Behavioural | State | `WorkItemState` and its 8 states |
| Behavioural | Command | `TransitionCommand`, `WorkerCommand`, `AllocationCommand`, `ApprovalCommand`, `ProgressCommand`, `CommandHistory` |
| Behavioural | Memento | `WorkItemMemento` |
| Behavioural | Observer | `Observer`, `ParticipantObserver`, `generalObserver` |
| Behavioural | Strategy | `AssignmentStrategy`, `ApprovalStrategy`, `ApproverPolicy`, `ReviewAction`, `RunPolicy` and their subclasses |
| Behavioural | Visitor | `WorkItemVisitor`, `ProgressReportVisitor`, `EscalationReportVisitor`, `StalledWorkVisitor` |
| Behavioural | Chain of Responsibility | `AssignmentHandler`, `LevelHandler`, `ManagerHandler`, `ReviewHandler` (the Reviewer at the end) |
| Behavioural | Mediator | `BatchRunner` ("Run all tickets") coordinating the cases through `CaseDesk` |

Pattern mappings and justifications: `docs/patterns/`. UML exports and Visual
Paradigm files: `docs/diagrams/`. Final report: `report/`.

## 7. Folder layout

```
src/        engine, industries, tests, TUI, web server and Makefile
src/third_party/  cpp-httplib and nlohmann/json (web server only)
web/        the web GUI page (plain HTML/CSS/JS, calls the REST API)
docs/       diagrams (exports + .vpp), pattern write-ups, submission notes
report/     final PDF report
```

## 8. How we work with Git

- `main` is for releases, `dev` is where finished work is merged.
- Each person works on a branch named `feature/<what>-<name>`, made from `dev`.
- Work goes into `dev` only through a pull request, and one other member reviews it first.
- Before opening a pull request: `make clean`, build with no warnings, and run all the tests.

## 9. Team

| Member | GitHub |
|---|---|
| Lindelwe (Lee) Mabaleka | lee-codes012 |
| Melaney (Mel) | Melcodes03 |
| Rati | keratiineh |
| Stephen | u25176502-commits |
| Matshidiso (Tshidi) | u25227506-eng |

## 10. Third-party code
- doctest 2.4.11, © Viktor Kirilov, MIT licence – `src/doctest.h`.
- cpp-httplib 0.63.0, © Yuji Hirose, MIT licence – `src/third_party/httplib.h` (web GUI server).
- nlohmann/json 3.12.0, © Niels Lohmann, MIT licence – `src/third_party/json.hpp` (web GUI JSON).

## Running with Docker

Docker builds and runs TIVIDY in a clean Linux container, so no local compiler is needed.
Requires Docker Desktop (running).

From the top level of the repository:

| Step | Command |
|---|---|
| Build the image | `docker build -t tividy .` |
| Run TIVIDY (interactive) | `docker run -it --rm tividy` |
| Run all automated tests | `docker run --rm tividy make test` |