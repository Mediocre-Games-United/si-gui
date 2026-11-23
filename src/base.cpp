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
#include "ui_progressbar.hpp"

// #define INSTALL_FAKE

static fpath installpkg;
static si::InstallPackage pkg = si::InstallPackage::blank();

class ExtractProgress : public UIBase {
public:
    ExtractProgress(ArchiveTarXZ &astream,fpath tpath) : install_path(tpath), pkg_xz(astream) {
        list = pkg_xz.get_entries();
        list_max = list.size();
        if (std::filesystem::exists(install_path)) std::filesystem::remove_all(install_path);
        std::filesystem::create_directories(install_path);

        prog = new UIProgressBar();
        prog->set_progress(0,1);

        prog->size = scale2(.7,0,0,12);
        prog->anchor = vec(.5,0);
        prog->position = scale2(.5,.5,0,2);

        txt = new UIText("",&medium_font);
        txt->size = scale2(.7,0,0,24);
        txt->anchor = vec(.5,1);
        // txt->text_anchor = vec(1,.5);
        txt->position = scale2(.5,.5,0,-2);

        add_object(prog);
        add_object(txt);
    }
    void ui_frame(F64 dt) override {
        if (list.empty()) {
            cbu::log_verbose("List empty!");
            txt->set_text("");
            prog->set_progress(1,1);
        } else {
            fpath et = list.back();
            list.pop_back();

            BYTEARRAY stream = pkg_xz.get_entry(et);
            fpath tpath = install_path / et;
            cbu::write_to_file_safe_binary(tpath,stream);

            SZ i = list_max - list.size();
            F64 prc = (F64(i) / F64(list_max)) * 100.0;

            txt->set_text(std::vformat(std::string(get_text("installer/progress")),std::make_format_args(et,prc)));
            prog->set_progress(i,list_max);
        }

        update_layer();
    }

    UIText *txt;
    UIProgressBar *prog;
    fpath install_path;
    vector<fpath> list;
    SZ list_max;
    ArchiveTarXZ &pkg_xz;
};
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
            if (stage == 0) {
                title->position = scale2::from_scale(0,0);
                title->size = scale2::from_scale(1,.2);
                title->anchor = vec::zero();

                btns_hbox->position = scale2::from_scale(0,1);
                btns_hbox->size = scale2::from_scale(1,.2);
                btns_hbox->anchor = vec(0,1);

                // #ifdef _WIN32
                // const char* value = std::getenv("LOCALAPPDATA");
                // #else
                // const char* value = std::getenv("HOME");
                // #endif
                // if (!value) throw std::runtime_error("Could not get install path");
                // #ifdef _WIN32
                // fpath install_path = fpath(value) / pkg.name;
                // #else
                // fpath install_path = fpath(value) / "Applications" / pkg.name;
                // #endif

                install_path = cbu::get_local_install_path(pkg.name);

                UIVBoxLayout *vbox = new UIVBoxLayout();
                UIText *opt = new UIText(get_text("installer/thisuser"),&medium_font);
                UIText *path = new UIText(std::vformat(std::string(get_text("installer/path")),std::make_format_args(install_path)),&medium_font);

                vbox->add_object(opt);
                vbox->add_object(path);
                content->add_object(vbox);

                update();
                stage = 1;
                return;
            } if (stage == 1) {
                queue_call([this]() {
                    btns_hbox->clear_objects();
                    update();
                });
                content->clear_objects();
                bg->delete_object(title);

                ExtractProgress *pgr = new ExtractProgress(pkg.get_pkg(),install_path);

                content->add_object(pgr);
                update();
                stage = 2;
                return;
            }
        });
        cancel->activated.add_callback([]() {
            engine_quit(1);
        });

        content->quick_set_centered(scale2::from_scale(1,.6));

        btns_hbox->add_object(cont_btn);
        btns_hbox->add_object(cancel);

        bg->add_object(content);
        bg->add_object(title);
        bg->add_object(btns_hbox);

        add_object(bg);

        update();
    }

    fpath install_path;
    SZ stage = 0;
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
        cbu::log_error(false,std::format("FATAL: Failed to load package: {}",err.what()));

        return true;
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
