/* ui/qt/gui.cc --
   Written and Copyright (C) 2017-2022 by vmc.

   This file is part of woinc.

   woinc is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   woinc is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with woinc. If not, see <http://www.gnu.org/licenses/>. */

#include "qt/gui.h"

#include <cassert>
#ifndef NDEBUG
#include <iostream>
#endif

#include <QKeyEvent>
#include <QMenuBar>
#include <QMessageBox>

#include <woinc/ui/error.h>

#include "qt/controller.h"
#include "qt/dialogs/about_dialog.h"
#include "qt/dialogs/add_project_wizard.h"
#include "qt/dialogs/preferences_dialog.h"
#include "qt/dialogs/select_computer_dialog.h"
#include "qt/menu.h"
#include "qt/model.h"
#include "qt/tabs_widget.h"

namespace woinc { namespace ui { namespace qt {

void Gui::open(const Model &model, Controller &controller) {
    setWindowTitle("woincqt");

    // TODO should only be set if we don't have these values from some settings/config file
    resize(QSize(1000, 600));

    create_file_menu_(model, controller);
    create_view_menu_();
    create_activity_menu_(model, controller);
    create_options_menu_(model, controller);
    create_tools_menu_(model, controller);
    create_help_menu_();

    auto *tabs_widget = new TabsWidget(model, controller, this);
    connect(this, &Gui::current_tab_to_be_changed, tabs_widget, &TabsWidget::switch_to_tab);

    setCentralWidget(tabs_widget);
    show();
}

void Gui::keyPressEvent(QKeyEvent *event) {
    if (event->modifiers() == Qt::ControlModifier && event->key() == Qt::Key_Q) {
        emit quit();
    } else {
        QMainWindow::keyPressEvent(event);
    }
}

void Gui::show_info(QString title, QString message) {
    QMessageBox::information(this, title, message, QMessageBox::Ok);
}

void Gui::show_warning(QString title, QString message) {
    QMessageBox::warning(this, title, message, QMessageBox::Ok);
}

void Gui::show_error(QString title, QString message) {
    QMessageBox::critical(this, title, message, QMessageBox::Ok);
}

void Gui::create_file_menu_(const Model &model, Controller &controller) {
    auto *menu = new FileMenu("&File", this);
    menuBar()->addMenu(menu);

    connect(&model, &Model::host_selected,   menu, &FileMenu::select_host);
    connect(&model, &Model::host_unselected, menu, &FileMenu::unselect_host);
#ifndef NDEBUG
    menu->connected();
#endif

    connect(menu, &FileMenu::computer_to_be_selected, [this]() {
        auto dlg = new SelectComputerDialog(this);
        connect(dlg, &SelectComputerDialog::computer_selected, this, &Gui::computer_selected);
        dlg->setAttribute(Qt::WA_DeleteOnClose);
        dlg->open();
    });

    connect(menu, &FileMenu::shutdown_to_be_triggered, &controller, &Controller::trigger_client_shutdown);
    connect(menu, &FileMenu::to_quit, this, &Gui::quit);
}

void Gui::create_view_menu_() {
    auto *menu = new ViewMenu("&View", this);
    menuBar()->addMenu(menu);

    connect(menu, &ViewMenu::current_tab_to_be_changed, this, &Gui::current_tab_to_be_changed);
}

void Gui::create_activity_menu_(const Model &model, Controller &controller) {
    auto *menu = new ActivityMenu("&Activity", this);
    menuBar()->addMenu(menu);

    connect(&model, &Model::run_modes_updated, menu, &ActivityMenu::update_run_modes);
    connect(&model, &Model::host_selected,     menu, &ActivityMenu::select_host);
    connect(&model, &Model::host_unselected,   menu, &ActivityMenu::unselect_host);
#ifndef NDEBUG
    menu->connected();
#endif

    connect(menu, &ActivityMenu::run_mode_set, [&controller](QString host, RunMode mode) {
        controller.set_run_mode(host, mode);
    });
    connect(menu, &ActivityMenu::gpu_mode_set, [&controller](QString host, RunMode mode) {
        controller.set_gpu_mode(host, mode);
    });
    connect(menu, &ActivityMenu::network_mode_set, [&controller](QString host, RunMode mode) {
        controller.set_network_mode(host, mode);
    });
}

void Gui::create_options_menu_(const Model &model, Controller &controller) {
    auto *options_menu = new OptionsMenu("&Options", this);
    menuBar()->addMenu(options_menu);
    connect(&model, &Model::host_selected,     options_menu, &OptionsMenu::select_host);
    connect(&model, &Model::host_unselected,   options_menu, &OptionsMenu::unselect_host);
#ifndef NDEBUG
    options_menu->connected();
#endif

    connect(options_menu, &OptionsMenu::computation_preferences_to_be_shown,
            [=, &controller](QString host) {
                // TODO Load the prefs in the dialog while showing a loading animation
                controller.load_global_prefs(
                    host, GetGlobalPrefsMode::Working,
                    [=, &controller](GlobalPreferences loaded_prefs) {
                        auto *dlg = new PreferencesDialog(std::move(loaded_prefs), this);

                        connect(dlg, &PreferencesDialog::save, [=, &controller](GlobalPreferences prefs, GlobalPreferencesMask mask) {
                            controller.save_global_prefs(
                                host, std::move(prefs), std::move(mask),
                                [=, &controller](bool status) {
                                    if (status)
                                        controller.read_global_prefs(host, [](bool){}, [](QString) {});
                                    else
                                        QMessageBox::critical(this, QStringLiteral("Error"), QStringLiteral("The client failed to save the preferences."), QMessageBox::Ok);

                                },
                                [=](QString error) {
                                    QMessageBox::critical(this, QStringLiteral("Error"), error, QMessageBox::Ok);
                                });
                        });

                        dlg->setAttribute(Qt::WA_DeleteOnClose);
                        dlg->open();
                    },
                    [=](QString error) {
                        show_error(QString::fromUtf8("Error"), error);
                    });
            });

    connect(options_menu, &OptionsMenu::config_files_to_be_read, [=, &controller](QString host) {
        controller.read_config_files(
            host,
            [=](bool success){
                if (!success)
                    QMessageBox::critical(this, QStringLiteral("Error"), QStringLiteral("Failed to read the config files"), QMessageBox::Ok);
            },
            [=](QString error) {
                QMessageBox::critical(this, QStringLiteral("Error"), error, QMessageBox::Ok);
            });
    });

    connect(options_menu, &OptionsMenu::local_prefs_file_to_be_read, [=, &controller](QString host) {
        controller.read_global_prefs(
            host,
            [=](bool success){
                if (!success)
                    QMessageBox::critical(this, QStringLiteral("Error"), QStringLiteral("Failed to read the preferences file"), QMessageBox::Ok);
            },
            [=](QString error) {
                QMessageBox::critical(this, QStringLiteral("Error"), error, QMessageBox::Ok);
            });
    });
}

void Gui::create_tools_menu_(const Model &model, Controller &controller) {
    auto *menu = new ToolsMenu("&Tools", this);
    menuBar()->addMenu(menu);

    connect(&model, &Model::host_selected,   menu, &ToolsMenu::select_host);
    connect(&model, &Model::host_unselected, menu, &ToolsMenu::unselect_host);
#ifndef NDEBUG
    menu->connected();
#endif

    connect(menu, &ToolsMenu::add_project_wizard_to_be_shown,
            [=, &controller](QString host) {
                auto *wizard = new AddProjectWizard(controller, std::move(host), this);
                wizard->setAttribute(Qt::WA_DeleteOnClose);
                wizard->open();
            });

    connect(menu, &ToolsMenu::cpu_benchmarks_to_be_run, &controller, &Controller::run_cpu_benchmarks);
    connect(menu, &ToolsMenu::pending_transfers_to_be_retried, &controller, &Controller::retry_pending_transfers);
}

void Gui::create_help_menu_() {
    auto *menu = new HelpMenu("&Help", this);
    menuBar()->addMenu(menu);

    connect(menu, &HelpMenu::about_dialog_to_be_shown, []() {
        auto *dlg = new AboutDialog();
        dlg->setAttribute(Qt::WA_DeleteOnClose);
        dlg->open();
    });
}

}}}
