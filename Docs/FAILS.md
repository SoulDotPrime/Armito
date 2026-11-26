# Fase 3.2.5.1 — sobreposições e limitações (2026-11-26)

Relatório `integer_alias_predicates.csv` registra por codificação 106
predicados simultaneamente preferidos LSL/UBFIZ em UBFM; runtime escolhe
UBFIZ, LLVM MC 18.1.3 imprime LSL. Há ainda 170 impressões LLVM BFI quando
o catálogo Arm escolhe BFC para `Rn=ZR`; ambas preservam a codificação BFM.
Diferença de mnemonic não prova divergência arquitetural. Antes de encerrar
3.2.5.1, fundamentar desempate normativo entre aliases concorrentes e provar
equivalência formal independente de cada alias; execução real só será
verificada nas futuras fases semânticas. Nenhuma correção C foi justificada
apenas pelas diferenças de apresentação. A lacuna do corpus histórico de 506
REJECT continua irrecuperada e não se confunde com estas sobreposições.

# Fase 3.2.5 — cobertura pendente (2026-11-26)

Campanha `integer_condition_audit.csv`: 6.656 pares condicionais confrontados
com probe e LLVM, sem divergência arquitetural justificada nessa amostra.
Diferenças apenas textuais (por exemplo CNEG com fonte ZR) não são tratadas
como falhas de codificação. Falta validar todas as condições/predicados de
alias e prioridades das 93 entradas nas 195 formas; a fase permanece aberta.
Não confundir esse trabalho com a lacuna histórica irrecuperada dos 506 REJECT.

# Fase 3.2.4 — resolução dos 56 REJECT atuais (2026-11-26)

Os 56 pares individualizados em
`Tests/Decoder/AARCH64/integer_immediate_all_candidates.csv` são
verificados um a um contra `bitmask_exhaustive.csv`: 40 possuem S==levels
(logical immediate all-ones reservado) e 16 possuem len<1. Todos são
rejeitados pelo runtime e pelo LLVM MC 18.1.3; a invalidade decorre do
oráculo arquitetural, não da rejeição LLVM isolada. Os outros 960 casos
transferidos têm máscaras válidas iguais às produzidas pelo extrator.
Nenhuma divergência comprovada de DecodeBitMasks exigiu correção C. A lacuna
do corpus histórico de 506 REJECT permanece independente e não resolvida.

# Lacuna histórica permanente da Fase 3.2.3 (2026-11-26)

Busca realizada: `git log --all`, `git reflog --all`, branch
`backup/main-before-sync`, refs de checkpoints Cline e Agent, `git fsck
--no-reflogs --unreachable`, stash e artefatos `/tmp/armito-phase323*`.
O CSV de 62.851 pares, o gerador e o probe aparecem pela primeira vez no
commit `86cd1bb`; não existem no pai, no backup ou nos checkpoints anteriores.
Os nove commits inalcançáveis e 295 blobs soltos examinados não fornecem o
corpus de 506. Logs com o prefixo `/tmp/armito-phase323-` do resumo não estão
presentes. O único probe anterior preservado em
`/tmp/armito-phase3231-original-probe` (SHA-256
`da3173477b117b78ce270c752e76a456768f4dc417be1c9b3fbd827bb753e6e0`)
é artefato local criado para comparar a lógica anterior às guardas, **não**
o probe do resumo perdido; o arquivo original de candidatos não foi
recuperado. Não há comparação par a par possível com o corpus perdido, nem
atribuição confiável das duas codificações em diferença.

Aplicando esse probe anterior **somente aos 62.851 pares atualmente
preservados**, obtêm-se 504 `REJECT`: 448 bitfield + 56 log_imm. As 9.176
respostas LLVM-invalid entre os então aceitos correspondem exatamente a
24 addsub_ext + 128 addsub_shift + 256 log_shift + 8.768 bitfield.
O resumo perdido descreve 506 `REJECT` (459 bitfield + 47 addsub_shift) e
904 + 264 + 2.312 + 5.696 rejeições LLVM por esses quatro grupos. Contagens
iguais de 9.176 **não demonstram** identidade de pares. A diferença pode
envolver geração, probe, classificação ou revisão de catálogo; sem os
artefatos, não se pode isolar qual fator mudou. As guardas novas, que
rejeitam os 9.176 pares inválidos do CSV atual, explicam a diferença entre
probes **no mesmo corpus**, não entre corpora. Não reconstruir registros
históricos a partir de totais. Baseline atual independente dessa lacuna,
limitada à 3.2.3; 1.016 bitmasks transferidos para 3.2.4.

# Fase 3.2.3.1 — resolução de rejeições (registro anterior, 2026-11-26)

Registro completo em `Tests/Decoder/AARCH64/integer_immediate_all_candidates.csv`
(62.851 candidatos); relatórios de rejeições e exceções em
`Tests/Decoder/AARCH64/integer_immediate_rejections.csv`
(9.682 linhas: 9.624 inválidas, 56 bitmasks com validade pendente na 3.2.4,
2 SETF) e
`Tests/Decoder/AARCH64/integer_register_rejections.csv` (880 inválidas).
As 9.176 rejeições LLVM antigas são inválidas pela regra Arm: 24 `imm3>4`,
384 shifts de 32 bits e 8.768 combinações bitfield de 32 bits. Nas 880
históricas: 203 `imm3>4`, 398 shifts de 32 bits, 140 shifts reservados e
139 bitfields de 32 bits. Quinze pares (forma, codificação) são compartilhados
entre corpora; 9.609 são exclusivos do corpus imediato e 865 do de
registradores. Os 448 bitfield `REJECT` antigos e os 56 bitmasks `REJECT`
explicam os 504 casos ausentes da contagem de aceitos **neste corpus**; os 2
SETF são `COMPLETE`, mas não têm comparação numérica de operando LLVM.
Todos os 9.176 invalidamente aceitos agora são rejeitados pelo catálogo.
Nenhuma rejeição passa a contar como validação positiva de operandos.
Os 56 bitmasks não foram considerados `Decode_UNDEF` nesta fase: a regra
`N/sf` é verificável diretamente, mas o diagnóstico de `DecodeBitMasks`
depende da Fase 3.2.4.
**Bloqueio remanescente:** o resumo histórico relatava 506 rejeições do
probe (459 bitfield e 47 addsub_shift), e 9.176 rejeições LLVM distribuídas
em 904 addsub_ext, 264 addsub_shift, 2.312 log_shift e 5.696 bitfield.
O corpus regenerado aqui produz 504 rejeições do probe (448 bitfield e 56
log_imm) e 9.176 LLVM-invalid aceitos antes da correção distribuídos em
Os logs originais não foram encontrados; números globais iguais não
identificam codificações iguais. Este bloqueio permanece histórico; somente
a baseline oficial atual pode ser declarada concluída.

# Fase 3.2.3 — classificação de rejeições immediate (2026-11-26)

Das 880 rejeições LLVM do corpus de registradores, 203 são `addsub_ext`
(por exemplo `2B227D57`, `imm3=7`), 246 `addsub_shift` (`2B00FC00`,
`imm6=63` com `sf=0`), 292 `log_shift` (`6A00FC00`, `imm6=63` com `sf=0`)
e 139 `bitfield` (`333F0000`, `immr=63` com `sf=0`). Essas codificações não
foram contadas como evidência positiva do LLVM. Restrições sobre extensão
(`imm3 > 4`), limite de shift de 32 bits e `immr/imms` de 32 bits precisam
de auditoria de validade do catálogo separada da comparação de operandos.
No corpus novo de immediates, 9.176 codificações aceitas foram rejeitadas
pelo LLVM (incluindo combinações inválidas de `immr/imms` de 32 bits).
`DecodeBitMasks` de logical-immediate pertence à Fase 3.2.4. A classificação
LLVM-invalid NÃO é prova de que Armito esteja arquiteturalmente correto nem
uma desculpa para ampliar artificialmente o conjunto verificado.

# Fase 3.2.2 — encerramento do bloqueio de registradores (2026-11-26)

As 67 divergências anteriores foram preservadas no relatório CSV e agora
passam como regressões. Das 28 formas sem comparação positiva, 24 envolviam
campos implícitos/aliases e 4 exigiam `+cpa` no LLVM MC 18.1.3; as 195 agora
têm verificação positiva de registradores explícitos. Há 880 codificações
aceitas pelo catálogo e rejeitadas pelo LLVM neste corpus: não são contadas
como validadas pelo LLVM, nem prova geral de invalidade do catálogo. Não
existem divergências de registrador injustificadas no corpus verificado.
Fase 3.2 integral permanece bloqueada por classes de operandos não testadas.

# Pendências do diferencial Integer (histórico anterior, 2026-11-26)

`make -B test` e `make test` falham no novo teste por forma: 67 divergências
de registradores em codificações aceitas pelo catálogo; 28 formas sem
comparação positiva de registradores LLVM. O teste distingue estas categorias
e retorna falha, sem normalizar diferenças SP/XZR ou WSP/WZR. Os 195 COMPLETE
da auditoria anterior NÃO estão independentemente validados. Comparações
de immediates, máscaras, aliases, implícitos e codificações inválidas ainda
faltam. `make catalog-test` e builds EDK2 GCC X64 DEBUG/RELEASE passaram.

# Failure Model

## 1. Autoridade única

`fail.c` é a autoridade responsável por detectar falhas em Armito.

Essa decisão evita que Core, Decoder, ArmitoBoot e outros componentes
mantenham detectores paralelos com regras divergentes.

## 2. Separação de responsabilidades

  -----------------------------------------------------------------------
  Componente                          Responsabilidade
  ----------------------------------- -----------------------------------
  `fail.c`                            Detectar e determinar que uma falha
                                      ocorreu

  Core Diagnostics                    Receber, encaminhar e apresentar
                                      informações da falha

  `Display.c`                         Exibir a informação quando o
                                      dispositivo de saída estiver
                                      disponível

  `LogGen.c`                          Registrar a informação no mecanismo
                                      de log

  Core                                Coordenar o fluxo e o encerramento
                                      seguro quando necessário

  Subsistema de origem                Retornar estado e contexto
                                      suficientes para a autoridade de
                                      falhas
  -----------------------------------------------------------------------

Core Diagnostics não deve reimplementar a detecção nem competir com
`fail.c`.

## 3. Fluxo conceitual

``` text
Subsystem / ArmitoBoot / Decoder
              |
              v
           fail.c
              |
              v
       Failure information
              |
        +-----+-----+
        |           |
        v           v
    Display.c    LogGen.c
        ^           ^
        +-----+-----+
              |
       Core Diagnostics
       (adapter / routing)
```

O fluxo exato de chamadas deverá respeitar as interfaces reais de
`fail.c`, `Display.c` e `LogGen.c`. O diagrama representa
responsabilidades, não uma API já implementada.

## 4. Dados de uma falha

A estrutura de informação de falha deve permitir, conforme aplicável: -
identificador ou código; - severidade; - componente de origem; - etapa
do ciclo de vida; - endereço ou contexto técnico, se seguro e
disponível; - descrição curta; - estado recuperável ou fatal; - ação
tomada.

Não incluir dados não inicializados ou ponteiros cujo tempo de vida não
esteja garantido.

## 5. Severidade e resposta

A política de severidade deve distinguir pelo menos: - informativa; -
aviso; - erro recuperável; - erro fatal.

A classificação final deve pertencer ao contrato do sistema de falhas,
não ser reinventada por cada subsistema.

## 6. Falha de apresentação

A ausência de Display ou de um dispositivo gráfico não pode impedir o
registro da falha. Da mesma forma, falha no logger não deve provocar
recursão infinita no próprio sistema de falhas.

O sistema deve definir fallback seguro, por exemplo: - log serial,
quando disponível; - buffer de diagnóstico em memória previamente
reservado; - encerramento controlado, quando não houver canal confiável.

Esses fallbacks são requisitos de projeto a implementar e validar.

## 7. Recursão e falhas secundárias

O tratamento de uma falha pode falhar. O mecanismo precisa impedir: -
reentrada infinita; - alocação dinâmica não segura durante falha; -
dependência circular entre fail, Display e LogGen; - perda da falha
original por uma falha secundária.

## 8. Regras obrigatórias

1.  `fail.c` permanece como detector único.
2.  Core Diagnostics apenas instrumentaliza apresentação e
    encaminhamento.
3.  Display e LogGen não assumem detecção global.
4.  A falha original deve ser preservada.
5.  Falhas durante boot devem ter caminho mínimo de diagnóstico.
6.  Ausência de GOP não pode ser tratada automaticamente como ausência
    de qualquer saída.
7.  O sistema deve conseguir encerrar com segurança quando a recuperação
    não for possível.

## 9. Integração pendente

-   Mapear a API real de `fail.c`.
-   Mapear a API real de `Display.c`.
-   Mapear a API real de `LogGen.c`.
-   Definir estrutura comum de falha.
-   Implementar adaptador no Core Diagnostics.
-   Testar falhas antes e depois da inicialização do Core.
-   Testar ausência de display, falha de log e falha durante o próprio
    diagnóstico.

## 10. Falhas de autenticação ARM64E

Uma assinatura PAC inválida é uma falha arquitetural do estado convidado,
não uma falha global do Core. A API PAC registra a falha e invalida o
resultado; helpers de branch e return autenticados também registram uma
exceção no estado AArch64. Falhas de configuração, chave indisponível ou
provedor são retornadas como erros da API, não convertidas silenciosamente
em sucesso. Ainda não há executor que encaminhe essas exceções ao
`fail.c` ou implemente a política de trap de um runtime.

## 11. Limites do catálogo e Decoder AArch64

O catálogo fixado fornece padrões de bits, campos, grupos e AST conditions;
ele não é por si só um decoder semântico completo. A implementação C usa
índice estático de 64 buckets, precedência por especificidade e avalia
`should_be_mask`. `PATTERN_MATCHED_UNVALIDATED` é uma correspondência de
bits, não uma afirmação de instrução válida.

As AST conditions aplicáveis a guards são compiladas para bytecode C
estático e avaliadas sem interpretar JSON ou strings. As 599 conditions
são avaliáveis, incluindo `BFXPreferred`, `MoveWidePreferred` e
`SVEMoveMaskPreferred` para seleção de aliases. O perfil é o da CPU guest
e não deriva capacidades da CPU hospedeira. Quando uma expressão
necessária não pode ser avaliada, o resultado permanece
`PATTERN_MATCHED_UNVALIDATED`. Os estados de match bruto, válido,
reservado, inválido e extensão indisponível são separados.

Os testes verificam amostras positivas e negativas para os 3.099 padrões,
bit flips e variações determinísticas, comparando lookup indexado com
matcher linear: 59.103 lookups e 104.702 checks sem divergências. O
conjunto diferencial independente inclui 33 codificações LLVM MC, incluindo
PAC e LSE, mas não valida todas as codificações ou extensões opcionais. As
177 sobreposições potenciais são ordenadas por especificidade, condições
avaliáveis e features; testes estruturais não percorrem todo o produto
cartesiano dos campos nem provam a semântica de execução.

Core inicializa as tabelas e registra o extrator; ArmitoBoot inicializa
Core sem decodificar bytes no boot. Os builds EDK2 DEBUG/RELEASE incluem
as tabelas e o extrator no EFI, mas não houve execução do catálogo em
firmware real. Isso permanece um limite para declarar validação runtime
no caminho UEFI.

## 12. Extração universal de operandos

Fase 3.2: o conjunto Integer/Data Processing (195 formas/112 operações)
retorna COMPLETE para um testemunho validado por forma. A cobertura
diferencial independente compara só seis palavras e registradores/modificadores;
não abrange todos os immediates, bitfields, flags NZCV, aliases e operandos
implícitos. Não usar a contagem 195/195 como prova de que a implementação
atende ao critério de conclusão: formas com campos dependentes de estado
ou múltiplos valores ainda exigem validação independente sistemática.
Foram adicionados testes de máscaras de bitfield para casos representativos,
mas não uma comparação independente das 195 formas em seus extremos.

Os 4.296 forms têm descritores estruturais (45.360 no total), mas a
extração semântica permanece incompleta: 3.093/3.099 amostras
representativas de padrões foram extraídas, alcançando 3.026 forms e
1.478/2.095 IDs de operação; os resultados podem ser parciais/raw. O teste
estrutural não prova que todos os valores e combinações de operandos de
cada forma sejam interpretados.

SIMD/NEON, SVE, SME, lanes, listas de registradores e parte dos modos de
endereçamento e operandos de sistema permanecem parciais ou não suportados.
Os diferenciais LLVM MC existentes comparam 33 codificações e mnemonics,
não operandos de todas as formas. Operações sem extração completa devem
permanecer marcadas como parciais/não suportadas; isso não pode ser
convertido em suporte semântico ou em instrução NOP.

Auditoria de 2026-11-26: as seis amostras de padrões sem extração são
`0x2F00FC00` (FCVTZU: `immh == 0`, vedado pelo guard), `0x0420C000`,
`0x0420C800`, `0x0430C000`, `0x0430C400` (formas SVE INC/DEC: `size == 0`,
enquanto as variantes exigem valores específicos) e `0x00000000` (UDF,
classificado como padrão reservado). A rejeição dessas palavras não é falha
do extrator: os cinco primeiros resultados são INVALID_ENCODING e o último
RESERVED_PATTERN. Gerar palavras por máscara de padrão isoladamente não
seleciona uma forma válida.

Das 1.270 formas não alcançadas anteriormente, a auditoria limitada a 96
candidatos alcançou 1.262 adicionais. Restam sete formas de branch/return
autenticado ou comum sob os padrões 565/566 (`BLRAAZ`, `BLRABZ`, `BLR`,
`BRAAZ`, `BRABZ`, `RETAA`, `RETAB`) cuja seleção por lookup é obscurecida
por prioridade/sobreposição e a forma reservada UDF sob o padrão 2598.
Não se concluiu que as sete sejam impossíveis: faltam testemunhas
selecionadas por forma ou validação independente. Estados medidos: 0 formas
completas, 4.255 parciais, 33 raw, 0 não suportadas, 8 não alcançadas.
`AARCH64_OPERAND_EXTRACTION_COMPLETE` não foi atribuído a resultados
parciais; GPR sem largura, campos de sistema raw e operandos vetoriais
requerem interpretação arquitetural adicional. A Fase 3 permanece aberta.
