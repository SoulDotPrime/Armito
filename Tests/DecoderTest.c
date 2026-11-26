#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "Includes/Decoder.h"
#include "AARCH64/Includes/AARCH64.h"
#include "Includes/IR.h"

static uint32_t tests_run = 0;
static uint32_t tests_failed = 0;
static const char *current_test = "";

#define CHECK(expr) do { \
    tests_run++; \
    if (!(expr)) { \
        tests_failed++; \
        printf("  FAIL  %s: %s (%s:%d)\n", \
            current_test, #expr, __FILE__, __LINE__); \
    } \
} while (0)

#define CHECK_DEC(expr, expected) do { \
    ArmitoDecoderStatus s_ = (ArmitoDecoderStatus)(expr); \
    ArmitoDecoderStatus e_ = (ArmitoDecoderStatus)(expected); \
    tests_run++; \
    if (s_ != e_) { \
        tests_failed++; \
        printf("  FAIL  %s: %s devolveu %d (%s), esperado %d (%s:%d)\n", \
            current_test, #expr, (int)s_, ArmitoDecoderStatusName(s_), \
            (int)e_, __FILE__, __LINE__); \
    } \
} while (0)

static const uint8_t program[] = {
    0x1F, 0x20, 0x03, 0xD5,
    0x00, 0x00, 0x80, 0xD2,
    0x00, 0x00, 0x00, 0x8B,
    0xC0, 0x03, 0x5F, 0xD6,
    0x00, 0x00, 0x00, 0x14,
    0x1F, 0x20, 0x03, 0xD5
};

static ArmitoDecoderConfig MakeConfig(void)
{
    ArmitoDecoderConfig config;

    config.isa = ARMITO_ISA_AARCH64;
    config.entry_address = 0x1000;
    config.max_instructions = 16;
    config.stop_at_branch = 1;
    config.record_unsupported = 1;

    return config;
}

static void TestLifecycle(void)
{
    ArmitoDecoderConfig config = MakeConfig();

    current_test = "ciclo de vida do decoder";

    CHECK(ArmitoDecoderGetState() == ARMITO_DECODER_UNINITIALIZED);

    CHECK_DEC(ArmitoDecoderInitialize(NULL), ARMITO_DECODER_INVALID_ARGUMENT);

    config.isa = ARMITO_ISA_UNKNOWN;
    CHECK_DEC(ArmitoDecoderInitialize(&config), ARMITO_DECODER_ERROR);

    config = MakeConfig();
    config.max_instructions = 0;
    CHECK_DEC(ArmitoDecoderInitialize(&config), ARMITO_DECODER_INVALID_ARGUMENT);

    config = MakeConfig();
    CHECK_DEC(ArmitoDecoderInitialize(&config), ARMITO_DECODER_SUCCESS);
    CHECK(ArmitoDecoderGetState() == ARMITO_DECODER_READY);
    CHECK(ArmitoDecoderGetISA() == ARMITO_ISA_AARCH64);

    CHECK_DEC(ArmitoDecoderReset(), ARMITO_DECODER_SUCCESS);
    CHECK(ArmitoDecoderGetState() == ARMITO_DECODER_READY);
}

static void TestSource(void)
{
    ArmitoDecoderSource src;
    ArmitoDecoderInstruction instruction;
    ArmitoDecoderConfig config = MakeConfig();

    current_test = "memoria de origem";

    CHECK_DEC(ArmitoDecoderInitialize(&config), ARMITO_DECODER_SUCCESS);

    CHECK_DEC(ArmitoDecoderSourceGet(&src), ARMITO_DECODER_INVALID_STATE);

    CHECK_DEC(
        ArmitoDecoderSourceSet(program, sizeof(program), 0x1001, 1, 1),
        ARMITO_DECODER_INVALID_ARGUMENT
    );

    CHECK_DEC(
        ArmitoDecoderSourceSet(NULL, 16, 0x1000, 1, 1),
        ARMITO_DECODER_INVALID_ARGUMENT
    );

    CHECK_DEC(
        ArmitoDecoderSourceSet(program, sizeof(program), 0x1000, 1, 1),
        ARMITO_DECODER_SUCCESS
    );

    CHECK_DEC(ArmitoDecoderSourceGet(&src), ARMITO_DECODER_SUCCESS);
    CHECK(src.size == sizeof(program));
    CHECK(src.address == 0x1000);
    CHECK(src.executable == 1);

    CHECK_DEC(ArmitoDecoderSourceFetch(0, &instruction), ARMITO_DECODER_SUCCESS);
    CHECK(instruction.address == 0x1000);
    CHECK(instruction.encoding == 0xD503201F);

    CHECK_DEC(
        ArmitoDecoderSourceFetch(4, &instruction), ARMITO_DECODER_SUCCESS
    );
    CHECK(instruction.address == 0x1004);
    CHECK(instruction.encoding == 0xD2800000);

    CHECK_DEC(
        ArmitoDecoderSourceFetch(2, &instruction),
        ARMITO_DECODER_INVALID_ARGUMENT
    );

    CHECK_DEC(
        ArmitoDecoderSourceFetch(sizeof(program), &instruction),
        ARMITO_DECODER_OUT_OF_RANGE
    );

    CHECK_DEC(
        ArmitoDecoderSourceFetch(sizeof(program) + 4, &instruction),
        ARMITO_DECODER_OUT_OF_RANGE
    );

    CHECK_DEC(
        ArmitoDecoderSourceSet(program, 2, 0x1000, 1, 1),
        ARMITO_DECODER_SUCCESS
    );
    CHECK_DEC(
        ArmitoDecoderSourceFetch(0, &instruction), ARMITO_DECODER_TRUNCATED
    );

    CHECK_DEC(
        ArmitoDecoderSourceSet(program, sizeof(program), 0x1000, 0, 1),
        ARMITO_DECODER_SUCCESS
    );
    CHECK_DEC(
        ArmitoDecoderSourceFetch(0, &instruction), ARMITO_DECODER_INVALID_STATE
    );

    CHECK_DEC(ArmitoDecoderSourceClear(), ARMITO_DECODER_SUCCESS);
    CHECK_DEC(ArmitoDecoderSourceGet(&src), ARMITO_DECODER_INVALID_STATE);
}

static void TestRegisters(void)
{
    ArmitoDecoderRegisterRef reg;

    current_test = "decodificacao de registrador";

    CHECK_DEC(
        ArmitoDecoderRegisterDecode(0, 1, 64, &reg), ARMITO_DECODER_SUCCESS
    );
    CHECK(reg.index == 0);
    CHECK(reg.is_sp == 0);
    CHECK(reg.is_zero == 0);
    CHECK(reg.gpr == ARMITO_DECODER_GPR_X0);
    CHECK(strcmp(ArmitoDecoderRegisterName(reg.gpr), "x0") == 0);

    CHECK_DEC(
        ArmitoDecoderRegisterDecode(31, 1, 64, &reg), ARMITO_DECODER_SUCCESS
    );
    CHECK(reg.is_sp == 1);
    CHECK(reg.is_zero == 0);
    CHECK(reg.gpr == ARMITO_DECODER_GPR_SP);
    CHECK(strcmp(ArmitoDecoderRegisterName(reg.gpr), "sp") == 0);

    CHECK_DEC(
        ArmitoDecoderRegisterDecode(31, 0, 64, &reg), ARMITO_DECODER_SUCCESS
    );
    CHECK(reg.is_sp == 0);
    CHECK(reg.is_zero == 1);
    CHECK(reg.gpr == ARMITO_DECODER_GPR_XZR);
    CHECK(strcmp(ArmitoDecoderRegisterName(reg.gpr), "xzr") == 0);

    CHECK_DEC(
        ArmitoDecoderRegisterDecode(30, 0, 64, &reg), ARMITO_DECODER_SUCCESS
    );
    CHECK(reg.gpr == ARMITO_DECODER_GPR_X30_LR);

    CHECK_DEC(
        ArmitoDecoderRegisterDecode(29, 0, 32, &reg), ARMITO_DECODER_SUCCESS
    );
    CHECK(reg.width_bits == 32);
    CHECK(reg.gpr == ARMITO_DECODER_GPR_X29_FP);

    CHECK_DEC(
        ArmitoDecoderRegisterDecode(32, 0, 64, &reg),
        ARMITO_DECODER_INVALID_ARGUMENT
    );
    CHECK_DEC(
        ArmitoDecoderRegisterDecode(0, 0, 16, &reg),
        ARMITO_DECODER_INVALID_ARGUMENT
    );
    CHECK_DEC(
        ArmitoDecoderRegisterDecode(0, 0, 64, NULL),
        ARMITO_DECODER_INVALID_ARGUMENT
    );
}

static void TestClassification(void)
{
    AARCH64Opcode opcode;
    const char *mnemonic;

    current_test = "classificacao AArch64";

    CHECK_DEC(AARCH64Classify(0xD503201F, &opcode), ARMITO_DECODER_SUCCESS);
    CHECK(opcode == AARCH64_OPCODE_NOP);
    CHECK(strcmp(AARCH64OpcodeName(opcode), "nop") == 0);

    CHECK_DEC(AARCH64Classify(0xD65F03C0, &opcode), ARMITO_DECODER_SUCCESS);
    CHECK(opcode == AARCH64_OPCODE_RET);

    CHECK_DEC(AARCH64Classify(0x14000000, &opcode), ARMITO_DECODER_SUCCESS);
    CHECK(opcode == AARCH64_OPCODE_B);

    CHECK_DEC(AARCH64Classify(0x94000000, &opcode), ARMITO_DECODER_SUCCESS);
    CHECK(opcode == AARCH64_OPCODE_BL);

    CHECK_DEC(AARCH64Classify(0xD61F0000, &opcode), ARMITO_DECODER_SUCCESS);
    CHECK(opcode == AARCH64_OPCODE_BR);

    CHECK_DEC(AARCH64Classify(0xD63F0000, &opcode), ARMITO_DECODER_SUCCESS);
    CHECK(opcode == AARCH64_OPCODE_BLR);

    CHECK_DEC(AARCH64Classify(0x54FFFFE0, &opcode), ARMITO_DECODER_SUCCESS);
    CHECK(opcode == AARCH64_OPCODE_B_COND);
    CHECK_DEC(AARCH64ClassifyMnemonic(0x54FFFFE0, &mnemonic), ARMITO_DECODER_SUCCESS);
    CHECK(strcmp(mnemonic, "b.eq") == 0);
    CHECK_DEC(AARCH64ClassifyGeneric(0x14000000, &opcode), ARMITO_DECODER_SUCCESS);
    CHECK(opcode == AARCH64_OPCODE_B);

    CHECK_DEC(AARCH64Classify(0xB4000040, &opcode), ARMITO_DECODER_SUCCESS);
    CHECK(opcode == AARCH64_OPCODE_CBZ);

    CHECK_DEC(AARCH64Classify(0xB5000040, &opcode), ARMITO_DECODER_SUCCESS);
    CHECK(opcode == AARCH64_OPCODE_CBNZ);

    CHECK_DEC(AARCH64Classify(0x36000040, &opcode), ARMITO_DECODER_SUCCESS);
    CHECK(opcode == AARCH64_OPCODE_TBZ);

    CHECK_DEC(AARCH64Classify(0x37000040, &opcode), ARMITO_DECODER_SUCCESS);
    CHECK(opcode == AARCH64_OPCODE_TBNZ);

    CHECK_DEC(AARCH64Classify(0xAA0103E0, &opcode), ARMITO_DECODER_SUCCESS);
    CHECK(opcode == AARCH64_OPCODE_MOV_REG);

    CHECK_DEC(AARCH64Classify(0xD2800000, &opcode), ARMITO_DECODER_SUCCESS);
    CHECK(opcode == AARCH64_OPCODE_MOVZ);

    CHECK_DEC(AARCH64Classify(0x8B020020, &opcode), ARMITO_DECODER_SUCCESS);
    CHECK(opcode == AARCH64_OPCODE_ADD_SHIFTED);

    CHECK_DEC(AARCH64Classify(0xCB020020, &opcode), ARMITO_DECODER_SUCCESS);
    CHECK(opcode == AARCH64_OPCODE_SUB_SHIFTED);

    CHECK_DEC(AARCH64Classify(0xF9400020, &opcode), ARMITO_DECODER_UNSUPPORTED_INSTRUCTION);

    CHECK_DEC(AARCH64Classify(0xFFFFFFFF, &opcode), ARMITO_DECODER_UNSUPPORTED_INSTRUCTION);
    CHECK(opcode == AARCH64_OPCODE_UNKNOWN);

    CHECK_DEC(AARCH64Classify(0x00000000, NULL), ARMITO_DECODER_INVALID_ARGUMENT);
}

static void TestOperandExtraction(void)
{
    AARCH64Instruction instruction;

    current_test = "extracao de operandos AArch64";

    CHECK_DEC(AARCH64Decode(0x17FFFFFE, &instruction), ARMITO_DECODER_SUCCESS);
    CHECK(instruction.opcode == AARCH64_OPCODE_B);
    CHECK(instruction.immediate == -8);

    CHECK_DEC(AARCH64Decode(0x54FFFFE0, &instruction), ARMITO_DECODER_SUCCESS);
    CHECK(instruction.opcode == AARCH64_OPCODE_B_COND);
    CHECK(instruction.condition == 0);
    CHECK(instruction.immediate == -4);

    CHECK_DEC(AARCH64Decode(0xB4000040, &instruction), ARMITO_DECODER_SUCCESS);
    CHECK(instruction.width_bits == 64);
    CHECK(instruction.rd_index == 0);
    CHECK(instruction.immediate == 8);

    CHECK_DEC(AARCH64Decode(0x36000040, &instruction), ARMITO_DECODER_SUCCESS);
    CHECK(instruction.width_bits == 32);
    CHECK(instruction.shift_amount == 0);
    CHECK(instruction.immediate == 8);

    CHECK_DEC(AARCH64Decode(0xD2800000, &instruction), ARMITO_DECODER_SUCCESS);
    CHECK(instruction.opcode == AARCH64_OPCODE_MOVZ);
    CHECK(instruction.width_bits == 64);
    CHECK(instruction.rd_index == 0);

    CHECK_DEC(AARCH64Decode(0xD2E00020, &instruction), ARMITO_DECODER_SUCCESS);
    CHECK(instruction.shift_amount == 48);
    CHECK((uint64_t)instruction.immediate == 0x0001000000000000ULL);

    CHECK_DEC(AARCH64Decode(0xAA0103E0, &instruction), ARMITO_DECODER_SUCCESS);
    CHECK(instruction.opcode == AARCH64_OPCODE_MOV_REG);
    CHECK(instruction.rd_index == 0);
    CHECK(instruction.rm_index == 1);
    CHECK(instruction.rn_index == 31);
    CHECK(instruction.rn_is_zero == 1);

    CHECK_DEC(AARCH64Decode(0x91000420, &instruction), ARMITO_DECODER_SUCCESS);
    CHECK(instruction.opcode == AARCH64_OPCODE_ADD_IMM);
    CHECK(instruction.width_bits == 64);
    CHECK(instruction.rd_index == 0);
    CHECK(instruction.rn_index == 1);
    CHECK(instruction.immediate == 1);

    CHECK_DEC(AARCH64Decode(0x8B020020, &instruction), ARMITO_DECODER_SUCCESS);
    CHECK(instruction.opcode == AARCH64_OPCODE_ADD_SHIFTED);
    CHECK(instruction.rd_index == 0);
    CHECK(instruction.rn_index == 1);
    CHECK(instruction.rm_index == 2);

    CHECK_DEC(AARCH64Decode(0xD65F03C0, &instruction), ARMITO_DECODER_SUCCESS);
    CHECK(instruction.opcode == AARCH64_OPCODE_RET);
    CHECK(instruction.rn_index == 30);
}

static void TestSingleInstruction(void)
{
    ArmitoDecoderConfig config = MakeConfig();
    ArmitoDecoderInstruction instruction;
    ArmitoDecodeResult result;

    current_test = "decodificacao de instrucao unica";

    CHECK_DEC(ArmitoDecoderInitialize(&config), ARMITO_DECODER_SUCCESS);

    CHECK_DEC(
        ArmitoDecoderSourceSet(program, sizeof(program), 0x1000, 1, 1),
        ARMITO_DECODER_SUCCESS
    );

    CHECK_DEC(
        ArmitoDecoderSourceFetch(0, &instruction), ARMITO_DECODER_SUCCESS
    );
    CHECK_DEC(
        ArmitoDecoderDecodeInstruction(&instruction, &result),
        ARMITO_DECODER_SUCCESS
    );
    CHECK(result == ARMITO_DECODE_OK);
    CHECK(ArmitoIROperationCount() == 1);

    CHECK_DEC(
        ArmitoDecoderSourceFetch(4, &instruction), ARMITO_DECODER_SUCCESS
    );
    CHECK_DEC(
        ArmitoDecoderDecodeInstruction(&instruction, &result),
        ARMITO_DECODER_UNSUPPORTED_INSTRUCTION
    );
    CHECK(result == ARMITO_DECODE_UNSUPPORTED);
    CHECK(ArmitoIROperationCount() == 2);

    {
        ArmitoIROperation operation;

        CHECK(ArmitoIROperationGet(2, &operation) == ARMITO_IR_SUCCESS);
        CHECK(operation.kind == ARMITO_IR_OP_UNSUPPORTED);
        CHECK(operation.source_encoding == 0xD2800000);
        CHECK(operation.source_length == 4);
    }

    {
        ArmitoDecoderStats stats;

        CHECK_DEC(ArmitoDecoderGetStats(&stats), ARMITO_DECODER_SUCCESS);
        CHECK(stats.instructions_seen == 2);
        CHECK(stats.instructions_decoded == 1);
        CHECK(stats.instructions_unsupported == 1);
        CHECK(stats.operations_emitted == 2);
        CHECK(stats.last_encoding == 0xD2800000);
    }

    CHECK_DEC(
        ArmitoDecoderDecodeInstruction(NULL, &result),
        ARMITO_DECODER_INVALID_ARGUMENT
    );
    CHECK_DEC(
        ArmitoDecoderDecodeInstruction(&instruction, NULL),
        ARMITO_DECODER_INVALID_ARGUMENT
    );
}

static void TestBlockDecoding(void)
{
    ArmitoDecoderConfig config = MakeConfig();
    ArmitoDecoderSummary summary;

    current_test = "decodificacao de bloco";

    CHECK_DEC(ArmitoDecoderInitialize(&config), ARMITO_DECODER_SUCCESS);

    CHECK_DEC(
        ArmitoDecoderSourceSet(program, sizeof(program), 0x1000, 1, 1),
        ARMITO_DECODER_SUCCESS
    );

    CHECK_DEC(
        ArmitoDecoderDecodeBlock(0x1000, &summary), ARMITO_DECODER_SUCCESS
    );

    CHECK(summary.isa == ARMITO_ISA_AARCH64);
    CHECK(summary.instructions_decoded == 2);
    CHECK(summary.instructions_unsupported == 2);
    CHECK(summary.blocks_emitted == 1);
    CHECK(summary.next_address == 0x1010);
    CHECK(summary.last_result == ARMITO_DECODE_OK);
    CHECK(ArmitoIRBlockCount() == 1);

    CHECK_DEC(
        ArmitoDecoderDecodeBlock(0x2000, &summary),
        ARMITO_DECODER_OUT_OF_RANGE
    );
    CHECK_DEC(
        ArmitoDecoderDecodeBlock(0x1002, &summary),
        ARMITO_DECODER_INVALID_ARGUMENT
    );

    CHECK_DEC(ArmitoDecoderSourceClear(), ARMITO_DECODER_SUCCESS);
    CHECK_DEC(
        ArmitoDecoderDecodeBlock(0x1000, &summary),
        ARMITO_DECODER_INVALID_STATE
    );

    CHECK_DEC(
        ArmitoDecoderDecodeBlock(0x1000, NULL),
        ARMITO_DECODER_INVALID_ARGUMENT
    );

    CHECK_DEC(ArmitoDecoderInitialize(&config), ARMITO_DECODER_SUCCESS);
    CHECK_DEC(
        ArmitoDecoderSourceSet(program, 2, 0x3000, 1, 1),
        ARMITO_DECODER_SUCCESS
    );
    CHECK_DEC(
        ArmitoDecoderDecodeBlock(0x3000, &summary),
        ARMITO_DECODER_TRUNCATED
    );
    CHECK(summary.last_result == ARMITO_DECODE_INVALID);

    {
        static const uint8_t invalid_branch[] = { 0x0F, 0x00, 0x00, 0x54 };

        CHECK_DEC(ArmitoDecoderInitialize(&config), ARMITO_DECODER_SUCCESS);
        CHECK_DEC(
            ArmitoDecoderSourceSet(invalid_branch, sizeof(invalid_branch), 0x4000, 1, 1),
            ARMITO_DECODER_SUCCESS
        );
        CHECK_DEC(
            ArmitoDecoderDecodeBlock(0x4000, &summary),
            ARMITO_DECODER_INVALID_INSTRUCTION
        );
        CHECK(summary.instructions_invalid == 1);
    }
}

static void TestStopAtBranch(void)
{
    ArmitoDecoderConfig config = MakeConfig();
    ArmitoDecoderSummary summary;

    current_test = "parada em desvio";

    config.max_instructions = 64;
    config.stop_at_branch = 1;

    CHECK_DEC(ArmitoDecoderInitialize(&config), ARMITO_DECODER_SUCCESS);

    {
        static const uint8_t loop[] = {
            0x1F, 0x20, 0x03, 0xD5,
            0x1F, 0x20, 0x03, 0xD5,
            0xFE, 0xFF, 0xFF, 0x17,
            0x1F, 0x20, 0x03, 0xD5
        };

        CHECK_DEC(
            ArmitoDecoderSourceSet(loop, sizeof(loop), 0x2000, 1, 1),
            ARMITO_DECODER_SUCCESS
        );

        CHECK_DEC(
            ArmitoDecoderDecodeBlock(0x2000, &summary), ARMITO_DECODER_SUCCESS
        );

        CHECK(summary.instructions_decoded == 3);
        CHECK(summary.next_address == 0x200C);
    }

    config.stop_at_branch = 0;

    CHECK_DEC(ArmitoDecoderInitialize(&config), ARMITO_DECODER_SUCCESS);

    {
        static const uint8_t loop[] = {
            0x1F, 0x20, 0x03, 0xD5,
            0x1F, 0x20, 0x03, 0xD5,
            0xFE, 0xFF, 0xFF, 0x17,
            0x1F, 0x20, 0x03, 0xD5
        };

        CHECK_DEC(
            ArmitoDecoderSourceSet(loop, sizeof(loop), 0x2000, 1, 1),
            ARMITO_DECODER_SUCCESS
        );

        CHECK_DEC(
            ArmitoDecoderDecodeBlock(0x2000, &summary), ARMITO_DECODER_SUCCESS
        );

        CHECK(summary.instructions_decoded == 4);
        CHECK(summary.next_address == 0x2010);
    }

    config.max_instructions = 2;

    CHECK_DEC(ArmitoDecoderInitialize(&config), ARMITO_DECODER_SUCCESS);

    CHECK_DEC(
        ArmitoDecoderSourceSet(program, sizeof(program), 0x3000, 1, 1),
        ARMITO_DECODER_SUCCESS
    );
    CHECK_DEC(
        ArmitoDecoderDecodeBlock(0x3000, &summary), ARMITO_DECODER_SUCCESS
    );
    CHECK(summary.instructions_decoded + summary.instructions_unsupported == 2);
}

static void TestIRContract(void)
{
    ArmitoDecoderConfig config = MakeConfig();
    ArmitoDecoderSummary summary;
    ArmitoIRBlock block;

    current_test = "contrato com a IR";

    CHECK_DEC(ArmitoDecoderInitialize(&config), ARMITO_DECODER_SUCCESS);

    CHECK_DEC(
        ArmitoDecoderSourceSet(program, sizeof(program), 0x1000, 1, 1),
        ARMITO_DECODER_SUCCESS
    );
    CHECK_DEC(
        ArmitoDecoderDecodeBlock(0x1000, &summary), ARMITO_DECODER_SUCCESS
    );

    CHECK(ArmitoIRTypeCount() == 3);
    CHECK(ArmitoIRCPURegisterCount() == 32);

    CHECK(ArmitoIRBlockGet(1, &block) == ARMITO_IR_SUCCESS);
    CHECK(block.guest_address == 0x1000);
    CHECK(block.operation_count == 4);
    {
        ArmitoIROperation operation;

        CHECK(ArmitoIROperationGet(4, &operation) == ARMITO_IR_SUCCESS);
        CHECK(operation.kind == ARMITO_IR_OP_RETURN);
        CHECK(operation.input_count == 1);
        CHECK(operation.inputs[0] != ARMITO_IR_VALUE_INVALID);
    }

    CHECK(ArmitoIRValidate() == ARMITO_IR_SUCCESS);
    CHECK(ArmitoIRFinalize() == ARMITO_IR_SUCCESS);
    CHECK(ArmitoIRGetState() == ARMITO_IR_FINALIZED);

    {
        ArmitoIRValueID x0 = ARMITO_IR_VALUE_INVALID;
        ArmitoIRValueID sp = ARMITO_IR_VALUE_INVALID;

        CHECK(
            ArmitoIRCPUStateLookup(ARMITO_IR_REG_GPR, 0, &x0) ==
            ARMITO_IR_SUCCESS
        );
        CHECK(x0 != ARMITO_IR_VALUE_INVALID);

        CHECK(
            ArmitoIRCPUStateLookup(ARMITO_IR_REG_STACK_POINTER, 31, &sp) ==
            ARMITO_IR_SUCCESS
        );
        CHECK(sp != ARMITO_IR_VALUE_INVALID);
        CHECK(sp != x0);
    }

    CHECK_DEC(
        ArmitoDecoderDecodeBlock(0x1000, &summary),
        ARMITO_DECODER_INVALID_STATE
    );
}

static void TestStatusNames(void)
{
    current_test = "nomes de status";

    CHECK(strcmp(ArmitoDecoderStatusName(ARMITO_DECODER_SUCCESS), "success") == 0);
    CHECK(strcmp(ArmitoDecoderStatusName(ARMITO_DECODER_TRUNCATED), "truncated") == 0);
    CHECK(
        strcmp(
            ArmitoDecoderStatusName(ARMITO_DECODER_UNSUPPORTED_INSTRUCTION),
            "unsupported-instruction"
        ) == 0
    );
    CHECK(strcmp(ArmitoDecoderStatusName((ArmitoDecoderStatus)99), "unknown") == 0);
    CHECK(strcmp(AARCH64OpcodeName(AARCH64_OPCODE_ADD_SHIFTED), "add.shifted") == 0);
    CHECK(strcmp(AARCH64OpcodeName(AARCH64_OPCODE_INVALID), "invalid") == 0);
}

int main(void)
{
    printf("Armito Decoder AArch64 - testes de host\n\n");

    TestLifecycle();
    TestSource();
    TestRegisters();
    TestClassification();
    TestOperandExtraction();
    TestSingleInstruction();
    TestBlockDecoding();
    TestStopAtBranch();
    TestIRContract();
    TestStatusNames();

    printf("\n%u verificacoes, %u falhas\n", tests_run, tests_failed);

    return (tests_failed == 0) ? 0 : 1;
}
