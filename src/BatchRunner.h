#ifndef BATCHRUNNER_H
#define BATCHRUNNER_H

#include <deque>
#include <map>
#include <string>
#include <vector>
#include "RunPolicy.h"

/// @brief Mediator pattern: the Colleague interface. What the runner may ask of the cases.
/// TividyService implements it, so every step is a real engine action (a Command in the
/// case's own history) and the cases never know about each other or about the runner.
class CaseDesk {
    public:
        virtual ~CaseDesk() {}
        virtual std::string openTicket() = 0;                                   ///< opens one new case; returns its id
        virtual std::vector<StepOption> options(const std::string& caseId) = 0;  ///< what the engine allows now
        virtual bool step(const std::string& caseId, const StepOption& option) = 0;
        virtual bool finished(const std::string& caseId) = 0;                   ///< Completed or Failed
        virtual int review(const std::string& caseId) = 0;                      ///< re-offers stalled work; how many
};

/// @brief Mediator pattern: ConcreteMediator. "Run all tickets".
///
/// Opens N tickets, keeps at most `slots` of them running (the desk's capacity) and the rest
/// in a waiting queue. Each tick, every running ticket takes its next allowed step, chosen by
/// the RunPolicy (Strategy). A finished ticket frees its slot and the next waiting ticket
/// starts. A ticket that cannot move for a few ticks is sent to review. Time is driven by
/// tick() calls (no thread), so a run is deterministic and testable.
class BatchRunner {
    public:
        BatchRunner(CaseDesk& desk, RunPolicy* policy, int count, int slots);   ///< owns policy
        ~BatchRunner();
        BatchRunner(const BatchRunner&) = delete;
        BatchRunner& operator=(const BatchRunner&) = delete;

        void tick();
        bool isDone() const;
        bool isPaused() const;
        void setPaused(bool paused);

        const std::vector<std::string>& getRunning() const;
        const std::deque<std::string>& getWaiting() const;
        const std::vector<std::string>& getFinished() const;
        const std::vector<std::string>& getAll() const;
        int getTicks() const;
        int getSteps() const;
        int getReviews() const;
        int getSlots() const;
        int longestWait() const;            ///< most ticks any ticket spent in the queue
        std::string policyName() const;

    private:
        CaseDesk& desk;
        RunPolicy* policy;
        int slots;
        std::vector<std::string> all;
        std::vector<std::string> running;
        std::deque<std::string> waiting;
        std::vector<std::string> done;
        std::map<std::string, int> idle;      ///< ticks a running ticket could not move
        std::map<std::string, int> queued;    ///< ticks a ticket waited before it started
        int ticks;
        int steps;
        int reviews;
        bool paused;

        void promote();
};

#endif //BATCHRUNNER_H
