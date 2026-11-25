# Universal IR

## 1. Papel

A Universal IR é a representação intermediária independente de ISA que
fica entre o Decoder e os backends de destino.

``` text
AArch64 (Apple M2)  ->  Decoder  ->  Universal IR  ->  Backend x86_64 (FX-8320E)
```

Nada específico de AArch64 pode aparecer na IR, e nada específico de
x86_64 pode ser exigido por ela. Quando uma instrução não tem operação
equivalente na IR, ela é registrada como `ARMITO_IR_OP_UNSUPPORTED` com
a codificação de origem preservada — nunca aproximada.

## 2. Topologia

``` text
IR/
├── Includes/
│   ├── IR.h            API de ciclo de vida e validação
│   ├── IRTypes.h       status, estados, IDs e limites
│   ├── IRTypeset.h     tipos e larguras
│   ├── IRValue.h       valores: registrador virtual, imediato, memória
│   ├── IROperation.h   operações e flags
│   ├── IRBlock.h       blocos básicos e rótulos
│   └── IRCPUState.h    mapeamento do estado da CPU de origem
├── IR.c
├── IRTypeset.c
├── IRValue.c
├── IROperation.c
├── IRBlock.c
└── IRCPUState.c
```

## 3. Modelo de dados

### 3.1 Tipos (`ArmitoIRType`)

Um tipo é descrito por classe, largura em bits, número de lanes e
sinalização. Não há ponteiros implícitos: `is_pointer` é explícito
porque o destino pode ter endereçamento diferente da origem.

Classes: `VOID`, `INT`, `FLOAT`, `VECTOR`, `FLAGS`.

### 3.2 Valores (`ArmitoIRValue`)

Todo operando é um valor com identidade própria (SSA), de um dos tipos:

| Tipo | Uso |
|---|---|
| `VIRTUAL_REGISTER` | registrador de origem renomeado para SSA |
| `IMMEDIATE` | constante |
| `PC` | valor do contador de programa da origem |
| `MEMORY` | acesso à memória (load/store) com endereço e deslocamento |
| `FLAGS` | banco de flags como valor |

O registrador físico da origem não é preservado como registrador: ele é
apenas o nome de origem (`source_name`), e o mapeamento fica em
`IRCPUState`. Isso é o que permite o backend alocar registradores
livremente.

### 3.3 Operações (`ArmitoIROperation`)

Cada operação tem `kind`, tipo e valor de resultado, até 3 entradas,
máscara de flags e alvos de salto. A origem é registrada em
`source_offset`, `source_encoding` e `source_length` para rastrear a
instrução que a gerou.

Grupos de operação:

- dados: `move`, `select`, `load`, `store`;
- aritmética: `add`, `sub`, `mul`, `divs/divu`, `rems/remu`, `neg`;
- lógica: `and`, `or`, `xor`, `not`;
- deslocamento: `shl`, `lsr`, `asr`, `rol`, `ror`;
- comparação: `cmp.eq/ne/lt/le/gt/ge` (com e sem sinal) e `cmp.zero`;
- conversão: `sext`, `zext`, `trunc`, `bitcast`, `extract`, `insert`;
- controle: `branch`, `branch.cond`, `branch.indirect`, `call`,
  `return`, `trap`;
- outros: `barrier`, `unsupported`.

### 3.4 Blocos (`ArmitoIRBlock`)

Um bloco básico agrupa operações contíguas e guarda o endereço de
origem. Blocos referenciam-se por rótulo (`ArmitoIRLabelID`), nunca por
endereço de destino — o layout final pertence ao backend.

### 3.5 Estado da CPU (`ArmitoIRCPURegister`)

O mapeamento `(classe, índice) -> valor` descreve como o estado da CPU
de origem foi projetado na IR. Classes: `GPR`, `VECTOR`, `FLAGS`, `PC`,
`STACK_POINTER`.

## 4. Contrato com o Decoder

1. `ArmitoIRInitialize()` uma vez por tradução.
2. Para cada instrução: declarar valores, emitir operação, anotar flags,
   origem e alvos.
3. Instrução sem tradução: emitir `ARMITO_IR_OP_UNSUPPORTED` com a
   codificação de origem — nunca omitir.
4. `ArmitoIRFinalize()` valida a IR inteira antes de o backend rodar.

## 5. Contrato com o Backend

O backend recebe a IR finalizada e não pode modificar a estrutura.
Ele deve:

- mapear cada `kind` para instrução nativa ou sequência equivalente;
- tratar `ARMITO_IR_OP_UNSUPPORTED` como rejeição controlada;
- honrar a máscara de flags quando o destino tiver flags diferentes;
- não depender de `source_offset` para gerar código.

## 6. Limites estáticos

| Limite | Valor |
|---|---|
| tipos | 32 |
| valores | 256 |
| operações | 1024 |
| blocos | 64 |
| registradores de origem | 64 |
| entradas por operação | 3 |
| bytes de origem por operação | 256 |

Os limites são estáticos porque a IR precisa caber em ambiente
freestanding, sem alocador dinâmico (ver decisão D-003 do Core).

## 7. Validação

`ArmitoIRValidate()` verifica:

- IDs de tipo e valor dentro dos limites;
- alvos de salto apontando para blocos existentes;
- `store` com pelo menos 2 entradas e `load` com pelo menos 1;
- `add` com exatamente 2 entradas;
- operações `UNSUPPORTED` são aceitas sem verificação semântica.

`ArmitoIRFinalize()` roda a validação e, em caso de erro, deixa a IR em
`ARMITO_IR_FAILED`.

## 8. Estado de implementação

| Item | Estado |
|---|---|
| Tipos, valores, operações, blocos, estado da CPU | implementado |
| Ciclo de vida e validação | implementado |
| Testes de host (`Tests/IRTest.c`) | 172 verificações, 0 falhas |
| Serialização / desserialização | não implementada |
| Integração com Decoder real | pendente (Fase 05) |
| Backend x86_64 | pendente (Fase 07) |

Este documento descreve o contrato da IR. A IR está implementada e
testada isoladamente; ela ainda não recebe instruções de um decoder
AArch64 real.
