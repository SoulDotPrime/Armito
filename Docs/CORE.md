# Core

## 1. Papel do Core

Core é o núcleo de orquestração do Armito. Ele gerencia o ciclo de vida
dos módulos, suas dependências, inicialização, execução coordenada,
monitoramento, comunicação e encerramento.

Core não é o Decoder e não deve conter a lógica específica de tradução
de instruções.

## 2. Topologia atual

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
│   ├── Includes/CoreInit.h
│   └── CoreInit.c
├── Registry/
│   ├── Includes/CoreRegistry.h
│   └── CoreRegistry.c
├── Dependency/
│   ├── Includes/CoreDependency.h
│   └── CoreDependency.c
├── Diagnostics/
│   ├── Includes/CoreDiagnostics.h
│   └── CoreDiagnostics.c
├── Dispatch/
│   ├── Includes/CoreDispatch.h
│   └── CoreDispatch.c
├── Monitor/
│   ├── Includes/CoreMonitor.h
│   └── CoreMonitor.c
├── Topology/
│   ├── Includes/CoreTopology.h
│   └── CoreTopology.c
├── Scheduler/
│   ├── Includes/CoreScheduler.h
│   └── CoreScheduler.c
├── IPC/
│   ├── Includes/CoreIPC.h
│   └── CoreIPC.c
├── Shutdown/
│   ├── Includes/CoreShutdown.h
│   └── CoreShutdown.c
└── Core.c
```

## 3. Responsabilidades

  Subsistema    Responsabilidade
  ------------- -------------------------------------------------------------
  Config        Configuração, limites, validação e bloqueio da configuração
  Init          Preparar, inicializar, iniciar e encerrar módulos
  Registry      Registrar módulos e manter seus estados
  Dependency    Registrar e resolver dependências
  Diagnostics   Encaminhar e apresentar informações de falhas
  Dispatch      Despachar chamadas para módulos
  Monitor       Expor estado e métricas de execução
  Topology      Construir e consultar a ordem topológica
  Scheduler     Executar tarefas cooperativas por prioridade
  IPC           Trocar mensagens entre módulos
  Shutdown      Coordenar encerramento e reset do Core

## 4. Tipos e estados

`CoreTypes.h` define os estados gerais do Core, estados dos módulos,
prioridades e identificadores.

Estados de módulo atualmente previstos:

-   `ARMITO_MODULE_UNREGISTERED`
-   `ARMITO_MODULE_REGISTERED`
-   `ARMITO_MODULE_INITIALIZING`
-   `ARMITO_MODULE_INITIALIZED`
-   `ARMITO_MODULE_RUNNING`
-   `ARMITO_MODULE_STOPPED`
-   `ARMITO_MODULE_INITIALIZATION_FAILED`
-   `ARMITO_MODULE_EXECUTION_FAILED`

O significado de cada transição deve ser validado na auditoria do Init e
do Registry.

## 5. Ciclo de vida pretendido

1.  Inicializar Config.
2.  Preparar Registry e Dependency.
3.  Inicializar os serviços-base do Core.
4.  Registrar módulos.
5.  Construir a topologia a partir das dependências.
6.  Inicializar subsistemas opcionais habilitados.
7.  Bloquear a configuração para impedir alterações durante a execução.
8.  Inicializar módulos na ordem resolvida.
9.  Iniciar módulos.
10. Em encerramento, parar módulos na ordem inversa e liberar
    subsistemas.

A sequência exata e o rollback em caso de falha ainda precisam ser
comprovados por testes.

## 6. API pública conhecida

A interface pública principal em `Core/Includes/Core.h` contém:

-   `ArmitoCoreRegisterModule`
-   `ArmitoCoreInitialize`
-   `ArmitoCoreStart`
-   `ArmitoCoreShutdown`
-   `ArmitoCoreGetModuleState`

A API pública ainda precisa ser auditada. A operação de adicionar
dependências, por exemplo, existe na interface interna do subsistema
Dependency, mas precisa de uma decisão explícita sobre sua exposição
pela API pública.

## 7. Configuração atual

A configuração inclui limites para:

-   módulos: máximo estrutural 64;
-   dependências por módulo: máximo estrutural 16;
-   tarefas: máximo estrutural 64;
-   mensagens IPC: máximo estrutural 64;
-   payload IPC: máximo estrutural 256 bytes.

Também há habilitação para Scheduler, IPC, Monitor e Diagnostics. Esses
limites são limites de implementação conhecidos, não uma garantia de
desempenho ou adequação a qualquer plataforma.

## 8. Limitações e pontos de auditoria

-   Rollback parcial de inicialização ainda precisa ser corrigido.
-   Falha durante `initialize()` precisa ter política definida para
    recursos parcialmente alocados.
-   Falha durante `start()` precisa deixar os módulos em estado
    consistente.
-   Shutdown e reinicialização precisam ser testados em ciclos
    repetidos.
-   O encerramento da configuração deve ocorrer depois dos demais
    subsistemas.
-   Registry e Dependency precisam deixar de depender implicitamente de
    IDs sequenciais se o desenho futuro permitir IDs esparsos.
-   Scheduler é cooperativo, sem preempção ou sincronização concorrente.
-   IPC usa fila fixa e ainda precisa de abstração de memória adequada
    ao ambiente freestanding.
-   IPC precisa de política para permissões, estado dos módulos e
    sincronização.
-   Monitor precisa distinguir módulos obrigatórios e opcionais.
-   Diagnostics precisa ser conectado ao `fail.c`, `Display.c` e
    `LogGen.c`.
-   O estado arquitetural AArch64, incluindo o estado PAC ARM64E, fica
    em `Architecture/AARCH64/`; sua integração com o ciclo de execução do Core
    ainda não existe.

## 9. Regra de falhas

Core não detecta nem classifica independentemente falhas globais.
`fail.c` é a autoridade de detecção. Core recebe o resultado e encaminha
a apresentação.

## 10. Critério de conclusão do Core

Core só deve ser considerado consolidado quando: - todos os subsistemas
tiverem contratos e estados definidos; - inicialização e rollback forem
determinísticos; - shutdown e reinicialização forem seguros; -
dependências forem resolvidas e validadas; - APIs públicas e internas
estiverem documentadas; - testes unitários e integrados cobrirem os
caminhos de sucesso e falha; - o build funcionar no ambiente de destino
declarado.
