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

## Processo de alteração

Para alterar uma decisão: 1. identificar o número da decisão; 2.
explicar o motivo da revisão; 3. registrar a nova decisão e suas
consequências; 4. atualizar documentos afetados; 5. ajustar o roadmap e
o status.
