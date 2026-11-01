#include "RunPolicy.h"

int HappyPathPolicy::choose(const std::vector<StepOption>& options) {
    static const char* order[] = {"approve", "complete", "start", "allocate"};
    for (int k = 0; k < 4; k++) {
        for (int i = 0; i < (int)options.size(); i++) {
            if (options[i].action == order[k]) return i;
        }
    }
    return -1;
}

std::string HappyPathPolicy::name() const { return "happy"; }

int WithDeclinesPolicy::choose(const std::vector<StepOption>& options) {
    for (int i = 0; i < (int)options.size(); i++) {
        const StepOption& o = options[i];
        char last = o.taskId.empty() ? '0' : o.taskId[o.taskId.size() - 1];
        bool odd = (last - '0') % 2 == 1;   //T1, T3 ... are declined once
        if (o.action != "decline" || o.retryCount != 0 || !odd) continue;
        bool started = false;   //declining work in progress sends it back for rework (it counts)
        for (int k = 0; k < (int)options.size(); k++) {
            started = started || (options[k].taskId == o.taskId && options[k].action == "complete");
        }
        if (started && declined.insert(currentCase + "/" + o.taskId).second) return i;
    }
    return happy.choose(options);
}

std::string WithDeclinesPolicy::name() const { return "declines"; }

void WithDeclinesPolicy::setCase(const std::string& caseId) { currentCase = caseId; }
