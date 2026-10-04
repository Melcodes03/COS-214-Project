CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -pedantic

SRCS = WorkItem.cpp Task.cpp Stage.cpp DepthFirstIterator.cpp WorkflowDefinition.cpp WorkflowInstance.cpp \
       WorkItemState.cpp CreatedState.cpp AvailableState.cpp AssignedState.cpp InProgressState.cpp \
       CompletedState.cpp RejectedState.cpp EscalatedState.cpp CancelledState.cpp WorkItemDecorator.cpp \
	   PriorityDecorator.cpp ValidationDecorator.cpp AuditingDecorator.cpp \
	   WorkItemMemento.cpp TransitionCommand.cpp CommandHistory.cpp \
	   WorkItemVisitor.cpp ProgressReportVisitor.cpp EscalationReportVisitor.cpp \
	   Participant.cpp RoleBasedAssignment.cpp LeastWorkloadAssignment.cpp WorkAllocator.cpp \
	   SingleApproval.cpp MultiLevelApproval.cpp ApprovalRequest.cpp main.cpp
OBJS = $(SRCS:.cpp=.o)
TARGET = tividy_test
CM_TARGET = cm_test
CM_OBJS = $(filter-out main.o,$(OBJS)) command_memento_test.o
VS_TARGET = vs_test
VS_OBJS = $(filter-out main.o,$(OBJS)) visitor_strategy_test.o

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

$(VS_TARGET): $(VS_OBJS)
	$(CXX) $(CXXFLAGS) -o $(VS_TARGET) $(VS_OBJS)

test_vs: $(VS_TARGET)
	./$(VS_TARGET)

clean: 
	rm -f $(OBJS) $(TARGET) $(CM_TARGET) command_memento_test.o $(VS_TARGET) visitor_strategy_test.o