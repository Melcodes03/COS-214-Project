#ifndef COMMANDHISTORY_H
#define COMMANDHISTORY_H

#include <string>
#include <vector>
#include "Command.h"

//Command: Invoker. Runs commands and remembers the ones that worked, so they can
//be undone, redone, or rolled back to a marked point.
class CommandHistory {
    public:
        CommandHistory();
        ~CommandHistory();

        CommandHistory(const CommandHistory&) = delete;
        CommandHistory& operator=(const CommandHistory&) = delete;

        //Takes ownership of cmd. Returns false (and records nothing) if it was refused.
        bool execute(Command* cmd);

        bool undo();   //false if there is nothing to undo
        bool redo();   //false if there is nothing to redo, or the redo was refused

        //Rollback: mark() remembers "now"; rollbackTo() undoes everything done since.
        int mark() const;
        int rollbackTo(int marker);   //returns how many commands were undone

        int undoCount() const;
        int redoCount() const;
        std::string lastCommandName() const;   //"" if nothing to undo

    private:
        std::vector<Command*> done;
        std::vector<Command*> undone;

        void clearRedo();
};

#endif //COMMANDHISTORY_H
