#ifndef WORKERCOMMAND_H
#define WORKERCOMMAND_H

#include "Command.h"
#include "TransitionCommand.h"

class Participant;

/// Command: ConcreteCommand. A worker action (start, complete, decline) on a WorkItem.
///
/// It wraps a TransitionCommand, which already knows how to snapshot and restore
/// the work item, and adds the one thing a worker action also changes: the
/// participant's workload. Without this, undoing "complete" would put the item
/// back in progress but leave the worker's task count wrong.
class WorkerCommand : public Command {
    public:
        /// @param worker        the participant doing the action (not owned)
        /// @param target        the work item acted on (not owned)
        /// @param transition    the WorkItem operation to run, e.g. &WorkItem::complete
        /// @param name          label shown in the history listing
        /// @param releasesWork  true if a successful action frees one of the worker's tasks
        WorkerCommand(Participant* worker, WorkItem* target,
                      TransitionCommand::Transition transition,
                      const std::string& name, bool releasesWork);
        ~WorkerCommand() override;

        WorkerCommand(const WorkerCommand&) = delete;
        WorkerCommand& operator=(const WorkerCommand&) = delete;

        /// @return false (and changes nothing) if the item's State or a Decorator refused it
        bool execute() override;
        void undo() override;
        std::string getName() const override;

    private:
        Participant* worker;
        TransitionCommand* inner;   //owned
        bool releasesWork;
        bool released;              //did execute() actually lower the workload?
};

#endif //WORKERCOMMAND_H
