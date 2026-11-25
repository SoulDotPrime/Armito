# Compatibility

## 1. Escopo

Este documento separa compatibilidade de arquitetura de CPU de
compatibilidade de firmware e plataforma. São problemas diferentes e
precisam ser validados separadamente.

## 2. Direção inicial de ISA

  Papel                         Arquitetura inicial
  ----------------------------- ------------------------------------------------
  Origem                        ARM64 / AArch64, com foco no ecossistema Apple
  Destino                       x86_64
  Representação intermediária   Universal IR independente de ISA

A direção ARM64 Apple → x86_64 é o objetivo inicial de pesquisa. Isso
não significa que toda instrução ARM64, toda extensão Apple ou qualquer
aplicação Apple já seja suportada.

## 3. Níveis de compatibilidade

### Nível A --- Decodificação

O Decoder reconhece instruções e produz uma representação estruturada.
Isso não basta para executar software.

### Nível B --- Equivalência semântica

As operações da IR preservam os efeitos da instrução de origem,
incluindo largura, flags, memória e fluxo de controle.

### Nível C --- Geração de código

Um backend gera código válido para a arquitetura de destino.

### Nível D --- Ambiente de execução

Convenções de chamada, bibliotecas, chamadas de sistema, exceções e
serviços necessários são tratados.

### Nível E --- Aplicação

Uma aplicação real é executada e validada no ambiente de destino.

Uma declaração de suporte deve indicar explicitamente até qual nível foi
validada.

## 4. UEFI

O caminho UEFI é o caminho prioritário para a integração inicial com
ArmitoBoot.

Requisitos a validar: - arquitetura do binário EFI; - protocolos
disponíveis; - alocação e mapa de memória; - serviços de console e
saída; - GOP quando disponível; - transição correta do ambiente de
boot; - handoff de contexto entre ArmitoBoot e Core.

A presença de UEFI não garante que GOP ou todos os protocolos opcionais
estejam disponíveis.

## 5. UEFI com CSM

CSM é uma camada de compatibilidade Legacy oferecida por determinados
firmwares UEFI. Sua disponibilidade e seus recursos variam por firmware
e configuração.

Armito pode prever caminhos separados para: - UEFI nativo; - UEFI com
CSM habilitado; - Legacy BIOS puro.

O caminho UEFI+CSM deve tratar a possibilidade de GOP ausente e de
serviços UEFI limitados. A implementação não deve presumir que todos os
recursos do caminho UEFI nativo estarão presentes.

## 6. Legacy BIOS puro

Um executável EFI não inicializa diretamente em um ambiente Legacy BIOS
puro. Para suportá-lo, ArmitoBoot precisará de um estágio de entrada
compatível com BIOS, normalmente em código de 16 bits, que prepare o
ambiente e transfira o controle para uma etapa posterior.

Esse caminho exige desenvolvimento e testes próprios. Não deve ser
anunciado como suportado apenas porque o projeto compila para x86_64.

## 7. Compatibilidade de hardware

Compatibilidade com uma ISA não implica compatibilidade com qualquer
hardware que implemente essa ISA. Extensões de CPU, recursos opcionais,
instruções privilegiadas e características de memória precisam ser
detectados e tratados.

## 8. Política para instruções não suportadas

Cada instrução deve ter um dos seguintes resultados documentados: -
traduzida diretamente; - traduzida por sequência equivalente; -
encaminhada a um helper ou mecanismo de emulação; - rejeitada de forma
controlada como não suportada.

Nunca se deve emitir código aproximado sem preservar a semântica.

## 9. Matriz de status

  Área                      Direção          Estado
  ------------------------- ---------------- ------------------------------------------
  ARM64 Apple como origem   Inicial          Planejado / em pesquisa
  x86_64 como destino       Inicial          Planejado / em pesquisa
  Universal IR              Camada central   Em definição
  UEFI                      Boot             Caminho prioritário, integração pendente
  UEFI + CSM                Boot             Possibilidade arquitetural, não validada
  Legacy BIOS puro          Boot             Exige estágio próprio, não implementado
  Outros pares de ISA       Expansão         Futuro

Atualize esta matriz somente com base em testes reproduzíveis.
