#ifndef ARMITO_CORE_MEMORY_H
#define ARMITO_CORE_MEMORY_H

#include <stddef.h>
#include <stdint.h>

/*
 * Copia de memoria do Armito.
 *
 * O projeto e um pacote EDK2 (UEFI), onde a biblioteca C padrao nao existe:
 * <string.h> nao esta disponivel no ambiente de firmware. Esta funcao e a
 * unica dependencia que o Core tinha da libc e agora vive no proprio projeto.
 * Regiao de origem e destino nao podem se sobrepor (mesma regra de memcpy).
 */
void ArmitoMemoryCopy(void *destination, const void *source, size_t size);

/*
 * Preenche uma regiao com um byte.
 */
void ArmitoMemorySet(void *destination, uint8_t value, size_t size);

#endif
