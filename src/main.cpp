#include <iostream>
#include <string> 
#include "Task.h"
#include "Stage.h"
#include "WorkflowDefinition.h"
#include "WorkflowInstance.h"
#include "WorkItemIterator.h"
#include "PriorityDecorator.h"
#include "ValidationDecorator.h"
#include "AuditingDecorator.h"

static bool alwaysValid(const WorkItem&) { return true; }
static bool neverValid(const WorkItem&) { return false; }


int main(){
    //building a small template: process -> (Stage A -> A1, A2), Task B
    Stage* process = new Stage("P", "Process");
    Stage* stageA = new Stage("A", "Stage A");

    stageA->add(new Task("A1", "Task A1"));
    stageA->add(new Task("A2", "Task A2"));
    process->add(stageA);
    process->add(new Task("B", "Task B"));

    WorkflowDefinition definition("Generic Process", 1, process);
    WorkflowInstance first("I1", &definition);
    WorkflowInstance second("I2", &definition);

    //1. iterator traverses the whole tree
    std::string order = "";
    WorkItemIterator* it = first.getRoot()->createIterator();
    for(it->first(); !it->isDone(); it->next()){
        order += it->currentItem()->getId() + " ";
    }
    delete it;


    //2. leaf rejecting children
    Task leaf("X", "Leaf");
    Task* extra = new Task("Y", "Extra");
    bool leafRejects = !leaf.add(extra);
    delete extra;

    //3. instances have their own copies
    first.getRoot()->add(new Task("C", "Task C"));
    bool independent = second.getRoot()->getChildCount() == 2;

    //4. state lifecycle
    Task t("S", "State test");
    bool stateOk = t.getStateName() == "Created"
                && !t.complete() //invalid in Created
                && t.makeAvailable() && t.assign() && t.start()
                && t.complete()
                && t.getStateName() == "Completed"
                && !t.cancel();    //final state

    //5. retry limit on rework
    Task r("R", "Retry test");
    r.setMaxRetries(1);
    bool retryOk = r.makeAvailable() && r.assign() && r.start()
                && r.reject() && r.getStateName() == "Rejected"
                && r.assign() && r.getStateName() == "Assigned"  //first rework allowed
                && r.getRetryCount() == 1
                && r.start() && r.reject()
                && !r.assign()  //no retries left
                && r.getStateName() == "Rejected"
                && r.cancel() && r.getStateName() == "Cancelled";

    //6. clones start fresh but keep the configured limit
    WorkItem* copy = r.clone();
    bool cloneFresh = copy->getStateName() == "Created" && (copy->getRetryCount() == 0) && (copy->getMaxRetries() == 1);
    delete copy;

    //7. decorators: audit(validation(priority(task))) behaves as a normal task
    AuditingDecorator* audit = new AuditingDecorator(
        new ValidationDecorator(new PriorityDecorator(new Task("D", "Decorated"), 5), alwaysValid));
    WorkItem* item = audit;
    bool decoratedOk = item->getId() == "D" && item->getPriority() == 5
                    && item->makeAvailable() && item->assign() && item->start()
                    && item->complete()
                    && item->getStateName() == "Completed"
                    && audit->getAuditLog().size() == 4
                    && audit->getAuditLog()[0] == "makeAvailable: Created -> Available";

    //8. validation blocks completion; the audit log records the refusal
    AuditingDecorator* audit2 = new AuditingDecorator(
        new ValidationDecorator(new Task("E", "Invalid"), neverValid));
    bool validationOk = audit2->makeAvailable() && audit2->assign() && audit2->start()
                     && !audit2->complete()
                     && audit2->getStateName() == "InProgress"
                     && audit2->getAuditLog().back() == "complete: InProgress -> refused";

    //9. cloning a decorated item gives an independent, fresh copy
    WorkItem* dup = item->clone();
    bool cloneDecoratedOk = dup != item && dup->getStateName() == "Created"
                         && dup->getPriority() == 5 && dup->getId() == "D"
                         && dup->makeAvailable() && item->getStateName() == "Completed";
    delete dup;

    //10. a decorated item inside a Stage is traversed by the iterator
    Stage* holder = new Stage("H", "Holder");
    holder->add(item);   //Stage takes ownership
    std::string holderOrder = "";
    WorkItemIterator* hit = holder->createIterator();
    for (hit->first(); !hit->isDone(); hit->next()){
        holderOrder += hit->currentItem()->getId() + " ";
    }
    delete hit;
    bool compositeOk = holderOrder == "H D ";
    delete holder;       //also deletes the whole decorator chain
    delete audit2;

    bool passed = order == "P A A1 A2 B " && leafRejects && independent
               && stateOk && retryOk && cloneFresh
               && decoratedOk && validationOk && cloneDecoratedOk && compositeOk;
    
    std::cout << (passed ? "All tests passed." : "Tests failed") << std::endl;

    return passed ? 0 : 1;
}

