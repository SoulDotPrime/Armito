CC      ?= gcc
CFLAGS  ?= -std=gnu11 -Wall -Wextra -O2
CPPFLAGS ?= -ICore -IIR -IDecoder -ICPU -ITests -I.

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
    Decoder/AARCH64/Catalog/AARCH64Catalog.c \
    Decoder/AARCH64/Operands/AARCH64Operands.c \
    Decoder/AARCH64/AARCH64.c

AARCH64_CPU_STATE_SRC := CPU/AARCH64/AARCH64CPUState.c
AARCH64_ARM64E_SRC := \
    CPU/AARCH64/AARCH64PointerAuth.c \
    CPU/AARCH64/AARCH64MachO.c

HOST_BUILD_DIR := Build/Host

CORE_OBJ := $(patsubst %.c,$(HOST_BUILD_DIR)/%.o,$(CORE_SRC))
IR_OBJ   := $(patsubst %.c,$(HOST_BUILD_DIR)/%.o,$(IR_SRC))
DECODER_OBJ := $(patsubst %.c,$(HOST_BUILD_DIR)/%.o,$(DECODER_SRC))
AARCH64_CPU_STATE_OBJ := $(patsubst %.c,$(HOST_BUILD_DIR)/%.o,$(AARCH64_CPU_STATE_SRC))
AARCH64_ARM64E_OBJ := $(patsubst %.c,$(HOST_BUILD_DIR)/%.o,$(AARCH64_ARM64E_SRC))

CORE_TEST := $(HOST_BUILD_DIR)/coretest
IR_TEST   := $(HOST_BUILD_DIR)/irtest
DECODER_TEST := $(HOST_BUILD_DIR)/decodertest
AARCH64_CPU_STATE_TEST := $(HOST_BUILD_DIR)/aarch64cpustatetest
ARM64E_TEST := $(HOST_BUILD_DIR)/arm64etest
AARCH64_CATALOG_TEST := $(HOST_BUILD_DIR)/aarch64catalogtest
AARCH64_CATALOG_INDEX_TEST := $(HOST_BUILD_DIR)/aarch64catalogindextest
AARCH64_CATALOG_REFERENCE_TEST := $(HOST_BUILD_DIR)/aarch64catalogreferencetest
AARCH64_OPERANDS_TEST := $(HOST_BUILD_DIR)/aarch64operandstest
INTEGER_OPERANDS_REFERENCE := $(HOST_BUILD_DIR)/aarch64integerreference
INTEGER_OPERANDS_REFERENCE_OBJ := $(HOST_BUILD_DIR)/Tests/Decoder/AARCH64/IntegerOperandReferenceTest.o
INTEGER_FORM_PROBE := $(HOST_BUILD_DIR)/aarch64integerformprobe
INTEGER_FORM_PROBE_OBJ := $(HOST_BUILD_DIR)/Tests/Decoder/AARCH64/IntegerFormProbe.o
AARCH64_CATALOG_DIR := Decoder/AARCH64/Catalog
AARCH64_TEST_DIR := Tests/Decoder/AARCH64
AARCH64_FIXTURE_DIR := Tests/Fixtures/AARCH64
CORE_TEST_SRC := Tests/Core/CoreTest.c
IR_TEST_SRC := Tests/IR/IRTest.c
DECODER_TEST_SRC := Tests/Decoder/DecoderTest.c
AARCH64_CPU_STATE_TEST_SRC := Tests/CPU/AARCH64/CPUStateHostTest.c
ARM64E_TEST_SRC := Tests/CPU/AARCH64/ARM64EHostTest.c
AARCH64_CATALOG_TEST_SRC := $(AARCH64_TEST_DIR)/CatalogCoverageTest.c
AARCH64_CATALOG_INDEX_TEST_SRC := $(AARCH64_TEST_DIR)/CatalogIndexTest.c
AARCH64_CATALOG_REFERENCE_TEST_SRC := $(AARCH64_TEST_DIR)/CatalogReferenceTest.c
AARCH64_OPERANDS_TEST_SRC := $(AARCH64_TEST_DIR)/OperandExtractionTest.c

HOST_TEST_SRC := \
    $(CORE_TEST_SRC) \
    $(IR_TEST_SRC) \
    $(DECODER_TEST_SRC) \
    $(AARCH64_CPU_STATE_TEST_SRC) \
    $(ARM64E_TEST_SRC) \
    $(AARCH64_CATALOG_TEST_SRC) \
    $(AARCH64_CATALOG_INDEX_TEST_SRC) \
    $(AARCH64_CATALOG_REFERENCE_TEST_SRC) \
    $(AARCH64_OPERANDS_TEST_SRC)
HOST_TEST_OBJ := $(patsubst %.c,$(HOST_BUILD_DIR)/%.o,$(HOST_TEST_SRC))
CORE_TEST_OBJ := $(HOST_BUILD_DIR)/$(CORE_TEST_SRC:.c=.o)
IR_TEST_OBJ := $(HOST_BUILD_DIR)/$(IR_TEST_SRC:.c=.o)
DECODER_TEST_OBJ := $(HOST_BUILD_DIR)/$(DECODER_TEST_SRC:.c=.o)
AARCH64_CPU_STATE_TEST_OBJ := $(HOST_BUILD_DIR)/$(AARCH64_CPU_STATE_TEST_SRC:.c=.o)
ARM64E_TEST_OBJ := $(HOST_BUILD_DIR)/$(ARM64E_TEST_SRC:.c=.o)
AARCH64_CATALOG_TEST_OBJ := $(HOST_BUILD_DIR)/$(AARCH64_CATALOG_TEST_SRC:.c=.o)
AARCH64_CATALOG_INDEX_TEST_OBJ := $(HOST_BUILD_DIR)/$(AARCH64_CATALOG_INDEX_TEST_SRC:.c=.o)
AARCH64_CATALOG_REFERENCE_TEST_OBJ := $(HOST_BUILD_DIR)/$(AARCH64_CATALOG_REFERENCE_TEST_SRC:.c=.o)
AARCH64_OPERANDS_TEST_OBJ := $(HOST_BUILD_DIR)/$(AARCH64_OPERANDS_TEST_SRC:.c=.o)
HOST_OBJECTS := $(CORE_OBJ) $(IR_OBJ) $(DECODER_OBJ) \
    $(AARCH64_CPU_STATE_OBJ) $(AARCH64_ARM64E_OBJ) $(HOST_TEST_OBJ)
HOST_DEPFLAGS := -MMD -MP

.PHONY: all test core-test ir-test decoder-test cpu-state-test arm64e-test \
	catalog-structure-test catalog-coverage-test catalog-index-test \
	catalog-differential-test catalog-test operand-extraction-test clean

all: $(CORE_TEST) $(IR_TEST) $(DECODER_TEST) $(AARCH64_CPU_STATE_TEST) $(ARM64E_TEST)

$(CORE_TEST): $(CORE_OBJ) $(DECODER_OBJ) $(IR_OBJ) $(CORE_TEST_OBJ)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(CORE_TEST_OBJ) $(CORE_OBJ) $(DECODER_OBJ) $(IR_OBJ) -o $@

$(IR_TEST): $(IR_OBJ) $(IR_TEST_OBJ)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(IR_TEST_OBJ) $(IR_OBJ) -o $@

$(DECODER_TEST): $(DECODER_OBJ) $(IR_OBJ) $(DECODER_TEST_OBJ)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(DECODER_TEST_OBJ) $(DECODER_OBJ) $(IR_OBJ) -o $@

$(AARCH64_CPU_STATE_TEST): $(AARCH64_CPU_STATE_OBJ) $(AARCH64_CPU_STATE_TEST_OBJ)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(AARCH64_CPU_STATE_TEST_OBJ) $(AARCH64_CPU_STATE_OBJ) -o $@

$(ARM64E_TEST): $(AARCH64_CPU_STATE_OBJ) $(AARCH64_ARM64E_OBJ) $(DECODER_OBJ) $(IR_OBJ) $(ARM64E_TEST_OBJ)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(ARM64E_TEST_OBJ) $(AARCH64_CPU_STATE_OBJ) $(AARCH64_ARM64E_OBJ) $(DECODER_OBJ) $(IR_OBJ) -o $@

$(AARCH64_CATALOG_TEST): $(DECODER_OBJ) $(IR_OBJ) $(AARCH64_CATALOG_TEST_OBJ) $(AARCH64_FIXTURE_DIR)/AARCH64CatalogPatterns.inc
	$(CC) $(CFLAGS) $(CPPFLAGS) $(AARCH64_CATALOG_TEST_OBJ) $(DECODER_OBJ) $(IR_OBJ) -o $@

$(AARCH64_CATALOG_INDEX_TEST): $(HOST_BUILD_DIR)/Decoder/AARCH64/Catalog/AARCH64Catalog.o $(AARCH64_CATALOG_INDEX_TEST_OBJ)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(AARCH64_CATALOG_INDEX_TEST_OBJ) $(HOST_BUILD_DIR)/Decoder/AARCH64/Catalog/AARCH64Catalog.o -o $@

$(AARCH64_CATALOG_REFERENCE_TEST): $(HOST_BUILD_DIR)/Decoder/AARCH64/Catalog/AARCH64Catalog.o $(AARCH64_CATALOG_REFERENCE_TEST_OBJ)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(AARCH64_CATALOG_REFERENCE_TEST_OBJ) $(HOST_BUILD_DIR)/Decoder/AARCH64/Catalog/AARCH64Catalog.o -o $@

$(AARCH64_OPERANDS_TEST): $(HOST_BUILD_DIR)/Decoder/AARCH64/Catalog/AARCH64Catalog.o $(HOST_BUILD_DIR)/Decoder/AARCH64/Operands/AARCH64Operands.o $(AARCH64_OPERANDS_TEST_OBJ)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(AARCH64_OPERANDS_TEST_OBJ) $(HOST_BUILD_DIR)/Decoder/AARCH64/Catalog/AARCH64Catalog.o $(HOST_BUILD_DIR)/Decoder/AARCH64/Operands/AARCH64Operands.o -o $@

$(INTEGER_OPERANDS_REFERENCE): $(HOST_BUILD_DIR)/Decoder/AARCH64/Catalog/AARCH64Catalog.o $(HOST_BUILD_DIR)/Decoder/AARCH64/Operands/AARCH64Operands.o $(INTEGER_OPERANDS_REFERENCE_OBJ)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(INTEGER_OPERANDS_REFERENCE_OBJ) $(HOST_BUILD_DIR)/Decoder/AARCH64/Catalog/AARCH64Catalog.o $(HOST_BUILD_DIR)/Decoder/AARCH64/Operands/AARCH64Operands.o -o $@

$(INTEGER_FORM_PROBE): $(HOST_BUILD_DIR)/Decoder/AARCH64/Catalog/AARCH64Catalog.o $(HOST_BUILD_DIR)/Decoder/AARCH64/Operands/AARCH64Operands.o $(INTEGER_FORM_PROBE_OBJ)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(INTEGER_FORM_PROBE_OBJ) $(HOST_BUILD_DIR)/Decoder/AARCH64/Catalog/AARCH64Catalog.o $(HOST_BUILD_DIR)/Decoder/AARCH64/Operands/AARCH64Operands.o -o $@

$(HOST_BUILD_DIR)/Decoder/AARCH64/Catalog/AARCH64Catalog.o: Decoder/AARCH64/Catalog/AARCH64Catalog.h Decoder/AARCH64/Catalog/AARCH64CatalogData.inc
$(HOST_BUILD_DIR)/Decoder/AARCH64/AARCH64.o: Decoder/AARCH64/Catalog/AARCH64Catalog.h

$(HOST_BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(HOST_DEPFLAGS) -c $< -o $@

-include $(HOST_OBJECTS:.o=.d)

core-test: $(CORE_TEST)
	./$(CORE_TEST)

ir-test: $(IR_TEST)
	./$(IR_TEST)

decoder-test: $(DECODER_TEST)
	./$(DECODER_TEST)

cpu-state-test: $(AARCH64_CPU_STATE_TEST)
	./$(AARCH64_CPU_STATE_TEST)

arm64e-test: $(ARM64E_TEST)
	./$(ARM64E_TEST)

catalog-structure-test:
	python3 $(AARCH64_CATALOG_DIR)/catalog_tool.py validate

catalog-index-test: $(AARCH64_CATALOG_INDEX_TEST)
	./$(AARCH64_CATALOG_INDEX_TEST)

catalog-differential-test: $(AARCH64_CATALOG_REFERENCE_TEST)
	python3 $(AARCH64_TEST_DIR)/catalog_llvm_differential.py ./$(AARCH64_CATALOG_REFERENCE_TEST)

catalog-coverage-test: $(AARCH64_CATALOG_TEST)
	./$(AARCH64_CATALOG_TEST)

catalog-test: catalog-structure-test catalog-coverage-test catalog-index-test \
	catalog-differential-test

operand-extraction-test: $(AARCH64_OPERANDS_TEST) $(INTEGER_OPERANDS_REFERENCE) $(INTEGER_FORM_PROBE)
	./$(AARCH64_OPERANDS_TEST)
	python3 $(AARCH64_TEST_DIR)/integer_operand_llvm_differential.py ./$(INTEGER_OPERANDS_REFERENCE)
	python3 $(AARCH64_TEST_DIR)/integer_form_differential.py ./$(INTEGER_FORM_PROBE)
	python3 $(AARCH64_TEST_DIR)/integer_immediate_differential.py ./$(INTEGER_FORM_PROBE)
	python3 $(AARCH64_TEST_DIR)/integer_immediate_rejection_audit.py ./$(INTEGER_FORM_PROBE)
	python3 $(AARCH64_TEST_DIR)/bitmask_exhaustive.py ./$(INTEGER_FORM_PROBE)

test: $(CORE_TEST) $(IR_TEST) $(DECODER_TEST) $(AARCH64_CPU_STATE_TEST) $(ARM64E_TEST) catalog-test operand-extraction-test
	./$(CORE_TEST)
	./$(IR_TEST)
	./$(DECODER_TEST)
	./$(AARCH64_CPU_STATE_TEST)
	./$(ARM64E_TEST)

clean:
	rm -rf $(HOST_BUILD_DIR)
