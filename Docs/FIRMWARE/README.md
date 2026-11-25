# Dumps do firmware (Mac mini M2) — leitura e engenharia reversa

Fonte: `/home/gui/Desktop/Armito-Workspace/Firmware/` (485 MB, 14 arquivos).
Ferramentas usadas: `python3` (struct/plistlib) e `llvm-mc --disassemble --triple=aarch64`
(LLVM 18). Nada foi baixado nem instalado.

## 1. Inventário e formatos reais

| Arquivo | Tamanho | Formato identificado |
|---|---|---|
| `kernelcache.img` | 125.337.600 | Mach-O 64 (`0xFEEDFACF`), cputype `0x0100000C` = **CPU_TYPE_ARM64**, subtipo `2` = ARM64E |
| `sptm.img` | 1.327.104 | Mach-O 64 ARM64E |
| `txm.img` | 606.208 | Mach-O 64 ARM64E |
| `ramdisk.img` | 373.293.056 | cabeçalho próprio (`NXSB` em +0x20 = contêiner Apple) |
| `sepfw.im4p` | 6.129.902 | IM4P (payload IMG4), identificador `sepi` |
| `boot-manifest.im4m` | 10.040 | IM4M (manifesto de assinatura, `MANB`) |
| `devicetree.adt` / `original-devicetree.adt` | 279.392 / 295.952 | DeviceTree compilado (magic `0x14`/"#address-cells") |
| `bootargs.bin` | 1.152 | NVRAM de boot-args |
| `trustcache.bin` | 23.316 | trust cache (versão 1, 1 entrada no cabeçalho) |
| `tss-answers.plist` | 24.935 | respostas TSS (assinatura de restauração) |
| `bundle.json` | 194.846 | metadados da restauração |
| `USB-C_HPM,47.bin` | 162.176 | firmware do controlador USB-C (HPM) |

## 2. Cabeçalhos Mach-O (dumps gerados)

`sptm.img.macho.json`, `txm.img.macho.json`, `kernelcache.img.macho.json` — cada um
com `cputype`, `ncmds`, e por segmento `vmaddr`/`vmsize`/`fileoff`/`filesize` e
seções com `addr`/`size`/`fileoff`/`flags`.

- **SPTM** (8 segmentos, 14 seções): `__TEXT_EXEC` em `0xfffffff0270b4000`
  (393 KB de código), `__LATE_CONST` de 0x90000, `__BOOTDATA` de 0x18000.
- **TXM** (6 segmentos, 14 seções): `__TEXT_EXEC` em `0xfffffff017034000`
  (344 KB) **e** `__TEXT_BOOT_EXEC` em `0xfffffff017088000` (32 KB) — o segundo é
  quase todo `udf`, ou seja, área de boot real é mínima.
- **kernelcache** (9 segmentos, 362 load commands): `__PRELINK_TEXT` de
  0xDF4000 (14,6 MB de código de kexts pré-linkado), `__TEXT_EXEC` de 0x3A90000
  (61 MB, XNU), `__PRELINK_INFO` de 0x3C4000.

## 3. Código extraído (`extracted/`)

| Arquivo | Base virtual | Bytes |
|---|---|---|
| `sptm_TEXT_EXEC.bin` | `0xfffffff0270b4000` | 393.216 |
| `sptm_TEXT.bin` | `0xfffffff027004000` | 98.304 |
| `txm_TEXT_EXEC.bin` | `0xfffffff017034000` | 344.064 |
| `txm_TEXT_BOOT_EXEC.bin` | `0xfffffff017088000` | 32.768 |
| `txm_TEXT.bin` | `0xfffffff017004000` | 114.688 |

Endereço de uma instrução no binário: `base + offset_no_arquivo`.
Disassembly completo: `sptm_TEXT_EXEC.dis`, `txm_TEXT_EXEC.dis`,
`txm_TEXT_BOOT_EXEC.dis` (formato `llvm-mc`, um instrução por linha).

## 4. Perfil de instruções (o que o decoder vai encontrar de verdade)

`*.mnemonicos.txt` traz a contagem completa. Resumo:

- **SPTM `__TEXT_EXEC`** — 95.959 instruções, 140 mnemônicos distintos.
  Top: `add` 12.455 · `udf` 10.122 · `mov` 8.874 · `adrp` 8.377 · `ldr` 6.459 ·
  `stp` 5.280 · `bl` 3.623 · `cmp` 3.583 · `str` 3.503 · `ldrb` 2.639 ·
  `b` 2.362 · `nop` 2.253 · `ldp` 1.796 · `cbz` 1.493 · `b.ne` 1.445.
- **TXM `__TEXT_EXEC`** — 83.778 instruções, 141 mnemônicos.
  Top: `mov` 16.160 · `add` 8.811 · `stp` 5.302 · `bl` 5.116 · `ldr` 4.858 ·
  `udf` 4.153 · `ldp` 3.884 · `cmp` 3.609 · `hint` 2.879 · `b` 2.688.
- **TXM `__TEXT_BOOT_EXEC`** — 8.182 instruções, só 33 mnemônicos, **7.934 são
  `udf`**: é uma região de preenchimento/armadilha, não código útil.

## 5. O que isso diz sobre o projeto Arm ito

1. O alvo real é **ARM64E** (variante de ponteiros autenticados), não ARM64 puro:
   o decoder precisa classificar `PACIA`/`PACIB`/`AUTIA`/`AUTIB`/`XPACI` e as
   instruções de sistema usadas (`msr`/`mrs` de `SPSel`, `VBAR_EL1`, `CurrentEL`,
   `HCR_EL2`, `TPIDR_EL1`/`TPIDRRO_EL0`).
2. As primeiras instruções do SPTM são um **prólogo real de boot**:
   `msr SPSel,#0; mov x10,xzr; mov sp,x10; msr SPSel,#1; mov sp,x10; adrp x10;
   add x10,x10,#0; msr VBAR_EL1,x10; mrs x10,CurrentEL; cmp x10,#8; b.ne ...` —
   ótimo caso de teste ponta a ponta para o Decoder (MOV.reg, ADD.imm, B.cond,
   MRS/MSR, ADRP).
3. `ldr`/`str`/`stp`/`ldp`/`ldrb` dominam: sem LOAD/STORE decodificado não há
   como traduzir nem 1% deste firmware. A fase atual do Decoder (que manda
   LOAD/STORE para `UNSUPPORTED`) não serve para este material.
4. `bl` alto (3.6k no SPTM) confirma que o caminho de chamada precisa funcionar.

## 6. Kernel

`kernelcache.img` é o XNU **27.2.0**, `xnu-13432.41.4~25/RELEASE_ARM64_T8112`
(compilado em 14/out/2026, chip T8112 = M2). O `__PRELINK_INFO` é um plist com
**375 kexts** (348 com endereço de carga dentro de `__PRELINK_TEXT`, 27
DriverKit sem código pré-linkado). Lista completa em `kernelcache-kexts.json`
(`bundle`, `exe`, `load_addr`, `size`). Os maiores: `IO80211Family` (3,8 MB),
`AppleAVE2` (2,9 MB), `AppleBCMWLANCore` (2,8 MB), `IOThunderboltFamily` (2,8 MB),
`AppleM2ScalerCSCDriver` (2,7 MB).

## 7. Limites desta análise

- Não há símbolos nem `__LINKEDIT` interpretado: os endereços de função vêm do
  disassembly linear, que se confunde com dados dentro de `__const`/`__cstring`.
- `ramdisk.img`, `sepfw.im4p` e `boot-manifest.im4m` foram identificados pelo
  cabeçalho, não extraídos.
- Nada foi executado; é leitura estática.
