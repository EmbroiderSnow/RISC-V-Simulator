## 1. General Compilation Targets
BUILD = build
$(shell mkdir -p $(BUILD)/objs)

ALL_SRCS = $(wildcard src/*.c)
MAIN_SRC = src/main.c
TEST_SRCS = src/inst_test.c src/single_test.c

SIMULATOR_SRCS = $(filter-out $(TEST_SRCS), $(ALL_SRCS))
TEST_RUNNER_SRCS = $(filter-out $(MAIN_SRC), $(ALL_SRCS))

SIMULATOR_OBJS = $(patsubst %.c, $(BUILD)/objs/%.o, $(notdir $(SIMULATOR_SRCS)))
TEST_RUNNER_OBJS = $(patsubst %.c, $(BUILD)/objs/%.o, $(notdir $(TEST_RUNNER_SRCS)))

SIMULATOR = $(BUILD)/Simulator
TEST_RUNNER = $(BUILD)/TestRunner

## 2. General Compilation Flags
CC  = gcc
CXX = g++
LD  = $(CC)
CFLAGS   = -I./include -Wall
CXXFLAGS = $(CFLAGS)

## 3. Rules
all: $(SIMULATOR)

$(BUILD)/objs/%.o: src/%.c
	@echo + CC "->" $<
	@$(CC) -std=gnu11 $(CFLAGS) -c -o $@ $<

$(BUILD)/objs/%.o: src/%.cpp
	@echo + CXX "->" $<
	@$(CXX) -std=c++17 $(CXXFLAGS) -c -o $@ $<

$(SIMULATOR): $(SIMULATOR_OBJS)
	@echo + LD "Simulator" "->" $^
	@$(LD) -o $@ $^

$(TEST_RUNNER): $(TEST_RUNNER_OBJS)
	@echo + LD "TestRunner" "->" $^
	@$(LD) -o $@ $^

unit_test: $(TEST_RUNNER)
	@echo + RUN "->" $<
	@$(TEST_RUNNER)

clean:
	rm -rf $(BUILD)

.PHONY: clean all unit_test
