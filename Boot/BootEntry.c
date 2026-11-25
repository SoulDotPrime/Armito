#include <Uefi.h>
#include "Boot.h"

EFI_STATUS EFIAPI UefiMain(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) 
{
    return ArmitoBoot(ImageHandle, SystemTable);
}