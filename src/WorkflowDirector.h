#ifndef WORKFLOWDIRECTOR_H
#define WORKFLOWDIRECTOR_H

#include <string>

class WorkflowBuilder;

/// Builder: abstract Director.
///
/// Each industry has one Director that holds its "recipe": which stages and
/// tasks its process has, in which order, and how they are decorated. The
/// Director only talks to the abstract WorkflowBuilder, so the same recipe can
/// produce a WorkflowDefinition (DefinitionBuilder) or a text outline
/// (OutlineBuilder). Industry wording lives here, never in engine classes.
class WorkflowDirector {
    public:
        virtual ~WorkflowDirector() {}

        /// Sends this industry's steps to @p builder.
        virtual void construct(WorkflowBuilder& builder) = 0;

        /// Name shown when the user picks an industry, e.g. "Software customer support".
        virtual std::string getIndustryName() const = 0;
};

#endif //WORKFLOWDIRECTOR_H
