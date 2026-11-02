#include "AssignmentHandler.h"

AssignmentHandler::AssignmentHandler() : next(nullptr) {}

AssignmentHandler::~AssignmentHandler() { delete next; }

void AssignmentHandler::setNext(AssignmentHandler* nextHandler) {
    if (nextHandler == next) {
        return;
    }
    delete next;
    next = nextHandler;
}

AssignmentHandler* AssignmentHandler::getNext() const { return next; }

AssignmentHandler* AssignmentHandler::levelOf(const Participant& worker) {
    if (covers(worker)) {
        return this;
    }
    return next == nullptr ? nullptr : next->levelOf(worker);
}

Participant* AssignmentHandler::passOn(WorkItem& item, const std::vector<Participant*>& candidates) {
    return next == nullptr ? nullptr : next->handle(item, candidates);
}
