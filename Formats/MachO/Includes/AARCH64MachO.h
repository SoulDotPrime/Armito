#ifndef ARMITO_AARCH64_MACHO_H
#define ARMITO_AARCH64_MACHO_H

#include <stdint.h>

#define AARCH64_MACHO_CPU_TYPE_ARM64 0x0100000Cu
#define AARCH64_MACHO_CPU_SUBTYPE_MASK 0xFF000000u
#define AARCH64_MACHO_CPU_SUBTYPE_ARM64_ALL 0u
#define AARCH64_MACHO_CPU_SUBTYPE_ARM64_V8 1u
#define AARCH64_MACHO_CPU_SUBTYPE_ARM64E 2u
#define AARCH64_MACHO_CPU_SUBTYPE_ARM64E_X1 12u

#define AARCH64_MACHO_CPU_SUBTYPE_VERSIONED_PTRAUTH_ABI 0x80000000u
#define AARCH64_MACHO_CPU_SUBTYPE_KERNEL_PTRAUTH_ABI 0x40000000u
#define AARCH64_MACHO_CPU_SUBTYPE_PTRAUTH_VERSION_MASK 0x3F000000u
#define AARCH64_MACHO_CPU_SUBTYPE_PTRAUTH_VERSION_SHIFT 24u

#define AARCH64_MACHO_ARM64_RELOC_AUTHENTICATED_POINTER 11u

typedef enum {
    AARCH64_MACHO_SUCCESS = 0,
    AARCH64_MACHO_INVALID_ARGUMENT,
    AARCH64_MACHO_TRUNCATED,
    AARCH64_MACHO_INVALID_FORMAT,
    AARCH64_MACHO_UNSUPPORTED_ARCH,
    AARCH64_MACHO_INVALID_SUBTYPE,
    AARCH64_MACHO_NOT_FOUND,
    AARCH64_MACHO_OUT_OF_RANGE
} AARCH64MachOStatus;

typedef enum {
    AARCH64_MACHO_SUBTYPE_UNKNOWN = 0,
    AARCH64_MACHO_SUBTYPE_ARM64_ALL,
    AARCH64_MACHO_SUBTYPE_ARM64_V8,
    AARCH64_MACHO_SUBTYPE_ARM64E,
    AARCH64_MACHO_SUBTYPE_ARM64E_X1
} AARCH64MachOSubtype;

typedef struct {
    uint32_t cpu_type;
    uint32_t raw_cpu_subtype;
    uint32_t subtype_value;
    uint32_t capability_bits;
    uint32_t pointer_auth_abi_version;
    uint32_t kernel_pointer_auth_abi_version;
    uint64_t slice_offset;
    uint64_t slice_size;
    AARCH64MachOSubtype subtype;
    uint8_t is_arm64e;
    uint8_t versioned_pointer_auth_abi;
    uint8_t kernel_pointer_auth_abi;
} AARCH64MachOArchitecture;

AARCH64MachOStatus AARCH64MachOIdentifyArchitecture(
    uint32_t cpu_type,
    uint32_t raw_cpu_subtype,
    AARCH64MachOArchitecture *architecture
);
AARCH64MachOStatus AARCH64MachOParse64Header(
    const uint8_t *data,
    uint64_t size,
    AARCH64MachOArchitecture *architecture
);
AARCH64MachOStatus AARCH64MachOFindARM64ESlice(
    const uint8_t *data,
    uint64_t size,
    AARCH64MachOArchitecture *architecture
);

#endif
