# Referência arquitetural para Fase 3.2.3.1 (2026-11-26)

Pacote Arm oficial `ISA_A64_xml_A_profile-2025-12.tar.gz`, SHA256
`845ed227a6692ddb6b602da2ecbbac776620195a9c001ec576ced3a9a53dc26b`:
`add_addsub_ext.xml`, `add_addsub_shift.xml`, `and_log_shift.xml`,
`bfm.xml`, `sbfm.xml`, `ubfm.xml`, `setf.xml`, `smax_imm.xml`,
`smin_imm.xml`, `addg.xml`, `subg.xml` e `autiasppc_imm.xml`.
As regras Decode_UNDEF são independentes do catálogo derivado do AARCHMRS
2024-12; LLVM MC 18.1.3 fornece comparação numérica em encodings válidos.
Nenhuma das 9.176 rejeições **do corpus reproduzido** requer habilitar
extensão adicional: os quatro grupos afetados não exigem feature. A
distribuição por grupo do resumo de retomada difere da deste corpus e ainda
requer reconciliação com os artefatos originais. SETF depende de `FEAT_FlagM`,
já habilitado pelo probe e LLVM, e seu `mask=1101` não é impresso.

# Referências da Fase 3.2.3 (2026-11-26)

LLVM MC 18.1.3, aarch64 e features
`+crc,+pauth,+mte,+cssc,+flagm,+pauth-lr,+cpa` são o oráculo para valores
immediate impressos. Aliases bitfield têm parâmetros derivados de `immr/imms`
comparados numericamente; aliases MOVZ/MOVN imprimem constantes efetivas.
LLVM-invalid não conta como comparação positiva; campos implícitos SETF não
são inferidos da ausência de operandos na saída do LLVM. O catálogo AArchMRS
é a fonte das posições, larguras e restrições de codificação; comparação
independente de efeitos implícitos permanece pendente.

# Referências da Fase 3.2.2 (2026-11-26)

LLVM MC Ubuntu 18.1.3, `--triple=aarch64`, features
`+crc,+pauth,+mte,+cssc,+flagm,+pauth-lr,+cpa`; a versão é verificada pelo
driver. AArchMRS v9Ap6-A, schema 2.5.3, build 406, commit
66d0604c6b800745edfbd4891ba450add54ede25 fornece formas/guardas.
Somente registradores **explícitos** são comparados ao LLVM; as codificações
rejeitadas por LLVM (880 casos) não entram no denominador das verificadas.
Campos implícitos fixos PAuth e Ra SMULH/UMULH têm classificação arquitetural
separada; não são inferidos de operandos que LLVM não imprime.

# Referências da auditoria Integer (histórico anterior, 2026-11-26)

LLVM MC Ubuntu 18.1.3 (AArch64, `+crc,+pauth,+mte,+cssc,+flagm,+pauth-lr`),
Python 3.12.3 e catálogo Arm AARCHMRS v9Ap6-A, schema 2.5.3, build 406,
commit 66d0604c6b800745edfbd4891ba450add54ede25 (SHA-256 da fonte
2234d548ad50a95c3119166fad7077f333c4f9fe4cf87a274ed80abc066054b2).
O driver recusa versões de LLVM diferentes. LLVM não reconhece algumas
instruções deste perfil; ausência de comparação não equivale a sucesso.

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

### 3.2 Cobertura do Decoder e catálogo AArch64

O catálogo usa o Arm AARCHMRS A-profile v9Ap6-A de 2024-12 como fonte
estruturada: 2.095 IDs de operação distintos referenciados, 4.296 formas,
288 aliases e 3.099 padrões únicos de máscara/valor. As métricas contam
entidades diferentes; um padrão pode representar vários forms, e form
catalogado não significa suportado. Requisitos, conditions e campos
herdados da árvore de instruções são mantidos. Os metadados e tabelas C
geradas estão cobertos por `Decoder/AARCH64/Catalog/NOTICE.txt`.

O caminho de produção usa tabelas C estáticas e índice pelos bits [31:26]:
64 buckets, 3.883 referências, média observada de 90,87 candidatos e
máximo de 335. As restrições fixas `should_be_mask` e guards AST compilados
participam da seleção; os 177 overlaps potenciais são ordenados pela
especificidade e empates são rejeitados. O perfil de capabilities é
explícito para a CPU guest; não consulta a CPU hospedeira. 105 features
podem ser representadas. As 599 conditions são avaliáveis, incluindo
`BFXPreferred`, `MoveWidePreferred` e `SVEMoveMaskPreferred` para aliases.
Uma condição necessária não avaliável resulta em
`PATTERN_MATCHED_UNVALIDATED`, nunca em suporte presumido.

Nos testes host, cada padrão teve amostras positivas e negativas e as
variações determinísticas foram comparadas com busca linear: 59.103
lookups, 104.702 verificações, zero divergências, 0,808 s no build forçado
(0,782 s no incremental).
O armazenamento estático medido é 2.291.506 bytes. A comparação
diferencial com LLVM MC cobre 33 codificações de aritmética, memória,
controle de fluxo, PAC e LSE; aliases textuais são normalizados (por
exemplo, MOVN é exibido como MOV pelo LLVM).

Nas amostras mínimas, o classificador reconheceu 93/3.099 padrões (105
formas), reportou 3.000 como não suportados e 6 como inválidos/reservados.
O decode extraiu operandos de 92 amostras, reportou 3.000 não suportadas e
rejeitou 7 como inválidas. A camada universal tem descritores para
4.296/4.296 formas (45.360 descritores) e extraiu 3.093/3.099 amostras de
padrões, alcançando amostras de 3.026 formas e 1.478/2.095 IDs de
operação, incluindo resultados parciais/raw. Essas métricas não provam
validade arquitetural para todas as combinações dos campos, extração
semântica integral, equivalência de operandos, tradução ou backend. Core inicializa as tabelas e registra o
extrator sem decodificar instruções no boot. Os builds EDK2 DEBUG/RELEASE
ligam catálogo, descritores e extrator em EFIs de 2.498.560 bytes;
isso não substitui execução em firmware.

O catálogo cobre extensões A-profile descritas pelo AARCHMRS escolhido,
incluindo famílias A64 SIMD/FP/crypto, SVE e SME. Não é uma fonte de
instruções proprietárias não publicadas da Apple. A ABI Apple ARM64E e
seus metadados permanecem fora da contagem de instruções do catálogo.

### 3.3 Extração de operandos

Fase 3.2 Integer: 195/195 formas atingem COMPLETE com um testemunho por
forma, 112 operações inventariadas; seis palavras têm comparação LLVM MC
de registradores e modificadores. Esse valor é cobertura de amostras do
extrator, não certificação de operandos de todas as combinações válidas ou
de aliases implícitos. O nível A integral para Integer ainda não foi
demonstrado; demais famílias permanecem fora deste incremento.
As máscaras de bitfield derivadas do encoding estão disponíveis, mas a
comparação diferencial de bitfields com operandos e aliases LLVM ainda
não cobre todas as combinações de campos.

A API C de operandos tipados separa valor bruto, interpretado, parcial e
não suportado, e é acessível pelo serviço registrado no Decoder. Ela
implementa subconjuntos de GPR, SP/zero, immediates, bitmasks, branches,
endereços PC-relative, memória/pairs, system registers e metadados PAC.
SIMD/NEON, SVE, SME, lanes, listas e alguns modos de endereçamento
continuam parciais. O extrator não traduz instruções para IR nem atesta
compatibilidade de execução. Os 33 casos LLVM MC atuais não comparam todos
os operandos.

Medição por forma em 2026-11-26: 4.288/4.296 formas alcançadas no limite
de 96 candidatos determinísticos por forma, 0 completas, 4.255 parciais,
33 raw, 0 não suportadas e 8 não alcançadas. Assim, nem as 3.093 amostras
de padrões nem as 4.288 formas alcançadas demonstram nível A integral. A
comparação LLVM MC continua limitada a mnemonics de 33 instruções; não há
comparação independente de operandos efetivos para 4.296 formas. Os builds
EDK2 DEBUG e RELEASE foram repetidos, com tabelas e extrator no link, não
execução UEFI em hardware. O dump M2 não foi localizado no workspace Armito;
nenhum conteúdo de firmware foi incluído ou publicado.

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
  ARM64 Apple como origem   Inicial          Decoder parcial; 4.296 formas descritas; extração ainda parcial
  ARM64E                    Subconjunto       Identificação, PAC/IR e testes host; sem backend/runtime
  x86_64 como destino       Inicial          Planejado / em pesquisa
  Universal IR              Camada central   Operações PAC explícitas; tradução PAC pendente
  UEFI                      Boot             Tabelas/extrator ligados; builds X64 DEBUG/RELEASE; firmware real não validado
  UEFI + CSM                Boot             Possibilidade arquitetural, não validada
  Legacy BIOS puro          Boot             Exige estágio próprio, não implementado
  Outros pares de ISA       Expansão         Futuro

Atualize esta matriz somente com base em testes reproduzíveis.
