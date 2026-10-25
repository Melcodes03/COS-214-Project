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

static int failures = 0;
static void check(bool ok, const char* what){
    std::cout << (ok ? "  pass  " : "  FAIL  ") << what << std::endl;
    if (!ok) failures++;
}
static bool neverValid(const WorkItem&) { return false; }

static TransitionCommand* cmd(WorkItem* item, TransitionCommand::Transition t, const char* name){
    return new TransitionCommand(item, t, name);
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
        Participant thandi("Thandi", "clerk");
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
        Participant sipho("Sipho", "clerk");
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
        Participant p1("P1", "clerk"), p2("P2", "clerk");
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
        Participant p("P", "clerk");
        CommandHistory h;
        item->makeAvailable(); item->assign(); p.addTask();
        p.start(item, h);
        check(!p.complete(item, h), "validation still blocks a worker's complete");
        check(p.getActiveTasks() == 1 && item->getStateName() == "InProgress", "blocked complete kept the worker's task");
        delete item;
    }

    std::cout << (failures == 0 ? "All Command/Memento tests passed." : "Some tests FAILED.") << std::endl;
    return failures == 0 ? 0 : 1;
}
