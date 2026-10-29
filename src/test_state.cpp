/* Unit tests for the work-item lifecycle (State) and the decorators. */
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "Task.h"
#include "AuditingDecorator.h"
#include "PriorityDecorator.h"
#include "ValidationDecorator.h"

namespace {
bool alwaysValid(const WorkItem&) { return true; }
bool neverValid(const WorkItem&) { return false; }

// Brings a new task to InProgress.
void startWork(WorkItem& item) {
    item.makeAvailable();
    item.assign();
    item.start();
}
} // namespace

TEST_CASE("a work item moves through its whole lifecycle") {
    Task task("t", "Task");
    CHECK(task.getStateName() == "Created");
    CHECK(task.makeAvailable());
    CHECK(task.getStateName() == "Available");
    CHECK(task.assign());
    CHECK(task.getStateName() == "Assigned");
    CHECK(task.start());
    CHECK(task.getStateName() == "InProgress");
    CHECK(task.complete());
    CHECK(task.getStateName() == "Completed");
}

TEST_CASE("operations that are invalid in a state are refused and change nothing") {
    Task task("t", "Task");
    CHECK_FALSE(task.assign());     // Created: not yet available
    CHECK_FALSE(task.start());
    CHECK_FALSE(task.complete());
    CHECK_FALSE(task.reject());
    CHECK_FALSE(task.escalate());
    CHECK(task.getStateName() == "Created");

    task.makeAvailable();
    CHECK_FALSE(task.start());      // Available: must be assigned first
    CHECK_FALSE(task.complete());
    CHECK(task.getStateName() == "Available");
}

TEST_CASE("completed and cancelled are final: nothing is allowed any more") {
    Task done("d", "Done");
    startWork(done);
    done.complete();
    CHECK_FALSE(done.makeAvailable());
    CHECK_FALSE(done.assign());
    CHECK_FALSE(done.cancel());
    CHECK_FALSE(done.escalate());
    CHECK(done.getStateName() == "Completed");

    Task dropped("c", "Cancelled");
    CHECK(dropped.cancel());
    CHECK_FALSE(dropped.makeAvailable());
    CHECK_FALSE(dropped.assign());
    CHECK(dropped.getStateName() == "Cancelled");
}

TEST_CASE("an assigned item can be declined and goes back to Available") {
    Task task("t", "Task");
    task.makeAvailable();
    task.assign();
    CHECK(task.reject());
    CHECK(task.getStateName() == "Available");
    CHECK(task.assign());           // it can be given to someone else
}

TEST_CASE("rework is allowed only while retries remain") {
    Task task("t", "Task");
    task.setMaxRetries(1);
    startWork(task);
    CHECK(task.reject());
    CHECK(task.getStateName() == "Rejected");

    CHECK(task.assign());           // first rework
    CHECK(task.getRetryCount() == 1);
    CHECK(task.start());
    CHECK(task.reject());
    CHECK_FALSE(task.assign());     // no retries left
    CHECK(task.getStateName() == "Rejected");
    CHECK(task.cancel());           // the only way out
    CHECK(task.getStateName() == "Cancelled");
}

TEST_CASE("an item nobody responds to can be escalated and then reassigned") {
    Task waiting("w", "Waiting");
    waiting.makeAvailable();
    waiting.assign();
    CHECK(waiting.escalate());
    CHECK(waiting.getStateName() == "Escalated");
    CHECK(waiting.assign());

    Task working("k", "Working");
    startWork(working);
    CHECK(working.escalate());      // also possible while InProgress
    CHECK(working.getStateName() == "Escalated");
}

TEST_CASE("a clone starts fresh but keeps its configuration") {
    Task task("t", "Task");
    task.setMaxRetries(5);
    startWork(task);
    task.reject();
    WorkItem* copy = task.clone();
    CHECK(copy->getStateName() == "Created");
    CHECK(copy->getRetryCount() == 0);
    CHECK(copy->getMaxRetries() == 5);
    CHECK(task.getStateName() == "Rejected");   // the original is untouched
    delete copy;
}

TEST_CASE("a validation decorator refuses completion when its rule fails") {
    ValidationDecorator blocked(new Task("b", "Blocked"), neverValid);
    startWork(blocked);
    CHECK_FALSE(blocked.complete());
    CHECK(blocked.getStateName() == "InProgress");

    ValidationDecorator allowed(new Task("a", "Allowed"), alwaysValid);
    startWork(allowed);
    CHECK(allowed.complete());
    CHECK(allowed.getStateName() == "Completed");
}

TEST_CASE("an auditing decorator logs every attempt, including refused ones") {
    AuditingDecorator audited(new Task("t", "Task"));
    CHECK_FALSE(audited.complete());    // refused: still Created
    CHECK(audited.makeAvailable());

    REQUIRE(audited.getAuditLog().size() == 2);
    CHECK(audited.getAuditLog()[0] == "complete: Created -> refused");
    CHECK(audited.getAuditLog()[1] == "makeAvailable: Created -> Available");
}

TEST_CASE("decorators stack and a decorated clone is independent") {
    AuditingDecorator stacked(new ValidationDecorator(new PriorityDecorator(new Task("t", "Task"), 7), alwaysValid));
    startWork(stacked);
    CHECK(stacked.getPriority() == 7);
    CHECK(stacked.complete());

    WorkItem* copy = stacked.clone();
    CHECK(copy->getStateName() == "Created");   // a fresh copy
    CHECK(copy->getPriority() == 7);            // with the same decorators
    CHECK(stacked.getStateName() == "Completed");
    delete copy;
}