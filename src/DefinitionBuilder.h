#ifndef DEFINITIONBUILDER_H
#define DEFINITIONBUILDER_H

#include "WorkflowBuilder.h"
#include <string>
#include <vector>

class Stage;
class WorkItem;
class WorkflowDefinition;

/// Builder: ConcreteBuilder. Builds a WorkflowDefinition (the reusable template).
///
/// It keeps a stack of open stages. A task that was just added waits in
/// "pending" so that withPriority(), withValidation() and withAudit() can wrap it
/// in the matching concrete Decorator. The pending task is placed into its stage
/// as soon as the next step starts (another task, a new stage, endStage() or
/// getResult()).
///
/// This class creates every concrete Decorator, so Directors never need to know
/// the decorator classes.
class DefinitionBuilder : public WorkflowBuilder {
    private:
        std::string name;
        int version;
        Stage* root;                        //owned until getResult() hands it over
        std::vector<Stage*> openStages;     //openStages[0] is always the root
        WorkItem* pending;                  //task added last, not yet placed in its stage

        void placePending();                //moves pending into the open stage

    public:
        DefinitionBuilder(const std::string& name, int version);
        ~DefinitionBuilder() override;

        //owns raw pointers, so copying is disabled
        DefinitionBuilder(const DefinitionBuilder&) = delete;
        DefinitionBuilder& operator=(const DefinitionBuilder&) = delete;

        WorkflowBuilder& beginStage(const std::string& id, const std::string& name) override;
        WorkflowBuilder& addTask(const std::string& id, const std::string& name, const std::string& role) override;
        WorkflowBuilder& withPriority(int priority) override;
        WorkflowBuilder& withValidation(ValidationDecorator::Rule rule) override;
        WorkflowBuilder& withAudit() override;
        WorkflowBuilder& endStage() override;

        /// Finishes the build and returns the definition. The caller owns it.
        /// The builder is single-use: a second call returns nullptr, and any steps
        /// called after getResult() are ignored.
        WorkflowDefinition* getResult();
};

#endif //DEFINITIONBUILDER_H
