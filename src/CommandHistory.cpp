#include "CommandHistory.h"

CommandHistory::CommandHistory(){}

CommandHistory::~CommandHistory(){
    for (int i = 0; i < (int)done.size(); i++){
        delete done[i];
    }
    clearRedo();
}

void CommandHistory::clearRedo(){
    for (int i = 0; i < (int)undone.size(); i++){
        delete undone[i];
    }
    undone.clear();
}

bool CommandHistory::execute(Command* cmd){
    if (cmd == nullptr){
        return false;
    }
    if (!cmd->execute()){
        delete cmd;
        return false;
    }
    done.push_back(cmd);
    clearRedo();   //a new action makes the old redo branch meaningless
    return true;
}

bool CommandHistory::undo(){
    if (done.empty()){
        return false;
    }
    Command* cmd = done.back();
    done.pop_back();
    cmd->undo();
    undone.push_back(cmd);
    return true;
}

bool CommandHistory::redo(){
    if (undone.empty()){
        return false;
    }
    Command* cmd = undone.back();
    undone.pop_back();
    if (!cmd->execute()){
        delete cmd;
        return false;
    }
    done.push_back(cmd);
    return true;
}

int CommandHistory::mark() const {
    return (int)done.size();
}

int CommandHistory::rollbackTo(int marker){
    int count = 0;
    while ((int)done.size() > marker && undo()){
        count++;
    }
    return count;
}

int CommandHistory::undoCount() const { return (int)done.size(); }
int CommandHistory::redoCount() const { return (int)undone.size(); }

std::string CommandHistory::lastCommandName() const {
    return done.empty() ? std::string("") : done.back()->getName();
}

std::vector<std::string> CommandHistory::getHistory() const {
    std::vector<std::string> names;
    for (int i = 0; i < (int)done.size(); i++){
        names.push_back(done[i]->getName());
    }
    return names;
}

std::vector<std::string> CommandHistory::getRedoList() const {
    std::vector<std::string> names;
    for (int i = (int)undone.size() - 1; i >= 0; i--){   //top of the redo stack is redone first
        names.push_back(undone[i]->getName());
    }
    return names;
}
