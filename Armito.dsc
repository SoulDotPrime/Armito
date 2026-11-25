[Defines]
    DSC_SPECIFICATION           =   0x0001001B
    PLATFORM_NAME               =   Armito
    PLATFORM_GUID               =   3A9F72C1-5E84-4D60B713-2C8E91A645F0
    PLATFORM_VERSION            =   0.0.1
    OUTPUT_DIRECTORY            =   Build/Armito
    SUPPORTED_ARCHITECTURES     =   X64
    BUILD_TARGETS               =   DEBUG | RELEASE | NOOPT

[LibraryClasses]
    UefiApplicationEntryPoint|MdePkg/Library/UefiApplicationEntryPoint/UefiApplicationEntryPoint.inf

[Components]
    Armito.inf
