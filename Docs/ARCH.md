# Contrato parcial de aliases Integer — 3.2.5.1

O alias é metadado de apresentação associado à forma/operation_id canônicos;
seleção de alias não deve sobrescrever operação ou operandos arquiteturais.
`Tests/Decoder/AARCH64/integer_alias_predicates.py` interpreta separadamente
os 93 ASTs de condição/preferência da fonte Arm usando campos extraídos por
posição do opcode, sem chamar a lógica de predicados C. Inclui comparação de
registradores SP/ZR pelo valor codificado e função independente de bitmasks
para MoveWidePreferred. Predicados falsos/verdadeiros e sobreposições são
registrados individualmente. UBFM admite simultaneamente LSL e UBFIZ quando
imms+1==immr; o runtime escolhe UBFIZ, LLVM imprime LSL, ambos representam o
mesmo opcode UBFM. A semântica de execução futura precisa verificar efeitos
de NZCV, SP/ZR, extensões e larguras no tradutor/IR, além da equivalência
formal de cada alias; esta campanha não implementa execução.

# Condições e aliases Integer — campanha parcial 3.2.5

O oráculo separado em `Tests/Decoder/AARCH64/integer_condition_audit.py`
implementa a tabela de `ConditionHolds` do XML Arm: EQ/NE por Z, CS/CC por C,
MI/PL por N, VS/VC por V, HI/LS por C e Z, GE/LT por N==V, GT/LE por N==V
e Z; AL e NV avaliam verdadeiro no pseudocódigo, mas não são inversos
intercambiáveis na seleção de aliases. Inversão de alias apenas para cond<14.
NZCV é tratado como quatro bits N:Z:C:V. Predicados de preferência não são
restrições de codificação; aliases não mudam a operação canônica do probe.
Esta auditoria parcial não constitui validação de semântica em execução.

# Contrato de máscaras — Fase 3.2.4 (2026-11-26)

O pseudocódigo Arm `DecodeBitMasks` recebe `immediate=true` nas oito formas
logical immediate e `false` nas seis bitfield. `immN:NOT(imms)` determina
len; levels=(1<<len)-1, S=imms&levels, R=immr&levels, diff=(S-R)&63,
esize=1<<len, welem=Ones(S+1), telem=Ones((diff&levels)+1). `wmask` replica
ROR(welem,R), `tmask` replica telem até a largura da operação. Logical
immediate reserva S==levels; bitfield o admite, mas exige N==sf e, em 32 bits,
immr[5]==imms[5]==0. A implementação C existente evita shifts de 64 bits
e rotação por largura inteira mediante testes e máscara de largura; o oráculo
não chama nem compartilha rotinas do runtime. Aliases bitfield são conferidos
por seus parâmetros arquiteturais, não por igualdade textual de mnemonic.

# Contrato de rastreabilidade da baseline 3.2.3 (2026-11-26)

Chave de identidade: `(form, encoding)`; a lista preserva ordenação e inclui
todos os `REJECT`. `integer_immediate_rejection_audit.py` recalcula formas,
amostras e deduplicação determinística (`dict.fromkeys` mantém a ordem de
inserção), consulta o probe e LLVM MC e exige igualdade de **bytes** com
`integer_immediate_all_candidates.csv` e
`integer_immediate_rejections.csv`. Jamais substituir automaticamente a
baseline durante testes; modificar um corpus exige revisão explícita e novo
hash. As categorias são atribuídas uma vez por candidato; a soma de todos os
cinco grupos deve ser 62.851 e a chave não pode se repetir. O diferencial
immediate confronta campos brutos com bits do próprio opcode e os valores
efetivos com LLVM e invariantes de sinal/escala. Os `SETF` são verificados
contra os quatro bits fixos Arm (`1101`) e registrados separadamente, pois
LLVM não imprime uma máscara numérica. Rejeição LLVM sozinha não é oráculo de
invalidade. A verificação da validade do próprio `DecodeBitMasks` foi
**transferida**, integralmente (1.016), para 3.2.4.

# Guardas Integer immediate (Fase 3.2.3.1)

`AARCH64CatalogFormEncodingValid` aplica as regras oficiais Arm às formas
`addsub_ext` (imm3 até 4), `addsub_shift` (shift não reservado e imm6[5]
limpo em 32 bits), `log_shift` (imm6[5] limpo em 32 bits) e `bitfield`
(immr[5] e imms[5] limpos em 32 bits). Essas guardas antecedem a seleção
da forma e não afetam o número de descritores. O teste host consulta LLVM MC,
as regras Decode_UNDEF Arm e invariantes independentes para sinal e escala,
sem chamar o extrator para determinar a validade esperada. O SETF `mask=1101`
é um campo fixo de codificação, não imediato de montagem; o oráculo oficial
Arm fixa seus quatro bits. Casos bitmask ficam separados da validação de valor.
As rejeições `log_imm` dependentes de `DecodeBitMasks` não são classificadas
como invalidade arquitetural até a Fase 3.2.4.

# Immediates Integer: modelo de evidência da Fase 3.2.3

O probe C expõe valores brutos, interpretados, sinalizados e escalas; a
comparação host mantém separados os campos individuais e os sintéticos
`scaled_immediate`, `move_wide_immediate` e `pc_relative_address`. ADDG/SUBG
exibem deslocamento em bytes (`imm6 << 4`), enquanto o imediato de tag `imm4`
fica sem escala. PAuth-LR imprime o deslocamento negativo em unidades de quatro
bytes. SMAX/SMIN interpretam `imm8` com sinal; UMAX/UMIN com extensão de zero.
Os parâmetros bitfield são conferidos sob a transformação de cada alias;
`DecodeBitMasks` não é inferido de texto bitfield e segue reservado à 3.2.4.

# Registradores Integer: modelo de evidência da Fase 3.2.2

`AARCH64_OPERAND_FLAG_IMPLICIT_REGISTER` distingue campos GPR que existem no
descritor mas não são operandos explícitos (por exemplo Ra fixo em SMULH,
Rn fixo em XPACI e Rd/Rn fixos em certas variantes PAuth). O diferencial não
confunde um alias que omite/reorganiza campos com a forma canônica: só remove
registradores impressos sob restrições verificadas (`Rd==31`, `Rn==31`,
`Ra==31`, `Rn==Rm`, conforme o alias); a largura do registrador impresso
por SXTH/SXTW é tratada como apresentação do alias e não altera a largura
canônica no extrator. O status COMPLETE não equivale a equivalência semântica
nem certifica immediates ou registradores implícitos fixos.

# Limites da verificação por forma Integer (histórico anterior, 2026-11-26)

O probe host em C (`Tests/Decoder/AARCH64/IntegerFormProbe.c`) fornece para
cada codificação a forma selecionada e os operandos interpretados; não integra
o firmware. O driver Python usa LLVM MC como referência independente somente
para registradores explícitos e mantém falha para divergências, sem transformar
`COMPLETE` em verificação independente. Operand masks, NZCV, condições,
operandos implícitos e validade de encodings requerem oráculos adicionais.

# Armito Architecture

## 1. Visão geral

Armito é dividido em camadas para evitar que inicialização, coordenação,
decodificação e tradução fiquem acopladas.

``` text
+------------------------------+
|          ArmitoBoot          |
| Firmware / ambiente / handoff|
+---------------+--------------+
                |
                v
+------------------------------+
|             Core             |
| Lifecycle, registry, deps,   |
| dispatch, scheduler, IPC,     |
| monitor, diagnostics         |
+---------------+--------------+
                |
                v
+------------------------------+
|           Decoder            |
| Instruction stream -> IR     |
+---------------+--------------+
                |
                v
+------------------------------+
|          Universal IR        |
| Architecture-neutral model   |
+---------------+--------------+
                |
                v
+------------------------------+
| Translator / Target Backend  |
| IR -> target instructions    |
+---------------+--------------+
                |
                v
+------------------------------+
|       Execution Platform     |
+------------------------------+
```

Este é o modelo-alvo. O desenho não afirma que todas as camadas já
estejam implementadas.

## 2. ArmitoBoot

ArmitoBoot é o ponto de entrada e preparação do ambiente. Suas
responsabilidades previstas incluem:

-   identificar o ambiente de boot;
-   reunir informações de plataforma;
-   preparar os serviços necessários;
-   inicializar o Core;
-   entregar o contexto de boot ao Core;
-   transferir o fluxo sem invocar diretamente o Decoder.

O ponto de entrada chama `ArmitoCoreInitialize()` e
`ArmitoCoreStart()`. O boot não chama o Decoder diretamente nem cria uma
instrução artificial; o Core gerencia o ciclo de vida do Decoder.

## 3. Core

Core é o orquestrador do runtime do Armito. Ele controla o ciclo de vida
dos módulos e a ordem em que os serviços são preparados e executados.

Core não deve assumir responsabilidades internas de módulos
especializados. Em particular, não deve duplicar o detector global de
falhas.

Consulte [CORE.md](CORE.md).

## 4. Decoder

Decoder recebe blocos de instruções da arquitetura de origem,
processa-os sequencialmente e produz Universal IR.

Responsabilidades esperadas:

-   validar entradas e limites do bloco;
-   identificar instruções;
-   decodificar campos e operandos;
-   preservar a ordem e o fluxo semântico;
-   emitir operações de IR;
-   reportar instruções inválidas ou não suportadas;
-   não executar diretamente código de destino.

O contrato de entrada e saída, o tratamento de instruções variáveis e a
política de blocos precisam ser especificados antes de declarar o
Decoder concluído.

### 4.1 Catálogo e módulos AArch64

`Decoder/AARCH64/Catalog/AARCH64InstructionCatalog.json` é o inventário
central de operações, formas, aliases e padrões de codificação do perfil
Arm A-profile v9Ap6-A (AARCHMRS 2024-12). As máscaras são compostas com as
codificações dos grupos ancestrais. Condições, campos, famílias e features
são preservados; a catalogação não declara suporte ou tradução.

`Catalog/AARCH64Catalog.c` consome `AARCH64CatalogData.inc`, gerado pelo
`catalog_tool.py`; o runtime usa somente tabelas C `const`, sem JSON,
Python ou alocação dinâmica. A busca usa um índice estático pelos bits
[31:26], com 64 buckets e 3.883 referências a padrões. A prioridade é a
quantidade de bits fixos; candidatos mais específicos são avaliados antes
dos mais gerais. Os 177 pares com sobreposição potencial têm prioridades
distintas; empates ainda são rejeitados como ambíguos.

A API retorna o padrão e suas formas, IDs de operação, módulo/família,
identificadores de extensão e condição, campos de codificação/operandos e
o caminho pelos grupos ancestrais. Restrições AARCHMRS `should_be_mask` e
guards AST são avaliados antes de selecionar o padrão. O gerador compila
as expressões em bytecode e tabelas C estáticas; o runtime não usa JSON,
interpreta strings nem aloca memória dinamicamente. As 599 conditions
catalogadas são avaliáveis; `BFXPreferred`, `MoveWidePreferred` e
`SVEMoveMaskPreferred` também são usadas na seleção de aliases. O perfil de
extensões representa a CPU guest e não consulta nem herda capacidades da
CPU x86_64 hospedeira. ABI Apple ARM64E é um atributo separado e não
habilita features automaticamente. A API separa match bruto, válido,
reservado, inválido, extensão indisponível e
`PATTERN_MATCHED_UNVALIDATED`; este último permanece explícito quando uma
expressão necessária não pode ser avaliada.
Validação arquitetural não significa extração completa de operandos,
tradução IR ou execução.

O caminho atual de `AARCH64Classify` usa a tabela para identificar formas
fora do subconjunto existente e retorna `CATALOG_ONLY` como não suportada.
O lookup indexado foi comparado com matcher linear (59.103 lookups,
104.702 checks, zero divergências); 33 codificações foram comparadas com
LLVM MC, incluindo instruções PAC e LSE. Isso não esgota o produto
cartesiano dos campos, a semântica de todas as instruções ou a execução de
extensões opcionais.

O módulo está listado em `Armito.inf`. Core inicializa o catálogo estático
e registra a API do extrator, sem decodificar uma instrução no boot.
ArmitoBoot inicializa/inicia Core. Os builds EDK2 DEBUG/RELEASE confirmam
que tabelas, descritores e extrator estão ligados no PE32+; ainda não houve
execução da busca em firmware real.

### 4.2 Extração universal de operandos

Integer/Data Processing (`dpimm` e `dpreg`) separa campos seletores de
codificação dos operandos arquiteturais com a flag
`AARCH64_OPERAND_FLAG_ENCODING_SELECTOR`. A extração determina largura GPR
por forma/campo, inclusive operandos mistos de multiplicação longa, CRC e
extensão, e preserva os demais campos brutos para diagnóstico. COMPLETE
requer que cada campo de operando exposto seja interpretado e que o lookup
tenha validado a codificação; a flag de seletor não é autorização geral
para aceitar operandos raw. O teste por forma usa até 96 candidatos e mede
um único testemunho: 195/195 Integer COMPLETE, sem prova de todos os valores
ou de todas as alternativas de alias. O diferencial efetivo LLVM cobre seis
palavras, não todas as formas.
As formas bitfield expõem `bitfield_write_mask` e `bitfield_top_mask` como
operandos compostos, além de manter `immr` e `imms` disponíveis.

`Decoder/AARCH64/Operands/AARCH64Operands.c` extrai operandos a partir de
descritores C estáticos por forma. `AARCH64OperandSet` representa valores
raw, interpretados, parciais e não suportados, com registradores, largura,
roles, deslocamentos, endereços, aliases e metadados PAC. O extrator pode
ser obtido por `ArmitoDecoderGetOperandExtractor`; a inicialização registra
esse serviço e verifica referências/bytes das tabelas sem fabricar ou
decodificar instruções.

Todos os 4.296 forms têm descritores estruturais (45.360 no total), e
3.093/3.099 amostras de padrões foram aceitas pelos testes (inclusive
parciais/raw); elas alcançam 3.026 forms e 1.478/2.095 IDs de operação.
As rotinas atuais cobrem subconjuntos de GPR, SP/zero, immediates e
bitmasks, branches e endereços PC-relative, modos de memória, pares,
system registers e metadados PAC. A tabela de 288 aliases avalia as três
condições preferenciais e não seleciona um alias quando a condição não é
avaliável. Reconhecimento de descritor não significa extração semântica
completa.

SIMD/NEON, SVE, SME, lanes, listas e alguns modos de endereçamento ou
operandos de sistema permanecem parciais/não suportados. O diferencial
LLVM ainda cobre 33 codificações de mnemonic/classificação, não a
equivalência operand-by-operand. A extração não foi conectada à tradução
IR para não degradar operações incompletas em NOP ou semântica aproximada.

A auditoria por forma usa até 96 codificações determinísticas com todas as
features do perfil de teste ativadas e exige que o lookup retorne a própria
forma; a aceitação de padrão não é confundida com cobertura por forma. Os
resultados são 0 completos, 4.255 parciais, 33 raw e 8 não alcançados em
4.296 formas. Os modos pre/post-index com imm9 têm offset assinado e flags
explícitas de writeback/post-index; isso não completa listas, lanes, SVE/SME
nem torna os descritores raw operandos arquiteturais interpretados.

### 4.3 Apple ARM64E

`CPU/AARCH64/AARCH64MachO` identifica `CPU_TYPE_ARM64` e separa o subtype
base dos bits de capacidade. `CPU_SUBTYPE_ARM64E` tem valor base 2;
`ARM64E_X1` é reconhecido separadamente. A camada também lê cabeçalhos
Mach-O 64-bit e localiza slices ARM64E em arquivos universais, mas não é
um loader e ainda não interpreta relocations autenticadas.

O Decoder preserva as operações PAC em operações específicas da IR,
incluindo assinatura, autenticação, remoção, branch/call/return e load
autenticados. Elas são reconhecidas e modeladas, mas continuam
explicitamente não suportadas pela tradução enquanto não houver backend.
O estado da CPU contém as chaves, contexto e falhas PAC; não há ainda
runtime que conecte esse estado à execução traduzida.

## 5. Universal IR

A IR deve representar operações sem depender diretamente dos códigos de
operação de uma ISA específica. Deve contemplar, conforme a evolução do
projeto:

-   operações aritméticas e lógicas;
-   leitura e escrita de registradores virtuais;
-   acesso à memória;
-   comparações e flags;
-   desvios e blocos básicos;
-   chamadas e retornos;
-   conversões e extensões;
-   operações atômicas;
-   operações especiais ou intrínsecas;
-   metadados de origem e condições de validação.

A IR precisa ter regras formais de tipos, largura, efeitos colaterais e
semântica. Uma IR "universal" não deve apagar diferenças semânticas
entre arquiteturas. O modelo ARM64E mantém as operações PAC distintas e
não as transforma em NOP; consulte [IR.md](IR.md).

## 6. Translator e backends

O tradutor converte IR para uma arquitetura de destino. Backends
separados permitem que o modelo intermediário permaneça independente da
ISA de destino.

Um backend precisa definir:

-   mapeamento de operações;
-   alocação de registradores;
-   convenções de chamada;
-   geração de desvios;
-   acessos à memória;
-   operações sem mapeamento direto;
-   alinhamento e requisitos de código;
-   validação do código emitido.

## 7. Serviços de plataforma

A execução real pode exigir camadas adicionais para ABI, chamadas de
sistema, bibliotecas, exceções, memória virtual e serviços do sistema
operacional. Essas funções não devem ser confundidas com a tradução de
instruções da CPU.

## 8. Fluxo de falhas

`fail.c` é a autoridade responsável por detectar falhas. Os componentes
podem retornar estados e fornecer contexto, mas não devem criar um
segundo sistema concorrente de detecção.

O Core Diagnostics atua como adaptador de apresentação e encaminhamento
para `Display.c` e `LogGen.c`.

## 9. Regras de dependência

-   ArmitoBoot inicializa e inicia o Core.
-   Core coordena os módulos.
-   ArmitoBoot não chama Decoder diretamente.
-   Decoder produz IR; não deve assumir as responsabilidades do backend.
-   Backends consomem IR e geram instruções de destino.
-   Falhas seguem a autoridade única definida em [FAILS.md](FAILS.md).

## 10. Evolução

A arquitetura deve ser consolidada por contratos e testes antes da
expansão para novos pares de ISA. Primeiro deve ser provado um caminho
vertical completo e verificável; depois, a cobertura deve crescer de
forma incremental.
