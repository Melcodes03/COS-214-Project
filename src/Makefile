CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -pedantic

SRCS = WorkItem.cpp Task.cpp Stage.cpp DepthFirstIterator.cpp WorkflowDefinition.cpp WorkflowInstance.cpp \
       WorkItemState.cpp CreatedState.cpp AvailableState.cpp AssignedState.cpp InProgressState.cpp \
       CompletedState.cpp RejectedState.cpp EscalatedState.cpp CancelledState.cpp WorkItemDecorator.cpp \
	   PriorityDecorator.cpp ValidationDecorator.cpp AuditingDecorator.cpp main.cpp
OBJS = $(SRCS:.cpp=.o)
TARGET = tividy_test

all: $(TARGET)

$(TARGET) : $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

%.o: %.cpp	
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean: 
	rm -f $(OBJS) $(TARGET)