/*Unit tests for the workflow engine: advancing, routing, the cancel rule,
instance isolation and the lifecycle rules they rely on*/
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "Stage.h"
#include "doctest.h"
#include "Task.h"
#include "WorkflowDefinition.h"
#include "WorkflowInstance.h"
#include "ValidationDecorator.h"
#include "PriorityDecorator.h"

namespace {

//Runs one task through a worker's normal path: assign, start, complete.
bool finish(WorkItem* item) {
    return item->assign() && item->start() && item->complete();
}

bool neverValid(const WorkItem&) { return false; }

//Routing rule used by the choice tests: amounts over 20000 take branch 1, others branch 0.
int byAmount(const DataMap& data) {
    DataMap::const_iterator found = data.find("amount");
    if (found == data.end()) return -1;
    return std::stoi(found->second) > 20000 ? 1 : 0;
}

//process (sequential) -> stage S1 (a1, a2), task b
WorkflowDefinition* makeSequentialDefinition() {
    Stage* process = new Stage("P", "Process");
    Stage* s1 = new Stage("S1", "Stage 1");
    s1->add(new Task("a1", "Task a1"));
    s1->add(new Task("a2", "Task a2"));
    process->add(s1);
    process->add(new Task("b", "Task b"));
    return new WorkflowDefinition("Sequential", 1, process);
}

} //namespace

TEST_CASE("a new instance is running, has no data and offers nothing until advanced") {
    WorkflowDefinition* def = makeSequentialDefinition();
    WorkflowInstance instance("I1", def);

    CHECK(instance.getStatus() == WorkflowInstance::RUNNING);
    CHECK(instance.getStatusName() == "Running");
    CHECK_FALSE(instance.hasData("amount"));
    CHECK(instance.getData("amount", "none") == "none");
    CHECK(instance.getAvailableWork().empty());
    delete def;
}

TEST_CASE("a sequential stage opens its children one at a time") {
    WorkflowDefinition* def = makeSequentialDefinition();
    WorkflowInstance instance("I1", def);
    WorkItem* s1 = instance.getRoot()->getChild(0);
    WorkItem* a1 = s1->getChild(0);
    WorkItem* a2 = s1->getChild(1);
    WorkItem* b = instance.getRoot()->getChild(1);

    instance.advance();
    CHECK(a1->isAvailable());
    CHECK(a2->getStateName() == "Created");   //waits for a1
    CHECK(b->getStateName() == "Created");
    CHECK(instance.getAvailableWork().size() == 1);

    CHECK(finish(a1));
    instance.advance();
    CHECK(a2->isAvailable());
    CHECK(b->getStateName() == "Created");

    CHECK(finish(a2));
    instance.advance();
    CHECK(s1->isCompleted());     //the stage completed itself
    CHECK(b->isAvailable());
    CHECK(instance.getStatus() == WorkflowInstance::RUNNING);

    CHECK(finish(b));
    CHECK(instance.advance() == WorkflowInstance::COMPLETED);
    CHECK(instance.getRoot()->isCompleted());
    CHECK(instance.getAvailableWork().empty());
    delete def;
}

TEST_CASE("a parallel stage opens all its children and waits for all of them") {
    Stage* process = new Stage("P", "Process");
    Stage* investigate = new Stage("INV", "Investigate");
    investigate->setParallel();
    investigate->add(new Task("logs", "Gather logs"));
    investigate->add(new Task("repro", "Reproduce issue"));
    process->add(investigate);
    WorkflowDefinition def("Parallel", 1, process);
    WorkflowInstance instance("I1", &def);
    WorkItem* logs = instance.getRoot()->getChild(0)->getChild(0);
    WorkItem* repro = instance.getRoot()->getChild(0)->getChild(1);

    instance.advance();
    CHECK(logs->isAvailable());
    CHECK(repro->isAvailable());    //both open together
    CHECK(instance.getAvailableWork().size() == 2);

    CHECK(finish(logs));
    CHECK(instance.advance() == WorkflowInstance::RUNNING);   //join: still waiting for repro
    CHECK(finish(repro));
    CHECK(instance.advance() == WorkflowInstance::COMPLETED);
}

TEST_CASE("a choice stage runs only the branch picked from the instance data") {
    Stage* process = new Stage("P", "Process");
    Stage* approval = new Stage("AP", "Approval");
    approval->setChoice(byAmount);
    approval->add(new Task("single", "Single approval"));   //branch 0
    approval->add(new Task("multi", "Multi-level approval"));   //branch 1
    process->add(approval);
    WorkflowDefinition def("Routing", 1, process);

    WorkflowInstance small("small", &def);
    small.setData("amount", "12000");
    WorkflowInstance large("large", &def);
    large.setData("amount", "85000");
    small.advance();
    large.advance();

    WorkItem* smallAp = small.getRoot()->getChild(0);
    WorkItem* largeAp = large.getRoot()->getChild(0);
    CHECK(smallAp->getChild(0)->isAvailable());
    CHECK(smallAp->getChild(1)->getStateName() == "Created");   //branch not taken
    CHECK(largeAp->getChild(1)->isAvailable());
    CHECK(largeAp->getChild(0)->getStateName() == "Created");

    CHECK(finish(smallAp->getChild(0)));
    CHECK(small.advance() == WorkflowInstance::COMPLETED);   //the untaken branch does not block it
    CHECK(large.getStatus() == WorkflowInstance::RUNNING);   //instances route independently
}

TEST_CASE("a choice stage with no matching branch fails the workflow") {
    Stage* process = new Stage("P", "Process");
    Stage* approval = new Stage("AP", "Approval");
    approval->setChoice(byAmount);
    approval->add(new Task("single", "Single approval"));
    process->add(approval);
    WorkflowDefinition def("Routing", 1, process);
    WorkflowInstance instance("I1", &def);   //no "amount" data, so no branch matches

    CHECK(instance.advance() == WorkflowInstance::FAILED);
    CHECK(instance.getAvailableWork().empty());
}

TEST_CASE("cancelling a task fails the whole workflow") {
    WorkflowDefinition* def = makeSequentialDefinition();
    WorkflowInstance instance("I1", def);
    instance.advance();
    WorkItem* a1 = instance.getRoot()->getChild(0)->getChild(0);

    CHECK(a1->cancel());
    CHECK(instance.advance() == WorkflowInstance::FAILED);
    CHECK(instance.getStatusName() == "Failed");
    CHECK(instance.getAvailableWork().empty());   // othing more is offered
    CHECK(instance.advance() == WorkflowInstance::FAILED);   //a failed workflow stays failed
    delete def;
}

TEST_CASE("instances never share progress or data, and the definition is untouched") {
    WorkflowDefinition* def = makeSequentialDefinition();
    WorkflowInstance first("I1", def);
    WorkflowInstance second("I2", def);
    first.setData("customer", "Ana");

    first.advance();
    CHECK(finish(first.getRoot()->getChild(0)->getChild(0)));
    first.advance();

    CHECK(second.getRoot()->getChild(0)->getChild(0)->getStateName() == "Created");
    CHECK(def->getRoot()->getChild(0)->getChild(0)->getStateName() == "Created");
    CHECK_FALSE(second.hasData("customer"));
    CHECK(first.getData("customer") == "Ana");
    CHECK(first.getRoot() != second.getRoot());
    delete def;
}

TEST_CASE("a validation decorator can stop a task completing, so the workflow keeps waiting") {
    Stage* process = new Stage("P", "Process");
    process->add(new ValidationDecorator(new Task("v", "Verify"), neverValid));
    WorkflowDefinition def("Validated", 1, process);
    WorkflowInstance instance("I1", &def);
    instance.advance();
    WorkItem* verify = instance.getRoot()->getChild(0);

    CHECK(instance.getAvailableWork().size() == 1);   //the decorated item is what is offered
    CHECK(verify->assign());
    CHECK(verify->start());
    CHECK_FALSE(verify->complete());   //refused by the rule
    CHECK(instance.advance() == WorkflowInstance::RUNNING);
}

TEST_CASE("the required role is copied by clone() and forwarded by decorators") {
    Task task("t", "Task");
    task.setRequiredRole("Engineer");
    WorkItem* copy = task.clone();
    CHECK(copy->getRequiredRole() == "Engineer");

    PriorityDecorator decorated(new Task("d", "Decorated"), 5);
    decorated.setRequiredRole("Assessor");
    CHECK(decorated.getRequiredRole() == "Assessor");
    WorkItem* decoratedCopy = decorated.clone();
    CHECK(decoratedCopy->getRequiredRole() == "Assessor");
    CHECK(decoratedCopy->getPriority() == 5);
    delete decoratedCopy;
    delete copy;
}

TEST_CASE("invalid lifecycle actions are refused and change nothing") {
    Task task("t", "Task");
    CHECK_FALSE(task.complete());    //cannot complete a Created item
    CHECK_FALSE(task.start());
    CHECK(task.getStateName() == "Created");
    CHECK(task.makeAvailable());
    CHECK(task.isAvailable());
    CHECK(finish(&task));
    CHECK(task.isCompleted());
    CHECK_FALSE(task.cancel());   //Completed is final
    CHECK_FALSE(task.isCancelled());
}
