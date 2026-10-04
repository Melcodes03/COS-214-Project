#include <iostream>
#include <string>
#include <vector>
#include "Task.h"
#include "AuditingDecorator.h"
#include "ValidationDecorator.h"
#include "PriorityDecorator.h"
#include "notificationService.h"
#include "ParticipantObserver.h"
#include "CommunicationAdapter.h"

//Test double: records what would have been sent instead of contacting anything
class RecordingChannel : public CommunicationAdapter {
    public:
        std::vector<std::string> sent;
        void send(std::string recipient, std::string message, std::string = "normal") override {
            sent.push_back(recipient + " | " + message);
        }
};

static bool alwaysValid(const WorkItem&) { return true; }

int main(){
    bool ok = true;
    auto check = [&](bool cond, const char* what){
        std::cout << (cond ? "  pass: " : "  FAIL: ") << what << std::endl;
        if (!cond) ok = false;
    };

    //items are declared first so they are destroyed last (observers must not outlive their subject)
    Task plain("T1", "Plain task");
    AuditingDecorator* decorated = new AuditingDecorator(
        new ValidationDecorator(new PriorityDecorator(new Task("T2", "Decorated task"), 5), alwaysValid));

    RecordingChannel channel;

    std::cout << "1. every successful transition notifies, refused ones do not" << std::endl;
    {
        notificationService notifier(&plain, &channel, "ana@example.com");
        check(plain.makeAvailable() && plain.assign() && plain.start() && plain.complete(), "lifecycle runs");
        check(channel.sent.size() == 4, "four transitions -> four messages");
        check(channel.sent[0] == "ana@example.com | Work item T1 (Plain task): Created -> Available", "message text is correct");
        check(!plain.cancel(), "cancel on a Completed item is refused");
        check(channel.sent.size() == 4, "refused transition sends nothing");
        check(notifier.getObservedState() == plain.getState(), "observer tracks the item's state");
    }

    std::cout << "2. destroying an observer detaches it" << std::endl;
    {
        Task t("T3", "Short-lived");
        {
            notificationService n(&t, &channel, "x");
        }   //n destroyed here, so it detaches itself
        size_t before = channel.sent.size();
        t.makeAvailable();
        check(channel.sent.size() == before, "no message after the observer is gone");
    }

    std::cout << "3. observers attached through a decorator still hear about changes" << std::endl;
    {
        size_t before = channel.sent.size();
        notificationService n(decorated, &channel, "bo@example.com");
        decorated->makeAvailable();
        decorated->assign();
        check(channel.sent.size() == before + 2, "two transitions on the decorated item -> two messages");
    }

    std::cout << "4. one subject, several observers, each with its own behaviour" << std::endl;
    {
        Task t("T4", "Shared");
        notificationService n(&t, &channel, "cy@example.com");
        ParticipantObserver all(&t, "Dee", "");
        ParticipantObserver escalationsOnly(&t, "Eli", "Escalated");
        size_t before = channel.sent.size();
        t.makeAvailable(); t.assign(); t.start(); t.escalate();
        check(channel.sent.size() == before + 4, "notification service heard all four");
        check(all.getInbox().size() == 4, "participant following everything got four");
        check(escalationsOnly.getInbox().size() == 1, "participant interested in Escalated got one");
        check(escalationsOnly.getInbox()[0] == "Eli was told: T4 InProgress -> Escalated", "inbox text is correct");
    }

    std::cout << "5. detach() stops further messages" << std::endl;
    {
        Task t("T5", "Detach me");
        ParticipantObserver p(&t, "Fay", "Available");
        t.makeAvailable();
        t.detach(&p);
        t.assign();
        check(p.getInbox().size() == 1, "only the change before detach() was received");
    }

    delete decorated;
    std::cout << (ok ? "All observer tests passed." : "Observer tests failed") << std::endl;
    return ok ? 0 : 1;
}
