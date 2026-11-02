#ifndef PROGRESSCOMMAND_H
#define PROGRESSCOMMAND_H

#include <string>
#include "Command.h"

class WorkflowInstance;
class WorkItemMemento;

/// Command: ConcreteCommand (decorating another Command).
///
/// Finishing a task makes the engine open the next work (WorkflowInstance::advance).
/// To a user that is part of the same action, so undoing the action must take it back
/// too; otherwise an earlier task would be open again while later work stayed open.
///
/// This command runs the wrapped command, then advances the instance. Undo reverses
/// the wrapped command (workloads, allocator records), then restores the whole tree
/// from a WorkItemMemento taken before the action, and recomputes the instance status.
/// A refused wrapped command leaves nothing behind, so it is never recorded.
class ProgressCommand : public Command {
    public:
        /// @param inner    the action to run; owned
        /// @param instance the instance to advance afterwards; not owned
        ProgressCommand(Command* inner, WorkflowInstance* instance);
        ~ProgressCommand() override;

        ProgressCommand(const ProgressCommand&) = delete;
        ProgressCommand& operator=(const ProgressCommand&) = delete;

        bool execute() override;
        void undo() override;
        std::string getName() const override;

    private:
        Command* inner;
        WorkflowInstance* instance;
        WorkItemMemento* before;   ///< whole-tree snapshot taken by execute(); owned
        bool done;
};

#endif //PROGRESSCOMMAND_H
