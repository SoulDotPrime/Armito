# Decoder AArch64 — API publica e contrato de entrada (Fase 05.01)

Status: **API e contrato fechados; implementacao em depuracao (33 falhas em 166 verificacoes).**

## 1. Contrato de entrada

O Decoder nunca recebe ponteiro cru de memoria do firmware. Ele recebe uma
**fonte de instrucoes** (`ArmitoDecoderSource`) que descreve onde o codigo esta:

- `base` — endereco virtual da primeira instrucao (deve ser multiplo de 4);
- `length` — tamanho em bytes da regiao decodificavel;
- `read` — funcao de leitura fornecida pelo chamador (permite mapear paginas
  de forma segura, sem dereferenciar endereco arbitrario);
- `context` — opaco devolvido em cada `read`.

Regras do contrato:

1. Toda instrucao AArch64 tem 4 bytes e o PC e sempre alinhado a 4; endereco
   desalinhado ou fora de `[base, base+length)` e erro de entrada, nao
   instrucao invalida.
2. O decoder **nao escreve** na fonte; le uma vez por instrucao e trata leitura
   falha como `ARMITO_DECODER_READ_ERROR`.
3. O decoder nao aloca memoria: estado vive em `ArmitoDecoder` e nos buffers do
   chamador.
4. Saida de uma instrucao: `AARCH64Opcode` (classificacao) + `AARCH64Instruction`
   (operandos ja extraidos: registradores, imediato, condicao, deslocamento).
5. Instrucao nao traduzida nao e aproximada: vira `ARMITO_IR_OP_UNSUPPORTED`
   com a codificacao original preservada (D-021).

## 2. API publica

### Ciclo de vida (`Decoder/Includes/Decoder.h`)

| Funcao | Contrato |
|---|---|
| `ArmitoDecoderInitialize(ArmitoDecoder *)` | estado inicial; idempotente |
| `ArmitoDecoderReset(ArmitoDecoder *)` | zera estatisticas, mantem fonte |
| `ArmitoDecoderSetSource(ArmitoDecoder *, const ArmitoDecoderSource *)` | valida base/4 e `read != NULL` |
| `ArmitoDecoderDecodeInstruction(...)` | decodifica uma instrucao |
| `ArmitoDecoderDecodeBlock(...)` | decodifica ate `max_instructions` ou ate desvio |
| `ArmitoDecoderToIR(...)` | traduz o ultimo bloco para a IR universal |
| `ArmitoDecoderGetStats(...)` | contadores |

### Classificacao (`Decoder/AARCH64/Includes/AARCH64.h`)

`AARCH64Classify(encoding, &opcode)` e `AARCH64Decode(encoding, &instruction)`
sao puras: nao tocam memoria do processo, nao dependem de estado global.
`AARCH64ToIR` e a unica funcao que emite na IR.

### Codigos de retorno (`ArmitoDecoderStatus`)

`SUCCESS`, `INVALID_ARGUMENT`, `INVALID_STATE`, `READ_ERROR`,
`UNALIGNED_ADDRESS`, `TRUNCATED_INSTRUCTION`, `UNSUPPORTED_INSTRUCTION`,
`IR_ERROR`.

## 3. Cobertura desta fase

Classificadas: `NOP`, `RET`, `BR`, `BL` (imediato e registrador), `B.cond`,
`CBZ`/`CBNZ`, `TBZ`/`TBNZ`, `MOV` (registrador), `MOV` imediato,
`ADD`/`SUB` deslocados. Todo o resto do espaco de codificacao devolve
`UNSUPPORTED_INSTRUCTION` sem consumir a instrucao do bloco.

## 4. Ponto aberto

`Tests/DecoderTest.c` ainda falha em 33 de 166 verificacoes, concentradas em
`AARCH64Classify` para o grupo de desvios. A investigacao mostrou que o
classificador gera a comparacao correta no binario mas o desvio de controle
nao e tomado como esperado — e preciso isolar com um binario minimo antes de
mexer no resto do decoder.
