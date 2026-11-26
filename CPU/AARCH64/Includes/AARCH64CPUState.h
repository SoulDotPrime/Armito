#ifndef ARMITO_AARCH64_CPU_STATE_H
#define ARMITO_AARCH64_CPU_STATE_H

#include <stdint.h>

#define AARCH64_CPU_GPR_COUNT 31u
#define AARCH64_CPU_VECTOR_COUNT 32u

#define AARCH64_NZCV_N 0x08u
#define AARCH64_NZCV_Z 0x04u
#define AARCH64_NZCV_C 0x02u
#define AARCH64_NZCV_V 0x01u
#define AARCH64_NZCV_MASK 0x0Fu

typedef enum {
    AARCH64_CPU_STATE_SUCCESS = 0,
    AARCH64_CPU_STATE_INVALID_ARGUMENT,
    AARCH64_CPU_STATE_OUT_OF_RANGE
} AARCH64CPUStateStatus;

typedef enum {
    AARCH64_GPR31_IS_ZERO = 0,
    AARCH64_GPR31_IS_SP = 1
} AARCH64GPR31Behavior;

typedef struct {
    uint64_t low;
    uint64_t high;
} AARCH64VectorRegister;

typedef struct {
    uint64_t low;
    uint64_t high;
} AARCH64PointerAuthKey;

typedef struct {
    uint8_t enabled;
    AARCH64PointerAuthKey api_a;
    AARCH64PointerAuthKey api_b;
    AARCH64PointerAuthKey apda;
    AARCH64PointerAuthKey apdb;
    AARCH64PointerAuthKey apga;
} AARCH64PointerAuthState;

typedef struct {
    uint8_t pending;
    uint32_t code;
    uint64_t pc;
    uint64_t address;
    uint64_t syndrome;
} AARCH64CPUExceptionState;

typedef struct {
    uint64_t x[AARCH64_CPU_GPR_COUNT];
    uint64_t sp;
    uint64_t pc;
    uint8_t nzcv;
    uint32_t fpcr;
    uint32_t fpsr;
    AARCH64VectorRegister v[AARCH64_CPU_VECTOR_COUNT];
    AARCH64CPUExceptionState exception;
    AARCH64PointerAuthState pointer_auth;
} AARCH64CPUState;

AARCH64CPUStateStatus AARCH64CPUStateReset(AARCH64CPUState *state);
AARCH64CPUStateStatus AARCH64CPUStateReadGPR(
    const AARCH64CPUState *state,
    uint8_t index,
    uint8_t width_bits,
    AARCH64GPR31Behavior index31_behavior,
    uint64_t *value
);
AARCH64CPUStateStatus AARCH64CPUStateWriteGPR(
    AARCH64CPUState *state,
    uint8_t index,
    uint8_t width_bits,
    AARCH64GPR31Behavior index31_behavior,
    uint64_t value
);
AARCH64CPUStateStatus AARCH64CPUStateReadPC(
    const AARCH64CPUState *state,
    uint64_t *value
);
AARCH64CPUStateStatus AARCH64CPUStateWritePC(
    AARCH64CPUState *state,
    uint64_t value
);
AARCH64CPUStateStatus AARCH64CPUStateReadNZCV(
    const AARCH64CPUState *state,
    uint8_t *value
);
AARCH64CPUStateStatus AARCH64CPUStateWriteNZCV(
    AARCH64CPUState *state,
    uint8_t value
);
AARCH64CPUStateStatus AARCH64CPUStateReadFPCR(
    const AARCH64CPUState *state,
    uint32_t *value
);
AARCH64CPUStateStatus AARCH64CPUStateWriteFPCR(
    AARCH64CPUState *state,
    uint32_t value
);
AARCH64CPUStateStatus AARCH64CPUStateReadFPSR(
    const AARCH64CPUState *state,
    uint32_t *value
);
AARCH64CPUStateStatus AARCH64CPUStateWriteFPSR(
    AARCH64CPUState *state,
    uint32_t value
);
AARCH64CPUStateStatus AARCH64CPUStateReadVector(
    const AARCH64CPUState *state,
    uint8_t index,
    uint8_t width_bits,
    AARCH64VectorRegister *value
);
AARCH64CPUStateStatus AARCH64CPUStateWriteVector(
    AARCH64CPUState *state,
    uint8_t index,
    uint8_t width_bits,
    const AARCH64VectorRegister *value
);
AARCH64CPUStateStatus AARCH64CPUStateSetException(
    AARCH64CPUState *state,
    uint32_t code,
    uint64_t pc,
    uint64_t address,
    uint64_t syndrome
);
AARCH64CPUStateStatus AARCH64CPUStateClearException(AARCH64CPUState *state);

#endif
