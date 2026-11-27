# Fase 3.3.3 — controle de fluxo fechado (2026-11-26)

76/76 formas com extração COMPLETE e 53 não-alvo formalmente encaminhadas
à Fase 3.6 (31 hints/7 barreiras/4 PSTATE/11 sistema). Relatório novo:
`control_flow_comprehensive_validation.csv`, 634 linhas, SHA-256
`5a96fa23…`. Reconciliação: 23.385 casos históricos de 3.3 (22.216
positivos, 1.113 rejeições pela forma, 54 por feature e 2 prioridades
de forma irmã), 1.336 positivos LLVM; mais 412 rebases PC e 4 perfis
PAuth_LR sem PAuth. Cobertura registrada: 32 operações, 28 nomes de
operandos, 11 dependências, 11 restrições e 5 features. O runtime agora
marca dependência PC de diretos/BLR;
BR/RET continuam sem dependência de PC para alvo. Os 13 CSVs anteriores,
290.770 ocorrências, 3.099/4.296/45.360 e 506 REJECT permanecem.
Testes sequenciais e builds EDK2 DEBUG/RELEASE EXIT 0. El atual,
Apple ARM64E proprietário e execução semântica continuam fora do escopo;
3.4 e 3.6 não iniciadas.

# Fase 3.3.2 — autenticação e exceções extraídas (2026-11-26)

27/27 formas autenticadas/excepcionais COMPLETE, sem divergência nas
campanhas. `authenticated_control_flow_validation.csv` tem 1.142 linhas
(1.088 casos + 53 formas futuras + cabeçalho); SHA-256 `acebf7ec…`.
539 casos LLVM positivos, 530 flips fixos rejeitados pela forma, 17
features rejeitadas e 2 sobreposições de forma irmã válidas. Dependências
arquiteturais explícitas no `AARCH64OperandSet` incluem IA/IB, contexto
zero/registrador/SP, link/PC, LR, PSTATE, ELR/SPSR e registradores de debug.
As 53 formas de hints/barreiras/PSTATE/sistema foram classificadas para a
futura 3.3.3 (não iniciada). Os 12 hashes e 289.682 ocorrências anteriores
foram preservados; testes e EDK2 X64 GCC DEBUG/RELEASE EXIT 0. Semântica
PAC/exception não executada; Apple ARM64E proprietário não validado.

# Fase 3.3.1 — controle de fluxo convencional validado (2026-11-26)

129 formas de controle auditadas: 49 convencionais com evidência positiva de
COMPLETE e 80 não convencionais classificadas, sem alegar suporte. O runtime C
expõe endereço do alvo, tipo direto/indireto/condicional/retorno, condição,
fallthrough (PC+4), chamada/escrita de LR, leitura de LR em RET X30, leitura
de NZCV em B.cond/BC.cond, largura 8/16/32/64 de comparação e bit_index.
Campos estruturais RAW são seletores; `Rm` de BR/BLR/RET não é registrador
arquitetural. `imm6` de FEAT_CMPBR compara; `imm9` desloca o PC. Auditoria
`control_flow_validation.py`: 22.297 casos (21.677 positivos, 620 negativos),
797 confrontos LLVM MC 21.1.8, 0 divergências, CSV SHA-256 `edddbdea…`.
Os 11 relatórios 3.2 e 267.385 ocorrências históricas permanecem; 3.099/
4.296/45.360 e 506 REJECT preservados. Testes sequenciais e EDK2 DEBUG/
RELEASE EXIT 0; nenhuma execução semântica e 3.3.2 não iniciada.

# Fase 3.2.8 — validação Integer abrangente e finalização (2026-11-26)

Consolidação e auditoria de 3.2.0–3.2.7. 195/195 formas Integer/Data Processing
com extração COMPLETE demonstrada por evidência positiva (semente distintiva por
forma, perfil completo de features, referência LLVM MC qualificada); 93/93 aliases
validados (92 selecionados + REV64 negativo-only); 112 operações. Novo oráculo
`integer_comprehensive_validation.py` + `integer_comprehensive_validation.csv`
(289 linhas = 195 formas + 93 aliases + cabeçalho; SHA-256 `3f24c847…`), com
cobertura por forma/operação/alias/operando/dependência/restrição/feature, ligado
ao `operand-extraction-test`.

As 38 formas `runtime-partial` da 3.2.7 foram ELIMINADAS: a causa raiz não era
rebaixamento COMPLETE→PARTIAL do runtime, e sim a semente ambígua do oráculo
`integer_invalid_encodings.py`, que usava o `value` bruto do padrão. Os bits
discriminadores de opcode ([11:10]) ficam como wildcard na máscara, então a
semente colidia com a forma irmã (ASRV/LSLV/LSRV/RORV = `1AC02000`/`FFE0F000`;
SDIV/UDIV = `1AC00800`/`FFE0F800`). O runtime já resolve corretamente cada forma
(LSLV `1AC02000`, LSRV `1AC02400`, ASRV `1AC02800`, RORV `1AC02C00`, UDIV
`1AC00800`, SDIV `1AC00C00`) com COMPLETE. Correção: o oráculo deriva uma semente
distintiva por forma (`sample_words` + probe). `integer_invalid_encodings` agora
3.173 casos, 0 runtime-partial, 0 undue-accept; SHA-256 `550ba37f…` (alteração
justificada e versionada; os outros 9 hashes preservados).

Campos estruturais de opcode (kind=RAW: sf/op/opc/op0-3/S/N) diferenciados dos
operandos arquiteturais: os RAW arquiteturais (nzcv/mask/hw/seletores) já são
promovidos a INTERPRETED; os RAW restantes são estruturais e não rebaixam COMPLETE.
Nenhuma alteração C foi necessária (experimento revertido para preservar
baselines). 3.099 padrões/4.296 formas/45.360 descritores e 267.385 ocorrências
históricas preservados; 506 REJECT mantidos. `make -B test`/`make test`/
`make catalog-test` EXIT 0 (LLVM 18.1.8 qualificada); EDK2 X64 GCC DEBUG/RELEASE
EXIT 0. Semântica em execução adiada; 3.3 não iniciada.

# Fase 3.2.7 — codificações classificadas (2026-11-26)

`integer_invalid_encodings.py`: 3.173 casos, 195 formas, 0 undue-accept,
38 runtime-partial (válidas, limitação do runtime), `integer_invalid_encodings.csv`
SHA-256 `977a6320…`. Classifica undefined (bits fixos), runtime-partial, feature-
gated (CSSC/MTE/CPA/PAuth/PAuth_LR/CRC32/FlagM). ASRV/AUTIASPPC investigados:
válidos, rejeitados por PARTIAL (campos RAW), não inválidos. LLVM complementar.
9 hashes e 264.212 ocorrências preservados; 3.099/4.296/45.360. `make -B test`/
`make test`/`make catalog-test` EXIT 0; EDK2 DEBUG/RELEASE EXIT 0. 506 REJECT
mantidos; semântica adiada; 3.2.8 não iniciada.

# Fase 3.2.6 — operandos implícitos classificados (2026-11-26)

`integer_implicit_operands.py` classifica as 195 formas Integer: 1.800 casos,
195 formas, 0 divergências, `integer_implicit_operands.csv` SHA-256 `6df6ade8…`.
Cobre NZCV (CCMP/CCMN, ADDS/SUBS/ANDS/BICS/ADCS/SBCS/SETF/RMIF), carry
(ADC/SBC/ADCS/SBCS), condicionais (CSEL/CSINC/CSINV/CSNEG/CCMP/CCMN), SP/ZR/
WSP/XZR, leituras implícitas de destino (MOVK, BFM), larguras e parciais.
Runtime C não alterado (flag IMPLICIT_REGISTER reservada ao contexto SP; contrato
de leitura é metadado do oráculo). 8 hashes e 262.412 ocorrências preservados;
3.099/4.296/45.360. `make -B test`/`make test`/`make catalog-test` EXIT 0; EDK2
DEBUG/RELEASE EXIT 0. Semântica em execução adiada; 506 REJECT mantidos; 3.2.7
não iniciada.

# Fase 3.2.5.2 — toolchain qualificada; 3.2.5 aprovada (2026-11-26)

Gates literais `18.1.3` removidos e substituídos por política explícita em
`llvm_toolchain.py` (`LLVM_MC` seleciona o executável; `QUALIFIED` = 18.1.3
histórico, 18.1.8, 21.1.8; `require_qualified` rejeita o resto com diagnóstico).
Qualificação empírica por reprodução de TODAS as baselines (não um único corpus):
18.1.8 e 21.1.8 com `operand-extraction-test` EXIT 0. `make -B test`, `make test`
e `make catalog-test` com EXIT 0 (padrão 21.1.8). EDK2 X64 GCC DEBUG/RELEASE
EXIT 0 (2.502.656 bytes). Preservados 93 aliases (68/25), 48.005 pares, 142.731
linhas, 262.412 ocorrências, 3.099/4.296/45.360 e os 8 hashes. Runtime C intacto;
semântica em execução adiada; 506 REJECT mantidos; 3.2.6 não iniciada.

# Fase 3.2.5.1 — predicados classificados; encerramento bloqueado (2026-11-26)

Retomada sem repetição. Inventário mantido: 195 formas, 93 aliases em 66 formas,
48.005 pares, 142.731 linhas (94.854 verificadas, 47.877 rejeitadas). Classificação
independente dos predicados: 68 variáveis (ambas as polaridades presentes),
25 constantes-verdadeiros (só positivo, correto), 0 constantes-falsos; REV64
`preferred=false` nunca selecionado. Asserção adicionada a
`integer_alias_predicates.py` (CSV inalterado, SHA-256 `e9f412f7…`). Novo
relatório `integer_alias_coverage.csv` (93 linhas, SHA-256 `05e1d7d7…`) e
`integer_alias_coverage.py` auto-verificável, ligado ao `operand-extraction-test`.
106 sobreposições LSL/UBFIZ e 170 diferenças BFC/BFI confirmadas como preferência
textual da mesma operação UBFM/BFM, sem divergência arquitetural. Hashes das
214.407 ocorrências anteriores preservados. `make catalog-test` passou;
`make -B test`/`make test` falham só no gate de versão (18.1.3 exigido; há 21.1.8
e 18.1.8, que reproduzem os hashes). EDK2 X64 GCC DEBUG/RELEASE passaram
(2.502.656 bytes). 3.2.5.1 segue aberta por indisponibilidade de LLVM 18.1.3 e
por depender de execução semântica futura; não iniciar 3.2.6.

# Fase 3.2.5.1 — cobertura de Decoder parcial (2026-11-26)

93 aliases inventariados individualmente; 48.005 pares forma/codificação em
66 formas, 142.731 linhas CSV de predicado/prioridade. Os 92 aliases elegíveis
foram observados como seleção; REV64 tem `preferred=false` no catálogo Arm.
106 sobreposições LSL/UBFIZ (36 de 32 bits, 70 de 64 bits) exigem distinção
entre preferência textual e operação UBFM. O oráculo concorda com as escolhas
do probe na amostra; LLVM imprime LSL nesses 106 casos, sem diferença de
codificação. Evidência não é prova de equivalência semântica executada;
predicados constantes não têm caso negativo válido e alguns predicados
`true` dependem de rejeição da forma, não do alias. 3.2.5.1 segue aberta.
6.656 condicionais e as baselines anteriores permanecem imutáveis.

# Fase 3.2.5 — em andamento (2026-11-26)

Inventário: 195 formas Integer/Data Processing; 93 aliases em 66 formas;
16 formas condicionais. O oráculo independente cobre 6.656 codificações
condsel/condcmp, todas as 16 condições em ambas as larguras, NZCV de 0 a 15
para CCMP/CCMN, igualdade e desigualdade dos registradores, AL/NV e aliases
CSET/CSETM/CINC/CINV/CNEG. Compara campos e escolhas de alias com o probe
e texto aplicável com LLVM MC 18.1.3. Ainda não valida todos os 93 aliases,
prioridades ou equivalência semântica; fase NÃO encerrada. Baselines 3.2.2,
3.2.3 e 3.2.4 continuam preservadas. Lacuna histórica ainda aberta.

# Fase 3.2.4 — máscaras auditadas na baseline atual (2026-11-26)

As 14 formas AND/ORR/EOR/ANDS/SBFM/UBFM/BFM foram verificadas nas duas
larguras aplicáveis, inclusive N alternativo, 64 valores de immr e 64 de
imms: 114.688 verificações individuais. Um oráculo Python independente
calcula len, levels, S, R, diff, esize, welem, telem, wmask e tmask por bits;
os valores do extrator C e os parâmetros dos aliases LLVM 18.1.3 concordam.
Dos 1.016 registros transferidos, 960 são válidos e os 56 REJECT são
inválidos (40 S==levels e 16 len<1). A 3.2.3 e seus CSVs não foram alterados;
a divergência de proveniência do corpus histórico permanece documentada.

# Fase 3.2.3 — fechamento restrito à baseline atual (2026-11-26)

Baseline oficial: commit `86cd1bb` introduziu o CSV completo de 62.851 pares
únicos (forma, codificação). A auditoria atual compara a geração inteira com
esse arquivo **byte a byte**, sem opção de sobrescrita; qualquer alteração
da ordem, codificação, categoria, razão ou texto LLVM falha. Partição completa
e mutuamente exclusiva: 52.209 `llvm-verified`, 9.624
`architecturally-invalid`, 960 `deferred-bitmask`, 56
`deferred-bitmask-reject`, dois `implicit-setf`. O relatório reduzido contém
as 9.624 regras arquiteturais, os 56 casos transferidos e os dois SETF;
as 52.209 verificações positivas permanecem no CSV completo e no diferencial
LLVM por forma. `DecodeBitMasks` (todos os 1.016 casos) é responsabilidade
exclusiva da 3.2.4, não executada nesta auditoria. Fase 3.2.3 encerrada **só
para esta baseline e somente para immediates aplicáveis**. A fase 3.2 geral
permanece aberta; a lacuna do corpus de 506 não foi resolvida por suposição.

# Fase 3.2.3.1 — imediatos, auditoria do corpus reproduzido (registro anterior)

Os 62.851 pedidos estão registrados individualmente em
`Tests/Decoder/AARCH64/integer_immediate_all_candidates.csv` e são
particionados sem sobreposição: 52.209 comparações
positivas LLVM, 9.624 inválidos arquiteturais, 960 bitmasks aceitos adiados, 56
bitmasks rejeitados e adiados (sem diagnóstico da validade DecodeBitMasks) e 2 `SETF` com
`mask=1101` fixo e implícito. Neste corpus há 504 `REJECT` (448 bitfield
inválidos + 56 bitmasks ainda sem classificação); 2 SETF são resultados
`COMPLETE`, não rejeições. O resumo da retomada relatava 506 `REJECT` em
outra distribuição (459 bitfield + 47 addsub_shift), e essa discrepância
**não está resolvida** porque os logs originais não estão disponíveis.
Os 9.176 aceitos pelo probe anterior reproduzido e rejeitados pelo LLVM estão incluídos
nos 9.624 inválidos (os 448 restantes já eram rejeitados pelo probe).
Arquivos CSV preservam cada codificação. A cobertura de registradores permanece
30.212 pedidos, 26.653 positivos em 195/195 formas e 880 inválidos nesse
corpus, dos quais 15 pares (forma, codificação) coincidem com o corpus immediate.
As 67 regressões históricas de registradores permanecem. A pendência de
conciliação do corpus anterior não altera o fechamento **limitado** acima.

# Fase 3.2.3 — immediates Integer (2026-11-26)

O probe agora expõe também `raw_value`, `signed_value` e `scale`. O diferencial
`integer_immediate_differential.py` compara números decimais/hexadecimais como
inteiros e verifica os parâmetros de aliases bitfield sem comparar apenas
texto. Preserva integralmente a regressão de registradores de 30.212 casos.
`SETF8/SETF16` possuem campo `mask` fixo e implícito: seu valor bruto é testado,
mas não é tratado como immediate explícito impresso pelo LLVM.
Máscaras de logical-immediate são deixadas à Fase 3.2.4.
Inventário: 89 formas / 118 campos, 62.851 candidatos; 52.209 comparações
positivas, 9.176 rejeições LLVM, 960 máscaras lógicas adiadas e 2 máscaras
SETF implícitas. Nenhum campo fora dessas exceções ficou sem comparação
positiva; a auditoria arquitetural das rejeições foi concluída em 3.2.3.1.

# Fase 3.2.2 — registradores Integer (2026-11-26)

Auditoria do probe e do diferencial: as 67 diferenças iniciais estão
registradas individualmente com codificação, forma, operação, Armito e LLVM em
`Tests/Decoder/AARCH64/integer_register_baseline.csv` (52 de tratamento do
registrador especial por campo, 13 de Rd lógico-imediato SP/WSP e 2 de largura
impressa pelo alias bitfield). As regressões reexecutam os 67 testemunhos.
O extrator marca explicitamente registradores implícitos em formas com campos
fixos PAuth/XPAC, SMULH/UMULH e DP immediate PAuth. O diferencial compara
registradores explícitos dos aliases após verificar seus pré-requisitos de
codificação e conserva os operandos canônicos para avaliação separada.

LLVM MC Ubuntu 18.1.3 (`+cpa` incluído) fornece ao menos uma comparação
positiva para cada uma das 195 formas. Do corpus original de 14.116, a suite
chega a 30.212 candidatos únicos (32 valores de cada campo GPR livre e pares
0/15/30/31): 27.533 aceitos pelo catálogo/COMPLETE, 26.653 comparados com
LLVM, 880 rejeitados pelo LLVM; zero divergências de registrador não
justificadas e zero campos GPR livres sem os 32 valores verificados.
`make -B test`, `make test`, `make catalog-test` (sequenciais) e EDK2 GCC X64
DEBUG/RELEASE passaram. A Fase 3.2 **integral** segue aberta: este marco
não valida immediates, condições, bitmasks ou efeitos implícitos.

# Auditoria Integer independente (histórico anterior, 2026-11-26)

O novo diferencial usa LLVM MC 18.1.3 e gera 14.116 candidatos determinísticos
para as 195 formas / 112 operações. 11.437 codificações passaram pelo catálogo
e obtiveram COMPLETE. A comparação atual é **apenas de registradores explícitos**;
167 formas possuem pelo menos uma comparação positiva, 28 não possuem.
Persistem 67 divergências de registrador (incluindo casos SP/zero em MTE/PAuth
e a impressão LLVM de logical immediates e aliases). A nova suíte falha em
tais divergências: `make -B test` e `make test` falharam; `make catalog-test`
passou. EDK2 X64 GCC DEBUG e RELEASE passaram; a cópia opcional de PDB foi
ignorada pelo make do EDK2. Não há atestado de conclusão da Fase 3.2.

# Development Status

**Última consolidação documental:** 2026-11-26\
**Foco atual:** consolidação do Core e extração universal de operandos AArch64\
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
pre-index com writeback permanecem não reconhecidas. A suíte Decoder passou
224 verificações host; ARM64E passou 514 verificações e CPU State 58.
Os testes host estão organizados em `Tests/Core/`, `Tests/IR/`,
`Tests/Decoder/` e `Tests/CPU/AARCH64/`; os fixtures ficam em
`Tests/Fixtures/AARCH64/`.
`Tests/` deixou de ser ignorado para permitir versionar os testes; as regras
de `Docs/FIRMWARE/` e `Docs/AUDI/` permanecem. A execução em firmware UEFI
real não foi testada.

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

## 5.3 Expansão do Decoder AArch64 — Fase 1

O inventário centralizado está em
`Decoder/AARCH64/Catalog/AARCH64InstructionCatalog.json`. Ele foi derivado
do Arm AARCHMRS A-profile v9Ap6-A (2024-12, schema 2.5.3, build 406), com
commit e SHA-256 fixados no catálogo. O arquivo
`Decoder/AARCH64/Catalog/NOTICE.txt` preserva os termos BSD-3-Clause.

As métricas não são intercambiáveis: há 2.095 IDs de operação distintos
referenciados por formas, 4.296 formas de instrução, 288 registros formais
de alias e 3.099 padrões únicos de largura/máscara/valor. A fonte também
tem 2.262 definições de operação, incluindo 167 OperationAlias, e 1.503
mnemonics distintos. Os padrões incluem a composição de codificações
herdadas dos grupos MRA. São padrões com campos variáveis, não palavras de
máquina concretas nem contagem de instruções implementadas.

A tabela registra máscara, valor, especificidade/prioridade, campos,
família, condições e identificadores de extensões. Foram detectados 177
pares de padrões com sobreposição potencial e zero pares com prioridade
igual. Sobreposição de bits é apenas um candidato a conflito: condições
semânticas e aliases ainda precisam ser avaliados pelo decoder modular.
Empates futuros devem resultar em ambiguidade explícita, nunca numa escolha
silenciosa.

Na validação host original da Fase 1, uma amostra de bits fixos por padrão
foi testada, preenchendo bits variáveis com zero. O classificador reconheceu
93/3.099 amostras como conhecidas (105 formas) e 3.006 como não suportadas.
Entre as conhecidas, `AARCH64Decode` extraiu operandos em 92 e rejeitou uma
como inválida. Essa medição histórica não prova cobertura integral de
qualquer forma, seus operandos, extensões ou semântica. A integração e
validação posteriores do catálogo no EDK2 estão registradas na Seção 5.4.

As famílias de origem contêm: SVE 1.303 formas, SME 833, load/store 996,
SIMD/FP/crypto 839, data processing 195, control 129 e reserved 1. As
famílias e módulos propostos são metadados de organização, não
implementações. AARCHMRS não descreve extensões proprietárias privadas da
Apple; ARM64E/PAC arquitetural e convenções ABI Apple continuam sendo
categorias distintas.

Na execução original da Fase 1, `make -B decoder-test cpu-state-test
arm64e-test catalog-test` passou após recompilação: Decoder 224/0, CPU State
58/0, ARM64E 514/0 e inventário estrutural/cobertura host aprovados. A
comparação `catalog_tool.py check` também passou usando a fonte pinada
externa. Naquele estado, `make test` não pôde iniciar porque
`Tests/Core/CoreTest.c` e `Tests/IR/IRTest.c` estavam ausentes; os arquivos
foram restaurados depois, e a suíte completa passou conforme Seção 5.4. Para
repetir a comparação da fonte, use
`python3 Decoder/AARCH64/Catalog/catalog_tool.py check --source <Instructions.json>`.

Na Fase 1 o build EDK2 não foi reexecutado porque INF/DSC não haviam mudado
e as fixtures eram host-only. A validação EDK2 da Fase 2 está registrada na
Seção 5.4.

## 5.4 Expansão do Decoder AArch64 — Fase 2

`Decoder/AARCH64/Catalog/AARCH64Catalog.c` fornece a busca em tabelas C
estáticas geradas de `AARCH64InstructionCatalog.json`. A tabela gerada
contém 3.099 padrões, 4.296 formas, 761 grupos, 25.666 descritores locais
de campo e 3.883 referências no índice de 64 buckets, chaveado pelos bits
[31:26]. As tabelas ocupam 2.291.506 bytes segundo `sizeof` no host; o
arquivo de dados C gerado ocupa 5.841.970 bytes. O gerador continua
validando commit, SHA-256 e licença fixados; JSON/Python são usados apenas
na geração/validação, nunca na decodificação.

A busca usa prioridade por número de bits fixos, verifica
`should_be_mask`, avalia guards compilados de condições AARCHMRS e continua
examinando candidatos quando um padrão falha. O gerador emite bytecode e
tabelas C constantes; a decodificação não interpreta JSON, strings de
expressão ou memória dinâmica. O perfil de capabilities da CPU convidada
é explícito e separado da CPU x86_64 hospedeira. ARM64E é representado
como ABI separada e não ativa extensões implicitamente. Os resultados
distinguem correspondência bruta, codificação válida, reservada, inválida,
extensão indisponível e `PATTERN_MATCHED_UNVALIDATED`. Este último é
mantido quando uma expressão necessária não pode ser avaliada; validar
uma codificação não declara suporte semântico nem tradução.

O `AARCH64Classify` consulta o catálogo para formas fora do decoder
semântico e retorna `CATALOG_ONLY`/não suportada. `AARCH64Decode`, extração
completa de operandos, tradução para IR e execução não foram ampliadas para
as 4.296 formas. O Core inicializa e encerra o Decoder no seu ciclo de
vida; `ArmitoBoot` inicializa/inicia o Core, mas não injeta bytes nem
decodifica uma instrução durante o boot. A execução UEFI real ainda não
foi testada.

Validação:

- `make -B test` e `make test`: Core 78/0, IR 172/0, Decoder 226/0, CPU
  State 58/0 e ARM64E 514/0; `catalog-test` também passou.
- `make test`: repetido sem rebuild forçado e passou com os mesmos números.
- Índice host: 59.103 lookups, 104.702 verificações, zero divergências
  frente ao matcher linear; máximo de 335 candidatos e média de 90,87 por
  lookup; 0,808 s no build forçado e 0,782 s no incremental. As 3.099
  entradas tiveram amostra positiva e amostra negativa de bit fixo; a
  geração registra 599 condições, zero condições/guards não suportados e
  105 features do guest. `BFXPreferred`, `MoveWidePreferred` e
  `SVEMoveMaskPreferred` são avaliadas para aliases.
- Diferencial LLVM MC: 33 codificações, incluindo aritmética, memória,
  controle de fluxo, PAC e LSE; aliases textuais são normalizados.
- EDK2 X64 DEBUG e RELEASE com toolchain GCC: builds e links concluídos
  sem erros de compilação; ambos geraram `Armito.efi` PE32+ de 2.498.560
  bytes. As seções do EFI incluem o catálogo e os descritores estáticos;
  os símbolos do DEBUG incluem `AARCH64CatalogPatterns`,
  `AARCH64CatalogOperandDescriptors` e `AARCH64OperandsExtractWithProfile`.
  O make do EDK2 ignora a ausência do arquivo opcional `*.pdb`. A execução
  do catálogo em firmware UEFI real não foi testada.

As referências de pacote do DSC/INF foram ajustadas ao workspace EDK2
(`/home/gui/edk2`, pacote `ArmitoPkg`). O primeiro comando com tag
`GCC5` e tentativas com caminhos de workspace incompatíveis falharam antes
da compilação; os builds finais com `GCC` passaram. Compilação e link não
demonstram execução UEFI real.

## 5.5 Extração universal de operandos — Fase 3

Na Fase 3.2, o inventário Integer/Data Processing contém 44 formas DP
immediate e 151 DP register, 112 IDs de operação. Um testemunho positivo
por forma resultou em 195 COMPLETE, zero parcial/raw/não suportada e zero
não alcançada. COMPLETE exige status arquitetural válido, todos os campos
de operandos interpretados e apenas seletores de codificação marcados como
tais; não implica equivalência independente de todos os valores possíveis.
Seis palavras foram diferenciadas contra LLVM MC para registradores e
modificadores; a comparação exaustiva de operandos, aliases e restrições
por forma continua pendente. A conclusão da Fase 3.2 não foi atestada.
O extrator também expõe máscaras de escrita/topo derivadas de DecodeBitMasks
para bitfields; há testes de valores em cenários de extração e inserção.

`AARCH64Operands` define operandos tipados, estados de extração
interpretado/parcial/raw/não suportado e um extrator C estático baseado nos
descritores AARCHMRS. A API fica separada da tradução para IR e pode ser
obtida pelo Decoder após sua inicialização; Core registra o serviço sem
decodificar uma instrução no boot. A inicialização valida a integridade
estrutural das referências às tabelas C, sem JSON, Python, alocação
dinâmica ou dependência de libc no runtime.

A cobertura estrutural é 4.296/4.296 formas e 45.360 descritores. Nos
fixtures, 3.093/3.099 amostras representativas de padrões foram extraídas;
as amostras aceitas (inclusive parciais/raw) alcançam 3.026 formas e
1.478/2.095 IDs de operação.
Os casos implementados abrangem subconjuntos de GPR, SP/zero, immediates
simples/compostos, logical immediates, MOV-wide, branches/endereço relativo
ao PC, offsets de memória, pares, registradores de sistema, metadados PAC
e seleção dos 288 aliases com as três condições preferenciais. Os 6 casos
restantes não são contabilizados como extraídos; esse total não equivale a
cobertura por forma ou a semântica completa.

SIMD/NEON, SVE, SME, lanes, listas de registradores, algumas combinações
de endereçamento e variações de operandos de sistema continuam parciais ou
não suportadas. O diferencial LLVM MC ainda compara 33 codificações e
mnemonics, não todos os operandos. Não houve alteração de backend ou
declaração de execução semântica.

Auditoria adicional (2026-11-26): 4.288/4.296 formas alcançadas com até 96
candidatos determinísticos por forma, incluindo 1.262 que não eram alcançadas
pelas amostras anteriores de padrão. Das 4.288, 0 são completas, 4.255
parciais, 33 raw e 0 não suportadas; 8 permanecem não alcançadas. A busca
por forma não é uma prova de cobertura de todos os valores de campos. Os
seis padrões sem extração são cinco codificações inválidas e uma reservada;
ver FAILS.md. Foram adicionados offsets assinados imm9 para pre/post-index
e flags de writeback/post-index para esses modos e os pares. A Fase 3 não
atende ao critério de conclusão; diferencial de operandos ainda pendente.

## 6. Integração com ArmitoBoot

O ponto de entrada chama `ArmitoCoreInitialize()` e
`ArmitoCoreStart()`. O Core inicializa o Decoder com ISA AArch64 e sem
configurar capabilities presumidas; no encerramento, o Decoder volta ao
estado não inicializado. Boot não decodifica instruções. Ainda dependem
de implementação e teste: contexto/handoff de boot, diagnóstico,
gerenciamento de memória, execução de firmware e caminho vertical de
tradução.

## 7. Critérios para atualizar este arquivo

Use estes estados: - **Planejado:** definido, ainda sem implementação. -
**Em desenvolvimento:** código em construção. - **Implementado:** código
existe, sem implicar validação. - **Integrado:** conectado aos
componentes necessários. - **Validado:** compilado e testado no ambiente
indicado. - **Concluído:** critérios de aceitação documentados foram
cumpridos.

Atualize a data e os estados após cada marco confirmado. Não converter
"implementado" em "validado" sem evidência de teste.
# Fase 3.4.1 — Memory/Load/Store convencional (2026-11-26)

91/91 formas convencionais possuem extração C COMPLETE, 905 formas da família
Load/Store permanecem especializadas e foram encaminhadas exclusivamente à
3.4.2. O relatório possui 2.172 linhas lógicas (91 testemunhos completos,
1.084 rejeições fixas de bits, 91 testes boundary, 905 classificados especiais
e cabeçalho), SHA-256
`e7893e5f0c2b6443f1df6d978ad7d31136d9a34dfadca708a4ec3fcccfa2de41`.
Metadados incluem acesso/dado/pair, direção, writeback, SP/ZR, sinal/zero,
literal, offset, expressão registrador e sobreposição estática/executável.
Os 14 hashes anteriores e os invariantes 3.099/4.296/45.360/506 permanecem.

