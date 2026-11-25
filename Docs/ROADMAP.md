# Armito Roadmap

## Objetivo final

Consolidar o Core, construir e validar o caminho de tradução até a
Universal IR e os backends, integrar o fluxo com ArmitoBoot e executar
um ciclo de boot e tradução reproduzível.

O roadmap distingue desenvolvimento de código, integração e validação.
Uma etapa só deve ser marcada como concluída quando seus critérios forem
atendidos.

## Fase 01 --- Auditoria estrutural do Core

-   [x] 01.01 --- Revisão inicial de tipos e estados (`CoreTypes`)
-   [ ] 01.02 --- Consolidar Config
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
    -   [ ] 01.02.20 --- Integrar Config ao gerenciador de shutdown
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

## Fase 02 --- Consolidação do Core

-   [ ] 02.01 --- Corrigir dependências entre subsistemas
-   [ ] 02.02 --- Padronizar retornos e transições de estado
-   [ ] 02.03 --- Implementar rollback completo de inicialização
-   [ ] 02.04 --- Corrigir encerramento e reinicialização
-   [ ] 02.05 --- Revisar APIs públicas e internas
-   [ ] 02.06 --- Definir política de módulos obrigatórios e opcionais
-   [ ] 02.07 --- Definir contratos de erro e timeout

## Fase 03 --- Memória e portabilidade

-   [ ] 03.01 --- Definir abstração de alocação e liberação
-   [ ] 03.02 --- Remover dependências incompatíveis com ambiente
    freestanding
-   [ ] 03.03 --- Integrar bibliotecas e serviços EDK2 necessários
-   [ ] 03.04 --- Definir gerenciamento e propriedade de recursos
-   [ ] 03.05 --- Revisar alinhamento, largura e tipos
-   [ ] 03.06 --- Definir limites estáticos e dinâmicos

## Fase 04 --- Diagnósticos e recuperação

-   [ ] 04.01 --- Mapear interface de `fail.c`
-   [ ] 04.02 --- Integrar `CoreDiagnostics` com `fail.c`
-   [ ] 04.03 --- Integrar `Display.c`
-   [ ] 04.04 --- Integrar `LogGen.c`
-   [ ] 04.05 --- Definir fallback sem GOP
-   [ ] 04.06 --- Definir prevenção de reentrada e falhas secundárias
-   [ ] 04.07 --- Validar encerramento seguro

## Fase 05 --- Decoder

-   [ ] 05.01 --- Definir API pública e contrato de entrada
-   [ ] 05.02 --- Definir representação de bloco de instruções
-   [ ] 05.03 --- Definir limites e validação de memória de origem
-   [ ] 05.04 --- Implementar decodificação sequencial
-   [ ] 05.05 --- Implementar identificação de instruções
-   [ ] 05.06 --- Implementar extração de operandos
-   [ ] 05.07 --- Tratar instruções inválidas e não suportadas
-   [ ] 05.08 --- Criar testes de decodificação

## Fase 06 --- Universal IR

-   [x] 06.01 --- Definir modelo de tipos e larguras
-   [x] 06.02 --- Definir registradores virtuais e estado de CPU
-   [x] 06.03 --- Definir operações aritméticas e lógicas
-   [x] 06.04 --- Definir operações de memória
-   [x] 06.05 --- Definir flags e condições
-   [x] 06.06 --- Definir blocos básicos e fluxo de controle
-   [x] 06.07 --- Definir chamadas, retornos e efeitos colaterais
-   [ ] 06.08 --- Definir validação e serialização, se necessária
    -   [x] 06.08.01 --- Validação estrutural e semântica
    -   [ ] 06.08.02 --- Serialização / desserialização
-   [ ] 06.09 --- Criar testes semânticos da IR
    -   [x] 06.09.01 --- Testes de contrato (`Tests/IRTest.c`, 172
        verificações)
    -   [ ] 06.09.02 --- Testes semânticos com instruções AArch64 reais
-   [ ] 06.10 --- Phi / merge de valores em blocos
-   [ ] 06.11 --- Integrar a IR ao ciclo do Core como módulo

## Fase 07 --- Tradução e backend x86_64

-   [ ] 07.01 --- Definir contrato Decoder → IR → Backend
-   [ ] 07.02 --- Mapear operações IR para x86_64
-   [ ] 07.03 --- Implementar alocação de registradores
-   [ ] 07.04 --- Implementar geração de fluxo de controle
-   [ ] 07.05 --- Implementar acessos à memória
-   [ ] 07.06 --- Tratar operações sem mapeamento direto
-   [ ] 07.07 --- Validar código emitido
-   [ ] 07.08 --- Testar equivalência semântica

## Fase 08 --- Ambiente de execução

-   [ ] 08.01 --- Definir ABI de destino
-   [ ] 08.02 --- Definir convenções de chamada
-   [ ] 08.03 --- Definir tratamento de exceções
-   [ ] 08.04 --- Definir chamadas de sistema e serviços
-   [ ] 08.05 --- Definir integração com bibliotecas
-   [ ] 08.06 --- Definir política de instruções privilegiadas
-   [ ] 08.07 --- Validar execução de programas de teste

## Fase 09 --- ArmitoBoot

-   [ ] 09.01 --- Definir contrato Core ↔ ArmitoBoot
-   [ ] 09.02 --- Definir estrutura de contexto de boot
-   [ ] 09.03 --- Definir detecção de ambiente e firmware
-   [ ] 09.04 --- Definir transferência de controle
-   [ ] 09.05 --- Integrar gerenciamento de memória
-   [ ] 09.06 --- Integrar diagnóstico de boot
-   [ ] 09.07 --- Validar caminho UEFI

## Fase 10 --- Compatibilidade de firmware

-   [ ] 10.01 --- Validar UEFI nativo
-   [ ] 10.02 --- Avaliar UEFI com CSM
-   [ ] 10.03 --- Projetar estágio Legacy BIOS separado
-   [ ] 10.04 --- Validar disponibilidade de GOP e fallback de saída
-   [ ] 10.05 --- Documentar matriz real de plataformas

## Fase 11 --- Integração completa

-   [ ] 11.01 --- ArmitoBoot inicializa Core
-   [ ] 11.02 --- Core inicializa módulos na ordem definida
-   [ ] 11.03 --- Core coordena o fluxo até Decoder
-   [ ] 11.04 --- Decoder produz Universal IR
-   [ ] 11.05 --- Backend gera código de destino
-   [ ] 11.06 --- Validar comunicação entre componentes
-   [ ] 11.07 --- Testar ciclo completo

## Fase 12 --- Validação e expansão

-   [ ] 12.01 --- Compilação independente e integrada
-   [ ] 12.02 --- Testes unitários
-   [ ] 12.03 --- Testes de dependências e ciclo de vida
-   [ ] 12.04 --- Testes de falhas e recuperação
-   [ ] 12.05 --- Testes semânticos de instruções
-   [ ] 12.06 --- Testes em hardware e firmware declarados
-   [ ] 12.07 --- Documentar limitações conhecidas
-   [ ] 12.08 --- Planejar novos pares de arquiteturas

## Critérios gerais de conclusão

Uma etapa só é concluída quando: 1. a implementação correspondente
existe; 2. o contrato está documentado; 3. compila no ambiente indicado;
4. os testes definidos passam; 5. as limitações restantes estão
registradas.

## Próxima etapa

**05.01 --- Definir a API pública e o contrato de entrada do Decoder
AArch64**, consumindo a IR já definida em `IR/` (ver `Docs/IR.md`).
