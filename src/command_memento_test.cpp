#include <iostream>
#include <string>
#include "Task.h"
#include "Stage.h"
#include "WorkflowDefinition.h"
#include "WorkflowInstance.h"
#include "PriorityDecorator.h"
#include "ValidationDecorator.h"
#include "AuditingDecorator.h"
#include "WorkItemMemento.h"
#include "TransitionCommand.h"
#include "CommandHistory.h"
#include "Observer.h"
#include "Participant.h"
#include "WorkAllocator.h"
#include "LeastWorkloadAssignment.h"
#include "LevelHandler.h"
#include "ManagerHandler.h"
#include "AllocationCommand.h"
#include "ApprovalCommand.h"
#include "ApprovalRequest.h"
#include "SingleApproval.h"
#include "MultiLevelApproval.h"

static int failures = 0;
static void check(bool ok, const char* what){
    std::cout << (ok ? "  pass  " : "  FAIL  ") << what << std::endl;
    if (!ok) failures++;
}
static bool neverValid(const WorkItem&) { return false; }

static TransitionCommand* cmd(WorkItem* item, TransitionCommand::Transition t, const char* name){
    return new TransitionCommand(item, t, name);
}


//the invariant the bugs broke: a worker's task count equals the items on their worklist
static bool consistent(Participant& p){
    return p.getActiveTasks() == (int)p.getWorklist().size();
}
static int totalLoad(Participant& a, Participant& b){
    return a.getActiveTasks() + b.getActiveTasks();
}

//records every notification it receives
struct SpyObserver : public Observer {
    int calls;
    std::string lastOld, lastNew;
    explicit SpyObserver(WorkItem* subject) : Observer(subject), calls(0) {}
    void update(WorkItemState* newState, std::string oldState) override {
        calls++;
        lastOld = oldState;
        lastNew = newState->getName();
    }
};

int main(){
    std::cout << "Command + Memento tests" << std::endl;

    //1. undo / redo of one item, and Undo respects the lifecycle
    {
        Task t("T", "Task");
        CommandHistory h;
        check(h.execute(cmd(&t, &WorkItem::makeAvailable, "makeAvailable")), "execute makeAvailable");
        check(h.execute(cmd(&t, &WorkItem::assign, "assign")), "execute assign");
        check(t.getStateName() == "Assigned", "state is Assigned");
        check(h.undo() && t.getStateName() == "Available", "undo assign -> Available");
        check(h.undo() && t.getStateName() == "Created", "undo makeAvailable -> Created");
        check(!h.undo(), "nothing left to undo");
        check(h.redo() && t.getStateName() == "Available", "redo makeAvailable");
        check(h.redo() && t.getStateName() == "Assigned", "redo assign");
    }

    //2. a refused command is not recorded and changes nothing
    {
        Task t("T", "Task");
        CommandHistory h;
        check(!h.execute(cmd(&t, &WorkItem::complete, "complete")), "complete in Created is refused");
        check(h.undoCount() == 0 && t.getStateName() == "Created", "refused command left no trace");
    }

    //3. a new command clears the redo branch
    {
        Task t("T", "Task");
        CommandHistory h;
        h.execute(cmd(&t, &WorkItem::makeAvailable, "makeAvailable"));
        h.undo();
        check(h.redoCount() == 1, "one redo available");
        h.execute(cmd(&t, &WorkItem::cancel, "cancel"));
        check(h.redoCount() == 0 && t.getStateName() == "Cancelled", "new action cleared redo");
    }

    //4. undo restores the retry counter, not just the state name
    {
        Task t("T", "Task");
        t.setMaxRetries(1);
        CommandHistory h;
        h.execute(cmd(&t, &WorkItem::makeAvailable, "makeAvailable"));
        h.execute(cmd(&t, &WorkItem::assign, "assign"));
        h.execute(cmd(&t, &WorkItem::start, "start"));
        h.execute(cmd(&t, &WorkItem::reject, "reject"));
        check(t.getStateName() == "Rejected" && t.getRetryCount() == 0, "Rejected, no retries used");
        h.execute(cmd(&t, &WorkItem::assign, "assign (rework)"));
        check(t.getRetryCount() == 1, "rework used a retry");
        h.undo();
        check(t.getStateName() == "Rejected" && t.getRetryCount() == 0, "undo gave the retry back");
        check(h.redo() && t.getRetryCount() == 1, "redo uses it again");
    }

    //5. Memento is deep: nested Stage -> Stage -> Task all restored independently
    {
        Stage* root = new Stage("P", "Process");
        Stage* a = new Stage("A", "Stage A");
        Task* a1 = new Task("A1", "A1");
        Task* b = new Task("B", "B");
        a->add(a1);
        root->add(a);
        root->add(b);

        a1->makeAvailable();
        WorkItemMemento* snap = root->createMemento();

        a1->assign(); a1->start();
        b->makeAvailable();
        a->makeAvailable();
        check(a1->getStateName() == "InProgress" && b->getStateName() == "Available", "tree changed after snapshot");

        check(root->restore(*snap), "restore succeeded");
        check(a1->getStateName() == "Available", "nested task A1 restored");
        check(b->getStateName() == "Created", "sibling B restored");
        check(a->getStateName() == "Created", "middle stage A restored");

        //snapshot must stay valid and reusable
        a1->assign();
        check(root->restore(*snap) && a1->getStateName() == "Available", "same snapshot restores twice");
        delete snap;
        delete root;
    }

    //6. restore refuses if the tree changed shape, and changes nothing
    {
        Stage root("P", "Process");
        Task* x = new Task("X", "X");
        root.add(x);
        WorkItemMemento* snap = root.createMemento();
        x->makeAvailable();
        root.add(new Task("Y", "Y"));
        check(!root.restore(*snap), "restore refused: a child was added");
        check(x->getStateName() == "Available", "refused restore left the tree untouched");
        delete snap;
    }

    //7. snapshot / restore through decorators
    {
        AuditingDecorator* audit = new AuditingDecorator(
            new ValidationDecorator(new PriorityDecorator(new Task("D", "Decorated"), 5), neverValid));
        WorkItem* item = audit;
        CommandHistory h;
        h.execute(cmd(item, &WorkItem::makeAvailable, "makeAvailable"));
        h.execute(cmd(item, &WorkItem::assign, "assign"));
        h.execute(cmd(item, &WorkItem::start, "start"));
        check(!h.execute(cmd(item, &WorkItem::complete, "complete")), "validation decorator still blocks complete via a Command");
        check(h.undo() && item->getStateName() == "Assigned", "undo through 3 decorators");
        check(item->getPriority() == 5, "decorator data untouched by undo");
        delete item;
    }

    //8. a decorated child inside a Stage is snapshotted too
    {
        Stage root("P", "Process");
        root.add(new PriorityDecorator(new Task("C", "Child"), 3));
        WorkItemMemento* snap = root.createMemento();
        root.getChild(0)->makeAvailable();
        check(root.restore(*snap) && root.getChild(0)->getStateName() == "Created", "decorated child restored");
        delete snap;
    }

    //9. rollback to a marked point, on a real WorkflowInstance
    {
        Stage* process = new Stage("P", "Process");
        process->add(new Task("A", "A"));
        WorkflowDefinition def("Generic", 1, process);
        WorkflowInstance inst("I1", &def);
        WorkflowInstance other("I2", &def);

        WorkItem* task = inst.getRoot()->getChild(0);
        CommandHistory h;
        h.execute(cmd(task, &WorkItem::makeAvailable, "makeAvailable"));
        int checkpoint = h.mark();
        h.execute(cmd(task, &WorkItem::assign, "assign"));
        h.execute(cmd(task, &WorkItem::start, "start"));
        h.execute(cmd(task, &WorkItem::complete, "complete"));
        check(task->getStateName() == "Completed", "work item completed");
        check(h.rollbackTo(checkpoint) == 3, "rollback undid 3 commands");
        check(task->getStateName() == "Available", "rolled back to the checkpoint");
        check(other.getRoot()->getChild(0)->getStateName() == "Created", "other instance never affected");
        check(h.lastCommandName() == "makeAvailable", "history still holds the earlier command");
    }


    //10. undo tells observers (Revision 4)
    {
        Task t("T", "Task");
        SpyObserver spy(&t);
        CommandHistory h;
        h.execute(cmd(&t, &WorkItem::makeAvailable, "makeAvailable"));
        h.execute(cmd(&t, &WorkItem::assign, "assign"));
        check(spy.calls == 2, "observer told about both executes");
        h.undo();
        check(spy.calls == 3, "observer told about the undo");
        check(spy.lastOld == "Assigned" && spy.lastNew == "Available", "undo notification says Assigned -> Available");
        h.redo();
        check(spy.calls == 4 && spy.lastNew == "Assigned", "observer told about the redo");
    }

    //11. undo through decorators still reaches the observer; rollback notifies once per step
    {
        AuditingDecorator* item = new AuditingDecorator(new PriorityDecorator(new Task("D", "Decorated"), 1));
        {
            SpyObserver spy(item);   //attach() is forwarded to the wrapped item
            CommandHistory h;
            h.execute(cmd(item, &WorkItem::makeAvailable, "makeAvailable"));
            int mark = h.mark();
            h.execute(cmd(item, &WorkItem::assign, "assign"));
            h.execute(cmd(item, &WorkItem::start, "start"));
            int before = spy.calls;
            h.rollbackTo(mark);
            check(spy.calls == before + 2, "rollback of 2 steps notified twice, through decorators");
        }   //the observer must go before the item it watches
        delete item;
    }

    //12. an undo that changes nothing does not notify
    {
        Task t("T", "Task");
        SpyObserver spy(&t);
        WorkItemMemento* snap = t.createMemento();   //Created
        t.restore(*snap);                            //already Created
        check(spy.calls == 0, "restoring an unchanged state sends no notification");
        delete snap;
    }

    //13. undo is written to the audit log (Revision 4)
    {
        AuditingDecorator* a = new AuditingDecorator(new Task("A", "A"));
        CommandHistory h;
        h.execute(cmd(a, &WorkItem::makeAvailable, "makeAvailable"));
        h.execute(cmd(a, &WorkItem::assign, "assign"));
        h.undo();
        const std::vector<std::string>& log = a->getAuditLog();
        check(log.size() == 3, "audit log has 2 actions + 1 undo");
        check(log.back() == "restore: Assigned -> Available", "audit entry describes the undo");
        delete a;
    }

    //14. history listing
    {
        Task t("T", "Task");
        CommandHistory h;
        check(h.getHistory().empty(), "history starts empty");
        h.execute(cmd(&t, &WorkItem::makeAvailable, "makeAvailable"));
        h.execute(cmd(&t, &WorkItem::assign, "assign"));
        h.execute(cmd(&t, &WorkItem::complete, "complete"));   //refused: Assigned cannot complete
        std::vector<std::string> list = h.getHistory();
        check(list.size() == 2 && list[0] == "makeAvailable" && list[1] == "assign", "listed oldest first, refused command absent");
        h.undo();
        check(h.getHistory().size() == 1, "undo removes the entry from history");
        std::vector<std::string> redo = h.getRedoList();
        check(redo.size() == 1 && redo[0] == "assign", "undone command appears in redo list");
    }

    //15. worker commands: start / complete update the item and the workload, and undo reverts both
    {
        Task t("T", "Report");
        Participant thandi("Thandi", "clerk", 4);
        CommandHistory h;
        t.makeAvailable(); t.assign(); thandi.addTask();   //what WorkAllocator::allocate does
        check(thandi.start(&t, h) && t.getStateName() == "InProgress", "worker starts the item");
        check(thandi.complete(&t, h) && t.getStateName() == "Completed", "worker completes the item");
        check(thandi.getActiveTasks() == 0, "completing freed a task");
        std::vector<std::string> list = h.getHistory();
        check(list.size() == 2 && list[1] == "Thandi completes Report", "history names who did what");
        h.undo();
        check(t.getStateName() == "InProgress" && thandi.getActiveTasks() == 1, "undo complete: item and workload both back");
        h.redo();
        check(t.getStateName() == "Completed" && thandi.getActiveTasks() == 0, "redo complete frees the task again");
    }

    //16. decline hands the item back, frees the worker, and is undoable
    {
        Task t("T", "Report");
        Participant sipho("Sipho", "clerk", 6);
        CommandHistory h;
        t.makeAvailable(); t.assign(); sipho.addTask();
        check(sipho.decline(&t, h) && t.getStateName() == "Available", "decline from Assigned returns item to Available");
        check(sipho.getActiveTasks() == 0, "decline freed a task");
        h.undo();
        check(t.getStateName() == "Assigned" && sipho.getActiveTasks() == 1, "undo decline restores item and workload");
    }

    //17. refused worker actions change nothing, and workers do not affect each other
    {
        Task a("A", "A"), b("B", "B");
        Participant p1("P1", "clerk", 4), p2("P2", "clerk", 4);
        CommandHistory h;
        a.makeAvailable(); a.assign(); p1.addTask();
        b.makeAvailable(); b.assign(); p2.addTask();
        check(!p1.complete(&a, h), "complete before start is refused");
        check(a.getStateName() == "Assigned" && p1.getActiveTasks() == 1 && h.getHistory().empty(), "refused action left no trace");
        check(!p1.start(nullptr, h), "null item is refused");
        p1.start(&a, h); p1.complete(&a, h);
        check(p1.getActiveTasks() == 0 && p2.getActiveTasks() == 1 && b.getStateName() == "Assigned", "other worker and item untouched");
    }

    //18. worker action through a Validation decorator is still blocked
    {
        WorkItem* item = new ValidationDecorator(new Task("V", "Checked"), neverValid);
        Participant p("P", "clerk", 4);
        CommandHistory h;
        item->makeAvailable(); item->assign(); p.addTask();
        p.start(item, h);
        check(!p.complete(item, h), "validation still blocks a worker's complete");
        check(p.getActiveTasks() == 1 && item->getStateName() == "InProgress", "blocked complete kept the worker's task");
        delete item;
    }


    //19. BUG 1 fixed: undoing "complete" puts the item back on the worker's worklist
    {
        Task t("T", "Report");
        Participant p("Thandi", "clerk", 4);
        WorkAllocator a(new LeastWorkloadAssignment());
        a.addParticipant(&p);
        CommandHistory h;
        t.makeAvailable(); a.allocate(t);
        p.start(&t, h); p.complete(&t, h);
        check(p.getWorklist().empty() && p.getActiveTasks() == 0, "after complete: worklist and workload empty");
        h.undo();
        check(t.getStateName() == "InProgress", "undo complete: item back in progress");
        check(p.getWorklist().size() == 1 && p.getWorklist()[0] == &t, "undo complete: item back on the worklist");
        check(consistent(p), "undo complete: workload matches worklist");
        h.redo();
        check(p.getWorklist().empty() && consistent(p), "redo complete: off the worklist again");
    }

    //20. BUG 2 fixed: declining takes the item off the worklist, and undo restores its position
    {
        Task t1("T1", "First"), t2("T2", "Second");
        Participant p("Sipho", "clerk", 4);
        WorkAllocator a(new LeastWorkloadAssignment());
        a.addParticipant(&p);
        CommandHistory h;
        t1.makeAvailable(); a.allocate(t1);
        t2.makeAvailable(); a.allocate(t2);
        check(p.getWorklist().size() == 2 && p.getWorklist()[0] == &t1, "worker holds both, T1 first");
        check(p.decline(&t1, h) && t1.getStateName() == "Available", "declined T1 returns to Available");
        check(p.getWorklist().size() == 1 && p.getWorklist()[0] == &t2 && consistent(p), "declined item left the worklist");
        h.undo();
        check(p.getWorklist().size() == 2 && p.getWorklist()[0] == &t1 && p.getWorklist()[1] == &t2, "undo decline: T1 back in its original position");
        check(consistent(p) && t1.getStateName() == "Assigned", "undo decline: workload matches worklist");
    }

    //21. allocation goes through history: undoable, redoable, and refused cleanly
    {
        Task t("T", "Report");
        Participant p("Thandi", "clerk", 4);
        std::vector<Participant*> everyone(1, &p);
        WorkAllocator a(new LeastWorkloadAssignment());
        a.addParticipant(&p);
        CommandHistory h;
        check(!h.execute(new AllocationCommand(&a, &t, everyone, &WorkAllocator::allocate, "Assign Report")), "allocating a Created item is refused");
        check(h.getHistory().empty() && p.getActiveTasks() == 0, "refused allocation left no trace");
        t.makeAvailable();
        check(h.execute(new AllocationCommand(&a, &t, everyone, &WorkAllocator::allocate, "Assign Report")), "allocation of an Available item works");
        check(t.getStateName() == "Assigned" && a.getAssignee("T") == &p && p.getActiveTasks() == 1 && consistent(p), "item assigned, recorded, worker loaded");
        h.undo();
        check(t.getStateName() == "Available" && a.getAssignee("T") == nullptr, "undo: item Available, allocator forgot it");
        check(p.getActiveTasks() == 0 && p.getWorklist().empty(), "undo: worker freed");
        h.redo();
        check(t.getStateName() == "Assigned" && a.getAssignee("T") == &p && consistent(p), "redo: assigned again");
    }

    //22. BUG 3 fixed: decline, re-assign, then undo everything: the workload stays correct at every step
    {
        Task t("T", "Report");
        Participant p1("P1", "clerk", 4), p2("P2", "clerk", 4);
        std::vector<Participant*> everyone;
        everyone.push_back(&p1); everyone.push_back(&p2);
        WorkAllocator a(new LeastWorkloadAssignment());
        a.addParticipant(&p1); a.addParticipant(&p2);
        CommandHistory h;
        t.makeAvailable();
        h.execute(new AllocationCommand(&a, &t, everyone, &WorkAllocator::allocate, "Assign"));
        Participant* first = a.getAssignee("T");
        check(first != nullptr && totalLoad(p1, p2) == 1, "assigned: one task in total");
        first->decline(&t, h);
        check(totalLoad(p1, p2) == 0 && consistent(p1) && consistent(p2), "declined: no task in total");
        h.execute(new AllocationCommand(&a, &t, everyone, &WorkAllocator::allocate, "Assign again"));
        check(totalLoad(p1, p2) == 1 && t.getStateName() == "Assigned" && consistent(p1) && consistent(p2), "re-assigned: one task in total");
        h.undo();
        check(totalLoad(p1, p2) == 0 && t.getStateName() == "Available" && consistent(p1) && consistent(p2), "undo re-assign: no task in total");
        h.undo();
        check(totalLoad(p1, p2) == 1 && t.getStateName() == "Assigned" && a.getAssignee("T") == first, "undo decline: back with the first worker, one task in total");
        check(consistent(p1) && consistent(p2), "worklists agree with workloads");
        h.undo();
        check(totalLoad(p1, p2) == 0 && t.getStateName() == "Available" && a.getAssignee("T") == nullptr, "undo first assignment: nothing held");
    }

    //23. escalate and reassign through history, with a chain of levels
    {
        Task t("T", "Report");
        Participant clerk("Thandi", "clerk", 4), boss("Naledi", "Manager", 4);
        std::vector<Participant*> everyone;
        everyone.push_back(&clerk); everyone.push_back(&boss);
        WorkAllocator a(new LeastWorkloadAssignment());
        LevelHandler* level = new LevelHandler("clerk", 1, new LeastWorkloadAssignment());
        level->setNext(new ManagerHandler(&boss));
        a.setChain(level);
        a.addParticipant(&clerk); a.addParticipant(&boss);
        CommandHistory h;
        t.makeAvailable();
        h.execute(new AllocationCommand(&a, &t, everyone, &WorkAllocator::allocate, "Assign"));
        clerk.start(&t, h);
        check(a.getAssignee("T") == &clerk && t.getStateName() == "InProgress", "clerk has it and started");
        check(h.execute(new AllocationCommand(&a, &t, everyone, &WorkAllocator::escalate, "Escalate")), "no response: escalated");
        check(a.getAssignee("T") == &boss && boss.getActiveTasks() == 1 && clerk.getActiveTasks() == 0, "manager has it, clerk freed");
        h.undo();
        check(t.getStateName() == "InProgress" && a.getAssignee("T") == &clerk, "undo escalate: back with the clerk, in progress");
        check(clerk.getActiveTasks() == 1 && clerk.getWorklist().size() == 1 && boss.getActiveTasks() == 0 && boss.getWorklist().empty(), "undo escalate: both workloads and worklists restored");
        check(a.getLevel("T") != nullptr, "undo escalate: allocator remembers the clerk level");
    }
    {
        Task t("T", "Report");
        Participant clerk("Thandi", "clerk", 4), boss("Naledi", "Manager", 4);
        std::vector<Participant*> everyone;
        everyone.push_back(&clerk); everyone.push_back(&boss);
        WorkAllocator a(new LeastWorkloadAssignment());
        LevelHandler* level = new LevelHandler("clerk", 1, new LeastWorkloadAssignment());
        level->setNext(new ManagerHandler(&boss));
        a.setChain(level);
        a.addParticipant(&clerk); a.addParticipant(&boss);
        CommandHistory h;
        t.makeAvailable();
        h.execute(new AllocationCommand(&a, &t, everyone, &WorkAllocator::allocate, "Assign"));
        clerk.decline(&t, h);
        check(h.execute(new AllocationCommand(&a, &t, everyone, &WorkAllocator::reassign, "Reassign")) && a.getAssignee("T") == &boss, "declined item passed up to the manager");
        h.undo();
        h.undo();
        check(t.getStateName() == "Assigned" && a.getAssignee("T") == &clerk && clerk.getActiveTasks() == 1 && boss.getActiveTasks() == 0, "undo reassign and decline: back with the clerk");
        check(consistent(clerk) && consistent(boss), "worklists agree with workloads");
    }

    //24. BUG 4 fixed: approvals go through history
    {
        Task t("T", "Report");
        t.makeAvailable(); t.assign(); t.start();
        ApprovalRequest req(&t, new SingleApproval());
        CommandHistory h;
        check(h.execute(new ApprovalCommand(&req, &t, true, "Approver approves")), "an approval vote is recorded");
        check(t.getStateName() == "Completed" && req.getDecision() == ApprovalStrategy::APPROVED, "the vote completed the item");
        check(!h.execute(new ApprovalCommand(&req, &t, false, "Late vote")), "a vote after the decision is refused");
        check(h.getHistory().size() == 1, "the refused vote is not in the history");
        h.undo();
        check(t.getStateName() == "InProgress" && req.getDecision() == ApprovalStrategy::PENDING && req.voteCount() == 0, "undo: item in progress, request pending, vote taken back");
        check(h.execute(new ApprovalCommand(&req, &t, false, "Approver rejects")) && t.getStateName() == "Rejected" && req.getDecision() == ApprovalStrategy::REJECTED, "a different vote after the undo rejects the item");
        h.undo();
        h.redo();
        check(t.getStateName() == "Rejected" && req.voteCount() == 1, "redo repeats the rejection");
    }
    {
        Task t("T", "Report");
        t.makeAvailable(); t.assign(); t.start();
        ApprovalRequest req(&t, new MultiLevelApproval(2));
        CommandHistory h;
        h.execute(new ApprovalCommand(&req, &t, true, "Level 1 approves"));
        check(req.getDecision() == ApprovalStrategy::PENDING && t.getStateName() == "InProgress" && req.voteCount() == 1, "first of two approvals leaves it pending");
        h.execute(new ApprovalCommand(&req, &t, true, "Level 2 approves"));
        check(t.getStateName() == "Completed", "second approval completes the item");
        h.rollbackTo(0);
        check(t.getStateName() == "InProgress" && req.voteCount() == 0 && req.getDecision() == ApprovalStrategy::PENDING, "rollback takes back both votes");
    }

    //25. undoing an approval tells observers and the audit log
    {
        AuditingDecorator* item = new AuditingDecorator(new Task("A", "Audited"));
        item->makeAvailable(); item->assign(); item->start();
        {
            SpyObserver spy(item);
            ApprovalRequest req(item, new SingleApproval());
            CommandHistory h;
            h.execute(new ApprovalCommand(&req, item, true, "Approve"));
            int calls = spy.calls;
            h.undo();
            check(spy.calls == calls + 1 && spy.lastOld == "Completed" && spy.lastNew == "InProgress", "undoing the approval notified the observer");
            check(item->getAuditLog().back() == "restore: Completed -> InProgress", "undoing the approval is in the audit log");
        }
        delete item;
    }

    std::cout << (failures == 0 ? "All Command/Memento tests passed." : "Some tests FAILED.") << std::endl;
    return failures == 0 ? 0 : 1;
}
