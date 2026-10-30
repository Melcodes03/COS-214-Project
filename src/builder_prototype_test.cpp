/// doctest tests for the Builder (WorkflowBuilder, DefinitionBuilder,
/// OutlineBuilder, SupportDirector), the Prototype (clone() and instance
/// isolation) and the Industry 1 support data.
/// Build and run with: make test_builder

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include <string>
#include <vector>

#include "DefinitionBuilder.h"
#include "OutlineBuilder.h"
#include "SupportDirector.h"
#include "SupportData.h"
#include "WorkflowDefinition.h"
#include "WorkflowInstance.h"
#include "WorkItem.h"
#include "WorkItemIterator.h"
#include "Participant.h"

namespace {

bool neverValid(const WorkItem&) { return false; }

/// Ids of every item in the tree, in depth-first order.
std::string idsOf(WorkItem* root) {
    std::string out;
    WorkItemIterator* it = root->createIterator();
    for (it->first(); !it->isDone(); it->next()) {
        out += it->currentItem()->getId() + " ";
    }
    delete it;
    return out;
}

/// Finds an item by id anywhere in the tree (nullptr if missing).
WorkItem* find(WorkItem* root, const std::string& id) {
    WorkItem* found = nullptr;
    WorkItemIterator* it = root->createIterator();
    for (it->first(); !it->isDone() && found == nullptr; it->next()) {
        if (it->currentItem()->getId() == id) {
            found = it->currentItem();
        }
    }
    delete it;
    return found;
}

/// Builds the support definition through the Director.
WorkflowDefinition* supportDefinition() {
    DefinitionBuilder builder("Support case", 1);
    SupportDirector director;
    director.construct(builder);
    return builder.getResult();
}

}

TEST_SUITE("Builder") {

    TEST_CASE("DefinitionBuilder builds stages and tasks in order") {
        DefinitionBuilder builder("Generic", 2);
        builder.beginStage("A", "Stage A")
                   .addTask("A1", "Task A1", "Worker")
                   .addTask("A2", "Task A2", "Worker")
               .endStage()
               .addTask("B", "Task B", "Worker");
        WorkflowDefinition* def = builder.getResult();

        REQUIRE(def != nullptr);
        CHECK(def->getName() == "Generic");
        CHECK(def->getVersion() == 2);
        CHECK(idsOf(const_cast<WorkItem*>(def->getRoot())) == "root A A1 A2 B ");
        delete def;
    }

    TEST_CASE("nested stages close in the right order") {
        DefinitionBuilder builder("Nested", 1);
        builder.beginStage("Outer", "Outer")
                   .beginStage("Inner", "Inner")
                       .addTask("I1", "Inner task", "Worker")
                   .endStage()
                   .addTask("O1", "Outer task", "Worker")
               .endStage();
        WorkflowDefinition* def = builder.getResult();
        WorkItem* root = const_cast<WorkItem*>(def->getRoot());

        WorkItem* outer = root->getChild(0);
        REQUIRE(outer != nullptr);
        CHECK(outer->getChildCount() == 2);                 //Inner stage + O1
        CHECK(outer->getChild(0)->getId() == "Inner");
        CHECK(outer->getChild(1)->getId() == "O1");
        delete def;
    }

    TEST_CASE("withPriority, withValidation and withAudit decorate the last task") {
        DefinitionBuilder builder("Decorated", 1);
        builder.addTask("T", "Task", "Worker").withPriority(3).withValidation(neverValid).withAudit();
        WorkflowDefinition* def = builder.getResult();
        WorkItem* task = const_cast<WorkItem*>(def->getRoot())->getChild(0);

        REQUIRE(task != nullptr);
        CHECK(task->getId() == "T");
        CHECK(task->getPriority() == 3);
        CHECK(task->makeAvailable());
        CHECK(task->assign());
        CHECK(task->start());
        CHECK_FALSE(task->complete());                 //the validation rule refuses
        CHECK(task->getStateName() == "InProgress");
        delete def;
    }

    TEST_CASE("decorating with no task added is ignored, not a crash") {
        DefinitionBuilder builder("Empty", 1);
        builder.withPriority(5).withAudit().beginStage("S", "Stage").withPriority(2).endStage();
        WorkflowDefinition* def = builder.getResult();
        CHECK(idsOf(const_cast<WorkItem*>(def->getRoot())) == "root S ");
        delete def;
    }

    TEST_CASE("extra endStage() never closes the root") {
        DefinitionBuilder builder("Extra", 1);
        builder.endStage().endStage().addTask("T", "Task", "Worker");
        WorkflowDefinition* def = builder.getResult();
        CHECK(def->getRoot()->getChildCount() == 1);
        delete def;
    }

    TEST_CASE("the builder is single-use") {
        DefinitionBuilder builder("Once", 1);
        builder.addTask("T", "Task", "Worker");
        WorkflowDefinition* first = builder.getResult();
        builder.addTask("X", "Ignored", "Worker");    //ignored after getResult()
        CHECK(first != nullptr);
        CHECK(builder.getResult() == nullptr);
        CHECK(first->getRoot()->getChildCount() == 1);
        delete first;
    }

    TEST_CASE("an unfinished builder cleans up after itself") {
        //no getResult(): the destructor must free the tree and the pending task
        //(run under valgrind to see there is no leak)
        DefinitionBuilder* builder = new DefinitionBuilder("Abandoned", 1);
        builder->beginStage("S", "Stage").addTask("T", "Task", "Worker").withAudit();
        delete builder;
        CHECK(true);
    }

    TEST_CASE("OutlineBuilder turns the same steps into text") {
        OutlineBuilder outline("Demo");
        outline.beginStage("S1", "Stage one")
                   .addTask("T1", "First", "Worker").withPriority(2).withAudit()
               .endStage();
        CHECK(outline.getResult() ==
              "Demo\n"
              "  [S1] Stage one\n"
              "    - T1 First (Worker) [priority 2] [audited]\n");
    }
}

TEST_SUITE("Director: Industry 1 support") {

    TEST_CASE("SupportDirector builds the four support stages") {
        WorkflowDefinition* def = supportDefinition();
        WorkItem* root = const_cast<WorkItem*>(def->getRoot());

        REQUIRE(root->getChildCount() == 4);
        CHECK(root->getChild(0)->getName() == "Intake");
        CHECK(root->getChild(1)->getName() == "Investigation");
        CHECK(root->getChild(2)->getName() == "Resolution");
        CHECK(root->getChild(3)->getName() == "Closure");
        CHECK(idsOf(root) == "root S1 S1-T1 S1-T2 S2 S2-T1 S2-T2 S3 S3-T1 S3-T2 S4 S4-T1 S4-T2 ");
        delete def;
    }

    TEST_CASE("support task priorities match the escalation levels") {
        WorkflowDefinition* def = supportDefinition();
        WorkItem* root = const_cast<WorkItem*>(def->getRoot());
        CHECK(find(root, "S1-T1")->getPriority() == 1);   //General Assistant
        CHECK(find(root, "S2-T1")->getPriority() == 2);   //Technical Assistant
        CHECK(find(root, "S2-T2")->getPriority() == 3);   //Engineer
        CHECK(find(root, "S4-T2")->getPriority() == 4);   //above every level: Support Manager
        delete def;
    }

    TEST_CASE("same Director, different builder: the outline matches the definition") {
        OutlineBuilder outline("Support case");
        SupportDirector director;
        director.construct(outline);
        std::string text = outline.getResult();
        CHECK(text.find("[S2] Investigation") != std::string::npos);
        CHECK(text.find("S4-T2 Close support case (Support Manager) [priority 4] [audited]") != std::string::npos);
        CHECK(director.getIndustryName() == "Software customer support");
    }

    TEST_CASE("the support team covers every role the process needs") {
        std::vector<Participant*> team = SupportData::createTeam();
        const char* roles[] = {"General Assistant", "Technical Assistant", "Engineer", "Support Manager"};
        for (int r = 0; r < 4; r++) {
            bool found = false;
            for (int i = 0; i < (int)team.size(); i++) {
                if (team[i]->getRole() == roles[r]) {
                    found = true;
                }
            }
            CHECK_MESSAGE(found, "missing role: " << roles[r]);
        }
        for (int i = 0; i < (int)team.size(); i++) {
            delete team[i];
        }
        CHECK(SupportData::sampleCases().size() >= 3);
    }
}

TEST_SUITE("Prototype") {

    TEST_CASE("every instance gets its own copy of the tree") {
        WorkflowDefinition* def = supportDefinition();
        {
            WorkflowInstance first("SC-1001", def);
            WorkflowInstance second("SC-1002", def);

            CHECK(first.getRoot() != second.getRoot());
            CHECK(first.getRoot() != def->getRoot());
            CHECK(idsOf(first.getRoot()) == idsOf(second.getRoot()));
        }
        delete def;   //the definition must outlive its instances
    }

    TEST_CASE("working on one instance never changes another") {
        WorkflowDefinition* def = supportDefinition();
        {
            WorkflowInstance first("SC-1001", def);
            WorkflowInstance second("SC-1002", def);

            WorkItem* log1 = find(first.getRoot(), "S1-T1");
            CHECK(log1->makeAvailable());
            CHECK(log1->assign());
            CHECK(log1->start());
            CHECK(log1->complete());

            CHECK(log1->getStateName() == "Completed");
            CHECK(find(second.getRoot(), "S1-T1")->getStateName() == "Created");
            CHECK(find(const_cast<WorkItem*>(def->getRoot()), "S1-T1")->getStateName() == "Created");
        }
        delete def;
    }

    TEST_CASE("clones start fresh but keep their configuration") {
        WorkflowDefinition* def = supportDefinition();
        WorkItem* original = find(const_cast<WorkItem*>(def->getRoot()), "S2-T2");
        original->setMaxRetries(1);

        WorkItem* copy = original->clone();
        CHECK(copy != original);
        CHECK(copy->getId() == "S2-T2");
        CHECK(copy->getPriority() == 3);         //decorator copied too
        CHECK(copy->getMaxRetries() == 1);       //configuration kept
        CHECK(copy->getRetryCount() == 0);       //runtime data reset
        CHECK(copy->getStateName() == "Created");
        delete copy;
        delete def;
    }

    TEST_CASE("adding work to one instance does not add it to the others") {
        WorkflowDefinition* def = supportDefinition();
        {
            WorkflowInstance first("SC-1001", def);
            WorkflowInstance second("SC-1002", def);
            int before = second.getRoot()->getChildCount();

            first.getRoot()->getChild(0)->add(find(second.getRoot(), "S1-T1")->clone());

            CHECK(first.getRoot()->getChild(0)->getChildCount() == 3);
            CHECK(second.getRoot()->getChild(0)->getChildCount() == 2);
            CHECK(second.getRoot()->getChildCount() == before);
        }
        delete def;
    }
}
