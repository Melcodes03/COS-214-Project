CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -pedantic

SRCS = WorkItem.cpp Task.cpp Stage.cpp DepthFirstIterator.cpp WorkflowDefinition.cpp WorkflowInstance.cpp \
       WorkItemState.cpp CreatedState.cpp AvailableState.cpp AssignedState.cpp InProgressState.cpp \
       CompletedState.cpp RejectedState.cpp EscalatedState.cpp CancelledState.cpp WorkItemDecorator.cpp \
	   PriorityDecorator.cpp ValidationDecorator.cpp AuditingDecorator.cpp \
	   WorkItemMemento.cpp TransitionCommand.cpp WorkerCommand.cpp AllocationCommand.cpp ApprovalCommand.cpp CommandHistory.cpp \
	   WorkItemVisitor.cpp ProgressReportVisitor.cpp EscalationReportVisitor.cpp \
	   Participant.cpp RoleBasedAssignment.cpp LeastWorkloadAssignment.cpp WorkAllocator.cpp \
	   AssignmentHandler.cpp LevelHandler.cpp ManagerHandler.cpp ReportScreen.cpp \
	   SingleApproval.cpp MultiLevelApproval.cpp ApprovalRequest.cpp \
	   DefinitionBuilder.cpp OutlineBuilder.cpp WorkflowDirector.cpp DemoData.cpp \
	   ProgressCommand.cpp main.cpp
OBJS = $(SRCS:.cpp=.o)
TARGET = tividy_test
CM_TARGET = cm_test
CM_OBJS = $(filter-out main.o,$(OBJS)) command_memento_test.o Observer.o
VS_TARGET = vs_test
VS_OBJS = $(filter-out main.o,$(OBJS)) visitor_strategy_test.o
CHAIN_TARGET = chain_test
CHAIN_OBJS = $(filter-out main.o,$(OBJS)) chain_report_test.o
BUILDER_TARGET = builder_test
BUILDER_OBJS = $(filter-out main.o,$(OBJS)) builder_prototype_test.o
COMPOSITE_TARGET = composite_test
COMPOSITE_OBJS = $(filter-out main.o,$(OBJS)) composite_iterator_test.o
ADAPTER_TARGET = adapter_test
ADAPTER_SRCS = adapter-test.cpp CommunicationAdapter.cpp EmailConnector.cpp MailService.cpp \
			   SmsConnector.cpp SmsGateway.cpp
OBSERVER_TARGET = observer_test
OBSERVER_SRCS = observer-test.cpp Observer.cpp generalObserver.cpp ParticipantObserver.cpp \
				CommunicationAdapter.cpp Task.cpp WorkItem.cpp DepthFirstIterator.cpp WorkItemState.cpp \
				Participant.cpp WorkerCommand.cpp TransitionCommand.cpp CommandHistory.cpp \
				CreatedState.cpp AvailableState.cpp AssignedState.cpp InProgressState.cpp CompletedState.cpp \
				RejectedState.cpp EscalatedState.cpp CancelledState.cpp WorkItemDecorator.cpp \
				PriorityDecorator.cpp ValidationDecorator.cpp AuditingDecorator.cpp WorkItemMemento.cpp

# TUI (Stephen): shared engine sources plus the text interface
TUI_ENGINE = $(filter-out main.o,$(OBJS)) Observer.o TUI.o IndustrySetup.o
TUI_TARGET = tividy_tui
TUI_TEST_TARGET = tui_test_runner

# Web GUI: REST server (web_main) over the TividyService facade, page in ../web
WEB_TARGET = tividy_web
# Observer + Adapter classes the web service uses for notifications
NOTIFY_OBJS = Observer.o ParticipantObserver.o CommunicationAdapter.o EmailConnector.o MailService.o SmsConnector.o SmsGateway.o
WEB_OBJS = $(filter-out main.o,$(OBJS)) $(NOTIFY_OBJS) TividyService.o web_main.o
WEBTEST_TARGET = web_test
WEBTEST_OBJS = $(filter-out main.o,$(OBJS)) $(NOTIFY_OBJS) TividyService.o web_service_test.o

# Windows (MSYS2/MinGW) needs the Windows socket library for the HTTP server
ifeq ($(OS),Windows_NT)
WEB_LIBS = -lws2_32
endif

.PHONY: all run clean test test_all test_cm test_vs test_chain test_builder test_composite test_workflow test_state test_adapter test_observer tui run_tui test_tui web run_web test_web

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

# doctest tests for the escalation chain, the allocator and the reports
$(CHAIN_TARGET): $(CHAIN_OBJS)
	$(CXX) $(CXXFLAGS) -o $(CHAIN_TARGET) $(CHAIN_OBJS)

test_chain: $(CHAIN_TARGET)
	./$(CHAIN_TARGET)

# doctest tests for the Builder, the generic Director, Prototype and both industries' data
$(BUILDER_TARGET): $(BUILDER_OBJS)
	$(CXX) $(CXXFLAGS) -o $(BUILDER_TARGET) $(BUILDER_OBJS)

test_builder: $(BUILDER_TARGET)
	./$(BUILDER_TARGET)

# doctest tests for the Composite and the Iterator
$(COMPOSITE_TARGET): $(COMPOSITE_OBJS)
	$(CXX) $(CXXFLAGS) -o $(COMPOSITE_TARGET) $(COMPOSITE_OBJS)

test_composite: $(COMPOSITE_TARGET)
	./$(COMPOSITE_TARGET)

# doctest tests for the engine: advance(), routing, the cancel rule, instance isolation
WORKFLOW_TARGET = workflow_test
WORKFLOW_OBJS = $(filter-out main.o,$(OBJS)) Observer.o test_workflow.o
$(WORKFLOW_TARGET): $(WORKFLOW_OBJS)
	$(CXX) $(CXXFLAGS) -o $(WORKFLOW_TARGET) $(WORKFLOW_OBJS)

test_workflow: $(WORKFLOW_TARGET)
	./$(WORKFLOW_TARGET)

# doctest tests for the work-item lifecycle (State) and the decorators
STATE_TARGET = state_test
STATE_OBJS = $(filter-out main.o,$(OBJS)) Observer.o test_state.o
$(STATE_TARGET): $(STATE_OBJS)
	$(CXX) $(CXXFLAGS) -o $(STATE_TARGET) $(STATE_OBJS)

test_state: $(STATE_TARGET)
	./$(STATE_TARGET)

# Runs every test program, one after another. Stops at the first failure.
test_all: run test_cm test_vs test_chain test_builder test_composite test_workflow test_state test_observer test_adapter test_tui test_web

$(ADAPTER_TARGET): $(ADAPTER_SRCS)
	$(CXX) $(CXXFLAGS) -o $@ $(ADAPTER_SRCS)

test_adapter: $(ADAPTER_TARGET)
	./$(ADAPTER_TARGET)

$(OBSERVER_TARGET): $(OBSERVER_SRCS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBSERVER_SRCS)

test_observer: $(OBSERVER_TARGET)
	./$(OBSERVER_TARGET)

# TUI: interactive, and its scripted test
$(TUI_TARGET): $(TUI_ENGINE) tui_main.o
	$(CXX) $(CXXFLAGS) -o $(TUI_TARGET) $(TUI_ENGINE) tui_main.o

tui: $(TUI_TARGET)

run_tui: $(TUI_TARGET)
	./$(TUI_TARGET)

$(TUI_TEST_TARGET): $(TUI_ENGINE) tui_test.o
	$(CXX) $(CXXFLAGS) -o $(TUI_TEST_TARGET) $(TUI_ENGINE) tui_test.o

test_tui: $(TUI_TEST_TARGET)
	./$(TUI_TEST_TARGET)

# Web GUI (needs pthreads): make run_web, then open http://localhost:8080
$(WEB_TARGET): $(WEB_OBJS)
	$(CXX) $(CXXFLAGS) -o $(WEB_TARGET) $(WEB_OBJS) -pthread $(WEB_LIBS)

web: $(WEB_TARGET)

run_web: $(WEB_TARGET)
	./$(WEB_TARGET) --web ../web

# doctest tests for the web service (the facade the REST API sits on)
$(WEBTEST_TARGET): $(WEBTEST_OBJS)
	$(CXX) $(CXXFLAGS) -o $(WEBTEST_TARGET) $(WEBTEST_OBJS) -pthread $(WEB_LIBS)

test_web: $(WEBTEST_TARGET)
	./$(WEBTEST_TARGET)

clean: 
	rm -f TUI.o IndustrySetup.o tui_main.o tui_test.o $(TUI_TARGET) $(TUI_TEST_TARGET) \
	      $(WEB_TARGET) $(WEBTEST_TARGET) web_main.o web_service_test.o TividyService.o $(NOTIFY_OBJS)
	rm -f $(OBJS) $(TARGET) $(CM_TARGET) $(ADAPTER_TARGET) $(OBSERVER_TARGET) command_memento_test.o Observer.o $(VS_TARGET) visitor_strategy_test.o test_observer $(CHAIN_TARGET) chain_report_test.o $(BUILDER_TARGET) builder_prototype_test.o $(COMPOSITE_TARGET) composite_iterator_test.o $(WORKFLOW_TARGET) test_workflow.o $(STATE_TARGET) test_state.o

# runs every automated test suite (used by the Docker image)
test: test_workflow test_chain test_cm test_vs
