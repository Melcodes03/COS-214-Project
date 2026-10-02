CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -pedantic

SRCS = WorkItem.cpp Task.cpp Stage.cpp DepthFirstIterator.cpp WorkflowDefinition.cpp WorkflowInstance.cpp main.cpp
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