#include "Observer.h"

Observer::Observer(WorkItem *subject) : subject(subject)
{
    if (subject)
    {
        subject->attach(this);
    }
}
Observer::~Observer()
{
    if (subject)
    {
        subject->detach(this);
    }
}