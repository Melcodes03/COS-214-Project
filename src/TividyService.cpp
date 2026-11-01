#include "TividyService.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <ctime>
#include <deque>
#include <fstream>
#include <map>
#include <set>
#include <sstream>

#include "third_party/json.hpp"

#include "AllocationCommand.h"
#include "ApprovalCommand.h"
#include "ApprovalRequest.h"
#include "AssignmentHandler.h"
#include "CommandHistory.h"
#include "DefinitionBuilder.h"
#include "DemoData.h"
#include "EmailConnector.h"
#include "LeastWorkloadAssignment.h"
#include "LevelHandler.h"
#include "ManagerHandler.h"
#include "MultiLevelApproval.h"
#include "Participant.h"
#include "ParticipantObserver.h"
#include "ProgressCommand.h"
#include "ReportScreen.h"
#include "SingleApproval.h"
#include "SmsConnector.h"
#include "TransitionCommand.h"
#include "WorkAllocator.h"
#include "WorkItem.h"
#include "WorkItemIterator.h"
#include "WorkItemMemento.h"
#include "WorkerCommand.h"
#include "WorkflowDefinition.h"
#include "WorkflowDirector.h"
#include "WorkflowInstance.h"

typedef nlohmann::json Json;

// ---------------------------------------------------------------------------------------
// Catalogue: one entry per industry. Pure data plus two factory functions from DemoData.
// ---------------------------------------------------------------------------------------

namespace {

struct CaseTemplate {
    std::string label;
    std::string caseId;
    std::map<std::string, std::string> data;
};

/// One person in an industry's team, as plain data (a fresh Participant is made per case).
struct PersonSpec {
    std::string name;
    std::string role;
    int capacity;
};

/// Everything needed to run one industry: its workflow description, its team and its sample
/// cases. Built-in industries come from DemoData; custom ones from the "New workflow" form.
/// Both are just data, so both run on exactly the same engine code.
struct Industry {
    int id;
    std::string key;
    std::string name;
    std::string description;
    WorkflowSpec spec;
    std::vector<PersonSpec> team;
    std::vector<CaseTemplate> cases;
    std::vector<std::string> roles;   ///< lowest escalation level first; the last one is the manager level
    bool custom;
    std::string organisation;         ///< Organisation -> Workspace -> Workflow -> Case
    std::string workspace;
};

/// One line of the activity log kept across every case (dashboard, alerts, history).
struct Activity {
    long seq;
    std::time_t when;
    std::string caseId;
    std::string workflow;
    std::string organisation;
    std::string workspace;
    std::string kind;   ///< case-opened, assigned, started, completed, approval, approved, sentback,
                        ///< escalated, cancelled, ready, approval-pending, case-completed, case-failed,
                        ///< sms, email, undo, workflow-created
    std::string text;
    std::string who;
};

/// Copies a team made by DemoData into plain data and frees the Participants.
std::vector<PersonSpec> teamData(const std::vector<Participant*>& people) {
    std::vector<PersonSpec> team;
    for (size_t i = 0; i < people.size(); ++i) {
        PersonSpec p;
        p.name = people[i]->getName();
        p.role = people[i]->getRole();
        p.capacity = people[i]->getCapacity();
        team.push_back(p);
        delete people[i];
    }
    return team;
}

/// Roles ordered by the lowest task priority that needs them. Priorities double as
/// escalation levels (see DemoData), so this is the order of the escalation chain.
std::vector<std::string> rolesInLevelOrder(const WorkflowSpec& spec) {
    std::map<std::string, int> lowest;
    for (size_t i = 0; i < spec.stages.size(); ++i) {
        for (size_t j = 0; j < spec.stages[i].tasks.size(); ++j) {
            const TaskSpec& t = spec.stages[i].tasks[j];
            std::map<std::string, int>::iterator found = lowest.find(t.role);
            if (found == lowest.end() || t.priority < found->second) lowest[t.role] = t.priority;
        }
    }
    std::vector<std::pair<int, std::string> > ordered;
    for (std::map<std::string, int>::const_iterator it = lowest.begin(); it != lowest.end(); ++it) {
        ordered.push_back(std::make_pair(it->second, it->first));
    }
    std::sort(ordered.begin(), ordered.end());
    std::vector<std::string> roles;
    for (size_t i = 0; i < ordered.size(); ++i) roles.push_back(ordered[i].second);
    return roles;
}

}  // namespace

struct TividyCatalog {
    std::deque<Industry> industries;   ///< a deque, so adding a custom industry never moves the others
    std::vector<Activity> feed;        ///< activity across every case, oldest first
    long nextSeq = 1;

    TividyCatalog() {
        Industry support;
        support.id = 1;
        support.key = "support_desk";
        support.name = "Software Customer Support";
        support.description = "Handles inbound software bug reports. Work starts with a general "
                              "assistant and escalates level by level up to the support manager. "
                              "The fix must be approved, and critical cases get an incident report.";
        support.spec = DemoData::makeSupportSpec();
        support.team = teamData(DemoData::createSupportTeam());
        support.custom = false;
        support.organisation = "Pretoria Software Co.";
        support.workspace = "Customer Support";
        std::vector<DemoData::SupportCase> sc = DemoData::sampleSupportCases();
        for (size_t i = 0; i < sc.size(); ++i) {
            CaseTemplate c;
            c.caseId = sc[i].caseId;
            c.label = sc[i].caseId + " - " + sc[i].customer;
            c.data["caseId"] = sc[i].caseId;
            c.data["customer"] = sc[i].customer;
            c.data["product"] = sc[i].product;
            c.data["summary"] = sc[i].summary;
            c.data["severity"] = sc[i].severity;
            support.cases.push_back(c);
        }
        industries.push_back(support);

        Industry admissions;
        admissions.id = 2;
        admissions.key = "university_admissions";
        admissions.name = "University Admissions";
        admissions.description = "Application intake, parallel document and identity checks, "
                                 "academic assessment, then a score-based decision (offer, waiting "
                                 "list or rejection) followed by enrolment. An offer needs two sign-offs.";
        admissions.spec = DemoData::makeAdmissionsSpec();
        admissions.team = teamData(DemoData::createAdmissionsTeam());
        admissions.custom = false;
        admissions.organisation = "University of Pretoria";
        admissions.workspace = "Admissions Office";
        std::vector<DemoData::AdmissionsApplication> ap = DemoData::sampleApplications();
        for (size_t i = 0; i < ap.size(); ++i) {
            CaseTemplate c;
            c.caseId = ap[i].applicationId;
            c.label = ap[i].applicationId + " - " + ap[i].applicant;
            c.data["applicationId"] = ap[i].applicationId;
            c.data["applicant"] = ap[i].applicant;
            c.data["programme"] = ap[i].programme;
            c.data["score"] = std::to_string(ap[i].score);
            admissions.cases.push_back(c);
        }
        industries.push_back(admissions);

        for (size_t i = 0; i < industries.size(); ++i) {
            industries[i].roles = rolesInLevelOrder(industries[i].spec);
        }
    }

    const Industry* find(int id) const {
        for (size_t i = 0; i < industries.size(); ++i) {
            if (industries[i].id == id) return &industries[i];
        }
        return nullptr;
    }
};

// ---------------------------------------------------------------------------------------
// One loaded scenario. Owns everything it created.
// ---------------------------------------------------------------------------------------

/// One message an observer sent out through an adapter (email or SMS).
struct Notification {
    std::string channel;   ///< "Email" or "SMS"
    std::string to;
    std::string text;
};

/// An observer watching one task, the channel it sends through, and how much of its inbox
/// has already been shown.
struct Watch {
    ParticipantObserver* observer;
    std::string channel;
    size_t seen;
    WorkItem* item;      ///< the task being watched
    std::string state;   ///< the state that triggers the message
    std::string to;      ///< who is told (the manager)
};

struct TividyScenario {
    const Industry* industry;
    std::map<std::string, std::string> caseData;
    WorkflowDefinition* definition;
    WorkflowInstance* instance;
    WorkAllocator* allocator;
    CommandHistory* history;
    std::vector<Participant*> people;
    EmailConnector* email;   ///< Adapter: email channel
    SmsConnector* sms;       ///< Adapter: SMS channel
    std::vector<Watch> watches;
    std::vector<Notification> notifications;
    int number = 0;                                      ///< 1 for the first case opened, 2 for the next...
    std::time_t opened = 0;
    std::map<std::string, ApprovalRequest*> approvals;   ///< task id -> its current approval (Strategy context)
    std::vector<ApprovalRequest*> allRequests;           ///< every request made, owned (old ones stay for undo)

    TividyScenario()
        : industry(nullptr), definition(nullptr), instance(nullptr), allocator(nullptr), history(nullptr),
          email(nullptr), sms(nullptr) {}

    ~TividyScenario() {
        for (size_t i = 0; i < watches.size(); ++i) delete watches[i].observer;   //they detach from the items
        delete history;     //commands point at items, people and approvals, so they go first
        for (size_t i = 0; i < allRequests.size(); ++i) delete allRequests[i];
        delete instance;    //the instance borrows the definition
        delete definition;
        delete allocator;   //also deletes the escalation chain
        for (size_t i = 0; i < people.size(); ++i) delete people[i];
        delete email;
        delete sms;
    }

    TividyScenario(const TividyScenario&) = delete;
    TividyScenario& operator=(const TividyScenario&) = delete;
};

namespace {

// ---- looking things up ---------------------------------------------------------------

WorkItem* findItem(WorkItem& root, const std::string& id) {
    return ReportScreen::find(root, id);
}

Participant* findPerson(TividyScenario& s, const std::string& name) {
    for (size_t i = 0; i < s.people.size(); ++i) {
        if (s.people[i]->getName() == name) return s.people[i];
    }
    return nullptr;
}

bool holds(Participant& who, WorkItem* item) {
    std::vector<WorkItem*>& list = who.getWorklist();
    return std::find(list.begin(), list.end(), item) != list.end();
}

/// Who currently has the item, or nullptr. The allocator keeps its last record after the
/// work leaves a person, so only states in which someone really holds the item count.
Participant* holderOf(TividyScenario& s, WorkItem& item) {
    static const std::set<std::string> held = {"Assigned", "InProgress", "Completed"};
    if (held.find(item.getStateName()) == held.end()) return nullptr;
    return s.allocator->getAssignee(item.getId());
}

/// True when the person holding the item is at the top of the escalation chain. Nobody is
/// above them, so they cannot decline or escalate: passing work on from there would leave it
/// with nobody (the "freeze" at the end of the chain).
bool atLastLevel(TividyScenario& s, WorkItem& item) {
    if (holderOf(s, item) == nullptr) return false;
    AssignmentHandler* level = s.allocator->getLevel(item.getId());
    return level != nullptr && level->getNext() == nullptr;
}

/// A task that has not opened yet (still Created) cannot be cancelled: nothing has happened
/// to it, and cancelling would fail the whole case for work that may never run.
bool notOpenedYet(WorkItem& item) {
    return item.getStateName() == "Created";
}

// ---- notifications: Observer + Adapter --------------------------------------------------

/// Watches every task for the manager level: an SMS when work is escalated, an email when
/// work is sent back for rework or cancelled. ParticipantObserver (Observer) hears the state
/// change and sends it through the SMS or email connector (Adapter).
void watchTasks(TividyScenario& s) {
    Participant* manager = nullptr;
    const std::string& top = s.industry->roles.back();
    for (size_t k = 0; k < s.people.size() && manager == nullptr; ++k) {
        if (s.people[k]->getRole() == top) manager = s.people[k];
    }
    if (manager == nullptr) return;
    struct Rule { const char* state; const char* channel; };
    const Rule rules[] = { {"Escalated", "SMS"}, {"Rejected", "Email"}, {"Cancelled", "Email"} };
    WorkItemIterator* it = s.instance->getRoot()->createIterator();
    for (it->first(); !it->isDone(); it->next()) {
        WorkItem* item = it->currentItem();
        if (item->getChildCount() > 0) continue;
        for (size_t r = 0; r < sizeof(rules) / sizeof(rules[0]); ++r) {
            ParticipantObserver* o = new ParticipantObserver(item, rules[r].state, manager);
            bool sms = std::string(rules[r].channel) == "SMS";
            o->addContactMethod(sms ? static_cast<CommunicationAdapter*>(s.sms) : s.email);
            Watch w = { o, rules[r].channel, 0, item, rules[r].state, manager->getName() };
            s.watches.push_back(w);
        }
    }
    delete it;
}

/// Moves new observer messages into the case's notification list and the event list, in
/// plain words for the screen (the observer's own inbox keeps the raw message).
void collectNotifications(TividyScenario& s, std::vector<std::string>& events) {
    static const std::map<std::string, std::string> why = {
        {"Escalated", "was escalated and needs someone more senior"},
        {"Rejected", "was sent back for rework"},
        {"Cancelled", "was cancelled, so the case has failed"}};
    for (size_t i = 0; i < s.watches.size(); ++i) {
        Watch& w = s.watches[i];
        std::vector<std::string>& inbox = w.observer->getInbox();
        for (; w.seen < inbox.size(); ++w.seen) {
            Notification n;
            n.channel = w.channel;
            n.to = w.to;
            n.text = "'" + w.item->getName() + "' (" + w.item->getId() + ") " + why.at(w.state) + ".";
            s.notifications.push_back(n);
            events.push_back(w.channel + " to " + n.to + ": " + n.text);
        }
    }
}

// ---- approvals: Strategy ------------------------------------------------------------------

/// The spec entry for a task id such as "S3-T2", or nullptr.
const TaskSpec* taskSpecFor(const TividyScenario& s, const std::string& id) {
    size_t dash = id.find("-T");
    if (id.size() < 4 || id[0] != 'S' || dash == std::string::npos) return nullptr;
    int stage = std::atoi(id.substr(1, dash - 1).c_str()) - 1;
    int task = std::atoi(id.substr(dash + 2).c_str()) - 1;
    const WorkflowSpec& spec = s.industry->spec;
    if (stage < 0 || stage >= (int)spec.stages.size()) return nullptr;
    if (task < 0 || task >= (int)spec.stages[stage].tasks.size()) return nullptr;
    return &spec.stages[stage].tasks[task];
}

/// How many sign-offs the task needs (0 = it is finished with Complete).
int signoffsFor(const TividyScenario& s, const WorkItem& item) {
    const TaskSpec* t = taskSpecFor(s, item.getId());
    return t != nullptr ? t->signoffs : 0;
}

/// The approval rule for a task: one sign-off or several in a row.
ApprovalStrategy* strategyFor(int signoffs) {
    if (signoffs <= 1) return new SingleApproval();
    return new MultiLevelApproval(signoffs);
}

/// Sign-offs given so far on the task's current (undecided) approval.
int signoffsGiven(const TividyScenario& s, const WorkItem& item) {
    std::map<std::string, ApprovalRequest*>::const_iterator f = s.approvals.find(item.getId());
    if (f == s.approvals.end() || f->second == nullptr || f->second->getDecision() != ApprovalStrategy::PENDING) return 0;
    return f->second->voteCount();
}

// ---- small commands used by the web layer -------------------------------------------------

/// Command pattern: a macro command. Runs its steps in order as ONE history entry, so a
/// single Undo takes back all of them. The first step must succeed; later steps are
/// "best effort" (if nobody can take the work, it simply waits).
class StepsCommand : public Command {
    public:
        StepsCommand(const std::string& name, Command* first) : name(name) { steps.push_back(first); }
        ~StepsCommand() override { for (size_t i = 0; i < steps.size(); ++i) delete steps[i]; }
        StepsCommand(const StepsCommand&) = delete;
        StepsCommand& operator=(const StepsCommand&) = delete;
        void then(Command* step) { steps.push_back(step); }
        bool execute() override {
            done.clear();
            for (size_t i = 0; i < steps.size(); ++i) {
                bool ok = steps[i]->execute();
                if (ok) done.push_back(steps[i]);
                else if (i == 0) return false;
            }
            return true;
        }
        void undo() override {
            for (size_t i = done.size(); i > 0; --i) done[i - 1]->undo();
            done.clear();
        }
        std::string getName() const override { return name; }
    private:
        std::string name;
        std::vector<Command*> steps;
        std::vector<Command*> done;
};

/// After work leaves a person (sent back), passes it up the escalation chain. If nobody
/// higher is free it falls back to the start of the chain, and if nobody at all is free it
/// waits; either way the step itself never fails.
class PassOnCommand : public Command {
    public:
        PassOnCommand(TividyScenario& s, WorkItem& item) : s(s), item(item), used(nullptr) {}
        ~PassOnCommand() override { delete up; delete start; }
        PassOnCommand(const PassOnCommand&) = delete;
        PassOnCommand& operator=(const PassOnCommand&) = delete;
        bool execute() override {
            used = nullptr;
            delete up; delete start;
            up = new AllocationCommand(s.allocator, &item, s.people, &WorkAllocator::reassign, "Pass on");
            start = new AllocationCommand(s.allocator, &item, s.people, &WorkAllocator::allocate, "Pass on");
            if (up->execute()) used = up;
            else if (start->execute()) used = start;
            return used != nullptr;
        }
        void undo() override { if (used != nullptr) used->undo(); used = nullptr; }
        std::string getName() const override { return "Pass on: " + item.getName(); }
    private:
        TividyScenario& s;
        WorkItem& item;
        AllocationCommand* up = nullptr;
        AllocationCommand* start = nullptr;
        AllocationCommand* used;
};

/// One sign-off (approve or reject) on a task, using ApprovalRequest + its ApprovalStrategy
/// and the existing ApprovalCommand. When the vote reaches a decision the item is completed
/// or rejected, and the holder's slot is freed. A task that comes back after rework gets a
/// fresh approval.
class SignoffCommand : public Command {
    public:
        SignoffCommand(TividyScenario& s, WorkItem& item, Participant* holder, bool approve)
            : s(s), item(item), holder(holder), approve(approve), vote(nullptr),
              previous(nullptr), created(false), released(false), listIndex(-1) {}
        ~SignoffCommand() override { delete vote; }
        SignoffCommand(const SignoffCommand&) = delete;
        SignoffCommand& operator=(const SignoffCommand&) = delete;
        bool execute() override {
            ApprovalRequest*& current = s.approvals[item.getId()];
            previous = current;
            created = false;
            if (current == nullptr || current->getDecision() != ApprovalStrategy::PENDING) {
                current = new ApprovalRequest(&item, strategyFor(signoffsFor(s, item)));
                s.allRequests.push_back(current);
                created = true;
            }
            delete vote;
            vote = new ApprovalCommand(current, &item, approve, getName());
            if (!vote->execute()) { if (created) current = previous; return false; }
            released = false;
            if (item.getStateName() != "InProgress" && holder != nullptr) {   //decided: the work leaves the holder
                std::vector<WorkItem*>& list = holder->getWorklist();
                listIndex = (int)(std::find(list.begin(), list.end(), &item) - list.begin());
                holder->finishTask();
                holder->remove(&item);
                released = true;
            }
            return true;
        }
        void undo() override {
            if (released) {
                holder->addTask();
                std::vector<WorkItem*>& list = holder->getWorklist();
                int at = listIndex < (int)list.size() ? listIndex : (int)list.size();
                list.insert(list.begin() + at, &item);
                released = false;
            }
            vote->undo();
            if (created) {
                if (previous != nullptr) s.approvals[item.getId()] = previous;
                else s.approvals.erase(item.getId());
            }
        }
        std::string getName() const override { return (approve ? "Approve: " : "Reject: ") + item.getName(); }
    private:
        TividyScenario& s;
        WorkItem& item;
        Participant* holder;
        bool approve;
        ApprovalCommand* vote;
        ApprovalRequest* previous;
        bool created;
        bool released;
        int listIndex;
};

/// Why a failed case failed: the first cancelled task.
std::string failReason(TividyScenario& s) {
    std::string reason;
    WorkItemIterator* it = s.instance->getRoot()->createIterator();
    for (it->first(); !it->isDone() && reason.empty(); it->next()) {
        WorkItem* item = it->currentItem();
        if (item->getChildCount() == 0 && item->getStateName() == "Cancelled") {
            reason = item->getId() + " (" + item->getName() + ") was cancelled";
        }
    }
    delete it;
    return reason;
}

// ---- the lifecycle table: API action -> WorkItem operation ---------------------------

struct LifecycleAction {
    const char* name;
    bool (WorkItem::*operation)();
};

const LifecycleAction kActions[] = {
    {"allocate", &WorkItem::assign},
    {"start", &WorkItem::start},
    {"complete", &WorkItem::complete},
    {"decline", &WorkItem::reject},
    {"escalate", &WorkItem::escalate},
    {"cancel", &WorkItem::cancel},
};
const int kActionCount = sizeof(kActions) / sizeof(kActions[0]);

/// Which actions the engine would accept right now. The engine itself answers: each action
/// is tried on a throw-away clone restored to the item's current state, so the real item,
/// its observers and its audit log are never touched, and the answer can never disagree
/// with the State pattern.
std::vector<std::string> validActionsFor(TividyScenario& s, WorkItem& item) {
    std::vector<std::string> valid;
    if (item.getChildCount() > 0 || s.instance->getStatus() != WorkflowInstance::RUNNING) {
        return valid;
    }
    WorkItemMemento* snapshot = item.createMemento();
    for (int i = 0; i < kActionCount; ++i) {
        WorkItem* probe = item.clone();
        probe->restore(*snapshot);
        if ((probe->*kActions[i].operation)()) valid.push_back(kActions[i].name);
        delete probe;
    }
    delete snapshot;
    //house rules on top of the State pattern (see atLastLevel and notOpenedYet)
    bool last = atLastLevel(s, item);
    bool closed = notOpenedYet(item);
    bool signoff = signoffsFor(s, item) > 0;
    std::vector<std::string> allowed;
    for (size_t i = 0; i < valid.size(); ++i) {
        if (closed && valid[i] == "cancel") continue;
        if (last && (valid[i] == "decline" || valid[i] == "escalate")) continue;
        if (signoff && valid[i] == "complete") {   //a sign-off task is finished by approving it
            allowed.push_back("approve");
            allowed.push_back("reject");
            continue;
        }
        allowed.push_back(valid[i]);
    }
    return allowed;
}

// ---- snapshots and events ------------------------------------------------------------

typedef std::map<std::string, std::string> StateMap;   // item id -> state name

void collect(WorkItem& root, StateMap& states, std::map<std::string, std::string>& names) {
    WorkItemIterator* it = root.createIterator();
    for (it->first(); !it->isDone(); it->next()) {
        states[it->currentItem()->getId()] = it->currentItem()->getStateName();
        names[it->currentItem()->getId()] = it->currentItem()->getName();
    }
    delete it;
}

struct Before {
    StateMap states;
    std::string status;
};

Before capture(TividyScenario& s) {
    Before b;
    std::map<std::string, std::string> names;
    collect(*s.instance->getRoot(), b.states, names);
    b.status = s.instance->getStatusName();
    return b;
}

/// Events are the difference between two snapshots, so they are right for every kind of
/// change: an action, the stages it opened, an undo, a redo or a rollback.
std::vector<std::string> eventsSince(TividyScenario& s, const Before& before) {
    StateMap now;
    std::map<std::string, std::string> names;
    collect(*s.instance->getRoot(), now, names);
    std::vector<std::string> events;
    WorkItemIterator* it = s.instance->getRoot()->createIterator();
    for (it->first(); !it->isDone(); it->next()) {   //iterate in tree order for a readable log
        const std::string id = it->currentItem()->getId();
        StateMap::const_iterator old = before.states.find(id);
        if (old != before.states.end() && old->second != now[id]) {
            events.push_back(id + " (" + names[id] + "): " + old->second + " -> " + now[id]);
        }
    }
    delete it;
    if (before.status != s.instance->getStatusName()) {
        events.push_back("Workflow " + s.instance->getId() + ": " + before.status + " -> " +
                         s.instance->getStatusName());
    }
    return events;
}

// ---- dependencies and progress ---------------------------------------------------------

const char* modeName(const TividyScenario& s, int stage) {
    static const char* names[] = {"sequential", "parallel", "choice"};
    int mode = stage < (int)s.industry->spec.stages.size() ? (int)s.industry->spec.stages[stage].mode : 0;
    return names[(mode >= 0 && mode < 3) ? mode : 0];
}

/// What a task that has not opened yet is waiting for, in plain words ("" when it is open).
/// A task in a later stage waits for the stage before; in an "in order" stage it waits for
/// the task before it. Tasks of a parallel stage open together, so they only wait for the stage.
std::string waitingFor(TividyScenario& s, int stage, int task) {
    WorkItem* root = s.instance->getRoot();
    WorkItem* st = root->getChild(stage);
    WorkItem* t = st->getChild(task);
    if (t->getStateName() != "Created" || st->isCompleted()) return std::string();
    if (st->getStateName() == "Created") {
        return stage > 0 ? "the '" + root->getChild(stage - 1)->getName() + "' stage to finish" : std::string("the case to start");
    }
    std::string mode = modeName(s, stage);
    if (mode == "sequential" && task > 0) return "'" + st->getChild(task - 1)->getName() + "' to finish";
    if (mode == "choice") return "the decision";
    return std::string();
}

/// Tasks done and tasks to do, counting one branch per decision stage.
void progressOf(TividyScenario& s, int& done, int& total) {
    done = total = 0;
    WorkItem* root = s.instance->getRoot();
    for (int i = 0; i < root->getChildCount(); ++i) {
        WorkItem* st = root->getChild(i);
        if (std::string(modeName(s, i)) == "choice") {
            ++total;
            if (st->isCompleted()) ++done;
            continue;
        }
        for (int k = 0; k < st->getChildCount(); ++k) {
            ++total;
            if (st->getChild(k)->getStateName() == "Completed") ++done;
        }
    }
}

/// The branches the case's decision stages took so far ("Issue offer of admission").
std::vector<std::string> outcomesOf(TividyScenario& s) {
    std::vector<std::string> out;
    WorkItem* root = s.instance->getRoot();
    for (int i = 0; i < root->getChildCount(); ++i) {
        if (std::string(modeName(s, i)) != "choice") continue;
        WorkItem* st = root->getChild(i);
        for (int k = 0; k < st->getChildCount(); ++k) {
            std::string state = st->getChild(k)->getStateName();
            if (state != "Created") out.push_back(st->getChild(k)->getName());
        }
    }
    return out;
}

// ---- JSON -------------------------------------------------------------------------------

Json itemJson(TividyScenario& s, WorkItem& item, bool stageCompleted) {
    Json j;
    j["id"] = item.getId();
    j["name"] = item.getName();
    j["state"] = item.getStateName();
    if (item.getChildCount() > 0) {
        Json tasks = Json::array();
        bool done = item.isCompleted();
        for (int i = 0; i < item.getChildCount(); ++i) tasks.push_back(itemJson(s, *item.getChild(i), done));
        j["tasks"] = tasks;
        return j;
    }
    Participant* who = holderOf(s, item);
    j["assignedTo"] = who != nullptr ? Json(who->getName()) : Json(nullptr);
    j["requiredRole"] = item.getRequiredRole();
    j["priority"] = item.getPriority();
    j["retryCount"] = item.getRetryCount();
    j["maxRetries"] = item.getMaxRetries();
    j["validActions"] = validActionsFor(s, item);
    j["lastLevel"] = atLastLevel(s, item);
    int needed = signoffsFor(s, item);
    if (needed > 0) {
        Json a;
        a["needed"] = needed;
        a["given"] = signoffsGiven(s, item);
        j["signoffs"] = a;
    }
    const TaskSpec* spec = taskSpecFor(s, item.getId());
    if (spec != nullptr && !spec->whenKey.empty()) j["when"] = spec->whenKey + " " + spec->whenOp + " " + spec->whenValue;
    //a finished stage never opens the branches of a choice it did not take
    j["skipped"] = stageCompleted && item.getStateName() == "Created";
    return j;
}

Json stateJson(TividyScenario* s) {
    Json j;
    if (s == nullptr) {
        j["loaded"] = false;
        j["industry"] = nullptr;
        return j;
    }
    j["loaded"] = true;
    j["industry"] = s->industry->name;
    j["industryId"] = s->industry->id;
    j["instanceId"] = s->instance->getId();
    j["status"] = s->instance->getStatusName();
    j["failReason"] = s->instance->getStatus() == WorkflowInstance::FAILED ? failReason(*s) : std::string();
    j["levels"] = s->industry->roles;
    j["custom"] = s->industry->custom;
    j["caseData"] = s->caseData;
    j["organisation"] = s->industry->organisation;
    j["workspace"] = s->industry->workspace;
    int done = 0, total = 0;
    progressOf(*s, done, total);
    j["progress"] = {{"done", done}, {"total", total}};
    Json stages = Json::array();
    WorkItem* root = s->instance->getRoot();
    for (int i = 0; i < root->getChildCount(); ++i) {
        Json st = itemJson(*s, *root->getChild(i), false);
        st["mode"] = modeName(*s, i);
        for (size_t k = 0; k < st["tasks"].size(); ++k) {
            std::string w = waitingFor(*s, i, (int)k);
            if (!w.empty() && !st["tasks"][k]["skipped"].get<bool>()) st["tasks"][k]["waitingFor"] = w;
        }
        stages.push_back(st);
    }
    j["stages"] = stages;

    Json people = Json::array();
    for (size_t i = 0; i < s->people.size(); ++i) {
        Participant* p = s->people[i];
        Json pj;
        pj["name"] = p->getName();
        pj["role"] = p->getRole();
        pj["workload"] = p->getActiveTasks();
        pj["capacity"] = p->getCapacity();
        pj["available"] = p->isAvailable();
        Json list = Json::array();
        for (size_t k = 0; k < p->getWorklist().size(); ++k) list.push_back(p->getWorklist()[k]->getId());
        pj["worklist"] = list;
        people.push_back(pj);
    }
    j["participants"] = people;

    CommandHistory& h = *s->history;
    Json hist;
    hist["canUndo"] = h.undoCount() > 0;
    hist["canRedo"] = h.redoCount() > 0;
    hist["undoCount"] = h.undoCount();
    hist["redoCount"] = h.redoCount();
    hist["marker"] = h.mark();
    hist["lastCommand"] = h.lastCommandName();
    hist["done"] = h.getHistory();
    hist["undone"] = h.getRedoList();
    j["history"] = hist;

    Json notes = Json::array();
    for (size_t i = 0; i < s->notifications.size(); ++i) {
        Json n;
        n["channel"] = s->notifications[i].channel;
        n["to"] = s->notifications[i].to;
        n["text"] = s->notifications[i].text;
        notes.push_back(n);
    }
    j["notifications"] = notes;
    return j;
}

// ---- running actions ---------------------------------------------------------------------

bool run(TividyScenario& s, Command* command) {
    return s.history->execute(new ProgressCommand(command, s.instance));
}

typedef bool (*ActionFn)(TividyScenario&, WorkItem&, const std::string& who, std::string& error);

bool checkWho(Participant* holder, const std::string& who, std::string& error) {
    if (holder == nullptr) {
        error = "Nobody holds this task yet. Allocate it first.";
        return false;
    }
    if (!who.empty() && who != holder->getName()) {
        error = "This task is held by " + holder->getName() + ", not " + who + ".";
        return false;
    }
    return true;
}

bool doAllocate(TividyScenario& s, WorkItem& item, const std::string& who, std::string& error) {
    Participant* preferred = nullptr;
    if (!who.empty()) {
        preferred = findPerson(s, who);
        if (preferred == nullptr) {
            error = "Unknown participant '" + who + "'.";
            return false;
        }
    }
    //work that someone already held moves UP the escalation chain; new work starts at the bottom
    AllocationCommand::Operation op = s.allocator->getLevel(item.getId()) != nullptr
                                          ? &WorkAllocator::reassign : &WorkAllocator::allocate;
    std::string label = preferred != nullptr ? "Allocate: " + item.getName() + " -> " + who
                                             : "Allocate: " + item.getName();
    bool ok = run(s, new AllocationCommand(s.allocator, &item, s.people, op, label, preferred));
    if (!ok && preferred == nullptr && op == &WorkAllocator::reassign) {   //nobody higher: back to the start of the chain
        ok = run(s, new AllocationCommand(s.allocator, &item, s.people, &WorkAllocator::allocate, label));
    }
    if (!ok) {
        error = preferred != nullptr
            ? who + " cannot take " + item.getId() + " now (at capacity, unavailable, or the task cannot be assigned)."
            : "Nobody can take " + item.getId() + " now: no free person at the required level or above.";
    }
    return ok;
}

bool doStart(TividyScenario& s, WorkItem& item, const std::string& who, std::string& error) {
    Participant* holder = holderOf(s, item);
    if (!checkWho(holder, who, error)) return false;
    return run(s, new WorkerCommand(holder, &item, &WorkItem::start, "Start: " + item.getName(), false));
}

bool doComplete(TividyScenario& s, WorkItem& item, const std::string& who, std::string& error) {
    Participant* holder = holderOf(s, item);
    if (!checkWho(holder, who, error)) return false;
    if (signoffsFor(s, item) > 0) {
        error = item.getId() + " needs " + std::to_string(signoffsFor(s, item)) + " sign-off(s). Use Approve or Reject.";
        return false;
    }
    return run(s, new WorkerCommand(holder, &item, &WorkItem::complete, "Complete: " + item.getName(), true));
}

/// Decline gives the work back and passes it straight up the escalation chain (one Undo
/// takes back both). With no rework left it stays sent back; only Cancel is possible then.
bool doDecline(TividyScenario& s, WorkItem& item, const std::string& who, std::string& error) {
    Participant* holder = holderOf(s, item);
    if (!checkWho(holder, who, error)) return false;
    StepsCommand* steps = new StepsCommand("Decline: " + item.getName(),
        new WorkerCommand(holder, &item, &WorkItem::reject, "Decline: " + item.getName(), true));
    steps->then(new PassOnCommand(s, item));
    return run(s, steps);
}

/// One sign-off. A rejection sends the work back and passes it up the chain, like Decline.
bool doSignoff(TividyScenario& s, WorkItem& item, const std::string& who, std::string& error, bool approve) {
    Participant* holder = holderOf(s, item);
    if (!checkWho(holder, who, error)) return false;
    if (signoffsFor(s, item) == 0) {
        error = item.getId() + " does not need a sign-off. Use Complete.";
        return false;
    }
    if (item.getStateName() != "InProgress") {
        error = item.getId() + " can only be signed off once it has been started.";
        return false;
    }
    SignoffCommand* vote = new SignoffCommand(s, item, holder, approve);
    if (approve) return run(s, vote);
    StepsCommand* steps = new StepsCommand(vote->getName(), vote);
    steps->then(new PassOnCommand(s, item));
    return run(s, steps);
}

bool doApprove(TividyScenario& s, WorkItem& item, const std::string& who, std::string& error) {
    return doSignoff(s, item, who, error, true);
}

bool doReject(TividyScenario& s, WorkItem& item, const std::string& who, std::string& error) {
    return doSignoff(s, item, who, error, false);
}

bool doEscalate(TividyScenario& s, WorkItem& item, const std::string&, std::string&) {
    return run(s, new AllocationCommand(s.allocator, &item, s.people, &WorkAllocator::escalate,
                                        "Escalate: " + item.getName()));
}

bool doCancel(TividyScenario& s, WorkItem& item, const std::string&, std::string&) {
    Participant* holder = holderOf(s, item);
    if (holder != nullptr && holds(*holder, &item)) {   //free the person's slot as well
        return run(s, new WorkerCommand(holder, &item, &WorkItem::cancel, "Cancel: " + item.getName(), true));
    }
    return run(s, new TransitionCommand(&item, &WorkItem::cancel, "Cancel: " + item.getName()));
}

struct ActionEntry {
    const char* name;
    ActionFn run;
    const char* verb;
};

const ActionEntry kTable[] = {
    {"allocate", &doAllocate, "allocated"},
    {"start", &doStart, "started"},
    {"complete", &doComplete, "completed"},
    {"decline", &doDecline, "declined"},
    {"escalate", &doEscalate, "escalated"},
    {"cancel", &doCancel, "cancelled"},
    {"approve", &doApprove, "approved"},
    {"reject", &doReject, "rejected"},
};

// ---- building a scenario ---------------------------------------------------------------

/// One level per role, lowest first. The last role is the manager level, which takes
/// whatever nobody below can (ManagerHandler).
AssignmentHandler* buildChain(const std::vector<std::string>& roles, const std::vector<Participant*>& people) {
    AssignmentHandler* first = nullptr;
    AssignmentHandler* last = nullptr;
    for (size_t i = 0; i < roles.size(); ++i) {
        AssignmentHandler* level = nullptr;
        if (i + 1 < roles.size()) {
            level = new LevelHandler(roles[i], (int)i + 1, new LeastWorkloadAssignment());
        } else {
            Participant* manager = nullptr;
            for (size_t k = 0; k < people.size() && manager == nullptr; ++k) {
                if (people[k]->getRole() == roles[i]) manager = people[k];
            }
            level = new ManagerHandler(manager);
        }
        if (last == nullptr) first = level; else last->setNext(level);
        last = level;
    }
    return first;
}

TividyScenario* buildScenario(const Industry& industry, const std::string& caseId,
                              const std::map<std::string, std::string>& data) {
    TividyScenario* s = new TividyScenario();
    s->industry = &industry;
    s->caseData = data;

    DefinitionBuilder builder(industry.spec.name, 1);
    WorkflowDirector().construct(builder, industry.spec);
    s->definition = builder.getResult();
    s->instance = new WorkflowInstance(caseId, s->definition);
    for (std::map<std::string, std::string>::const_iterator it = data.begin(); it != data.end(); ++it) {
        s->instance->setData(it->first, it->second);
    }

    for (size_t i = 0; i < industry.team.size(); ++i) {
        const PersonSpec& p = industry.team[i];
        s->people.push_back(new Participant(p.name, p.role, p.capacity));
    }
    s->allocator = new WorkAllocator(new LeastWorkloadAssignment());
    for (size_t i = 0; i < s->people.size(); ++i) s->allocator->addParticipant(s->people[i]);
    s->allocator->setChain(buildChain(industry.roles, s->people));

    s->history = new CommandHistory();
    s->email = new EmailConnector();
    s->sms = new SmsConnector();
    watchTasks(*s);
    s->instance->advance();   //opens the first work
    return s;
}

}  // namespace

// ---------------------------------------------------------------------------------------
// TividyService
// ---------------------------------------------------------------------------------------

TividyService::TividyService() : catalog(new TividyCatalog()), scenario(nullptr) {}

TividyService::~TividyService() {
    for (size_t i = 0; i < open.size(); ++i) delete open[i];
    delete catalog;
}

/// Adds a case and makes it current. Cases are never removed behind the user's back: every
/// case stays open (finished ones too) until the server stops.
void TividyService::openCase(TividyScenario* added) {
    added->number = (int)open.size() + 1;
    added->opened = now();
    open.push_back(added);
    scenario = added;
    record("case-opened", "Case " + added->instance->getId() + " opened on " + added->industry->name + ".", "");
}

/// Adds a line to the activity log, for the current case.
void TividyService::record(const std::string& kind, const std::string& text, const std::string& who) {
    Activity a;
    a.seq = catalog->nextSeq++;
    a.when = now();
    if (scenario != nullptr) {
        a.caseId = scenario->instance->getId();
        a.workflow = scenario->industry->name;
        a.organisation = scenario->industry->organisation;
        a.workspace = scenario->industry->workspace;
    }
    a.kind = kind;
    a.text = text;
    a.who = who;
    catalog->feed.push_back(a);
}

std::time_t TividyService::now() const {
    return replaying && replayTime != 0 ? replayTime : std::time(nullptr);
}

/// Appends @p entry (a JSON object text) to the log when @p reply says the change worked.
std::string TividyService::logged(const std::string& reply, const std::string& entry) {
    if (replaying || logPath.empty()) return reply;
    Json r = Json::parse(reply, nullptr, false);
    if (r.is_discarded() || !r.value("ok", false)) return reply;
    Json e = Json::parse(entry);
    e["at"] = (long long)std::time(nullptr);
    std::ofstream out(logPath.c_str(), std::ios::app);
    out << e.dump() << "\n";
    return reply;
}

int TividyService::useLog(const std::string& path) {
    int replayed = 0;
    std::ifstream in(path.c_str());
    std::string line;
    replaying = true;
    while (std::getline(in, line)) {
        Json e = Json::parse(line, nullptr, false);
        if (e.is_discarded() || !e.is_object()) continue;
        replayTime = (std::time_t)e.value("at", 0LL);
        std::string op = e.value("op", "");
        if (op == "industry") loadIndustry(e.value("industryId", 0), e.value("caseIndex", 0), e.value("data", ""));
        else if (op == "workflow") createWorkflow(e.value("spec", ""));
        else if (op == "act") act(e.value("action", ""), e.value("taskId", ""), e.value("who", ""), e.value("caseId", ""));
        else if (op == "undo") undo();
        else if (op == "redo") redo();
        else if (op == "rollback") rollback(e.value("marker", 0));
        else if (op == "case") switchCase(e.value("index", -1));
        else continue;
        ++replayed;
    }
    replaying = false;
    replayTime = 0;
    logPath = path;
    return replayed;
}

std::string TividyService::reset() {
    std::lock_guard<std::mutex> guard(lock);
    for (size_t i = 0; i < open.size(); ++i) delete open[i];
    open.clear();
    scenario = nullptr;
    delete catalog;
    catalog = new TividyCatalog();
    if (!logPath.empty()) std::ofstream(logPath.c_str(), std::ios::trunc);
    return reply(true, "Started fresh: every case and every workflow built in the app was removed.", std::vector<std::string>());
}

/// "SC-1001", or "SC-1001-2" when a case with that id is already open.
std::string TividyService::uniqueCaseId(const std::string& base) const {
    for (int n = 1; ; ++n) {
        std::string id = n == 1 ? base : base + "-" + std::to_string(n);
        bool taken = false;
        for (size_t i = 0; i < open.size() && !taken; ++i) taken = open[i]->instance->getId() == id;
        if (!taken) return id;
    }
}

namespace {

Json casesJson(const std::vector<TividyScenario*>& open, const TividyScenario* current) {
    Json list = Json::array();
    for (size_t i = 0; i < open.size(); ++i) {
        Json c;
        c["index"] = (int)i;
        c["id"] = open[i]->instance->getId();
        c["industry"] = open[i]->industry->name;
        c["status"] = open[i]->instance->getStatusName();
        c["current"] = open[i] == current;
        c["number"] = open[i]->number;
        c["organisation"] = open[i]->industry->organisation;
        c["workspace"] = open[i]->industry->workspace;
        list.push_back(c);
    }
    return list;
}

}  // namespace

std::string TividyService::withCases(const std::string& stateText) const {
    Json j = Json::parse(stateText);
    j["openCases"] = casesJson(open, scenario);
    return j.dump();
}

std::string TividyService::reply(bool ok, const std::string& message, const std::vector<std::string>& events) {
    Json j;
    j["ok"] = ok;
    j["message"] = message;
    j["events"] = events;
    j["state"] = stateJson(scenario);
    j["state"]["openCases"] = casesJson(open, scenario);
    return j.dump();
}

std::string TividyService::snapshotJson() {
    return withCases(stateJson(scenario).dump());
}

std::string TividyService::switchCase(int index) {
    std::lock_guard<std::mutex> guard(lock);
    std::vector<std::string> none;
    if (index < 0 || index >= (int)open.size()) {
        return reply(false, "There is no open case number " + std::to_string(index) + ".", none);
    }
    scenario = open[index];
    return logged(reply(true, "Switched to case " + scenario->instance->getId() + " (" + scenario->industry->name +
                       "). Other cases are unchanged.", none), Json({{"op", "case"}, {"index", index}}).dump());
}

std::string TividyService::industries() {
    std::lock_guard<std::mutex> guard(lock);
    Json list = Json::array();
    for (size_t i = 0; i < catalog->industries.size(); ++i) {
        const Industry& in = catalog->industries[i];
        Json j;
        j["id"] = in.id;
        j["key"] = in.key;
        j["name"] = in.name;
        j["description"] = in.description;
        const WorkflowSpec& spec = in.spec;
        Json stages = Json::array();
        for (size_t k = 0; k < spec.stages.size(); ++k) stages.push_back(spec.stages[k].name);
        j["stages"] = stages;
        j["roles"] = in.roles;
        j["custom"] = in.custom;
        j["organisation"] = in.organisation;
        j["workspace"] = in.workspace;
        std::vector<std::string> fields;   //case details that a decision stage looks at
        for (size_t k = 0; k < spec.stages.size(); ++k) {
            for (size_t t = 0; t < spec.stages[k].tasks.size(); ++t) {
                const std::string& key = spec.stages[k].tasks[t].whenKey;
                if (!key.empty() && std::find(fields.begin(), fields.end(), key) == fields.end()) fields.push_back(key);
            }
        }
        j["decisionFields"] = fields;
        Json cases = Json::array();
        for (size_t k = 0; k < in.cases.size(); ++k) {
            Json c;
            c["index"] = (int)k;
            c["label"] = in.cases[k].label;
            c["data"] = in.cases[k].data;
            cases.push_back(c);
        }
        j["cases"] = cases;
        list.push_back(j);
    }
    return list.dump();
}

std::string TividyService::loadIndustry(int industryId, int caseIndex, const std::string& dataOverrides) {
    std::lock_guard<std::mutex> guard(lock);
    const Industry* industry = catalog->find(industryId);
    if (industry == nullptr) {
        return reply(false, "Unknown industry id " + std::to_string(industryId) + ".", std::vector<std::string>());
    }
    if (caseIndex < 0 || caseIndex >= (int)industry->cases.size()) {
        return reply(false, "Unknown case index " + std::to_string(caseIndex) + ".", std::vector<std::string>());
    }
    const CaseTemplate& chosen = industry->cases[caseIndex];
    std::map<std::string, std::string> data = chosen.data;
    if (!dataOverrides.empty()) {
        Json over = Json::parse(dataOverrides, nullptr, false);
        if (over.is_discarded() || !over.is_object()) {
            return reply(false, "The data overrides must be a JSON object.", std::vector<std::string>());
        }
        for (Json::const_iterator it = over.begin(); it != over.end(); ++it) {
            data[it.key()] = it.value().is_string() ? it.value().get<std::string>() : it.value().dump();
        }
    }
    openCase(buildScenario(*industry, uniqueCaseId(chosen.caseId), data));
    std::vector<std::string> events;
    events.push_back("Started case " + scenario->instance->getId() + " on " + industry->name + ".");
    return logged(reply(true, "Started case " + scenario->instance->getId() + " on " + industry->name + ".", events),
                  Json({{"op", "industry"}, {"industryId", industryId}, {"caseIndex", caseIndex}, {"data", dataOverrides}}).dump());
}

namespace {

std::string trimmed(const std::string& text) {
    size_t a = text.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return std::string();
    size_t b = text.find_last_not_of(" \t\r\n");
    return text.substr(a, b - a + 1);
}

std::string textField(const Json& j, const char* key) {
    Json::const_iterator f = j.find(key);
    return f != j.end() && f->is_string() ? trimmed(f->get<std::string>()) : std::string();
}

/// Staff made up for a workflow whose form left the people list empty: two people at each
/// level and one manager, named after the level ("Assessor 1", "Assessor 2", "Claims Manager").
std::vector<PersonSpec> defaultTeam(const std::vector<std::string>& roles) {
    std::vector<PersonSpec> team;
    for (size_t r = 0; r < roles.size(); ++r) {
        bool manager = r + 1 == roles.size();
        for (int n = 1; n <= (manager ? 1 : 2); ++n) {
            PersonSpec p;
            p.name = manager ? roles[r] : roles[r] + " " + std::to_string(n);
            p.role = roles[r];
            p.capacity = manager ? 5 : 3;
            team.push_back(p);
        }
    }
    return team;
}

/// "Hospital admission" -> "HA". Used for case numbers such as HA-1001.
std::string initials(const std::string& name) {
    std::string out;
    bool word = false;
    for (size_t i = 0; i < name.size() && out.size() < 3; ++i) {
        bool letter = std::isalpha(static_cast<unsigned char>(name[i])) != 0;
        if (letter && !word) out += static_cast<char>(std::toupper(static_cast<unsigned char>(name[i])));
        word = letter;
    }
    return out.empty() ? std::string("CW") : out;
}

std::string valueText(const Json& j) {
    if (j.is_string()) return trimmed(j.get<std::string>());
    if (j.is_number()) return j.dump();
    return std::string();
}

/// Turns the "New workflow" form into an Industry. Returns an empty string when the input is
/// usable, otherwise a message for the user. The rules keep every case finishable: every task
/// has a role, every role has at least one person, the last level is the manager, and every
/// decision stage has an "otherwise" branch.
std::string readCustomIndustry(const Json& in, Industry& out, std::map<std::string, std::string>& caseData) {
    const int kMaxStages = 10, kMaxTasks = 10, kMaxPeople = 20;
    out.name = textField(in, "name");
    if (out.name.empty()) return "Give the workflow a name.";
    if (out.name.size() > 60) return "Keep the name under 60 characters.";

    Json::const_iterator lv = in.find("levels");
    if (lv == in.end() || !lv->is_array() || lv->empty()) {
        return "List at least one escalation level (lowest first). The last level is the manager.";
    }
    if (lv->size() > 8) return "Use at most 8 escalation levels.";
    for (size_t i = 0; i < lv->size(); ++i) {
        std::string role = (*lv)[i].is_string() ? trimmed((*lv)[i].get<std::string>()) : std::string();
        if (role.empty()) return "Escalation level " + std::to_string(i + 1) + " has no name.";
        if (std::find(out.roles.begin(), out.roles.end(), role) != out.roles.end()) {
            return "The level '" + role + "' is listed twice.";
        }
        out.roles.push_back(role);
    }

    //people are optional: without them, staff are made up for every level
    Json::const_iterator pp = in.find("people");
    bool made = pp == in.end() || !pp->is_array() || pp->empty();
    if (made) {
        out.team = defaultTeam(out.roles);
    } else {
        if ((int)pp->size() > kMaxPeople) return "Use at most " + std::to_string(kMaxPeople) + " people.";
        std::set<std::string> names;
        for (size_t i = 0; i < pp->size(); ++i) {
            const Json& pj = (*pp)[i];
            if (!pj.is_object()) return "Person " + std::to_string(i + 1) + " is not filled in properly.";
            PersonSpec p;
            p.name = textField(pj, "name");
            p.role = textField(pj, "role");
            p.capacity = 3;
            Json::const_iterator cap = pj.find("capacity");
            if (cap != pj.end() && cap->is_number_integer()) p.capacity = cap->get<int>();
            if (p.name.empty()) return "Person " + std::to_string(i + 1) + " has no name.";
            if (!names.insert(p.name).second) return "There are two people called '" + p.name + "'.";
            if (std::find(out.roles.begin(), out.roles.end(), p.role) == out.roles.end()) {
                return p.name + "'s level '" + p.role + "' is not one of the escalation levels.";
            }
            if (p.capacity < 1 || p.capacity > 20) return p.name + "'s capacity must be between 1 and 20.";
            out.team.push_back(p);
        }
        for (size_t r = 0; r < out.roles.size(); ++r) {
            bool someone = false;
            for (size_t i = 0; i < out.team.size(); ++i) someone = someone || out.team[i].role == out.roles[r];
            if (!someone) return "Nobody is at the level '" + out.roles[r] + "'. Add a person for every level, or leave People empty.";
        }
    }

    //the case detail that decision stages look at, e.g. {"field": "amount", "value": 60000}
    std::string field, value;
    Json::const_iterator dj = in.find("decision");
    if (dj != in.end() && dj->is_object()) {
        field = textField(*dj, "field");
        Json::const_iterator v = dj->find("value");
        if (v != dj->end()) value = valueText(*v);
    }
    if (!field.empty()) {
        if (field.size() > 30) return "Keep the case detail's name under 30 characters.";
        if (value.empty()) return "Give '" + field + "' a value for the first case.";
        caseData[field] = value;
    }

    static const std::set<std::string> kOps = {"==", "!=", "<", "<=", ">", ">="};
    Json::const_iterator st = in.find("stages");
    if (st == in.end() || !st->is_array() || st->empty()) return "Add at least one stage.";
    if ((int)st->size() > kMaxStages) return "Use at most " + std::to_string(kMaxStages) + " stages.";
    out.spec.name = out.name;
    int decisions = 0;
    for (size_t i = 0; i < st->size(); ++i) {
        const Json& sj = (*st)[i];
        if (!sj.is_object()) return "Stage " + std::to_string(i + 1) + " is not filled in properly.";
        StageSpec stage;
        stage.name = textField(sj, "name");
        if (stage.name.empty()) return "Stage " + std::to_string(i + 1) + " has no name.";
        std::string mode = textField(sj, "mode");
        stage.mode = mode == "parallel" ? PARALLEL : (mode == "choice" ? CHOICE : SEQUENTIAL);
        if (stage.mode == CHOICE) {
            ++decisions;
            if (field.empty()) return "Stage '" + stage.name + "' is a decision, so name the case detail it looks at (for example amount).";
        }
        Json::const_iterator tj = sj.find("tasks");
        if (tj == sj.end() || !tj->is_array() || tj->empty()) return "Stage '" + stage.name + "' has no tasks.";
        if ((int)tj->size() > kMaxTasks) return "Stage '" + stage.name + "' has more than " + std::to_string(kMaxTasks) + " tasks.";
        bool otherwise = false;
        for (size_t k = 0; k < tj->size(); ++k) {
            const Json& t = (*tj)[k];
            if (!t.is_object()) return "A task in '" + stage.name + "' is not filled in properly.";
            TaskSpec task;
            task.name = textField(t, "name");
            task.role = textField(t, "role");
            if (task.name.empty()) return "A task in '" + stage.name + "' has no name.";
            std::vector<std::string>::const_iterator at = std::find(out.roles.begin(), out.roles.end(), task.role);
            if (at == out.roles.end()) return "Task '" + task.name + "' needs a level from the escalation levels.";
            task.priority = (int)(at - out.roles.begin()) + 1;   //the chain starts at this task's own level
            Json::const_iterator so = t.find("signoffs");
            if (so != t.end() && so->is_number_integer()) task.signoffs = so->get<int>();
            if (task.signoffs < 0 || task.signoffs > 3) return "Task '" + task.name + "' can need 0 to 3 sign-offs.";
            if (stage.mode == CHOICE) {
                Json::const_iterator w = t.find("when");
                if (w == t.end() || !w->is_object()) {
                    if (otherwise) return "Decision '" + stage.name + "' has two 'otherwise' branches.";
                    otherwise = true;   //the default branch: runs when nothing above matches
                } else {
                    task.whenKey = field;
                    task.whenOp = textField(*w, "op");
                    Json::const_iterator v = w->find("value");
                    task.whenValue = v != w->end() ? valueText(*v) : std::string();
                    if (kOps.find(task.whenOp) == kOps.end()) return "Task '" + task.name + "' has an unknown comparison '" + task.whenOp + "'.";
                    if (task.whenValue.empty()) return "Task '" + task.name + "' needs a value to compare " + field + " with.";
                }
            }
            stage.tasks.push_back(task);
        }
        if (stage.mode == CHOICE && !otherwise) {
            return "Decision '" + stage.name + "' needs an 'otherwise' branch, so every case has somewhere to go.";
        }
        out.spec.stages.push_back(stage);
    }
    out.custom = true;
    out.organisation = textField(in, "organisation");
    out.workspace = textField(in, "workspace");
    if (out.organisation.empty()) out.organisation = "My organisation";
    if (out.workspace.empty()) out.workspace = "General";
    if (out.organisation.size() > 60 || out.workspace.size() > 60) return "Keep the organisation and workspace names under 60 characters.";
    out.description = "Built in the app: " + std::to_string(out.spec.stages.size()) + " stage(s)" +
                      (decisions > 0 ? " incl. " + std::to_string(decisions) + " decision(s) on " + field : std::string()) +
                      ", " + std::to_string(out.team.size()) + (made ? " made-up" : "") + " staff, escalating " +
                      std::to_string(out.roles.size()) + " level(s) up to " + out.roles.back() + ".";
    return std::string();
}

}  // namespace

std::string TividyService::createWorkflow(const std::string& specJson) {
    std::lock_guard<std::mutex> guard(lock);
    std::vector<std::string> none;
    Json in = Json::parse(specJson, nullptr, false);
    if (in.is_discarded() || !in.is_object()) return reply(false, "The workflow must be a JSON object.", none);

    Industry industry;
    std::map<std::string, std::string> data;
    std::string problem = readCustomIndustry(in, industry, data);
    if (!problem.empty()) return reply(false, problem, none);

    industry.id = (int)catalog->industries.size() + 1;
    industry.key = "custom_" + std::to_string(industry.id);
    CaseTemplate first;
    first.caseId = initials(industry.name) + "-" + std::to_string(industry.id) + "001";
    first.label = first.caseId + " - first case";
    for (std::map<std::string, std::string>::const_iterator it = data.begin(); it != data.end(); ++it) {
        first.label += " (" + it->first + " " + it->second + ")";
    }
    first.data = data;
    first.data["caseId"] = first.caseId;
    industry.cases.push_back(first);
    catalog->industries.push_back(industry);
    const Industry& added = catalog->industries.back();

    scenario = nullptr;   //so the log line below is not filed under the previous case
    record("workflow-created", "Workflow '" + added.name + "' created in " + added.organisation + " / " + added.workspace + ".", "");
    catalog->feed.back().workflow = added.name;
    catalog->feed.back().organisation = added.organisation;
    catalog->feed.back().workspace = added.workspace;
    openCase(buildScenario(added, uniqueCaseId(added.cases[0].caseId), added.cases[0].data));
    std::vector<std::string> events;
    events.push_back("Created the workflow '" + added.name + "' and started case " + scenario->instance->getId() + ".");
    Json j = Json::parse(reply(true, "Created '" + added.name + "' and started case " + scenario->instance->getId() + ".", events));
    j["industryId"] = added.id;
    return logged(j.dump(), Json({{"op", "workflow"}, {"spec", specJson}}).dump());
}

std::string TividyService::state() {
    std::lock_guard<std::mutex> guard(lock);
    return snapshotJson();
}

std::string TividyService::act(const std::string& action, const std::string& taskId,
                               const std::string& participantName) {
    std::lock_guard<std::mutex> guard(lock);
    std::string id = scenario != nullptr ? scenario->instance->getId() : std::string();
    return logged(actHere(action, taskId, participantName),
                  Json({{"op", "act"}, {"action", action}, {"taskId", taskId}, {"who", participantName}, {"caseId", id}}).dump());
}

std::string TividyService::act(const std::string& action, const std::string& taskId,
                               const std::string& participantName, const std::string& caseId) {
    std::lock_guard<std::mutex> guard(lock);
    std::string entry = Json({{"op", "act"}, {"action", action}, {"taskId", taskId}, {"who", participantName},
                              {"caseId", caseId.empty() && scenario != nullptr ? scenario->instance->getId() : caseId}}).dump();
    if (caseId.empty() || (scenario != nullptr && scenario->instance->getId() == caseId)) {
        return logged(actHere(action, taskId, participantName), entry);
    }
    TividyScenario* target = nullptr;
    for (size_t i = 0; i < open.size() && target == nullptr; ++i) {
        if (open[i]->instance->getId() == caseId) target = open[i];
    }
    if (target == nullptr) return reply(false, "No open case with id '" + caseId + "'.", std::vector<std::string>());
    TividyScenario* current = scenario;
    scenario = target;                       //act on that case...
    Json j = Json::parse(actHere(action, taskId, participantName));
    scenario = current;                      //...and stay on the case the user is looking at
    j["state"] = stateJson(scenario);
    j["state"]["openCases"] = casesJson(open, scenario);
    j["caseId"] = caseId;
    return logged(j.dump(), entry);
}

namespace {

/// "S1-T1 (Log support case): Available -> Assigned" -> {"S1-T1", "Log support case", "Assigned"}
bool parseEvent(const std::string& e, std::string& id, std::string& name, std::string& to) {
    size_t open = e.find(" ("), close = e.find("): "), arrow = e.rfind(" -> ");
    if (open == std::string::npos || close == std::string::npos || arrow == std::string::npos) return false;
    id = e.substr(0, open);
    name = e.substr(open + 2, close - open - 2);
    to = e.substr(arrow + 4);
    return true;
}

}  // namespace

std::string TividyService::actHere(const std::string& action, const std::string& taskId,
                                   const std::string& participantName) {
    std::vector<std::string> none;
    if (scenario == nullptr) return reply(false, "No industry is loaded. Pick one first.", none);

    const ActionEntry* entry = nullptr;
    for (size_t i = 0; i < sizeof(kTable) / sizeof(kTable[0]); ++i) {
        if (action == kTable[i].name) entry = &kTable[i];
    }
    if (entry == nullptr) return reply(false, "Unknown action '" + action + "'.", none);

    WorkItem* item = findItem(*scenario->instance->getRoot(), taskId);
    if (item == nullptr) return reply(false, "No task with id '" + taskId + "'.", none);
    if (item->getChildCount() > 0) {
        return reply(false, taskId + " is a stage. Stages open and close themselves; act on its tasks.", none);
    }
    if (scenario->instance->getStatus() != WorkflowInstance::RUNNING) {
        return reply(false, "Case " + scenario->instance->getId() + " is " +
                            scenario->instance->getStatusName() + "; no further actions are possible.", none);
    }

    if (action == "cancel" && notOpenedYet(*item)) {
        return reply(false, taskId + " has not opened yet, so there is nothing to cancel.", none);
    }
    if ((action == "decline" || action == "escalate") && atLastLevel(*scenario, *item)) {
        Participant* top = holderOf(*scenario, *item);
        return reply(false, top->getName() + " is the last level (" + top->getRole() +
                            "), so the task cannot be passed on. Complete it, or cancel the case.", none);
    }

    std::string stateBefore = item->getStateName();
    Participant* was = holderOf(*scenario, *item);
    std::string wasName = was != nullptr ? was->getName() : std::string();
    Before before = capture(*scenario);
    std::string error;
    bool ok = entry->run(*scenario, *item, participantName, error);
    if (!ok) {
        if (error.empty()) {
            error = std::string("Cannot ") + entry->name + " " + taskId + " (state: " + stateBefore + ").";
        }
        return reply(false, error, none);
    }

    Participant* now = holderOf(*scenario, *item);
    std::string message = "Task " + taskId + " " + entry->verb;
    if (action == "decline" || action == "reject") {
        message += " by " + wasName;
        if (now != nullptr && item->getStateName() == "Assigned") message += " and passed on to " + now->getName() + ".";
        else if (item->getRetryCount() >= item->getMaxRetries()) message += ". No reworks are left, so it can only be cancelled.";
        else message += ". Nobody is free to take it right now, so it waits.";
    } else if (action == "approve" && item->getStateName() == "InProgress") {
        int needed = signoffsFor(*scenario, *item), given = signoffsGiven(*scenario, *item);
        message = "Sign-off " + std::to_string(given) + " of " + std::to_string(needed) + " given on " + taskId +
                  ". " + std::to_string(needed - given) + " more needed.";
    } else if (action == "allocate" || action == "escalate") {
        Participant* assigned = scenario->allocator->getAssignee(taskId);
        if (item->getStateName() == "Assigned" && assigned != nullptr) {
            message += " to " + assigned->getName() + ".";
        } else {
            message += "; nobody above could take it right now, so it waits as " + item->getStateName() + ".";
        }
    } else if (now != nullptr) {
        message += " by " + now->getName() + ".";
    } else {
        message += ".";
    }
    std::vector<std::string> events = eventsSince(*scenario, before);
    size_t first = events.size();
    collectNotifications(*scenario, events);

    //the activity log: the action, then what it caused (work that opened, sign-offs waiting,
    //the case finishing or failing, and the messages the observers sent)
    static const std::map<std::string, std::string> kinds = {
        {"allocate", "assigned"}, {"start", "started"}, {"complete", "completed"}, {"approve", "approved"},
        {"reject", "sentback"}, {"decline", "sentback"}, {"escalate", "escalated"}, {"cancel", "cancelled"}};
    std::string kind = kinds.at(action);
    if (action == "approve" && item->getStateName() == "InProgress") kind = "approval";
    Participant* actor = action == "allocate" || action == "escalate" ? holderOf(*scenario, *item) : nullptr;
    record(kind, message, actor != nullptr ? actor->getName() : wasName);
    for (size_t i = 0; i < first; ++i) {
        std::string id, name, to;
        if (events[i].compare(0, 9, "Workflow ") == 0) {
            if (scenario->instance->getStatus() == WorkflowInstance::COMPLETED) {
                record("case-completed", "Case " + scenario->instance->getId() + " is completed.", "");
            } else if (scenario->instance->getStatus() == WorkflowInstance::FAILED) {
                record("case-failed", "Case " + scenario->instance->getId() + " failed: " + failReason(*scenario) + ".", "");
            }
            continue;
        }
        if (!parseEvent(events[i], id, name, to) || id.find("-T") == std::string::npos || id == taskId) continue;
        WorkItem* opened = findItem(*scenario->instance->getRoot(), id);
        if (to == "Available" && opened != nullptr) {
            record("ready", "'" + name + "' (" + id + ") is ready for a " + opened->getRequiredRole() + ".", "");
        }
    }
    if (item->getStateName() == "InProgress" && signoffsFor(*scenario, *item) > 0 && action == "start") {
        record("approval-pending", "'" + item->getName() + "' (" + taskId + ") is waiting for " +
               std::to_string(signoffsFor(*scenario, *item)) + " sign-off(s).", wasName);
    }
    for (size_t i = first; i < events.size(); ++i) {
        record(events[i].compare(0, 3, "SMS") == 0 ? "sms" : "email", events[i], "");
    }
    return reply(true, message, events);
}

std::string TividyService::undo() {
    std::lock_guard<std::mutex> guard(lock);
    std::vector<std::string> none;
    if (scenario == nullptr) return reply(false, "No industry is loaded. Pick one first.", none);
    std::string last = scenario->history->lastCommandName();
    Before before = capture(*scenario);
    if (!scenario->history->undo()) return reply(false, "Nothing to undo.", none);
    std::vector<std::string> events = eventsSince(*scenario, before);
    collectNotifications(*scenario, events);
    record("undo", "Undid: " + last + ".", "");
    return logged(reply(true, "Undid last action: " + last, events), "{\"op\":\"undo\"}");
}

std::string TividyService::redo() {
    std::lock_guard<std::mutex> guard(lock);
    std::vector<std::string> none;
    if (scenario == nullptr) return reply(false, "No industry is loaded. Pick one first.", none);
    Before before = capture(*scenario);
    if (!scenario->history->redo()) return reply(false, "Nothing to redo (or the redo is no longer possible).", none);
    std::vector<std::string> events = eventsSince(*scenario, before);
    collectNotifications(*scenario, events);
    record("undo", "Redid: " + scenario->history->lastCommandName() + ".", "");
    return logged(reply(true, "Redid: " + scenario->history->lastCommandName(), events), "{\"op\":\"redo\"}");
}

std::string TividyService::rollback(int marker) {
    std::lock_guard<std::mutex> guard(lock);
    std::vector<std::string> none;
    if (scenario == nullptr) return reply(false, "No industry is loaded. Pick one first.", none);
    if (marker < 0 || marker > scenario->history->mark()) {
        return reply(false, "Marker " + std::to_string(marker) + " is outside the history (0 to " +
                            std::to_string(scenario->history->mark()) + ").", none);
    }
    Before before = capture(*scenario);
    int undone = scenario->history->rollbackTo(marker);
    record("undo", "Rolled back " + std::to_string(undone) + " action(s) to checkpoint " + std::to_string(marker) + ".", "");
    std::vector<std::string> events = eventsSince(*scenario, before);
    collectNotifications(*scenario, events);
    return logged(reply(true, "Rolled back " + std::to_string(undone) + " action(s) to marker " +
                       std::to_string(marker) + ".", events), Json({{"op", "rollback"}, {"marker", marker}}).dump());
}

std::string TividyService::overview() {
    std::lock_guard<std::mutex> guard(lock);
    Json j;

    //Organisation -> Workspace -> Workflow
    Json spaces = Json::array();
    for (size_t i = 0; i < catalog->industries.size(); ++i) {
        const Industry& in = catalog->industries[i];
        bool found = false;
        for (size_t k = 0; k < spaces.size() && !found; ++k) {
            if (spaces[k]["organisation"] == in.organisation && spaces[k]["workspace"] == in.workspace) {
                spaces[k]["workflows"].push_back(in.name);
                found = true;
            }
        }
        if (!found) spaces.push_back({{"organisation", in.organisation}, {"workspace", in.workspace}, {"workflows", Json::array({in.name})}});
    }
    j["workspaces"] = spaces;

    Json cases = Json::array(), tasks = Json::array();
    std::map<std::string, Json> people;   //key: workflow + name (the same team works on every case of a workflow)
    for (size_t c = 0; c < open.size(); ++c) {
        TividyScenario& s = *open[c];
        Json cj;
        cj["index"] = (int)c;
        cj["number"] = s.number;
        cj["id"] = s.instance->getId();
        cj["industry"] = s.industry->name;
        cj["industryId"] = s.industry->id;
        cj["organisation"] = s.industry->organisation;
        cj["workspace"] = s.industry->workspace;
        cj["status"] = s.instance->getStatusName();
        cj["current"] = open[c] == scenario;
        cj["opened"] = (long long)s.opened;
        cj["caseData"] = s.caseData;
        int done = 0, total = 0;
        progressOf(s, done, total);
        cj["progress"] = {{"done", done}, {"total", total}};
        cj["outcomes"] = outcomesOf(s);
        int awaiting = 0, approvals = 0, active = 0;
        std::string stageNow;
        WorkItem* root = s.instance->getRoot();
        for (int i = 0; i < root->getChildCount(); ++i) {
            WorkItem* st = root->getChild(i);
            if (stageNow.empty() && st->getStateName() == "InProgress") stageNow = st->getName();
            for (int k = 0; k < st->getChildCount(); ++k) {
                WorkItem* t = st->getChild(k);
                Json tj = itemJson(s, *t, st->isCompleted());
                if (tj["skipped"].get<bool>()) continue;
                std::string state = t->getStateName();
                std::vector<std::string> va = tj["validActions"].get<std::vector<std::string> >();
                if (std::find(va.begin(), va.end(), "allocate") != va.end()) ++awaiting;
                if (state == "Assigned" || state == "InProgress") ++active;
                if (state == "InProgress" && signoffsFor(s, *t) > 0) ++approvals;
                tj["caseIndex"] = (int)c;
                tj["caseId"] = s.instance->getId();
                tj["workflow"] = s.industry->name;
                tj["organisation"] = s.industry->organisation;
                tj["workspace"] = s.industry->workspace;
                tj["stage"] = st->getName();
                tj["stageMode"] = modeName(s, i);
                tj["level"] = s.industry->roles.empty() ? t->getRequiredRole()
                    : s.industry->roles[std::max(1, std::min(t->getPriority(), (int)s.industry->roles.size())) - 1];
                std::string w = waitingFor(s, i, k);
                if (!w.empty()) tj["waitingFor"] = w;
                tasks.push_back(tj);
            }
        }
        cj["awaiting"] = awaiting;
        cj["pendingApprovals"] = approvals;
        cj["active"] = active;
        cj["stage"] = stageNow;
        cases.push_back(cj);

        for (size_t p = 0; p < s.people.size(); ++p) {
            Participant* who = s.people[p];
            std::string key = s.industry->name + "\n" + who->getName();
            Json& pj = people[key];
            if (pj.is_null()) {
                pj = {{"name", who->getName()}, {"role", who->getRole()}, {"workflow", s.industry->name},
                      {"organisation", s.industry->organisation}, {"workspace", s.industry->workspace},
                      {"capacity", who->getCapacity()}, {"workload", 0}, {"completed", 0}, {"holding", Json::array()}};
            }
            pj["workload"] = pj["workload"].get<int>() + who->getActiveTasks();
            for (size_t k = 0; k < who->getWorklist().size(); ++k) {
                WorkItem* t = who->getWorklist()[k];
                pj["holding"].push_back({{"caseId", s.instance->getId()}, {"taskId", t->getId()}, {"name", t->getName()}});
            }
        }
        WorkItemIterator* it = root->createIterator();
        for (it->first(); !it->isDone(); it->next()) {
            WorkItem* t = it->currentItem();
            if (t->getChildCount() > 0 || t->getStateName() != "Completed") continue;
            Participant* by = holderOf(s, *t);
            if (by == nullptr) continue;
            Json& pj = people[s.industry->name + "\n" + by->getName()];
            if (!pj.is_null()) pj["completed"] = pj["completed"].get<int>() + 1;
        }
        delete it;
    }
    j["cases"] = cases;
    j["tasks"] = tasks;
    Json staff = Json::array();
    for (std::map<std::string, Json>::const_iterator it = people.begin(); it != people.end(); ++it) staff.push_back(it->second);
    j["people"] = staff;

    Json feed = Json::array();
    const size_t kMaxFeed = 400;
    for (size_t i = catalog->feed.size(); i > 0 && feed.size() < kMaxFeed; --i) {
        const Activity& a = catalog->feed[i - 1];
        char clock[16];
        std::strftime(clock, sizeof clock, "%H:%M:%S", std::localtime(&a.when));
        feed.push_back({{"seq", a.seq}, {"time", clock}, {"at", (long long)a.when}, {"caseId", a.caseId}, {"workflow", a.workflow},
                        {"organisation", a.organisation}, {"workspace", a.workspace}, {"kind", a.kind},
                        {"text", a.text}, {"who", a.who}});
    }
    j["activity"] = feed;
    j["currentCase"] = scenario != nullptr ? Json(scenario->instance->getId()) : Json(nullptr);
    return j.dump();
}

std::string TividyService::report(const std::string& type) {
    std::lock_guard<std::mutex> guard(lock);
    Json j;
    if (scenario == nullptr) {
        j["error"] = "No industry is loaded. Pick one first.";
        return j.dump();
    }
    std::ostringstream out;
    if (type == "progress") {
        ReportScreen::showProgress(*scenario->instance->getRoot(), out);
    } else if (type == "escalations") {
        ReportScreen::showEscalations(*scenario->instance->getRoot(), out);
    } else {
        j["error"] = "Unknown report '" + type + "'.";
        return j.dump();
    }
    j["reportType"] = type;
    j["content"] = out.str();
    return j.dump();
}
