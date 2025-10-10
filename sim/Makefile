## 1. General Compilation Targets
BUILD = build
$(shell mkdir -p $(BUILD)/objs)

ALL_SRCS = $(wildcard src/*.c) $(wildcard src/*.cpp)
MAIN_SRC = src/main.c
TEST_SRCS = src/inst_test.c src/single_test.c

SIMULATOR_SRCS = $(filter-out $(TEST_SRCS), $(ALL_SRCS))
TEST_RUNNER_SRCS = $(filter-out $(MAIN_SRC), $(ALL_SRCS))

SIMULATOR_OBJS = $(patsubst %.c, $(BUILD)/objs/%.o, $(notdir $(SIMULATOR_SRCS)))
TEST_RUNNER_OBJS = $(patsubst %.c, $(BUILD)/objs/%.o, $(notdir $(TEST_RUNNER_SRCS)))

SIMULATOR = $(BUILD)/Simulator
TEST_RUNNER = $(BUILD)/TestRunner

## 2. General Compilation Flags
LLVM_LDFLAGS  := $(shell llvm-config --ldflags --libs riscv mc support --system-libs) 

CC  = gcc
CXX = g++
LD  = $(CXX)
COMMON_FLAGS = -g -I./include -Wall $(shell llvm-config --cppflags)
CFLAGS   = $(COMMON_FLAGS) -std=gnu11
CXXFLAGS = $(COMMON_FLAGS) -std=c++17
LDFLAGS  = $(LLVM_LDFLAGS) -lelf

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
	@$(LD) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

$(TEST_RUNNER): $(TEST_RUNNER_OBJS)
	@echo + LD "TestRunner" "->" $^
	@$(LD) -o $@ $^

unit_test: $(TEST_RUNNER)
	@echo + RUN "->" $<
	@$(TEST_RUNNER)

clean:
	rm -rf $(BUILD)

.PHONY: clean all unit_test
