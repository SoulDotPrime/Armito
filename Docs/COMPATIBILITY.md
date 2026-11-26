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

ARM64E é uma variante ABI Apple de AArch64 com convenções de Pointer
Authentication. Nesta etapa há reconhecimento estrutural, não
compatibilidade de execução.

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

### 3.1 Estado ARM64E

Em Mach-O, `CPU_TYPE_ARM64` identifica a família; o subtype base 2 é
`CPU_SUBTYPE_ARM64E`. Os bits de capacidade são separados antes de
interpretar o subtype. `CPU_SUBTYPE_ARM64E_X1` (base 12) é distinto e não
é tratado como subtype 2. Os metadados expõem os indicadores de ABI
Pointer Authentication e os campos de versão codificados no subtype:
flag versionado `0x80000000`, flag de ABI kernel `0x40000000` e versão
nos bits `[29:24]` (`0x3F000000`).

O ABI ARM64E usa IA/IB para ponteiros de código e DA/DB para ponteiros de
dados. Em linhas gerais, as chaves A destinam-se a usos globais (por
exemplo, function pointers e vtables) e as chaves B a usos locais (em
particular, endereços de retorno autenticados com IB e SP). A seleção
exposta pelo Armito é uma política explícita para esses usos, não uma
implementação de todos os esquemas de assinatura da ABI. Diversificadores
constantes são de 16 bits; address diversity utiliza o contexto de
armazenamento.

A camada de software separa semântica PAC do algoritmo, por meio de um
provedor substituível. O provedor determinístico incluído nos testes é
somente de compatibilidade e não reproduz nem oferece a segurança
criptográfica do hardware Apple. As operações PAC do Decoder são
preservadas na IR, mas não têm backend x86_64 funcional. O parser Mach-O
localiza cabeçalhos/slices, mas não carrega imagens nem interpreta
relocations autenticadas.

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
  ARM64 Apple como origem   Inicial          Decoder parcial; sem execução completa
  ARM64E                    Subconjunto       Identificação, PAC/IR e testes host; sem backend/runtime
  x86_64 como destino       Inicial          Planejado / em pesquisa
  Universal IR              Camada central   Operações PAC explícitas; tradução PAC pendente
  UEFI                      Boot             Caminho prioritário, integração pendente
  UEFI + CSM                Boot             Possibilidade arquitetural, não validada
  Legacy BIOS puro          Boot             Exige estágio próprio, não implementado
  Outros pares de ISA       Expansão         Futuro

Atualize esta matriz somente com base em testes reproduzíveis.
