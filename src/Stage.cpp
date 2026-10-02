#include "Stage.h"

Stage::Stage(const std::string& id, const std::string& name) : WorkItem(id, name){}

Stage::~Stage(){
    for (int i=0; i < (int)children.size(); i++){
        delete children[i];
    }
}

bool Stage::add(WorkItem* child){
    if (child == nullptr){
        return false;
    }
    children.push_back(child);
    return true;
}

WorkItem* Stage::getChild(int index) const {
    if (index < 0 || index >= getChildCount()){
        return nullptr;
    }
    return children[index];
}

int Stage::getChildCount() const {
    return (int)children.size();
}

WorkItem* Stage::clone() const{
    Stage* copy = new Stage(id, name);
    for (int i =0; i < (int)children.size(); i++){
        copy->add(children[i]->clone());
    }
    return copy;
}

