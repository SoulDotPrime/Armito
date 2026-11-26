# Relatório de fechamento do Core — 25/11/2026

**Repositório:** SoulDotPrime/Armito
**Base:** commit cb10763 (v0.0.1) → commit 5ee520a
**Escopo:** fechar o `Core/` inicial conforme a auditoria de 25/11/2026
**Verificação:** `make test` → **74 verificações, 0 falhas**

---

## 1. O que foi corrigido

| # | Defeito da auditoria | Situação |
|---|---|---|
| A | Core não reinicializava após shutdown (`ConfigShutdown` nunca chamado) | **corrigido** |
| B | `InitPrepare` falhava na segunda vez (Registry/Dependency já inicializados) | **corrigido** |
| C | Rollback inexistente em `InitExecute` / `InitStart` | **corrigido** |
| D | `ArmitoCoreInitialize` após `Start` devolvia ALREADY em vez de estado explícito | **corrigido** (máquina de estados) |
| E | Core sem módulos não subia (`NOT_INITIALIZED`) | **corrigido** |
| F | Core não compilava (3 caminhos de `#include` errados) | **corrigido** |
| G | Diagnostics sem handler, log engolido pela falha do display | pendente (Fase 05) |
| H | IPC: enviava para módulo parado, auto-envio, `memcpy` | pendente (Fase 03) |
| I | Scheduler não libera slots | pendente (Fase 03) |
| J | `Dependency.Resolve` supunha IDs sequenciais | **corrigido** |
| K | `Reset()` público sem trava | pendente (Fase 03) |
| L | `Start` sem rollback dos subsistemas opcionais | **corrigido** |
| M | Limites duplicados em cada arquivo | **corrigido** |

---

## 2. Decisões tomadas nesta etapa

### D-014 — Build de host obrigatório

Todo o `Core/` compila e roda em Linux/macOS com `make`, sem EDK2.
A suíte `Tests/Core/CoreTest.c` é a autoridade de regressão do Core.
O alvo EDK2 continua separado (`Armito.inf`), ainda sem o Core na lista.

### D-015 — Máquina de estados explícita do Core

`ArmitoCoreState`: `UNINITIALIZED → READY → RUNNING → FAILED/STOPPED`.
- `Initialize` só é válido em `UNINITIALIZED` e `STOPPED`.
- `RegisterModule`/`RegisterDependency` só em `READY`.
- `Start` só em `READY`; em `FAILED` exige `Initialize` de novo.
- `GetStatus()` passa a existir na API pública.

### D-016 — Rollback é responsabilidade do Init

`InitExecute` e `InitStart` desfazem o que já subiu:
marcam o módulo que falhou com `INITIALIZATION_FAILED` ou
`EXECUTION_FAILED` e chamam `shutdown()` dos anteriores na ordem inversa.
Um módulo é desligado exatamente uma vez.

### D-017 — IDs de módulo não são sequenciais

`Registry` expõe `GetAt(index)`; `Dependency` resolve por índice,
sem varrer `1..count`. IDs esparsos passam a ser válidos.

### D-018 — Limites centralizados

`CoreConfig.h` é a única fonte de `ARMITO_CORE_MAX_*`.
Nenhum subsistema redefine limite próprio.

---

## 3. Evidência de execução

```
$ make test
Armito Core - testes de host

74 verificacoes, 0 falhas
```

Cenários cobertos: ciclo básico, reinicialização, Core vazio,
erros de estado, rollback de initialize, rollback de start,
ordem de dependências, ciclo de dependências, descritor inválido,
bloqueio de configuração, validação de configuração.

---

## 4. O que ainda não está fechado

1. `fail.c`, `Display.c` e `LogGen.c` continuam **vazios** — Fase 05.
2. IPC sem sincronização, sem política de estado do destinatário e com
   `memcpy` (incompatível com freestanding) — Fase 03.
3. Scheduler sem liberação de slot — Fase 03.
4. `Armito.inf` não inclui `Core/` — a integração EDK2 é Fase 04.
5. `Decoder/`, `IR/` e `CPUToCPU/` são pastas vazias — Fases 07–09.

---

## 5. Próximo passo

**Fase 07 — contrato do Decoder e do Universal IR**, já com a topologia
definida pelo responsável do projeto:

```
Decoder/            decodificador (AArch64 Apple)
IR/                 Universal Intermediate Representation
CPUToCPU/AMD/FX/8320E/   backend de destino
```

Direção: **Mac mini M2 (ARM64 Apple) → FX-8320E (x86_64)**.
