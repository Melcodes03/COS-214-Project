#ifndef TRANSITIONCOMMAND_H
#define TRANSITIONCOMMAND_H

#include "Command.h"
#include "WorkItem.h"

class WorkItemMemento;

//Command: ConcreteCommand. Runs one lifecycle operation (assign, start, complete...)
//on a WorkItem (the Receiver). Before it runs, it takes a Memento of the item's
//subtree, so undo() puts back exactly what was there, including the retry counter,
//rather than trying to reverse the transition by hand (which State does not allow).
//
//The operation is passed as a pointer to a WorkItem member function, e.g.
//  new TransitionCommand(item, &WorkItem::complete, "complete")
//so a decorated item still runs its decorators (validation, auditing).
/** @brief Command that performs and can undo a work-item transition. */
class TransitionCommand : public Command {
    public:
        typedef bool (WorkItem::*Transition)();

        //target is not owned
        /**
         * @brief Constructs a transition command.
         * @param target Work item receiving the transition; the caller retains ownership.
         * @param transition Member function representing the transition to execute.
         * @param name Display name of the command.
         */
        TransitionCommand(WorkItem* target, Transition transition, const std::string& name);
        ~TransitionCommand() override;

        TransitionCommand(const TransitionCommand&) = delete;
        TransitionCommand& operator=(const TransitionCommand&) = delete;

        bool execute() override;
        void undo() override;
        std::string getName() const override;

    private:
        WorkItem* target;
        Transition transition;
        std::string name;
        WorkItemMemento* before;   //snapshot taken by execute(), owned
};

#endif //TRANSITIONCOMMAND_H
