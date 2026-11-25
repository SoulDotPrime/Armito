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

O contrato exato entre ArmitoBoot e Core será definido antes da
integração final.

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
entre arquiteturas.

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

-   ArmitoBoot inicializa o Core.
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
