#define NOB_WARN_DEPRECATED
#define NOB_EXPERIMENTAL_DELETE_OLD

#define BUILD_FOLDER "build"

#include "nob.h"

//--------------------------------------------------------------------------------
int main(int argc, char **argv)
{
    NOB_GO_REBUILD_URSELF(argc, argv);

    _Bool const folders_created = mkdir_if_not_exists(BUILD_FOLDER);
    if (!folders_created)
    {
        return EXIT_FAILURE;
    }

    Cmd cmd = { 0 };
    Procs procs = { 0 };

    struct
    {
        char const *input;
        char const *output;
        char const *flag;
    } targets[] = {
        { .input = "examples/fib.c", .output = (BUILD_FOLDER "/fib_enable"), .flag = "-DSIMPRO_ENABLE" },
        { .input = "examples/fib.c", .output = (BUILD_FOLDER "/fib_disable"), .flag = "-DSIMPRO_DISABLE" },
    };
    for (size_t i = 0; i < ARRAY_LEN(targets); ++i)
    {
        // clang-format off
        cmd_append(&cmd,
            "cc", targets[i].input, "-o", targets[i].output, targets[i].flag,
            "-std=c99", "-Wall", "-Wextra", "-Wconversion", "-Wpedantic", "-ggdb", "-O2"
        );
        // clang-format on
        cmd_run(&cmd, .async = &procs);
    }

    bool result = nob_procs_flush(&procs);
    if (!result)
    {
        goto CLEANUP;
    }

CLEANUP:
    da_free(procs);
    cmd_free(cmd);
    return result ? EXIT_SUCCESS : EXIT_FAILURE;
}

//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
#define NOB_IMPLEMENTATION
#include "nob.h"
