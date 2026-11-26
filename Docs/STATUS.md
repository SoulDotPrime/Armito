# Development Status

**Última consolidação documental:** 2026-11-26\
**Foco atual:** auditoria e consolidação do Core\
**Estado geral:** em desenvolvimento; não declarar pronto para
integração final.

## 1. Resumo

O projeto está concentrado na estrutura do Core. Existem implementações
para os subsistemas listados abaixo, mas ainda há auditorias, correções
de ciclo de vida, integração de diagnóstico e validação de build/testes
pendentes.

## 2. Estado do Core

  -----------------------------------------------------------------------
  Subsistema              Código / estrutura      Auditoria e validação
  ----------------------- ----------------------- -----------------------
  CoreTypes               Revisado na etapa 01.01 Revisão estrutural
                                                  inicial concluída

  Config                  Implementação de        Integração de shutdown
                          configuração e          e testes pendentes
                          validação               

  Registry                Implementado com        Auditoria completa
                          limites de configuração pendente

  Dependency              Implementado com        Auditoria de IDs,
                          resolução topológica    ciclos e limites
                                                  pendente

  Init                    Implementado            Rollback parcial
                                                  pendente

  Diagnostics             Adaptador de            Integração com
                          apresentação inicial    fail/Display/LogGen
                                                  pendente

  Dispatch                Implementado            Auditoria e testes
                                                  pendentes

  Monitor                 Implementado            Política para módulos
                                                  opcionais pendente

  Topology                Implementado            Auditoria e testes
                                                  pendentes

  Scheduler               Implementação           Concorrência,
                          cooperativa             reutilização e testes
                                                  pendentes

  IPC                     Fila fixa de mensagens  Memória freestanding,
                                                  sincronização e
                                                  permissões pendentes

  Shutdown                Implementado            Integração final com
                                                  Config e testes de
                                                  reinicialização
                                                  pendentes

  Core.c                  Orquestração inicial    Rollback e API pública
                                                  pendentes
  -----------------------------------------------------------------------

## 3. Configuração

A estrutura de configuração inclui limites para módulos, dependências,
tarefas, mensagens IPC e payload. Há flags para habilitar Scheduler,
IPC, Monitor e Diagnostics.

Os limites atuais refletem os limites estáticos das estruturas
existentes. Não representam capacidade validada em execução.

## 4. Próxima sequência de trabalho

1.  Integrar `ArmitoCoreConfigShutdown()` ao gerenciador de shutdown.
2.  Auditar Init e rollback.
3.  Auditar Registry.
4.  Auditar Dependency e resolução topológica.
5.  Auditar Dispatch, Monitor e Topology.
6.  Auditar Scheduler e IPC.
7.  Auditar Diagnostics e Shutdown.
8.  Auditar Core.c e consolidar API pública.
9.  Corrigir dependências e transições de estado.
10. Criar abstração de memória e adaptar para ambiente
    freestanding/EDK2.
11. Integrar fail.c, Display.c e LogGen.c.
12. Compilar e testar Core de forma independente.

## 5. Decoder e IR

O Decoder AArch64 possui API de fonte de bytes, classificacao e extracao de
operandos para um subconjunto de controle de fluxo, movimentacao e aritmetica.
NOP, B, BR e RET sao traduzidos para IR; instrucoes reconhecidas sem semantica
de IR completa sao preservadas como `ARMITO_IR_OP_UNSUPPORTED`. O Core e a IR
passam nos testes de host; o Decoder tambem passa sua suite de host (224
verificacoes). Os builds EDK2 X64 DEBUG e RELEASE passaram com GCC e geraram
aplicacoes EFI PE32+. A execucao em firmware UEFI real nao foi testada. Isso
nao representa suporte completo AArch64.

A IR possui tipos, valores, operacoes, blocos e um validador inicial. A
validacao nao cobre ainda todos os contratos semanticos da IR descritos no
roadmap. Mantenha Decoder e IR como parciais ate implementar e testar as
camadas restantes.

## 5.1 Estado arquitetural da CPU AArch64

O modulo `CPU/AARCH64/AARCH64CPUState` armazena X0-X30, SP, PC, NZCV, FPCR,
FPSR, V0-V31, estado de excecao e slots de chaves ARM64E. As vistas Wn/WSP
limitam escritas a 32 bits e zero-estendem; XZR/WZR leem zero e descartam
escritas. Vetores sao armazenados em 128 bits; escrita de 64 bits limpa a
metade superior. A suite host passa 58 verificacoes e os builds EDK2 X64
DEBUG e RELEASE incluem o modulo.

Os slots de ARM64E sao apenas armazenamento de estado. Assinatura,
autenticacao de ponteiros e politica de falha ainda nao estao implementadas.

## 6. Integração com ArmitoBoot

Ainda dependente de: - contrato Core ↔ ArmitoBoot; - estrutura de
contexto de boot; - transferência de controle; - integração do
diagnóstico; - gerenciamento de memória; - testes do ciclo completo.

## 7. Critérios para atualizar este arquivo

Use estes estados: - **Planejado:** definido, ainda sem implementação. -
**Em desenvolvimento:** código em construção. - **Implementado:** código
existe, sem implicar validação. - **Integrado:** conectado aos
componentes necessários. - **Validado:** compilado e testado no ambiente
indicado. - **Concluído:** critérios de aceitação documentados foram
cumpridos.

Atualize a data e os estados após cada marco confirmado. Não converter
"implementado" em "validado" sem evidência de teste.
