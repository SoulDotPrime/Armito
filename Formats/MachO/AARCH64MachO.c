#include "Includes/AARCH64MachO.h"

#define AARCH64_MACHO_MH_MAGIC_64 0xFEEDFACFu
#define AARCH64_MACHO_FAT_MAGIC 0xCAFEBABEu
#define AARCH64_MACHO_FAT_MAGIC_64 0xCAFEBABFu
#define AARCH64_MACHO_FAT_CIGAM 0xBEBAFECAu
#define AARCH64_MACHO_FAT_CIGAM_64 0xBFBAFECAu
#define AARCH64_MACHO_HEADER_64_SIZE 32u
#define AARCH64_MACHO_FAT_HEADER_SIZE 8u
#define AARCH64_MACHO_FAT_ARCH_SIZE 20u
#define AARCH64_MACHO_FAT_ARCH_64_SIZE 32u

static uint32_t AARCH64MachORead32LE(const uint8_t *data)
{
    return (uint32_t)data[0] |
           ((uint32_t)data[1] << 8) |
           ((uint32_t)data[2] << 16) |
           ((uint32_t)data[3] << 24);
}

static uint32_t AARCH64MachORead32BE(const uint8_t *data)
{
    return ((uint32_t)data[0] << 24) |
           ((uint32_t)data[1] << 16) |
           ((uint32_t)data[2] << 8) |
           (uint32_t)data[3];
}

static uint64_t AARCH64MachORead64BE(const uint8_t *data)
{
    return ((uint64_t)AARCH64MachORead32BE(data) << 32) |
           (uint64_t)AARCH64MachORead32BE(data + 4);
}

AARCH64MachOStatus AARCH64MachOIdentifyArchitecture(
    uint32_t cpu_type,
    uint32_t raw_cpu_subtype,
    AARCH64MachOArchitecture *architecture
)
{
    uint32_t subtype_value;

    if (architecture == 0)
        return AARCH64_MACHO_INVALID_ARGUMENT;

    if (cpu_type != AARCH64_MACHO_CPU_TYPE_ARM64)
        return AARCH64_MACHO_UNSUPPORTED_ARCH;

    subtype_value = raw_cpu_subtype & ~AARCH64_MACHO_CPU_SUBTYPE_MASK;
    architecture->cpu_type = cpu_type;
    architecture->raw_cpu_subtype = raw_cpu_subtype;
    architecture->subtype_value = subtype_value;
    architecture->capability_bits =
        raw_cpu_subtype & AARCH64_MACHO_CPU_SUBTYPE_MASK;
    architecture->pointer_auth_abi_version = 0;
    architecture->kernel_pointer_auth_abi_version = 0;
    architecture->slice_offset = 0;
    architecture->slice_size = 0;
    architecture->is_arm64e = 0;
    architecture->versioned_pointer_auth_abi = 0;
    architecture->kernel_pointer_auth_abi = 0;

    switch (subtype_value) {
    case AARCH64_MACHO_CPU_SUBTYPE_ARM64_ALL:
        architecture->subtype = AARCH64_MACHO_SUBTYPE_ARM64_ALL;
        return AARCH64_MACHO_SUCCESS;
    case AARCH64_MACHO_CPU_SUBTYPE_ARM64_V8:
        architecture->subtype = AARCH64_MACHO_SUBTYPE_ARM64_V8;
        return AARCH64_MACHO_SUCCESS;
    case AARCH64_MACHO_CPU_SUBTYPE_ARM64E:
        architecture->subtype = AARCH64_MACHO_SUBTYPE_ARM64E;
        architecture->is_arm64e = 1;
        architecture->versioned_pointer_auth_abi =
            (uint8_t)((raw_cpu_subtype &
                       AARCH64_MACHO_CPU_SUBTYPE_VERSIONED_PTRAUTH_ABI) != 0);
        architecture->kernel_pointer_auth_abi =
            (uint8_t)((raw_cpu_subtype &
                       AARCH64_MACHO_CPU_SUBTYPE_KERNEL_PTRAUTH_ABI) != 0);
        architecture->pointer_auth_abi_version =
            (raw_cpu_subtype &
             AARCH64_MACHO_CPU_SUBTYPE_PTRAUTH_VERSION_MASK) >>
            AARCH64_MACHO_CPU_SUBTYPE_PTRAUTH_VERSION_SHIFT;
        if (architecture->kernel_pointer_auth_abi)
            architecture->kernel_pointer_auth_abi_version =
                architecture->pointer_auth_abi_version;
        return AARCH64_MACHO_SUCCESS;
    case AARCH64_MACHO_CPU_SUBTYPE_ARM64E_X1:
        architecture->subtype = AARCH64_MACHO_SUBTYPE_ARM64E_X1;
        return AARCH64_MACHO_SUCCESS;
    default:
        architecture->subtype = AARCH64_MACHO_SUBTYPE_UNKNOWN;
        return AARCH64_MACHO_INVALID_SUBTYPE;
    }
}

AARCH64MachOStatus AARCH64MachOParse64Header(
    const uint8_t *data,
    uint64_t size,
    AARCH64MachOArchitecture *architecture
)
{
    AARCH64MachOStatus status;

    if (data == 0 || architecture == 0)
        return AARCH64_MACHO_INVALID_ARGUMENT;

    if (size < AARCH64_MACHO_HEADER_64_SIZE)
        return AARCH64_MACHO_TRUNCATED;

    if (AARCH64MachORead32LE(data) != AARCH64_MACHO_MH_MAGIC_64)
        return AARCH64_MACHO_INVALID_FORMAT;

    status = AARCH64MachOIdentifyArchitecture(
        AARCH64MachORead32LE(data + 4),
        AARCH64MachORead32LE(data + 8),
        architecture
    );
    if (status != AARCH64_MACHO_SUCCESS)
        return status;

    architecture->slice_size = size;
    return AARCH64_MACHO_SUCCESS;
}

static AARCH64MachOStatus AARCH64MachOReadFatArchitecture(
    const uint8_t *data,
    uint64_t size,
    uint8_t is_big_endian,
    uint8_t is_64bit,
    AARCH64MachOArchitecture *architecture
)
{
    uint32_t architecture_count;
    uint32_t entry_size;
    uint64_t table_size;
    uint32_t i;

    if (size < AARCH64_MACHO_FAT_HEADER_SIZE)
        return AARCH64_MACHO_TRUNCATED;

    architecture_count = is_big_endian
        ? AARCH64MachORead32BE(data + 4)
        : AARCH64MachORead32LE(data + 4);
    entry_size = is_64bit
        ? AARCH64_MACHO_FAT_ARCH_64_SIZE
        : AARCH64_MACHO_FAT_ARCH_SIZE;

    if ((uint64_t)architecture_count >
        (size - AARCH64_MACHO_FAT_HEADER_SIZE) / entry_size)
        return AARCH64_MACHO_TRUNCATED;

    table_size = (uint64_t)architecture_count * entry_size;
    if (table_size > size - AARCH64_MACHO_FAT_HEADER_SIZE)
        return AARCH64_MACHO_TRUNCATED;

    for (i = 0; i < architecture_count; i++) {
        const uint8_t *entry =
            data + AARCH64_MACHO_FAT_HEADER_SIZE + (uint64_t)i * entry_size;
        uint32_t cpu_type = is_big_endian
            ? AARCH64MachORead32BE(entry)
            : AARCH64MachORead32LE(entry);
        uint32_t raw_cpu_subtype = is_big_endian
            ? AARCH64MachORead32BE(entry + 4)
            : AARCH64MachORead32LE(entry + 4);
        uint64_t slice_offset;
        uint64_t slice_size;
        AARCH64MachOArchitecture thin_architecture;
        AARCH64MachOStatus status;

        if (is_64bit) {
            slice_offset = is_big_endian
                ? AARCH64MachORead64BE(entry + 8)
                : ((uint64_t)AARCH64MachORead32LE(entry + 12) << 32) |
                  AARCH64MachORead32LE(entry + 8);
            slice_size = is_big_endian
                ? AARCH64MachORead64BE(entry + 16)
                : ((uint64_t)AARCH64MachORead32LE(entry + 20) << 32) |
                  AARCH64MachORead32LE(entry + 16);
        } else {
            slice_offset = is_big_endian
                ? AARCH64MachORead32BE(entry + 8)
                : AARCH64MachORead32LE(entry + 8);
            slice_size = is_big_endian
                ? AARCH64MachORead32BE(entry + 12)
                : AARCH64MachORead32LE(entry + 12);
        }

        if (slice_offset > size || slice_size > size - slice_offset)
            return AARCH64_MACHO_OUT_OF_RANGE;

        if (cpu_type != AARCH64_MACHO_CPU_TYPE_ARM64 ||
            (raw_cpu_subtype & ~AARCH64_MACHO_CPU_SUBTYPE_MASK) !=
                AARCH64_MACHO_CPU_SUBTYPE_ARM64E)
            continue;

        status = AARCH64MachOParse64Header(
            data + slice_offset, slice_size, &thin_architecture
        );
        if (status != AARCH64_MACHO_SUCCESS)
            return status;

        if (thin_architecture.cpu_type != cpu_type ||
            thin_architecture.raw_cpu_subtype != raw_cpu_subtype)
            return AARCH64_MACHO_INVALID_FORMAT;

        thin_architecture.slice_offset = slice_offset;
        *architecture = thin_architecture;
        return AARCH64_MACHO_SUCCESS;
    }

    return AARCH64_MACHO_NOT_FOUND;
}

AARCH64MachOStatus AARCH64MachOFindARM64ESlice(
    const uint8_t *data,
    uint64_t size,
    AARCH64MachOArchitecture *architecture
)
{
    uint32_t magic;
    AARCH64MachOStatus status;

    if (data == 0 || architecture == 0)
        return AARCH64_MACHO_INVALID_ARGUMENT;

    if (size < 4)
        return AARCH64_MACHO_TRUNCATED;

    magic = AARCH64MachORead32BE(data);
    if (magic == AARCH64_MACHO_FAT_MAGIC)
        return AARCH64MachOReadFatArchitecture(
            data, size, 1, 0, architecture
        );
    if (magic == AARCH64_MACHO_FAT_MAGIC_64)
        return AARCH64MachOReadFatArchitecture(
            data, size, 1, 1, architecture
        );
    if (magic == AARCH64_MACHO_FAT_CIGAM)
        return AARCH64MachOReadFatArchitecture(
            data, size, 0, 0, architecture
        );
    if (magic == AARCH64_MACHO_FAT_CIGAM_64)
        return AARCH64MachOReadFatArchitecture(
            data, size, 0, 1, architecture
        );

    status = AARCH64MachOParse64Header(data, size, architecture);
    if (status != AARCH64_MACHO_SUCCESS)
        return status;

    return architecture->is_arm64e
        ? AARCH64_MACHO_SUCCESS
        : AARCH64_MACHO_NOT_FOUND;
}
