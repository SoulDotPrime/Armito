#ifndef ARMITO_AARCH64_POINTER_AUTH_H
#define ARMITO_AARCH64_POINTER_AUTH_H

#include "AARCH64CPUState.h"

typedef enum {
    AARCH64_POINTER_AUTH_SUCCESS = 0,
    AARCH64_POINTER_AUTH_INVALID_ARGUMENT,
    AARCH64_POINTER_AUTH_NOT_ENABLED,
    AARCH64_POINTER_AUTH_KEY_UNAVAILABLE,
    AARCH64_POINTER_AUTH_PROVIDER_ERROR,
    AARCH64_POINTER_AUTHENTICATION_FAILED,
    AARCH64_POINTER_AUTH_UNSUPPORTED
} AARCH64PointerAuthStatus;

typedef enum {
    AARCH64_PAC_USE_GLOBAL_FUNCTION_POINTER = 0,
    AARCH64_PAC_USE_LOCAL_RETURN_ADDRESS,
    AARCH64_PAC_USE_GLOBAL_DATA_POINTER,
    AARCH64_PAC_USE_LOCAL_DATA_POINTER,
    AARCH64_PAC_USE_VTABLE_ENTRY
} AARCH64PointerAuthAppleUse;

typedef AARCH64PointerAuthStatus (*AARCH64PointerAuthCompute)(
    void *provider_context,
    const AARCH64PointerAuthKey *key,
    uint64_t pointer_address,
    const AARCH64PointerAuthContext *auth_context,
    uint64_t pac_mask,
    uint64_t *signature
);

typedef struct {
    void *context;
    AARCH64PointerAuthCompute compute;
    uint8_t compatibility_only;
} AARCH64PointerAuthProvider;

AARCH64PointerAuthStatus AARCH64PointerAuthConfigure(
    AARCH64CPUState *state,
    uint64_t address_mask,
    uint64_t pac_mask,
    uint8_t enabled
);
AARCH64PointerAuthStatus AARCH64PointerAuthSetKey(
    AARCH64CPUState *state,
    AARCH64PointerAuthKeyID key_id,
    const AARCH64PointerAuthKey *key
);
AARCH64PointerAuthStatus AARCH64PointerAuthSelectAppleABIKey(
    AARCH64PointerAuthAppleUse use,
    AARCH64PointerAuthKeyID *key_id
);
AARCH64PointerAuthStatus AARCH64PointerAuthSign(
    AARCH64CPUState *state,
    const AARCH64PointerAuthProvider *provider,
    AARCH64PointerAuthKeyID key_id,
    uint64_t pointer,
    const AARCH64PointerAuthContext *auth_context,
    uint64_t *signed_pointer
);
AARCH64PointerAuthStatus AARCH64PointerAuthAuthenticate(
    AARCH64CPUState *state,
    const AARCH64PointerAuthProvider *provider,
    AARCH64PointerAuthKeyID key_id,
    uint64_t pointer,
    const AARCH64PointerAuthContext *auth_context,
    uint64_t *authenticated_pointer
);
AARCH64PointerAuthStatus AARCH64PointerAuthStrip(
    const AARCH64CPUState *state,
    uint64_t pointer,
    uint64_t *stripped_pointer
);
AARCH64PointerAuthStatus AARCH64PointerAuthProviderInitSoftware(
    AARCH64PointerAuthProvider *provider
);
AARCH64PointerAuthStatus AARCH64PointerAuthSignLRWithSP(
    AARCH64CPUState *state,
    const AARCH64PointerAuthProvider *provider,
    AARCH64PointerAuthKeyID key_id
);
AARCH64PointerAuthStatus AARCH64PointerAuthAuthenticateLRWithSP(
    AARCH64CPUState *state,
    const AARCH64PointerAuthProvider *provider,
    AARCH64PointerAuthKeyID key_id
);
AARCH64PointerAuthStatus AARCH64PointerAuthReturn(
    AARCH64CPUState *state,
    const AARCH64PointerAuthProvider *provider,
    AARCH64PointerAuthKeyID key_id
);
AARCH64PointerAuthStatus AARCH64PointerAuthBranch(
    AARCH64CPUState *state,
    const AARCH64PointerAuthProvider *provider,
    AARCH64PointerAuthKeyID key_id,
    uint64_t target,
    uint64_t modifier,
    uint8_t link,
    uint64_t return_address
);

#endif
