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

CORE_OBJ := $(patsubst %.c,Build/%.o,$(CORE_SRC))
IR_OBJ   := $(patsubst %.c,Build/%.o,$(IR_SRC))
DECODER_OBJ := $(patsubst %.c,Build/%.o,$(DECODER_SRC))

CORE_TEST := Build/coretest
IR_TEST   := Build/irtest
DECODER_TEST := Build/decodertest

.PHONY: all test core-test ir-test clean

all: $(CORE_TEST) $(IR_TEST) $(DECODER_TEST)

$(CORE_TEST): $(CORE_OBJ) Tests/CoreTest.c
	$(CC) $(CFLAGS) $(CPPFLAGS) Tests/CoreTest.c $(CORE_OBJ) -o $@

$(IR_TEST): $(IR_OBJ) Tests/IRTest.c
	$(CC) $(CFLAGS) $(CPPFLAGS) Tests/IRTest.c $(IR_OBJ) -o $@

$(DECODER_TEST): $(DECODER_OBJ) $(IR_OBJ) Tests/DecoderTest.c
	$(CC) $(CFLAGS) $(CPPFLAGS) Tests/DecoderTest.c $(DECODER_OBJ) $(IR_OBJ) -o $@

Build/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

core-test: $(CORE_TEST)
	./$(CORE_TEST)

ir-test: $(IR_TEST)
	./$(IR_TEST)

decoder-test: $(DECODER_TEST)
	./$(DECODER_TEST)

test: $(CORE_TEST) $(IR_TEST) $(DECODER_TEST)
	./$(CORE_TEST)
	./$(IR_TEST)
	./$(DECODER_TEST)

clean:
	rm -rf Build
