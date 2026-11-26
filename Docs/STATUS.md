# Development Status

**Última consolidação documental:** 2026-11-26\
**Foco atual:** consolidação do Core e base arquitetural ARM64E\
**Estado geral:** em desenvolvimento; não declarar pronto para
integração final.

## 1. Resumo

O Core continua em consolidação e a base ARM64E está parcialmente
implementada. Existem implementações para os subsistemas listados abaixo,
mas ainda há auditorias, correções de ciclo de vida, integração de
diagnóstico e validação de build/testes pendentes.

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

O Decoder AArch64 possui API de fonte de bytes, classificação e extração de
operandos para um subconjunto de controle de fluxo, movimentação e aritmética.
NOP, B, BR e RET têm tradução existente. Para ARM64E, as instruções PAC/AUT,
XPAC, PACIASP/AUTIASP, RETAA/RETAB, branches autenticados e LDRAA/LDRAB
offset-addressing reconhecidos geram operações PAC específicas da IR e
continuam marcados como não suportados pela tradução. As formas LDRAA/LDRAB
pre-index com writeback permanecem não reconhecidas. O Decoder passou 224
verificações host; a suíte
ARM64E passou 514 verificações host com ASan/UBSan; CPU State passou 58.
EDK2 X64 DEBUG e RELEASE foram compilados em workspace isolado com GCC e
geraram aplicações EFI PE32+. Execução em firmware UEFI real não foi testada.
Os fontes `Tests/CoreTest.c` e `Tests/IRTest.c` não existem no worktree atual,
embora sejam referenciados pelo Makefile; esses alvos não foram validados.

A IR possui tipos, valores, operacoes, blocos e um validador inicial. A
validacao nao cobre ainda todos os contratos semanticos da IR descritos no
roadmap. Mantenha Decoder e IR como parciais ate implementar e testar as
camadas restantes.

## 5.1 Estado arquitetural da CPU AArch64

O módulo `CPU/AARCH64/AARCH64CPUState` armazena X0-X30, SP, PC, NZCV, FPCR,
FPSR, V0-V31, estado de exceção e as chaves PAC IA, IB, DA, DB e GA. As
vistas Wn/WSP limitam escritas a 32 bits e zero-estendem; XZR/WZR leem zero
e descartam escritas. Vetores são armazenados em 128 bits; escrita de 64
bits limpa a metade superior. A suite host CPU State passa 58
verificações.

`AARCH64PointerAuth` implementa assinatura, autenticação, remoção de PAC,
diversificadores, address diversity, seleção de chaves Apple, helpers para
LR/SP e controle autenticado de fluxo. O provedor de software é
determinístico, apenas compatível para testes e não é criptograficamente
equivalente ao hardware Apple. A integração com um executor/backend ainda
não existe.

## 5.2 Estado ARM64E e Mach-O

`AARCH64MachO` reconhece ARM64 subtype base 2 sem confundir os bits de
capacidade; separa os indicadores de ABI Pointer Authentication e a versão
codificada. Distingue `ARM64E_X1` do subtype ARM64E. Lê cabeçalho Mach-O
64-bit e fat/fat64 para localizar slices ARM64E; não carrega imagens nem
processa relocations autenticadas. A suíte específica passou 514
verificações host. Os builds EDK2 X64 DEBUG e RELEASE incluem os módulos e
foram validados por compilação e link.

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
