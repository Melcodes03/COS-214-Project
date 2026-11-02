#ifndef RUNPOLICY_H
#define RUNPOLICY_H

#include <set>
#include <string>
#include <vector>

/// One thing that could be done next on a case: an engine action on a task.
struct StepOption {
    std::string taskId;
    std::string action;     ///< allocate, start, complete, approve, reject, decline, escalate
    int retryCount;
};

/// @brief Strategy pattern: Strategy. Chooses what "Run all tickets" does next on a case.
/// It only ever picks one of the actions the engine says are allowed, so the runner can never
/// cheat (for example set a task to Completed directly).
class RunPolicy {
    public:
        virtual ~RunPolicy() {}
        /// @return the index in @p options to run, or -1 to do nothing this tick
        virtual int choose(const std::vector<StepOption>& options) = 0;
        virtual std::string name() const = 0;
};

/// @brief ConcreteStrategy: everybody says yes. Approve, complete, start, then assign.
class HappyPathPolicy : public RunPolicy {
    public:
        int choose(const std::vector<StepOption>& options) override;
        std::string name() const override;
};

/// @brief ConcreteStrategy: like HappyPath, but every other task is sent back once after it
/// was started, so rework, the escalation chain and the rework limit are exercised too.
class WithDeclinesPolicy : public RunPolicy {
    private:
        std::set<std::string> declined;   ///< tasks already declined once (case id + task id)
        HappyPathPolicy happy;
    public:
        int choose(const std::vector<StepOption>& options) override;
        std::string name() const override;
        /// The runner tells the policy which case the options belong to.
        void setCase(const std::string& caseId);
    private:
        std::string currentCase;
};

#endif //RUNPOLICY_H
