#include <stdio.h>
#include <stdint.h>

#include "Includes/Core.h"
#include "Config/Includes/CoreConfig.h"
#include "Init/Includes/CoreInit.h"
#include "Registry/Includes/CoreRegistry.h"
#include "Dependency/Includes/CoreDependency.h"
#include "Decoder/Includes/Decoder.h"


static uint32_t tests_run = 0;
static uint32_t tests_failed = 0;
static const char *current_test = "";

#define CHECK(cond) do { \
    tests_run++; \
    if (!(cond)) { \
        tests_failed++; \
        printf("  FAIL  %s (%s:%d)\n", current_test, __FILE__, __LINE__); \
    } \
} while (0)

#define CHECK_STATUS(expr, expected) do { \
    ArmitoCoreStatus s_ = (ArmitoCoreStatus)(expr); \
    ArmitoCoreStatus e_ = (ArmitoCoreStatus)(expected); \
    tests_run++; \
    if (s_ != e_) { \
        tests_failed++; \
        printf("  FAIL  %s: %s devolveu %d, esperado %d (%s:%d)\n", \
            current_test, #expr, (int)s_, (int)e_, __FILE__, __LINE__); \
    } \
} while (0)

static uint32_t mod_init_calls = 0;
static uint32_t mod_start_calls = 0;
static uint32_t mod_shutdown_calls = 0;

static ArmitoCoreStatus ModInit(void)
{
    mod_init_calls++;
    return ARMITO_CORE_SUCCESS;
}

static ArmitoCoreStatus ModStart(void)
{
    mod_start_calls++;
    return ARMITO_CORE_SUCCESS;
}

static void ModShutdown(void)
{
    mod_shutdown_calls++;
}

static ArmitoCoreStatus ModInitFail(void)
{
    return ARMITO_CORE_ERROR;
}

static ArmitoCoreStatus ModStartFail(void)
{
    return ARMITO_CORE_EXECUTION_FAILED;
}

static void ResetCounters(void)
{
    mod_init_calls = 0;
    mod_start_calls = 0;
    mod_shutdown_calls = 0;
}

static void TestLifecycle(void)
{
    current_test = "ciclo de vida basico";
    ResetCounters();

    ArmitoDecoderStats decoder_stats;
    ArmitoCoreStatus status = ArmitoCoreInitialize();
    CHECK_STATUS(status, ARMITO_CORE_SUCCESS);
    CHECK(ArmitoCoreGetStatus() == ARMITO_CORE_READY);
    CHECK(ArmitoDecoderGetState() == ARMITO_DECODER_READY);

    ArmitoModuleDescriptor module;
    module.id = ARMITO_MODULE_ID_INVALID;
    module.name = "modulo";
    module.priority = ARMITO_PRIORITY_NORMAL;
    module.initialize = ModInit;
    module.start = ModStart;
    module.shutdown = ModShutdown;

    ArmitoModuleID id = ARMITO_MODULE_ID_INVALID;
    CHECK_STATUS(ArmitoCoreRegisterModule(&module, &id), ARMITO_CORE_SUCCESS);
    CHECK(id != ARMITO_MODULE_ID_INVALID);

    CHECK_STATUS(ArmitoCoreStart(), ARMITO_CORE_SUCCESS);
    CHECK(ArmitoCoreGetStatus() == ARMITO_CORE_RUNNING);
    CHECK_STATUS(
        ArmitoDecoderGetStats(&decoder_stats),
        ARMITO_DECODER_SUCCESS
    );
    CHECK(decoder_stats.instructions_seen == 0 &&
          decoder_stats.instructions_decoded == 0);
    CHECK(mod_init_calls == 1);
    CHECK(mod_start_calls == 1);
    CHECK(ArmitoCoreGetModuleState(id) == ARMITO_MODULE_RUNNING);

    ArmitoCoreShutdown();
    CHECK(ArmitoCoreGetStatus() == ARMITO_CORE_STOPPED);
    CHECK(ArmitoDecoderGetState() == ARMITO_DECODER_UNINITIALIZED);
    CHECK(mod_shutdown_calls == 1);
}

static void TestRestart(void)
{
    current_test = "reinicializacao apos shutdown";
    ResetCounters();

    CHECK_STATUS(ArmitoCoreInitialize(), ARMITO_CORE_SUCCESS);

    ArmitoModuleDescriptor module;
    module.id = ARMITO_MODULE_ID_INVALID;
    module.name = "modulo";
    module.priority = ARMITO_PRIORITY_NORMAL;
    module.initialize = ModInit;
    module.start = ModStart;
    module.shutdown = ModShutdown;

    ArmitoModuleID id = ARMITO_MODULE_ID_INVALID;
    CHECK_STATUS(ArmitoCoreRegisterModule(&module, &id), ARMITO_CORE_SUCCESS);
    CHECK_STATUS(ArmitoCoreStart(), ARMITO_CORE_SUCCESS);
    ArmitoCoreShutdown();

    CHECK_STATUS(ArmitoCoreInitialize(), ARMITO_CORE_SUCCESS);
    CHECK_STATUS(ArmitoCoreRegisterModule(&module, &id), ARMITO_CORE_SUCCESS);
    CHECK_STATUS(ArmitoCoreStart(), ARMITO_CORE_SUCCESS);
    CHECK(mod_init_calls == 2);
    CHECK(mod_start_calls == 2);
    ArmitoCoreShutdown();
    CHECK(mod_shutdown_calls == 2);
}

static void TestEmptyCore(void)
{
    current_test = "core sem modulos";

    CHECK_STATUS(ArmitoCoreInitialize(), ARMITO_CORE_SUCCESS);
    CHECK_STATUS(ArmitoCoreStart(), ARMITO_CORE_SUCCESS);
    CHECK(ArmitoCoreGetStatus() == ARMITO_CORE_RUNNING);
    ArmitoCoreShutdown();
}

static void TestStateErrors(void)
{
    current_test = "erros de estado";

    CHECK_STATUS(ArmitoCoreStart(), ARMITO_CORE_NOT_INITIALIZED);

    CHECK_STATUS(ArmitoCoreInitialize(), ARMITO_CORE_SUCCESS);
    CHECK_STATUS(ArmitoCoreInitialize(), ARMITO_CORE_ALREADY_INITIALIZED);

    ArmitoModuleDescriptor module;
    module.id = ARMITO_MODULE_ID_INVALID;
    module.name = "modulo";
    module.priority = ARMITO_PRIORITY_NORMAL;
    module.initialize = ModInit;
    module.start = ModStart;
    module.shutdown = ModShutdown;

    ArmitoModuleID id = ARMITO_MODULE_ID_INVALID;
    CHECK_STATUS(ArmitoCoreRegisterModule(&module, &id), ARMITO_CORE_SUCCESS);
    CHECK_STATUS(ArmitoCoreStart(), ARMITO_CORE_SUCCESS);
    CHECK_STATUS(ArmitoCoreStart(), ARMITO_CORE_ALREADY_INITIALIZED);
    CHECK_STATUS(ArmitoCoreInitialize(), ARMITO_CORE_ALREADY_INITIALIZED);
    CHECK_STATUS(ArmitoCoreRegisterModule(&module, &id), ARMITO_CORE_INVALID_STATE);

    ArmitoCoreShutdown();
}

static void TestRollbackOnInitFailure(void)
{
    current_test = "rollback: falha no initialize";

    CHECK_STATUS(ArmitoCoreInitialize(), ARMITO_CORE_SUCCESS);

    ArmitoModuleDescriptor ok;
    ok.id = ARMITO_MODULE_ID_INVALID;
    ok.name = "ok";
    ok.priority = ARMITO_PRIORITY_NORMAL;
    ok.initialize = ModInit;
    ok.start = ModStart;
    ok.shutdown = ModShutdown;

    ArmitoModuleDescriptor bad;
    bad.id = ARMITO_MODULE_ID_INVALID;
    bad.name = "bad";
    bad.priority = ARMITO_PRIORITY_NORMAL;
    bad.initialize = ModInitFail;
    bad.start = ModStart;
    bad.shutdown = ModShutdown;

    ArmitoModuleID id_ok = ARMITO_MODULE_ID_INVALID;
    ArmitoModuleID id_bad = ARMITO_MODULE_ID_INVALID;
    CHECK_STATUS(ArmitoCoreRegisterModule(&ok, &id_ok), ARMITO_CORE_SUCCESS);
    CHECK_STATUS(ArmitoCoreRegisterModule(&bad, &id_bad), ARMITO_CORE_SUCCESS);

    ResetCounters();
    CHECK_STATUS(ArmitoCoreStart(), ARMITO_CORE_INITIALIZATION_FAILED);

    CHECK(ArmitoCoreGetModuleState(id_ok) == ARMITO_MODULE_STOPPED);
    CHECK(ArmitoCoreGetModuleState(id_bad) == ARMITO_MODULE_INITIALIZATION_FAILED);
    CHECK(mod_shutdown_calls == 1);
    CHECK(ArmitoCoreGetStatus() == ARMITO_CORE_FAILED);

    ArmitoCoreShutdown();
}

static void TestRollbackOnStartFailure(void)
{
    current_test = "rollback: falha no start";

    CHECK_STATUS(ArmitoCoreInitialize(), ARMITO_CORE_SUCCESS);

    ArmitoModuleDescriptor ok;
    ok.id = ARMITO_MODULE_ID_INVALID;
    ok.name = "ok";
    ok.priority = ARMITO_PRIORITY_NORMAL;
    ok.initialize = ModInit;
    ok.start = ModStart;
    ok.shutdown = ModShutdown;

    ArmitoModuleDescriptor bad;
    bad.id = ARMITO_MODULE_ID_INVALID;
    bad.name = "bad";
    bad.priority = ARMITO_PRIORITY_NORMAL;
    bad.initialize = ModInit;
    bad.start = ModStartFail;
    bad.shutdown = ModShutdown;

    ArmitoModuleID id_ok = ARMITO_MODULE_ID_INVALID;
    ArmitoModuleID id_bad = ARMITO_MODULE_ID_INVALID;
    CHECK_STATUS(ArmitoCoreRegisterModule(&ok, &id_ok), ARMITO_CORE_SUCCESS);
    CHECK_STATUS(ArmitoCoreRegisterModule(&bad, &id_bad), ARMITO_CORE_SUCCESS);

    ResetCounters();
    {
        ArmitoCoreStatus st = ArmitoCoreStart();
        CHECK_STATUS(st, ARMITO_CORE_EXECUTION_FAILED);
    }

    CHECK(ArmitoCoreGetModuleState(id_ok) == ARMITO_MODULE_STOPPED);
    CHECK(ArmitoCoreGetModuleState(id_bad) == ARMITO_MODULE_STOPPED);
    CHECK(mod_shutdown_calls == 2);
    CHECK(mod_init_calls == 2);
    CHECK(ArmitoCoreGetStatus() == ARMITO_CORE_FAILED);

    ArmitoCoreShutdown();
}

static void TestDependencyOrder(void)
{
    current_test = "ordem de dependencias";

    CHECK_STATUS(ArmitoCoreInitialize(), ARMITO_CORE_SUCCESS);

    ArmitoModuleDescriptor a;
    a.id = ARMITO_MODULE_ID_INVALID;
    a.name = "a";
    a.priority = ARMITO_PRIORITY_NORMAL;
    a.initialize = ModInit;
    a.start = ModStart;
    a.shutdown = ModShutdown;

    ArmitoModuleDescriptor b = a;
    b.name = "b";

    ArmitoModuleID id_a = ARMITO_MODULE_ID_INVALID;
    ArmitoModuleID id_b = ARMITO_MODULE_ID_INVALID;
    CHECK_STATUS(ArmitoCoreRegisterModule(&a, &id_a), ARMITO_CORE_SUCCESS);
    CHECK_STATUS(ArmitoCoreRegisterModule(&b, &id_b), ARMITO_CORE_SUCCESS);

    CHECK_STATUS(ArmitoCoreRegisterDependency(id_b, id_a), ARMITO_CORE_SUCCESS);
    CHECK_STATUS(ArmitoCoreStart(), ARMITO_CORE_SUCCESS);
    ArmitoCoreShutdown();
}

static void TestDependencyCycle(void)
{
    current_test = "ciclo de dependencias";

    CHECK_STATUS(ArmitoCoreInitialize(), ARMITO_CORE_SUCCESS);

    ArmitoModuleDescriptor a;
    a.id = ARMITO_MODULE_ID_INVALID;
    a.name = "a";
    a.priority = ARMITO_PRIORITY_NORMAL;
    a.initialize = ModInit;
    a.start = ModStart;
    a.shutdown = ModShutdown;

    ArmitoModuleDescriptor b = a;
    b.name = "b";

    ArmitoModuleID id_a = ARMITO_MODULE_ID_INVALID;
    ArmitoModuleID id_b = ARMITO_MODULE_ID_INVALID;
    CHECK_STATUS(ArmitoCoreRegisterModule(&a, &id_a), ARMITO_CORE_SUCCESS);
    CHECK_STATUS(ArmitoCoreRegisterModule(&b, &id_b), ARMITO_CORE_SUCCESS);

    CHECK_STATUS(ArmitoCoreRegisterDependency(id_a, id_b), ARMITO_CORE_SUCCESS);
    CHECK_STATUS(ArmitoCoreRegisterDependency(id_b, id_a), ARMITO_CORE_SUCCESS);
    CHECK_STATUS(ArmitoCoreStart(), ARMITO_CORE_DEPENDENCY_ERROR);

    ArmitoCoreShutdown();
}

static void TestRegistryRejectsBadDescriptor(void)
{
    current_test = "registry recusa descritor invalido";

    CHECK_STATUS(ArmitoCoreInitialize(), ARMITO_CORE_SUCCESS);

    ArmitoModuleDescriptor module;
    module.id = ARMITO_MODULE_ID_INVALID;
    module.name = NULL;
    module.priority = ARMITO_PRIORITY_NORMAL;
    module.initialize = ModInit;
    module.start = ModStart;
    module.shutdown = ModShutdown;

    ArmitoModuleID id = ARMITO_MODULE_ID_INVALID;
    CHECK_STATUS(ArmitoCoreRegisterModule(&module, &id), ARMITO_CORE_ERROR);
    CHECK_STATUS(ArmitoCoreRegisterModule(NULL, &id), ARMITO_CORE_ERROR);

    ArmitoCoreShutdown();
}

static void TestConfigLock(void)
{
    current_test = "config bloqueada durante execucao";

    CHECK_STATUS(ArmitoCoreInitialize(), ARMITO_CORE_SUCCESS);

    ArmitoCoreConfig config;
    CHECK_STATUS(ArmitoCoreConfigGet(&config), ARMITO_CORE_SUCCESS);
    CHECK(config.max_modules == 64);

    CHECK_STATUS(ArmitoCoreStart(), ARMITO_CORE_SUCCESS);
    CHECK(ArmitoCoreConfigIsLocked() == 1);
    CHECK_STATUS(ArmitoCoreConfigSet(&config), ARMITO_CORE_INVALID_STATE);

    ArmitoCoreShutdown();
    CHECK_STATUS(ArmitoCoreConfigGet(&config), ARMITO_CORE_NOT_INITIALIZED);
}

static void TestConfigValidation(void)
{
    current_test = "validacao de configuracao";

    CHECK_STATUS(ArmitoCoreInitialize(), ARMITO_CORE_SUCCESS);

    ArmitoCoreConfig config;
    CHECK_STATUS(ArmitoCoreConfigGet(&config), ARMITO_CORE_SUCCESS);

    config.max_modules = 0;
    CHECK_STATUS(ArmitoCoreConfigSet(&config), ARMITO_CORE_CONFIGURATION_ERROR);

    CHECK_STATUS(ArmitoCoreConfigGet(&config), ARMITO_CORE_SUCCESS);
    config.max_ipc_payload = 512;
    CHECK_STATUS(ArmitoCoreConfigSet(&config), ARMITO_CORE_CONFIGURATION_ERROR);

    ArmitoCoreShutdown();
}

int main(void)
{
    printf("Armito Core - testes de host\n\n");

    TestLifecycle();
    TestRestart();
    TestEmptyCore();
    TestStateErrors();
    TestRollbackOnInitFailure();
    TestRollbackOnStartFailure();
    TestDependencyOrder();
    TestDependencyCycle();
    TestRegistryRejectsBadDescriptor();
    TestConfigLock();
    TestConfigValidation();

    printf("\n%d verificacoes, %d falhas\n", tests_run, tests_failed);

    return tests_failed == 0 ? 0 : 1;
}
