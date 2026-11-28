#include "Includes/CoreMemory.h"

void ArmitoMemoryCopy(void *destination, const void *source, size_t size)
{
    uint8_t *to = (uint8_t *)destination;
    const uint8_t *from = (const uint8_t *)source;
    size_t i;

    if (to == NULL || from == NULL)
        return;

    for (i = 0; i < size; i++)
        to[i] = from[i];
}

void ArmitoMemorySet(void *destination, uint8_t value, size_t size)
{
    uint8_t *to = (uint8_t *)destination;
    size_t i;

    if (to == NULL)
        return;

    for (i = 0; i < size; i++)
        to[i] = value;
}
