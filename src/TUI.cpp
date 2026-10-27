#include "TUI.h"
#include "IndustrySetup.h"

#include "WorkItem.h"
#include "Task.h"
#include "Stage.h"
#include "WorkflowInstance.h"
#include "WorkflowDefinition.h"
#include "WorkItemIterator.h"
#include "Participant.h"
#include "WorkAllocator.h"
#include "CommandHistory.h"
#include "WorkerCommand.h"
#include "AllocationCommand.h"
#include "TransitionCommand.h"
#include "ReportScreen.h"
#include "Observer.h"
#include "WorkItemState.h"
#include "LeastWorkloadAssignment.h"
#include "LevelHandler.h"
#include "ManagerHandler.h"

#include <algorithm>
#include <cctype>
#include <iomanip>

//colours
#define RESET   "\033[0m"
#define RED     "\033[31m"      // Standard Red
#define GREEN   "\033[32m"      // Standard Green
#define YELLOW  "\033[33m"      // Standard Yellow
#define BLUE    "\033[34m"      // Standard Blue
#define MAGENTA "\033[35m"      // Standard Magenta
#define CYAN    "\033[36m"      // Standard Cyan
#define BOLD    "\033[1m"       // Bold

// ---------------------------------------------------------------------------
// TUIObserver – prints inline notifications
// ---------------------------------------------------------------------------

TUIObserver::TUIObserver(WorkItem* item, std::ostream& out)
    : Observer(item), out(out) {}

TUIObserver::~TUIObserver() {}

void TUIObserver::update(WorkItemState* newState, std::string oldState) {
    out << "════════════════════════════════════NOTIFICTION══════════════════════════════════════════\n";
    out << CYAN << "  >> [Observer] " << RESET << subject->getId() << " (" << subject->getName()
        << "): " << RED << oldState << RESET << " -> " << GREEN << newState->getName() << RESET << "\n";
    out << "═════════════════════════════════════════════════════════════════════════════════════════\n";
}

// ---------------------------------------------------------------------------
// TUI
// ---------------------------------------------------------------------------

TUI::TUI(std::istream& in, std::ostream& out)
    : in(in), out(out), scenario(nullptr) {}

TUI::~TUI() {
    detachObservers();
    delete scenario;
}

int TUI::run() {
    showBanner();

    int choice = pickIndustry();
    if (choice == 0) return 0;

    // Build the selected industry
    IndustryScenario* s = nullptr;
    if (choice == 1) {
        s = new IndustryScenario();
        IndustryScenario built = IndustrySetup::createSupportDesk();
        // Move ownership
        s->name        = built.name;
        s->definition  = built.definition;  built.definition = nullptr;
        s->instance    = built.instance;    built.instance   = nullptr;
        s->allocator   = built.allocator;   built.allocator  = nullptr;
        s->history     = built.history;     built.history    = nullptr;
        s->participants= built.participants; built.participants.clear();
    } else if (choice == 2) {
        s = new IndustryScenario();
        IndustryScenario built = IndustrySetup::createUniversityAdmissions();
        s->name        = built.name;
        s->definition  = built.definition;  built.definition = nullptr;
        s->instance    = built.instance;    built.instance   = nullptr;
        s->allocator   = built.allocator;   built.allocator  = nullptr;
        s->history     = built.history;     built.history    = nullptr;
        s->participants= built.participants; built.participants.clear();
    } else if (choice == 3) {
        s = buildCustomWorkflow();
    }
    if (!s) return 0;
    scenario = s;

    out << "\n  Industry loaded: " << scenario->name << "\n";
    out << "  Instance: " << scenario->instance->getId() << "\n";
    out << "  Participants: ";
    for (size_t i = 0; i < scenario->participants.size(); ++i) {
        if (i > 0) out << ", ";
        out << scenario->participants[i]->getName()
            << " (" << scenario->participants[i]->getRole() << ")";
    }
    out << "\n";

    // Attach inline observers to all leaf tasks
    attachObservers();

    // Make all tasks available (Created -> Available)
    WorkItem* root = scenario->instance->getRoot();
    WorkItemIterator* it = root->createIterator();
    for (it->first(); !it->isDone(); it->next()) {
        WorkItem* item = it->currentItem();
        if (item->getChildCount() == 0) {  // leaf task
            item->makeAvailable();
        }
    }
    delete it;

    mainMenu();
    return 0;
}

// ---------------------------------------------------------------------------
// Screens
// ---------------------------------------------------------------------------

void TUI::showBanner() {
    printSeparator();
    out << BOLD << R"(
  ╔════════════════════════════════════════════════╗
  ║       TIVIDY  Workflow Management System       ║
  ║            Text User Interface (TUI)           ║
  ╚════════════════════════════════════════════════╝
)" << RESET << "\n";
}

int TUI::pickIndustry() {
    printHeader("Select Industry");
    out << BLUE "  1) Software Customer Support\n";
    out << "  2) University Application Process\n";
    out << "  3) Custom Workflow (Build from scratch)\n";
    out << "  0) Quit\n" << RESET;
    std::string choice = prompt("Industry");
    if (choice == "1") return 1;
    if (choice == "2") return 2;
    if (choice == "3" || choice == "custom") return 3;
    return 0;
}

void TUI::mainMenu() {
    bool running = true;
    while (running) {
        printSeparator();
        printHeader("Main Menu  [" + scenario->name + "]");
        out << BLUE << "  1) View work tree          "<< RESET <<"tree\n";
        out << BLUE <<"  2) View worklists          "<< RESET <<"worklists\n";
        out << BLUE <<"  3) Allocate a task         "<< RESET <<"allocate\n";
        out << BLUE <<"  4) Start a task            "<< RESET <<"start <id>\n";
        out << BLUE <<"  5) Complete a task         "<< RESET <<"complete <id>\n";
        out << BLUE <<"  6) Decline a task          "<< RESET <<"decline <id>\n";
        out << BLUE <<"  7) Escalate a task         "<< RESET <<"escalate <id>\n";
        out << BLUE <<"  8) Cancel a task           "<< RESET <<"cancel <id>\n";
        out << BLUE <<"  9) Progress report         "<< RESET <<"progress\n";
        out << BLUE <<" 10) Escalation report       "<< RESET <<"escalations\n";
        out << BLUE <<" 11) Command history         "<< RESET <<"history\n";
        out << BLUE <<" 12) Undo                    "<< RESET <<"undo\n";
        out << BLUE <<" 13) Redo                    "<< RESET <<"redo\n";
        out << BLUE <<" 14) Rollback                "<< RESET <<"rollback\n";
        out << BLUE <<" 15) Switch industry         "<< RESET <<"switch\n";
        out << BLUE <<"  0) Quit                    "<< RESET <<"quit\n" << RESET;

        std::string choice = prompt(">");

        // Normalise: if it starts with a digit, use the number
        if (choice == "0" || choice == "quit" || choice == "exit") {
            running = false;
        } else {
            handleMainMenuChoice(choice);
        }
    }
    out << "\n  Goodbye.\n";
}

void TUI::handleMainMenuChoice(const std::string& raw) {
    // Parse: could be a number, a command, or "command arg"
    std::istringstream ss(raw);
    std::string cmd;
    ss >> cmd;
    std::string arg;
    ss >> arg;  // may be empty

    if (cmd == "1" || cmd == "tree") {
        showWorkTree();
    } else if (cmd == "2" || cmd == "worklists") {
        showWorklists();
    } else if (cmd == "3" || cmd == "allocate") {
        if (!arg.empty()) {
            // Direct allocate with id
            WorkItem* item = findTask(arg);
            if (!item) { out << "  No task with id \"" << arg << "\".\n"; return; }
            Participant* who = scenario->allocator->allocate(*item);
            if (who) {
                who->receive(item);
                out << "  Allocated " << item->getId() << " (" << item->getName()
                    << ") to " << who->getName() << " (" << who->getRole() << ")\n";
            } else {
                out << "  Nobody can take " << item->getId() << " right now.\n";
            }
        } else {
            allocateTask();
        }
    } else if (cmd == "4" || cmd == "start") {
        if (!arg.empty()) {
            // Shortcut: start <id>
            WorkItem* item = findTask(arg);
            if (!item) { out << "  No task with id \"" << arg << "\".\n"; return; }
            // Find who holds it
            Participant* who = scenario->allocator->getAssignee(item->getId());
            if (!who) { out << "  " << arg << " is not assigned to anyone.\n"; return; }
            if (who->start(item, *scenario->history)) {
                out << "  " << who->getName() << " started " << item->getId() << ".\n";
            } else {
                out << "  Cannot start " << arg << " (state: " << item->getStateName() << ").\n";
            }
        } else {
            startTask();
        }
    } else if (cmd == "5" || cmd == "complete") {
        if (!arg.empty()) {
            WorkItem* item = findTask(arg);
            if (!item) { out << "  No task with id \"" << arg << "\".\n"; return; }
            Participant* who = scenario->allocator->getAssignee(item->getId());
            if (!who) { out << "  " << arg << " is not assigned to anyone.\n"; return; }
            if (who->complete(item, *scenario->history)) {
                scenario->allocator->release(item->getId());
                out << "  " << who->getName() << " completed " << item->getId() << ".\n";
            } else {
                out << "  Cannot complete " << arg << " (state: " << item->getStateName() << ").\n";
            }
        } else {
            completeTask();
        }
    } else if (cmd == "6" || cmd == "decline") {
        if (!arg.empty()) {
            WorkItem* item = findTask(arg);
            if (!item) { out << "  No task with id \"" << arg << "\".\n"; return; }
            Participant* who = scenario->allocator->getAssignee(item->getId());
            if (!who) { out << "  " << arg << " is not assigned to anyone.\n"; return; }
            std::string oldHolder = who->getName();
            if (who->decline(item, *scenario->history)) {
                Participant* next = scenario->allocator->reassign(*item);
                if (next) {
                    next->receive(item);
                    out << "  " << oldHolder << " declined " << item->getId()
                        << ". Reassigned to " << next->getName()
                        << " (" << next->getRole() << ").\n";
                } else {
                    out << "  " << oldHolder << " declined " << item->getId()
                        << ". Nobody above could take it.\n";
                }
            } else {
                out << "  Cannot decline " << arg << " (state: " << item->getStateName() << ").\n";
            }
        } else {
            declineTask();
        }
    } else if (cmd == "7" || cmd == "escalate") {
        if (!arg.empty()) {
            WorkItem* item = findTask(arg);
            if (!item) { out << "  No task with id \"" << arg << "\".\n"; return; }
            std::string oldHolder = "";
            Participant* prev = scenario->allocator->getAssignee(item->getId());
            if (prev) oldHolder = prev->getName();
            Participant* next = scenario->allocator->escalate(*item);
            if (next) {
                next->receive(item);
                out << "  Escalated " << item->getId() << " from " << oldHolder
                    << " to " << next->getName() << " (" << next->getRole() << ").\n";
            } else {
                out << "  Cannot escalate " << arg << ".\n";
            }
        } else {
            escalateTask();
        }
    } else if (cmd == "8" || cmd == "cancel") {
        if (!arg.empty()) {
            WorkItem* item = findTask(arg);
            if (!item) { out << "  No task with id \"" << arg << "\".\n"; return; }
            if (item->cancel()) {
                scenario->allocator->release(item->getId());
                out << "  Cancelled " << item->getId() << ".\n";
            } else {
                out << "  Cannot cancel " << arg << " (state: " << item->getStateName() << ").\n";
            }
        } else {
            cancelTask();
        }
    } else if (cmd == "9" || cmd == "progress") {
        showProgress();
    } else if (cmd == "10" || cmd == "escalations") {
        showEscalations();
    } else if (cmd == "11" || cmd == "history") {
        showHistory();
    } else if (cmd == "12" || cmd == "undo") {
        undoAction();
    } else if (cmd == "13" || cmd == "redo") {
        redoAction();
    } else if (cmd == "14" || cmd == "rollback") {
        rollbackAction();
    } else if (cmd == "15" || cmd == "switch") {
        // Switch industry: teardown and restart
        detachObservers();
        delete scenario;
        scenario = nullptr;
        int choice = pickIndustry();
        if (choice == 0) return;
        IndustryScenario* s2 = nullptr;
        if (choice == 1) {
            s2 = new IndustryScenario();
            IndustryScenario built = IndustrySetup::createSupportDesk();
            s2->name = built.name; s2->definition = built.definition; built.definition = nullptr;
            s2->instance = built.instance; built.instance = nullptr;
            s2->allocator = built.allocator; built.allocator = nullptr;
            s2->history = built.history; built.history = nullptr;
            s2->participants = built.participants; built.participants.clear();
        } else if (choice == 2) {
            s2 = new IndustryScenario();
            IndustryScenario built = IndustrySetup::createUniversityAdmissions();
            s2->name = built.name; s2->definition = built.definition; built.definition = nullptr;
            s2->instance = built.instance; built.instance = nullptr;
            s2->allocator = built.allocator; built.allocator = nullptr;
            s2->history = built.history; built.history = nullptr;
            s2->participants = built.participants; built.participants.clear();
        } else if (choice == 3) {
            s2 = buildCustomWorkflow();
        }
        if (!s2) return;
        scenario = s2;
        attachObservers();
        // Make tasks available
        WorkItem* root = scenario->instance->getRoot();
        WorkItemIterator* it = root->createIterator();
        for (it->first(); !it->isDone(); it->next()) {
            WorkItem* item = it->currentItem();
            if (item->getChildCount() == 0) item->makeAvailable();
        }
        delete it;
        out << "  Switched to " << scenario->name << ".\n";
    } else {
        out << "  Unknown command: \"" << raw << "\". Type a number or command name.\n";
    }
}

// ---------------------------------------------------------------------------
// Work tree display
// ---------------------------------------------------------------------------

void TUI::showWorkTree() {
    printHeader("Work Tree: " + scenario->instance->getId());
    out << GREEN;
    showWorkTreeRecursive(scenario->instance->getRoot(), 0);
    out << RESET;
    out << "\n";
}

void TUI::showWorkTreeRecursive(WorkItem* item, int depth) {
    for (int i = 0; i < depth; ++i) out << "    ";
    bool isStage = (item->getChildCount() > 0);
    out << (isStage ? "[Stage] " : "[Task]  ")
        << item->getId() << "  " << item->getName()
        << "  {" << item->getStateName() << "}";

    // Show who holds it
    Participant* holder = scenario->allocator->getAssignee(item->getId());
    if (holder) {
        out << RESET << "  -> " << holder->getName();
    }
    out << "\n";

    for (int c = 0; c < item->getChildCount(); ++c) {
        out << GREEN;
        showWorkTreeRecursive(item->getChild(c), depth + 1);
        out << RESET;
    }
}

// ---------------------------------------------------------------------------
// Worklists
// ---------------------------------------------------------------------------

void TUI::showWorklists() {
    printHeader("Worklists");
    for (size_t i = 0; i < scenario->participants.size(); ++i) {
        showParticipantWorklist(scenario->participants[i]);
    }
}

void TUI::showParticipantWorklist(Participant* p) {
    out << "  " << p->getName() << " (" << p->getRole()
        << ") [" << p->getActiveTasks() << "/" << p->getCapacity() << " active]";
    if (!p->isAvailable()) out << " OFFLINE";
    out << "\n";

    std::vector<WorkItem*>& wl = p->getWorklist();
    if (wl.empty()) {
        out << "    (no work items)\n";
    } else {
        for (size_t j = 0; j < wl.size(); ++j) {
            out << "    - " << wl[j]->getId() << "  " << wl[j]->getName()
                << "  {" << wl[j]->getStateName() << "}\n";
        }
    }
}

// ---------------------------------------------------------------------------
// Actions (menu-driven, prompted)
// ---------------------------------------------------------------------------

void TUI::allocateTask() {
    printHeader("Allocate Task");
    showWorkTree();
    std::string id = prompt("Task id to allocate");
    WorkItem* item = findTask(id);
    if (!item) { out << "  No task with id \"" << id << "\".\n"; return; }

    Participant* who = scenario->allocator->allocate(*item);
    if (who) {
        who->receive(item);
        out << "  Allocated " << item->getId() << " (" << item->getName()
            << ") to " << who->getName() << " (" << who->getRole() << ").\n";
    } else {
        out << "  Nobody can take " << item->getId() << " right now.\n";
    }
}

void TUI::startTask() {
    printHeader("Start Task");
    std::string id = prompt("Task id to start");
    WorkItem* item = findTask(id);
    if (!item) { out << "  No task with id \"" << id << "\".\n"; return; }
    Participant* who = scenario->allocator->getAssignee(item->getId());
    if (!who) { out << "  " << id << " is not assigned to anyone.\n"; return; }
    if (who->start(item, *scenario->history)) {
        out << "  " << who->getName() << " started " << item->getId() << ".\n";
    } else {
        out << "  Cannot start " << id << " (state: " << item->getStateName() << ").\n";
    }
}

void TUI::completeTask() {
    printHeader("Complete Task");
    std::string id = prompt("Task id to complete");
    WorkItem* item = findTask(id);
    if (!item) { out << "  No task with id \"" << id << "\".\n"; return; }
    Participant* who = scenario->allocator->getAssignee(item->getId());
    if (!who) { out << "  " << id << " is not assigned to anyone.\n"; return; }
    if (who->complete(item, *scenario->history)) {
        scenario->allocator->release(item->getId());
        out << "  " << who->getName() << " completed " << item->getId() << ".\n";
    } else {
        out << "  Cannot complete " << id << " (state: " << item->getStateName() << ").\n";
    }
}

void TUI::declineTask() {
    printHeader("Decline Task");
    std::string id = prompt("Task id to decline");
    WorkItem* item = findTask(id);
    if (!item) { out << "  No task with id \"" << id << "\".\n"; return; }
    Participant* who = scenario->allocator->getAssignee(item->getId());
    if (!who) { out << "  " << id << " is not assigned to anyone.\n"; return; }
    std::string oldHolder = who->getName();
    if (who->decline(item, *scenario->history)) {
        Participant* next = scenario->allocator->reassign(*item);
        if (next) {
            next->receive(item);
            out << "  " << oldHolder << " declined " << item->getId()
                << ". Reassigned to " << next->getName()
                << " (" << next->getRole() << ").\n";
        } else {
            out << "  " << oldHolder << " declined " << item->getId()
                << ". Nobody above could take it.\n";
        }
    } else {
        out << "  Cannot decline " << id << " (state: " << item->getStateName() << ").\n";
    }
}

void TUI::escalateTask() {
    printHeader("Escalate Task");
    std::string id = prompt("Task id to escalate");
    WorkItem* item = findTask(id);
    if (!item) { out << "  No task with id \"" << id << "\".\n"; return; }
    std::string oldHolder = "";
    Participant* prev = scenario->allocator->getAssignee(item->getId());
    if (prev) oldHolder = prev->getName();
    Participant* next = scenario->allocator->escalate(*item);
    if (next) {
        next->receive(item);
        out << "  Escalated " << item->getId() << " from " << oldHolder
            << " to " << next->getName() << " (" << next->getRole() << ").\n";
    } else {
        out << "  Cannot escalate " << id << ".\n";
    }
}

void TUI::cancelTask() {
    printHeader("Cancel Task");
    std::string id = prompt("Task id to cancel");
    WorkItem* item = findTask(id);
    if (!item) { out << "  No task with id \"" << id << "\".\n"; return; }
    if (item->cancel()) {
        scenario->allocator->release(item->getId());
        out << "  Cancelled " << item->getId() << ".\n";
    } else {
        out << "  Cannot cancel " << id << " (state: " << item->getStateName() << ").\n";
    }
}

// ---------------------------------------------------------------------------
// Reports
// ---------------------------------------------------------------------------

void TUI::showProgress() {
    printHeader("Progress Report");
    ReportScreen::showProgress(*scenario->instance->getRoot(), out);
    out << "\n";
}

void TUI::showEscalations() {
    printHeader("Escalation Report");
    ReportScreen::showEscalations(*scenario->instance->getRoot(), out);
    out << "\n";
}

// ---------------------------------------------------------------------------
// History, Undo, Redo, Rollback
// ---------------------------------------------------------------------------

void TUI::showHistory() {
    printHeader("Command History");
    std::vector<std::string> hist = scenario->history->getHistory();
    if (hist.empty()) {
        out << "  (no commands yet)\n";
    } else {
        for (size_t i = 0; i < hist.size(); ++i) {
            out << "  " << (i + 1) << ". " << hist[i] << "\n";
        }
    }
    out << "  Undo stack: " << scenario->history->undoCount()
        << "  Redo stack: " << scenario->history->redoCount() << "\n";

    std::vector<std::string> redoList = scenario->history->getRedoList();
    if (!redoList.empty()) {
        out << "  Redo available:\n";
        for (size_t i = 0; i < redoList.size(); ++i) {
            out << "    " << (i + 1) << ". " << redoList[i] << "\n";
        }
    }
}

void TUI::undoAction() {
    if (scenario->history->undo()) {
        out << "  Undone: " << scenario->history->lastCommandName() << "\n";
    } else {
        out << "  Nothing to undo.\n";
    }
}

void TUI::redoAction() {
    if (scenario->history->redo()) {
        out << "  Redone.\n";
    } else {
        out << "  Nothing to redo.\n";
    }
}

void TUI::rollbackAction() {
    printHeader("Rollback");
    out << "  Current history size: " << scenario->history->undoCount() << "\n";
    std::string markStr = prompt("Rollback to marker (0 = beginning)");
    std::istringstream ms(markStr);
    int marker = 0;
    ms >> marker;
    int count = scenario->history->rollbackTo(marker);
    out << "  Rolled back " << count << " command(s).\n";
}

// ---------------------------------------------------------------------------
// Utilities
// ---------------------------------------------------------------------------

std::string TUI::prompt(const std::string& msg) {
    out << "  " << msg << ": ";
    out.flush();
    std::string line;
    if (!std::getline(in, line)) return "";
    // Trim
    size_t start = line.find_first_not_of(" \t\r\n");
    size_t end   = line.find_last_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    return line.substr(start, end - start + 1);
}

WorkItem* TUI::findTask(const std::string& id) {
    return ReportScreen::find(*scenario->instance->getRoot(), id);
}

Participant* TUI::findParticipant(const std::string& name) {
    for (size_t i = 0; i < scenario->participants.size(); ++i) {
        if (scenario->participants[i]->getName() == name)
            return scenario->participants[i];
    }
    return nullptr;
}

void TUI::attachObservers() {
    WorkItem* root = scenario->instance->getRoot();
    WorkItemIterator* it = root->createIterator();
    for (it->first(); !it->isDone(); it->next()) {
        WorkItem* item = it->currentItem();
        TUIObserver* obs = new TUIObserver(item, out);
        observers.push_back(obs);
    }
    delete it;
}

void TUI::detachObservers() {
    // Detach all observers we attached — safe even if the tree has been deleted
    // because we only call detach while the scenario is still alive
    if (scenario && scenario->instance) {
        WorkItem* root = scenario->instance->getRoot();
        WorkItemIterator* it = root->createIterator();
        int obsIdx = 0;
        for (it->first(); !it->isDone(); it->next()) {
            WorkItem* item = it->currentItem();
            if (obsIdx < (int)observers.size()) {
                item->detach(observers[obsIdx]);
                ++obsIdx;
            }
        }
        delete it;
    }
    for (size_t i = 0; i < observers.size(); ++i)
        delete observers[i];
    observers.clear();
}

void TUI::printSeparator() {
    out << "  ------------------------------------------------\n";
}

void TUI::printHeader(const std::string& title) {
    out << "\n  === " << title << " ===\n\n";
}

IndustryScenario* TUI::buildCustomWorkflow() {
    printHeader("Create Custom Workflow");
    out << "  Build your own workflow step-by-step.\n\n";

    IndustryScenario* s = new IndustryScenario();

    // 1. Workflow Name & ID
    std::string wfName = prompt("Workflow Name [default: Custom Workflow]");
    if (wfName.empty()) wfName = "Custom Workflow";
    s->name = wfName;

    std::string wfId = prompt("Workflow ID prefix [default: CW]");
    if (wfId.empty()) wfId = "CW";

    Stage* root = new Stage(wfId, wfName);

    // 2. Stages
    std::string stagesStr = prompt("Number of stages (1-10) [default: 2]");
    int numStages = 2;
    if (!stagesStr.empty()) {
        try { numStages = std::stoi(stagesStr); } catch (...) { numStages = 2; }
    }
    if (numStages < 1) numStages = 1;
    if (numStages > 10) numStages = 10;

    for (int i = 1; i <= numStages; ++i) {
        out << "\n  --- Stage " << i << " of " << numStages << " ---\n";
        std::string defStageId = wfId + std::to_string(i);
        std::string stageId = prompt("Stage ID [default: " + defStageId + "]");
        if (stageId.empty()) stageId = defStageId;

        std::string defStageName = "Stage " + std::to_string(i);
        std::string stageName = prompt("Stage Name [default: " + defStageName + "]");
        if (stageName.empty()) stageName = defStageName;

        Stage* stage = new Stage(stageId, stageName);

        std::string mode = prompt("Execution mode: 1) Sequential  2) Parallel [default: 1]");
        if (mode == "2" || mode == "parallel") {
            stage->setParallel();
        } else {
            stage->setSequential();
        }

        std::string tasksStr = prompt("Number of tasks in this stage (1-10) [default: 2]");
        int numTasks = 2;
        if (!tasksStr.empty()) {
            try { numTasks = std::stoi(tasksStr); } catch (...) { numTasks = 2; }
        }
        if (numTasks < 1) numTasks = 1;
        if (numTasks > 10) numTasks = 10;

        for (int j = 1; j <= numTasks; ++j) {
            std::string defTaskId = stageId + "-" + std::to_string(j);
            std::string taskId = prompt("  Task " + std::to_string(j) + " ID [default: " + defTaskId + "]");
            if (taskId.empty()) taskId = defTaskId;

            std::string defTaskName = "Task " + taskId;
            std::string taskName = prompt("  Task " + std::to_string(j) + " Name [default: " + defTaskName + "]");
            if (taskName.empty()) taskName = defTaskName;

            std::string reqRole = prompt("  Required Role (press Enter for any role)");

            Task* task = new Task(taskId, taskName);
            if (!reqRole.empty()) {
                task->setRequiredRole(reqRole);
            }
            stage->add(task);
        }

        root->add(stage);
    }

    s->definition = new WorkflowDefinition(wfName, 1, root);
    s->instance   = new WorkflowInstance(wfId + "-I1", s->definition);

    // 3. Participants
    out << "\n  --- Participants ---\n";
    std::string partStr = prompt("Number of participants (1-10) [default: 3]");
    int numParts = 3;
    if (!partStr.empty()) {
        try { numParts = std::stoi(partStr); } catch (...) { numParts = 3; }
    }
    if (numParts < 1) numParts = 1;
    if (numParts > 10) numParts = 10;

    for (int k = 1; k <= numParts; ++k) {
        out << "\n  Participant " << k << " of " << numParts << ":\n";
        std::string defName = "User" + std::to_string(k);
        std::string pName = prompt("  Name [default: " + defName + "]");
        if (pName.empty()) pName = defName;

        std::string defRole = (k == numParts && numParts > 1) ? "Manager" : "Staff";
        std::string pRole = prompt("  Role [default: " + defRole + "]");
        if (pRole.empty()) pRole = defRole;

        std::string defCap = (pRole == "Manager") ? "10" : "3";
        std::string capStr = prompt("  Capacity (max tasks) [default: " + defCap + "]");
        int cap = 3;
        if (!capStr.empty()) {
            try { cap = std::stoi(capStr); } catch (...) { cap = (pRole == "Manager") ? 10 : 3; }
        } else {
            cap = (pRole == "Manager") ? 10 : 3;
        }
        if (cap < 1) cap = 1;

        s->participants.push_back(new Participant(pName, pRole, cap));
    }

    // 4. Allocator and Escalation Chain
    s->allocator = new WorkAllocator(new LeastWorkloadAssignment());
    for (size_t i = 0; i < s->participants.size(); ++i) {
        s->allocator->addParticipant(s->participants[i]);
    }

    // Determine manager participant (if any participant has a managerial role, or fallback to last)
    Participant* managerParticipant = nullptr;
    for (size_t i = 0; i < s->participants.size(); ++i) {
        std::string r = s->participants[i]->getRole();
        std::string lowerRole = r;
        for (size_t c = 0; c < lowerRole.size(); ++c)
            lowerRole[c] = (char)::tolower((unsigned char)lowerRole[c]);
        if (lowerRole.find("manager") != std::string::npos ||
            lowerRole.find("head") != std::string::npos ||
            lowerRole.find("lead") != std::string::npos ||
            lowerRole.find("director") != std::string::npos) {
            managerParticipant = s->participants[i];
            break;
        }
    }
    if (!managerParticipant && s->participants.size() > 1) {
        managerParticipant = s->participants.back();
    }

    // Collect distinct roles (excluding managerParticipant if manager is handled via ManagerHandler)
    std::vector<std::string> roles;
    for (size_t i = 0; i < s->participants.size(); ++i) {
        if (s->participants[i] == managerParticipant && s->participants.size() > 1) continue;
        const std::string& r = s->participants[i]->getRole();
        if (std::find(roles.begin(), roles.end(), r) == roles.end()) {
            roles.push_back(r);
        }
    }

    AssignmentHandler* headHandler = nullptr;
    AssignmentHandler* currentHandler = nullptr;
    int pri = 1;
    for (size_t i = 0; i < roles.size(); ++i) {
        LevelHandler* lh = new LevelHandler(roles[i], pri++, new LeastWorkloadAssignment());
        if (!headHandler) {
            headHandler = lh;
            currentHandler = lh;
        } else {
            currentHandler->setNext(lh);
            currentHandler = lh;
        }
    }

    if (managerParticipant) {
        ManagerHandler* mh = new ManagerHandler(managerParticipant);
        if (currentHandler) {
            currentHandler->setNext(mh);
        } else {
            headHandler = mh;
        }
    }
    s->allocator->setChain(headHandler);

    s->history = new CommandHistory();

    out << "\n  Workflow \"" << s->name << "\" created successfully!\n";
    return s;
}
