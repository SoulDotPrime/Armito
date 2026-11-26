#include "Includes/AARCH64CPUState.h"

AARCH64CPUStateStatus AARCH64CPUStateReset(AARCH64CPUState *state)
{
    uint32_t i;

    if (state == 0)
        return AARCH64_CPU_STATE_INVALID_ARGUMENT;

    for (i = 0; i < AARCH64_CPU_GPR_COUNT; i++)
        state->x[i] = 0;

    state->sp = 0;
    state->pc = 0;
    state->nzcv = 0;
    state->fpcr = 0;
    state->fpsr = 0;

    for (i = 0; i < AARCH64_CPU_VECTOR_COUNT; i++) {
        state->v[i].low = 0;
        state->v[i].high = 0;
    }

    state->exception.pending = 0;
    state->exception.code = 0;
    state->exception.pc = 0;
    state->exception.address = 0;
    state->exception.syndrome = 0;

    state->pointer_auth.enabled = 0;
    state->pointer_auth.api_a.low = 0;
    state->pointer_auth.api_a.high = 0;
    state->pointer_auth.api_b.low = 0;
    state->pointer_auth.api_b.high = 0;
    state->pointer_auth.apda.low = 0;
    state->pointer_auth.apda.high = 0;
    state->pointer_auth.apdb.low = 0;
    state->pointer_auth.apdb.high = 0;
    state->pointer_auth.apga.low = 0;
    state->pointer_auth.apga.high = 0;

    return AARCH64_CPU_STATE_SUCCESS;
}

static AARCH64CPUStateStatus AARCH64CPUStateValidateGPR(
    uint8_t index,
    uint8_t width_bits,
    AARCH64GPR31Behavior index31_behavior
)
{
    if (index > AARCH64_CPU_GPR_COUNT)
        return AARCH64_CPU_STATE_OUT_OF_RANGE;

    if (width_bits != 32 && width_bits != 64)
        return AARCH64_CPU_STATE_INVALID_ARGUMENT;

    if (index31_behavior != AARCH64_GPR31_IS_ZERO &&
        index31_behavior != AARCH64_GPR31_IS_SP)
        return AARCH64_CPU_STATE_INVALID_ARGUMENT;

    return AARCH64_CPU_STATE_SUCCESS;
}

AARCH64CPUStateStatus AARCH64CPUStateReadGPR(
    const AARCH64CPUState *state,
    uint8_t index,
    uint8_t width_bits,
    AARCH64GPR31Behavior index31_behavior,
    uint64_t *value
)
{
    AARCH64CPUStateStatus status;
    uint64_t result;

    if (state == 0 || value == 0)
        return AARCH64_CPU_STATE_INVALID_ARGUMENT;

    status = AARCH64CPUStateValidateGPR(index, width_bits, index31_behavior);
    if (status != AARCH64_CPU_STATE_SUCCESS)
        return status;

    if (index == AARCH64_CPU_GPR_COUNT) {
        result = index31_behavior == AARCH64_GPR31_IS_SP ? state->sp : 0;
    } else {
        result = state->x[index];
    }

    *value = width_bits == 32 ? (uint32_t)result : result;
    return AARCH64_CPU_STATE_SUCCESS;
}

AARCH64CPUStateStatus AARCH64CPUStateWriteGPR(
    AARCH64CPUState *state,
    uint8_t index,
    uint8_t width_bits,
    AARCH64GPR31Behavior index31_behavior,
    uint64_t value
)
{
    AARCH64CPUStateStatus status;
    uint64_t result = width_bits == 32 ? (uint32_t)value : value;

    if (state == 0)
        return AARCH64_CPU_STATE_INVALID_ARGUMENT;

    status = AARCH64CPUStateValidateGPR(index, width_bits, index31_behavior);
    if (status != AARCH64_CPU_STATE_SUCCESS)
        return status;

    if (index == AARCH64_CPU_GPR_COUNT) {
        if (index31_behavior == AARCH64_GPR31_IS_SP)
            state->sp = result;
        return AARCH64_CPU_STATE_SUCCESS;
    }

    state->x[index] = result;
    return AARCH64_CPU_STATE_SUCCESS;
}

AARCH64CPUStateStatus AARCH64CPUStateReadPC(
    const AARCH64CPUState *state,
    uint64_t *value
)
{
    if (state == 0 || value == 0)
        return AARCH64_CPU_STATE_INVALID_ARGUMENT;

    *value = state->pc;
    return AARCH64_CPU_STATE_SUCCESS;
}

AARCH64CPUStateStatus AARCH64CPUStateWritePC(
    AARCH64CPUState *state,
    uint64_t value
)
{
    if (state == 0)
        return AARCH64_CPU_STATE_INVALID_ARGUMENT;

    state->pc = value;
    return AARCH64_CPU_STATE_SUCCESS;
}

AARCH64CPUStateStatus AARCH64CPUStateReadNZCV(
    const AARCH64CPUState *state,
    uint8_t *value
)
{
    if (state == 0 || value == 0)
        return AARCH64_CPU_STATE_INVALID_ARGUMENT;

    *value = state->nzcv;
    return AARCH64_CPU_STATE_SUCCESS;
}

AARCH64CPUStateStatus AARCH64CPUStateWriteNZCV(
    AARCH64CPUState *state,
    uint8_t value
)
{
    if (state == 0)
        return AARCH64_CPU_STATE_INVALID_ARGUMENT;

    if ((value & (uint8_t)~AARCH64_NZCV_MASK) != 0)
        return AARCH64_CPU_STATE_INVALID_ARGUMENT;

    state->nzcv = value;
    return AARCH64_CPU_STATE_SUCCESS;
}

AARCH64CPUStateStatus AARCH64CPUStateReadFPCR(
    const AARCH64CPUState *state,
    uint32_t *value
)
{
    if (state == 0 || value == 0)
        return AARCH64_CPU_STATE_INVALID_ARGUMENT;

    *value = state->fpcr;
    return AARCH64_CPU_STATE_SUCCESS;
}

AARCH64CPUStateStatus AARCH64CPUStateWriteFPCR(
    AARCH64CPUState *state,
    uint32_t value
)
{
    if (state == 0)
        return AARCH64_CPU_STATE_INVALID_ARGUMENT;

    state->fpcr = value;
    return AARCH64_CPU_STATE_SUCCESS;
}

AARCH64CPUStateStatus AARCH64CPUStateReadFPSR(
    const AARCH64CPUState *state,
    uint32_t *value
)
{
    if (state == 0 || value == 0)
        return AARCH64_CPU_STATE_INVALID_ARGUMENT;

    *value = state->fpsr;
    return AARCH64_CPU_STATE_SUCCESS;
}

AARCH64CPUStateStatus AARCH64CPUStateWriteFPSR(
    AARCH64CPUState *state,
    uint32_t value
)
{
    if (state == 0)
        return AARCH64_CPU_STATE_INVALID_ARGUMENT;

    state->fpsr = value;
    return AARCH64_CPU_STATE_SUCCESS;
}

AARCH64CPUStateStatus AARCH64CPUStateReadVector(
    const AARCH64CPUState *state,
    uint8_t index,
    uint8_t width_bits,
    AARCH64VectorRegister *value
)
{
    if (state == 0 || value == 0)
        return AARCH64_CPU_STATE_INVALID_ARGUMENT;

    if (index >= AARCH64_CPU_VECTOR_COUNT)
        return AARCH64_CPU_STATE_OUT_OF_RANGE;

    if (width_bits != 64 && width_bits != 128)
        return AARCH64_CPU_STATE_INVALID_ARGUMENT;

    *value = state->v[index];
    if (width_bits == 64)
        value->high = 0;

    return AARCH64_CPU_STATE_SUCCESS;
}

AARCH64CPUStateStatus AARCH64CPUStateWriteVector(
    AARCH64CPUState *state,
    uint8_t index,
    uint8_t width_bits,
    const AARCH64VectorRegister *value
)
{
    if (state == 0 || value == 0)
        return AARCH64_CPU_STATE_INVALID_ARGUMENT;

    if (index >= AARCH64_CPU_VECTOR_COUNT)
        return AARCH64_CPU_STATE_OUT_OF_RANGE;

    if (width_bits != 64 && width_bits != 128)
        return AARCH64_CPU_STATE_INVALID_ARGUMENT;

    state->v[index].low = value->low;
    state->v[index].high = width_bits == 128 ? value->high : 0;
    return AARCH64_CPU_STATE_SUCCESS;
}

AARCH64CPUStateStatus AARCH64CPUStateSetException(
    AARCH64CPUState *state,
    uint32_t code,
    uint64_t pc,
    uint64_t address,
    uint64_t syndrome
)
{
    if (state == 0)
        return AARCH64_CPU_STATE_INVALID_ARGUMENT;

    state->exception.pending = 1;
    state->exception.code = code;
    state->exception.pc = pc;
    state->exception.address = address;
    state->exception.syndrome = syndrome;
    return AARCH64_CPU_STATE_SUCCESS;
}

AARCH64CPUStateStatus AARCH64CPUStateClearException(AARCH64CPUState *state)
{
    if (state == 0)
        return AARCH64_CPU_STATE_INVALID_ARGUMENT;

    state->exception.pending = 0;
    state->exception.code = 0;
    state->exception.pc = 0;
    state->exception.address = 0;
    state->exception.syndrome = 0;
    return AARCH64_CPU_STATE_SUCCESS;
}
