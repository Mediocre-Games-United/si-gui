#include "base.hpp"
#include "engine.hpp"
#include "archive.hpp"
#include <filesystem>

static fpath installpkg;
static void init() {
    engine_load_all_modules_sync();

}
bool intercept_main() {
    installpkg = cbu::resolve_path("res://package.si");

    if (!std::filesystem::exists(installpkg)) {
        cbu::log_error(false,"FATAL: Could not find install package in resources, quitting");

        return true;
    }

    return false;
}
void base_init() {
    engine_init_completed.add_callback(init);
}
