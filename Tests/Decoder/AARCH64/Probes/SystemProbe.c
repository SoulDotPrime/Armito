#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "Decoder/Includes/Decoder.h"
#include "AARCH64/Includes/AARCH64.h"
#include "AARCH64/Catalog/AARCH64Catalog.h"

int main(int argc, char **argv)
{
    ArmitoDecoderConfig config;
    ArmitoDecoderInstruction instruction;
    ArmitoDecoderStatus status;
    ArmitoDecodeResult result;
    AARCH64OperandSet operands;
    char identifier[160];
    uint32_t word;
    uint64_t address;

    if (argc != 1)
        return 1;

    (void)argv;

    memset(&config, 0, sizeof(config));
    config.isa = ARMITO_ISA_AARCH64;
    config.entry_address = 0x1000;
    config.max_instructions = 16;
    config.stop_at_branch = 1;
    config.record_unsupported = 0;

    if (ArmitoDecoderInitialize(&config) != ARMITO_DECODER_SUCCESS)
        return 1;

    while (scanf("%159s %x %llx", identifier, &word, &address) == 3) {
        instruction.offset = 0;
        instruction.address = address;
        instruction.encoding = (uint32_t)word;

        memset(&operands, 0, sizeof(operands));
        status = ArmitoDecoderDecodeInstructionWithOperands(
            &instruction, &result, &operands
        );

        if (status != ARMITO_DECODER_SUCCESS ||
            result != ARMITO_DECODE_OK) {
            printf("%s\t%08X\tUNSUPPORTED\n", identifier, (uint32_t)word);
            continue;
        }

        if (operands.status != AARCH64_OPERAND_EXTRACTION_COMPLETE) {
            printf("%s\t%08X\tNO-OPERANDS\n", identifier, (uint32_t)word);
            continue;
        }

        printf("%s\t%08X\tOK\tCOMPLETE", identifier, (uint32_t)word);
        printf("\tSYS:%u:%u:%u:%u:%u:%u:%u:%u:%u:%u:%u:%u:%u:%s:%u:%u:%u:%u",
            operands.system_op0, operands.system_op1,
            operands.system_crn, operands.system_crm,
            operands.system_op2, operands.system_rt,
            operands.system_register, operands.system_access,
            operands.system_is_implementation_defined,
            operands.system_is_sysl,
            operands.barrier_domain, operands.barrier_type,
            operands.hint_immediate,
            operands.system_register_name
                ? operands.system_register_name : "null",
            operands.pstate_field,
            operands.system_register_identity,
            0u, 0u);
        putchar('\n');
    }

    ArmitoDecoderShutdown();

    return ferror(stdin) || ferror(stdout) ? 1 : 0;
}