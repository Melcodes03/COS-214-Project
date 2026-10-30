#ifndef SUPPORTDIRECTOR_H
#define SUPPORTDIRECTOR_H

#include "WorkflowDirector.h"

/// Builder: ConcreteDirector for Industry 1, software customer support.
///
/// A customer's support case is logged, investigated, resolved and closed.
/// Task priorities match the escalation levels used by the Chain of
/// Responsibility: 1 = General Assistant, 2 = Technical Assistant,
/// 3 = Engineer, anything higher goes to the Support Manager.
class SupportDirector : public WorkflowDirector {
    public:
        void construct(WorkflowBuilder& builder) override;
        std::string getIndustryName() const override;
};

#endif //SUPPORTDIRECTOR_H
