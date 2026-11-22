#include "file.hpp"
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
#include "install_obj.hpp"

// #define INSTALL_FAKE

static fpath installpkg;
static si::InstallPackage pkg = si::InstallPackage::blank();

class IntroScreen : public UILayer {
public:
    IntroScreen() {
        bg = new UIContainer();
        content = new UIBase();
        UIText *title = new UIText(std::vformat(std::string(get_text("installer/welcome")),std::make_format_args(pkg.readable_name)),&big_font);
        title->position = scale2::from_scale(.5,.25);
        title->anchor = vec::half();
        title->size = scale2::from_scale(1,.5);

        btns_hbox = new UIHBoxLayout();
        cont_btn = new UIButton(get_text("installer/proceed"));
        UIButton *cancel = new UIButton(get_text("generic/back"));

        btns_hbox->position = scale2::from_scale(.5,.75);
        btns_hbox->size = scale2::from_scale(1,.5);
        btns_hbox->anchor = vec::half();

        c_obj = cont_btn->activated.add_callback([title,this]() {
            title->position = scale2::from_scale(0,0);
            title->size = scale2::from_scale(1,.2);
            title->anchor = vec::zero();

            btns_hbox->position = scale2::from_scale(0,1);
            btns_hbox->size = scale2::from_scale(1,.2);
            btns_hbox->anchor = vec(0,1);

            UIText *opt = new UIText(get_text("installer/thisuser"),&medium_font);
            content->add_object(opt);

            update();
            void *o = c_obj;
            queue_call([this,o]() {
                cont_btn->activated.remove_callback(o);
            });
            c_obj = cont_btn->activated.add_callback([this]() {
                content->clear_objects();

                update();

                void *o = c_obj;
                queue_call([this,o]() {
                    cont_btn->activated.remove_callback(o);
                });
            });
        });
        cancel->activated.add_callback([]() {
            engine_quit(1);
        });

        content->quick_set_centered(scale2::from_scale(1,.6));

        bg->add_object(content);
        bg->add_object(title);
        bg->add_object(btns_hbox);
        btns_hbox->add_object(cont_btn);
        btns_hbox->add_object(cancel);
        add_object(bg);

        update();
    }

    void *c_obj;
    UIButton *cont_btn;
    UIHBoxLayout *btns_hbox;
    UIContainer *bg;
    UIBase *content;
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
    installpkg = cbu::resolve_path("res://package.si");
#ifndef INSTALL_FAKE
    if (!std::filesystem::exists(installpkg)) {
        cbu::log_error(false,"FATAL: Could not find install package in resources, quitting");

        return true;
    }

    cbu::log_success("Found install package!");
    try {
        pkg = si::InstallPackage::load(installpkg);
        cbu::log_success("Install package loaded succesfully!");
    } catch (std::runtime_error &err) {
        cbu::log_error(false,std::format("Failed to load package: {}",err.what()));
        pkg = si::InstallPackage::blank();
    }
#else
    fpath folder = cbu::get_exe_path().parent_path();
    fpath apath = folder / "package.tar.xz";

    if (std::filesystem::exists(apath)) std::filesystem::remove(apath);
    if (std::filesystem::exists(installpkg)) std::filesystem::remove(installpkg);
    ArchiveTarXZ pkgarchive{7};
    bool res;
    res = pkgarchive.compress_folder_single(folder,apath);
    if (!res) cbu::log_error(false,"Failed to compress folder");

    res = pkg.set_name("testpkg","Test Package");
    if (!res) cbu::log_error(false,"Failed to set name");
    res = pkg.set_version(si::Version{0,1,0});
    if (!res) cbu::log_error(false,"Failed to set version");
    res = pkg.set_pkg(apath);
    if (!res) cbu::log_error(false,"Failed to set pkg");
    std::filesystem::remove(apath);

    res = pkg.save_to_file(installpkg);
    if (!res) cbu::log_error(false,"Failed to save file");
#endif
    return false;
}
void base_init() {
    engine_init_completed.add_callback(init);
}
