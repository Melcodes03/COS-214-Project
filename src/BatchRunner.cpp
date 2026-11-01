#include "BatchRunner.h"
#include <algorithm>

BatchRunner::BatchRunner(CaseDesk& desk, RunPolicy* policy, int count, int slots)
    : desk(desk), policy(policy), slots(slots < 1 ? 1 : slots), ticks(0), steps(0), reviews(0), paused(false) {
    for (int i = 0; i < count; i++) {
        std::string id = desk.openTicket();
        all.push_back(id);
        waiting.push_back(id);
        queued[id] = 0;
    }
    promote();
}

BatchRunner::~BatchRunner() { delete policy; }

void BatchRunner::promote() {
    while ((int)running.size() < slots && !waiting.empty()) {
        running.push_back(waiting.front());
        waiting.pop_front();
    }
}

void BatchRunner::tick() {
    if (isDone()) return;
    ++ticks;
    for (std::deque<std::string>::iterator it = waiting.begin(); it != waiting.end(); ++it) ++queued[*it];
    std::vector<std::string> still;
    for (size_t i = 0; i < running.size(); i++) {
        const std::string& id = running[i];
        if (!desk.finished(id)) {
            WithDeclinesPolicy* declines = dynamic_cast<WithDeclinesPolicy*>(policy);
            if (declines != nullptr) declines->setCase(id);
            std::vector<StepOption> opts = desk.options(id);
            int pick = policy->choose(opts);
            if (pick >= 0 && desk.step(id, opts[pick])) {
                ++steps;
                idle[id] = 0;
            } else if (++idle[id] >= 3) {   //stuck for 3 ticks: the Reviewer re-offers its work
                reviews += desk.review(id);
                idle[id] = 0;
            }
        }
        if (desk.finished(id)) done.push_back(id);
        else still.push_back(id);
    }
    running = still;
    promote();
}

bool BatchRunner::isDone() const { return running.empty() && waiting.empty(); }
bool BatchRunner::isPaused() const { return paused; }
void BatchRunner::setPaused(bool p) { paused = p; }
const std::vector<std::string>& BatchRunner::getRunning() const { return running; }
const std::deque<std::string>& BatchRunner::getWaiting() const { return waiting; }
const std::vector<std::string>& BatchRunner::getFinished() const { return done; }
const std::vector<std::string>& BatchRunner::getAll() const { return all; }
int BatchRunner::getTicks() const { return ticks; }
int BatchRunner::getSteps() const { return steps; }
int BatchRunner::getReviews() const { return reviews; }
int BatchRunner::getSlots() const { return slots; }
std::string BatchRunner::policyName() const { return policy->name(); }

int BatchRunner::longestWait() const {
    int most = 0;
    for (std::map<std::string, int>::const_iterator it = queued.begin(); it != queued.end(); ++it) most = std::max(most, it->second);
    return most;
}
