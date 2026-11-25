#include "Includes/IRBlock.h"

static ArmitoIRBlock blocks[ARMITO_IR_MAX_BLOCKS];
static uint32_t block_count = 0;
static uint32_t open_block = ARMITO_IR_INDEX_INVALID;

static void CopyName(char *dest, const char *src)
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
        blocks[i].id = ARMITO_IR_LABEL_INVALID;
        blocks[i].first_operation = 0;
        blocks[i].operation_count = 0;
        blocks[i].guest_address = 0;
        blocks[i].terminated = 0;
        blocks[i].name[0] = '\0';
    }

    block_count = 0;
    open_block = ARMITO_IR_INDEX_INVALID;

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

    if (open_block != ARMITO_IR_INDEX_INVALID)
        return ARMITO_IR_INVALID_STATE;

    if (block_count >= ARMITO_IR_MAX_BLOCKS)
        return ARMITO_IR_OUT_OF_RESOURCES;

    block = &blocks[block_count];
    block->id = block_count + 1;
    block->first_operation = ArmitoIROperationsCount();
    block->operation_count = 0;
    block->guest_address = guest_address;
    block->terminated = 0;
    CopyName(block->name, name);

    open_block = block_count;
    block_count++;

    *block_id = block->id;

    return ARMITO_IR_SUCCESS;
}

ArmitoIRStatus ArmitoIRBlockEnd(void)
{
    ArmitoIRBlock *block;

    if (open_block == ARMITO_IR_INDEX_INVALID)
        return ARMITO_IR_INVALID_STATE;

    block = &blocks[open_block];
    block->operation_count = ArmitoIROperationsCount() - block->first_operation;

    open_block = ARMITO_IR_INDEX_INVALID;

    return ARMITO_IR_SUCCESS;
}

ArmitoIRStatus ArmitoIRBlockGet(ArmitoIRLabelID block_id, ArmitoIRBlock *block)
{
    if (block == NULL)
        return ARMITO_IR_INVALID_ARGUMENT;

    if (block_id == ARMITO_IR_LABEL_INVALID || block_id > block_count)
        return ARMITO_IR_OUT_OF_RANGE;

    *block = blocks[block_id - 1];

    if (open_block != ARMITO_IR_INDEX_INVALID && block_id == open_block + 1)
        block->operation_count =
            ArmitoIROperationsCount() - block->first_operation;

    return ARMITO_IR_SUCCESS;
}

uint32_t ArmitoIRBlocksCount(void)
{
    return block_count;
}

uint32_t ArmitoIRBlockOperationCount(ArmitoIRLabelID block_id)
{
    ArmitoIRBlock block;

    if (ArmitoIRBlockGet(block_id, &block) != ARMITO_IR_SUCCESS)
        return 0;

    return block.operation_count;
}
