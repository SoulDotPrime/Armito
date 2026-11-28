#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "AARCH64/Includes/AARCH64CPUState.h"
#include "AARCH64/Includes/AARCH64PointerAuth.h"
#include "AARCH64/Includes/AARCH64MachO.h"
#include "Decoder/AARCH64/Includes/AARCH64.h"
#include "Decoder/Includes/Decoder.h"
#include "Includes/IR.h"

static uint32_t tests_run;
static uint32_t tests_failed;

#define CHECK(expression) do { \
    tests_run++; \
    if (!(expression)) { \
        tests_failed++; \
        printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #expression); \
    } \
} while (0)

static void Write32LE(uint8_t *destination, uint32_t value)
{
    destination[0] = (uint8_t)value;
    destination[1] = (uint8_t)(value >> 8);
    destination[2] = (uint8_t)(value >> 16);
    destination[3] = (uint8_t)(value >> 24);
}

static void Write32BE(uint8_t *destination, uint32_t value)
{
    destination[0] = (uint8_t)(value >> 24);
    destination[1] = (uint8_t)(value >> 16);
    destination[2] = (uint8_t)(value >> 8);
    destination[3] = (uint8_t)value;
}

static void WriteMachOHeader(uint8_t *header, uint32_t cpu_type, uint32_t subtype)
{
    memset(header, 0, 32);
    Write32LE(header, 0xFEEDFACFu);
    Write32LE(header + 4, cpu_type);
    Write32LE(header + 8, subtype);
}

static void TestMachOSubtypes(void)
{
    AARCH64MachOArchitecture architecture;
    uint32_t raw_subtype;
    uint8_t header[32];
    uint8_t fat[112];
    uint8_t fat64[80];

    CHECK(AARCH64MachOIdentifyArchitecture(
        AARCH64_MACHO_CPU_TYPE_ARM64,
        AARCH64_MACHO_CPU_SUBTYPE_ARM64E,
        &architecture
    ) == AARCH64_MACHO_SUCCESS);
    CHECK(architecture.subtype == AARCH64_MACHO_SUBTYPE_ARM64E);
    CHECK(architecture.is_arm64e == 1);
    CHECK(architecture.subtype_value == 2);

    raw_subtype = AARCH64_MACHO_CPU_SUBTYPE_ARM64E |
        AARCH64_MACHO_CPU_SUBTYPE_VERSIONED_PTRAUTH_ABI |
        AARCH64_MACHO_CPU_SUBTYPE_KERNEL_PTRAUTH_ABI |
        (5u << AARCH64_MACHO_CPU_SUBTYPE_PTRAUTH_VERSION_SHIFT);
    CHECK(AARCH64MachOIdentifyArchitecture(
        AARCH64_MACHO_CPU_TYPE_ARM64, raw_subtype, &architecture
    ) == AARCH64_MACHO_SUCCESS);
    CHECK(architecture.subtype == AARCH64_MACHO_SUBTYPE_ARM64E);
    CHECK(architecture.capability_bits ==
          (raw_subtype & AARCH64_MACHO_CPU_SUBTYPE_MASK));
    CHECK(architecture.versioned_pointer_auth_abi == 1);
    CHECK(architecture.kernel_pointer_auth_abi == 1);
    CHECK(architecture.pointer_auth_abi_version == 5);
    CHECK(architecture.kernel_pointer_auth_abi_version == 5);

    CHECK(AARCH64MachOIdentifyArchitecture(
        AARCH64_MACHO_CPU_TYPE_ARM64,
        AARCH64_MACHO_CPU_SUBTYPE_ARM64_ALL,
        &architecture
    ) == AARCH64_MACHO_SUCCESS);
    CHECK(architecture.subtype == AARCH64_MACHO_SUBTYPE_ARM64_ALL);
    CHECK(architecture.is_arm64e == 0);

    CHECK(AARCH64MachOIdentifyArchitecture(
        AARCH64_MACHO_CPU_TYPE_ARM64,
        AARCH64_MACHO_CPU_SUBTYPE_ARM64_V8,
        &architecture
    ) == AARCH64_MACHO_SUCCESS);
    CHECK(architecture.subtype == AARCH64_MACHO_SUBTYPE_ARM64_V8);

    CHECK(AARCH64MachOIdentifyArchitecture(
        AARCH64_MACHO_CPU_TYPE_ARM64,
        AARCH64_MACHO_CPU_SUBTYPE_ARM64E_X1,
        &architecture
    ) == AARCH64_MACHO_SUCCESS);
    CHECK(architecture.subtype == AARCH64_MACHO_SUBTYPE_ARM64E_X1);
    CHECK(architecture.is_arm64e == 0);

    CHECK(AARCH64MachOIdentifyArchitecture(
        AARCH64_MACHO_CPU_TYPE_ARM64, 3u, &architecture
    ) == AARCH64_MACHO_INVALID_SUBTYPE);
    CHECK(AARCH64MachOIdentifyArchitecture(
        0x01000007u, AARCH64_MACHO_CPU_SUBTYPE_ARM64E, &architecture
    ) == AARCH64_MACHO_UNSUPPORTED_ARCH);
    CHECK(AARCH64MachOIdentifyArchitecture(
        AARCH64_MACHO_CPU_TYPE_ARM64, 2u, 0
    ) == AARCH64_MACHO_INVALID_ARGUMENT);

    WriteMachOHeader(header, AARCH64_MACHO_CPU_TYPE_ARM64, raw_subtype);
    CHECK(AARCH64MachOParse64Header(
        header, sizeof(header), &architecture
    ) == AARCH64_MACHO_SUCCESS);
    CHECK(architecture.is_arm64e == 1);
    CHECK(architecture.pointer_auth_abi_version == 5);
    CHECK(AARCH64MachOParse64Header(
        header, sizeof(header) - 1, &architecture
    ) == AARCH64_MACHO_TRUNCATED);
    header[0] = 0;
    CHECK(AARCH64MachOParse64Header(
        header, sizeof(header), &architecture
    ) == AARCH64_MACHO_INVALID_FORMAT);

    memset(fat, 0, sizeof(fat));
    Write32BE(fat, 0xCAFEBABEu);
    Write32BE(fat + 4, 2);
    Write32BE(fat + 8, 0x01000007u);
    Write32BE(fat + 12, 3);
    Write32BE(fat + 16, 48);
    Write32BE(fat + 20, 32);
    Write32BE(fat + 24, 2);
    Write32BE(fat + 28, AARCH64_MACHO_CPU_TYPE_ARM64);
    Write32BE(fat + 32, raw_subtype);
    Write32BE(fat + 36, 80);
    Write32BE(fat + 40, 32);
    Write32BE(fat + 44, 2);
    WriteMachOHeader(
        fat + 80, AARCH64_MACHO_CPU_TYPE_ARM64, raw_subtype
    );
    CHECK(AARCH64MachOFindARM64ESlice(
        fat, sizeof(fat), &architecture
    ) == AARCH64_MACHO_SUCCESS);
    CHECK(architecture.is_arm64e == 1);
    CHECK(architecture.slice_offset == 80);
    CHECK(architecture.slice_size == 32);
    memset(fat64, 0, sizeof(fat64));
    Write32BE(fat64, 0xCAFEBABFu);
    Write32BE(fat64 + 4, 1);
    Write32BE(fat64 + 8, AARCH64_MACHO_CPU_TYPE_ARM64);
    Write32BE(fat64 + 12, raw_subtype);
    Write32BE(fat64 + 16, 0);
    Write32BE(fat64 + 20, 48);
    Write32BE(fat64 + 24, 0);
    Write32BE(fat64 + 28, 32);
    Write32BE(fat64 + 32, 2);
    WriteMachOHeader(
        fat64 + 48, AARCH64_MACHO_CPU_TYPE_ARM64, raw_subtype
    );
    CHECK(AARCH64MachOFindARM64ESlice(
        fat64, sizeof(fat64), &architecture
    ) == AARCH64_MACHO_SUCCESS);
    CHECK(architecture.slice_offset == 48);
    CHECK(architecture.slice_size == 32);
    CHECK(AARCH64MachOFindARM64ESlice(
        fat, 47, &architecture
    ) == AARCH64_MACHO_TRUNCATED);

    Write32BE(fat + 36, 0xFFFFFFFFu);
    CHECK(AARCH64MachOFindARM64ESlice(
        fat, sizeof(fat), &architecture
    ) == AARCH64_MACHO_OUT_OF_RANGE);
    CHECK(AARCH64_MACHO_ARM64_RELOC_AUTHENTICATED_POINTER == 11u);
}

static AARCH64PointerAuthStatus PreparePointerAuth(
    AARCH64CPUState *state,
    AARCH64PointerAuthProvider *provider
)
{
    AARCH64PointerAuthKey key;
    AARCH64PointerAuthStatus status;

    if (AARCH64CPUStateReset(state) != AARCH64_CPU_STATE_SUCCESS)
        return AARCH64_POINTER_AUTH_PROVIDER_ERROR;
    status = AARCH64PointerAuthConfigure(
        state, 0x0000FFFFFFFFFFFFULL, 0xFFFF000000000000ULL, 1
    );
    if (status != AARCH64_POINTER_AUTH_SUCCESS)
        return status;

    key.low = 0x123456789ABCDEF0ULL;
    key.high = 0x0FEDCBA987654321ULL;
    status = AARCH64PointerAuthSetKey(state, AARCH64_PAC_KEY_IA, &key);
    if (status != AARCH64_POINTER_AUTH_SUCCESS)
        return status;
    key.low ^= 0x1111111111111111ULL;
    key.high ^= 0x1111111111111111ULL;
    status = AARCH64PointerAuthSetKey(state, AARCH64_PAC_KEY_IB, &key);
    if (status != AARCH64_POINTER_AUTH_SUCCESS)
        return status;
    key.low ^= 0x2222222222222222ULL;
    key.high ^= 0x2222222222222222ULL;
    status = AARCH64PointerAuthSetKey(state, AARCH64_PAC_KEY_DA, &key);
    if (status != AARCH64_POINTER_AUTH_SUCCESS)
        return status;
    key.low ^= 0x4444444444444444ULL;
    key.high ^= 0x4444444444444444ULL;
    status = AARCH64PointerAuthSetKey(state, AARCH64_PAC_KEY_DB, &key);
    if (status != AARCH64_POINTER_AUTH_SUCCESS)
        return status;
    return AARCH64PointerAuthProviderInitSoftware(provider);
}

static void TestPointerAuthOperations(void)
{
    AARCH64CPUState state;
    AARCH64PointerAuthProvider provider;
    AARCH64PointerAuthContext context = {
        0xA1B2C3D4E5F60718ULL, 0, 0x1234, 0
    };
    AARCH64PointerAuthContext changed_context;
    AARCH64PointerAuthKeyID selected_key;
    uint64_t pointer = 0x0000123456789ABCULL;
    uint64_t signed_pointer;
    uint64_t authenticated_pointer;
    uint64_t other_signature;
    uint64_t stripped_pointer;

    CHECK(PreparePointerAuth(&state, &provider) == AARCH64_POINTER_AUTH_SUCCESS);
    CHECK(provider.compatibility_only == 1);

    CHECK(AARCH64PointerAuthSign(
        &state,
        &provider,
        AARCH64_PAC_KEY_IA,
        pointer,
        &context,
        &signed_pointer
    ) == AARCH64_POINTER_AUTH_SUCCESS);
    CHECK(signed_pointer != pointer);
    CHECK((signed_pointer & state.pointer_auth.address_mask) == pointer);
    CHECK((signed_pointer & state.pointer_auth.pac_mask) != 0);
    CHECK(AARCH64PointerAuthAuthenticate(
        &state,
        &provider,
        AARCH64_PAC_KEY_IA,
        signed_pointer,
        &context,
        &authenticated_pointer
    ) == AARCH64_POINTER_AUTH_SUCCESS);
    CHECK(authenticated_pointer == pointer);

    CHECK(AARCH64PointerAuthStrip(
        &state, signed_pointer, &stripped_pointer
    ) == AARCH64_POINTER_AUTH_SUCCESS);
    CHECK(stripped_pointer == pointer);

    changed_context = context;
    changed_context.modifier++;
    CHECK(AARCH64PointerAuthAuthenticate(
        &state,
        &provider,
        AARCH64_PAC_KEY_IA,
        signed_pointer,
        &changed_context,
        &authenticated_pointer
    ) == AARCH64_POINTER_AUTHENTICATION_FAILED);
    CHECK(state.pointer_auth.authentication_failed == 1);
    CHECK(state.pointer_auth.failed_pointer == signed_pointer);
    CHECK((state.pointer_auth.invalid_pointer & (1ULL << 63)) != 0);
    CHECK(state.pointer_auth.failure_count == 1);

    CHECK(AARCH64PointerAuthAuthenticate(
        &state,
        &provider,
        AARCH64_PAC_KEY_IB,
        signed_pointer,
        &context,
        &authenticated_pointer
    ) == AARCH64_POINTER_AUTHENTICATION_FAILED);

    changed_context = context;
    changed_context.address_diversity = 1;
    changed_context.context_address = 0x100000;
    CHECK(AARCH64PointerAuthSign(
        &state,
        &provider,
        AARCH64_PAC_KEY_DA,
        pointer,
        &changed_context,
        &signed_pointer
    ) == AARCH64_POINTER_AUTH_SUCCESS);
    CHECK(AARCH64PointerAuthAuthenticate(
        &state,
        &provider,
        AARCH64_PAC_KEY_DA,
        signed_pointer,
        &changed_context,
        &authenticated_pointer
    ) == AARCH64_POINTER_AUTH_SUCCESS);
    CHECK(authenticated_pointer == pointer);
    changed_context.context_address++;
    CHECK(AARCH64PointerAuthAuthenticate(
        &state,
        &provider,
        AARCH64_PAC_KEY_DA,
        signed_pointer,
        &changed_context,
        &authenticated_pointer
    ) == AARCH64_POINTER_AUTHENTICATION_FAILED);

    changed_context = context;
    changed_context.discriminator++;
    CHECK(AARCH64PointerAuthSign(
        &state,
        &provider,
        AARCH64_PAC_KEY_IA,
        pointer,
        &changed_context,
        &other_signature
    ) == AARCH64_POINTER_AUTH_SUCCESS);
    CHECK(other_signature != signed_pointer);

    CHECK(AARCH64PointerAuthSelectAppleABIKey(
        AARCH64_PAC_USE_GLOBAL_FUNCTION_POINTER, &selected_key
    ) == AARCH64_POINTER_AUTH_SUCCESS);
    CHECK(selected_key == AARCH64_PAC_KEY_IA);
    CHECK(AARCH64PointerAuthSelectAppleABIKey(
        AARCH64_PAC_USE_LOCAL_RETURN_ADDRESS, &selected_key
    ) == AARCH64_POINTER_AUTH_SUCCESS);
    CHECK(selected_key == AARCH64_PAC_KEY_IB);
    CHECK(AARCH64PointerAuthSelectAppleABIKey(
        AARCH64_PAC_USE_GLOBAL_DATA_POINTER, &selected_key
    ) == AARCH64_POINTER_AUTH_SUCCESS);
    CHECK(selected_key == AARCH64_PAC_KEY_DA);
    CHECK(AARCH64PointerAuthSelectAppleABIKey(
        AARCH64_PAC_USE_LOCAL_DATA_POINTER, &selected_key
    ) == AARCH64_POINTER_AUTH_SUCCESS);
    CHECK(selected_key == AARCH64_PAC_KEY_DB);

    CHECK(AARCH64PointerAuthConfigure(
        &state, 0xFFFF000000000000ULL, 0xFFFF000000000000ULL, 1
    ) == AARCH64_POINTER_AUTH_INVALID_ARGUMENT);
    CHECK(AARCH64PointerAuthConfigure(
        &state,
        0x0000FFFFFFFFFFFFULL,
        0x00FF000000000000ULL,
        1
    ) == AARCH64_POINTER_AUTH_SUCCESS);
    pointer = 0xAB00123456789ABCULL;
    CHECK(AARCH64PointerAuthSign(
        &state,
        &provider,
        AARCH64_PAC_KEY_IA,
        pointer,
        &context,
        &signed_pointer
    ) == AARCH64_POINTER_AUTH_SUCCESS);
    CHECK((signed_pointer & 0xFF00000000000000ULL) ==
          (pointer & 0xFF00000000000000ULL));
    CHECK(AARCH64PointerAuthAuthenticate(
        &state,
        &provider,
        AARCH64_PAC_KEY_IA,
        signed_pointer,
        &context,
        &authenticated_pointer
    ) == AARCH64_POINTER_AUTH_SUCCESS);
    CHECK(authenticated_pointer == pointer);
    CHECK(AARCH64PointerAuthConfigure(
        &state, 0x0000FFFFFFFFFFFFULL, 0xFFFF000000000000ULL, 1
    ) == AARCH64_POINTER_AUTH_SUCCESS);
    CHECK(AARCH64PointerAuthSetKey(
        &state, AARCH64_PAC_KEY_INVALID, &state.pointer_auth.api_a
    ) == AARCH64_POINTER_AUTH_INVALID_ARGUMENT);
    CHECK(AARCH64PointerAuthProviderInitSoftware(0) ==
          AARCH64_POINTER_AUTH_INVALID_ARGUMENT);
    CHECK(AARCH64PointerAuthConfigure(
        &state, 0x0000FFFFFFFFFFFFULL, 0xFFFF000000000000ULL, 0
    ) == AARCH64_POINTER_AUTH_SUCCESS);
    CHECK(AARCH64PointerAuthSign(
        &state,
        &provider,
        AARCH64_PAC_KEY_IA,
        pointer,
        &context,
        &signed_pointer
    ) == AARCH64_POINTER_AUTH_NOT_ENABLED);
    CHECK(AARCH64PointerAuthSign(
        0, &provider, AARCH64_PAC_KEY_IA, pointer, &context, &signed_pointer
    ) == AARCH64_POINTER_AUTH_INVALID_ARGUMENT);
}

static void TestPointerAuthControlFlow(void)
{
    AARCH64CPUState state;
    AARCH64PointerAuthProvider provider;
    AARCH64PointerAuthContext context = { 0, 0, 0, 0 };
    uint64_t target = 0x0000000012345000ULL;
    uint64_t signed_target;

    CHECK(PreparePointerAuth(&state, &provider) == AARCH64_POINTER_AUTH_SUCCESS);
    state.sp = 0x00000000ABCDF000ULL;
    state.pc = 0x1000;
    state.x[30] = target;
    CHECK(AARCH64PointerAuthSignLRWithSP(
        &state, &provider, AARCH64_PAC_KEY_IA
    ) == AARCH64_POINTER_AUTH_SUCCESS);
    CHECK(state.x[30] != target);
    CHECK(state.pointer_auth.return_address_signed == 1);
    CHECK(state.pointer_auth.last_modifier == state.sp);

    state.sp += 16;
    CHECK(AARCH64PointerAuthAuthenticateLRWithSP(
        &state, &provider, AARCH64_PAC_KEY_IA
    ) == AARCH64_POINTER_AUTHENTICATION_FAILED);
    CHECK(state.pointer_auth.authentication_failed == 1);
    CHECK((state.x[30] & (1ULL << 63)) != 0);

    CHECK(PreparePointerAuth(&state, &provider) == AARCH64_POINTER_AUTH_SUCCESS);
    state.sp = 0x00000000ABCDF000ULL;
    state.pc = 0x1000;
    state.x[30] = target;
    CHECK(AARCH64PointerAuthSignLRWithSP(
        &state, &provider, AARCH64_PAC_KEY_IB
    ) == AARCH64_POINTER_AUTH_SUCCESS);
    CHECK(AARCH64PointerAuthReturn(
        &state, &provider, AARCH64_PAC_KEY_IB
    ) == AARCH64_POINTER_AUTH_SUCCESS);
    CHECK(state.pc == target);
    CHECK(state.exception.pending == 0);
    CHECK(state.pointer_auth.return_address_signed == 0);

    context.modifier = 0x5566778899AABBCCULL;
    CHECK(AARCH64PointerAuthSign(
        &state,
        &provider,
        AARCH64_PAC_KEY_IA,
        target,
        &context,
        &signed_target
    ) == AARCH64_POINTER_AUTH_SUCCESS);
    state.pc = 0x2000;
    CHECK(AARCH64PointerAuthBranch(
        &state,
        &provider,
        AARCH64_PAC_KEY_IA,
        signed_target,
        context.modifier,
        1,
        0x2004
    ) == AARCH64_POINTER_AUTH_SUCCESS);
    CHECK(state.pc == target);
    CHECK(state.x[30] == 0x2004);

    state.pc = 0x3000;
    CHECK(AARCH64PointerAuthBranch(
        &state,
        &provider,
        AARCH64_PAC_KEY_IA,
        signed_target,
        context.modifier + 1,
        0,
        0
    ) == AARCH64_POINTER_AUTHENTICATION_FAILED);
    CHECK(state.pc == 0x3000);
    CHECK(state.exception.pending == 1);
    CHECK(state.exception.code == AARCH64_CPU_EXCEPTION_POINTER_AUTH);
}

typedef struct {
    uint32_t encoding;
    AARCH64Opcode opcode;
    AARCH64AuthKey key;
} EncodedInstruction;

static void TestPointerAuthDecoder(void)
{
    static const EncodedInstruction cases[] = {
        { 0xDAC10020u, AARCH64_OPCODE_PACIA, AARCH64_AUTH_KEY_IA },
        { 0xDAC10462u, AARCH64_OPCODE_PACIB, AARCH64_AUTH_KEY_IB },
        { 0xDAC108A4u, AARCH64_OPCODE_PACDA, AARCH64_AUTH_KEY_DA },
        { 0xDAC10CE6u, AARCH64_OPCODE_PACDB, AARCH64_AUTH_KEY_DB },
        { 0xDAC123E8u, AARCH64_OPCODE_PACIZA, AARCH64_AUTH_KEY_IA },
        { 0xDAC127E9u, AARCH64_OPCODE_PACIZB, AARCH64_AUTH_KEY_IB },
        { 0xDAC12BEAu, AARCH64_OPCODE_PACDZA, AARCH64_AUTH_KEY_DA },
        { 0xDAC12FEBu, AARCH64_OPCODE_PACDZB, AARCH64_AUTH_KEY_DB },
        { 0xDAC111ACu, AARCH64_OPCODE_AUTIA, AARCH64_AUTH_KEY_IA },
        { 0xDAC115EEu, AARCH64_OPCODE_AUTIB, AARCH64_AUTH_KEY_IB },
        { 0xDAC11A30u, AARCH64_OPCODE_AUTDA, AARCH64_AUTH_KEY_DA },
        { 0xDAC11E72u, AARCH64_OPCODE_AUTDB, AARCH64_AUTH_KEY_DB },
        { 0xDAC133F4u, AARCH64_OPCODE_AUTIZA, AARCH64_AUTH_KEY_IA },
        { 0xDAC137F5u, AARCH64_OPCODE_AUTIZB, AARCH64_AUTH_KEY_IB },
        { 0xDAC13BF6u, AARCH64_OPCODE_AUTDZA, AARCH64_AUTH_KEY_DA },
        { 0xDAC13FF7u, AARCH64_OPCODE_AUTDZB, AARCH64_AUTH_KEY_DB },
        { 0xDAC143F8u, AARCH64_OPCODE_XPACI, AARCH64_AUTH_KEY_IA },
        { 0xDAC147F9u, AARCH64_OPCODE_XPACD, AARCH64_AUTH_KEY_DA },
        { 0xD503233Fu, AARCH64_OPCODE_PACIASP, AARCH64_AUTH_KEY_IA },
        { 0xD503237Fu, AARCH64_OPCODE_PACIBSP, AARCH64_AUTH_KEY_IB },
        { 0xD50323BFu, AARCH64_OPCODE_AUTIASP, AARCH64_AUTH_KEY_IA },
        { 0xD50323FFu, AARCH64_OPCODE_AUTIBSP, AARCH64_AUTH_KEY_IB },
        { 0xD65F0BFFu, AARCH64_OPCODE_RETAA, AARCH64_AUTH_KEY_IA },
        { 0xD65F0FFFu, AARCH64_OPCODE_RETAB, AARCH64_AUTH_KEY_IB },
        { 0xD71F0801u, AARCH64_OPCODE_BRAA, AARCH64_AUTH_KEY_IA },
        { 0xD71F0C43u, AARCH64_OPCODE_BRAB, AARCH64_AUTH_KEY_IB },
        { 0xD73F0885u, AARCH64_OPCODE_BLRAA, AARCH64_AUTH_KEY_IA },
        { 0xD73F0CC7u, AARCH64_OPCODE_BLRAB, AARCH64_AUTH_KEY_IB },
        { 0xD61F08BFu, AARCH64_OPCODE_BRAAZ, AARCH64_AUTH_KEY_IA },
        { 0xD61F0CDFu, AARCH64_OPCODE_BRABZ, AARCH64_AUTH_KEY_IB },
        { 0xD63F08FFu, AARCH64_OPCODE_BLRAAZ, AARCH64_AUTH_KEY_IA },
        { 0xD63F0D1Fu, AARCH64_OPCODE_BLRABZ, AARCH64_AUTH_KEY_IB },
        { 0xF8200528u, AARCH64_OPCODE_LDRAA, AARCH64_AUTH_KEY_DA },
        { 0xF8A0056Au, AARCH64_OPCODE_LDRAB, AARCH64_AUTH_KEY_DB }
    };
    uint32_t i;
    AARCH64Instruction instruction;
    AARCH64Opcode opcode;
    const char *mnemonic;

    for (i = 0; i < (uint32_t)(sizeof(cases) / sizeof(cases[0])); i++) {
        CHECK(AARCH64Classify(cases[i].encoding, &opcode) ==
              ARMITO_DECODER_SUCCESS);
        CHECK(opcode == cases[i].opcode);
        CHECK(AARCH64Decode(cases[i].encoding, &instruction) ==
              ARMITO_DECODER_SUCCESS);
        CHECK(instruction.opcode == cases[i].opcode);
        CHECK(instruction.auth_key == cases[i].key);
        CHECK(AARCH64ClassifyMnemonic(
            cases[i].encoding, &mnemonic
        ) == ARMITO_DECODER_SUCCESS);
        CHECK(strcmp(mnemonic, AARCH64OpcodeName(cases[i].opcode)) == 0);
    }

    CHECK(AARCH64Decode(0xDAC10020u, &instruction) == ARMITO_DECODER_SUCCESS);
    CHECK(instruction.rd_index == 0);
    CHECK(instruction.rn_index == 1);
    CHECK(instruction.rn_is_zero == 0);

    CHECK(AARCH64Decode(0xDAC123E8u, &instruction) == ARMITO_DECODER_SUCCESS);
    CHECK(instruction.rd_index == 8);
    CHECK(instruction.auth_modifier_is_zero == 1);
    CHECK(instruction.rn_is_zero == 1);

    CHECK(AARCH64Decode(0xD503233Fu, &instruction) == ARMITO_DECODER_SUCCESS);
    CHECK(instruction.rd_index == 30);
    CHECK(instruction.auth_modifier_is_sp == 1);

    CHECK(AARCH64Decode(0xD71F0801u, &instruction) == ARMITO_DECODER_SUCCESS);
    CHECK(instruction.rn_index == 0);
    CHECK(instruction.rm_index == 1);

    CHECK(AARCH64Decode(0xD61F08BFu, &instruction) == ARMITO_DECODER_SUCCESS);
    CHECK(instruction.rn_index == 5);
    CHECK(instruction.auth_modifier_is_zero == 1);

    CHECK(AARCH64Decode(0xF8201420u, &instruction) == ARMITO_DECODER_SUCCESS);
    CHECK(instruction.rd_index == 0);
    CHECK(instruction.rn_index == 1);
    CHECK(instruction.immediate == 8);
    CHECK(instruction.auth_address_diversity == 1);

    CHECK(AARCH64Decode(0xF87FE462u, &instruction) == ARMITO_DECODER_SUCCESS);
    CHECK(instruction.immediate == -16);

    CHECK(AARCH64Classify(0xDAC12000u, &opcode) ==
          ARMITO_DECODER_UNSUPPORTED_INSTRUCTION);
    CHECK(AARCH64Classify(0xD503233Eu, &opcode) ==
          ARMITO_DECODER_UNSUPPORTED_INSTRUCTION);
    CHECK(AARCH64Classify(0xF8200928u, &opcode) ==
          ARMITO_DECODER_UNSUPPORTED_INSTRUCTION);
    CHECK(AARCH64Classify(0xF8201C20u, &opcode) ==
          ARMITO_DECODER_UNSUPPORTED_INSTRUCTION);
    CHECK(AARCH64Classify(0xF8FFEC62u, &opcode) ==
          ARMITO_DECODER_UNSUPPORTED_INSTRUCTION);
    CHECK(AARCH64Classify(0xFFFFFFFFu, &opcode) ==
          ARMITO_DECODER_UNSUPPORTED_INSTRUCTION);
}

static void TestPointerAuthIRInstruction(
    uint32_t encoding,
    ArmitoIROperationKind expected_kind,
    ArmitoIRPointerAuthKey expected_key,
    uint8_t expected_flags,
    uint8_t expected_effect_class,
    uint8_t expected_effect_index,
    uint8_t expected_input_count
)
{
    ArmitoDecoderConfig config;
    ArmitoDecoderInstruction source_instruction;
    ArmitoDecodeResult result;
    ArmitoIROperation operation;

    config.isa = ARMITO_ISA_AARCH64;
    config.entry_address = 0x1000;
    config.max_instructions = 8;
    config.stop_at_branch = 1;
    config.record_unsupported = 1;

    CHECK(ArmitoDecoderInitialize(&config) == ARMITO_DECODER_SUCCESS);
    source_instruction.offset = 0;
    source_instruction.address = 0x1000;
    source_instruction.encoding = encoding;
    CHECK(ArmitoDecoderDecodeInstruction(
        &source_instruction, &result
    ) == ARMITO_DECODER_UNSUPPORTED_INSTRUCTION);
    CHECK(result == ARMITO_DECODE_UNSUPPORTED);
    CHECK(ArmitoIROperationsCount() == 1);
    CHECK(ArmitoIROperationGet(1, &operation) == ARMITO_IR_SUCCESS);
    CHECK(operation.kind == expected_kind);
    CHECK(operation.pointer_auth_key == expected_key);
    CHECK(operation.pointer_auth_flags == expected_flags);
    CHECK(operation.input_count == expected_input_count);
    CHECK(operation.source_encoding == encoding);
    CHECK(operation.source_length == 4);
    if (expected_effect_class != 0xFFu) {
        CHECK(operation.has_effect_register == 1);
        CHECK(operation.effect_register_class == expected_effect_class);
        CHECK(operation.effect_register_index == expected_effect_index);
    } else {
        CHECK(operation.has_effect_register == 0);
    }
    CHECK(ArmitoIRValidate() == ARMITO_IR_SUCCESS);
}

static void TestPointerAuthIR(void)
{
    ArmitoIRTypeID type_u64;
    ArmitoIRValueID input;
    ArmitoIROperationID operation_id;
    ArmitoIROperation operation;

    TestPointerAuthIRInstruction(
        0xDAC10020u,
        ARMITO_IR_OP_PAC_SIGN,
        ARMITO_IR_PAC_KEY_IA,
        0,
        ARMITO_IR_REG_GPR,
        0,
        2
    );
    TestPointerAuthIRInstruction(
        0xDAC123E8u,
        ARMITO_IR_OP_PAC_SIGN,
        ARMITO_IR_PAC_KEY_IA,
        ARMITO_IR_PAC_FLAG_MODIFIER_IS_ZERO,
        ARMITO_IR_REG_GPR,
        8,
        2
    );
    TestPointerAuthIRInstruction(
        0xD50323BFu,
        ARMITO_IR_OP_PAC_AUTHENTICATE,
        ARMITO_IR_PAC_KEY_IA,
        ARMITO_IR_PAC_FLAG_FAIL_CLOSED |
            ARMITO_IR_PAC_FLAG_MODIFIER_IS_SP,
        ARMITO_IR_REG_GPR,
        30,
        2
    );
    TestPointerAuthIRInstruction(
        0xDAC143F8u,
        ARMITO_IR_OP_PAC_STRIP,
        ARMITO_IR_PAC_KEY_IA,
        0,
        ARMITO_IR_REG_GPR,
        24,
        1
    );
    TestPointerAuthIRInstruction(
        0xD71F0801u,
        ARMITO_IR_OP_PAC_BRANCH_AUTHENTICATED,
        ARMITO_IR_PAC_KEY_IA,
        ARMITO_IR_PAC_FLAG_FAIL_CLOSED,
        ARMITO_IR_REG_PC,
        0,
        2
    );
    TestPointerAuthIRInstruction(
        0xD73F0CC7u,
        ARMITO_IR_OP_PAC_CALL_AUTHENTICATED,
        ARMITO_IR_PAC_KEY_IB,
        ARMITO_IR_PAC_FLAG_FAIL_CLOSED |
            ARMITO_IR_PAC_FLAG_WRITES_LINK_REGISTER,
        ARMITO_IR_REG_PC,
        0,
        2
    );
    TestPointerAuthIRInstruction(
        0xD65F0BFFu,
        ARMITO_IR_OP_PAC_RETURN_AUTHENTICATED,
        ARMITO_IR_PAC_KEY_IA,
        ARMITO_IR_PAC_FLAG_FAIL_CLOSED |
            ARMITO_IR_PAC_FLAG_MODIFIER_IS_SP,
        ARMITO_IR_REG_PC,
        0,
        2
    );
    TestPointerAuthIRInstruction(
        0xF8200528u,
        ARMITO_IR_OP_PAC_LOAD_AUTHENTICATED,
        ARMITO_IR_PAC_KEY_DA,
        ARMITO_IR_PAC_FLAG_ADDRESS_DIVERSITY |
            ARMITO_IR_PAC_FLAG_FAIL_CLOSED,
        ARMITO_IR_REG_GPR,
        8,
        2
    );

    CHECK(strcmp(
        ArmitoIROperationName(ARMITO_IR_OP_PAC_SIGN), "pac.sign"
    ) == 0);
    CHECK(strcmp(
        ArmitoIROperationName(ARMITO_IR_OP_PAC_AUTHENTICATION_FAILURE),
        "pac.authentication.failure"
    ) == 0);

    CHECK(ArmitoIRInitialize() == ARMITO_IR_SUCCESS);
    CHECK(ArmitoIRTypesRegister(
        ARMITO_IR_CLASS_INT, 64, 1, 0, 0, "u64", &type_u64
    ) == ARMITO_IR_SUCCESS);
    CHECK(ArmitoIRValuesDeclare(
        ARMITO_IR_VALUE_IMMEDIATE, type_u64, "invalid-pointer", &input
    ) == ARMITO_IR_SUCCESS);
    CHECK(ArmitoIROperationEmit(
        ARMITO_IR_OP_PAC_AUTHENTICATION_FAILURE,
        ARMITO_IR_TYPE_INVALID,
        ARMITO_IR_VALUE_INVALID,
        &input,
        1,
        &operation_id
    ) == ARMITO_IR_SUCCESS);
    CHECK(ArmitoIROperationSetPointerAuth(
        operation_id,
        ARMITO_IR_PAC_KEY_IA,
        ARMITO_IR_PAC_FLAG_FAIL_CLOSED |
            ARMITO_IR_PAC_FLAG_ADDRESS_DIVERSITY,
        0x1234
    ) == ARMITO_IR_SUCCESS);
    CHECK(ArmitoIROperationGet(operation_id, &operation) == ARMITO_IR_SUCCESS);
    CHECK(operation.kind == ARMITO_IR_OP_PAC_AUTHENTICATION_FAILURE);
    CHECK(operation.pointer_auth_discriminator == 0x1234);
    CHECK((operation.pointer_auth_flags &
           ARMITO_IR_PAC_FLAG_ADDRESS_DIVERSITY) != 0);
    CHECK(ArmitoIRValidate() == ARMITO_IR_SUCCESS);

    CHECK(ArmitoIRInitialize() == ARMITO_IR_SUCCESS);
    CHECK(ArmitoIRTypesRegister(
        ARMITO_IR_CLASS_INT, 64, 1, 0, 0, "u64", &type_u64
    ) == ARMITO_IR_SUCCESS);
    CHECK(ArmitoIRValuesDeclare(
        ARMITO_IR_VALUE_IMMEDIATE, type_u64, "pointer", &input
    ) == ARMITO_IR_SUCCESS);
    CHECK(ArmitoIROperationEmit(
        ARMITO_IR_OP_PAC_SIGN,
        type_u64,
        ARMITO_IR_VALUE_INVALID,
        &input,
        1,
        &operation_id
    ) == ARMITO_IR_SUCCESS);
    CHECK(ArmitoIROperationSetPointerAuth(
        operation_id, ARMITO_IR_PAC_KEY_IA, 0, 0
    ) == ARMITO_IR_SUCCESS);
    CHECK(ArmitoIRValidate() == ARMITO_IR_SEMANTIC_ERROR);
}

static void TestAuthenticatedBranchStopsBlock(void)
{
    static const uint8_t instructions[] = {
        0x01, 0x08, 0x1F, 0xD7,
        0x1F, 0x20, 0x03, 0xD5
    };
    ArmitoDecoderConfig config;
    ArmitoDecoderSummary summary;

    config.isa = ARMITO_ISA_AARCH64;
    config.entry_address = 0x1000;
    config.max_instructions = 8;
    config.stop_at_branch = 1;
    config.record_unsupported = 1;

    CHECK(ArmitoDecoderInitialize(&config) == ARMITO_DECODER_SUCCESS);
    CHECK(ArmitoDecoderSourceSet(
        instructions,
        sizeof(instructions),
        0x1000,
        1,
        1
    ) == ARMITO_DECODER_SUCCESS);
    CHECK(ArmitoDecoderDecodeBlock(0x1000, &summary) ==
          ARMITO_DECODER_SUCCESS);
    CHECK(summary.instructions_unsupported == 1);
    CHECK(summary.instructions_decoded == 0);
    CHECK(summary.next_address == 0x1004);
    CHECK(ArmitoIROperationsCount() == 1);
}

int main(void)
{
    TestMachOSubtypes();
    TestPointerAuthOperations();
    TestPointerAuthControlFlow();
    TestPointerAuthDecoder();
    TestPointerAuthIR();
    TestAuthenticatedBranchStopsBlock();

    printf(
        "ARM64E: %u checks, %u failures\n",
        tests_run,
        tests_failed
    );
    return tests_failed == 0 ? 0 : 1;
}
