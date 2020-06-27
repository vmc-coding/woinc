/* ui/qt/main.cc --
   Written and Copyright (C) 2017-2019 by vmc.

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

#include <cstdlib>
#include <iostream>

#include <QApplication>

#include <woinc/defs.h>
#include <woinc/types.h>

#include "qt/adapter.h"
#include "qt/controller.h"
#include "qt/defs.h"
#include "qt/gui.h"
#include "qt/model_handler.h"
#include "qt/types.h"

namespace woincqt = woinc::ui::qt;

namespace {

void register_meta_types__();

//void connect_adapter_controller__(woincqt::HandlerAdapter *adapter, woincqt::Controller *ctrl);
void connect_adapter_model__(woincqt::HandlerAdapter *adapter, woincqt::ModelHandler *model);
void connect_app_controller__(QApplication *app, woincqt::Controller *ctrl);
void connect_gui_controller__(woincqt::Gui *gui, woincqt::Controller *ctrl);
void connect_model_controller__(woincqt::ModelHandler *model, woincqt::Controller *ctrl);
void connect_gui_app__(woincqt::Gui *gui, QApplication *app);

}

int main(int argc, char **argv) {
    QApplication app(argc, argv);

    register_meta_types__();

    woincqt::Controller controller;
    woincqt::Gui gui;
    woincqt::ModelHandler model;
    woincqt::HandlerAdapter adapter;

    controller.connect(&adapter);

    connect_adapter_model__(&adapter, &model);
    connect_app_controller__(&app, &controller);
    connect_gui_controller__(&gui, &controller);
    connect_model_controller__(&model, &controller);
    connect_gui_app__(&gui, &app);

    controller.register_handler(static_cast<woinc::ui::HostHandler*>(&adapter));
    controller.register_handler(static_cast<woinc::ui::PeriodicTaskHandler*>(&adapter));

    gui.open(model, controller);

    // temp workaround to provide a password; should of course be done via the UI or proper cmd flags
    if (argc == 2) {
        // TODO argv may not be in Utf8
        controller.add_host(QString::fromUtf8("localhost"),
                                     QString::fromUtf8("localhost"),
                                     woinc::ui::qt::DEFAULT_PORT,
                                     QString::fromUtf8(argv[1]));
    }

    return app.exec();
}

namespace {

void register_meta_types__() {
    qRegisterMetaType<woinc::ui::Error>();

    qRegisterMetaType<woinc::CCStatus>();
    qRegisterMetaType<woinc::ClientState>();
    qRegisterMetaType<woinc::DiskUsage>();
    qRegisterMetaType<woinc::FileTransfers>();
    qRegisterMetaType<woinc::Notices>();
    qRegisterMetaType<woinc::Messages>();
    qRegisterMetaType<woinc::Projects>();
    qRegisterMetaType<woinc::Statistics>();
    qRegisterMetaType<woinc::Tasks>();

    qRegisterMetaType<woinc::ui::qt::AppVersions>();
    qRegisterMetaType<woinc::ui::qt::DiskUsage>();
    qRegisterMetaType<woinc::ui::qt::Events>();
    qRegisterMetaType<woinc::ui::qt::FileTransfers>();
    qRegisterMetaType<woinc::ui::qt::Notices>();
    qRegisterMetaType<woinc::ui::qt::Project>();
    qRegisterMetaType<woinc::ui::qt::Projects>();
    qRegisterMetaType<woinc::ui::qt::RunModes>();
    qRegisterMetaType<woinc::ui::qt::Statistics>();
    qRegisterMetaType<woinc::ui::qt::Tasks>();
    qRegisterMetaType<woinc::ui::qt::Workunits>();
}

void connect_adapter_model__(woincqt::HandlerAdapter *adapter, woincqt::ModelHandler *model) {
#define WOINC_CONNECT(FROM, TO) QObject::connect(adapter, &woincqt::HandlerAdapter::FROM, \
                                                 model, &woincqt::ModelHandler::TO, \
                                                 Qt::QueuedConnection)
    WOINC_CONNECT(added, add_host);
    WOINC_CONNECT(removed, remove_host);
    WOINC_CONNECT(updated_cc_status, update_cc_status);
    WOINC_CONNECT(updated_client_state, update_client_state);
    WOINC_CONNECT(updated_disk_usage, update_disk_usage);
    WOINC_CONNECT(updated_file_transfers, update_file_transfers);
    WOINC_CONNECT(updated_notices, update_notices);
    WOINC_CONNECT(updated_messages, update_messages);
    WOINC_CONNECT(updated_projects, update_projects);
    WOINC_CONNECT(updated_statistics, update_statistics);
    WOINC_CONNECT(updated_tasks, update_tasks);
#undef WOINC_CONNECT
}

void connect_app_controller__(QApplication *app, woincqt::Controller *controller) {
    QObject::connect(app, &QApplication::aboutToQuit, controller, &woincqt::Controller::trigger_shutdown);
}

void connect_gui_controller__(woincqt::Gui *gui, woincqt::Controller *controller) {
    QObject::connect(gui, &woincqt::Gui::computer_selected, controller, &woincqt::Controller::add_host);
    QObject::connect(controller, &woincqt::Controller::info_occurred, gui, &woincqt::Gui::show_info);
    QObject::connect(controller, &woincqt::Controller::warning_occurred, gui, &woincqt::Gui::show_warning);
    QObject::connect(controller, &woincqt::Controller::error_occurred, gui, &woincqt::Gui::show_error);
}

void connect_model_controller__(woincqt::ModelHandler *model, woincqt::Controller *controller) {
#define WOINC_CONNECT(FROM, TO) QObject::connect(model, &woincqt::ModelHandler::FROM, \
                                                 controller, &woincqt::Controller::TO, \
                                                 Qt::QueuedConnection)
    WOINC_CONNECT(disk_usage_update_needed, schedule_disk_usage_update);
    WOINC_CONNECT(projects_update_needed  , schedule_projects_update);
    WOINC_CONNECT(state_update_needed     , schedule_state_update);
    WOINC_CONNECT(statistics_update_needed, schedule_statistics_update);
    WOINC_CONNECT(tasks_update_needed     , schedule_tasks_update);
#undef WOINC_CONNECT
}

void connect_gui_app__(woincqt::Gui *gui, QApplication *app) {
    QObject::connect(gui, &woincqt::Gui::quit, app, &QApplication::quit);
}

}
