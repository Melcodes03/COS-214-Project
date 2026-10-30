#ifndef COMMAND_H
#define COMMAND_H

#include <string>

//Command: abstract Command. One user/system action on the workflow that can be undone.
class Command {
    public:
        virtual ~Command() {}

        //returns false if the action was refused; a refused command changes nothing
        virtual bool execute() = 0;
        virtual void undo() = 0;
        virtual std::string getName() const = 0;
};

#endif //COMMAND_H
