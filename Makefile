SIM = sim/build/Simulator
TARGET = test/build/$(T).bin

all:
	@echo "-------Build Simulator-------"
	@$(MAKE) -C sim
	@echo "-------Build Test-------"
	@$(MAKE) -C test T=$(T)
	@echo "-------Start Simulation-------"
	@$(SIM) $(T) --batch

debug: 
	@echo "-------Build Simulator-------"
	@$(MAKE) -C sim
	@echo "-------Build Test-------"
	@$(MAKE) -C test T=$(T)
	@echo "-------Start Debugging-------"
	@$(SIM) $(T) --debug

itrace: 
	@echo "-------Build Simulator-------"
	@$(MAKE) -C sim
	@echo "-------Build Test-------"
	@$(MAKE) -C test T=$(T)
	@echo "-------Start Debugging-------"
	@$(SIM) $(T) --itrace

unit_test:
	@echo "------- Building and Running Simulator Unit Tests -------"
	@$(MAKE) -C sim unit_test

clean:
	@$(MAKE) -C sim clean
	@$(MAKE) -C test clean

.PHONY: clean all unit_test
