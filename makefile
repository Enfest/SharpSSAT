RM := rm -rf
SUBDIRS := \
src \

# Solver selection - default to Kissat, can be overridden
SOLVER ?= kissat

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS := \
./src/strategy.cpp \
./src/basic_types.cpp \
./src/component_management.cpp \
./src/component_types.cpp \
./src/instance.cpp \
./src/main.cpp \
./src/solver.cpp

# Solver-specific source files
ifeq ($(SOLVER),kissat)
CPP_SRCS += ./src/kissat_integration.cpp
else ifeq ($(SOLVER),cadical)
CPP_SRCS += ./src/cadical_integration.cpp
endif

OBJS := \
strategy.o \
basic_types.o \
component_management.o \
component_types.o \
instance.o \
main.o \
solver.o

# Solver-specific object files
ifeq ($(SOLVER),kissat)
OBJS += kissat_integration.o
else ifeq ($(SOLVER),cadical)
OBJS += cadical_integration.o
endif

CPP_DEPS := \
strategy.d \
basic_types.d \
component_management.d \
component_types.d \
instance.d \
main.d \
solver.d

# Solver-specific dependency files
ifeq ($(SOLVER),kissat)
CPP_DEPS += kissat_integration.d
else ifeq ($(SOLVER),cadical)
CPP_DEPS += cadical_integration.d
endif

%.o: ./src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
ifeq ($(SOLVER),cadical)
	g++ -O3 -std=c++11 -DCADICAL_SOLVER -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
else
	g++ -O3 -std=c++11 -DKISSAT_SOLVER -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
endif
	@echo 'Finished building: $<'
	@echo ' '

# Kissat library path and flags
KISSAT_DIR := ../kissat
KISSAT_LIB := $(KISSAT_DIR)/build/libkissat.a
KISSAT_INCLUDE := -I$(KISSAT_DIR)/src

# CaDiCaL library path and flags
CADICAL_DIR := ../cadical
CADICAL_LIB := $(CADICAL_DIR)/build/libcadical.a
CADICAL_INCLUDE := -I$(CADICAL_DIR)/src

LIBS := -lgmpxx -lgmp

# Solver-specific libraries and includes
ifeq ($(SOLVER),kissat)
SOLVER_LIB := $(KISSAT_LIB)
SOLVER_INCLUDE := $(KISSAT_INCLUDE)
TARGET_NAME := SharpSSAT-Kissat
else ifeq ($(SOLVER),cadical)
SOLVER_LIB := $(CADICAL_LIB)
SOLVER_INCLUDE := $(CADICAL_INCLUDE)
TARGET_NAME := SharpSSAT-CaDiCaL
else
$(error Invalid solver: $(SOLVER). Use SOLVER=kissat or SOLVER=cadical)
endif

# All Target
all: $(TARGET_NAME)

# Tool invocations
$(TARGET_NAME): $(OBJS) $(SOLVER_LIB)
	@echo 'Building target: $@ with $(SOLVER)'
	@echo 'Invoking: GCC C++ Linker'
	g++ -L/usr/lib/ -o "$(TARGET_NAME)" $(OBJS) $(SOLVER_LIB) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

# Build Kissat library if needed
$(KISSAT_LIB):
	@echo 'Building Kissat library...'
	cd $(KISSAT_DIR) && ./configure && make
	@echo 'Kissat library built successfully'

# Build CaDiCaL library if needed
$(CADICAL_LIB):
	@echo 'Building CaDiCaL library...'
	cd $(CADICAL_DIR) && ./configure && make
	@echo 'CaDiCaL library built successfully'

# Specific targets for each solver
kissat: SOLVER=kissat
kissat: SharpSSAT-Kissat

cadical: SOLVER=cadical
cadical: SharpSSAT-CaDiCaL

# Other Targets
clean:
	-$(RM) $(OBJS) $(CPP_DEPS) SharpSSAT SharpSSAT-Kissat SharpSSAT-CaDiCaL
	-@echo ' '

clean-kissat:
	cd $(KISSAT_DIR) && make clean
	-@echo ' '

clean-cadical:
	cd $(CADICAL_DIR) && make clean
	-@echo ' '

clean-all: clean clean-kissat clean-cadical

.PHONY: all kissat cadical clean clean-kissat clean-cadical clean-all dependents

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(CPP_DEPS)),)
-include $(CPP_DEPS)
endif
endif