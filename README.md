# TIVIDY – Workflow Management System

COS 214 Final Project, University of Pretoria, 2026.

TIVIDY is one generic workflow engine written in C++11. The engine knows about
work items, stages, people, states and history, but nothing about any single
business. Each industry gives it its own roles, rules, data and order of steps.

> **Status:** sections marked _(to do)_ are filled in when that part is merged.

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

```bash
cd src
make run
```

_(to do: the text interface – pick an industry, start an instance, log in as a
participant, view your worklist, take actions, view reports and history.)_

## 4. Tests

| Command | What it tests | Framework |
|---|---|---|
| `make test_chain` | Chain of Responsibility, WorkAllocator escalation, progress and escalation reports, report screens | doctest |
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
- Completion path: the case is logged, investigated, fixed and closed.
- Exception paths: a worker declines or does not respond, so the case moves up
  the escalation chain; a cancelled task ends the whole workflow.

_(to do: how to run it in the TUI and sample data.)_

### Industry 2
_(to do once the team has chosen the industry.)_

## 6. Design patterns

| Category | Pattern | Main classes |
|---|---|---|
| Creational | Builder | `WorkflowBuilder`, `WorkflowDirector` _(to do)_ |
| Creational | Prototype | `WorkItem::clone()`, `WorkflowInstance` |
| Structural | Composite | `WorkItem`, `Task`, `Stage` |
| Structural | Decorator | `PriorityDecorator`, `ValidationDecorator`, `AuditingDecorator` |
| Structural | Adapter | `CommunicationAdapter`, `EmailConnector`, `SmsConnector` |
| Behavioural | Iterator | `WorkItemIterator`, `DepthFirstIterator` |
| Behavioural | State | `WorkItemState` and its 8 states |
| Behavioural | Command | `TransitionCommand`, `WorkerCommand`, `CommandHistory` |
| Behavioural | Memento | `WorkItemMemento` |
| Behavioural | Observer | `Observer`, `ParticipantObserver`, `generalObserver` |
| Behavioural | Strategy | `AssignmentStrategy`, `ApprovalStrategy` and their subclasses |
| Behavioural | Visitor | `WorkItemVisitor`, `ProgressReportVisitor`, `EscalationReportVisitor` |
| Behavioural | Chain of Responsibility | `AssignmentHandler`, `LevelHandler`, `ManagerHandler` |

Pattern mappings and justifications: `docs/patterns/`. UML exports and Visual
Paradigm files: `docs/diagrams/`. Final report: `report/`.

## 7. Folder layout

```
src/        engine, industries, tests and Makefile
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
