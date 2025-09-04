#define NOB_IMPLEMENTATION
#define NOB_STRIP_PREFIX
#include "nob.h"

#define BUILD_DIR "build"
#define BIN_PATH BUILD_DIR"/game"
#define SRC_PATH "game.c"

bool build_game(Cmd*cmd) {
    nob_cc(cmd);
    nob_cc_inputs(cmd, SRC_PATH);
    nob_cc_output(cmd, BIN_PATH);
    nob_cc_flags(cmd);
    cmd_append(cmd, "-lraylib", "-lGL", "-lm", "-lpthread", "-ldl", "-lrt", "-lX11");
    return cmd_run_sync_and_reset(cmd);
}

int main(int argc, char** argv) {
    NOB_GO_REBUILD_URSELF(argc, argv);
    Cmd cmd = {0};

    if(!mkdir_if_not_exists(BUILD_DIR)) return 1;
    if(!build_game(&cmd)) return 1;

    if(argc <= 1) return 0;

    const char* arg = shift(argv, argc);

    if(strcmp(arg, "run")) {
        cmd_append(&cmd, BIN_PATH);
        if(!cmd_run_sync_and_reset(&cmd)) return 1;
    }

    return 0;
}
