# Architectural Decisions

Este documento registra decisões estabelecidas para orientar o
desenvolvimento. Uma decisão só deve ser alterada mediante revisão
explícita e atualização deste arquivo.

## D-001 --- Linguagem principal

**Decisão:** C é a linguagem principal do Armito. Assembly pode ser
usado quando necessário.

**Motivo:** controle de baixo nível, integração com firmware e
previsibilidade em ambientes de sistema.

## D-002 --- Rust fora do escopo

**Decisão:** Rust não será usado na implementação atual do Armito.

## D-003 --- Core como orquestrador

**Decisão:** ArmitoBoot inicializa e transfere o controle ao Core. Core
coordena módulos e subsistemas.

**Consequência:** ArmitoBoot não chama Decoder diretamente.

## D-004 --- Decoder separado

**Decisão:** Decoder recebe blocos de instruções, decodifica
sequencialmente e produz Universal IR.

**Consequência:** Decoder não deve misturar decodificação com geração de
código de destino.

## D-005 --- Universal IR

**Decisão:** usar uma representação intermediária entre ISA de origem e
backend de destino.

**Consequência:** a IR deve preservar semântica, tipos, larguras e
efeitos colaterais, não apenas nomes genéricos de operações.

## D-006 --- Detector único de falhas

**Decisão:** `fail.c` é a autoridade responsável por detectar qualquer
falha.

**Consequência:** Core Diagnostics apresenta e encaminha; não duplica a
detecção.

## D-007 --- Saídas de diagnóstico separadas

**Decisão:** `Display.c` cuida da apresentação visual e `LogGen.c` do
registro em log.

## D-008 --- Compatibilidade de boot por caminhos

**Decisão:** considerar caminhos distintos para UEFI nativo, UEFI+CSM e
Legacy BIOS puro.

**Consequência:** Legacy BIOS puro exige estágio de entrada próprio; um
binário EFI isolado não cobre esse caminho.

## D-009 --- Desenvolvimento incremental

**Decisão:** concluir e auditar o Core antes da integração com Decoder e
ArmitoBoot.

## D-010 --- Arquivos completos nas alterações

**Decisão de fluxo de trabalho:** alterações de código devem ser
apresentadas como arquivos completos, com caminho explícito.

## D-011 --- Código sem comentários

**Decisão de estilo:** não inserir comentários dentro do código
fornecido.

## D-012 --- Uma alteração por vez

**Decisão de fluxo de trabalho:** apresentar uma alteração por etapa e
aguardar a confirmação do usuário antes de avançar.

## D-013 --- Estado não presumido

**Decisão:** código escrito não deve ser tratado como funcionalidade
validada sem compilação e teste. Documentação deve distinguir
implementado, integrado e validado.

## D-021 --- Tabela de operações explícita

**Decisão:** toda instrução de origem deve resultar em uma operação
enumerada da IR ou em `ARMITO_IR_OP_UNSUPPORTED` com a codificação de
origem preservada.

**Consequência:** nunca se emite código aproximado sem semântica
equivalente (alinhado à seção 8 de `COMPATIBILITY.md`). Uma instrução
não suportada é visível na IR e rejeitável pelo backend.

## Processo de alteração

Para alterar uma decisão: 1. identificar o número da decisão; 2.
explicar o motivo da revisão; 3. registrar a nova decisão e suas
consequências; 4. atualizar documentos afetados; 5. ajustar o roadmap e
o status.

## D-019 --- Universal IR de forma SSA com limites estáticos

**Decisão:** a IR usa forma SSA (cada valor tem um único produtor), com
tipos explícitos, operações enumeradas, blocos básicos por rótulo e
limites estáticos iguais aos do Core.

**Consequência:** a IR cabe em ambiente freestanding, sem alocador
dinâmico, e o backend pode alocar registradores livremente. O custo é
que construções com múltiplas atribuições exigem phi ou cópia explícita
via `select`/`move`; phi ainda não está implementado.

## D-020 --- Localização da IR em `IR/`

**Decisão:** a Universal IR vive em `IR/`, na raiz do repositório, e não
dentro de `Decoder/`. `Decoder/` contém o decodificador; `IR/` é a
camada compartilhada entre o decodificador e os backends.

**Consequência:** `Decoder/IR/` (vazio) deixa de ser o local da IR e
precisa ser removido. Um backend em `CPUToCPU/` inclui `IR/` sem incluir
`Decoder/`.

## D-022 --- Semântica PAC explícita

**Decisão:** Pointer Authentication tem operações próprias na IR e um
provedor criptográfico substituível na camada AArch64. Operações PAC
reconhecidas sem backend permanecem explicitamente não suportadas.

**Consequência:** PAC não pode ser reduzido a NOP ou a uma operação genérica
sem chave, modificador e efeitos preservados. O provedor de software
disponibilizado para testes é uma implementação de compatibilidade, não
uma reprodução criptográfica do hardware Apple.

## D-023 --- Identificação ARM64E sem loader implícito

**Decisão:** a integração Mach-O desta etapa identifica metadados ARM64E e
slices universais, sem declarar que carregamento ou execução de Mach-O
estejam implementados.

**Consequência:** parsing de relocations autenticadas, loader, backend e
runtime devem ser etapas explícitas e testadas separadamente.

## D-024 --- Catálogo AArch64 versionado e rastreável

**Decisão:** o inventário de instruções AArch64 é gerado do Arm AARCHMRS
A-profile v9Ap6-A (2024-12), com commit, hash da fonte e licença
registrados. Formas, aliases, IDs de operação e padrões de máscara/valor
são métricas separadas.

**Consequência:** atualizações do catálogo exigem uma nova versão de fonte,
regeneração verificável e revisão dos totais. AARCHMRS não representa
instruções proprietárias privadas da Apple. A presença de uma entrada não
declara implementação nem suporte.

## D-025 --- Despacho modular e ambiguidade explícita

**Decisão:** os decoders por família compartilham uma interface central.
Padrões idênticos são deduplicados; a prioridade de um padrão é o número
de bits fixos em sua máscara. Condições de extensão e de operandos fazem
parte da decisão, não são descartadas pelo match da máscara.

**Consequência:** padrões mais específicos precedem padrões mais gerais.
Qualquer empate que não seja resolvido por uma regra semântica explícita
é reportado como ambíguo e não pode gerar IR. Catálogo, classificação,
extração de operandos, tradução para IR e suporte de backend permanecem
estados distintos.
