/**
 * Standalone host test: start ETS VM and run hello.abc
 * Cross-compiled for ARM64, runs under QEMU user-mode.
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "hongengine_c_api.h"

int main(int argc, char *argv[]) {
    const char *filesDir = (argc > 1) ? argv[1] : "/tmp/hongengine_test";
    const char *abcPath = (argc > 2) ? argv[2] : nullptr;

    printf("=== HongEngine Host Test ===\n");
    printf("filesDir: %s\n", filesDir);
    printf("abcPath:  %s\n", abcPath ? abcPath : "(none)");

    // Step 1: Create engine state
    HongEngineState *state = hongengine_state_create(filesDir);
    if (!state) {
        fprintf(stderr, "FAIL: hongengine_state_create returned NULL\n");
        return 1;
    }
    printf("OK: state created\n");

    // Step 2: Find a test .abc file
    char testAbc[1024];
    if (abcPath) {
        snprintf(testAbc, sizeof(testAbc), "%s", abcPath);
    } else {
        snprintf(testAbc, sizeof(testAbc), "%s/test/hello.abc", filesDir);
    }
    printf("Using ABC: %s\n", testAbc);

    // Step 3: Validate ABC
    HongAbcInfo info = hongengine_validate_abc(testAbc);
    printf("ABC validation: valid=%d magic=%s version=%s fileSize=%u numClasses=%u\n",
           info.valid, info.magic, info.version, info.file_size, info.num_classes);
    if (!info.valid) {
        fprintf(stderr, "FAIL: invalid ABC: %s\n", info.error ? info.error : "unknown");
        hongengine_state_destroy(state);
        return 1;
    }

    // Step 4: Create ETS runtime (full init mode with stdlib)
    char stdlibAbc[1024];
    snprintf(stdlibAbc, sizeof(stdlibAbc), "%s/test/etsstdlib.abc", filesDir);
    printf("Using stdlib: %s\n", stdlibAbc);

    HongAbcInfo stdlibInfo = hongengine_validate_abc(stdlibAbc);
    printf("Stdlib validation: valid=%d magic=%s version=%s fileSize=%u numClasses=%u\n",
           stdlibInfo.valid, stdlibInfo.magic, stdlibInfo.version,
           stdlibInfo.file_size, stdlibInfo.num_classes);
    if (!stdlibInfo.valid) {
        fprintf(stderr, "FAIL: invalid stdlib ABC: %s\n", stdlibInfo.error ? stdlibInfo.error : "unknown");
        hongengine_state_destroy(state);
        return 1;
    }

    HongEngineError err = hongengine_ets_create_runtime(state, testAbc, stdlibAbc);
    if (err != HONGENGINE_OK) {
        const char *detail = hongengine_ets_get_last_error(state);
        fprintf(stderr, "FAIL: runtime creation failed: code=%d detail=%s\n", err, detail ? detail : "?");
        hongengine_state_destroy(state);
        return 1;
    }
    printf("OK: runtime created\n");

    // Step 5: Execute module
    printf("Executing module...\n");
    HongEtsResult result = hongengine_ets_execute_module(state, nullptr);
    if (result.success) {
        printf("SUCCESS: exit_code=%d\n", result.exit_code);
    } else {
        fprintf(stderr, "FAIL: execution failed: exit_code=%d error=%s\n",
                result.exit_code, result.error ? result.error : "?");
        hongengine_ets_destroy_runtime(state);
        hongengine_state_destroy(state);
        return 1;
    }

    // Step 6: Cleanup
    printf("Cleaning up...\n");
    hongengine_ets_destroy_runtime(state);
    hongengine_state_destroy(state);
    printf("DONE\n");
    return result.exit_code;
}
