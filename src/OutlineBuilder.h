#ifndef OUTLINEBUILDER_H
#define OUTLINEBUILDER_H

#include "WorkflowBuilder.h"
#include <string>

/// Builder: ConcreteBuilder. Builds a readable text outline of a process.
///
/// It receives exactly the same steps as DefinitionBuilder but produces a
/// string instead of a WorkflowDefinition. The TUI uses it to show a process
/// before an instance is started. It also proves the point of the Builder
/// pattern: the same Director, a different product.
class OutlineBuilder : public WorkflowBuilder {
    private:
        std::string text;
        int depth;          //how many stages are open (controls indentation)
        bool lastWasTask;   //decorations only apply right after a task line
        bool lastWasStage;  //a stage mode only applies right after a stage line
        std::string stageMark;   //the " (mode)" label currently on the last stage line

        void decorate(const std::string& label);
        void markStage(const std::string& label);

        std::string indent() const;

    public:
        explicit OutlineBuilder(const std::string& processName);

        WorkflowBuilder& beginStage(const std::string& id, const std::string& name) override;
        WorkflowBuilder& inSequence() override;
        WorkflowBuilder& inParallel() override;
        WorkflowBuilder& choosingBy(Stage::Selector chooser) override;
        WorkflowBuilder& addTask(const std::string& id, const std::string& name, const std::string& role) override;
        WorkflowBuilder& withPriority(int priority) override;
        WorkflowBuilder& withValidation(ValidationDecorator::Rule rule) override;
        WorkflowBuilder& withAudit() override;
        WorkflowBuilder& endStage() override;

        std::string getResult() const;
};

#endif //OUTLINEBUILDER_H