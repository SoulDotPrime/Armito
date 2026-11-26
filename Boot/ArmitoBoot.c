#include <Uefi.h>
#include "Boot.h"
#include "../Core/Includes/Core.h"

EFI_STATUS EFIAPI ArmitoBoot(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) 
{
    ArmitoCoreStatus status;

    (void)ImageHandle;
    (void)SystemTable;

    status = ArmitoCoreInitialize();
    if (status != ARMITO_CORE_SUCCESS &&
        status != ARMITO_CORE_ALREADY_INITIALIZED)
        return EFI_ABORTED;

    status = ArmitoCoreStart();
    if (status != ARMITO_CORE_SUCCESS &&
        status != ARMITO_CORE_ALREADY_INITIALIZED) {
        ArmitoCoreShutdown();
        return EFI_ABORTED;
    }

    return EFI_SUCCESS;
}