#include <stdio.h>
#include <stdint.h>
#include <string.h>

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

#define CHECK_IR(expr, expected) do { \
    ArmitoIRStatus s_ = (ArmitoIRStatus)(expr); \
    ArmitoIRStatus e_ = (ArmitoIRStatus)(expected); \
    tests_run++; \
    if (s_ != e_) { \
        tests_failed++; \
        printf("  FAIL  %s: %s devolveu %d, esperado %d (%s:%d)\n", \
            current_test, #expr, (int)s_, (int)e_, __FILE__, __LINE__); \
    } \
} while (0)

static void TestLifecycle(void)
{
    current_test = "ciclo de vida";

    CHECK(ArmitoIRGetState() == ARMITO_IR_UNINITIALIZED);

    CHECK_IR(ArmitoIRInitialize(), ARMITO_IR_SUCCESS);
    CHECK(ArmitoIRGetState() == ARMITO_IR_READY);
    CHECK_IR(ArmitoIRInitialize(), ARMITO_IR_SUCCESS);
    CHECK(ArmitoIRGetState() == ARMITO_IR_READY);
    CHECK_IR(ArmitoIRFinalize(), ARMITO_IR_SUCCESS);
    CHECK(ArmitoIRGetState() == ARMITO_IR_FINALIZED);
    CHECK_IR(ArmitoIRReset(), ARMITO_IR_SUCCESS);
    CHECK(ArmitoIRGetState() == ARMITO_IR_READY);
    CHECK(ArmitoIRTypeCount() == 0);
    CHECK(ArmitoIRValueCount() == 0);
    CHECK(ArmitoIROperationCount() == 0);
    CHECK(ArmitoIRBlockCount() == 0);
    CHECK(ArmitoIRCPURegisterCount() == 0);
}

static void TestTypeset(void)
{
    ArmitoIRTypeID u32_id = ARMITO_IR_TYPE_INVALID;
    ArmitoIRTypeID u32_lookup = ARMITO_IR_TYPE_INVALID;
    ArmitoIRTypeID void_id = ARMITO_IR_TYPE_INVALID;
    ArmitoIRType type;
    uint32_t size = 0;

    current_test = "conjunto de tipos";

    CHECK_IR(ArmitoIRInitialize(), ARMITO_IR_SUCCESS);

    CHECK_IR(
        ArmitoIRTypesRegister(ARMITO_IR_CLASS_INT, 32, 1, 0, 0, "u32", &u32_id),
        ARMITO_IR_SUCCESS
    );
    CHECK(u32_id == 1);

    CHECK_IR(
        ArmitoIRTypesRegister(ARMITO_IR_CLASS_VOID, 0, 0, 0, 0, "void", &void_id),
        ARMITO_IR_SUCCESS
    );
    CHECK(void_id == 2);

    CHECK_IR(
        ArmitoIRTypesLookup(ARMITO_IR_CLASS_INT, 32, 1, 0, &u32_lookup),
        ARMITO_IR_SUCCESS
    );
    CHECK(u32_lookup == u32_id);

    CHECK_IR(
        ArmitoIRTypesRegister(ARMITO_IR_CLASS_INT, 0, 1, 0, 0, "bad", &u32_id),
        ARMITO_IR_INVALID_ARGUMENT
    );

    CHECK_IR(
        ArmitoIRTypesRegister(ARMITO_IR_CLASS_INT, 32, 1, 0, 0, "dup", &u32_id),
        ARMITO_IR_SUCCESS
    );
    CHECK(u32_id == 3);

    CHECK_IR(ArmitoIRTypesGet(u32_id, &type), ARMITO_IR_SUCCESS);
    CHECK(type.bit_width == 32);
    CHECK(type.lane_count == 1);

    CHECK_IR(ArmitoIRTypesSize(u32_id, &size), ARMITO_IR_SUCCESS);
    CHECK(size == 4);

    CHECK_IR(ArmitoIRTypesSize(void_id, &size), ARMITO_IR_SUCCESS);
    CHECK(size == 0);

    CHECK_IR(ArmitoIRTypesGet(0, &type), ARMITO_IR_OUT_OF_RANGE);
    CHECK_IR(ArmitoIRTypesGet(99, &type), ARMITO_IR_OUT_OF_RANGE);
    CHECK(ArmitoIRTypesCount() == 3);

    {
        uint32_t i;
        ArmitoIRTypeID ignored = ARMITO_IR_TYPE_INVALID;
        ArmitoIRStatus last = ARMITO_IR_SUCCESS;

        for (i = 0; i < ARMITO_IR_MAX_TYPES; i++)
            last = ArmitoIRTypesRegister(
                ARMITO_IR_CLASS_INT, 8, 1, 0, 0, "fill", &ignored
            );

        CHECK(last == ARMITO_IR_OUT_OF_RESOURCES);
    }
}

static void TestValues(void)
{
    ArmitoIRTypeID u32_id = ARMITO_IR_TYPE_INVALID;
    ArmitoIRValueID reg = ARMITO_IR_VALUE_INVALID;
    ArmitoIRValueID imm = ARMITO_IR_VALUE_INVALID;
    ArmitoIRValueID mem = ARMITO_IR_VALUE_INVALID;
    ArmitoIRValue value;

    current_test = "valores";

    CHECK_IR(ArmitoIRInitialize(), ARMITO_IR_SUCCESS);

    CHECK_IR(
        ArmitoIRTypesRegister(ARMITO_IR_CLASS_INT, 32, 1, 0, 0, "u32", &u32_id),
        ARMITO_IR_SUCCESS
    );

    CHECK_IR(
        ArmitoIRValuesDeclare(ARMITO_IR_VALUE_VIRTUAL_REGISTER, u32_id, "v0", &reg),
        ARMITO_IR_SUCCESS
    );
    CHECK(reg == 1);

    CHECK_IR(
        ArmitoIRValuesDeclare(ARMITO_IR_VALUE_IMMEDIATE, u32_id, "imm", &imm),
        ARMITO_IR_SUCCESS
    );

    CHECK_IR(
        ArmitoIRValuesDeclare(ARMITO_IR_VALUE_MEMORY, u32_id, "mem", &mem),
        ARMITO_IR_SUCCESS
    );

    CHECK_IR(ArmitoIRValuesSetImmediate(imm, 42), ARMITO_IR_SUCCESS);
    CHECK_IR(ArmitoIRValuesGet(imm, &value), ARMITO_IR_SUCCESS);
    CHECK(value.immediate == 42);
    CHECK(value.kind == ARMITO_IR_VALUE_IMMEDIATE);

    CHECK_IR(ArmitoIRValuesSetImmediate(reg, 7), ARMITO_IR_INVALID_STATE);

    CHECK_IR(
        ArmitoIRValuesSetMemory(mem, ARMITO_IR_MEM_LOAD, reg, -8),
        ARMITO_IR_SUCCESS
    );
    CHECK_IR(ArmitoIRValuesGet(mem, &value), ARMITO_IR_SUCCESS);
    CHECK(value.memory_kind == ARMITO_IR_MEM_LOAD);
    CHECK(value.address == reg);
    CHECK(value.displacement == -8);

    CHECK_IR(
        ArmitoIRValuesSetMemory(mem, ARMITO_IR_MEM_NONE, reg, 0),
        ARMITO_IR_INVALID_ARGUMENT
    );

    CHECK_IR(ArmitoIRValuesGet(0, &value), ARMITO_IR_OUT_OF_RANGE);
    CHECK_IR(ArmitoIRValuesGet(99, &value), ARMITO_IR_OUT_OF_RANGE);
    CHECK(ArmitoIRValuesCount() == 3);
}

static void TestOperations(void)
{
    ArmitoIRTypeID u32_id = ARMITO_IR_TYPE_INVALID;
    ArmitoIRTypeID flags_id = ARMITO_IR_TYPE_INVALID;
    ArmitoIRValueID a = ARMITO_IR_VALUE_INVALID;
    ArmitoIRValueID b = ARMITO_IR_VALUE_INVALID;
    ArmitoIRValueID result = ARMITO_IR_VALUE_INVALID;
    ArmitoIROperationID add_id = 0;
    ArmitoIROperationID cmp_id = 0;
    ArmitoIROperation operation;
    ArmitoIRValueID inputs[2];

    current_test = "operacoes";

    CHECK_IR(ArmitoIRInitialize(), ARMITO_IR_SUCCESS);

    CHECK_IR(
        ArmitoIRTypesRegister(ARMITO_IR_CLASS_INT, 32, 1, 0, 0, "u32", &u32_id),
        ARMITO_IR_SUCCESS
    );
    CHECK_IR(
        ArmitoIRTypesRegister(ARMITO_IR_CLASS_FLAGS, 4, 1, 0, 0, "flags", &flags_id),
        ARMITO_IR_SUCCESS
    );

    CHECK_IR(
        ArmitoIRValuesDeclare(ARMITO_IR_VALUE_VIRTUAL_REGISTER, u32_id, "a", &a),
        ARMITO_IR_SUCCESS
    );
    CHECK_IR(
        ArmitoIRValuesDeclare(ARMITO_IR_VALUE_VIRTUAL_REGISTER, u32_id, "b", &b),
        ARMITO_IR_SUCCESS
    );
    CHECK_IR(
        ArmitoIRValuesDeclare(ARMITO_IR_VALUE_VIRTUAL_REGISTER, u32_id, "r", &result),
        ARMITO_IR_SUCCESS
    );

    inputs[0] = a;
    inputs[1] = b;

    CHECK_IR(
        ArmitoIROperationEmit(
            ARMITO_IR_OP_ADD, u32_id, result, inputs, 2, &add_id
        ),
        ARMITO_IR_SUCCESS
    );
    CHECK(add_id == 1);

    CHECK_IR(
        ArmitoIROperationSetFlags(add_id, ARMITO_IR_FLAG_N | ARMITO_IR_FLAG_Z),
        ARMITO_IR_SUCCESS
    );
    CHECK_IR(
        ArmitoIROperationSetSource(add_id, 0x10, 0x8B020020, 4),
        ARMITO_IR_SUCCESS
    );

    CHECK_IR(ArmitoIROperationGet(add_id, &operation), ARMITO_IR_SUCCESS);
    CHECK(operation.kind == ARMITO_IR_OP_ADD);
    CHECK(operation.input_count == 2);
    CHECK(operation.inputs[0] == a);
    CHECK(operation.inputs[1] == b);
    CHECK(operation.result == result);
    CHECK(operation.flag_mask == (ARMITO_IR_FLAG_N | ARMITO_IR_FLAG_Z));
    CHECK(operation.source_encoding == 0x8B020020);
    CHECK(operation.source_length == 4);

    CHECK_IR(
        ArmitoIROperationEmit(
            ARMITO_IR_OP_CMP_LT_SIGNED, flags_id, ARMITO_IR_VALUE_INVALID,
            inputs, 2, &cmp_id
        ),
        ARMITO_IR_SUCCESS
    );
    CHECK(cmp_id == 2);

    CHECK_IR(ArmitoIROperationSetFlags(add_id, 0xFF), ARMITO_IR_INVALID_ARGUMENT);
    CHECK_IR(ArmitoIROperationGet(0, &operation), ARMITO_IR_OUT_OF_RANGE);
    CHECK_IR(ArmitoIROperationGet(99, &operation), ARMITO_IR_OUT_OF_RANGE);

    CHECK_IR(
        ArmitoIROperationEmit(
            ARMITO_IR_OP_NOP, ARMITO_IR_TYPE_INVALID, ARMITO_IR_VALUE_INVALID,
            NULL, 0, &add_id
        ),
        ARMITO_IR_SUCCESS
    );

    {
        ArmitoIRValueID bad[1];
        ArmitoIROperationID ignored = 0;

        bad[0] = 99;

        CHECK_IR(
            ArmitoIROperationEmit(
                ARMITO_IR_OP_MOVE, u32_id, ARMITO_IR_VALUE_INVALID,
                bad, 1, &ignored
            ),
            ARMITO_IR_OUT_OF_RANGE
        );
    }

    CHECK(strcmp(ArmitoIROperationName(ARMITO_IR_OP_ADD), "add") == 0);
    CHECK(strcmp(ArmitoIROperationName(ARMITO_IR_OP_BRANCH_CONDITIONAL), "branch.cond") == 0);
    CHECK(strcmp(ArmitoIROperationName((ArmitoIROperationKind)999), "invalid") == 0);
}

static void TestBlocks(void)
{
    ArmitoIRTypeID u32_id = ARMITO_IR_TYPE_INVALID;
    ArmitoIRValueID a = ARMITO_IR_VALUE_INVALID;
    ArmitoIRValueID b = ARMITO_IR_VALUE_INVALID;
    ArmitoIRLabelID block1 = ARMITO_IR_LABEL_INVALID;
    ArmitoIRLabelID block2 = ARMITO_IR_LABEL_INVALID;
    ArmitoIROperationID op_id = 0;
    ArmitoIRBlock block;
    ArmitoIRValueID inputs[2];

    current_test = "blocos basicos";

    CHECK_IR(ArmitoIRInitialize(), ARMITO_IR_SUCCESS);

    CHECK_IR(
        ArmitoIRTypesRegister(ARMITO_IR_CLASS_INT, 64, 1, 0, 0, "u64", &u32_id),
        ARMITO_IR_SUCCESS
    );
    CHECK_IR(
        ArmitoIRValuesDeclare(ARMITO_IR_VALUE_VIRTUAL_REGISTER, u32_id, "a", &a),
        ARMITO_IR_SUCCESS
    );
    CHECK_IR(
        ArmitoIRValuesDeclare(ARMITO_IR_VALUE_VIRTUAL_REGISTER, u32_id, "b", &b),
        ARMITO_IR_SUCCESS
    );

    CHECK_IR(ArmitoIRBlockBegin(0x1000, "entry", &block1), ARMITO_IR_SUCCESS);
    CHECK(block1 == 1);
    CHECK_IR(ArmitoIRBlockBegin(0x2000, "nested", &block2), ARMITO_IR_INVALID_STATE);

    inputs[0] = a;
    inputs[1] = b;

    CHECK_IR(
        ArmitoIROperationEmit(
            ARMITO_IR_OP_ADD, u32_id, a, inputs, 2, &op_id
        ),
        ARMITO_IR_SUCCESS
    );
    CHECK_IR(
        ArmitoIROperationEmit(
            ARMITO_IR_OP_SUB, u32_id, b, inputs, 2, &op_id
        ),
        ARMITO_IR_SUCCESS
    );

    CHECK_IR(ArmitoIRBlockEnd(), ARMITO_IR_SUCCESS);
    CHECK_IR(ArmitoIRBlockEnd(), ARMITO_IR_INVALID_STATE);

    CHECK_IR(ArmitoIRBlockGet(block1, &block), ARMITO_IR_SUCCESS);
    CHECK(block.guest_address == 0x1000);
    CHECK(block.operation_count == 2);
    CHECK(block.first_operation == 0);
    CHECK(ArmitoIRBlockOperationCount(block1) == 2);

    CHECK_IR(ArmitoIRBlockBegin(0x2000, "exit", &block2), ARMITO_IR_SUCCESS);
    CHECK(block2 == 2);
    CHECK_IR(ArmitoIRBlockEnd(), ARMITO_IR_SUCCESS);

    CHECK_IR(ArmitoIRBlockGet(block2, &block), ARMITO_IR_SUCCESS);
    CHECK(block.first_operation == 2);
    CHECK(block.operation_count == 0);

    CHECK_IR(ArmitoIRBlockGet(0, &block), ARMITO_IR_OUT_OF_RANGE);
    CHECK_IR(ArmitoIRBlockGet(99, &block), ARMITO_IR_OUT_OF_RANGE);
    CHECK(ArmitoIRBlocksCount() == 2);
}

static void TestCPUState(void)
{
    ArmitoIRTypeID u64_id = ARMITO_IR_TYPE_INVALID;
    ArmitoIRValueID x0 = ARMITO_IR_VALUE_INVALID;
    ArmitoIRValueID x1 = ARMITO_IR_VALUE_INVALID;
    ArmitoIRValueID found = ARMITO_IR_VALUE_INVALID;
    ArmitoIRCPURegister reg;

    current_test = "estado da CPU";

    CHECK_IR(ArmitoIRInitialize(), ARMITO_IR_SUCCESS);

    CHECK_IR(
        ArmitoIRTypesRegister(ARMITO_IR_CLASS_INT, 64, 1, 0, 0, "u64", &u64_id),
        ARMITO_IR_SUCCESS
    );
    CHECK_IR(
        ArmitoIRValuesDeclare(ARMITO_IR_VALUE_VIRTUAL_REGISTER, u64_id, "x0", &x0),
        ARMITO_IR_SUCCESS
    );
    CHECK_IR(
        ArmitoIRValuesDeclare(ARMITO_IR_VALUE_VIRTUAL_REGISTER, u64_id, "x1", &x1),
        ARMITO_IR_SUCCESS
    );

    CHECK_IR(
        ArmitoIRCPUStateMap(ARMITO_IR_REG_GPR, 0, 64, "x0", x0),
        ARMITO_IR_SUCCESS
    );
    CHECK_IR(
        ArmitoIRCPUStateMap(ARMITO_IR_REG_GPR, 1, 64, "x1", x1),
        ARMITO_IR_SUCCESS
    );
    CHECK_IR(
        ArmitoIRCPUStateMap(ARMITO_IR_REG_GPR, 0, 64, "x0-dup", x1),
        ARMITO_IR_INVALID_STATE
    );

    CHECK_IR(
        ArmitoIRCPUStateLookup(ARMITO_IR_REG_GPR, 1, &found),
        ARMITO_IR_SUCCESS
    );
    CHECK(found == x1);

    CHECK_IR(
        ArmitoIRCPUStateLookup(ARMITO_IR_REG_GPR, 5, &found),
        ARMITO_IR_NOT_SUPPORTED
    );
    CHECK(found == ARMITO_IR_VALUE_INVALID);

    CHECK_IR(ArmitoIRCPUStateGet(0, &reg), ARMITO_IR_SUCCESS);
    CHECK(reg.index == 0);
    CHECK(reg.reg_class == ARMITO_IR_REG_GPR);
    CHECK(reg.bit_width == 64);
    CHECK(strcmp(reg.source_name, "x0") == 0);

    CHECK_IR(ArmitoIRCPUStateGet(99, &reg), ARMITO_IR_OUT_OF_RANGE);
    CHECK(ArmitoIRCPURegisterCount() == 2);
}

static void TestValidate(void)
{
    ArmitoIRTypeID u32_id = ARMITO_IR_TYPE_INVALID;
    ArmitoIRValueID a = ARMITO_IR_VALUE_INVALID;
    ArmitoIRValueID b = ARMITO_IR_VALUE_INVALID;
    ArmitoIRValueID r = ARMITO_IR_VALUE_INVALID;
    ArmitoIROperationID op_id = 0;
    ArmitoIRValueID inputs[2];

    current_test = "validacao";

    CHECK_IR(ArmitoIRInitialize(), ARMITO_IR_SUCCESS);
    CHECK_IR(ArmitoIRValidate(), ARMITO_IR_SUCCESS);

    CHECK_IR(
        ArmitoIRTypesRegister(ARMITO_IR_CLASS_INT, 32, 1, 0, 0, "u32", &u32_id),
        ARMITO_IR_SUCCESS
    );
    CHECK_IR(
        ArmitoIRValuesDeclare(ARMITO_IR_VALUE_VIRTUAL_REGISTER, u32_id, "a", &a),
        ARMITO_IR_SUCCESS
    );
    CHECK_IR(
        ArmitoIRValuesDeclare(ARMITO_IR_VALUE_VIRTUAL_REGISTER, u32_id, "b", &b),
        ARMITO_IR_SUCCESS
    );
    CHECK_IR(
        ArmitoIRValuesDeclare(ARMITO_IR_VALUE_VIRTUAL_REGISTER, u32_id, "r", &r),
        ARMITO_IR_SUCCESS
    );

    inputs[0] = a;
    inputs[1] = b;

    CHECK_IR(
        ArmitoIROperationEmit(ARMITO_IR_OP_ADD, u32_id, r, inputs, 2, &op_id),
        ARMITO_IR_SUCCESS
    );
    CHECK_IR(ArmitoIRValidate(), ARMITO_IR_SUCCESS);

    CHECK_IR(
        ArmitoIROperationEmit(ARMITO_IR_OP_ADD, u32_id, r, inputs, 1, &op_id),
        ARMITO_IR_SUCCESS
    );
    CHECK_IR(ArmitoIRValidate(), ARMITO_IR_SEMANTIC_ERROR);

    CHECK_IR(ArmitoIRReset(), ARMITO_IR_SUCCESS);

    CHECK_IR(
        ArmitoIRTypesRegister(ARMITO_IR_CLASS_INT, 64, 1, 0, 0, "u64", &u32_id),
        ARMITO_IR_SUCCESS
    );
    CHECK_IR(
        ArmitoIRValuesDeclare(ARMITO_IR_VALUE_VIRTUAL_REGISTER, u32_id, "a", &a),
        ARMITO_IR_SUCCESS
    );
    CHECK_IR(
        ArmitoIRValuesDeclare(ARMITO_IR_VALUE_VIRTUAL_REGISTER, u32_id, "b", &b),
        ARMITO_IR_SUCCESS
    );
    inputs[0] = a;
    inputs[1] = b;

    CHECK_IR(
        ArmitoIROperationEmit(
            ARMITO_IR_OP_BRANCH_CONDITIONAL, ARMITO_IR_TYPE_INVALID,
            ARMITO_IR_VALUE_INVALID, inputs, 2, &op_id
        ),
        ARMITO_IR_SUCCESS
    );
    CHECK_IR(ArmitoIROperationSetTarget(op_id, 9, ARMITO_IR_LABEL_INVALID), ARMITO_IR_SUCCESS);
    CHECK_IR(ArmitoIRValidate(), ARMITO_IR_SEMANTIC_ERROR);

    CHECK_IR(ArmitoIRFinalize(), ARMITO_IR_SEMANTIC_ERROR);
    CHECK(ArmitoIRGetState() == ARMITO_IR_FAILED);
}

static void TestResetClearsEverything(void)
{
    ArmitoIRTypeID u32_id = ARMITO_IR_TYPE_INVALID;
    ArmitoIRValueID v = ARMITO_IR_VALUE_INVALID;
    ArmitoIRLabelID label = ARMITO_IR_LABEL_INVALID;
    ArmitoIROperationID op_id = 0;

    current_test = "reset limpa tudo";

    CHECK_IR(ArmitoIRInitialize(), ARMITO_IR_SUCCESS);

    CHECK_IR(
        ArmitoIRTypesRegister(ARMITO_IR_CLASS_INT, 32, 1, 0, 0, "u32", &u32_id),
        ARMITO_IR_SUCCESS
    );
    CHECK_IR(
        ArmitoIRValuesDeclare(ARMITO_IR_VALUE_VIRTUAL_REGISTER, u32_id, "v", &v),
        ARMITO_IR_SUCCESS
    );
    CHECK_IR(
        ArmitoIROperationEmit(
            ARMITO_IR_OP_NOP, ARMITO_IR_TYPE_INVALID, ARMITO_IR_VALUE_INVALID,
            NULL, 0, &op_id
        ),
        ARMITO_IR_SUCCESS
    );
    CHECK_IR(ArmitoIRBlockBegin(0x0, "b", &label), ARMITO_IR_SUCCESS);
    CHECK_IR(ArmitoIRBlockEnd(), ARMITO_IR_SUCCESS);
    CHECK_IR(ArmitoIRCPUStateMap(ARMITO_IR_REG_GPR, 0, 32, "v", v), ARMITO_IR_SUCCESS);

    CHECK(ArmitoIRTypeCount() == 1);
    CHECK(ArmitoIRValueCount() == 1);
    CHECK(ArmitoIROperationCount() == 1);
    CHECK(ArmitoIRBlockCount() == 1);
    CHECK(ArmitoIRCPURegisterCount() == 1);

    CHECK_IR(ArmitoIRReset(), ARMITO_IR_SUCCESS);

    CHECK(ArmitoIRTypeCount() == 0);
    CHECK(ArmitoIRValueCount() == 0);
    CHECK(ArmitoIROperationCount() == 0);
    CHECK(ArmitoIRBlockCount() == 0);
    CHECK(ArmitoIRCPURegisterCount() == 0);
    CHECK(ArmitoIRGetState() == ARMITO_IR_READY);
}

static void TestLimits(void)
{
    ArmitoIRValueID ignored = ARMITO_IR_VALUE_INVALID;
    ArmitoIRStatus last = ARMITO_IR_SUCCESS;
    uint32_t i;

    current_test = "limites estaticos";

    CHECK_IR(ArmitoIRInitialize(), ARMITO_IR_SUCCESS);

    for (i = 0; i < ARMITO_IR_MAX_VALUES; i++)
        last = ArmitoIRValuesDeclare(
            ARMITO_IR_VALUE_VIRTUAL_REGISTER, ARMITO_IR_TYPE_INVALID, "v", &ignored
        );

    CHECK(last == ARMITO_IR_SUCCESS);
    CHECK(ArmitoIRValuesCount() == ARMITO_IR_MAX_VALUES);

    CHECK_IR(
        ArmitoIRValuesDeclare(
            ARMITO_IR_VALUE_VIRTUAL_REGISTER, ARMITO_IR_TYPE_INVALID, "v", &ignored
        ),
        ARMITO_IR_OUT_OF_RESOURCES
    );

    {
        ArmitoIROperationID op_id = 0;

        last = ARMITO_IR_SUCCESS;

        for (i = 0; i < ARMITO_IR_MAX_OPERATIONS; i++)
            last = ArmitoIROperationEmit(
                ARMITO_IR_OP_NOP, ARMITO_IR_TYPE_INVALID, ARMITO_IR_VALUE_INVALID,
                NULL, 0, &op_id
            );

        CHECK(last == ARMITO_IR_SUCCESS);
        CHECK(ArmitoIROperationsCount() == ARMITO_IR_MAX_OPERATIONS);

        CHECK_IR(
            ArmitoIROperationEmit(
                ARMITO_IR_OP_NOP, ARMITO_IR_TYPE_INVALID, ARMITO_IR_VALUE_INVALID,
                NULL, 0, &op_id
            ),
            ARMITO_IR_OUT_OF_RESOURCES
        );
    }
}

int main(void)
{
    printf("Armito IR - testes de host\n\n");

    TestLifecycle();
    TestResetClearsEverything();
    TestTypeset();
    TestValues();
    TestOperations();
    TestBlocks();
    TestCPUState();
    TestValidate();
    TestLimits();

    printf("\n%u verificacoes, %u falhas\n", tests_run, tests_failed);

    return (tests_failed == 0) ? 0 : 1;
}
