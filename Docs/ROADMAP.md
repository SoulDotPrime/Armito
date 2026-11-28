# Fase 3.3.3 — validação abrangente e fechamento de controle (2026-11-26)

Fase 3.3 encerrada: 76/76 formas-alvo com testemunho COMPLETE (49 desvios
convencionais, 16 autenticados e 11 excepcionais), com classificação exclusiva
das 53 formas restantes para a **Fase 3.6**: 31 hints, 7 barreiras, 4 PSTATE
e 11 de sistema. Esse encaminhamento substitui a indicação provisória de
3.3.3 registrada nas notas históricas de 3.3.1/3.3.2; 3.6 não iniciada.

`control_flow_comprehensive_validation.py` + `.csv` (634 linhas, SHA-256
`5a96fa23d0351af1660a1ffe46f2c0b0959368296882da5286503568c72dd62e`)
conferem os 13 relatórios fixados, 23.385 casos de 3.3.1/3.3.2 (22.216
COMPLETE, 1.113 flips rejeitados pela forma, 54 gates de feature e 2
colisões válidas RET*SPPCR/RETAA-B) e 1.336 positivos LLVM qualificados.
Novas verificações: 412 rebases de PC, incluindo extremos, wrap de 64 bits
e alinhamento; 4 perfis independentes `PAuth_LR` habilitado/
`PAuth` desabilitado. Dos flips, LLVM considera 679 não disponíveis na
referência e 434 codificações válidas de outra forma, não todas inválidas.
Corrigido `branch_reads_pc` nos desvios convencionais diretos e BLR (BR/RET
não leem PC para determinar alvo). Preservados 13 hashes, 290.770
ocorrências históricas, 3.099 padrões/4.296 formas/45.360 descritores e
506 REJECT; `make -B test`, `make test`, `make catalog-test` e EDK2 X64 GCC
DEBUG/RELEASE EXIT 0. Sem execução semântica, algoritmo PAC Apple ou
tradução nativa; Fase 3.4 não iniciada.

# Fase 3.3.2 — desvios autenticados e exceções (2026-11-26)

Encerradas 27/27 formas COMPLETE: 16 desvios autenticados (8 BRA*/BLRA*,
2 RETAA/RETAB, 2 RET*SPPCR, 2 RET*SPPC, 2 ERETAA/ERETAB) e 11 formas
excepcionais (9 do grupo `exception`, ERET e DRPS). Metadados públicos Arm
agora distinguem alvo e modificador de registrador, zero/SP, chave IA/IB,
chamada/retorno, PC, LR, PSTATE, ELR/SPSR ou DLR/DSPSR de debug, imediato
de 16 bits, classe excepcional e requisito de EL2/EL3; perfil de guest não
contém EL atual e a extração não executa exceções ou PAC.

`authenticated_control_flow_validation.py`/`.csv` (SHA-256
`acebf7ec98c12d14000310c15cc65ef86dd5ba4b2dfed38b3e9541f07ca6e982`)
contêm 1.088 casos: 539 COMPLETE confirmados com LLVM MC 21.1.8, 530
flips fixos rejeitados pela forma, 17 feature gates e 2 colisões válidas
RETAASPPCR/RETABSPPCR -> RETAA/RETAB quando Rm=31. SMC/DCPS3 requerem EL3
na referência; a necessidade de EL2 para HVC é metadado, não validação de
EL corrente. As 53 formas restantes são 31 hints (incluindo alguns PAuth),
7 barreiras, 4 PSTATE e 11 instruções de sistema; encaminhadas à futura
3.3.3, **não iniciada**. Nenhum comportamento proprietário Apple ARM64E foi
inferido; Compatibility/CPU continua sem backend. Os 12 hashes, 289.682
ocorrências históricas, 3.099 padrões, 4.296 formas, 45.360 descritores e
a lacuna dos 506 REJECT permanecem. `make -B test`, `make test`,
`make catalog-test`, EDK2 DEBUG/RELEASE EXIT 0.

# Fase 3.3.1 — desvios convencionais (2026-11-26)

Encerrada a extração de 49 formas convencionais: `branch_imm` 2 (B/BL),
`condbranch` 2 (B.cond/BC.cond), `compbranch` 4 (CBZ/CBNZ), `compbranch_imm`
12, `compbranch_regs` 12 e `compbranch_regs2` 12 (FEAT_CMPBR), `testbranch` 2
(TBZ/TBNZ) e `branch_reg` 3 (BR/BLR/RET). As outras 80/129 formas foram
inventariadas, não declaradas completas: 16 desvios autenticados, 11 formas de
retorno de exceção/exceção e 53 hints, barreiras, estado e sistema (alguns
hints PAuth). O próximo trabalho autenticado/exceção fica para 3.3.2, não
iniciada; as demais famílias de controle não são desviadas para execução.

`control_flow_validation.py`/`.csv` fixam 22.297 casos (21.677 COMPLETE, 583
flips de bits fixos rejeitados por forma e 37 feature gates rejeitados),
797 comparações LLVM MC 21.1.8 `+cmpbr,+hbc` e 49/49 testemunhos COMPLETE.
SHA-256 `edddbdeab51b3491df3189d94445dbd19254a8e8e9443765adf0a1a61db4a4ff`.
Domínios exaustivos: 18.432 combinações `imm9`, 1.440 índices de Rt/Rn,
768 de Rm, 768 imediatos de comparação `imm6`, 128 índices de bit e 32
condições B.cond/BC.cond; offsets maiores usam extremos determinísticos.
Offsets de imm26/imm19/imm14/imm9 são sinalizados, escalados por quatro e
relativos ao PC; `imm6` de CBcc é imediato de comparação, não offset. O runtime
expõe tipo/alvo/fallthrough, LR, NZCV, largura de comparação e bit_index sem
implementar execução. `make -B test`, `make test`, `make catalog-test` e builds
EDK2 X64 GCC DEBUG/RELEASE passaram. As 11 baselines 3.2, suas 267.385
ocorrências históricas, 3.099 padrões, 4.296 formas, 45.360 descritores e a
lacuna dos 506 REJECT permanecem intactos.

# Fase 3.2.8 — validação abrangente Integer e finalização (2026-11-26)

Consolida 3.2.0–3.2.7 e fecha a Fase 3.2. Novo oráculo
`integer_comprehensive_validation.py` + `.csv` (SHA-256 `3f24c847…`, 289 linhas)
prova 195/195 formas COMPLETE (evidência positiva por forma, perfil completo de
features, LLVM MC qualificada), 93/93 aliases (92 selecionados + REV64
negativo-only) e 112 operações, com cobertura por forma/operação/alias/operando/
dependência/restrição/feature. As 38 formas `runtime-partial` da 3.2.7 eram
falso-positivo do oráculo de codificações inválidas (semente ambígua pelo `value`
bruto; bits de opcode [11:10] são wildcard e colidem com a forma irmã). O runtime
já extraía COMPLETE cada forma com a semente correta; a correção foi no oráculo
(semente distintiva), re-fixando `integer_invalid_encodings.csv` em `550ba37f…`.
Campos estruturais de opcode (RAW) diferenciados dos operandos arquiteturais; sem
alteração C. 3.099/4.296/45.360 e 267.385 ocorrências preservados; 506 REJECT
mantidos. `make -B test`/`make test`/`make catalog-test` EXIT 0; EDK2 DEBUG/RELEASE
EXIT 0. Semântica em execução adiada; 3.3 não iniciada.

# Fase 3.2.7 — codificações inválidas/reservadas/unsupported (2026-11-26)

Oráculo independente `integer_invalid_encodings.py` audita as 195 formas e
classifica cada codificação: válida, `undefined` (bits fixos/ reservados
divergentes), `runtime-partial` (válida mas o probe retorna REJECT/PARTIAL) e
`feature-gated` (dependente de FEAT_CSSC/MTE/CPA/PAuth/PAuth_LR/CRC32/FlagM).
3.173 casos (semente + flip de cada bit fixo), 195 formas, 0 `undue-accept`
(o probe nunca aceita um padrão indefinido), 38 formas `runtime-partial`
(ex.: ASRV, LSRV, RORV, SDIV, REV, CLS, CSINC, CSNEG, UMADDL, UMULH) — todas
arquiteturalmente válidas, rejeitadas por limitação do runtime (PARTIAL por
campos de opcode RAW), NÃO inválidas. LLVM MC usado como referência
complementar (leniente com bits reservados), não autoridade. `integer_invalid_
encodings.csv` (SHA-256 `977a6320…`), cobertura por forma/restrição/feature/
classificação. 9 hashes e 264.212 ocorrências preservados; 3.099/4.296/45.360.
`make -B test`/`make test`/`make catalog-test` EXIT 0; EDK2 DEBUG/RELEASE EXIT 0.
506 REJECT mantidos; semântica em execução adiada; 3.2.8 não iniciada.

# Fase 3.2.6 — operandos implícitos Integer (2026-11-26)

Oráculo independente `integer_implicit_operands.py` classifica as 195 formas
Integer/Data Processing segundo a especificação Arm: operandos explícitos,
implícitos (leituras de merge), derivados e constantes; dependências de leitura/
escrita; NZCV (CCMP/CCMN lêem e escrevem; ADDS/SUBS/ANDS/BICS/ADCS/SBCS/SETF/
RMIF escrevem); carry (ADC/SBC/ADCS/SBCS); condicionais (CSEL/CSINC/CSINV/
CSNEG/CCMP/CCMN); SP/ZR/WSP/XZR contextuais; leituras implícitas de destino
(MOVK, BFM); larguras e dependências parciais. 1.800 casos (positivos/
negativos/limítrofes por registrador 0/1/30/31), 195 formas, 0 divergências;
`integer_implicit_operands.csv` com SHA-256 `6df6ade8…`, cobertura por forma/
operação/operando/tipo. A flag `IMPLICIT_REGISTER` do runtime permanece reservada
ao contexto SP (baseline de registradores intacta); o contrato de leitura
implícita é metadado arquitetural do oráculo, não altera o runtime C. 8 hashes e
262.412 ocorrências preservados; 3.099/4.296/45.360 intactos. `make -B test`,
`make test`, `make catalog-test` EXIT 0; EDK2 X64 GCC DEBUG/RELEASE EXIT 0
(2.502.656 bytes). Equivalência semântica em execução segue requisito de fases
futuras; 506 REJECT mantidos; 3.2.7 não iniciada.

# Fase 3.2.5.2 — compatibilidade de toolchain e finalização (2026-11-26)

Substituiu os gates literais de `LLVM MC 18.1.3` por uma política explícita de
toolchains verificadas. Novo módulo `Tests/Decoder/AARCH64/Validation/llvm_toolchain.py`:
`LLVM_MC` seleciona o executável (padrão `llvm-mc`); `QUALIFIED` registra
18.1.3 (referência histórica), 18.1.8 e 21.1.8; `require_qualified` rejeita
qualquer outra versão com diagnóstico claro. A qualificação é empírica: cada
toolchain é aceita somente se reproduzir **todas** as baselines fixadas
(registrador 30.212, immediate 62.851, bitmask 114.688, condição 6.656, alias
48.005, rejeições), nunca por um único corpus. 18.1.8 e 21.1.8 foram qualificadas
assim (`operand-extraction-test` com EXIT 0 em ambas). `make -B test`, `make test`
e `make catalog-test` passam com EXIT 0 (padrão 21.1.8); EDK2 X64 GCC DEBUG e
RELEASE passam (EFIs de 2.502.656 bytes). Preservados: 93 aliases (68 variáveis,
25 constantes-verdadeiros), 48.005 pares, 142.731 linhas, 262.412 ocorrências
acumuladas, 3.099/4.296/45.360 e os 8 hashes fixados. Runtime C não tocado;
equivalência semântica em execução segue requisito das fases semânticas futuras;
lacuna dos 506 REJECT mantida; 3.2.6 não iniciada. Comandos reproduzíveis:
`make test`; `LLVM_MC=/caminho/llvm-mc make operand-extraction-test`.

# Fase 3.2.5.1 — validação de predicados concluída; encerramento bloqueado (2026-11-26)

Retomada da subfase sem repetir trabalho comprovado. O oráculo
`integer_alias_predicates.py` e o relatório `integer_alias_predicates.csv`
(SHA-256 `e9f412f7…`) permanecem intactos e reproduzem byte a byte. Esta
sessão: (1) classificou os 93 predicados — 68 variáveis com polaridade
positiva e negativa, 25 constantes-verdadeiros só com caso positivo, 0
constantes-falsos; REV64 (`preferred=false`) nunca selecionado; a asserção
foi adicionada ao oráculo sem alterar o CSV; (2) criou o relatório permanente
`integer_alias_coverage.csv` (93 linhas, SHA-256 `05e1d7d7…`) e
`integer_alias_coverage.py`, ligado ao `operand-extraction-test`; (3) fundamentou
que as 106 sobreposições LSL/UBFIZ (36 de 32 bits, 70 de 64 bits) e as 170
diferenças BFC/BFI são preferências textuais da MESMA operação canônica
UBFM/BFM, não divergências arquiteturais; (4) preservou os hashes das 214.407
ocorrências anteriores. `make catalog-test` passou; `make -B test` e `make test`
falham APENAS no gate de versão `integer_form_differential.py`/`integer_
immediate_differential.py` (exigem LLVM MC 18.1.3; o ambiente só oferece
21.1.8 e 18.1.8, ambos reproduzindo os hashes fixados). EDK2 X64 GCC DEBUG e
RELEASE passaram (EFIs de 2.502.656 bytes). Pendências: LLVM 18.1.3 indisponível
(sem sudo, sem binário x86_64, apt só 18.1.8) impede o `make test` completo; a
equivalência semântica em execução (NZCV/SP/ZR/larguras no tradutor/IR) é
dependente das fases semânticas futuras. Não iniciar 3.2.6. Lacuna histórica
dos 506 REJECT mantida.

# Fase 3.2.5.1 — campanha de predicados e prioridades, ainda aberta (2026-11-26)

Inventário completo dos 93 aliases Integer nas 66 formas; campanha determinística
de 48.005 pares, 142.731 registros individuais de alias. Oráculo Python
independente do catálogo C avalia AST de condições, funções Arm BFXPreferred e
MoveWidePreferred, cruza o probe e LLVM MC 18.1.3. 92 aliases possuem seleção
positiva; REV64 tem preferência Arm explicitamente falsa. Sobreposição
LSL/UBFIZ identificada em 106 codificações. Ainda não foi provada equivalência
semântica independente para todos os aliases, nem prioridade normativa
inequívoca para essa sobreposição; por isso a validação 3.2.5.1 não está
encerrada. Não iniciar 3.2.6. Lacuna histórica dos 506 REJECT mantida.

# Fase 3.2.5 — auditoria parcial; fase aberta (2026-11-26)

Campanha nova para condições das 16 formas `condsel`/`condcmp` e aliases
condicionais: 6.656 pares em relatório imutável. Ainda falta auditar
independentemente os predicados e prioridades das 93 entradas de alias
Integer, os demais grupos e a equivalência semântica; não declarar a fase
concluída nem iniciar a 3.2.6. A lacuna histórica dos 506 REJECT permanece.

# Fase 3.2.4 — DecodeBitMasks e validação bitfield (2026-11-26)

Concluída na baseline atual: oráculo independente por bit verifica 114.688 pares
(14 formas × 2 valores de N × 64 immr × 64 imms) e máscaras extraídas. Os
1.016 casos transferidos da 3.2.3 foram confrontados por par, inclusive os 56
rejeitados (40 all-ones, 16 len inválido). O runtime C existente concordou com
o pseudocódigo Arm; não houve correção arquitetural necessária. Resultado e
reprodução: `Docs/COMPATIBILITY.md`. A 3.2.5 e tradução semântica não foram
iniciadas. A lacuna do corpus histórico de 506 continua aberta.

# Fase 3.2.3: baseline atual encerrada; lacuna histórica registrada (2026-11-26)

Para a baseline versionada em `86cd1bb`, o teste imutável regenera e compara
byte a byte os 62.851 candidatos (89 formas/118 campos): 52.209 verificações
numéricas LLVM, 9.624 rejeições arquiteturais justificadas, dois `SETF`
fixos. Os 1.016 `log_imm` (960 aceitos, 56 rejeitados) estão formalmente
transferidos à 3.2.4, **não** validados nesta fase. Os 67 testemunhos de
regressão de registradores, as 195 formas e as 880 rejeições arquiteturais
do seu corpus continuam verificadas. Somente a cobertura immediate aplicável
à baseline atual permite encerrar **3.2.3 para esta baseline**; 3.2 integral,
efeitos implícitos, demais operandos e tradução semântica continuam abertos.
Não iniciar 3.2.4 nesta execução. A diferença com as contagens do resumo
anterior é uma lacuna histórica permanente, não equivalência inferida; detalhes
e hashes em `STATUS.md`, `COMPATIBILITY.md` e `FAILS.md`.

# Fase 3.2.3.1: auditoria reproduzível Integer immediate (histórico, 2026-11-26)

No corpus reproduzido nesta árvore, 9.176 rejeições pelo LLVM e 880 do
corpus de registradores foram reconciliadas por
codificação contra as regras Decode_UNDEF do pacote oficial Arm ISA A64
2025-12 (hash SHA256 `845ed227a6692ddb6b602da2ecbbac776620195a9c001ec576ced3a9a53dc26b`).
Não houve limitação de feature LLVM entre elas: `imm3 > 4`, shifts proibidos
de 32 bits e `immr/imms[5]` de bitfield de 32 bits são invalidade da ISA.
O catálogo passa a rejeitá-las; 62.851 candidatos e 30.212 candidatos de
registradores continuam gerados, sem contabilizar rejeições como verificações.
Immediates não-bitmask aplicáveis neste corpus têm comparação LLVM e validação
das restrições Arm; 960 casos DecodeBitMasks aceitos e 56 rejeitados permanecem
exclusivamente na 3.2.4. O resumo histórico da retomada descrevia 506 REJECT
(459 bitfield, 47 addsub_shift) e distribuição diferente das 9.176 rejeições;
o corpus reproduzido aqui apresenta 504 REJECT (448 bitfield, 56 log_imm).
Sem os artefatos originais, não é correto afirmar identidade entre os corpora.
Este registro descreve o bloqueio anterior à fixação da nova baseline; a
discrepância permanece como lacuna histórica, sem bloquear a baseline atual.
Não avançar para tradução semântica.

# Fase 3.2.3: immediates Integer (2026-11-26)

O diferencial por forma cobre os campos immediate de 89 formas Integer/Data
Processing, com valores brutos e efetivos separados. Testa campos de 3 a 12 bits
exaustivamente quando o LLVM aceita a codificação; combinações `immr/imms`
bitfield são exaustivas. A extensão de sinal de SMAX/SMIN e as escalas de
ADDG/SUBG e PAuth-LR foram corrigidas. `DecodeBitMasks` de logical-immediate
permanece reservada à 3.2.4, assim como os efeitos implícitos e as condições.
Dos 62.851 pedidos, 52.209 comparações são positivas; 9.176 foram rejeitados
pelo LLVM, 960 máscaras lógicas adiadas e 2 máscaras fixas implícitas SETF
sem operando impresso. Os bloqueios de validade e SETF são resolvidos na 3.2.3.1.

# Fase 3.2.2: registradores Integer (2026-11-26)

Concluída a verificação independente dos registradores aplicáveis às 195
formas: LLVM MC 18.1.3, 67 regressões históricas reproduzidas e resolvidas,
26.653 comparações válidas e cobertura exaustiva de cada valor de campo GPR
livre. COMPLETE continua sendo estado do extrator, separado dessa evidência.
A Fase 3.2 integral permanece aberta para demais operandos e restrições;
não avançar para tradução semântica.

# Auditoria independente Integer (histórico anterior, 2026-11-26)

A Fase 3.2 permanece aberta: 195 COMPLETE representam testemunhos, não 195
formas independentemente verificadas. O diferencial automatizado por forma
em `Tests/Decoder/AARCH64/integer_form_differential.py` encontra divergências
arquiteturais de registrador ainda não resolvidas. Não iniciar 3.3 até
comparar immediates, máscaras, aliases, implícitos e inválidos de todas as
formas, inclusive as não suportadas pelo LLVM MC instalado.

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

## Fase 05A --- Expansão do Decoder AArch64, Fase 1

-   [x] 05A.01 --- Auditar Decoder, IR, CPU State, testes e documentação
-   [x] 05A.02 --- Fixar uma fonte estruturada e reproduzível para o catálogo
-   [x] 05A.03 --- Catalogar operações, formas, aliases e padrões de codificação
-   [x] 05A.04 --- Preservar campos, condições, extensões e famílias da fonte
-   [x] 05A.05 --- Definir prioridade e política explícita para sobreposições
-   [x] 05A.06 --- Validar IDs, referências, campos e composição hierárquica
-   [x] 05A.07 --- Medir cobertura representativa do Decoder existente
-   [ ] 05A.08 --- Implementar os módulos de decode da Fase 2

O catálogo descreve a especificação AArch64 A-profile v9Ap6-A de
2024-12; sua existência não marca nenhuma forma como suportada. Consulte
`Decoder/AARCH64/Catalog/` para os dados, ferramenta de validação e
atribuição de licença.

## Fase 05B --- Expansão do Decoder AArch64, Fase 2

-   [x] 05B.01 --- Gerar tabelas C estáticas reproduzíveis do catálogo
-   [x] 05B.02 --- Implementar busca por índice de bits e prioridade
-   [x] 05B.03 --- Validar restrições fixas e conflitos de prioridade
-   [x] 05B.04 --- Comparar busca indexada com matcher linear host
-   [x] 05B.05 --- Integrar resultado catalog-only sem declarar suporte
-   [x] 05B.06 --- Integrar compilação EDK2 X64 DEBUG/RELEASE
-   [x] 05B.07 --- Avaliar AST conditions e requisitos de features
-   [x] 05B.08 --- Classificar integralmente válido, inválido, reservado e indisponível
-   [x] 05B.09 --- Validar amostras arquiteturais e diferenciais por padrão
-   [ ] 05B.10 --- Extrair operandos e decodificar formas por família
-   [ ] 05B.11 --- Integrar semântica à IR e backend sem degradar operações
-   [ ] 05B.12 --- Exercitar o Decoder no caminho de runtime/firmware

As condições AST de guards são compiladas para bytecode C e avaliadas com
capabilities explícitas do guest. `BFXPreferred`, `MoveWidePreferred` e
`SVEMoveMaskPreferred` também são avaliadas para seleção de aliases. As 599
conditions catalogadas e as 288 entradas de alias são avaliadas em runtime
sem interpretar JSON ou strings; nenhuma condition permanece não suportada.
Amostras positivas/negativas cobrem os 3.099 padrões e 33 casos diferenciam
AARCHMRS contra LLVM MC. O Core/ArmitoBoot inicializa o catálogo e registra
a API de operandos sem decodificar durante o boot. Reconhecer, validar ou
extrair operandos não declara suporte semântico, tradução IR, backend ou
execução.

## Fase 05C --- Expansão do Decoder AArch64, Fase 3: Universal Operand Extraction

### Fase 3.2 --- Integer/Data Processing

- [x] Inventariar 195 formas (`dpimm` 44, `dpreg` 151) e 112 IDs de operação.
- [x] Separar seletores de codificação dos operandos e medir estados por forma.
- [x] Validar larguras mistas GPR e seis diferenças de registradores/modificadores com LLVM MC.
- [x] Extrair máscaras de escrita e de topo para bitfields de 32/64 bits.
- [ ] Validar todos os operandos efetivos, aliases, restrições e extremos de cada forma por referência independente.

Uma palavra positiva por forma produziu 195 estados COMPLETE, mas os seis
casos diferenciais não demonstram correção integral das 195 formas. Não
declarar a Fase 3.2 concluída nem iniciar tradução semântica.

-   [x] 05C.01 --- Gerar descritores tipados estáticos para as 4.296 formas
-   [x] 05C.02 --- Definir operandos universais com estados raw/parcial/interpretado/não suportado
-   [x] 05C.03 --- Extrair subconjuntos GPR, immediates, branches, memória e system registers
-   [x] 05C.04 --- Preservar campos PAC e contexto de autenticação
-   [x] 05C.05 --- Avaliar condições de aliases preferenciais em C
-   [x] 05C.06 --- Integrar a inicialização estrutural e registrar o extrator sem decodificar no boot
-   [x] 05C.07 --- Verificar descritores de todas as formas e amostras representativas
-   [ ] 05C.08 --- Completar extração de operandos SIMD/NEON, SVE e SME por forma
-   [ ] 05C.09 --- Completar listas, lanes, registradores de sistema e modos de endereçamento
-   [ ] 05C.10 --- Expandir diferencial LLVM MC para comparar operandos e aliases
-   [ ] 05C.11 --- Cobrir todas as formas aplicáveis com casos positivos e negativos

A validação atual encontra descritores para 4.296/4.296 formas (45.360
descritores) e extrai 3.093/3.099 amostras representativas de padrões; essas
amostras aceitas, inclusive parciais/raw, alcançam 3.026 formas e
1.478/2.095 IDs de operação. Os casos SIMD/SVE/SME, listas, lanes e alguns
modos de endereçamento ainda podem resultar em extração parcial ou não
suportada. Essas métricas não provam cobertura semântica integral por forma.

Auditoria host de 2026-11-26: a busca determinística de até 96 candidatos
por forma alcançou 4.288/4.296 formas (0 completas, 4.255 parciais, 33 raw,
0 não suportadas, 8 não alcançadas). Não é prova exaustiva de alcançabilidade
ou validade de todos os operandos. O estado da Fase 05C permanece aberto;
não iniciar a fase seguinte com base nessas métricas.

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
-   [x] 06.09.01 --- Testes de contrato (`Tests/IR/IRTest.c`, 172 verificações)
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

**05C.08 --- Completar a extração por família e forma**. Manter as
operações sem extração completa ou backend explicitamente identificadas
como parciais/não suportadas; não avançar para tradução semântica antes de
fechar e medir as lacunas da extração.
# Fase 3.4.1 — Memory/Load/Store convencionais

Extração convencional concluída para 94 formas, incluindo as três PRFM, com
902 formas classificadas sem sobreposição: 433 para 3.4.2, 380 atomics/MOPS
para 3.6, 83 exclusive/acquire-release para 3.6 e 6 PAC/GCS para 3.5/3.6.
Cobertos escalares, literais e pares: carga/armazenamento 8/16/32/64,
sign/zero extension, offsets imediatos, registrador, literal, pre/post-index,
pair no-alias e prefetch. Oráculo:
`memory_operand_validation.py/.csv`, SHA-256
`313b773c73ff173519effcb6513dc2abf0b53a38f6c5071bf94a632770967606`.
Não há MMU, TLB, execução semântica ou acessos reais; 3.4.2 não iniciada.
# Fase 3.4.2 — Memory/Load/Store especializada

Extração especializada concluída para 433 formas (105 operações, 8 features,
12 aliases) em 18 famílias: AdvSIMD estrutural/lane
(`asisdlse/sep/so/sop`), unprivileged/unscaled/writeback/pair/lit/regoff,
MOPS (`memcms`), LSUI RMW (`memop_unpriv`), MTE (`ldsttags`/STGP) e prefetch
(RPRFM/PRFUM). As 469 formas restantes seguem encaminhadas a 3.5/3.6 (PAC/GCS,
exclusive/ordering, atomics). Oráculo: `specialized_memory_validation.py/.csv`,
SHA-256 `2979f0373dbfe08c89959fdcec602dedb9c0eaaf340f93ccdd93d455d7887285`.
Campanha: 47.197 casos, 5.001 comparações LLVM MC 21.1.8, 433/433 sementes
COMPLETE distintas, 12 aliases, 6.570 flips fixos e 429 feature-negatives
rejeitados. Metadados simbólicos cobrem banco/largura de registrador, lista
estrutural, interleave/replicate, merge de lane inativa, validade de lane,
estágios/registradores/options MOPS, estado de tag MTE, tipo de offset, bytes
totais, post-index e zero-discard MOPS. 94 formas convencionais e 15 hashes
históricos preservados. Sem MMU/TLB, execução semântica ou acessos reais;
3.4.3/3.5/3.6 não iniciadas.
# Fase 3.4.3 — Integração Load/Store no fluxo real

As 527 formas COMPLETE (94 convencionais + 433 especializadas) foram integradas
ao fluxo real via `ArmitoDecoderDecodeInstructionWithOperands` e
`AARCH64DecodeWithAddress`. O fluxo recebe 32 bits, classifica candidatos no
catálogo indexado (64 buckets por bits [31:26]), resolve máscara/valor/guards,
verifica features, seleciona extrator, extrai e normaliza operandos, e retorna
representação estruturada determinística. Formas encaminhadas (469, 18 grupos)
retornam UNSUPPORTED; reservadas/inválidas retornam INVALID. Campanha
end-to-end: `integration_validation.py/.csv`, 996 casos (527 OK + 469
UNSUPPORTED), SHA-256 `3fa4b4e2…`. 15 hashes históricos e invariantes
3.099/4.296/45.360/506 preservados. Sem MMU/TLB, execução semântica ou acessos
reais; 3.5/3.6 não iniciadas.
# Fase 3.5.1 — System Instructions & System Registers

53 formas de sistema em 9 grupos tornam-se COMPLETE. `AARCH64SysReg.h/.c` define
identidade canônica (~60 registradores, encoding op0/op1/CRn/CRm/op2, access
read/write, implementation-defined). `AARCH64Operands.c` extrai campos de sistema
(op0/op1/CRn/CRm/op2/Rt), resolve nome/identidade via `AARCH64SysRegResolve`,
classifica access pelo bit L (MRS=read, MSR=write), SYSL por bit 21, barreiras
domínio/tipo, hints immediate, PSTATE campo. `AARCH64DecodeLoadStore` aceita
formas de sistema PATTERN_MATCHED_UNVALIDATED; `AARCH64OperandsExtractWithProfile`
isenta sistema do status PARTIAL (alias/catalog_status/raw+interpreted); GPR de
sistema recebe `register_width=64`. NOP/HINT e PAC-SP roteiam pelo catálogo para
extrair operandos preservando campos PAuth/IR. `Decoder.c` só retorna cedo
(sem IR) quando operandos são solicitados (`operands != NULL`). Oráculo:
`system_validation.py/.csv`, 53 casos, 50 comparações LLVM MC, SHA-256
`207ef6aee587e9ce9db579a1a10aab90638ba332be00ca0f22c4a43dd2281bcb`.
15 hashes históricos e invariantes 3.099/4.296/45.360/506 preservados.
Testes: Core 78/0, IR 172/0, Decoder 226/0, CPU 58/0, ARM64E 514/0; EDK2 X64
GCC DEBUG/RELEASE EXIT 0. Sem MMU/TLB, execução semântica ou acessos reais;
3.5.2 em andamento e 3.6 não iniciada.

# Fase 3.5.2 — Security-relevant instructions (em andamento)

Seeds de 22 formas-alvo COMPLETE: exceções, cargas autenticadas, operações
MTE de tag arithmetic, PACGA, ERET/DRPS. `security_validation.py/.csv` está
no gate `make test`: 397 casos (22 seeds, 363 mutações de bits fixos e 12
rejeições por feature), 22 comparações LLVM MC; SHA-256
`d52277ecfe29270e7c50df55571068041e637c362624a0182d9872993f1af947`.
Chaves DA/DB/GA usam o enum de operandos; o metadata existente de exceções
não é sobrescrito pela síntese de operandos. BTI C/J/JC produzem OK/COMPLETE;
GCSSTR/GCSSTTR seguem forwarded/UNSUPPORTED para 3.6; `ldsttags` permanece
COMPLETE. EDK2 DEBUG/RELEASE passaram usando `Armito/Armito.dsc`; pendente
3.4.R E–I.
