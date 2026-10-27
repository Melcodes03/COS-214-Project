#include "ManagerHandler.h"

ManagerHandler::ManagerHandler(Participant* manager) : manager(manager) {}

Participant* ManagerHandler::handle(WorkItem& item, const std::vector<Participant*>& candidates) {
    if (manager != nullptr) {
        return manager;
    }
    return passOn(item, candidates);   //no manager set: let any later link try
}

bool ManagerHandler::covers(const Participant& worker) const {
    return &worker == manager;
}
