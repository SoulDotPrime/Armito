CC      ?= gcc
CFLAGS  ?= -std=gnu11 -Wall -Wextra -O2
CPPFLAGS ?= -ICore

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

CORE_OBJ := $(patsubst %.c,Build/%.o,$(CORE_SRC))

TEST_SRC := Tests/CoreTest.c
TEST_BIN := Build/coretest

.PHONY: all test clean

all: $(TEST_BIN)

$(TEST_BIN): $(CORE_OBJ) $(TEST_SRC)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(TEST_SRC) $(CORE_OBJ) -o $@

Build/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

test: $(TEST_BIN)
	./$(TEST_BIN)

clean:
	rm -rf Build
