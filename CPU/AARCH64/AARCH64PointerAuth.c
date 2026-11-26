#include "Includes/AARCH64PointerAuth.h"

static AARCH64PointerAuthKey *AARCH64PointerAuthGetKey(
    AARCH64PointerAuthState *state,
    AARCH64PointerAuthKeyID key_id
)
{
    switch (key_id) {
    case AARCH64_PAC_KEY_IA: return &state->api_a;
    case AARCH64_PAC_KEY_IB: return &state->api_b;
    case AARCH64_PAC_KEY_DA: return &state->apda;
    case AARCH64_PAC_KEY_DB: return &state->apdb;
    case AARCH64_PAC_KEY_GA: return &state->apga;
    default: return 0;
    }
}

static AARCH64PointerAuthStatus AARCH64PointerAuthValidate(
    AARCH64CPUState *state,
    const AARCH64PointerAuthProvider *provider,
    AARCH64PointerAuthKeyID key_id,
    const AARCH64PointerAuthContext *auth_context,
    AARCH64PointerAuthKey **key
)
{
    if (state == 0 || provider == 0 || auth_context == 0 || key == 0)
        return AARCH64_POINTER_AUTH_INVALID_ARGUMENT;

    if (!state->pointer_auth.enabled)
        return AARCH64_POINTER_AUTH_NOT_ENABLED;

    if (provider->compute == 0)
        return AARCH64_POINTER_AUTH_INVALID_ARGUMENT;

    if ((uint32_t)key_id >= AARCH64_PAC_KEY_COUNT)
        return AARCH64_POINTER_AUTH_INVALID_ARGUMENT;

    if ((state->pointer_auth.initialized_key_mask & (1u << key_id)) == 0)
        return AARCH64_POINTER_AUTH_KEY_UNAVAILABLE;

    if (state->pointer_auth.pac_mask == 0 ||
        state->pointer_auth.address_mask == 0 ||
        (state->pointer_auth.pac_mask & state->pointer_auth.address_mask) != 0)
        return AARCH64_POINTER_AUTH_INVALID_ARGUMENT;

    if (auth_context->address_diversity > 1)
        return AARCH64_POINTER_AUTH_INVALID_ARGUMENT;

    *key = AARCH64PointerAuthGetKey(&state->pointer_auth, key_id);
    if (*key == 0)
        return AARCH64_POINTER_AUTH_INVALID_ARGUMENT;

    return AARCH64_POINTER_AUTH_SUCCESS;
}

static void AARCH64PointerAuthRecordContext(
    AARCH64PointerAuthState *state,
    AARCH64PointerAuthKeyID key_id,
    const AARCH64PointerAuthContext *auth_context,
    uint8_t operation
)
{
    state->last_key = (uint8_t)key_id;
    state->last_operation = operation;
    state->last_modifier = auth_context->modifier;
    state->last_context_address = auth_context->context_address;
    state->last_context = *auth_context;
}

static AARCH64PointerAuthStatus AARCH64PointerAuthComputeSignature(
    AARCH64CPUState *state,
    const AARCH64PointerAuthProvider *provider,
    AARCH64PointerAuthKeyID key_id,
    uint64_t pointer_address,
    const AARCH64PointerAuthContext *auth_context,
    uint64_t *signature
)
{
    AARCH64PointerAuthKey *key;
    AARCH64PointerAuthStatus status;

    status = AARCH64PointerAuthValidate(
        state, provider, key_id, auth_context, &key
    );
    if (status != AARCH64_POINTER_AUTH_SUCCESS)
        return status;

    status = provider->compute(
        provider->context,
        key,
        pointer_address,
        auth_context,
        state->pointer_auth.pac_mask,
        signature
    );
    if (status != AARCH64_POINTER_AUTH_SUCCESS)
        return AARCH64_POINTER_AUTH_PROVIDER_ERROR;

    if ((*signature & ~state->pointer_auth.pac_mask) != 0)
        return AARCH64_POINTER_AUTH_PROVIDER_ERROR;

    return AARCH64_POINTER_AUTH_SUCCESS;
}

AARCH64PointerAuthStatus AARCH64PointerAuthConfigure(
    AARCH64CPUState *state,
    uint64_t address_mask,
    uint64_t pac_mask,
    uint8_t enabled
)
{
    if (state == 0)
        return AARCH64_POINTER_AUTH_INVALID_ARGUMENT;

    if (enabled > 1 || address_mask == 0 || pac_mask == 0 ||
        (address_mask & pac_mask) != 0)
        return AARCH64_POINTER_AUTH_INVALID_ARGUMENT;

    state->pointer_auth.address_mask = address_mask;
    state->pointer_auth.pac_mask = pac_mask;
    state->pointer_auth.enabled = enabled;
    state->pointer_auth.authentication_failed = 0;
    state->pointer_auth.return_address_signed = 0;

    return AARCH64_POINTER_AUTH_SUCCESS;
}

AARCH64PointerAuthStatus AARCH64PointerAuthSetKey(
    AARCH64CPUState *state,
    AARCH64PointerAuthKeyID key_id,
    const AARCH64PointerAuthKey *key
)
{
    AARCH64PointerAuthKey *destination;

    if (state == 0 || key == 0)
        return AARCH64_POINTER_AUTH_INVALID_ARGUMENT;

    if ((uint32_t)key_id >= AARCH64_PAC_KEY_COUNT)
        return AARCH64_POINTER_AUTH_INVALID_ARGUMENT;

    destination = AARCH64PointerAuthGetKey(&state->pointer_auth, key_id);
    if (destination == 0)
        return AARCH64_POINTER_AUTH_INVALID_ARGUMENT;

    *destination = *key;
    state->pointer_auth.initialized_key_mask |= (uint8_t)(1u << key_id);

    return AARCH64_POINTER_AUTH_SUCCESS;
}

AARCH64PointerAuthStatus AARCH64PointerAuthSelectAppleABIKey(
    AARCH64PointerAuthAppleUse use,
    AARCH64PointerAuthKeyID *key_id
)
{
    if (key_id == 0)
        return AARCH64_POINTER_AUTH_INVALID_ARGUMENT;

    switch (use) {
    case AARCH64_PAC_USE_GLOBAL_FUNCTION_POINTER:
    case AARCH64_PAC_USE_VTABLE_ENTRY:
        *key_id = AARCH64_PAC_KEY_IA;
        return AARCH64_POINTER_AUTH_SUCCESS;
    case AARCH64_PAC_USE_LOCAL_RETURN_ADDRESS:
        *key_id = AARCH64_PAC_KEY_IB;
        return AARCH64_POINTER_AUTH_SUCCESS;
    case AARCH64_PAC_USE_GLOBAL_DATA_POINTER:
        *key_id = AARCH64_PAC_KEY_DA;
        return AARCH64_POINTER_AUTH_SUCCESS;
    case AARCH64_PAC_USE_LOCAL_DATA_POINTER:
        *key_id = AARCH64_PAC_KEY_DB;
        return AARCH64_POINTER_AUTH_SUCCESS;
    default:
        return AARCH64_POINTER_AUTH_INVALID_ARGUMENT;
    }
}

AARCH64PointerAuthStatus AARCH64PointerAuthSign(
    AARCH64CPUState *state,
    const AARCH64PointerAuthProvider *provider,
    AARCH64PointerAuthKeyID key_id,
    uint64_t pointer,
    const AARCH64PointerAuthContext *auth_context,
    uint64_t *signed_pointer
)
{
    AARCH64PointerAuthStatus status;
    uint64_t signature;

    if (signed_pointer == 0)
        return AARCH64_POINTER_AUTH_INVALID_ARGUMENT;
    if (state == 0)
        return AARCH64_POINTER_AUTH_INVALID_ARGUMENT;

    status = AARCH64PointerAuthComputeSignature(
        state,
        provider,
        key_id,
        pointer & state->pointer_auth.address_mask,
        auth_context,
        &signature
    );
    if (status != AARCH64_POINTER_AUTH_SUCCESS)
        return status;

    *signed_pointer =
        (pointer & ~state->pointer_auth.pac_mask) | signature;
    state->pointer_auth.authentication_failed = 0;
    AARCH64PointerAuthRecordContext(
        &state->pointer_auth, key_id, auth_context, 1
    );

    return AARCH64_POINTER_AUTH_SUCCESS;
}

AARCH64PointerAuthStatus AARCH64PointerAuthAuthenticate(
    AARCH64CPUState *state,
    const AARCH64PointerAuthProvider *provider,
    AARCH64PointerAuthKeyID key_id,
    uint64_t pointer,
    const AARCH64PointerAuthContext *auth_context,
    uint64_t *authenticated_pointer
)
{
    AARCH64PointerAuthStatus status;
    uint64_t signature;
    uint64_t pointer_address;
    uint64_t supplied_signature;

    if (state == 0 || authenticated_pointer == 0)
        return AARCH64_POINTER_AUTH_INVALID_ARGUMENT;

    pointer_address = pointer & state->pointer_auth.address_mask;
    supplied_signature = pointer & state->pointer_auth.pac_mask;
    status = AARCH64PointerAuthComputeSignature(
        state,
        provider,
        key_id,
        pointer_address,
        auth_context,
        &signature
    );
    if (status != AARCH64_POINTER_AUTH_SUCCESS)
        return status;

    AARCH64PointerAuthRecordContext(
        &state->pointer_auth, key_id, auth_context, 2
    );

    if (signature == supplied_signature) {
        *authenticated_pointer =
            pointer & ~state->pointer_auth.pac_mask;
        state->pointer_auth.authentication_failed = 0;
        return AARCH64_POINTER_AUTH_SUCCESS;
    }

    *authenticated_pointer =
        (pointer & ~state->pointer_auth.pac_mask) | (1ULL << 63);
    state->pointer_auth.authentication_failed = 1;
    state->pointer_auth.failed_pointer = pointer;
    state->pointer_auth.invalid_pointer = *authenticated_pointer;
    state->pointer_auth.failure_count++;

    return AARCH64_POINTER_AUTHENTICATION_FAILED;
}

AARCH64PointerAuthStatus AARCH64PointerAuthStrip(
    const AARCH64CPUState *state,
    uint64_t pointer,
    uint64_t *stripped_pointer
)
{
    if (state == 0 || stripped_pointer == 0)
        return AARCH64_POINTER_AUTH_INVALID_ARGUMENT;

    *stripped_pointer = pointer & ~state->pointer_auth.pac_mask;
    return AARCH64_POINTER_AUTH_SUCCESS;
}

static uint64_t AARCH64PointerAuthMix(uint64_t value)
{
    value ^= value >> 30;
    value *= 0xBF58476D1CE4E5B9ULL;
    value ^= value >> 27;
    value *= 0x94D049BB133111EBULL;
    value ^= value >> 31;
    return value;
}

static AARCH64PointerAuthStatus AARCH64PointerAuthSoftwareCompute(
    void *provider_context,
    const AARCH64PointerAuthKey *key,
    uint64_t pointer_address,
    const AARCH64PointerAuthContext *auth_context,
    uint64_t pac_mask,
    uint64_t *signature
)
{
    uint64_t value;

    (void)provider_context;

    if (key == 0 || auth_context == 0 || signature == 0 || pac_mask == 0)
        return AARCH64_POINTER_AUTH_INVALID_ARGUMENT;

    value = AARCH64PointerAuthMix(key->low ^ pointer_address);
    value ^= AARCH64PointerAuthMix(key->high ^ auth_context->modifier);
    value ^= AARCH64PointerAuthMix((uint64_t)auth_context->discriminator);
    if (auth_context->address_diversity)
        value ^= AARCH64PointerAuthMix(auth_context->context_address);

    *signature = AARCH64PointerAuthMix(value) & pac_mask;
    return AARCH64_POINTER_AUTH_SUCCESS;
}

AARCH64PointerAuthStatus AARCH64PointerAuthProviderInitSoftware(
    AARCH64PointerAuthProvider *provider
)
{
    if (provider == 0)
        return AARCH64_POINTER_AUTH_INVALID_ARGUMENT;

    provider->context = 0;
    provider->compute = AARCH64PointerAuthSoftwareCompute;
    provider->compatibility_only = 1;
    return AARCH64_POINTER_AUTH_SUCCESS;
}

AARCH64PointerAuthStatus AARCH64PointerAuthSignLRWithSP(
    AARCH64CPUState *state,
    const AARCH64PointerAuthProvider *provider,
    AARCH64PointerAuthKeyID key_id
)
{
    AARCH64PointerAuthContext auth_context = { 0, 0, 0, 0 };
    uint64_t signed_lr;
    AARCH64PointerAuthStatus status;

    if (state == 0)
        return AARCH64_POINTER_AUTH_INVALID_ARGUMENT;

    auth_context.modifier = state->sp;
    status = AARCH64PointerAuthSign(
        state, provider, key_id, state->x[30], &auth_context, &signed_lr
    );
    if (status != AARCH64_POINTER_AUTH_SUCCESS)
        return status;

    state->x[30] = signed_lr;
    state->pointer_auth.return_address_signed = 1;
    return AARCH64_POINTER_AUTH_SUCCESS;
}

AARCH64PointerAuthStatus AARCH64PointerAuthAuthenticateLRWithSP(
    AARCH64CPUState *state,
    const AARCH64PointerAuthProvider *provider,
    AARCH64PointerAuthKeyID key_id
)
{
    AARCH64PointerAuthContext auth_context = { 0, 0, 0, 0 };
    uint64_t authenticated_lr = 0;
    AARCH64PointerAuthStatus status;

    if (state == 0)
        return AARCH64_POINTER_AUTH_INVALID_ARGUMENT;

    auth_context.modifier = state->sp;
    status = AARCH64PointerAuthAuthenticate(
        state, provider, key_id, state->x[30], &auth_context, &authenticated_lr
    );
    if (status == AARCH64_POINTER_AUTH_SUCCESS ||
        status == AARCH64_POINTER_AUTHENTICATION_FAILED) {
        state->x[30] = authenticated_lr;
        if (status == AARCH64_POINTER_AUTH_SUCCESS)
            state->pointer_auth.return_address_signed = 0;
    }

    return status;
}

AARCH64PointerAuthStatus AARCH64PointerAuthReturn(
    AARCH64CPUState *state,
    const AARCH64PointerAuthProvider *provider,
    AARCH64PointerAuthKeyID key_id
)
{
    AARCH64PointerAuthContext auth_context = { 0, 0, 0, 0 };
    uint64_t target = 0;
    AARCH64PointerAuthStatus status;

    if (state == 0)
        return AARCH64_POINTER_AUTH_INVALID_ARGUMENT;

    auth_context.modifier = state->sp;
    status = AARCH64PointerAuthAuthenticate(
        state, provider, key_id, state->x[30], &auth_context, &target
    );
    if (status == AARCH64_POINTER_AUTHENTICATION_FAILED) {
        AARCH64CPUStateSetException(
            state,
            AARCH64_CPU_EXCEPTION_POINTER_AUTH,
            state->pc,
            target,
            (uint64_t)key_id
        );
        return status;
    }
    if (status != AARCH64_POINTER_AUTH_SUCCESS)
        return status;

    state->pc = target;
    state->pointer_auth.return_address_signed = 0;
    return AARCH64_POINTER_AUTH_SUCCESS;
}

AARCH64PointerAuthStatus AARCH64PointerAuthBranch(
    AARCH64CPUState *state,
    const AARCH64PointerAuthProvider *provider,
    AARCH64PointerAuthKeyID key_id,
    uint64_t target,
    uint64_t modifier,
    uint8_t link,
    uint64_t return_address
)
{
    AARCH64PointerAuthContext auth_context = { 0, 0, 0, 0 };
    uint64_t authenticated_target;
    AARCH64PointerAuthStatus status;

    if (state == 0 || link > 1)
        return AARCH64_POINTER_AUTH_INVALID_ARGUMENT;

    auth_context.modifier = modifier;
    status = AARCH64PointerAuthAuthenticate(
        state,
        provider,
        key_id,
        target,
        &auth_context,
        &authenticated_target
    );
    if (status == AARCH64_POINTER_AUTHENTICATION_FAILED) {
        AARCH64CPUStateSetException(
            state,
            AARCH64_CPU_EXCEPTION_POINTER_AUTH,
            state->pc,
            authenticated_target,
            (uint64_t)key_id
        );
        return status;
    }
    if (status != AARCH64_POINTER_AUTH_SUCCESS)
        return status;

    if (link)
        state->x[30] = return_address;
    state->pc = authenticated_target;

    return AARCH64_POINTER_AUTH_SUCCESS;
}
