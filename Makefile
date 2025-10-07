SIM = sim/build/Simulator
TARGET = test/build/$(T).bin

all:
	@echo "-------Build Simulator-------"
	@$(MAKE) -C sim
	@echo "-------Build Test-------"
	@$(MAKE) -C test T=$(T)
	@echo "-------Start Simulation-------"
	@$(SIM) $(TARGET) -b

debug: 
	@echo "-------Build Simulator-------"
	@$(MAKE) -C sim
	@echo "-------Build Test-------"
	@$(MAKE) -C test T=$(T)
	@echo "-------Start Debugging-------"
	@$(SIM) $(TARGET) -d

unit_test:
	@echo "------- Building and Running Simulator Unit Tests -------"
	@$(MAKE) -C sim unit_test

clean:
	@$(MAKE) -C sim clean
	@$(MAKE) -C test clean

.PHONY: clean all unit_test
