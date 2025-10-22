T ?= dummy
MODEL ?= iss
ARGS ?=

SIM = sim/build/Simulator
TARGET = test/build/$(T).bin

.PHONY: all build build-sim build-test

all: build
build: build-sim build-test

build-sim:
	@echo "-------Build Simulator-------"
	@$(MAKE) -C sim

build-test:
	@echo "-------Build Test-------"
	@$(MAKE) -C test T=$(T)

.PHONY: run iss mc debug itrace ftrace

# Usage:
# 	make run T=dummy MODEL=mc
#	make run T=dummy MODEL=iss ARGS="--itrace"
run: build
	@echo "-------Start Simulation-------"
	@echo "  MODEL: $(MODEL)"
	@echo "  IMAGE: $(TARGET)"
	@echo "  ARGS:  $(ARGS)"
	@echo "------------------------------"
	@$(SIM) $(MODEL) $(T) $(ARGS)

# run iss
# Usage: make iss T=dummy
iss: 
	@$(MAKE) run MODEL=iss T=$(T) ARGS="--batch"

mc:
	@$(MAKE) run MODEL=mc T=$(T) ARGS=""

debug: 
	@$(MAKE) run MODEL=iss T=$(T) ARGS="--debug"

itrace: 
	@$(MAKE) run MODEL=iss T=$(T) ARGS="--itrace"

ftrace: 
	@$(MAKE) run MODEL=iss T=$(T) ARGS="--ftrace"

.PHONY: clean 

clean:
	@$(MAKE) -C sim clean
	@$(MAKE) -C test clean
