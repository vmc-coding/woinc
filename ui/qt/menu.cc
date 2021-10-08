/* ui/qt/menu.cc --
   Written and Copyright (C) 2017-2021 by vmc.

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

#include "qt/menu.h"

#include <cassert>

#include <QDesktopServices>
#include <QUrl>

namespace woinc { namespace ui { namespace qt {

// ----- HostAwareMenu -----

HostAwareMenu::HostAwareMenu(const QString &title, QWidget *parent)
    : QMenu(title, parent)
{}

#ifndef NDEBUG

HostAwareMenu::~HostAwareMenu() {
    assert(connected_);
}

void HostAwareMenu::connected() {
    connected_ = true;
}

#endif

void HostAwareMenu::select_host(QString host) {
    selected_host_ = std::move(host);
    host_selected_();
}

void HostAwareMenu::unselect_host(QString /*host*/) {
    selected_host_.clear();
    host_unselected_();
}

void HostAwareMenu::host_selected_() {
    setEnabled(true);
}

void HostAwareMenu::host_unselected_(){
    setEnabled(false);
}

// ----- FileMenu -----

FileMenu::FileMenu(const QString &title, QWidget *parent)
    : QMenu(title, parent)
{
    auto *new_window = addAction("New woincqt window");
    new_window->setShortcuts(QKeySequence::New);
    //connect(exit, &QAction::triggered, [this]() { emit new_window(); });
    new_window->setEnabled(false);

    auto *select_computer = addAction("Select computer..."); // Ctrl+Shift+I
    select_computer->setShortcut(Qt::CTRL + Qt::SHIFT + Qt::Key_I);
    connect(select_computer, &QAction::triggered, this, &FileMenu::computer_to_be_selected);

    addAction("Shut down connected client...")->setEnabled(false);

    addSeparator();

    addAction("Close the woincqt window")->setEnabled(false); // Ctrl+W

    auto *exit = addAction("Exit woincqt");
    exit->setShortcuts(QKeySequence::Quit);
    connect(exit, &QAction::triggered, this, &FileMenu::to_quit);
}

// ----- ViewMenu -----

ViewMenu::ViewMenu(const QString &title, QWidget *parent)
    : QMenu(title, parent)
{
#define ADD_ACTION(TITLE, SHORTCUT, TYPE) do { \
        auto *action = addAction(TITLE); \
        action->setShortcut(QKeySequence(SHORTCUT)); \
        connect(action, &QAction::triggered, [this]() { emit current_tab_to_be_changed(TYPE); }); \
    } while (0)

    ADD_ACTION("&Notices", Qt::CTRL + Qt::SHIFT + Qt::Key_N, TAB::NOTICES);
    ADD_ACTION("&Projects", Qt::CTRL + Qt::SHIFT + Qt::Key_P, TAB::PROJECTS);
    ADD_ACTION("&Tasks", Qt::CTRL + Qt::SHIFT + Qt::Key_T, TAB::TASKS);
    ADD_ACTION("Trans&fers", Qt::CTRL + Qt::SHIFT + Qt::Key_X, TAB::TRANSFERS);
    ADD_ACTION("&Statistics", Qt::CTRL + Qt::SHIFT + Qt::Key_S, TAB::STATISTICS);
    ADD_ACTION("&Disk", Qt::CTRL + Qt::SHIFT + Qt::Key_D, TAB::DISK);
    ADD_ACTION("&Events", Qt::CTRL + Qt::SHIFT + Qt::Key_E, TAB::EVENTS);

#undef ADD_ACTION
}

// ----- ActivityMenu -----

ActivityMenu::ActivityMenu(const QString &title, QWidget *parent)
    : HostAwareMenu(title, parent)
{
    setEnabled(false);

#define ADD_ACTION(ACTION, TITLE, SIGNAL, MODE) do { \
        ACTION = group->addAction(TITLE); \
        ACTION->setCheckable(true); \
        addAction(ACTION); \
        connect(ACTION, &QAction::triggered, [this]() { \
            emit SIGNAL(selected_host_, MODE); \
        }); \
    } while (0)

    auto *group = new QActionGroup(this);

    ADD_ACTION(run_always_, "&Run always", run_mode_set, RunMode::Always);
    ADD_ACTION(run_auto_, "Run based on &preferences", run_mode_set, RunMode::Auto);
    ADD_ACTION(run_never_, "&Suspend", run_mode_set, RunMode::Never);

    addSeparator();

    group = new QActionGroup(this);

    ADD_ACTION(gpu_always_, "Use GPU always", gpu_mode_set, RunMode::Always);
    ADD_ACTION(gpu_auto_, "Use GPU based on preferences", gpu_mode_set, RunMode::Auto);
    ADD_ACTION(gpu_never_, "&Suspend GPU", gpu_mode_set, RunMode::Never);

    addSeparator();

    group = new QActionGroup(this);

    ADD_ACTION(network_always_, "Network activity always", network_mode_set, RunMode::Always);
    ADD_ACTION(network_auto_, "Network activity based on preferences", network_mode_set, RunMode::Auto);
    ADD_ACTION(network_never_, "Suspend network activity", network_mode_set, RunMode::Never);

#undef ADD_ACTION
}

void ActivityMenu::update_run_modes(RunModes modes) {
    run_always_->setChecked(modes.cpu == RunMode::Always);
    run_auto_->setChecked(modes.cpu == RunMode::Auto);
    run_never_->setChecked(modes.cpu == RunMode::Never);

    gpu_always_->setChecked(modes.gpu == RunMode::Always);
    gpu_auto_->setChecked(modes.gpu == RunMode::Auto);
    gpu_never_->setChecked(modes.gpu == RunMode::Never);

    network_always_->setChecked(modes.network == RunMode::Always);
    network_auto_->setChecked(modes.network == RunMode::Auto);
    network_never_->setChecked(modes.network == RunMode::Never);
}

// ----- OptionsMenu -----

OptionsMenu::OptionsMenu(const QString &title, QWidget *parent)
    : HostAwareMenu(title, parent)
{
    setEnabled(false);

    connect(addAction("Computation &preferences..."), &QAction::triggered, [&]() {
        emit computation_preferences_to_be_shown(selected_host_);
    });

    addAction("Exclusive applications...")->setEnabled(false);
    addSeparator();
    addAction("Select columns...")->setEnabled(false);
    addAction("Event Log options...")->setEnabled(false); // Ctrl+Shift+F
    addAction("&Other options...")->setEnabled(false);
    addSeparator();
    connect(addAction("Read config files"), &QAction::triggered, [&]() {
        emit config_files_to_be_read(selected_host_);
    });
    connect(addAction("Read local prefs file"), &QAction::triggered, [&]() {
        emit local_prefs_file_to_be_read(selected_host_);
    });
}

// ----- ToolsMenu -----

ToolsMenu::ToolsMenu(const QString &title, QWidget *parent)
    : HostAwareMenu(title, parent)
{
    setEnabled(false);

    auto *add_project_action = addAction("&Add project");
    connect(add_project_action, &QAction::triggered, [&]() {
        emit add_project_wizard_to_be_shown(selected_host_);
    });

    addSeparator();
    connect(addAction("Run CPU &benchmarks"), &QAction::triggered, [&]() {
        emit cpu_benchmarks_to_be_run(selected_host_);
    });
    connect(addAction("Retry pending transfers"), &QAction::triggered, [&]() {
        emit pending_transfers_to_be_retried(selected_host_);
    });
}

// ----- HelpMenu -----

HelpMenu::HelpMenu(const QString &title, QWidget *parent)
    : QMenu(title, parent)
{
    connect(addAction("BOINC &help"), &QAction::triggered, []() {
        QDesktopServices::openUrl(QUrl("https://boinc.berkeley.edu/wiki/BOINC_Help"));
    });
    addSeparator();
    connect(addAction("&BOINC web site"), &QAction::triggered, []() {
        QDesktopServices::openUrl(QUrl("https://boinc.berkeley.edu/"));
    });
    connect(addAction("&woinc web site"), &QAction::triggered, []() {
        QDesktopServices::openUrl(QUrl("https://github.com/vmc-coding/woinc"));
    });
    addSeparator();
    addAction("Check for new BOINC version")->setEnabled(false);
    addAction("Check for new woincqt version")->setEnabled(false);
    addSeparator();
    addAction("About woinc&qt...")->setEnabled(false);
}

}}}
