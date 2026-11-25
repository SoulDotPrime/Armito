CC      ?= gcc
CFLAGS  ?= -std=gnu11 -Wall -Wextra -O2
CPPFLAGS ?= -ICore -IIR

CORE_SRC := \
    Core/Core.c \
    Core/Config/CoreConfig.c \
    Core/Config/CoreConfigValidation.c \
    Core/Registry/CoreRegistry.c \
    Core/Dependency/CoreDependency.c \
    Core/Init/CoreInit.c \
    Core/Diagnostics/CoreDiagnostics.c \
    Core/Dispatch/CoreDispatch.c \
    Core/Monitor/CoreMonitor.c \
    Core/Topology/CoreTopology.c \
    Core/Scheduler/CoreScheduler.c \
    Core/IPC/CoreIPC.c \
    Core/Shutdown/CoreShutdown.c

IR_SRC := \
    IR/IR.c \
    IR/IRTypeset.c \
    IR/IRValue.c \
    IR/IROperation.c \
    IR/IRBlock.c \
    IR/IRCPUState.c

CORE_OBJ := $(patsubst %.c,Build/%.o,$(CORE_SRC))
IR_OBJ   := $(patsubst %.c,Build/%.o,$(IR_SRC))

CORE_TEST := Build/coretest
IR_TEST   := Build/irtest

.PHONY: all test core-test ir-test clean

all: $(CORE_TEST) $(IR_TEST)

$(CORE_TEST): $(CORE_OBJ) Tests/CoreTest.c
	$(CC) $(CFLAGS) $(CPPFLAGS) Tests/CoreTest.c $(CORE_OBJ) -o $@

$(IR_TEST): $(IR_OBJ) Tests/IRTest.c
	$(CC) $(CFLAGS) $(CPPFLAGS) Tests/IRTest.c $(IR_OBJ) -o $@

Build/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

core-test: $(CORE_TEST)
	./$(CORE_TEST)

ir-test: $(IR_TEST)
	./$(IR_TEST)

test: $(CORE_TEST) $(IR_TEST)
	./$(CORE_TEST)
	./$(IR_TEST)

clean:
	rm -rf Build
