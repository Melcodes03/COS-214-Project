 <p align="center">
  <img src="assets/wave-top.svg" width="100%" alt="TIVIDY teal waves"/>
</p>

<h1 align="center">TIVIDY</h1>

<p align="center">
  <strong>Workflow Management System</strong>
  <br/>
  COS 214 Final Project · University of Pretoria · 2026
</p>

<p align="center">
  A generic C++11 workflow engine for software customer support
  and university admissions.
</p>

---

## 1. Requirements

| Tool | Version | Notes |
|---|---|---|
| g++ | 7 or newer, with C++11 | On Windows use MSYS2 / MinGW-w64 |
| make | GNU Make | On Windows use `mingw32-make` |
| A web browser | Any recent one | Only for the web interface |
| valgrind | Any | Optional, Linux only, for memory checks |
| Docker | Any | Optional, see "Running with Docker" at the end |

Nothing else needs installing. The test framework (doctest 2.4.11) and the two libraries used by the web server (cpp-httplib 0.63.0 and nlohmann/json 3.12.0) are included in the repository.

## 2. Build

```bash
cd src
make clean     # Run this after pulling; the Makefile does not track header changes
make
```

Everything is compiled with `-std=c++11 -Wall -Wextra -pedantic`.

## 3. Run

### Web interface

```bash
cd src
make run_web
```

Open [http://localhost:8080](http://localhost:8080) in a browser.

To use another port:

```bash
./tividy_web --port 9000 --web ../web
```

The page starts with the default scenarios already open: 3 support cases and 3 admissions applications.

Everything you do is saved to `src/tividy-log.jsonl` and restored when the server restarts.

- `./tividy_web --fresh` starts again from the defaults.
- **History → Reset to defaults** does the same from the page.

### Text interface

```bash
cd src
make run_tui
```

### Console demo

```bash
cd src
make run
```

## 4. Try Each Industry

Start the web interface and pick a workflow. Press **Assign**, **Start** and **Complete** on each task as it opens. Only the buttons the engine allows are shown.

**View as** (top bar) shows one person's work, and **Run all tickets** (under My work) runs a batch of tickets by itself.

### Industry 1: Software Customer Support

**Completion path**

Open SC-1001 (the critical case) and work through Intake, Investigation, Resolution and Closure.

"Verify fix" needs two sign-offs, so use **Approve** instead of **Complete**.

Follow-up then opens "Write incident report" because the severity is critical.

SC-1002 and SC-1003 are normal cases and get the satisfaction survey instead.

**Exception path**

- Assign a task and press **Decline**. It moves up one level of the escalation chain.
- **Reject** on a sign-off sends the work back for rework (at most 3 times).
- **Cancel** ends the whole case as Failed.
- **Undo** brings it back.

### Industry 2: University Admissions

**Completion path**

Open ADM-2001 (score 78). The two checks in Verification open at the same time.

After the academic review, the score picks the branch.

A score of 70 or more gives an offer, which the Registrar signs off, followed by enrolment.

**Other branches**

- ADM-2002 (score 62) goes to the waiting list.
- ADM-2003 (score 48) gets a rejection notice.
- You can also type a different score when you start the case.

**Exception path**

The same as support: decline, reject, escalate, cancel and undo.

## 5. Run the Tests

Run all tests from the `src` directory:

```bash
make test_all
```

This runs everything and stops at the first failure.

| Command | What it tests | Framework |
|---|---|---|
| `make test_web` | The web service: actions, escalation chain, cancel, sign-offs, decisions, several cases, building a workflow, defaults, batch runs | doctest |
| `make test_workflow` | Sequential, parallel and choice stages, the cancel rule, instance isolation | doctest |
| `make test_state` | The work item lifecycle (State) and the decorators | doctest |
| `make test_chain` | Chain of Responsibility, escalation, progress and escalation reports | doctest |
| `make test_builder` | Builder, the director, Prototype, the industry data | doctest |
| `make test_composite` | Composite and Iterator | doctest |
| `make test_tui` | The text interface, scripted | assert program |
| `make run` | Composite, Iterator, State, Decorator, instance isolation | assert program |
| `make test_cm` | Command, Memento, undo, redo, history | assert program |
| `make test_vs` | Visitor and Strategy | assert program |
| `make test_observer` | Observer | assert program |
| `make test_adapter` | Email and SMS adapters | assert program |

### Memory Check

On Linux:

```bash
valgrind --leak-check=full ./chain_test
```

### Useful Doctest Options

```bash
./chain_test --list-test-cases
./chain_test -tc="*declined*"
./chain_test -s
```

The `-s` option shows every check.

## 6. Where to Find Things

| Directory / File | Description |
|---|---|
| `src/` | The engine, both industries (`DemoData.cpp`), tests, text interface, web server and Makefile |
| `web/` | The web page |
| `docs/web-api.md` | The REST API |
| `docs/diagrams/` | UML exports and Visual Paradigm files |
| `docs/patterns/` | Pattern mappings and justifications |
| `report/` | The final PDF report |

## 7. Team

| Member | Student number | GitHub |
|---|---|---|
| Lindelwe (Lee) Mabaleka | u25019385 | [lee-codes012](https://github.com/lee-codes012) |
| Melaney (Mel) Mutunhu | u25463382 | [Melcodes03](https://github.com/Melcodes03) |
| Rati Molefe | u25324170 | [keratiineh](https://github.com/keratiineh) |
| Stephen Odhiambo | u25176502 | [u25176502-commits](https://github.com/u25176502-commits) |
| Matshidiso (Tshidi) Dibakoane | u25227506 | [u25227506-eng](https://github.com/u25227506-eng) |

---

## Running with Docker

Docker builds and runs TIVIDY in a clean Linux container, so no local compiler is needed.

It requires Docker Desktop to be running.

From the top folder of the repository:

| Step | Command |
|---|---|
| Build the image | `docker build -t tividy .` |
| Run the console demo | `docker run --rm tividy` |
| Run all the automated tests | `docker run --rm tividy make test_all` |

The image runs the console demo and the tests.

To use the web or text interface, build and run it directly as described in Section 3.

---

<p align="center">
  <img src="assets/wave-bottom.svg" width="100%" alt="TIVIDY teal waves"/>
</p>