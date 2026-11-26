CC      ?= gcc
CFLAGS  ?= -std=gnu11 -Wall -Wextra -O2
CPPFLAGS ?= -ICore -IIR -IDecoder -I.

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
    Core/Memory/CoreMemory.c \
    Core/Shutdown/CoreShutdown.c

IR_SRC := \
    IR/IR.c \
    IR/IRTypeset.c \
    IR/IRValue.c \
    IR/IROperation.c \
    IR/IRBlock.c \
    IR/IRCPUState.c

DECODER_SRC := \
    Decoder/Decoder.c \
    Decoder/DecoderSource.c \
    Decoder/DecoderRegister.c \
    Decoder/DecoderStats.c \
    Decoder/AARCH64/AARCH64.c

AARCH64_CPU_STATE_SRC := CPU/AARCH64/AARCH64CPUState.c

CORE_OBJ := $(patsubst %.c,Build/%.o,$(CORE_SRC))
IR_OBJ   := $(patsubst %.c,Build/%.o,$(IR_SRC))
DECODER_OBJ := $(patsubst %.c,Build/%.o,$(DECODER_SRC))
AARCH64_CPU_STATE_OBJ := $(patsubst %.c,Build/%.o,$(AARCH64_CPU_STATE_SRC))

CORE_TEST := Build/coretest
IR_TEST   := Build/irtest
DECODER_TEST := Build/decodertest
AARCH64_CPU_STATE_TEST := Build/aarch64cpustatetest

.PHONY: all test core-test ir-test decoder-test cpu-state-test clean

all: $(CORE_TEST) $(IR_TEST) $(DECODER_TEST) $(AARCH64_CPU_STATE_TEST)

$(CORE_TEST): $(CORE_OBJ) Tests/CoreTest.c
	$(CC) $(CFLAGS) $(CPPFLAGS) Tests/CoreTest.c $(CORE_OBJ) -o $@

$(IR_TEST): $(IR_OBJ) Tests/IRTest.c
	$(CC) $(CFLAGS) $(CPPFLAGS) Tests/IRTest.c $(IR_OBJ) -o $@

$(DECODER_TEST): $(DECODER_OBJ) $(IR_OBJ) Tests/DecoderTest.c
	$(CC) $(CFLAGS) $(CPPFLAGS) Tests/DecoderTest.c $(DECODER_OBJ) $(IR_OBJ) -o $@

$(AARCH64_CPU_STATE_TEST): $(AARCH64_CPU_STATE_OBJ) CPU/AARCH64/CPUStateHostTest.c
	$(CC) $(CFLAGS) $(CPPFLAGS) CPU/AARCH64/CPUStateHostTest.c $(AARCH64_CPU_STATE_OBJ) -o $@

Build/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

core-test: $(CORE_TEST)
	./$(CORE_TEST)

ir-test: $(IR_TEST)
	./$(IR_TEST)

decoder-test: $(DECODER_TEST)
	./$(DECODER_TEST)

cpu-state-test: $(AARCH64_CPU_STATE_TEST)
	./$(AARCH64_CPU_STATE_TEST)

test: $(CORE_TEST) $(IR_TEST) $(DECODER_TEST) $(AARCH64_CPU_STATE_TEST)
	./$(CORE_TEST)
	./$(IR_TEST)
	./$(DECODER_TEST)
	./$(AARCH64_CPU_STATE_TEST)

clean:
	rm -rf Build
