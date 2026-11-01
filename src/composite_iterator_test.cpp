/// doctest tests for the Composite (WorkItem, Task, Stage) and the Iterator
/// (WorkItemIterator, DepthFirstIterator).
/// Build and run with: make test_composite

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include <string>

#include "Task.h"
#include "Stage.h"
#include "PriorityDecorator.h"
#include "WorkItemIterator.h"

namespace {

std::string order(WorkItem* root) {
    std::string out;
    WorkItemIterator* it = root->createIterator();
    for (it->first(); !it->isDone(); it->next()) {
        out += it->currentItem()->getId() + " ";
    }
    delete it;
    return out;
}

/// P -> (A -> A1, A2), B
Stage* sampleTree() {
    Stage* process = new Stage("P", "Process");
    Stage* stageA = new Stage("A", "Stage A");
    stageA->add(new Task("A1", "Task A1"));
    stageA->add(new Task("A2", "Task A2"));
    process->add(stageA);
    process->add(new Task("B", "Task B"));
    return process;
}

}

TEST_SUITE("Composite") {

    TEST_CASE("a stage holds tasks and other stages") {
        Stage* process = sampleTree();
        CHECK(process->getChildCount() == 2);
        CHECK(process->getChild(0)->getChildCount() == 2);
        CHECK(process->getChild(0)->getChild(1)->getId() == "A2");
        delete process;
    }

    TEST_CASE("a task is a leaf: it refuses children") {
        Task leaf("X", "Leaf");
        Task* extra = new Task("Y", "Extra");
        CHECK_FALSE(leaf.add(extra));
        CHECK(leaf.getChildCount() == 0);
        CHECK(leaf.getChild(0) == nullptr);
        delete extra;   //not taken, so still ours
    }

    TEST_CASE("invalid child requests are safe") {
        Stage stage("S", "Stage");
        CHECK_FALSE(stage.add(nullptr));
        CHECK(stage.getChild(-1) == nullptr);
        CHECK(stage.getChild(5) == nullptr);
    }

    TEST_CASE("a decorated task still works inside a stage") {
        Stage* holder = new Stage("H", "Holder");
        holder->add(new PriorityDecorator(new Task("D", "Decorated"), 2));
        CHECK(holder->getChild(0)->getPriority() == 2);
        CHECK(order(holder) == "H D ");
        delete holder;   //deletes the decorator and the task inside it
    }
}

TEST_SUITE("Iterator") {

    TEST_CASE("depth-first order visits a stage before its children") {
        Stage* process = sampleTree();
        CHECK(order(process) == "P A A1 A2 B ");
        delete process;
    }

    TEST_CASE("first() restarts the walk") {
        Stage* process = sampleTree();
        WorkItemIterator* it = process->createIterator();
        it->first();
        it->next();
        it->next();
        CHECK(it->currentItem()->getId() == "A1");
        it->first();
        CHECK(it->currentItem()->getId() == "P");
        delete it;
        delete process;
    }

    TEST_CASE("a single task is a one-item walk") {
        Task task("T", "Only");
        WorkItemIterator* it = task.createIterator();
        it->first();
        CHECK_FALSE(it->isDone());
        CHECK(it->currentItem() == &task);
        it->next();
        CHECK(it->isDone());
        delete it;
    }

    TEST_CASE("an empty stage is visited on its own") {
        Stage* empty = new Stage("E", "Empty");
        CHECK(order(empty) == "E ");
        delete empty;
    }
}
