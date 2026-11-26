# Armito

**Armito** é um projeto de tradução e compatibilidade de instruções
entre arquiteturas, concebido para permitir que software desenvolvido
para uma arquitetura de origem seja executado em uma arquitetura de
destino por meio de decodificação, representação intermediária e
tradução controlada.

O projeto é desenvolvido em C, com assembly quando necessário. Sua
arquitetura é modular: o `ArmitoBoot` prepara o ambiente, o `Core`
coordena os componentes e o `Decoder` interpreta blocos de instruções
para produzir uma representação intermediária universal.

> O objetivo do Armito é construir uma infraestrutura própria de
> tradução e compatibilidade, não apenas um tradutor isolado de
> instruções.

## Objetivos

-   Criar uma base de tradução entre arquiteturas, começando pelo fluxo
    ARM64 Apple → x86_64.
-   Separar inicialização, orquestração, decodificação, tradução e
    execução.
-   Desenvolver uma representação intermediária independente da
    arquitetura de origem e de destino.
-   Permitir expansão para diferentes CPUs, ambientes e caminhos de
    compatibilidade.
-   Manter o projeto modular, verificável e preparado para integração
    com o ArmitoBoot.

## Arquitetura em alto nível

``` text
ArmitoBoot
    |
    v
   Core
    |
    +---- Registry
    +---- Dependency
    +---- Init / Shutdown
    +---- Dispatch
    +---- Scheduler
    +---- IPC
    +---- Monitor
    +---- Diagnostics
    +---- Topology
    |
    v
  Decoder
    |
    v
 Universal IR
    |
    v
 Translator / Backend
    |
    v
 Target Architecture
```

A topologia acima representa a direção arquitetural pretendida. Nem
todos os componentes ou fluxos estão concluídos ou validados.

## Estado atual

O trabalho inclui a consolidação do `Core` e uma base arquitetural
parcial para Apple ARM64E: identificação de subtype Mach-O, estado PAC,
operações PAC na IR e decodificação de instruções selecionadas. Isso não
representa execução de binários ARM64E. A presença de código não
significa, por si só, que cada subsistema esteja auditado, testado ou
pronto para produção.

Consulte [STATUS.md](STATUS.md) para o estado de desenvolvimento e
[ROADMAP.md](ROADMAP.md) para as próximas etapas.

## Compatibilidade

A direção inicial é ARM64 Apple como origem e x86_64 como destino. O
suporte estrutural ARM64E ainda não inclui loader Mach-O, backend x86_64
ou runtime Apple. O provedor PAC de software serve apenas para testes e
não reproduz a criptografia do hardware. Consulte
[COMPATIBILITY.md](COMPATIBILITY.md) e [STATUS.md](STATUS.md).

## Princípios do projeto

-   C como linguagem principal.
-   Assembly apenas quando necessário e com escopo explícito.
-   Módulos com responsabilidades delimitadas.
-   O `Core` orquestra; não substitui a responsabilidade dos
    subsistemas.
-   `fail.c` é a autoridade de detecção de falhas.
-   Diagnósticos do Core apresentam e encaminham informações; não
    duplicam a detecção.
-   Mudanças devem ser verificáveis e documentadas.

## Documentação

  --------------------------------------------------------------------------
  Documento                              Conteúdo
  -------------------------------------- -----------------------------------
  [WHATISIT.md](WHATISIT.md)             Definição, propósito e visão do
                                         Armito

  [ARCH.md](ARCH.md)                     Arquitetura geral e fluxo entre
                                         componentes

  [CORE.md](CORE.md)                     Estrutura, responsabilidades e
                                         estado do Core

  [IR.md](IR.md)                         Contrato e estado da Universal IR

  [DECODER.md](DECODER.md)               Contrato e estado do Decoder AArch64

  [COMPATIBILITY.md](COMPATIBILITY.md)   Arquiteturas, firmware e ambientes

  [FAILS.md](FAILS.md)                   Modelo de falhas e diagnóstico

  [DECISIONS.md](DECISIONS.md)           Decisões arquiteturais consolidadas

  [ROADMAP.md](ROADMAP.md)               Plano de desenvolvimento

  [STATUS.md](STATUS.md)                 Estado atual, pendências e
                                         validação
  --------------------------------------------------------------------------

## Aviso de maturidade

Armito está em desenvolvimento. Não deve ser tratado como um tradutor
universal funcional até que os caminhos de tradução, execução e
compatibilidade sejam implementados e testados em ambientes reais.
