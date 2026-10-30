#ifndef PARTICIPANT_H
#define PARTICIPANT_H

#include <string>
#include <vector>

using namespace std;

class WorkItem;
class CommandHistory;

//A person (or system) that work can be given to.
class Participant {
    private:
        std::string name;
        std::string role;
        int activeTasks;   //current workload
        int capacity;
        bool available;
        vector<WorkItem*> worklist;

    public:
        Participant(const std::string& name, const std::string& role, int capacity);

        std::string getName() const;
        std::string getRole() const;
        int getActiveTasks() const;
        void addTask();
        void finishTask();
        int getCapacity();

        bool isAvailable();

        void setAvailable(bool);

        bool canTakeWork();

        void receive(WorkItem*);

        void remove(WorkItem*);

        vector<WorkItem*>& getWorklist();

        /// Worker actions. Each runs as a Command through @p history, so it is
        /// recorded and can be undone. All return false (and change nothing) if
        /// the item is null or its current State / Decorators refuse the action.

        /// Assigned -> InProgress.
        bool start(WorkItem* item, CommandHistory& history);

        /// InProgress -> Completed. Frees one of this worker's tasks.
        bool complete(WorkItem* item, CommandHistory& history);

        /// Hand the item back: reject() on the item. From Assigned it returns to
        /// Available; from InProgress it becomes Rejected. Frees one of this worker's tasks.
        bool decline(WorkItem* item, CommandHistory& history);
};

#endif //PARTICIPANT_H
