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

static int failures = 0;
static void check(bool ok, const char* what){
    std::cout << (ok ? "  pass  " : "  FAIL  ") << what << std::endl;
    if (!ok) failures++;
}
static bool neverValid(const WorkItem&) { return false; }

static TransitionCommand* cmd(WorkItem* item, TransitionCommand::Transition t, const char* name){
    return new TransitionCommand(item, t, name);
}

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

    std::cout << (failures == 0 ? "All Command/Memento tests passed." : "Some tests FAILED.") << std::endl;
    return failures == 0 ? 0 : 1;
}
