CC      ?= gcc
CFLAGS  ?= -std=gnu11 -Wall -Wextra -O2
CPPFLAGS ?= -ICore -IIR -IDecoder -IArchitecture -IFormats/MachO -ITests -I.

CORE_SRC := \
    Core/Core.c \
    Core/CoreConfig.c \
    Core/CoreConfigValidation.c \
    Core/CoreRegistry.c \
    Core/CoreDependency.c \
    Core/CoreInit.c \
    Core/CoreDiagnostics.c \
    Core/CoreDispatch.c \
    Core/CoreMonitor.c \
    Core/CoreTopology.c \
    Core/CoreScheduler.c \
    Core/CoreIPC.c \
    Core/CoreMemory.c \
    Core/CoreShutdown.c

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

AARCH64_CPU_STATE_SRC := Architecture/AARCH64/AARCH64CPUState.c
AARCH64_ARM64E_SRC := \
    Architecture/AARCH64/AARCH64PointerAuth.c \
    Architecture/AARCH64/AARCH64SysReg.c \
    Formats/MachO/AARCH64MachO.c

HOST_BUILD_DIR := Build/Host

CORE_OBJ := $(patsubst %.c,$(HOST_BUILD_DIR)/%.o,$(CORE_SRC))
IR_OBJ   := $(patsubst %.c,$(HOST_BUILD_DIR)/%.o,$(IR_SRC))
DECODER_OBJ := $(patsubst %.c,$(HOST_BUILD_DIR)/%.o,$(DECODER_SRC))
AARCH64_CPU_STATE_OBJ := $(patsubst %.c,$(HOST_BUILD_DIR)/%.o,$(AARCH64_CPU_STATE_SRC))
AARCH64_ARM64E_OBJ := $(patsubst %.c,$(HOST_BUILD_DIR)/%.o,$(AARCH64_ARM64E_SRC))
AARCH64_SYSREG_OBJ := $(HOST_BUILD_DIR)/Architecture/AARCH64/AARCH64SysReg.o

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
INTEGER_OPERANDS_REFERENCE_OBJ := $(HOST_BUILD_DIR)/Tests/Decoder/AARCH64/Unit/IntegerOperandReferenceTest.o
INTEGER_FORM_PROBE := $(HOST_BUILD_DIR)/aarch64integerformprobe
INTEGER_FORM_PROBE_OBJ := $(HOST_BUILD_DIR)/Tests/Decoder/AARCH64/Probes/IntegerFormProbe.o
CONTROL_FLOW_PROBE := $(HOST_BUILD_DIR)/aarch64controlflowprobe
CONTROL_FLOW_PROBE_OBJ := $(HOST_BUILD_DIR)/Tests/Decoder/AARCH64/Probes/ControlFlowProbe.o
MEMORY_PROBE := $(HOST_BUILD_DIR)/aarch64memoryprobe
MEMORY_PROBE_OBJ := $(HOST_BUILD_DIR)/Tests/Decoder/AARCH64/Probes/MemoryProbe.o
INTEGRATION_PROBE := $(HOST_BUILD_DIR)/aarch64integrationprobe
INTEGRATION_PROBE_OBJ := $(HOST_BUILD_DIR)/Tests/Decoder/AARCH64/Probes/IntegrationProbe.o
SYSTEM_PROBE := $(HOST_BUILD_DIR)/aarch64systemprobe
SYSTEM_PROBE_OBJ := $(HOST_BUILD_DIR)/Tests/Decoder/AARCH64/Probes/SystemProbe.o
SECURITY_PROBE := $(HOST_BUILD_DIR)/aarch64securityprobe
SECURITY_PROBE_OBJ := $(HOST_BUILD_DIR)/Tests/Decoder/AARCH64/Probes/SecurityProbe.o
AARCH64_CATALOG_DIR := Decoder/AARCH64/Catalog
TOOLS_CATALOG_DIR := Tools/Catalog
AARCH64_TEST_DIR := Tests/Decoder/AARCH64
AARCH64_FIXTURE_DIR := Tests/Fixtures/AARCH64
CORE_TEST_SRC := Tests/Core/CoreTest.c
IR_TEST_SRC := Tests/IR/IRTest.c
DECODER_TEST_SRC := Tests/Decoder/DecoderTest.c
AARCH64_CPU_STATE_TEST_SRC := Tests/Architecture/AARCH64/CPUStateHostTest.c
ARM64E_TEST_SRC := Tests/Architecture/AARCH64/ARM64EHostTest.c
AARCH64_CATALOG_TEST_SRC := $(AARCH64_TEST_DIR)/Unit/CatalogCoverageTest.c
AARCH64_CATALOG_INDEX_TEST_SRC := $(AARCH64_TEST_DIR)/Unit/CatalogIndexTest.c
AARCH64_CATALOG_REFERENCE_TEST_SRC := $(AARCH64_TEST_DIR)/Unit/CatalogReferenceTest.c
AARCH64_OPERANDS_TEST_SRC := $(AARCH64_TEST_DIR)/Unit/OperandExtractionTest.c

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

$(CORE_TEST): $(CORE_OBJ) $(DECODER_OBJ) $(IR_OBJ) $(AARCH64_SYSREG_OBJ) $(CORE_TEST_OBJ)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(CORE_TEST_OBJ) $(CORE_OBJ) $(DECODER_OBJ) $(IR_OBJ) $(AARCH64_SYSREG_OBJ) -o $@

$(IR_TEST): $(IR_OBJ) $(IR_TEST_OBJ)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(IR_TEST_OBJ) $(IR_OBJ) -o $@

$(DECODER_TEST): $(DECODER_OBJ) $(IR_OBJ) $(AARCH64_SYSREG_OBJ) $(DECODER_TEST_OBJ)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(DECODER_TEST_OBJ) $(DECODER_OBJ) $(IR_OBJ) $(AARCH64_SYSREG_OBJ) -o $@

$(AARCH64_CPU_STATE_TEST): $(AARCH64_CPU_STATE_OBJ) $(AARCH64_CPU_STATE_TEST_OBJ)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(AARCH64_CPU_STATE_TEST_OBJ) $(AARCH64_CPU_STATE_OBJ) -o $@

$(ARM64E_TEST): $(AARCH64_CPU_STATE_OBJ) $(AARCH64_ARM64E_OBJ) $(DECODER_OBJ) $(IR_OBJ) $(ARM64E_TEST_OBJ)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(ARM64E_TEST_OBJ) $(AARCH64_CPU_STATE_OBJ) $(AARCH64_ARM64E_OBJ) $(DECODER_OBJ) $(IR_OBJ) -o $@

$(AARCH64_CATALOG_TEST): $(DECODER_OBJ) $(IR_OBJ) $(AARCH64_SYSREG_OBJ) $(AARCH64_CATALOG_TEST_OBJ) $(AARCH64_FIXTURE_DIR)/AARCH64CatalogPatterns.inc
	$(CC) $(CFLAGS) $(CPPFLAGS) $(AARCH64_CATALOG_TEST_OBJ) $(DECODER_OBJ) $(IR_OBJ) $(AARCH64_SYSREG_OBJ) -o $@

$(AARCH64_CATALOG_INDEX_TEST): $(HOST_BUILD_DIR)/Decoder/AARCH64/Catalog/AARCH64Catalog.o $(AARCH64_CATALOG_INDEX_TEST_OBJ)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(AARCH64_CATALOG_INDEX_TEST_OBJ) $(HOST_BUILD_DIR)/Decoder/AARCH64/Catalog/AARCH64Catalog.o -o $@

$(AARCH64_CATALOG_REFERENCE_TEST): $(HOST_BUILD_DIR)/Decoder/AARCH64/Catalog/AARCH64Catalog.o $(AARCH64_CATALOG_REFERENCE_TEST_OBJ)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(AARCH64_CATALOG_REFERENCE_TEST_OBJ) $(HOST_BUILD_DIR)/Decoder/AARCH64/Catalog/AARCH64Catalog.o -o $@

$(AARCH64_OPERANDS_TEST): $(HOST_BUILD_DIR)/Decoder/AARCH64/Catalog/AARCH64Catalog.o $(HOST_BUILD_DIR)/Decoder/AARCH64/Operands/AARCH64Operands.o $(AARCH64_SYSREG_OBJ) $(AARCH64_OPERANDS_TEST_OBJ)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(AARCH64_OPERANDS_TEST_OBJ) $(HOST_BUILD_DIR)/Decoder/AARCH64/Catalog/AARCH64Catalog.o $(HOST_BUILD_DIR)/Decoder/AARCH64/Operands/AARCH64Operands.o $(AARCH64_SYSREG_OBJ) -o $@

$(INTEGER_OPERANDS_REFERENCE): $(HOST_BUILD_DIR)/Decoder/AARCH64/Catalog/AARCH64Catalog.o $(HOST_BUILD_DIR)/Decoder/AARCH64/Operands/AARCH64Operands.o $(AARCH64_SYSREG_OBJ) $(INTEGER_OPERANDS_REFERENCE_OBJ)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(INTEGER_OPERANDS_REFERENCE_OBJ) $(HOST_BUILD_DIR)/Decoder/AARCH64/Catalog/AARCH64Catalog.o $(HOST_BUILD_DIR)/Decoder/AARCH64/Operands/AARCH64Operands.o $(AARCH64_SYSREG_OBJ) -o $@

$(INTEGER_FORM_PROBE): $(HOST_BUILD_DIR)/Decoder/AARCH64/Catalog/AARCH64Catalog.o $(HOST_BUILD_DIR)/Decoder/AARCH64/Operands/AARCH64Operands.o $(AARCH64_SYSREG_OBJ) $(INTEGER_FORM_PROBE_OBJ)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(INTEGER_FORM_PROBE_OBJ) $(HOST_BUILD_DIR)/Decoder/AARCH64/Catalog/AARCH64Catalog.o $(HOST_BUILD_DIR)/Decoder/AARCH64/Operands/AARCH64Operands.o $(AARCH64_SYSREG_OBJ) -o $@

$(CONTROL_FLOW_PROBE): $(HOST_BUILD_DIR)/Decoder/AARCH64/Catalog/AARCH64Catalog.o $(HOST_BUILD_DIR)/Decoder/AARCH64/Operands/AARCH64Operands.o $(AARCH64_SYSREG_OBJ) $(CONTROL_FLOW_PROBE_OBJ)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(CONTROL_FLOW_PROBE_OBJ) $(HOST_BUILD_DIR)/Decoder/AARCH64/Catalog/AARCH64Catalog.o $(HOST_BUILD_DIR)/Decoder/AARCH64/Operands/AARCH64Operands.o $(AARCH64_SYSREG_OBJ) -o $@

$(MEMORY_PROBE): $(HOST_BUILD_DIR)/Decoder/AARCH64/Catalog/AARCH64Catalog.o $(HOST_BUILD_DIR)/Decoder/AARCH64/Operands/AARCH64Operands.o $(AARCH64_SYSREG_OBJ) $(MEMORY_PROBE_OBJ)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(MEMORY_PROBE_OBJ) $(HOST_BUILD_DIR)/Decoder/AARCH64/Catalog/AARCH64Catalog.o $(HOST_BUILD_DIR)/Decoder/AARCH64/Operands/AARCH64Operands.o $(AARCH64_SYSREG_OBJ) -o $@

$(INTEGRATION_PROBE): $(DECODER_OBJ) $(IR_OBJ) $(AARCH64_SYSREG_OBJ) $(INTEGRATION_PROBE_OBJ)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(INTEGRATION_PROBE_OBJ) $(DECODER_OBJ) $(IR_OBJ) $(AARCH64_SYSREG_OBJ) -o $@

$(SYSTEM_PROBE): $(DECODER_OBJ) $(IR_OBJ) $(AARCH64_SYSREG_OBJ) $(SYSTEM_PROBE_OBJ)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(SYSTEM_PROBE_OBJ) $(DECODER_OBJ) $(IR_OBJ) $(AARCH64_SYSREG_OBJ) -o $@

$(SECURITY_PROBE): $(DECODER_OBJ) $(IR_OBJ) $(AARCH64_SYSREG_OBJ) $(SECURITY_PROBE_OBJ)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(SECURITY_PROBE_OBJ) $(DECODER_OBJ) $(IR_OBJ) $(AARCH64_SYSREG_OBJ) -o $@

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
	python3 $(TOOLS_CATALOG_DIR)/catalog_tool.py validate

catalog-index-test: $(AARCH64_CATALOG_INDEX_TEST)
	./$(AARCH64_CATALOG_INDEX_TEST)

catalog-differential-test: $(AARCH64_CATALOG_REFERENCE_TEST)
	python3 $(AARCH64_TEST_DIR)/Validation/catalog_llvm_differential.py ./$(AARCH64_CATALOG_REFERENCE_TEST)

catalog-coverage-test: $(AARCH64_CATALOG_TEST)
	./$(AARCH64_CATALOG_TEST)

catalog-test: catalog-structure-test catalog-coverage-test catalog-index-test \
	catalog-differential-test

operand-extraction-test: $(AARCH64_OPERANDS_TEST) $(INTEGER_OPERANDS_REFERENCE) $(INTEGER_FORM_PROBE) $(CONTROL_FLOW_PROBE) $(MEMORY_PROBE) $(INTEGRATION_PROBE) $(SYSTEM_PROBE) $(SECURITY_PROBE)
	./$(AARCH64_OPERANDS_TEST)
	python3 $(AARCH64_TEST_DIR)/Validation/integer_operand_llvm_differential.py ./$(INTEGER_OPERANDS_REFERENCE)
	python3 $(AARCH64_TEST_DIR)/Validation/integer_form_differential.py ./$(INTEGER_FORM_PROBE)
	python3 $(AARCH64_TEST_DIR)/Validation/integer_immediate_differential.py ./$(INTEGER_FORM_PROBE)
	python3 $(AARCH64_TEST_DIR)/Validation/integer_immediate_rejection_audit.py ./$(INTEGER_FORM_PROBE)
	python3 $(AARCH64_TEST_DIR)/Validation/bitmask_exhaustive.py ./$(INTEGER_FORM_PROBE)
	python3 $(AARCH64_TEST_DIR)/Validation/integer_condition_audit.py ./$(INTEGER_FORM_PROBE)
	python3 $(AARCH64_TEST_DIR)/Validation/integer_alias_predicates.py ./$(INTEGER_FORM_PROBE)
	python3 $(AARCH64_TEST_DIR)/Validation/integer_alias_coverage.py
	python3 $(AARCH64_TEST_DIR)/Validation/integer_implicit_operands.py ./$(INTEGER_FORM_PROBE)
	python3 $(AARCH64_TEST_DIR)/Validation/integer_invalid_encodings.py ./$(INTEGER_FORM_PROBE)
	python3 $(AARCH64_TEST_DIR)/Validation/integer_comprehensive_validation.py ./$(INTEGER_FORM_PROBE)
	python3 $(AARCH64_TEST_DIR)/Validation/control_flow_validation.py ./$(CONTROL_FLOW_PROBE)
	python3 $(AARCH64_TEST_DIR)/Validation/authenticated_control_flow_validation.py ./$(CONTROL_FLOW_PROBE)
	python3 $(AARCH64_TEST_DIR)/Validation/control_flow_comprehensive_validation.py ./$(CONTROL_FLOW_PROBE)
	python3 $(AARCH64_TEST_DIR)/Validation/memory_operand_validation.py ./$(MEMORY_PROBE)
	python3 $(AARCH64_TEST_DIR)/Validation/specialized_memory_validation.py ./$(MEMORY_PROBE)
	python3 $(AARCH64_TEST_DIR)/Validation/integration_validation.py ./$(INTEGRATION_PROBE)
	python3 $(AARCH64_TEST_DIR)/Validation/system_validation.py ./$(SYSTEM_PROBE)
	python3 $(AARCH64_TEST_DIR)/Validation/security_validation.py ./$(SECURITY_PROBE)

test: $(CORE_TEST) $(IR_TEST) $(DECODER_TEST) $(AARCH64_CPU_STATE_TEST) $(ARM64E_TEST) catalog-test operand-extraction-test
	./$(CORE_TEST)
	./$(IR_TEST)
	./$(DECODER_TEST)
	./$(AARCH64_CPU_STATE_TEST)
	./$(ARM64E_TEST)

clean:
	rm -rf $(HOST_BUILD_DIR)
