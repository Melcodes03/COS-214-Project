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

clean: 
	rm -f $(OBJS) $(TARGET) $(CM_TARGET) command_memento_test.o