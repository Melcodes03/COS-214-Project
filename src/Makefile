CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -pedantic

SRCS = WorkItem.cpp Task.cpp Stage.cpp DepthFirstIterator.cpp WorkflowDefinition.cpp WorkflowInstance.cpp \
       WorkItemState.cpp CreatedState.cpp AvailableState.cpp AssignedState.cpp InProgressState.cpp \
       CompletedState.cpp RejectedState.cpp EscalatedState.cpp CancelledState.cpp WorkItemDecorator.cpp \
	   PriorityDecorator.cpp ValidationDecorator.cpp AuditingDecorator.cpp \
	   WorkItemMemento.cpp TransitionCommand.cpp CommandHistory.cpp main.cpp
OBJS = $(SRCS:.cpp=.o)
TARGET = tividy_test
CM_TARGET = cm_test
CM_OBJS = $(filter-out main.o,$(OBJS)) command_memento_test.o
ADAPTER_TARGET = adapter_test
ADAPTER_SRCS = adapter-test.cpp CommunicationAdapter.cpp EmailConnector.cpp MailService.cpp \
			   SmsConnector.cpp SmsGateway.cpp
OBSERVER_TARGET = observer_test
OBSERVER_SRCS = observer-test.cpp Observer.cpp notificationService.cpp ParticipantObserver.cpp \
				CommunicationAdapter.cpp Task.cpp WorkItem.cpp DepthFirstIterator.cpp WorkItemState.cpp \
				CreatedState.cpp AvailableState.cpp AssignedState.cpp InProgressState.cpp CompletedState.cpp \
				RejectedState.cpp EscalatedState.cpp CancelledState.cpp WorkItemDecorator.cpp \
				PriorityDecorator.cpp ValidationDecorator.cpp AuditingDecorator.cpp WorkItemMemento.cpp

all: $(TARGET)

$(TARGET) : $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

%.o: %.cpp	
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

$(CM_TARGET): $(CM_OBJS)
	$(CXX) $(CXXFLAGS) -o $(CM_TARGET) $(CM_OBJS)

test_cm: $(CM_TARGET)
	./$(CM_TARGET)

$(ADAPTER_TARGET): $(ADAPTER_SRCS)
	$(CXX) $(CXXFLAGS) -o $@ $(ADAPTER_SRCS)

test_adapter: $(ADAPTER_TARGET)
	./$(ADAPTER_TARGET)

test_observer: $(OBSERVER_SRCS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBSERVER_SRCS)

clean: 
	rm -f $(OBJS) $(TARGET) $(CM_TARGET) $(ADAPTER_TARGET) $(OBSERVER_TARGET) command_memento_test.o