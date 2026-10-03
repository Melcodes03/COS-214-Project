#include "AuditingDecorator.h"

AuditingDecorator::AuditingDecorator(WorkItem* component)
    : WorkItemDecorator(component){}

//Runs the operation on the wrapped item and records what happened,
//e.g. "assign: Available -> Assigned" or "complete: InProgress -> refused"
bool AuditingDecorator::audited(const char* operation, bool (WorkItem::*action)()) {
    std::string before = wrapped->getStateName();
    bool ok = (wrapped->*action)();
    log.push_back(std::string(operation) + ": " + before + " -> " +
                  (ok ? wrapped->getStateName() : std::string("refused")));
    return ok;
}

bool AuditingDecorator::makeAvailable() { return audited("makeAvailable", &WorkItem::makeAvailable); }
bool AuditingDecorator::assign() { return audited("assign", &WorkItem::assign); }
bool AuditingDecorator::start() { return audited("start", &WorkItem::start); }
bool AuditingDecorator::complete() { return audited("complete", &WorkItem::complete); }
bool AuditingDecorator::reject() { return audited("reject", &WorkItem::reject); }
bool AuditingDecorator::escalate() { return audited("escalate", &WorkItem::escalate); }
bool AuditingDecorator::cancel() { return audited("cancel", &WorkItem::cancel); }

const std::vector<std::string>& AuditingDecorator::getAuditLog() const { return log; }

WorkItem* AuditingDecorator::clone() const {
    return new AuditingDecorator(wrapped->clone());   //a clone starts with an empty log
}