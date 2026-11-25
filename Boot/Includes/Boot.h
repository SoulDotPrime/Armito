#ifndef ARMITO_BOOT_H
#define ARMITO_BOOT_H

#include <Uefi.h>

EFI_STATUS EFIAPI ArmitoBoot(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable);

#endif