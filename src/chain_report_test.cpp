/// doctest tests for the Chain of Responsibility (escalation levels), the
/// WorkAllocator that uses it, the progress / escalation reports and their screens.
/// Build and run with: make test_chain

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include <sstream>
#include <string>
#include <vector>

#include "Task.h"
#include "Stage.h"
#include "PriorityDecorator.h"
#include "Participant.h"
#include "CommandHistory.h"
#include "WorkAllocator.h"
#include "AssignmentHandler.h"
#include "LevelHandler.h"
#include "ManagerHandler.h"
#include "LeastWorkloadAssignment.h"
#include "ProgressReportVisitor.h"
#include "EscalationReportVisitor.h"
#include "ReportScreen.h"
#include "CompletedState.h"
#include "EscalatedState.h"
#include "AssignedState.h"

namespace {

/// Four levels, as in the support desk: General Assistant (priority up to 1),
/// Technical Assistant (up to 2), Engineer (up to 3), then the manager.
AssignmentHandler* makeChain(Participant* manager) {
    LevelHandler* general = new LevelHandler("General Assistant", 1, new LeastWorkloadAssignment());
    LevelHandler* technical = new LevelHandler("Technical Assistant", 2, new LeastWorkloadAssignment());
    LevelHandler* engineer = new LevelHandler("Engineer", 3, new LeastWorkloadAssignment());
    general->setNext(technical);
    technical->setNext(engineer);
    engineer->setNext(new ManagerHandler(manager));
    return general;
}

/// A task that is ready to be assigned.
Task* readyTask(const std::string& id, const std::string& name) {
    Task* t = new Task(id, name);
    t->makeAvailable();
    return t;
}

/// The same, wrapped with a priority.
WorkItem* readyTask(const std::string& id, const std::string& name, int priority) {
    WorkItem* t = new PriorityDecorator(new Task(id, name), priority);
    t->makeAvailable();
    return t;
}

}

TEST_SUITE("Chain of Responsibility") {

    TEST_CASE("a normal task goes to the first level") {
        Participant thandi("Thandi", "General Assistant", 5);
        Participant sipho("Sipho", "Technical Assistant", 5);
        Participant boss("Naledi", "Manager", 5);
        std::vector<Participant*> people;
        people.push_back(&thandi);
        people.push_back(&sipho);

        AssignmentHandler* chain = makeChain(&boss);
        Task* task = readyTask("T1", "Reset password");

        CHECK(chain->handle(*task, people) == &thandi);

        delete task;
        delete chain;
    }

    TEST_CASE("a high-priority task skips the levels it is too high for") {
        Participant thandi("Thandi", "General Assistant", 5);
        Participant sipho("Sipho", "Technical Assistant", 5);
        Participant lerato("Lerato", "Engineer", 5);
        Participant boss("Naledi", "Manager", 5);
        std::vector<Participant*> people;
        people.push_back(&thandi);
        people.push_back(&sipho);
        people.push_back(&lerato);

        AssignmentHandler* chain = makeChain(&boss);
        WorkItem* outage = readyTask("T2", "Server outage", 3);
        WorkItem* critical = readyTask("T3", "Data breach", 9);

        CHECK(chain->handle(*outage, people) == &lerato);
        CHECK(chain->handle(*critical, people) == &boss);   //above every level

        delete outage;
        delete critical;
        delete chain;
    }

    TEST_CASE("a level with nobody in it passes the task on") {
        Participant lerato("Lerato", "Engineer", 5);
        Participant boss("Naledi", "Manager", 5);
        std::vector<Participant*> people;
        people.push_back(&lerato);   //no assistants today

        AssignmentHandler* chain = makeChain(&boss);
        Task* task = readyTask("T4", "Install software");

        CHECK(chain->handle(*task, people) == &lerato);

        std::vector<Participant*> nobody;
        CHECK(chain->handle(*task, nobody) == &boss);

        delete task;
        delete chain;
    }

    TEST_CASE("each level uses its own strategy to pick the worker") {
        Participant busy("Thandi", "General Assistant", 5);
        Participant free("Ayanda", "General Assistant", 5);
        Participant boss("Naledi", "Manager", 5);
        busy.addTask();
        busy.addTask();
        std::vector<Participant*> people;
        people.push_back(&busy);
        people.push_back(&free);

        AssignmentHandler* chain = makeChain(&boss);
        Task* task = readyTask("T5", "Printer jam");

        CHECK(chain->handle(*task, people) == &free);   //least workload

        delete task;
        delete chain;
    }

    TEST_CASE("a level skips workers who are full or not available") {
        Participant full("Thandi", "General Assistant", 1);
        Participant away("Ayanda", "General Assistant", 5);
        Participant sipho("Sipho", "Technical Assistant", 5);
        Participant boss("Naledi", "Manager", 5);
        full.addTask();              //at capacity
        away.setAvailable(false);    //offline
        std::vector<Participant*> people;
        people.push_back(&full);
        people.push_back(&away);
        people.push_back(&sipho);

        AssignmentHandler* chain = makeChain(&boss);
        Task* task = readyTask("T7", "Password reset");

        CHECK(chain->handle(*task, people) == &sipho);   //no free General Assistant

        away.setAvailable(true);
        CHECK(chain->handle(*task, people) == &away);

        delete task;
        delete chain;
    }

    TEST_CASE("levelOf finds the level a worker belongs to") {
        Participant sipho("Sipho", "Technical Assistant", 5);
        Participant boss("Naledi", "Manager", 5);
        Participant stranger("Pieter", "Cleaner", 5);

        AssignmentHandler* chain = makeChain(&boss);

        LevelHandler* level = dynamic_cast<LevelHandler*>(chain->levelOf(sipho));
        REQUIRE(level != nullptr);
        CHECK(level->getRole() == "Technical Assistant");
        CHECK(dynamic_cast<ManagerHandler*>(chain->levelOf(boss)) != nullptr);
        CHECK(chain->levelOf(stranger) == nullptr);

        delete chain;
    }

    TEST_CASE("the end of the chain with no manager gives nobody") {
        AssignmentHandler* chain = makeChain(nullptr);
        std::vector<Participant*> nobody;
        Task* task = readyTask("T6", "Orphan");

        CHECK(chain->handle(*task, nobody) == nullptr);

        delete task;
        delete chain;
    }
}

TEST_SUITE("WorkAllocator with the chain") {

    TEST_CASE("allocate assigns the task and remembers the level") {
        Participant thandi("Thandi", "General Assistant", 5);
        Participant boss("Naledi", "Manager", 5);
        WorkAllocator allocator(new LeastWorkloadAssignment());
        allocator.addParticipant(&thandi);
        allocator.setChain(makeChain(&boss));

        Task* task = readyTask("T1", "Reset password");
        CHECK(allocator.allocate(*task) == &thandi);
        CHECK(task->getState() == AssignedState::instance());
        CHECK(thandi.getActiveTasks() == 1);
        CHECK(allocator.getAssignee("T1") == &thandi);

        LevelHandler* level = dynamic_cast<LevelHandler*>(allocator.getLevel("T1"));
        REQUIRE(level != nullptr);
        CHECK(level->getRole() == "General Assistant");

        delete task;
    }

    TEST_CASE("a declined task goes up one level at a time, ending with the manager") {
        Participant thandi("Thandi", "General Assistant", 5);
        Participant sipho("Sipho", "Technical Assistant", 5);
        Participant lerato("Lerato", "Engineer", 5);
        Participant boss("Naledi", "Manager", 5);
        WorkAllocator allocator(new LeastWorkloadAssignment());
        allocator.addParticipant(&thandi);
        allocator.addParticipant(&sipho);
        allocator.addParticipant(&lerato);
        allocator.setChain(makeChain(&boss));
        CommandHistory history;

        Task* task = readyTask("T1", "VPN not working");
        REQUIRE(allocator.allocate(*task) == &thandi);

        REQUIRE(thandi.decline(task, history));
        CHECK(thandi.getActiveTasks() == 0);
        CHECK(allocator.reassign(*task) == &sipho);

        REQUIRE(sipho.decline(task, history));
        CHECK(allocator.reassign(*task) == &lerato);

        REQUIRE(lerato.decline(task, history));
        CHECK(allocator.reassign(*task) == &boss);
        CHECK(allocator.getAssignee("T1") == &boss);
        CHECK(task->getState() == AssignedState::instance());

        REQUIRE(boss.decline(task, history));
        CHECK(allocator.reassign(*task) == nullptr);   //nobody above the manager

        delete task;
    }

    TEST_CASE("a worker who does not respond: the task is escalated to the next level") {
        Participant thandi("Thandi", "General Assistant", 5);
        Participant sipho("Sipho", "Technical Assistant", 5);
        Participant boss("Naledi", "Manager", 5);
        WorkAllocator allocator(new LeastWorkloadAssignment());
        allocator.addParticipant(&thandi);
        allocator.addParticipant(&sipho);
        allocator.setChain(makeChain(&boss));

        Task* task = readyTask("T1", "Email bounce");
        REQUIRE(allocator.allocate(*task) == &thandi);

        CHECK(thandi.getWorklist().size() == 1);
        CHECK(allocator.escalate(*task) == &sipho);
        CHECK(thandi.getActiveTasks() == 0);   //freed
        CHECK(thandi.getWorklist().empty());   //and off their worklist
        CHECK(sipho.getWorklist().size() == 1);
        CHECK(sipho.getActiveTasks() == 1);
        CHECK(task->getState() == AssignedState::instance());

        delete task;
    }

    TEST_CASE("invalid escalations change nothing") {
        Participant thandi("Thandi", "General Assistant", 5);
        Participant boss("Naledi", "Manager", 5);

        SUBCASE("no chain set") {
            WorkAllocator allocator(new LeastWorkloadAssignment());
            allocator.addParticipant(&thandi);
            Task* task = readyTask("T1", "Reset password");
            REQUIRE(allocator.allocate(*task) == &thandi);
            CHECK(allocator.escalate(*task) == nullptr);
            CHECK(allocator.reassign(*task) == nullptr);
            CHECK(allocator.getAssignee("T1") == &thandi);
            CHECK(allocator.getLevel("T1") == nullptr);
            delete task;
        }

        SUBCASE("a completed task cannot be escalated") {
            WorkAllocator allocator(new LeastWorkloadAssignment());
            allocator.addParticipant(&thandi);
            allocator.setChain(makeChain(&boss));
            CommandHistory history;
            Task* task = readyTask("T1", "Reset password");
            REQUIRE(allocator.allocate(*task) == &thandi);
            REQUIRE(thandi.start(task, history));
            REQUIRE(thandi.complete(task, history));
            CHECK(allocator.escalate(*task) == nullptr);
            CHECK(task->getState() == CompletedState::instance());
            CHECK(boss.getActiveTasks() == 0);
            delete task;
        }
    }

    TEST_CASE("without a chain the allocator still uses its strategy") {
        Participant thandi("Thandi", "General Assistant", 5);
        Participant sipho("Sipho", "Technical Assistant", 5);
        thandi.addTask();
        WorkAllocator allocator(new LeastWorkloadAssignment());
        allocator.addParticipant(&thandi);
        allocator.addParticipant(&sipho);

        Task* task = readyTask("T1", "Reset password");
        CHECK(allocator.allocate(*task) == &sipho);
        delete task;
    }

    TEST_CASE("release forgets who holds the task") {
        Participant thandi("Thandi", "General Assistant", 5);
        Participant boss("Naledi", "Manager", 5);
        WorkAllocator allocator(new LeastWorkloadAssignment());
        allocator.addParticipant(&thandi);
        allocator.setChain(makeChain(&boss));

        Task* task = readyTask("T1", "Reset password");
        REQUIRE(allocator.allocate(*task) != nullptr);
        allocator.release("T1");
        CHECK(allocator.getAssignee("T1") == nullptr);
        CHECK(allocator.getLevel("T1") == nullptr);
        delete task;
    }
}

TEST_SUITE("Reports") {

    /// Support case: Log (1 task) and Investigate (2 tasks).
    Stage* makeCase(Task*& log, Task*& check, Task*& fix) {
        Stage* root = new Stage("S0", "Support case");
        Stage* logStage = new Stage("S1", "Log");
        Stage* investigate = new Stage("S2", "Investigate");
        log = readyTask("T1", "Log the case");
        check = readyTask("T2", "Check logs");
        fix = readyTask("T3", "Apply fix");
        logStage->add(log);
        investigate->add(check);
        investigate->add(fix);
        root->add(logStage);
        root->add(investigate);
        return root;
    }

    void finish(WorkItem* item) {
        item->assign();
        item->start();
        item->complete();
    }

    TEST_CASE("progress is shown for each stage and refreshed after each action") {
        Task* log = nullptr;
        Task* check = nullptr;
        Task* fix = nullptr;
        Stage* root = makeCase(log, check, fix);
        ProgressReportVisitor report;

        report.visitAll(*root);
        CHECK(report.getTaskCount() == 3);
        CHECK(report.percentComplete() == 0);
        CHECK(report.stageTaskCount("Investigate") == 2);

        finish(log);
        finish(check);
        report.reset();
        report.visitAll(*root);

        CHECK(report.getTaskCount() == 3);   //not counted twice after reset()
        CHECK(report.percentComplete() == 66);
        CHECK(report.stageCompletedCount("Log") == 1);
        CHECK(report.stagePercentComplete("Log") == 100);
        CHECK(report.stagePercentComplete("Investigate") == 50);
        CHECK(report.stageSummary().find("Investigate: 1/2 tasks (50%)") != std::string::npos);

        std::vector<std::string> names = report.getStageNames();
        REQUIRE(names.size() == 2);
        CHECK(names[0] == "Log");
        CHECK(names[1] == "Investigate");

        delete root;
    }

    TEST_CASE("an unknown stage reports zero") {
        ProgressReportVisitor report;
        CHECK(report.stageTaskCount("Nope") == 0);
        CHECK(report.stagePercentComplete("Nope") == 0);
        CHECK(report.percentComplete() == 0);
    }

    TEST_CASE("the escalation report lists escalated and rejected tasks") {
        Task* log = nullptr;
        Task* check = nullptr;
        Task* fix = nullptr;
        Stage* root = makeCase(log, check, fix);

        check->assign();
        check->escalate();   //Assigned -> Escalated
        fix->assign();
        fix->start();
        fix->reject();       //InProgress -> Rejected
        REQUIRE(check->getState() == EscalatedState::instance());

        EscalationReportVisitor report;
        report.visitAll(*root);
        const std::vector<std::string>& flagged = report.getFlagged();
        REQUIRE(flagged.size() == 2);
        CHECK(flagged[0].find("T2 (Investigate)") != std::string::npos);
        CHECK(flagged[1].find("T3 (Investigate)") != std::string::npos);

        delete root;
    }
}

TEST_SUITE("Report and escalation screens") {

    Stage* makeSmallCase() {
        Stage* root = new Stage("S0", "Support case");
        Stage* investigate = new Stage("S1", "Investigate");
        investigate->add(readyTask("T1", "Check logs"));
        investigate->add(readyTask("T2", "Apply fix"));
        root->add(investigate);
        return root;
    }

    TEST_CASE("the progress screen shows the total and each stage") {
        Stage* root = makeSmallCase();
        WorkItem* check = ReportScreen::find(*root, "T1");
        REQUIRE(check != nullptr);
        check->assign();
        check->start();
        check->complete();

        std::ostringstream out;
        ReportScreen::showProgress(*root, out);
        CHECK(out.str().find("=== Progress: Support case ===") != std::string::npos);
        CHECK(out.str().find("50% complete") != std::string::npos);
        CHECK(out.str().find("Investigate: 1/2 tasks (50%)") != std::string::npos);

        delete root;
    }

    TEST_CASE("the escalation screen says so when nothing is stuck") {
        Stage* root = makeSmallCase();
        std::ostringstream out;
        ReportScreen::showEscalations(*root, out);
        CHECK(out.str().find("No escalated or rejected tasks.") != std::string::npos);
        delete root;
    }

    TEST_CASE("escalating from the screen moves the task up and the report shows it") {
        Participant thandi("Thandi", "General Assistant", 5);
        Participant sipho("Sipho", "Technical Assistant", 5);
        Participant boss("Naledi", "Manager", 5);
        WorkAllocator allocator(new LeastWorkloadAssignment());
        allocator.addParticipant(&thandi);
        allocator.addParticipant(&sipho);
        allocator.setChain(makeChain(&boss));

        Stage* root = makeSmallCase();
        WorkItem* check = ReportScreen::find(*root, "T1");
        REQUIRE(allocator.allocate(*check) == &thandi);

        std::ostringstream out;
        CHECK(ReportScreen::escalateTask(*root, "T1", allocator, out));
        CHECK(out.str() == "Task T1 (Check logs) moved from Thandi to Sipho (Technical Assistant).\n");
        CHECK(allocator.getAssignee("T1") == &sipho);

        delete root;
    }

    TEST_CASE("declining from the screen gives the task to the next level") {
        Participant thandi("Thandi", "General Assistant", 5);
        Participant boss("Naledi", "Manager", 5);
        WorkAllocator allocator(new LeastWorkloadAssignment());
        allocator.addParticipant(&thandi);
        allocator.setChain(makeChain(&boss));
        CommandHistory history;

        Stage* root = makeSmallCase();
        WorkItem* fix = ReportScreen::find(*root, "T2");
        REQUIRE(allocator.allocate(*fix) == &thandi);
        REQUIRE(thandi.decline(fix, history));

        std::ostringstream out;
        CHECK(ReportScreen::reassignTask(*root, "T2", allocator, out));
        CHECK(out.str() == "Task T2 (Apply fix) moved from Thandi to Naledi (Manager).\n");

        delete root;
    }

    TEST_CASE("bad input gives a friendly message and changes nothing") {
        Participant thandi("Thandi", "General Assistant", 5);
        Participant boss("Naledi", "Manager", 5);
        WorkAllocator allocator(new LeastWorkloadAssignment());
        allocator.addParticipant(&thandi);
        allocator.setChain(makeChain(&boss));
        Stage* root = makeSmallCase();

        SUBCASE("unknown id") {
            std::ostringstream out;
            CHECK_FALSE(ReportScreen::escalateTask(*root, "T99", allocator, out));
            CHECK(out.str() == "There is no task with id \"T99\".\n");
        }

        SUBCASE("a task nobody holds yet cannot be escalated") {
            std::ostringstream out;
            CHECK_FALSE(ReportScreen::escalateTask(*root, "T1", allocator, out));
            CHECK(out.str() == "Task T1 cannot be escalated while it is Available.\n");
            CHECK(ReportScreen::find(*root, "T1")->getState() != EscalatedState::instance());
        }

        SUBCASE("the manager declines: nobody is above") {
            CommandHistory history;
            WorkItem* check = ReportScreen::find(*root, "T1");
            WorkAllocator onlyBoss(new LeastWorkloadAssignment());
            onlyBoss.setChain(makeChain(&boss));
            REQUIRE(onlyBoss.allocate(*check) == &boss);
            REQUIRE(boss.decline(check, history));
            std::ostringstream out;
            CHECK_FALSE(ReportScreen::reassignTask(*root, "T1", onlyBoss, out));
            CHECK(out.str() == "Nobody above Naledi could take task T1. It is now Available.\n");
        }

        delete root;
    }
}
