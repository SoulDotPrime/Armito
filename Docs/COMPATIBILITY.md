# Compatibilidade final de controle — 3.3.3 (2026-11-26)

`control_flow_comprehensive_validation.py`/`.csv`: 634 linhas, SHA-256
`5a96fa23d0351af1660a1ffe46f2c0b0959368296882da5286503568c72dd62e`.
76 formas COMPLETE, 23.385 casos herdados reconciliados, 412 rebases PC
e 4 perfis PAuth_LR-only; 1.336 confrontos positivos LLVM MC 21.1.8
qualificado e 1.113 flips classificados separadamente por forma/global.
O wrapper LLVM 18.1.8 preserva baselines de 3.2; MC 21.1.8 cobre
`+cmpbr,+hbc,+pauth,+pauth-lr,+tme,+el3` para controles novos.
13 hashes anteriores, 290.770 ocorrências, 3.099/4.296/45.360 e 506
REJECT preservados. `Armito.dsc`/`Armito.inf` preexistentes não tocados.
`make -B test`, `make test`, `make catalog-test`, EDK2 DEBUG/RELEASE EXIT 0.
Proveniência pública Arm não implica PAC proprietário Apple: provider
`compatibility_only`, Mac14_3 sem backend, CPUState sem EL/ELR/SPSR
completos e sem execução semântica. As 53 formas de controle restantes
foram encaminhadas à Fase 3.6, não iniciada.

# Autenticação/exceções — 3.3.2 (2026-11-26)

Oráculo `authenticated_control_flow_validation.py`, CSV de 1.142 linhas,
SHA-256 `acebf7ec98c12d14000310c15cc65ef86dd5ba4b2dfed38b3e9541f07ca6e982`.
27 sementes distintas COMPLETE, 539 referências LLVM MC 21.1.8 com
`+pauth,+pauth-lr,+tme,+el3`, 530 negativas pela forma e 17 features
desabilitadas; 2 palavras globalmente válidas são atribuídas a RETAA/
RETAB em vez de RET*SPPCR. SMC/DCPS3 falham na referência sem EL3.
Preservados os 12 relatórios anteriores, 289.682 ocorrências, 3.099/
4.296/45.360 e 506 REJECT. Testes sequenciais e builds EDK2 X64 GCC
DEBUG/RELEASE EXIT 0. O teste valida instruções públicas Arm, não PAC
proprietário Apple ARM64E ou execução nativa Mac14_3; o provider de
PointerAuth é `compatibility_only` e CPUToCPU/Apple/Mac14_3 está vazio.

# Compatibilidade de desvios — 3.3.1 (2026-11-26)

`control_flow_validation.py` + `control_flow_validation.csv` auditam as 129
formas de controle (49 convencionais COMPLETE, 80 não abrangidas) com 22.297
casos e 797 comparações LLVM. SHA-256 fixado:
`edddbdeab51b3491df3189d94445dbd19254a8e8e9443765adf0a1a61db4a4ff`.
O LLVM MC 21.1.8 qualificado (`llvm-mc-21` ou `LLVM_MC_CONTROL`) é necessário
para `+cmpbr,+hbc`; o wrapper 18.1.8 qualificado continua reproduzindo os
11 relatórios 3.2. A suíte exige concordância de forma, mnemonic, target,
displacement e metadados nos casos verificados; flips de bits fixos verificam
rejeição *pela forma*, não prova de palavra indefinida globalmente.
`make -B test`, `make test`, `make catalog-test` e EDK2 X64 GCC DEBUG/RELEASE
passaram. Nenhuma mudança em Armito.dsc/Armito.inf preexistentes, catálogo
ou 11 relatórios 3.2; 267.385 ocorrências históricas e 506 REJECT preservados.

# Validação abrangente Integer — 3.2.8 (2026-11-26)

Novo oráculo `Tests/Decoder/AARCH64/integer_comprehensive_validation.py` +
`integer_comprehensive_validation.csv` (289 linhas, SHA-256
`3f24c847e36c8df3cfa51db23146c11bcb903dbb6414afccedb21e06fcec2b49`), ligado ao
`operand-extraction-test`. Prova 195/195 formas COMPLETE (semente distintiva por
forma, perfil completo de features, LLVM MC qualificada), 93/93 aliases (92
selecionados + REV64 negativo-only) e 112 operações, com cobertura por
forma/operação/alias/operando/dependência/restrição/feature. As 38 formas
`runtime-partial` da 3.2.7 eram falso-positivo do oráculo de codificações inválidas
(semente ambígua pelo `value` bruto; bits de opcode [11:10] wildcard colidem com a
forma irmã); o runtime já extraía COMPLETE cada forma. `integer_invalid_encodings`
re-fixado em `550ba37f…` (alteração justificada; os outros 9 hashes preservados).
3.099/4.296/45.360 e 267.385 ocorrências preservados; 506 REJECT mantidos.
`make -B test`/`make test`/`make catalog-test` EXIT 0 (LLVM 18.1.8 qualificada,
`LLVM_MC` apontando para o wrapper); EDK2 X64 GCC DEBUG/RELEASE EXIT 0. Semântica
em execução requisito futuro. Reprodução: `make test`.

# Codificações inválidas Integer — 3.2.7 (2026-11-26)

`Tests/Decoder/AARCH64/integer_invalid_encodings.py` classifica as codificações
das 195 formas: undefined (bits fixos), runtime-partial (válida, limitação do
runtime), feature-gated (CSSC/MTE/CPA/PAuth/PAuth_LR/CRC32/FlagM). 3.173 casos,
195 formas, 0 undue-accept, 38 runtime-partial (ASRV/LSRV/RORV/SDIV/REV/CLS/
CSINC/CSNEG/UMADDL/UMULH etc. — válidas, PARTIAL por campos RAW). LLVM MC
complementar. Relatório `integer_invalid_encodings.csv` (SHA-256
`977a632043e5553f8ff99371017320ca6373fd384e5696f2f5fed9af10e4e387`), ligado ao
`operand-extraction-test`. 9 hashes e 264.212 ocorrências preservados;
3.099/4.296/45.360. `make -B test`/`make test`/`make catalog-test` EXIT 0 (LLVM
21.1.8); EDK2 X64 GCC DEBUG/RELEASE EXIT 0 (2.502.656 bytes). 506 REJECT
mantidos; semântica em execução requisito futuro. Reprodução: `make test`.

# Operandos implícitos Integer — 3.2.6 (2026-11-26)

Novo oráculo independente `Tests/Decoder/AARCH64/integer_implicit_operands.py`
classifica as 195 formas Integer/Data Processing segundo a especificação Arm:
explícitos, implícitos (merge), derivados, constantes; leituras/escritas; NZCV
(CCMP/CCMN; ADDS/SUBS/ANDS/BICS/ADCS/SBCS/SETF/RMIF); carry (ADC/SBC/ADCS/SBCS);
condicionais (CSEL/CSINC/CSINV/CSNEG/CCMP/CCMN); SP/ZR/WSP/XZR; MOVK/BFM; larguras
e parciais. 1.800 casos (0/1/30/31 por registrador), 195 formas, 0 divergências.
Relatório `integer_implicit_operands.csv` (1.484 linhas, SHA-256
`6df6ade821c168b50a1f3846743be99700c66b0889375dca05ee86ac7e4734c0`), ligado ao
`operand-extraction-test`. A flag C `IMPLICIT_REGISTER` não foi alterada (reservada
ao contexto SP; baseline de registradores `739a1296…` intacta); o contrato de
leitura implícita é metadado do oráculo. Preservados os 8 hashes e 262.412
ocorrências; 3.099/4.296/45.360. `make -B test`/`make test`/`make catalog-test`
EXIT 0 (LLVM 21.1.8 qualificada); EDK2 X64 GCC DEBUG/RELEASE EXIT 0 (2.502.656
bytes). Equivalência semântica em execução segue requisito futuro; 506 REJECT
mantidos. Reprodução: `make test`.

# Compatibilidade de toolchain LLVM — 3.2.5.2 (2026-11-26)

Os gates literais `LLVM MC 18.1.3` foram substituídos por política explícita de
toolchains verificadas em `Tests/Decoder/AARCH64/llvm_toolchain.py`. Seleção do
executável por `LLVM_MC` (padrão `llvm-mc`); registro `QUALIFIED` com 18.1.3
(referência histórica), 18.1.8 e 21.1.8; `require_qualified` rejeita outras
versões com diagnóstico claro. Qualificação empírica por reprodução integral de
TODAS as baselines fixadas (registrador 30.212, immediate 62.851, bitmask
114.688, condição 6.656, alias 48.005, rejeições), não por um único corpus.
Resultados desta execução (todos EXIT 0): `operand-extraction-test` qualificou
18.1.8 e 21.1.8; `make -B test`, `make test` e `make catalog-test` passam com o
padrão 21.1.8; EDK2 X64 GCC DEBUG e RELEASE passam (EFIs de 2.502.656 bytes).
Preservados: 93 aliases (68 variáveis, 25 constantes-verdadeiros), 48.005 pares,
142.731 linhas, 262.412 ocorrências acumuladas, 3.099/4.296/45.360 e os 8 hashes
fixados (alias `e9f412f7…`, cobertura `05e1d7d7…`, immediate `ed292fcf…`,
registrador `739a1296…`, bitmask `ffbd31d7…`, condição `49b7cf96…`, rejeições
`d6545bbc…`/`3af651c0…`). Runtime C não modificado; equivalência semântica em
execução permanece requisito das fases semânticas futuras; lacuna dos 506 REJECT
mantida. Reprodução: `make test`; `LLVM_MC=/caminho/llvm-mc make
operand-extraction-test`.

# Campanha 3.2.5.1 — fechamento de predicados (2026-11-26)

Retomada da campanha de aliases Integer sem alterar o relatório fixado
`integer_alias_predicates.csv` (SHA-256 `e9f412f7…`), que continua reproduzindo
byte a byte. Classificação independente dos 93 predicados: 68 variáveis com
polaridade positiva e negativa, 25 constantes-verdadeiros apenas com caso
positivo, 0 constantes-falsos; REV64 (`preferred=false`) nunca selecionado. A
asserção de classificação foi incorporada a `integer_alias_predicates.py` sem
mudar o CSV. Novo relatório determinístico
`Tests/Decoder/AARCH64/integer_alias_coverage.csv` (93 linhas, SHA-256
`05e1d7d7df3dde2c8aa1542d5a46a9e93ae0b62b9cee2b94a73b63d4bd0e5433`) e
`integer_alias_coverage.py`, ligado ao alvo `operand-extraction-test`.
As 106 sobreposições LSL/UBFIZ (LSL pref `immr==imms+1`, UBFIZ pref
`imms<immr`) e as 170 diferenças BFC/BFI (BFC `Rn==ZR`, BFI `Rn!=ZR`, BFXIL
`imms>=immr`) são preferências textuais da mesma operação canônica UBFM/BFM;
LLVM MC imprime LSL/BFI, o runtime escolhe UBFIZ/BFC, sem divergência
arquitetural. Os hashes das 214.407 ocorrências anteriores permanecem iguais.
Nesta execução: `make catalog-test` passou; `make -B test` e `make test` falham
apenas no gate de versão de `integer_form_differential.py` e
`integer_immediate_differential.py` (exigem LLVM MC 18.1.3; o ambiente oferece
21.1.8 por padrão e 18.1.8 extraído localmente, ambos reproduzindo os hashes
fixados byte a byte); EDK2 X64 GCC DEBUG e RELEASE passaram (EFIs de 2.502.656
bytes). A equivalência semântica em execução (NZCV, SP/ZR, larguras) depende das
fases semânticas futuras. 3.2.5.1 não declarada encerrada; 3.2.6 não iniciada.

# Campanha 3.2.5.1 — aliases Integer (2026-11-26)

Fonte normativa: pacote Arm `Build/arm-isa-2025-12.tar.gz` (SHA-256 abaixo),
arquivos `ubfm.xml`, `bfm.xml` e aliases correspondentes, e funções
`BFXPreferred`, `MoveWidePreferred` em `shared_pseudocode.xml`. O catálogo
JSON preserva os 93 ASTs extraídos do MRA Arm; oráculo Python independente
do runtime C: `Tests/Decoder/AARCH64/integer_alias_predicates.py`.
48.005 pares forma/codificação nas 66 formas com alias, 142.731 linhas CSV
por alias e codificação (incluindo 47.877 linhas de codificações rejeitadas e
94.854 verificadas). Relatório `Tests/Decoder/AARCH64/integer_alias_predicates.csv`
SHA-256 `e9f412f73d15c3595bb844da9dd9519da76f7fc8cdc92a057763b12cfd0babeb`.
O teste regenera e verifica bytes e hash, não sobrescreve. 106 sobreposições
LSL/UBFIZ; 170 diferenças textuais BFC/BFI. LLVM MC 18.1.3 é comparador,
não norma de preferência. Esta campanha é adicional às 214.407 ocorrências
prévias (30.212+62.851+114.688+6.656), sem alterar seus CSVs. Nenhuma
alegação de equivalência semântica completa nem de execução comprovada:
3.2.5.1 aberta até provar todos os aliases e a prioridade normativa.
Nesta execução passaram, sequencialmente, `make -B test`, `make test`,
`make catalog-test`, seguidos de EDK2 X64 GCC DEBUG e RELEASE (saída zero).
Os hashes das baselines anteriores, inclusive o CSV de 6.656 condicionais,
permaneceram iguais; catálogo/runtime C não foram modificados.

# Campanha parcial 3.2.5 — condições (2026-11-26)

Referência normativa: `Build/arm-isa-2025-12.tar.gz` (hash Arm registrado
abaixo), `shared_pseudocode.xml` (`ConditionHolds`), `csel.xml`, `csinc.xml`,
`csinv.xml`, `csneg.xml`, `ccmp_reg.xml`, `ccmp_imm.xml`, `ccmn_reg.xml` e
`ccmn_imm.xml` e arquivos XML de aliases condicionais. Oráculo Python
`Tests/Decoder/AARCH64/integer_condition_audit.py`; relatório imutável
`Tests/Decoder/AARCH64/integer_condition_audit.csv`, SHA-256
`49b7cf96af57e6172f95bac7b25895d5013c4b5e7eca594a0bb5119f0105f8fe`.
6.656 pares por forma/codificação, 16 formas, 16 condições; relatório
regenerado e comparado byte a byte sem sobrescrita. Esse corpus é **adicional**
às 207.751 ocorrências das três campanhas anteriores; não as modifica.
Limitação: predicados dos demais aliases e semântica não auditados; a fase
3.2.5 permanece aberta.
Nesta execução, `make -B test`, `make test` e `make catalog-test` passaram
sequencialmente; builds EDK2 X64 GCC DEBUG e RELEASE também passaram.

# Manifesto de validação da Fase 3.2.4 (2026-11-26)

Fonte normativa: pacote oficial Arm `Build/arm-isa-2025-12.tar.gz`, SHA-256
`845ed227a6692ddb6b602da2ecbbac776620195a9c001ec576ced3a9a53dc26b`,
`ISA_A64_xml_A_profile-2025-12/shared_pseudocode.xml`, função `DecodeBitMasks`.
LLVM MC 18.1.3 (`--disassemble --triple=aarch64`, recursos descritos abaixo)
é comparação independente adicional, não autoridade para invalidade.

`Tests/Decoder/AARCH64/bitmask_exhaustive.py` gera o relatório determinístico
`Tests/Decoder/AARCH64/bitmask_exhaustive.csv`, SHA-256
`ffbd31d7d7a6ad071602fc44b7b8fc776e0dc33cf62eaed80374f595ab4d8a3c`.
O teste regenera em memória e exige igualdade byte a byte e hash fixado;
não sobrescreve o CSV. Cobertura: 14 formas × N={0,1} × 64 immr × 64 imms =
114.688 pares. Cada linha identifica operação, largura, codificação, campos,
razão da classificação, len, levels, S, R, diff, esize, welem, telem, wmask,
tmask e texto LLVM. Para logical immediate, por operação e por largura:
3.648 válidos, 320 all-ones, 128 len inválido e 4.096 N/largura inválidos.
Para cada operação bitfield em 32 bits: 1.024 válidos, 3.072 imm[5]
inválidos e 4.096 N/largura inválidos; em 64 bits: 4.096 válidos e 4.096
N/largura inválidos. Total: 60.672 válidos e 54.016 inválidos. Conferência
por par dos 1.016 transferidos: 960 válidos, 40 all-ones e 16 len inválido.
O CSV permanece permanente; cada mudança exige revisão explícita de hash.
O alvo `operand-extraction-test` executa esse oráculo após as regressões 3.2.3.
Validação desta execução: `make -B test`, `make test`, `make catalog-test` e
EDK2 X64 GCC DEBUG/RELEASE, todos com código de saída zero. O inventário
existente (3.099 padrões, 4.296 formas, 45.360 descritores) não foi editado.

# Manifesto de reprodução da baseline 3.2.3 (2026-11-26)

Raiz Git de introdução do corpus: `86cd1bb`; IDs de commit identificam a
proveniência, SHA-256 abaixo identifica o **conteúdo**. Executar no diretório
raiz: `make -B test`, `make test`, `make catalog-test`, nessa ordem. O alvo
`operand-extraction-test` recompõe o corpus com os dados JSON, Python 3 e o
probe C e exige igualdade byte a byte com os CSVs; não os regrava. Na workspace
EDK2 (`/home/gui/edk2`, `ArmitoPkg` vinculado a esta árvore):
`build -p ArmitoPkg/Armito.dsc -a X64 -t GCC -b DEBUG`, seguido de `RELEASE`.
Nesta execução: os cinco comandos terminaram com código zero; 67 regressões
históricas de registradores, 195/195 formas, 26.653 verificações positivas
e 880 testemunhos de rejeição continuam executados. Ferramentas: Ubuntu LLVM
MC 18.1.3, `--disassemble --triple=aarch64` com
`--mattr=+crc,+pauth,+mte,+cssc,+flagm,+pauth-lr,+cpa`; Python 3.12.3,
GCC Ubuntu 13.3.0-6ubuntu2~24.04.1, Git 2.43.0; compilação host
`-std=gnu11 -Wall -Wextra -O2`, EDK2 X64/GCC DEBUG e RELEASE.

SHA-256 (caminhos relativos à raiz, salvo o pacote Arm em `Build/`):

| Artefato | SHA-256 |
| --- | --- |
| `Tests/Decoder/AARCH64/integer_immediate_all_candidates.csv` | `ed292fcffc46970c987fddf22e2f73d7a7d7dd75831d5bb187f072b6b5e12fb9` |
| `Tests/Decoder/AARCH64/integer_immediate_rejections.csv` | `d6545bbcd94cc1fbdb27958e96640f6c83ec5d75a24b136fcb8f4595d1403c20` |
| `Tests/Decoder/AARCH64/integer_register_rejections.csv` | `3af651c064766b3d41778432e19134ab96e62d8e6e74decc0ccffd2f1bd94b32` |
| `Tests/Decoder/AARCH64/integer_register_baseline.csv` | `739a129672c95570dddc8b256f2dc9bf9dae2957b470fa8c531130e3f2534015` |
| `Tests/Decoder/AARCH64/integer_immediate_differential.py` | `700c201cdfe96c896849749b5dc1d1084790f782db9ce0cea79ddd3e39b8c9fc` |
| `Tests/Decoder/AARCH64/integer_immediate_rejection_audit.py` | `16293dbf457e451b699c13c3887b98629f7ca539fb63e1731f1071ddb2d0b255` |
| `Tests/Decoder/AARCH64/integer_form_differential.py` | `f6d0cb8bb89b6ee0db73147ef72185646ea46a9e882ee652347ffcfb7999e008` |
| `Tests/Decoder/AARCH64/IntegerFormProbe.c` | `cc691b29faec94bcc86184490f98abb99e021fd5f69a90ded9488d210e60488c` |
| `Decoder/AARCH64/Catalog/AARCH64InstructionCatalog.json` | `5ef585e94947a0e112b248f067b0b2165670f67473e2e3ca795abfbe9f04b6a1` |
| `Build/arm-isa-2025-12.tar.gz` (local, não versionado) | `845ed227a6692ddb6b602da2ecbbac776620195a9c001ec576ced3a9a53dc26b` |

O XML Arm A64 2025-12 fixa `Decode_UNDEF` para imm3 > 4 (addsub_ext), shift=3 ou
imm6[5] em 32 bits (addsub_shift), imm6[5] em 32 bits (log_shift), e N/sf ou
immr[5]/imms[5] em 32 bits (bitfield). Cada uma das 9.624 linhas inválidas
na baseline atual tem regra específica, probe REJECT e LLVM-invalid no CSV;
nenhuma rejeição foi contabilizada como comparação positiva. `SETF` usa
`mask=1101` no XML e não possui operando de máscara impresso. A separação
`log_imm`/`DecodeBitMasks` (960+56) foi transferida à 3.2.4, não executada.

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
# Fase 3.4.1

91 formas convencionais GPR foram validadas independentemente com LLVM MC
21.1.8 e sementes distintivas: 91 seeds, 91 boundary-zero e 1.084 flips fixos
rejeitados pela forma. 905 formas foram classificadas como especializadas e não
constituem cobertura da fase; LSUI/MTE e variantes não-GPR/feature-gated ficam
para 3.4.2. 3.099 padrões, 4.296 formas e 45.360 descritores preservados.

