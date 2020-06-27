/* ui/qt/tabs_widget.cc --
   Written and Copyright (C) 2017-2020 by vmc.

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

#include "qt/tabs_widget.h"

#include "qt/controller.h"
#include "qt/model.h"
#include "qt/tabs/disk_tab.h"
#include "qt/tabs/events_tab.h"
#include "qt/tabs/notices_tab.h"
#include "qt/tabs/projects_tab.h"
#include "qt/tabs/statistics_tab.h"
#include "qt/tabs/tasks_tab.h"
#include "qt/tabs/transfers_tab.h"

namespace woinc { namespace ui { namespace qt {

TabsWidget::TabsWidget(const Model &model, Controller &controller, QWidget *parent)
    : QTabWidget(parent)
{
    {
        auto *tab = new NoticesTab(this);
        tab_indices_.emplace(TAB::NOTICES, addTab(tab, "Notices"));

        connect(&model, &Model::host_selected,     tab, &NoticesTab::select_host);
        connect(&model, &Model::host_unselected,   tab, &NoticesTab::unselect_host);
        connect(&model, &Model::notices_appended,  tab, &NoticesTab::append_notices);
        connect(&model, &Model::notices_refreshed, tab, &NoticesTab::refresh_notices);
    }

    {
        auto *tab = new ProjectsTab(this);
        tab_indices_.emplace(TAB::PROJECTS, addTab(tab, "Projects"));

        connect(&model, &Model::host_selected,      tab, &ProjectsTab::host_selected);
        connect(&model, &Model::host_unselected,    tab, &ProjectsTab::host_unselected);
        connect(&model, &Model::disk_usage_updated, tab, &ProjectsTab::disk_usage_updated);
        connect(&model, &Model::projects_updated,   tab, &ProjectsTab::projects_updated);

        connect(tab, &ProjectsTab::project_op_clicked, &controller, &Controller::do_project_op);
    }

    {
        auto *tab = new TasksTab(this);
        tab_indices_.emplace(TAB::TASKS, addTab(tab, "Tasks"));

        connect(&model, &Model::host_selected,   tab, &TasksTab::host_selected);
        connect(&model, &Model::host_unselected, tab, &TasksTab::host_unselected);
        connect(&model, &Model::tasks_updated,   tab, &TasksTab::tasks_updated);

        connect(tab, &TasksTab::task_op_clicked, &controller, &Controller::do_task_op);
        connect(tab, &TasksTab::active_only_tasks_clicked, &controller, &Controller::do_active_only_tasks);
    }

    {
        auto *tab = new TransfersTab(this);
        tab_indices_.emplace(TAB::TRANSFERS, addTab(tab, "Transfers"));

        connect(&model, &Model::file_transfers_updated, tab, &TransfersTab::file_transfers_updated);
        connect(&model, &Model::host_selected,          tab, &TransfersTab::host_selected);
        connect(&model, &Model::host_unselected,        tab, &TransfersTab::host_unselected);

        connect(tab, &TransfersTab::file_transfer_op_clicked, &controller, &Controller::do_file_transfer_op);
    }

    {
        auto *tab = new StatisticsTab(this);
        tab_indices_.emplace(TAB::STATISTICS, addTab(tab, "Statistics"));

        connect(&model, &Model::projects_updated,   tab, &StatisticsTab::projects_updated);
        connect(&model, &Model::statistics_updated, tab, &StatisticsTab::statistics_updated);
    }

    {
        auto *tab = new DiskTab(this);
        tab_indices_.emplace(TAB::DISK, addTab(tab, "Disk"));

        connect(&model, &Model::disk_usage_updated, tab, &DiskTab::update_disk_usage);
        connect(&model, &Model::host_selected,      tab, &DiskTab::select_host);
        connect(&model, &Model::host_unselected,    tab, &DiskTab::unselect_host);
    }

    {
        auto *tab = new EventsTab(this);
        tab_indices_.emplace(TAB::EVENTS, addTab(tab, "Events"));

        connect(&model, &Model::events_appended, tab, &EventsTab::events_appended);
        connect(&model, &Model::host_selected,   tab, &EventsTab::host_selected);
        connect(&model, &Model::host_unselected, tab, &EventsTab::host_unselected);
    }
}

void TabsWidget::switch_to_tab(TAB tab) {
    setCurrentIndex(tab_indices_[tab]);
}

}}}
