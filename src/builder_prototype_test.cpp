/// doctest tests for the Builder (WorkflowBuilder, DefinitionBuilder,
/// OutlineBuilder, the generic WorkflowDirector and WorkflowSpec), the Prototype
/// (clone() and instance isolation) and the support and admissions data.
/// Build and run with: make test_builder

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include <string>
#include <vector>

#include "DefinitionBuilder.h"
#include "OutlineBuilder.h"
#include "WorkflowDirector.h"
#include "WorkflowSpec.h"
#include "DemoData.h"
#include "WorkflowDefinition.h"
#include "WorkflowInstance.h"
#include "WorkItem.h"
#include "WorkItemIterator.h"
#include "Participant.h"
#include "Stage.h"

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

/// Choice rule used by the tests: branch 0 for amounts up to 20000, branch 1 above, -1 if no amount.
int byAmount(const DataMap& data) {
    DataMap::const_iterator found = data.find("amount");
    if (found == data.end()) {
        return -1;
    }
    return std::stoi(found->second) <= 20000 ? 0 : 1;
}

/// Ids of the items currently Available in an instance, e.g. "T1 T2 ".
std::string availableIds(WorkflowInstance& instance) {
    std::string out;
    std::vector<WorkItem*> ready = instance.getAvailableWork();
    for (int i = 0; i < (int)ready.size(); i++) {
        out += ready[i]->getId() + " ";
    }
    return out;
}

/// Builds the support definition through the Director.
WorkflowDefinition* supportDefinition() {
    DefinitionBuilder builder("Support case", 1);
    WorkflowDirector director;
    director.construct(builder, DemoData::makeSupportSpec());
    return builder.getResult();
}

/// Builds any spec through the one generic Director.
WorkflowDefinition* definitionFrom(const WorkflowSpec& spec) {
    DefinitionBuilder builder(spec.name, 1);
    WorkflowDirector director;
    director.construct(builder, spec);
    return builder.getResult();
}

TaskSpec taskSpec(const std::string& name, const std::string& role) {
    TaskSpec t;
    t.name = name;
    t.role = role;
    return t;
}

/// A task for a choice stage: it runs when data[key] <op> value (empty key = default branch).
TaskSpec branchSpec(const std::string& name, const std::string& key, const std::string& op, const std::string& value) {
    TaskSpec t = taskSpec(name, "R");
    t.whenKey = key;
    t.whenOp = op;
    t.whenValue = value;
    return t;
}

/// A one-stage spec with the given mode and tasks.
WorkflowSpec oneStage(Mode mode, const std::vector<TaskSpec>& tasks) {
    StageSpec stage;
    stage.name = "Stage";
    stage.mode = mode;
    stage.tasks = tasks;
    WorkflowSpec spec;
    spec.name = "Spec";
    spec.stages.push_back(stage);
    return spec;
}

/// Ids of the first available tasks of a fresh instance that has one data value set.
std::string firstAvailable(WorkflowDefinition* def, const std::string& key, const std::string& value) {
    WorkflowInstance instance("I", def);
    if (!key.empty()) {
        instance.setData(key, value);
    }
    instance.advance();
    return availableIds(instance);
}

/// Works every available task to completion, advancing until the instance stops.
/// Returns the ids of the tasks that were done, in order, e.g. "S1-T1 S1-T2 ".
std::string runToEnd(WorkflowInstance& instance) {
    std::string done;
    instance.advance();
    for (int guard = 0; guard < 50 && instance.getStatus() == WorkflowInstance::RUNNING; guard++) {
        std::vector<WorkItem*> ready = instance.getAvailableWork();
        for (int i = 0; i < (int)ready.size(); i++) {
            ready[i]->assign();
            ready[i]->start();
            ready[i]->complete();
            done += ready[i]->getId() + " ";
        }
        instance.advance();
    }
    return done;
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

TEST_SUITE("Builder: roles and stage modes") {

    TEST_CASE("addTask keeps the role, even under decorators") {
        DefinitionBuilder builder("Roles", 1);
        builder.addTask("T1", "Plain", "Engineer")
               .addTask("T2", "Decorated", "Claims Handler").withPriority(2).withAudit();
        WorkflowDefinition* def = builder.getResult();
        WorkItem* root = const_cast<WorkItem*>(def->getRoot());

        CHECK(root->getChild(0)->getRequiredRole() == "Engineer");
        CHECK(root->getChild(1)->getRequiredRole() == "Claims Handler");
        WorkItem* copy = root->getChild(1)->clone();
        CHECK(copy->getRequiredRole() == "Claims Handler");   //clone() keeps it too
        delete copy;
        delete def;
    }

    TEST_CASE("a stage is sequential unless told otherwise") {
        DefinitionBuilder builder("Default", 1);
        builder.beginStage("S", "Stage").addTask("A", "A", "R").addTask("B", "B", "R").endStage();
        WorkflowDefinition* def = builder.getResult();
        {
            WorkflowInstance instance("I1", def);
            instance.advance();
            CHECK(availableIds(instance) == "A ");   //B waits for A
        }
        delete def;
    }

    TEST_CASE("inSequence makes the next task wait for the one before it") {
        DefinitionBuilder builder("Sequence", 1);
        builder.beginStage("S", "Stage").inSequence().addTask("A", "A", "R").addTask("B", "B", "R").endStage();
        WorkflowDefinition* def = builder.getResult();
        {
            WorkflowInstance instance("I1", def);
            instance.advance();
            CHECK(availableIds(instance) == "A ");

            WorkItem* a = find(instance.getRoot(), "A");
            CHECK(a->assign());
            CHECK(a->start());
            CHECK(a->complete());
            instance.advance();
            CHECK(availableIds(instance) == "B ");
        }
        delete def;
    }

    TEST_CASE("inParallel opens every task together and joins at the end") {
        DefinitionBuilder builder("Parallel", 1);
        builder.beginStage("S", "Stage").inParallel().addTask("A", "A", "R").addTask("B", "B", "R").endStage();
        WorkflowDefinition* def = builder.getResult();
        {
            WorkflowInstance instance("I1", def);
            instance.advance();
            CHECK(availableIds(instance) == "A B ");

            const char* ids[] = {"A", "B"};
            for (int i = 0; i < 2; i++) {
                WorkItem* t = find(instance.getRoot(), ids[i]);
                CHECK(t->assign());
                CHECK(t->start());
                CHECK(t->complete());
                instance.advance();
                CHECK(instance.getStatus() == (i == 1 ? WorkflowInstance::COMPLETED : WorkflowInstance::RUNNING));
            }
        }
        delete def;
    }

    TEST_CASE("choosingBy runs only the branch the data selects") {
        DefinitionBuilder builder("Choice", 1);
        builder.beginStage("S", "Approval").choosingBy(byAmount)
                   .addTask("SMALL", "Single approval", "R")
                   .addTask("BIG", "Multi-level approval", "R")
               .endStage();
        WorkflowDefinition* def = builder.getResult();
        {
            WorkflowInstance small("I1", def);
            small.setData("amount", "5000");
            small.advance();
            CHECK(availableIds(small) == "SMALL ");

            WorkflowInstance big("I2", def);
            big.setData("amount", "85000");
            big.advance();
            CHECK(availableIds(big) == "BIG ");
        }
        delete def;
    }

    TEST_CASE("choosingBy with no matching branch fails the workflow") {
        DefinitionBuilder builder("NoMatch", 1);
        builder.beginStage("S", "Approval").choosingBy(byAmount).addTask("SMALL", "A", "R").endStage();
        WorkflowDefinition* def = builder.getResult();
        {
            WorkflowInstance instance("I1", def);   //no "amount" data, so the rule picks nothing
            CHECK(instance.advance() == WorkflowInstance::FAILED);
        }
        delete def;
    }

    TEST_CASE("the last mode step wins, and a mode step never disturbs the pending task") {
        DefinitionBuilder builder("Last", 1);
        builder.beginStage("S", "Stage").inParallel().inSequence()
                   .addTask("A", "A", "R").inParallel().withPriority(4)    //mode step between addTask and decorator
                   .addTask("B", "B", "R")
               .endStage();
        WorkflowDefinition* def = builder.getResult();
        WorkItem* root = const_cast<WorkItem*>(def->getRoot());
        CHECK(find(root, "A")->getPriority() == 4);   //the decorator still wrapped A
        {
            WorkflowInstance instance("I1", def);
            instance.advance();
            CHECK(availableIds(instance) == "A B ");   //the stage ended up parallel
        }
        delete def;
    }

    TEST_CASE("mode steps after getResult() are ignored, not a crash") {
        DefinitionBuilder builder("Done", 1);
        builder.beginStage("S", "Stage").addTask("A", "A", "R").endStage();
        WorkflowDefinition* def = builder.getResult();
        builder.inParallel().inSequence().choosingBy(byAmount);
        CHECK(builder.getResult() == nullptr);
        delete def;
    }

    TEST_CASE("OutlineBuilder names the mode on the stage line") {
        OutlineBuilder outline("Modes");
        outline.beginStage("S1", "Intake").inSequence()
                   .addTask("T1", "Register", "Clerk")
               .endStage()
               .beginStage("S2", "Checks").inParallel().inSequence().inParallel()
                   .addTask("T2", "Docs", "Clerk")
               .endStage()
               .beginStage("S3", "Approval").choosingBy(byAmount)
                   .addTask("T3", "Approve", "Manager")
               .endStage()
               .beginStage("S4", "Plain")
               .endStage();
        CHECK(outline.getResult() ==
              "Modes\n"
              "  [S1] Intake (sequential)\n"
              "    - T1 Register (Clerk)\n"
              "  [S2] Checks (parallel)\n"
              "    - T2 Docs (Clerk)\n"
              "  [S3] Approval (choice)\n"
              "    - T3 Approve (Manager)\n"
              "  [S4] Plain\n");
    }
}

TEST_SUITE("Director: Industry 1 support") {

    TEST_CASE("the generic Director builds the five support stages") {
        WorkflowDefinition* def = supportDefinition();
        WorkItem* root = const_cast<WorkItem*>(def->getRoot());

        REQUIRE(root->getChildCount() == 5);
        CHECK(root->getChild(0)->getName() == "Intake");
        CHECK(root->getChild(1)->getName() == "Investigation");
        CHECK(root->getChild(2)->getName() == "Resolution");
        CHECK(root->getChild(3)->getName() == "Closure");
        CHECK(root->getChild(4)->getName() == "Follow-up");   //choice on severity
        CHECK(idsOf(root) == "root S1 S1-T1 S1-T2 S2 S2-T1 S2-T2 S3 S3-T1 S3-T2 S4 S4-T1 S4-T2 S5 S5-T1 S5-T2 ");
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
        WorkflowDirector director;
        WorkflowSpec spec = DemoData::makeSupportSpec();
        director.construct(outline, spec);
        std::string text = outline.getResult();
        CHECK(text.find("[S2] Investigation") != std::string::npos);
        CHECK(text.find("S4-T2 Close support case (Support Manager) [priority 4] [audited]") != std::string::npos);
        CHECK(spec.name == "Software customer support");
    }

    TEST_CASE("the support team covers every role the process needs") {
        std::vector<Participant*> team = DemoData::createSupportTeam();
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
        CHECK(DemoData::sampleSupportCases().size() >= 3);
    }
}

TEST_SUITE("Director: generic WorkflowSpec") {

    TEST_CASE("ids are generated from the stage and task position") {
        StageSpec first;
        first.name = "First";
        first.tasks.push_back(taskSpec("A", "R"));
        first.tasks.push_back(taskSpec("B", "R"));
        StageSpec second;
        second.name = "Second";
        second.tasks.push_back(taskSpec("C", "R"));
        WorkflowSpec spec;
        spec.name = "Ids";
        spec.stages.push_back(first);
        spec.stages.push_back(second);

        WorkflowDefinition* def = definitionFrom(spec);
        CHECK(idsOf(const_cast<WorkItem*>(def->getRoot())) == "root S1 S1-T1 S1-T2 S2 S2-T1 ");
        delete def;
    }

    TEST_CASE("priority and audit from the spec decorate only the tasks that ask for it") {
        TaskSpec plain = taskSpec("Plain", "R");
        TaskSpec decorated = taskSpec("Decorated", "R");
        decorated.priority = 2;
        decorated.audit = true;
        OutlineBuilder outline("Deco");
        WorkflowDirector director;
        std::vector<TaskSpec> tasks;
        tasks.push_back(plain);
        tasks.push_back(decorated);
        director.construct(outline, oneStage(SEQUENTIAL, tasks));
        CHECK(outline.getResult() ==
              "Deco\n"
              "  [S1] Stage (sequential)\n"
              "    - S1-T1 Plain (R)\n"
              "    - S1-T2 Decorated (R) [priority 2] [audited]\n");
    }

    TEST_CASE("the stage mode in the spec decides how the tasks open") {
        std::vector<TaskSpec> tasks;
        tasks.push_back(taskSpec("A", "R"));
        tasks.push_back(taskSpec("B", "R"));

        WorkflowDefinition* sequential = definitionFrom(oneStage(SEQUENTIAL, tasks));
        CHECK(firstAvailable(sequential, "", "") == "S1-T1 ");
        delete sequential;

        WorkflowDefinition* parallel = definitionFrom(oneStage(PARALLEL, tasks));
        CHECK(firstAvailable(parallel, "", "") == "S1-T1 S1-T2 ");
        delete parallel;
    }

    TEST_CASE("a choice stage runs the first task whose condition matches") {
        std::vector<TaskSpec> tasks;
        tasks.push_back(branchSpec("Fast track", "score", ">=", "70"));
        tasks.push_back(branchSpec("Normal", "score", ">=", "50"));
        tasks.push_back(branchSpec("Decline", "", "", ""));          //default branch
        WorkflowDefinition* def = definitionFrom(oneStage(CHOICE, tasks));

        CHECK(firstAvailable(def, "score", "80") == "S1-T1 ");
        CHECK(firstAvailable(def, "score", "70") == "S1-T1 ");       //>= includes the boundary
        CHECK(firstAvailable(def, "score", "60") == "S1-T2 ");
        CHECK(firstAvailable(def, "score", "10") == "S1-T3 ");       //nothing matched: default
        CHECK(firstAvailable(def, "", "") == "S1-T3 ");              //no data at all: default
        delete def;
    }

    TEST_CASE("first match wins when several conditions match") {
        std::vector<TaskSpec> tasks;
        tasks.push_back(branchSpec("One", "n", ">", "1"));
        tasks.push_back(branchSpec("Two", "n", ">", "0"));
        WorkflowDefinition* def = definitionFrom(oneStage(CHOICE, tasks));
        CHECK(firstAvailable(def, "n", "5") == "S1-T1 ");
        delete def;
    }

    TEST_CASE("a choice stage with no default and no match fails the workflow") {
        std::vector<TaskSpec> tasks;
        tasks.push_back(branchSpec("Only", "score", ">=", "70"));
        WorkflowDefinition* def = definitionFrom(oneStage(CHOICE, tasks));
        {
            WorkflowInstance instance("I", def);
            instance.setData("score", "10");
            CHECK(instance.advance() == WorkflowInstance::FAILED);
        }
        delete def;
    }

    TEST_CASE("numbers compare as numbers, other values as text") {
        std::vector<TaskSpec> numeric;
        numeric.push_back(branchSpec("Big", "n", ">", "9"));
        numeric.push_back(branchSpec("Small", "", "", ""));
        WorkflowDefinition* numbers = definitionFrom(oneStage(CHOICE, numeric));
        CHECK(firstAvailable(numbers, "n", "10") == "S1-T1 ");       //as text, "10" < "9"
        delete numbers;

        std::vector<TaskSpec> text;
        text.push_back(branchSpec("Gold", "tier", "==", "gold"));
        text.push_back(branchSpec("Other", "tier", "!=", "gold"));
        WorkflowDefinition* words = definitionFrom(oneStage(CHOICE, text));
        CHECK(firstAvailable(words, "tier", "gold") == "S1-T1 ");
        CHECK(firstAvailable(words, "tier", "silver") == "S1-T2 ");
        delete words;
    }

    TEST_CASE("an unknown operator never matches") {
        std::vector<TaskSpec> tasks;
        tasks.push_back(branchSpec("Never", "n", "~=", "1"));
        tasks.push_back(branchSpec("Default", "", "", ""));
        WorkflowDefinition* def = definitionFrom(oneStage(CHOICE, tasks));
        CHECK(firstAvailable(def, "n", "1") == "S1-T2 ");
        delete def;
    }

    TEST_CASE("an empty spec builds an empty definition") {
        WorkflowSpec spec;
        spec.name = "Empty";
        WorkflowDefinition* def = definitionFrom(spec);
        REQUIRE(def != nullptr);
        CHECK(def->getRoot()->getChildCount() == 0);
        delete def;
    }
}

TEST_SUITE("Director: Industry 2 admissions") {

    TEST_CASE("the same Director builds the five admissions stages") {
        WorkflowSpec spec = DemoData::makeAdmissionsSpec();
        WorkflowDefinition* def = definitionFrom(spec);
        WorkItem* root = const_cast<WorkItem*>(def->getRoot());

        CHECK(spec.name == "University admissions");
        REQUIRE(root->getChildCount() == 5);
        CHECK(root->getChild(0)->getName() == "Intake");
        CHECK(root->getChild(1)->getName() == "Verification");
        CHECK(root->getChild(2)->getName() == "Assessment");
        CHECK(root->getChild(3)->getName() == "Decision");
        CHECK(root->getChild(4)->getName() == "Enrolment");
        CHECK(idsOf(root) == "root S1 S1-T1 S1-T2 S2 S2-T1 S2-T2 S3 S3-T1 S3-T2 "
                             "S4 S4-T1 S4-T2 S4-T3 S5 S5-T1 S5-T2 ");
        CHECK(find(root, "S4-T1")->getRequiredRole() == "Faculty Reviewer");   //prepares the offer; the Registrar signs it off
        delete def;
    }

    TEST_CASE("the outline names the verification and decision modes") {
        OutlineBuilder outline("University admissions");
        WorkflowDirector director;
        director.construct(outline, DemoData::makeAdmissionsSpec());
        std::string text = outline.getResult();
        CHECK(text.find("[S2] Verification (parallel)") != std::string::npos);
        CHECK(text.find("[S4] Decision (choice)") != std::string::npos);
        CHECK(text.find("S4-T1 Issue offer of admission (Faculty Reviewer) [priority 3] [audited]") != std::string::npos);
    }

    TEST_CASE("every sample application takes its own path through the decision") {
        WorkflowDefinition* def = definitionFrom(DemoData::makeAdmissionsSpec());
        std::vector<DemoData::AdmissionsApplication> applications = DemoData::sampleApplications();
        REQUIRE(applications.size() >= 3);
        {
            WorkflowInstance offer(applications[0].applicationId, def);          //score 78
            offer.setData("score", std::to_string(applications[0].score));
            std::string done = runToEnd(offer);
            CHECK(offer.getStatus() == WorkflowInstance::COMPLETED);
            CHECK(done.find("S4-T1 ") != std::string::npos);                     //offer
            CHECK(done.find("S4-T2 ") == std::string::npos);
            CHECK(done.find("S4-T3 ") == std::string::npos);
            CHECK(done.find("S5-T1 ") != std::string::npos);                     //enrolled
            CHECK(done.find("S5-T2 ") == std::string::npos);

            WorkflowInstance waiting(applications[1].applicationId, def);        //score 62
            waiting.setData("score", std::to_string(applications[1].score));
            done = runToEnd(waiting);
            CHECK(waiting.getStatus() == WorkflowInstance::COMPLETED);
            CHECK(done.find("S4-T2 ") != std::string::npos);                     //waiting list
            CHECK(done.find("S4-T1 ") == std::string::npos);
            CHECK(done.find("S5-T2 ") != std::string::npos);                     //archived, not enrolled
            CHECK(done.find("S5-T1 ") == std::string::npos);

            WorkflowInstance rejected(applications[2].applicationId, def);       //score 48
            rejected.setData("score", std::to_string(applications[2].score));
            done = runToEnd(rejected);
            CHECK(rejected.getStatus() == WorkflowInstance::COMPLETED);
            CHECK(done.find("S4-T3 ") != std::string::npos);                     //rejection (default branch)
            CHECK(done.find("S5-T2 ") != std::string::npos);
            CHECK(done.find("S5-T1 ") == std::string::npos);
        }
        delete def;
    }

    TEST_CASE("the admissions office covers every role the process needs") {
        std::vector<Participant*> team = DemoData::createAdmissionsTeam();
        const char* roles[] = {"Admissions Clerk", "Verification Officer", "Faculty Reviewer", "Registrar"};
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