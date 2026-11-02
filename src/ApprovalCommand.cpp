#include "ApprovalCommand.h"
#include "ApprovalRequest.h"
#include "WorkItem.h"
#include "WorkItemMemento.h"

ApprovalCommand::ApprovalCommand(ApprovalRequest* request, WorkItem* item, bool approve, const std::string& name)
    : request(request), item(item), approve(approve), name(name), before(nullptr), voted(false){}

ApprovalCommand::~ApprovalCommand(){
    delete before;
}

bool ApprovalCommand::execute(){
    voted = false;
    if (request == nullptr || item == nullptr){
        return false;
    }
    if (request->getDecision() != ApprovalStrategy::PENDING){
        return false;   //already decided: the vote would be ignored, so nothing to record
    }
    delete before;
    before = item->createMemento();   //the vote may complete or reject the item
    request->vote(approve);
    voted = true;
    return true;
}

void ApprovalCommand::undo(){
    if (!voted){
        return;
    }
    item->restore(*before);       //notifies the item's observers if the state changes back
    request->undoLastVote();
    voted = false;
}

std::string ApprovalCommand::getName() const {
    return name;
}
