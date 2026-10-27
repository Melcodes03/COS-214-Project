#include "IndustrySetup.h"

#include "Task.h"
#include "Stage.h"
#include "PriorityDecorator.h"
#include "WorkflowDefinition.h"
#include "WorkflowInstance.h"
#include "Participant.h"
#include "WorkAllocator.h"
#include "LeastWorkloadAssignment.h"
#include "LevelHandler.h"
#include "ManagerHandler.h"
#include "CommandHistory.h"

// ---------------------------------------------------------------------------
// IndustryScenario
// ---------------------------------------------------------------------------

IndustryScenario::IndustryScenario()
    : definition(nullptr), instance(nullptr),
      allocator(nullptr), history(nullptr) {}

IndustryScenario::~IndustryScenario() {
    delete instance;
    delete definition;
    delete allocator;
    delete history;
    for (size_t i = 0; i < participants.size(); ++i)
        delete participants[i];
}

// Prevent accidental copies – only moves or pointers should be used
IndustryScenario::IndustryScenario(const IndustryScenario&)
    : definition(nullptr), instance(nullptr),
      allocator(nullptr), history(nullptr) {}

IndustryScenario& IndustryScenario::operator=(const IndustryScenario&) {
    return *this;
}

// ---------------------------------------------------------------------------
// Industry 1 – Software customer support
// ---------------------------------------------------------------------------
// Stages: Log  ->  Investigate  ->  Resolve
// Tasks:  "Log the case"  |  "Check logs", "Reproduce bug"  |  "Apply fix", "Close case"
// Chain:  General Assistant (pri<=1) -> Technical Assistant (pri<=2)
//         -> Engineer (pri<=3) -> Support Manager

IndustryScenario IndustrySetup::createSupportDesk() {
    IndustryScenario s;
    s.name = "Software Customer Support";

    // ---- build the workflow tree ----
    Stage* root       = new Stage("SD",  "Support Case");
    Stage* logStage   = new Stage("SD1", "Log");
    Stage* investigate= new Stage("SD2", "Investigate");
    Stage* resolve    = new Stage("SD3", "Resolve");

    logStage->add(new Task("SD1-1", "Log the case"));

    investigate->add(new Task("SD2-1", "Check logs"));
    investigate->add(new Task("SD2-2", "Reproduce bug"));

    resolve->add(new Task("SD3-1", "Apply fix"));
    resolve->add(new Task("SD3-2", "Close case"));

    root->add(logStage);
    root->add(investigate);
    root->add(resolve);

    s.definition = new WorkflowDefinition("Support Case Process", 1, root);
    s.instance   = new WorkflowInstance("SD-I1", s.definition);

    // ---- participants ----
    s.participants.push_back(new Participant("Thandi",  "General Assistant",    3));
    s.participants.push_back(new Participant("Ayanda",  "General Assistant",    3));
    s.participants.push_back(new Participant("Sipho",   "Technical Assistant",  3));
    s.participants.push_back(new Participant("Lerato",  "Engineer",             3));
    s.participants.push_back(new Participant("Naledi",  "Manager",             10));

    // ---- allocator with escalation chain ----
    s.allocator = new WorkAllocator(new LeastWorkloadAssignment());
    for (size_t i = 0; i < s.participants.size(); ++i)
        s.allocator->addParticipant(s.participants[i]);

    LevelHandler* general   = new LevelHandler("General Assistant",    1, new LeastWorkloadAssignment());
    LevelHandler* technical = new LevelHandler("Technical Assistant",  2, new LeastWorkloadAssignment());
    LevelHandler* engineer  = new LevelHandler("Engineer",             3, new LeastWorkloadAssignment());
    general->setNext(technical);
    technical->setNext(engineer);
    // Manager is last in the chain
    engineer->setNext(new ManagerHandler(s.participants.back()));  // Naledi
    s.allocator->setChain(general);

    s.history = new CommandHistory();

    return s;
}

// ---------------------------------------------------------------------------
// Industry 2 – University application process
// ---------------------------------------------------------------------------
// Stages: Submit  ->  Verify Docs  ->  Academic Review  ->  Committee Decision
// Tasks:  "Receive application" | "Check documents", "Verify transcripts"
//         | "Assess grades", "Faculty review" | "Committee vote", "Issue offer/rejection"
// Chain:  Admin Clerk (pri<=1) -> Admissions Officer (pri<=2)
//         -> Department Head (pri<=3) -> Registrar

IndustryScenario IndustrySetup::createUniversityAdmissions() {
    IndustryScenario s;
    s.name = "University Application Process";

    // ---- build the workflow tree ----
    Stage* root       = new Stage("UA",  "Admissions Case");
    Stage* submit     = new Stage("UA1", "Submit");
    Stage* verify     = new Stage("UA2", "Verify Docs");
    Stage* review     = new Stage("UA3", "Academic Review");
    Stage* committee  = new Stage("UA4", "Committee Decision");

    submit->add(new Task("UA1-1", "Receive application"));

    verify->add(new Task("UA2-1", "Check documents"));
    verify->add(new Task("UA2-2", "Verify transcripts"));

    review->add(new Task("UA3-1", "Assess grades"));
    review->add(new Task("UA3-2", "Faculty review"));

    committee->add(new Task("UA4-1", "Committee vote"));
    committee->add(new Task("UA4-2", "Issue offer/rejection"));

    root->add(submit);
    root->add(verify);
    root->add(review);
    root->add(committee);

    s.definition = new WorkflowDefinition("Admissions Process", 1, root);
    s.instance   = new WorkflowInstance("UA-I1", s.definition);

    // ---- participants ----
    s.participants.push_back(new Participant("Zanele",  "Admin Clerk",          3));
    s.participants.push_back(new Participant("Bongani", "Admin Clerk",          3));
    s.participants.push_back(new Participant("Nomsa",   "Admissions Officer",   3));
    s.participants.push_back(new Participant("Tshepo",  "Department Head",      3));
    s.participants.push_back(new Participant("Prof Mokoena", "Registrar",      10));

    // ---- allocator with escalation chain ----
    s.allocator = new WorkAllocator(new LeastWorkloadAssignment());
    for (size_t i = 0; i < s.participants.size(); ++i)
        s.allocator->addParticipant(s.participants[i]);

    LevelHandler* clerk    = new LevelHandler("Admin Clerk",         1, new LeastWorkloadAssignment());
    LevelHandler* officer  = new LevelHandler("Admissions Officer",  2, new LeastWorkloadAssignment());
    LevelHandler* head     = new LevelHandler("Department Head",     3, new LeastWorkloadAssignment());
    clerk->setNext(officer);
    officer->setNext(head);
    head->setNext(new ManagerHandler(s.participants.back()));  // Prof Mokoena
    s.allocator->setChain(clerk);

    s.history = new CommandHistory();

    return s;
}
