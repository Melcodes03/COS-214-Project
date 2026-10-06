#ifndef PARTICIPANT_H
#define PARTICIPANT_H

#include <string>

//A person (or system) that work can be given to.
class Participant {
    private:
        std::string name;
        std::string role;
        int activeTasks;   //current workload

    public:
        Participant(const std::string& name, const std::string& role);

        std::string getName() const;
        std::string getRole() const;
        int getActiveTasks() const;
        void addTask();
        void finishTask();
};

#endif //PARTICIPANT_H
