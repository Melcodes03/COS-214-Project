#include <iostream>
#include <string>
#include "Task.h"
#include "Stage.h"
#include "WorkflowDefinition.h"
#include "WorkflowInstance.h"
#include "PriorityDecorator.h"
#include "AuditingDecorator.h"
#include "ProgressReportVisitor.h"
#include "EscalationReportVisitor.h"
#include "Participant.h"
#include "WorkAllocator.h"
#include "RoleBasedAssignment.h"
#include "LeastWorkloadAssignment.h"
#include "ApprovalRequest.h"
#include "SingleApproval.h"
#include "MultiLevelApproval.h"

static int failures = 0;
static void check(bool ok, const char* what){
    std::cout << (ok ? "  pass  " : "  FAIL  ") << what << std::endl;
    if (!ok) failures++;
}

//Process -> (Stage A -> A1, A2), Task B
static WorkItem* buildTemplate(){
    Stage* process = new Stage("P", "Process");
    Stage* stageA = new Stage("A", "Stage A");
    stageA->add(new Task("A1", "Task A1"));
    stageA->add(new Task("A2", "Task A2"));
    process->add(stageA);
    process->add(new Task("B", "Task B"));
    return process;
}

static void toInProgress(WorkItem* w){ w->makeAvailable(); w->assign(); w->start(); }

int main(){
    std::cout << "Visitor + Strategy tests" << std::endl;

    //1. Visitor: progress report over a running instance
    {
        WorkflowDefinition def("Generic", 1, buildTemplate());
        WorkflowInstance run("I1", &def);
        WorkItem* root = run.getRoot();
        WorkItem* a1 = root->getChild(0)->getChild(0);
        toInProgress(a1); a1->complete();

        ProgressReportVisitor report;
        report.visitAll(*root);
        check(report.getStageCount() == 2, "progress: 2 stages counted");
        check(report.getTaskCount() == 3, "progress: 3 tasks counted");
        check(report.countInState("Completed") == 1, "progress: 1 task completed");
        check(report.percentComplete() == 33, "progress: 33% complete");
        check(report.summary() == "2 stages, 3 tasks, 33% complete", "progress: summary text");
    }

    //2. Visitor: escalation report names the stage each problem task is in
    {
        WorkItem* root = buildTemplate();
        WorkItem* a2 = root->getChild(0)->getChild(1);
        WorkItem* b = root->getChild(1);
        a2->makeAvailable(); a2->assign(); a2->escalate();
        toInProgress(b); b->reject();

        EscalationReportVisitor esc;
        esc.visitAll(*root);
        check(esc.getFlagged().size() == 2, "escalation: 2 items flagged");
        check(esc.getFlagged()[0] == "A2 (Stage A): Escalated", "escalation: A2 escalated in Stage A");
        check(esc.getFlagged()[1] == "B (Process): Rejected", "escalation: B rejected in Process");
        delete root;
    }

    //3. Visitor sees through decorators (a decorated task is still a task)
    {
        Stage holder("H", "Holder");
        holder.add(new AuditingDecorator(new PriorityDecorator(new Task("D", "Decorated"), 5)));
        ProgressReportVisitor report;
        report.visitAll(holder);
        check(report.getTaskCount() == 1 && report.getStageCount() == 1, "visitor: decorated task counted as a task");
    }

    //4. Strategy: role-based assignment, then swap to least-workload at runtime
    {
        Participant clerk("Thandi", "Clerk"), manager("Sipho", "Manager"), clerk2("Ayanda", "Clerk");
        WorkAllocator allocator(new RoleBasedAssignment("Manager"));
        allocator.addParticipant(&clerk);
        allocator.addParticipant(&manager);
        allocator.addParticipant(&clerk2);

        Task t1("T1", "Approve budget"); t1.makeAvailable();
        check(allocator.allocate(t1) == &manager, "role-based: manager gets the task");
        check(t1.getStateName() == "Assigned", "role-based: task moved to Assigned");
        check(allocator.getAssignee("T1") == &manager, "role-based: assignee recorded");

        allocator.setStrategy(new LeastWorkloadAssignment());
        Task t2("T2", "Capture form"); t2.makeAvailable();
        check(allocator.allocate(t2) == &clerk, "least-workload: idle participant chosen");
        check(clerk.getActiveTasks() == 1 && manager.getActiveTasks() == 1, "least-workload: workloads updated");

        Task t3("T3", "Not ready");   //still Created, so State refuses assign
        check(allocator.allocate(t3) == nullptr && allocator.getAssignee("T3") == nullptr, "allocate refused when State says no");
        check(clerk2.getActiveTasks() == 0, "refused allocation leaves workload unchanged");

        WorkAllocator nobody(new RoleBasedAssignment("Auditor"));
        nobody.addParticipant(&clerk);
        Task t4("T4", "Audit"); t4.makeAvailable();
        check(nobody.allocate(t4) == nullptr && t4.getStateName() == "Available", "no matching role: item stays Available");
    }

    //5. Strategy: single vs multi-level approval drive the State transitions
    {
        Task t("S", "Single"); toInProgress(&t);
        ApprovalRequest single(&t, new SingleApproval());
        check(single.vote(true) == ApprovalStrategy::APPROVED && t.getStateName() == "Completed", "single approval completes the item");

        Task m("M", "Multi"); toInProgress(&m);
        ApprovalRequest multi(&m, new MultiLevelApproval(2));
        check(multi.vote(true) == ApprovalStrategy::PENDING && m.getStateName() == "InProgress", "multi-level: pending after level 1");
        check(multi.vote(true) == ApprovalStrategy::APPROVED && m.getStateName() == "Completed", "multi-level: approved after level 2");

        Task r("R", "Rejected"); toInProgress(&r);
        ApprovalRequest multiNo(&r, new MultiLevelApproval(3));
        multiNo.vote(true);
        check(multiNo.vote(false) == ApprovalStrategy::REJECTED && r.getStateName() == "Rejected", "multi-level: one no rejects");
        check(multiNo.vote(true) == ApprovalStrategy::REJECTED, "votes after a decision are ignored");

        Task e("E", "Nobody answers"); toInProgress(&e);
        ApprovalRequest late(&e, new MultiLevelApproval(2));
        check(late.deadlinePassed() && e.getStateName() == "Escalated", "deadline passed: item escalated");
        check(!single.deadlinePassed(), "deadline after a decision does nothing");
    }

    std::cout << (failures == 0 ? "All visitor/strategy tests passed." : "Some tests FAILED") << std::endl;
    return failures == 0 ? 0 : 1;
}
