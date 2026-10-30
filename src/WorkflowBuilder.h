#ifndef WORKFLOWBUILDER_H
#define WORKFLOWBUILDER_H

#include <string>
#include "ValidationDecorator.h"

/// Builder: abstract Builder.
///
/// Describes a workflow process one step at a time: open a stage, add tasks to it,
/// optionally decorate the task that was just added, then close the stage.
/// Every step returns the builder, so a Director can chain the calls:
///
///     builder.beginStage("S1", "Intake")
///                .addTask("T1", "Log case", "General Assistant").withPriority(1)
///            .endStage();
///
/// The steps say nothing about what is being produced. DefinitionBuilder turns
/// them into a runnable WorkflowDefinition; OutlineBuilder turns the same steps
/// into a readable text outline. A Director therefore never changes when a new
/// kind of product is needed.
class WorkflowBuilder {
    public:
        virtual ~WorkflowBuilder() {}

        /// Opens a new stage inside the stage that is currently open.
        virtual WorkflowBuilder& beginStage(const std::string& id, const std::string& name) = 0;

        /// Adds a task to the open stage. @p role is the role allowed to do the task.
        virtual WorkflowBuilder& addTask(const std::string& id, const std::string& name, const std::string& role) = 0;

        /// Decorates the task added last. Ignored if there is no such task.
        virtual WorkflowBuilder& withPriority(int priority) = 0;
        virtual WorkflowBuilder& withValidation(ValidationDecorator::Rule rule) = 0;
        virtual WorkflowBuilder& withAudit() = 0;

        /// Closes the stage opened last. The outermost (root) stage is never closed.
        virtual WorkflowBuilder& endStage() = 0;
};

#endif //WORKFLOWBUILDER_H
