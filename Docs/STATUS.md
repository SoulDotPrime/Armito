# Development Status

**Última consolidação documental:** 2026-11-25\
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

A arquitetura prevê Decoder e Universal IR como camadas centrais, mas o
foco atual é terminar a base do Core. Não marcar Decoder ou IR como
concluídos sem código, contratos e testes correspondentes.

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
