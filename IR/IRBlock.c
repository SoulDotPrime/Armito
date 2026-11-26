#include "Includes/IRBlock.h"

static ArmitoIRBlock ir_block_table[ARMITO_IR_MAX_BLOCKS];
static uint32_t ir_block_total = 0;
static uint32_t ir_open_block_index = ARMITO_IR_INDEX_INVALID;

static void IRBlockCopyName(char *dest, const char *src)
{
    uint32_t i = 0;

    if (dest == NULL)
        return;

    if (src == NULL) {
        dest[0] = '\0';
        return;
    }

    while (src[i] != '\0' && i < (ARMITO_IR_MAX_NAME - 1)) {
        dest[i] = src[i];
        i++;
    }

    dest[i] = '\0';
}

ArmitoIRStatus ArmitoIRBlocksReset(void)
{
    uint32_t i;

    for (i = 0; i < ARMITO_IR_MAX_BLOCKS; i++) {
        ir_block_table[i].id = ARMITO_IR_LABEL_INVALID;
        ir_block_table[i].first_operation = 0;
        ir_block_table[i].operation_count = 0;
        ir_block_table[i].guest_address = 0;
        ir_block_table[i].terminated = 0;
        ir_block_table[i].name[0] = '\0';
    }

    ir_block_total = 0;
    ir_open_block_index = ARMITO_IR_INDEX_INVALID;

    return ARMITO_IR_SUCCESS;
}

ArmitoIRStatus ArmitoIRBlockBegin(
    uint64_t guest_address,
    const char *name,
    ArmitoIRLabelID *block_id
)
{
    ArmitoIRBlock *block;

    if (block_id == NULL)
        return ARMITO_IR_INVALID_ARGUMENT;

    *block_id = ARMITO_IR_LABEL_INVALID;

    if (ir_open_block_index != ARMITO_IR_INDEX_INVALID)
        return ARMITO_IR_INVALID_STATE;

    if (ir_block_total >= ARMITO_IR_MAX_BLOCKS)
        return ARMITO_IR_OUT_OF_RESOURCES;

    block = &ir_block_table[ir_block_total];
    block->id = ir_block_total + 1;
    block->first_operation = ArmitoIROperationsCount();
    block->operation_count = 0;
    block->guest_address = guest_address;
    block->terminated = 0;
    IRBlockCopyName(block->name, name);

    ir_open_block_index = ir_block_total;
    ir_block_total++;

    *block_id = block->id;

    return ARMITO_IR_SUCCESS;
}

ArmitoIRStatus ArmitoIRBlockEnd(void)
{
    ArmitoIRBlock *block;

    if (ir_open_block_index == ARMITO_IR_INDEX_INVALID)
        return ARMITO_IR_INVALID_STATE;

    block = &ir_block_table[ir_open_block_index];
    block->operation_count = ArmitoIROperationsCount() - block->first_operation;

    ir_open_block_index = ARMITO_IR_INDEX_INVALID;

    return ARMITO_IR_SUCCESS;
}

ArmitoIRStatus ArmitoIRBlockGet(ArmitoIRLabelID block_id, ArmitoIRBlock *block)
{
    if (block == NULL)
        return ARMITO_IR_INVALID_ARGUMENT;

    if (block_id == ARMITO_IR_LABEL_INVALID || block_id > ir_block_total)
        return ARMITO_IR_OUT_OF_RANGE;

    *block = ir_block_table[block_id - 1];

    if (ir_open_block_index != ARMITO_IR_INDEX_INVALID && block_id == ir_open_block_index + 1)
        block->operation_count =
            ArmitoIROperationsCount() - block->first_operation;

    return ARMITO_IR_SUCCESS;
}

uint32_t ArmitoIRBlocksCount(void)
{
    return ir_block_total;
}

uint32_t ArmitoIRBlockOperationCount(ArmitoIRLabelID block_id)
{
    ArmitoIRBlock block;

    if (ArmitoIRBlockGet(block_id, &block) != ARMITO_IR_SUCCESS)
        return 0;

    return block.operation_count;
}
