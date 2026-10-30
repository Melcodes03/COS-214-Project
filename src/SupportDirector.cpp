#include "SupportDirector.h"
#include "WorkflowBuilder.h"

void SupportDirector::construct(WorkflowBuilder& builder) {
    builder
        .beginStage("S1", "Intake")
            .addTask("S1-T1", "Log support case", "General Assistant").withPriority(1).withAudit()
            .addTask("S1-T2", "Classify severity", "General Assistant").withPriority(1)
        .endStage()
        .beginStage("S2", "Investigation")
            .addTask("S2-T1", "Reproduce issue", "Technical Assistant").withPriority(2)
            .addTask("S2-T2", "Diagnose root cause", "Engineer").withPriority(3).withAudit()
        .endStage()
        .beginStage("S3", "Resolution")
            .addTask("S3-T1", "Implement fix", "Engineer").withPriority(3).withAudit()
            .addTask("S3-T2", "Verify fix", "Technical Assistant").withPriority(2)
        .endStage()
        .beginStage("S4", "Closure")
            .addTask("S4-T1", "Confirm resolution with customer", "General Assistant").withPriority(1)
            .addTask("S4-T2", "Close support case", "Support Manager").withPriority(4).withAudit()
        .endStage();
}

std::string SupportDirector::getIndustryName() const {
    return "Software customer support";
}
