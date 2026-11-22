#include "find_config.hpp"
#include "base.hpp"
#include "canvas.hpp"
#include "engine.hpp"
#include "archive.hpp"
#include "graphics.hpp"
#include <filesystem>
#include <format>
#include "ui_button.hpp"
#include "ui_container.hpp"
#include "ui_text.hpp"
#include "locale.hpp"
#include "ui_layout.hpp"

#define INSTALL_FAKE

static fpath installpkg;
static string pkgname;
static string pkgfullname;

class IntroScreen : public UILayer {
public:
    IntroScreen() {
        UIContainer *bg = new UIContainer();
        UIText *title = new UIText(std::vformat(std::string(get_text("installer/welcome")),std::make_format_args(pkgfullname)),&big_font);
        title->position = scale2::from_scale(.5,.25);
        title->anchor = vec::half();
        title->size = scale2::from_scale(1,.5);

        UIHBoxLayout *btns = new UIHBoxLayout();
        UIButton *cont = new UIButton(get_text("installer/proceed"));
        UIButton *cancel = new UIButton(get_text("generic/back"));

        btns->position = scale2::from_scale(.5,.75);
        btns->size = scale2::from_scale(1,.5);
        btns->anchor = vec::half();

        bg->add_object(title);
        bg->add_object(btns);
        btns->add_object(cont);
        btns->add_object(cancel);
        add_object(bg);

        update();
    }
};
static void init() {
    engine_load_all_modules_sync();
    graphics_set_window_resize(false);
    canvas_set_resize_enabled(false);
    set_canvas_size(INIT_CANVAS_W,INIT_CANVAS_H);
    graphics_set_window_size(INIT_CANVAS_W,INIT_CANVAS_H);

    engine_set_scene(new Scene());
    engine_activate_object(new IntroScreen());
}
bool intercept_main() {
#ifndef INSTALL_FAKE
    installpkg = cbu::resolve_path("res://package.si");

    if (!std::filesystem::exists(installpkg)) {
        cbu::log_error(false,"FATAL: Could not find install package in resources, quitting");

        return true;
    }

    cbu::log_success("Found install package!");
#endif
    return false;
}
void base_init() {
    engine_init_completed.add_callback(init);
}
