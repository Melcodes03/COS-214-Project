#ifndef WORKFLOWBUILDER_H
#define WORKFLOWBUILDER_H

#include <string>
#include "ValidationDecorator.h"
#include "Stage.h"   //for Stage::Selector

/// Builder: abstract Builder.
///
/// Describes a workflow process one step at a time: open a stage, add tasks to it,
/// optionally decorate the task that was just added, then close the stage.
/// Every step returns the builder, so a Director can chain the calls:
///
///     builder.beginStage("S1", "Intake").inParallel()
///                .addTask("T1", "Log case", "General Assistant").withPriority(1)
///                .addTask("T2", "Check policy", "Claims Handler")
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

        /// How the stage that was just opened runs its children. Call it right after
        /// beginStage(); calling it again replaces the earlier choice. They mirror
        /// Stage::setSequential(), setParallel() and setChoice().
        ///  - inSequence():  children run one after another (the default)
        ///  - inParallel():  all children open together and the stage waits for all of them
        ///  - choosingBy():  only the child picked by @p chooser runs; it receives the
        ///                   instance data and returns the child's index, or -1 for none
        virtual WorkflowBuilder& inSequence() = 0;
        virtual WorkflowBuilder& inParallel() = 0;
        virtual WorkflowBuilder& choosingBy(Stage::Selector chooser) = 0;

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