#include "Includes/IR.h"

static volatile uint32_t ir_module_state = ARMITO_IR_UNINITIALIZED;

static ArmitoIRStatus ArmitoIRValidatePointerAuthOperation(
    const ArmitoIROperation *operation
)
{
    uint8_t expected_inputs;
    uint8_t requires_result;

    switch (operation->kind) {
    case ARMITO_IR_OP_PAC_SIGN:
    case ARMITO_IR_OP_PAC_AUTHENTICATE:
    case ARMITO_IR_OP_PAC_BRANCH_AUTHENTICATED:
    case ARMITO_IR_OP_PAC_CALL_AUTHENTICATED:
    case ARMITO_IR_OP_PAC_RETURN_AUTHENTICATED:
    case ARMITO_IR_OP_PAC_LOAD_AUTHENTICATED:
        expected_inputs = 2;
        requires_result =
            (uint8_t)(operation->kind == ARMITO_IR_OP_PAC_SIGN ||
                      operation->kind == ARMITO_IR_OP_PAC_AUTHENTICATE ||
                      operation->kind == ARMITO_IR_OP_PAC_LOAD_AUTHENTICATED);
        break;
    case ARMITO_IR_OP_PAC_STRIP:
        expected_inputs = 1;
        requires_result = 1;
        break;
    case ARMITO_IR_OP_PAC_AUTHENTICATION_FAILURE:
        expected_inputs = 1;
        requires_result = 0;
        break;
    default:
        return ARMITO_IR_SUCCESS;
    }

    if (operation->input_count != expected_inputs)
        return ARMITO_IR_SEMANTIC_ERROR;

    if (requires_result && operation->result == ARMITO_IR_VALUE_INVALID)
        return ARMITO_IR_SEMANTIC_ERROR;

    if ((uint32_t)operation->pointer_auth_key >
        (uint32_t)ARMITO_IR_PAC_KEY_DB)
        return ARMITO_IR_SEMANTIC_ERROR;

    if ((operation->pointer_auth_flags & (uint8_t)~0x1Fu) != 0)
        return ARMITO_IR_SEMANTIC_ERROR;

    if (operation->has_effect_register &&
        operation->effect_register_class > ARMITO_IR_REG_STACK_POINTER)
        return ARMITO_IR_SEMANTIC_ERROR;

    return ARMITO_IR_SUCCESS;
}

ArmitoIRStatus ArmitoIRReset(void)
{
    ArmitoIRStatus status;

    status = ArmitoIRTypesReset();

    if (status != ARMITO_IR_SUCCESS)
        return status;

    status = ArmitoIRValuesReset();

    if (status != ARMITO_IR_SUCCESS)
        return status;

    status = ArmitoIROperationsReset();

    if (status != ARMITO_IR_SUCCESS)
        return status;

    status = ArmitoIRBlocksReset();

    if (status != ARMITO_IR_SUCCESS)
        return status;

    status = ArmitoIRCPUStateReset();

    if (status != ARMITO_IR_SUCCESS)
        return status;

    ir_module_state = ARMITO_IR_READY;

    return ARMITO_IR_SUCCESS;
}

ArmitoIRStatus ArmitoIRInitialize(void)
{
    return ArmitoIRReset();
}

ArmitoIRStatus ArmitoIRFinalize(void)
{
    ArmitoIRStatus status;

    if (ir_module_state != ARMITO_IR_READY && ir_module_state != ARMITO_IR_EMITTING)
        return ARMITO_IR_INVALID_STATE;

    status = ArmitoIRValidate();

    if (status != ARMITO_IR_SUCCESS) {
        ir_module_state = ARMITO_IR_FAILED;
        return status;
    }

    ir_module_state = ARMITO_IR_FINALIZED;

    return ARMITO_IR_SUCCESS;
}

ArmitoIRState ArmitoIRGetState(void)
{
    return (ArmitoIRState)ir_module_state;
}

ArmitoIRStatus ArmitoIRValidate(void)
{
    uint32_t i;
    uint32_t j;
    ArmitoIROperation operation;

    if (ir_module_state == ARMITO_IR_UNINITIALIZED)
        return ARMITO_IR_INVALID_STATE;

    for (i = 1; i <= ArmitoIROperationsCount(); i++) {
        if (ArmitoIROperationGet(i, &operation) != ARMITO_IR_SUCCESS)
            return ARMITO_IR_OUT_OF_RANGE;

        if (operation.kind == ARMITO_IR_OP_UNSUPPORTED)
            continue;

        if (operation.result_type != ARMITO_IR_TYPE_INVALID &&
            operation.result_type > ArmitoIRTypesCount())
            return ARMITO_IR_SEMANTIC_ERROR;

        if (operation.result != ARMITO_IR_VALUE_INVALID &&
            operation.result > ArmitoIRValuesCount())
            return ARMITO_IR_SEMANTIC_ERROR;

        for (j = 0; j < operation.input_count; j++) {
            if (operation.inputs[j] == ARMITO_IR_VALUE_INVALID ||
                operation.inputs[j] > ArmitoIRValuesCount())
                return ARMITO_IR_SEMANTIC_ERROR;
        }

        if (operation.kind == ARMITO_IR_OP_BRANCH ||
            operation.kind == ARMITO_IR_OP_BRANCH_CONDITIONAL) {
            if (operation.target != ARMITO_IR_LABEL_INVALID &&
                operation.target > ArmitoIRBlocksCount())
                return ARMITO_IR_SEMANTIC_ERROR;

            if (operation.target_false != ARMITO_IR_LABEL_INVALID &&
                operation.target_false > ArmitoIRBlocksCount())
                return ARMITO_IR_SEMANTIC_ERROR;
        }

        if (operation.kind == ARMITO_IR_OP_STORE && operation.input_count < 2)
            return ARMITO_IR_SEMANTIC_ERROR;

        if (operation.kind == ARMITO_IR_OP_LOAD && operation.input_count < 1)
            return ARMITO_IR_SEMANTIC_ERROR;

        if (operation.kind == ARMITO_IR_OP_ADD && operation.input_count != 2)
            return ARMITO_IR_SEMANTIC_ERROR;

        if (ArmitoIRValidatePointerAuthOperation(&operation) !=
            ARMITO_IR_SUCCESS)
            return ARMITO_IR_SEMANTIC_ERROR;
    }

    return ARMITO_IR_SUCCESS;
}

uint32_t ArmitoIRTypeCount(void)
{
    return ArmitoIRTypesCount();
}

uint32_t ArmitoIRValueCount(void)
{
    return ArmitoIRValuesCount();
}

uint32_t ArmitoIROperationCount(void)
{
    return ArmitoIROperationsCount();
}

uint32_t ArmitoIRBlockCount(void)
{
    return ArmitoIRBlocksCount();
}

uint32_t ArmitoIRCPURegisterCount(void)
{
    return ArmitoIRCPUStateCount();
}
