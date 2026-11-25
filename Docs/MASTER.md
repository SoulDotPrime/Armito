# Armito --- Documento Mestre de Desenvolvimento

**Projeto:** Armito\
**Responsável:** SoulDotPrime\
**Documento:** Especificação geral, arquitetura, estado atual e roadmap\
**Escopo deste documento:** consolidar o que foi definido nas conversas
até 25/11/2026 e servir como referência central de desenvolvimento.

------------------------------------------------------------------------

## 1. O que é o Armito

Armito é um projeto de tradução e compatibilidade entre arquiteturas de
processadores. Seu objetivo é criar uma infraestrutura capaz de receber
instruções de uma arquitetura de origem, interpretá-las, convertê-las
para uma representação intermediária universal e, posteriormente,
entregar essa representação a um componente responsável por produzir ou
executar operações compatíveis com a arquitetura de destino.

A primeira direção de desenvolvimento considerada é **ARM64 para
x86_64**, com atenção especial ao ecossistema Apple Silicon. Entretanto,
a arquitetura do projeto não deve ficar presa a essa combinação. O
objetivo de longo prazo é permitir diferentes combinações de
arquiteturas de origem e destino sem precisar reescrever todo o sistema
para cada par.

Em vez de construir um tradutor monolítico, Armito está sendo organizado
como uma infraestrutura modular. Cada componente possui uma
responsabilidade delimitada e se comunica por contratos definidos pelo
Core.

### 1.1 Visão central

O fluxo conceitual é:

1.  ArmitoBoot prepara o ambiente e inicia o Armito.
2.  ArmitoBoot transfere o controle para o Core.
3.  Core organiza os módulos, valida dependências e controla o ciclo de
    vida.
4.  Core aciona o Decoder no momento apropriado.
5.  Decoder interpreta sequencialmente os blocos de instruções da
    arquitetura de origem.
6.  Decoder produz uma representação intermediária universal, o
    **Universal IR**.
7.  A representação intermediária é encaminhada ao componente de
    tradução/execução de destino, incluindo a infraestrutura CPUToCPU.
8.  O resultado é preparado para a arquitetura de destino.

O Core é o orquestrador. O Decoder não deve ser chamado diretamente por
ArmitoBoot: **ArmitoBoot chama o Core, e o Core decide quando e como
chamar o Decoder.**

### 1.2 O que Armito pretende ser

Armito pretende ser:

-   Uma infraestrutura de tradução entre arquiteturas.
-   Um sistema modular, extensível e independente de um único par de
    arquiteturas.
-   Uma camada de abstração entre decodificação, representação
    intermediária e destino.
-   Uma base para compatibilidade de instruções e operações entre CPUs
    diferentes.
-   Um projeto que possa evoluir de uma primeira tradução ARM64 → x86_64
    para múltiplos alvos.
-   Um componente integrável ao fluxo de inicialização do ArmitoBoot.

### 1.3 O que Armito não deve ser

Armito não deve se tornar:

-   Um Decoder monolítico que contenha toda a lógica de tradução.
-   Um sistema em que ArmitoBoot conheça e controle diretamente cada
    subsistema.
-   Uma implementação limitada permanentemente a ARM64 e x86_64.
-   Um sistema em que cada Decoder produza uma saída exclusiva e
    incompatível com os demais.
-   Um sistema que duplique a detecção de falhas em vários módulos.

------------------------------------------------------------------------

## 2. Princípios de arquitetura

### 2.1 Modularidade

Cada subsistema deve ter uma responsabilidade explícita, uma API
definida e dependências controladas.

### 2.2 Representação intermediária universal

O Universal IR é o contrato central entre os componentes que interpretam
arquiteturas de origem e os componentes que traduzem ou executam
operações para arquiteturas de destino.

A intenção é que diferentes Decoders possam produzir o mesmo formato
intermediário e que diferentes backends possam consumi-lo. Assim, a
combinação de uma origem e um destino não exige necessariamente um
tradutor inteiramente independente.

O formato final do IR ainda precisa ser especificado. Não se deve
considerar sua semântica, codificação binária, modelo de registradores
ou regras de memória como concluídos antes dessa especificação.

### 2.3 Separação entre origem e destino

O Decoder deve conhecer as regras necessárias para interpretar a
arquitetura de origem. A tradução para uma arquitetura de destino deve
pertencer aos componentes responsáveis pelo backend e pela
infraestrutura CPUToCPU.

### 2.4 Orquestração centralizada

Core controla o ciclo de vida e a execução dos módulos. Os módulos não
devem iniciar uns aos outros de maneira arbitrária.

### 2.5 Falhas com autoridade única

`fail.c` é o responsável por detectar falhas. Core não deve duplicar
essa detecção nem criar uma segunda classificação concorrente.

Core apenas instrumentaliza a apresentação e o encaminhamento das
informações:

-   `Display.c`: apresentação visual.
-   `LogGen.c`: geração e registro de logs.
-   `Core/Diagnostics`: adaptação das informações de falha para os
    mecanismos de exibição e registro.

### 2.6 Portabilidade

O projeto deve evitar dependências acidentais do ambiente de
desenvolvimento. O Core precisa poder evoluir para um ambiente
freestanding/UEFI, com abstrações próprias para memória, serviços e
recursos de plataforma.

### 2.7 Código e manutenção

Convenções de desenvolvimento estabelecidas:

-   Linguagem principal: C.
-   Assembly pode ser utilizado quando necessário.
-   Rust não faz parte da implementação planejada.
-   Alterações devem ser entregues como arquivos completos, não como
    patches parciais.
-   Cada alteração deve indicar o caminho exato do arquivo.
-   Não inserir comentários dentro do código-fonte.
-   Desenvolver uma alteração por vez e confirmar sua aplicação antes de
    avançar.

------------------------------------------------------------------------

## 3. Arquitetura geral

A arquitetura lógica pode ser dividida em quatro grandes áreas.

### 3.1 Inicialização

Responsável por preparar o ambiente de execução e transferir o controle
para o Core.

Componente principal: **ArmitoBoot**.

### 3.2 Orquestração

Responsável por inicializar, registrar, organizar, executar, monitorar e
encerrar os módulos.

Componente principal: **Core**.

### 3.3 Tradução

Responsável por interpretar instruções da origem, produzir o Universal
IR e preparar operações para o destino.

Componentes centrais:

-   Decoder.
-   Universal IR.
-   CPUToCPU.
-   Adaptadores de arquitetura de origem.
-   Backends de arquitetura de destino.

### 3.4 Plataforma e integração

Responsável por fornecer os serviços de baixo nível necessários ao
funcionamento do sistema:

-   Memória.
-   Diagnósticos.
-   Logs.
-   Exibição.
-   Serviços de boot.
-   Interfaces específicas de plataforma.

------------------------------------------------------------------------

## 4. Fluxo de controle esperado

``` text
ArmitoBoot
    |
    v
  Core
    |
    +--> Config
    |
    +--> Registry
    |
    +--> Dependency
    |
    +--> Topology
    |
    +--> Init
    |
    +--> Diagnostics
    |
    +--> Monitor
    |
    +--> Scheduler
    |
    +--> IPC
    |
    v
 Decoder
    |
    v
 Universal IR
    |
    v
 CPUToCPU / Backend
    |
    v
 Arquitetura de destino
```

Este desenho representa o fluxo pretendido, não uma afirmação de que
todos os componentes já estejam implementados ou integrados.

------------------------------------------------------------------------

## 5. Core --- núcleo de orquestração

Core é a camada que governa a execução interna do Armito. Ele não deve
conter a lógica específica de decodificação ARM64, x86_64 ou de qualquer
outra arquitetura.

### 5.1 Responsabilidades

-   Gerenciar a configuração central.
-   Registrar módulos.
-   Controlar estados dos módulos.
-   Gerenciar dependências.
-   Resolver a ordem de inicialização.
-   Inicializar e iniciar módulos.
-   Despachar operações.
-   Coordenar tarefas.
-   Permitir comunicação entre módulos.
-   Monitorar estados.
-   Apresentar diagnósticos recebidos de `fail.c`.
-   Encerrar o sistema de forma ordenada.

### 5.2 Topologia atual do Core

``` text
Core/
├── Includes/
│   ├── Core.h
│   └── CoreTypes.h
├── Config/
│   ├── Includes/
│   │   ├── CoreConfig.h
│   │   └── CoreConfigValidation.h
│   ├── CoreConfig.c
│   └── CoreConfigValidation.c
├── Init/
│   ├── Includes/
│   │   └── CoreInit.h
│   └── CoreInit.c
├── Registry/
│   ├── Includes/
│   │   └── CoreRegistry.h
│   └── CoreRegistry.c
├── Dependency/
│   ├── Includes/
│   │   └── CoreDependency.h
│   └── CoreDependency.c
├── Diagnostics/
│   ├── Includes/
│   │   └── CoreDiagnostics.h
│   └── CoreDiagnostics.c
├── Dispatch/
│   ├── Includes/
│   │   └── CoreDispatch.h
│   └── CoreDispatch.c
├── Monitor/
│   ├── Includes/
│   │   └── CoreMonitor.h
│   └── CoreMonitor.c
├── Topology/
│   ├── Includes/
│   │   └── CoreTopology.h
│   └── CoreTopology.c
├── Scheduler/
│   ├── Includes/
│   │   └── CoreScheduler.h
│   └── CoreScheduler.c
├── IPC/
│   ├── Includes/
│   │   └── CoreIPC.h
│   └── CoreIPC.c
├── Shutdown/
│   ├── Includes/
│   │   └── CoreShutdown.h
│   └── CoreShutdown.c
└── Core.c
```

### 5.3 Subsistemas

  -----------------------------------------------------------------------
  Subsistema                          Responsabilidade
  ----------------------------------- -----------------------------------
  Config                              Configuração central, limites e
                                      funcionalidades habilitáveis

  Init                                Preparação, inicialização e início
                                      dos módulos

  Registry                            Registro e consulta dos módulos

  Dependency                          Cadastro e validação das
                                      dependências

  Topology                            Construção da ordem de execução

  Dispatch                            Despacho controlado de operações

  Scheduler                           Gerenciamento cooperativo de
                                      tarefas

  IPC                                 Comunicação entre módulos

  Monitor                             Acompanhamento de estados e
                                      indicadores

  Diagnostics                         Encaminhamento e apresentação de
                                      informações de falha

  Shutdown                            Encerramento ordenado dos
                                      subsistemas

  Core.c                              API pública e coordenação do ciclo
                                      de vida
  -----------------------------------------------------------------------

### 5.4 Estados de módulos

Os estados atualmente definidos em `CoreTypes.h` são:

-   `ARMITO_MODULE_UNREGISTERED`
-   `ARMITO_MODULE_REGISTERED`
-   `ARMITO_MODULE_INITIALIZING`
-   `ARMITO_MODULE_INITIALIZED`
-   `ARMITO_MODULE_RUNNING`
-   `ARMITO_MODULE_STOPPED`
-   `ARMITO_MODULE_INITIALIZATION_FAILED`
-   `ARMITO_MODULE_EXECUTION_FAILED`

### 5.5 Status do Core

Os status atualmente definidos incluem:

-   Sucesso.
-   Erro genérico.
-   Estado inválido.
-   Não inicializado.
-   Já inicializado.
-   Recursos esgotados.
-   Erro de dependência.
-   Falha de inicialização.
-   Falha de execução.
-   Erro de configuração.
-   Timeout.
-   Operação não suportada.

------------------------------------------------------------------------

## 6. Estado de desenvolvimento conhecido

Este estado representa o histórico de desenvolvimento consolidado até
25/11/2026. Itens marcados como implementados ainda precisam passar por
auditoria e testes antes de serem considerados estáveis.

### 6.1 Core --- estrutura

-   [x] Estrutura modular inicial criada.
-   [x] Tipos e estados básicos definidos.
-   [x] API pública inicial do Core definida.
-   [x] Registry implementado em sua primeira versão.
-   [x] Dependency implementado em sua primeira versão.
-   [x] Init implementado em sua primeira versão.
-   [x] Diagnostics implementado em sua primeira versão.
-   [x] Dispatch implementado em sua primeira versão.
-   [x] Monitor implementado em sua primeira versão.
-   [x] Topology implementado em sua primeira versão.
-   [x] Scheduler cooperativo implementado em sua primeira versão.
-   [x] IPC por fila implementado em sua primeira versão.
-   [x] Shutdown implementado em sua primeira versão.
-   [x] Core.c conectado aos subsistemas principais.

### 6.2 Configuração central --- 01.02

A configuração central foi iniciada e recebeu integração com Registry e
com os subsistemas opcionais.

-   [x] Estrutura de configuração.
-   [x] Valores padrão.
-   [x] Inicialização.
-   [x] Alteração de configuração.
-   [x] Consulta de configuração.
-   [x] Validação de limites.
-   [x] Limites aplicados ao Registry.
-   [x] Controle de bloqueio da configuração.
-   [x] Consulta de funcionalidades habilitadas.
-   [x] Aplicação da configuração no ciclo de vida do Core.
-   [x] Proteções de funcionalidades nos subsistemas Scheduler, IPC,
    Monitor e Diagnostics, conforme alterações trabalhadas.
-   [ ] Integração final do encerramento da configuração com Shutdown.
-   [ ] Testes de configuração e reinicialização.

**Observação:** a implementação de `ArmitoCoreConfigShutdown()` existe.
A integração de sua chamada no encerramento coordenado ainda precisa ser
confirmada no `CoreShutdown.c`.

### 6.3 Pontos técnicos conhecidos que ainda precisam de auditoria

-   Registry copia o descritor do módulo, mas o campo `name` é uma
    referência emprestada. É necessário definir a política de vida útil
    dessa string.
-   Dependency pressupõe IDs sequenciais em algumas operações. Essa
    suposição precisa ser removida ou formalizada.
-   Init ainda precisa de rollback robusto quando um módulo falha
    parcialmente durante a inicialização.
-   O encerramento precisa garantir a ordem inversa correta e ser seguro
    em estados parciais.
-   Monitor precisa tratar módulos opcionais sem declarar o sistema não
    saudável apenas porque um módulo desabilitado não está em execução.
-   Scheduler é cooperativo e não preemptivo na implementação atual.
-   Scheduler mantém tarefas concluídas ou falhas registradas; ainda não
    há política de remoção/reutilização de slots.
-   IPC usa `memcpy` e `<string.h>` na versão apresentada. Isso precisa
    ser revisto para o ambiente freestanding/EDK2.
-   IPC ainda precisa de política de sincronização, permissões e
    validação do estado do destinatário.
-   As estruturas de Scheduler e IPC usam tabelas fixas e limites
    máximos; a configuração reduz os limites efetivos.
-   Core.c ainda precisa de rollback completo caso a inicialização de um
    subsistema falhe no meio do processo.
-   Falhas em `InitExecute` ou `InitStart` precisam deixar o Core em um
    estado consistente para uma nova tentativa ou encerramento.
-   A API pública do Core precisa expor formalmente as operações de
    dependência necessárias aos módulos clientes, sem exigir que
    consumidores incluam cabeçalhos internos.
-   A integração real com `fail.c`, `Display.c` e `LogGen.c` ainda está
    pendente.
-   A compilação independente e os testes automatizados ainda precisam
    ser estabelecidos.

------------------------------------------------------------------------

## 7. Decoder

Decoder é o componente que interpreta instruções da arquitetura de
origem. Ele será acionado pelo Core, e não diretamente por ArmitoBoot.

### 7.1 Responsabilidades previstas

-   Receber blocos ou sequências de instruções.
-   Identificar a arquitetura de origem.
-   Interpretar instruções sequencialmente.
-   Validar os dados de entrada.
-   Decodificar instruções e operandos.
-   Identificar operações, registradores, memória e fluxo de controle.
-   Produzir instruções no Universal IR.
-   Reportar erros de decodificação ao mecanismo central de falhas.
-   Entregar o resultado ao próximo estágio por meio de uma interface
    definida.

### 7.2 Regra de independência

O Decoder não deve produzir diretamente código específico de uma única
arquitetura de destino. Sua saída primária é o Universal IR.

### 7.3 Decoders por arquitetura

A arquitetura deve permitir implementações específicas, por exemplo:

-   Decoder ARM64.
-   Decoder x86_64.
-   Outros decoders futuros.

Todos devem obedecer ao contrato comum de saída.

### 7.4 Itens ainda a especificar

-   Formato de entrada.
-   Identificação da arquitetura de origem.
-   Modelo de contexto de decodificação.
-   Representação de instruções inválidas ou não suportadas.
-   Tratamento de instruções privilegiadas.
-   Tratamento de exceções e condições especiais.
-   Regras de leitura de blocos e limites de memória.
-   Contrato de saída para o Universal IR.

------------------------------------------------------------------------

## 8. Universal IR

O Universal IR é a representação intermediária comum do Armito. É um dos
elementos mais importantes para a independência entre arquiteturas.

### 8.1 Objetivo

Permitir que um Decoder produza uma representação que possa ser
consumida por diferentes backends, sem que o Decoder precise conhecer
todos os destinos possíveis.

### 8.2 Requisitos arquiteturais

O IR deverá representar, no mínimo, conceitos como:

-   Operações aritméticas e lógicas.
-   Movimento de dados.
-   Operações de memória.
-   Registradores abstratos e mapeamento de registradores.
-   Comparações e condições.
-   Saltos e fluxo de controle.
-   Chamadas e retornos.
-   Conversões e extensões de dados.
-   Operações vetoriais, quando suportadas.
-   Barreiras e operações de sincronização.
-   Operações não suportadas ou que exijam tratamento especial.

A lista é uma base de requisitos, não uma especificação final.

### 8.3 Decisões ainda pendentes

-   Modelo SSA ou outro modelo de temporários.
-   Tipagem dos operandos.
-   Representação de flags e condições.
-   Modelo de memória e endianness.
-   Representação de exceções.
-   Metadados de origem.
-   Versão e compatibilidade do IR.
-   Formato em memória e eventual formato serializado.
-   Estratégia de validação do IR.

------------------------------------------------------------------------

## 9. CPUToCPU e backends

CPUToCPU faz parte da infraestrutura que conecta a representação
intermediária à arquitetura de destino.

### 9.1 Responsabilidade pretendida

-   Consumir o Universal IR.
-   Aplicar regras específicas da arquitetura de destino.
-   Resolver o mapeamento de registradores.
-   Resolver operações e convenções de chamada.
-   Gerar operações ou código compatível com o destino.
-   Tratar operações sem equivalência direta.
-   Reportar operações não suportadas.

### 9.2 Arquitetura de destino

A primeira meta de compatibilidade considerada é x86_64. A implementação
deve evitar que o backend x86_64 determine o formato interno do Decoder
ou do IR.

### 9.3 Extensibilidade

O projeto deve permitir adicionar outros backends sem alterar o contrato
central do Decoder.

------------------------------------------------------------------------

## 10. ArmitoBoot e relação com Core

ArmitoBoot é o responsável pela etapa de inicialização e preparação do
ambiente. Core é o responsável pela orquestração do Armito depois que
recebe o controle.

### 10.1 Divisão de responsabilidades

  -----------------------------------------------------------------------
  ArmitoBoot                          Core
  ----------------------------------- -----------------------------------
  Prepara o ambiente de boot          Gerencia o ambiente interno do
                                      Armito

  Obtém e organiza os dados iniciais  Registra e organiza módulos

  Inicializa os serviços necessários  Inicializa os subsistemas e módulos
  ao Core                             

  Invoca a API pública do Core        Controla o ciclo de vida

  Entrega o controle ao Core          Decide quando acionar o Decoder

  Trata a transição de boot           Coordena execução, comunicação e
                                      encerramento
  -----------------------------------------------------------------------

### 10.2 Regra de integração

ArmitoBoot não deve chamar Decoder diretamente. A integração deve
ocorrer por meio de um contrato público e estável entre ArmitoBoot e
Core.

### 10.3 Compatibilidade de boot

Foi discutida a possibilidade de suportar diferentes caminhos de
inicialização:

-   UEFI nativo.
-   UEFI com CSM.
-   Legacy BIOS, mediante um estágio de boot específico.

Um executável EFI não é, por si só, um executável de boot para BIOS
Legacy puro. Portanto, suporte a Legacy exige uma etapa própria
compatível com o modo de inicialização correspondente.

O caminho CSM também não deve pressupor disponibilidade de GOP ou de
todos os protocolos UEFI. A camada de plataforma deverá detectar e expor
os recursos realmente disponíveis.

------------------------------------------------------------------------

## 11. Falhas, diagnósticos e logs

### 11.1 Autoridade de falha

`fail.c` permanece como autoridade central para detectar falhas. Nenhum
subsistema do Core deve duplicar essa função.

### 11.2 Encaminhamento

O fluxo previsto é:

``` text
Componente que detecta a condição
          |
          v
        fail.c
          |
          v
   Core/Diagnostics
       /       \
      v         v
 Display.c   LogGen.c
```

O contrato real de integração ainda precisa ser definido a partir das
APIs existentes desses componentes.

### 11.3 Requisitos

-   Preservar a origem da falha.
-   Preservar código e severidade.
-   Permitir diagnóstico durante inicialização.
-   Permitir diagnóstico quando a interface gráfica não estiver
    disponível.
-   Não depender exclusivamente de Display.
-   Não transformar uma falha de apresentação em uma nova falha fatal.
-   Registrar informações suficientes para depuração.

------------------------------------------------------------------------

## 12. Memória e ambiente freestanding

O Core começou com estruturas estáticas e limites fixos. Isso facilita a
primeira implementação, mas não encerra o projeto de gerenciamento de
memória.

### 12.1 Objetivos

-   Criar uma abstração de memória própria.
-   Remover dependências incompatíveis com o ambiente final.
-   Definir alocação, liberação, alinhamento e limites.
-   Permitir integração com os serviços de memória disponíveis no boot.
-   Evitar que cada módulo implemente seu próprio mecanismo de memória.

### 12.2 Compatibilidade EDK2

A integração com EDK2 deve revisar:

-   Tipos e convenções de chamada.
-   Bibliotecas disponíveis.
-   Funções de memória.
-   Alinhamento.
-   Alocação e liberação.
-   Organização dos arquivos e build.
-   Dependências de bibliotecas padrão.
-   Uso de serviços UEFI somente quando disponíveis e válidos no estágio
    atual.

------------------------------------------------------------------------

## 13. Roadmap mestre

O roadmap abaixo organiza o trabalho desde a consolidação do Core até a
integração com ArmitoBoot. Os itens ainda não concluídos são objetivos
de desenvolvimento, não funcionalidades já existentes.

### Fase 01 --- Auditoria estrutural do Core

-   [x] 01.01 --- Revisão dos tipos e estados (`CoreTypes`)
-   [ ] 01.02 --- Configuração central (`CoreConfig`)
    -   [x] 01.02.01 --- Estrutura de configuração
    -   [x] 01.02.02 --- Valores padrão
    -   [x] 01.02.03 --- Inicialização
    -   [x] 01.02.04 --- Alteração de configuração
    -   [x] 01.02.05 --- Consulta de configuração
    -   [x] 01.02.06 --- Validação
    -   [x] 01.02.07 --- Limites dos subsistemas
    -   [x] 01.02.08 --- Controle de bloqueio
    -   [x] 01.02.09 --- Integração com Registry
    -   [x] 01.02.10 --- Aplicação da configuração no Core
    -   [x] 01.02.11 --- Consulta de funcionalidades
    -   [x] 01.02.12 --- Implementação da consulta
    -   [x] 01.02.13 --- Correção do momento de aplicação
    -   [x] 01.02.14 --- Proteção do Scheduler
    -   [x] 01.02.15 --- Proteção do IPC
    -   [x] 01.02.16 --- Proteção do Monitor
    -   [x] 01.02.17 --- Proteção do Diagnostics
    -   [x] 01.02.18 --- Declaração de encerramento
    -   [x] 01.02.19 --- Implementação de encerramento
    -   [ ] 01.02.20 --- Integração do encerramento da Config com
        Shutdown
    -   [ ] 01.02.21 --- Testes de ciclo de vida da Config
-   [ ] 01.03 --- Auditoria do Init
-   [ ] 01.04 --- Auditoria do Registry
-   [ ] 01.05 --- Auditoria do Dependency
-   [ ] 01.06 --- Auditoria do Dispatch
-   [ ] 01.07 --- Auditoria do Monitor
-   [ ] 01.08 --- Auditoria do Topology
-   [ ] 01.09 --- Auditoria do Scheduler
-   [ ] 01.10 --- Auditoria do IPC
-   [ ] 01.11 --- Auditoria do Diagnostics
-   [ ] 01.12 --- Auditoria do Shutdown
-   [ ] 01.13 --- Auditoria do Core.c e API pública

### Fase 02 --- Consolidação do Core

-   [ ] 02.01 --- Corrigir dependências entre subsistemas
-   [ ] 02.02 --- Padronizar retornos e estados
-   [ ] 02.03 --- Implementar rollback de inicialização
-   [ ] 02.04 --- Corrigir encerramento e reinicialização
-   [ ] 02.05 --- Revisar APIs públicas e internas
-   [ ] 02.06 --- Definir política de vida útil dos descritores e nomes
-   [ ] 02.07 --- Remover suposições frágeis sobre IDs
-   [ ] 02.08 --- Definir política de módulos opcionais
-   [ ] 02.09 --- Consolidar contratos entre subsistemas

### Fase 03 --- Infraestrutura de execução

-   [ ] 03.01 --- Orquestração de módulos
-   [ ] 03.02 --- Dependências e ordenação topológica
-   [ ] 03.03 --- Despacho de execução
-   [ ] 03.04 --- Scheduler cooperativo
-   [ ] 03.05 --- Comunicação IPC
-   [ ] 03.06 --- Monitoramento de execução
-   [ ] 03.07 --- Contratos de tarefas e operações
-   [ ] 03.08 --- Política de sincronização
-   [ ] 03.09 --- Política de permissões entre módulos

### Fase 04 --- Memória e portabilidade

-   [ ] 04.01 --- Abstração de memória
-   [ ] 04.02 --- Remover dependências incompatíveis com ambiente
    freestanding
-   [ ] 04.03 --- Compatibilidade EDK2
-   [ ] 04.04 --- Gerenciamento de recursos
-   [ ] 04.05 --- Revisão de alinhamento e tipos
-   [ ] 04.06 --- Política de alocação e liberação
-   [ ] 04.07 --- Testes de limites e falhas de alocação

### Fase 05 --- Diagnósticos e recuperação

-   [ ] 05.01 --- Integração com `fail.c`
-   [ ] 05.02 --- Integração com `Display.c`
-   [ ] 05.03 --- Integração com `LogGen.c`
-   [ ] 05.04 --- Propagação de falhas
-   [ ] 05.05 --- Recuperação e encerramento seguro
-   [ ] 05.06 --- Diagnóstico sem GOP
-   [ ] 05.07 --- Logs disponíveis em diferentes estágios de boot

### Fase 06 --- Compilação e testes do Core

-   [ ] 06.01 --- Compilação independente do Core
-   [ ] 06.02 --- Testes unitários
-   [ ] 06.03 --- Testes de dependências
-   [ ] 06.04 --- Testes de falhas
-   [ ] 06.05 --- Testes de inicialização e shutdown
-   [ ] 06.06 --- Testes em ambiente UEFI
-   [ ] 06.07 --- Testes de reinicialização
-   [ ] 06.08 --- Testes com subsistemas opcionais desabilitados
-   [ ] 06.09 --- Testes de limites de recursos

### Fase 07 --- Contrato do Decoder e Universal IR

-   [ ] 07.01 --- Definir contrato Core ↔ Decoder
-   [ ] 07.02 --- Definir estrutura de contexto de decodificação
-   [ ] 07.03 --- Definir formato de entrada
-   [ ] 07.04 --- Definir arquitetura de origem e identificação
-   [ ] 07.05 --- Especificar o Universal IR
-   [ ] 07.06 --- Definir validação do IR
-   [ ] 07.07 --- Definir erros e operações não suportadas
-   [ ] 07.08 --- Definir metadados de rastreabilidade
-   [ ] 07.09 --- Criar testes de conformidade do IR

### Fase 08 --- Decoder

-   [ ] 08.01 --- Estrutura modular do Decoder
-   [ ] 08.02 --- API pública do Decoder
-   [ ] 08.03 --- Contexto de decodificação
-   [ ] 08.04 --- Decodificação sequencial
-   [ ] 08.05 --- Primeiro Decoder de arquitetura
-   [ ] 08.06 --- Conversão para Universal IR
-   [ ] 08.07 --- Tratamento de instruções inválidas
-   [ ] 08.08 --- Testes com amostras conhecidas
-   [ ] 08.09 --- Integração do Decoder com Core

### Fase 09 --- CPUToCPU e backends

-   [ ] 09.01 --- Definir contrato Universal IR ↔ CPUToCPU
-   [ ] 09.02 --- Estruturar CPUToCPU
-   [ ] 09.03 --- Definir interface de backend
-   [ ] 09.04 --- Primeiro backend de destino
-   [ ] 09.05 --- Mapeamento de registradores
-   [ ] 09.06 --- Tradução de operações aritméticas e lógicas
-   [ ] 09.07 --- Tradução de memória
-   [ ] 09.08 --- Tradução de fluxo de controle
-   [ ] 09.09 --- Tratamento de operações sem equivalência direta
-   [ ] 09.10 --- Testes de equivalência funcional

### Fase 10 --- Integração de plataforma

-   [ ] 10.01 --- Definir abstração de plataforma
-   [ ] 10.02 --- Integrar serviços de memória
-   [ ] 10.03 --- Integrar serviços de diagnóstico
-   [ ] 10.04 --- Integrar saída visual
-   [ ] 10.05 --- Integrar logs
-   [ ] 10.06 --- Validar execução sem recursos gráficos
-   [ ] 10.07 --- Validar caminhos UEFI e CSM disponíveis

### Fase 11 --- Preparação do ArmitoBoot

-   [ ] 11.01 --- Definir contrato Core ↔ ArmitoBoot
-   [ ] 11.02 --- Definir estrutura de contexto de boot
-   [ ] 11.03 --- Definir transferência de controle
-   [ ] 11.04 --- Integrar diagnóstico de boot
-   [ ] 11.05 --- Integrar gerenciamento de memória
-   [ ] 11.06 --- Definir tratamento de serviços ausentes
-   [ ] 11.07 --- Definir estados de sucesso e falha do boot

### Fase 12 --- Integração final

-   [ ] 12.01 --- ArmitoBoot inicializa o Core
-   [ ] 12.02 --- Core inicializa os módulos
-   [ ] 12.03 --- Core aciona o Decoder
-   [ ] 12.04 --- Decoder produz Universal IR
-   [ ] 12.05 --- Universal IR é entregue ao CPUToCPU
-   [ ] 12.06 --- Backend prepara operações para o destino
-   [ ] 12.07 --- Validar comunicação entre módulos
-   [ ] 12.08 --- Testar ciclo completo de boot
-   [ ] 12.09 --- Validar integração final

------------------------------------------------------------------------

## 14. Critérios de conclusão

O Armito não deve ser considerado integrado apenas porque compila. A
conclusão precisa ser demonstrada por testes em cada camada.

### 14.1 Core concluído

-   Todos os subsistemas possuem contratos definidos.
-   Inicialização e encerramento são previsíveis.
-   Falhas parciais não deixam o Core em estado inconsistente.
-   Dependências são resolvidas corretamente.
-   Módulos opcionais são tratados corretamente.
-   Testes automatizados cobrem estados válidos e inválidos.
-   O Core compila no ambiente-alvo.

### 14.2 Decoder concluído para uma arquitetura

-   Recebe entrada válida e delimitada.
-   Decodifica instruções suportadas.
-   Rejeita ou sinaliza instruções inválidas.
-   Produz Universal IR válido.
-   Possui testes de conformidade.

### 14.3 Backend concluído para um destino

-   Consome Universal IR validado.
-   Traduz operações suportadas.
-   Sinaliza operações não suportadas.
-   Mantém semântica funcional nas operações cobertas.
-   Possui testes comparativos.

### 14.4 Integração final concluída

-   ArmitoBoot inicia o Core.
-   Core controla os módulos.
-   Core aciona Decoder.
-   Decoder produz IR.
-   CPUToCPU recebe o IR.
-   O backend de destino processa as operações.
-   Diagnósticos e logs funcionam nos caminhos de sucesso e falha.
-   O ciclo completo é repetível e testado.

------------------------------------------------------------------------

## 15. Registro de decisões e regras permanentes

1.  O projeto é chamado **Armito**.
2.  O responsável pelo projeto, nos contextos do ecossistema, é
    **SoulDotPrime**.
3.  A linguagem principal é C; Assembly pode ser usado quando
    necessário.
4.  Rust está fora do escopo atual.
5.  Core é o orquestrador central.
6.  ArmitoBoot chama Core; não chama Decoder diretamente.
7.  Core decide quando o Decoder entra em ação.
8.  Decoder trabalha com instruções da arquitetura de origem.
9.  Decoder entrega Universal IR, não código específico de um destino.
10. CPUToCPU e os backends cuidam da adaptação ao destino.
11. `fail.c` é a autoridade de detecção de falhas.
12. Core apenas instrumentaliza a apresentação e o registro das falhas.
13. `Display.c` é o caminho de apresentação visual.
14. `LogGen.c` é o caminho de geração de logs.
15. O suporte a Legacy BIOS exige um estágio próprio; um binário EFI não
    é suficiente.
16. A arquitetura deve permitir múltiplas origens e múltiplos destinos.
17. Alterações de código devem ser fornecidas como arquivos completos.
18. Cada alteração deve informar o caminho exato do arquivo.
19. Não inserir comentários dentro do código-fonte.
20. O desenvolvimento deve avançar em etapas pequenas, com confirmação
    antes da próxima alteração.

------------------------------------------------------------------------

## 16. Como manter este documento

Este arquivo é a referência mestre do projeto, mas não substitui os
contratos técnicos individuais de cada módulo.

A cada avanço relevante:

1.  Atualizar o estado do roadmap.
2.  Registrar decisões arquiteturais novas.
3.  Atualizar a topologia se arquivos ou subsistemas mudarem.
4.  Registrar limitações conhecidas.
5.  Diferenciar claramente o que está implementado, o que foi testado e
    o que está apenas planejado.
6.  Não marcar uma etapa como concluída somente porque o código foi
    escrito; exigir confirmação de aplicação e, quando aplicável, teste.

**Regra de rastreabilidade:** implementação, validação e integração são
estados diferentes. Um componente pode estar implementado sem estar
validado, e validado isoladamente sem estar integrado ao fluxo completo
do Armito.