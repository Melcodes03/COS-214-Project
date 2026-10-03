#ifndef AUDITINGDECORATOR_H
#define AUDITINGDECORATOR_H

#include "WorkItemDecorator.h"
#include <vector>

//Decorator: ConcreteDecorator. Records every lifecycle operation attempted on
//the work item, including refused ones.
class AuditingDecorator : public WorkItemDecorator {
    private:
        std::vector<std::string> log;

        bool audited(const char* operation, bool (WorkItem::*action)());

    public:
        explicit AuditingDecorator(WorkItem* component);

        bool makeAvailable() override;
        bool assign() override;
        bool start() override;
        bool complete() override;
        bool reject() override;
        bool escalate() override;
        bool cancel() override;

        const std::vector<std::string>& getAuditLog() const;
        WorkItem* clone() const override;
};

#endif //AUDITINGDECORATOR_HS