#include "Participant.h"

Participant::Participant(const std::string& name, const std::string& role)
    : name(name), role(role), activeTasks(0) {}

std::string Participant::getName() const { return name; }
std::string Participant::getRole() const { return role; }
int Participant::getActiveTasks() const { return activeTasks; }
void Participant::addTask() { activeTasks++; }
void Participant::finishTask() { if (activeTasks > 0) activeTasks--; }
