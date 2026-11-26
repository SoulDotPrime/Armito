#include <stdio.h>

#include "Includes/AARCH64CPUState.h"

static uint32_t tests_run;
static uint32_t tests_failed;

#define CHECK(expression) do { \
    tests_run++; \
    if (!(expression)) { \
        tests_failed++; \
        printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #expression); \
    } \
} while (0)

static void TestGPRViews(void)
{
    AARCH64CPUState state;
    uint64_t value = 0;

    CHECK(AARCH64CPUStateReset(&state) == AARCH64_CPU_STATE_SUCCESS);
    CHECK(AARCH64CPUStateWriteGPR(
        &state, 0, 64, AARCH64_GPR31_IS_ZERO, 0x1122334455667788ULL
    ) == AARCH64_CPU_STATE_SUCCESS);
    CHECK(AARCH64CPUStateReadGPR(
        &state, 0, 32, AARCH64_GPR31_IS_ZERO, &value
    ) == AARCH64_CPU_STATE_SUCCESS);
    CHECK(value == 0x55667788u);

    CHECK(AARCH64CPUStateWriteGPR(
        &state, 0, 32, AARCH64_GPR31_IS_ZERO, 0xAABBCCDD11223344ULL
    ) == AARCH64_CPU_STATE_SUCCESS);
    CHECK(AARCH64CPUStateReadGPR(
        &state, 0, 64, AARCH64_GPR31_IS_ZERO, &value
    ) == AARCH64_CPU_STATE_SUCCESS);
    CHECK(value == 0x11223344u);

    CHECK(AARCH64CPUStateWriteGPR(
        &state, 30, 64, AARCH64_GPR31_IS_ZERO, 0xCAFEBABE01234567ULL
    ) == AARCH64_CPU_STATE_SUCCESS);
    CHECK(AARCH64CPUStateReadGPR(
        &state, 30, 64, AARCH64_GPR31_IS_ZERO, &value
    ) == AARCH64_CPU_STATE_SUCCESS);
    CHECK(value == 0xCAFEBABE01234567ULL);
}

static void TestSPAndZeroRegister(void)
{
    AARCH64CPUState state;
    uint64_t value = 0xFFFFFFFFFFFFFFFFULL;

    CHECK(AARCH64CPUStateReset(&state) == AARCH64_CPU_STATE_SUCCESS);
    CHECK(AARCH64CPUStateWriteGPR(
        &state, 31, 32, AARCH64_GPR31_IS_SP, 0x12345678FFFFFFFFULL
    ) == AARCH64_CPU_STATE_SUCCESS);
    CHECK(AARCH64CPUStateReadGPR(
        &state, 31, 64, AARCH64_GPR31_IS_SP, &value
    ) == AARCH64_CPU_STATE_SUCCESS);
    CHECK(value == 0xFFFFFFFFu);

    CHECK(AARCH64CPUStateWriteGPR(
        &state, 31, 64, AARCH64_GPR31_IS_ZERO, 0xAAAAAAAAAAAAAAAAULL
    ) == AARCH64_CPU_STATE_SUCCESS);
    CHECK(AARCH64CPUStateReadGPR(
        &state, 31, 64, AARCH64_GPR31_IS_ZERO, &value
    ) == AARCH64_CPU_STATE_SUCCESS);
    CHECK(value == 0);
    CHECK(state.sp == 0xFFFFFFFFu);
}

static void TestControlAndFlags(void)
{
    AARCH64CPUState state;
    uint64_t pc;
    uint8_t nzcv;

    CHECK(AARCH64CPUStateReset(&state) == AARCH64_CPU_STATE_SUCCESS);
    CHECK(AARCH64CPUStateWritePC(&state, 0x123456789ABCDEF0ULL) ==
          AARCH64_CPU_STATE_SUCCESS);
    CHECK(AARCH64CPUStateReadPC(&state, &pc) == AARCH64_CPU_STATE_SUCCESS);
    CHECK(pc == 0x123456789ABCDEF0ULL);

    CHECK(AARCH64CPUStateWriteNZCV(
        &state, AARCH64_NZCV_N | AARCH64_NZCV_C
    ) == AARCH64_CPU_STATE_SUCCESS);
    CHECK(AARCH64CPUStateReadNZCV(&state, &nzcv) == AARCH64_CPU_STATE_SUCCESS);
    CHECK(nzcv == (AARCH64_NZCV_N | AARCH64_NZCV_C));
    CHECK(AARCH64CPUStateWriteNZCV(&state, 0x10) ==
          AARCH64_CPU_STATE_INVALID_ARGUMENT);
    CHECK(state.nzcv == (AARCH64_NZCV_N | AARCH64_NZCV_C));
}

static void TestFPAndVectorRegisters(void)
{
    AARCH64CPUState state;
    AARCH64VectorRegister vector = { 0x0123456789ABCDEFULL, 0xFEDCBA9876543210ULL };
    AARCH64VectorRegister result;
    uint32_t value32;

    CHECK(AARCH64CPUStateReset(&state) == AARCH64_CPU_STATE_SUCCESS);
    CHECK(AARCH64CPUStateWriteFPCR(&state, 0x12345678u) ==
          AARCH64_CPU_STATE_SUCCESS);
    CHECK(AARCH64CPUStateReadFPCR(&state, &value32) == AARCH64_CPU_STATE_SUCCESS);
    CHECK(value32 == 0x12345678u);
    CHECK(AARCH64CPUStateWriteFPSR(&state, 0x87654321u) ==
          AARCH64_CPU_STATE_SUCCESS);
    CHECK(AARCH64CPUStateReadFPSR(&state, &value32) == AARCH64_CPU_STATE_SUCCESS);
    CHECK(value32 == 0x87654321u);

    CHECK(AARCH64CPUStateWriteVector(&state, 31, 128, &vector) ==
          AARCH64_CPU_STATE_SUCCESS);
    CHECK(AARCH64CPUStateReadVector(&state, 31, 128, &result) ==
          AARCH64_CPU_STATE_SUCCESS);
    CHECK(result.low == vector.low && result.high == vector.high);
    CHECK(AARCH64CPUStateReadVector(&state, 31, 64, &result) ==
          AARCH64_CPU_STATE_SUCCESS);
    CHECK(result.low == vector.low && result.high == 0);

    CHECK(AARCH64CPUStateWriteVector(&state, 31, 64, &vector) ==
          AARCH64_CPU_STATE_SUCCESS);
    CHECK(state.v[31].low == vector.low && state.v[31].high == 0);
}

static void TestExceptionAndArguments(void)
{
    AARCH64CPUState state;
    uint64_t value = 0;

    CHECK(AARCH64CPUStateReset(&state) == AARCH64_CPU_STATE_SUCCESS);
    CHECK(AARCH64CPUStateSetException(
        &state, 7, 0x1000, 0x2000, 0x3000
    ) == AARCH64_CPU_STATE_SUCCESS);
    CHECK(state.exception.pending == 1);
    CHECK(state.exception.code == 7);
    CHECK(state.exception.pc == 0x1000);
    CHECK(state.exception.address == 0x2000);
    CHECK(state.exception.syndrome == 0x3000);
    CHECK(AARCH64CPUStateClearException(&state) == AARCH64_CPU_STATE_SUCCESS);
    CHECK(state.exception.pending == 0 && state.exception.code == 0);

    CHECK(AARCH64CPUStateReadGPR(
        &state, 32, 64, AARCH64_GPR31_IS_ZERO, &value
    ) == AARCH64_CPU_STATE_OUT_OF_RANGE);
    CHECK(AARCH64CPUStateReadGPR(
        &state, 0, 16, AARCH64_GPR31_IS_ZERO, &value
    ) == AARCH64_CPU_STATE_INVALID_ARGUMENT);
    CHECK(AARCH64CPUStateReadGPR(
        &state, 0, 64, (AARCH64GPR31Behavior)2, &value
    ) == AARCH64_CPU_STATE_INVALID_ARGUMENT);
    CHECK(AARCH64CPUStateReadPC(&state, 0) == AARCH64_CPU_STATE_INVALID_ARGUMENT);
    CHECK(AARCH64CPUStateReset(0) == AARCH64_CPU_STATE_INVALID_ARGUMENT);

    state.pointer_auth.enabled = 1;
    state.pointer_auth.apga.high = 0x1234;
    CHECK(AARCH64CPUStateReset(&state) == AARCH64_CPU_STATE_SUCCESS);
    CHECK(state.pointer_auth.enabled == 0);
    CHECK(state.pointer_auth.apga.high == 0);
}

int main(void)
{
    TestGPRViews();
    TestSPAndZeroRegister();
    TestControlAndFlags();
    TestFPAndVectorRegisters();
    TestExceptionAndArguments();

    printf(
        "AArch64 CPU State: %u checks, %u failures\n",
        tests_run,
        tests_failed
    );
    return tests_failed == 0 ? 0 : 1;
}
