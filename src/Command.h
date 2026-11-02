#ifndef COMMAND_H
#define COMMAND_H

#include <string>

//Command: abstract Command. One user/system action on the workflow that can be undone.
/** @brief Abstract action that can be executed and undone. */
class Command {
    public:
        virtual ~Command() {}

        //returns false if the action was refused; a refused command changes nothing
        /** @brief Executes the command. @return True when the action succeeds. */
        virtual bool execute() = 0;
        /** @brief Reverts the command when it has been executed. */
        virtual void undo() = 0;
        /** @brief Gets the command name. @return The name used to identify the command. */
        virtual std::string getName() const = 0;
};

#endif //COMMAND_H
