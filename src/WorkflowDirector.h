#ifndef WORKFLOWDIRECTOR_H
#define WORKFLOWDIRECTOR_H

#include "WorkflowSpec.h"

class WorkflowBuilder;

/// Builder: Director. ONE generic Director for every industry.
///
/// It reads a WorkflowSpec (plain data) and sends the matching steps to any
/// WorkflowBuilder. The same spec can therefore produce a WorkflowDefinition
/// (DefinitionBuilder) or a text outline (OutlineBuilder). Industry wording lives
/// in the data, never in this class, so a new industry needs a new WorkflowSpec
/// and no new Director.
///
/// Ids are generated from position: stage i is "S<i>", its j-th task "S<i>-T<j>".
/// A stage's mode is applied through a table, so there is no switch on the mode.
/// In a CHOICE stage the Director builds the Stage::Selector from the tasks'
/// whenKey/whenOp/whenValue: the first task whose condition matches wins, an empty
/// whenKey is the default branch, and if nothing matches the selector returns -1
/// (the stage then cancels the workflow).
class WorkflowDirector {
    private:
        typedef void (WorkflowDirector::*ModeStep)(WorkflowBuilder&, const StageSpec&) const;

        void applySequential(WorkflowBuilder& builder, const StageSpec& stage) const;
        void applyParallel(WorkflowBuilder& builder, const StageSpec& stage) const;
        void applyChoice(WorkflowBuilder& builder, const StageSpec& stage) const;

    public:
        /// Sends every stage and task of @p spec to @p builder.
        void construct(WorkflowBuilder& builder, const WorkflowSpec& spec) const;
};

#endif //WORKFLOWDIRECTOR_H