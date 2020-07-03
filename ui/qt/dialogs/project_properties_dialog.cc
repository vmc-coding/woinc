/* ui/qt/dialogs/project_properties_dialog.cc --
   Written and Copyright (C) 2019-2020 by vmc.

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

#include "qt/dialogs/project_properties_dialog.h"

#include <cassert>
#ifndef NDEBUG
#include <iostream>
#endif

#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QString>
#include <QVBoxLayout>

#include "qt/utils.h"

namespace {

QLabel *label__(const QString &text, QWidget *parent = nullptr) {
    return new QLabel(text, parent);
}

QLabel *bold_label__(const QString &text, QWidget *parent = nullptr) {
    auto lbl = label__(text, parent);
    lbl->setStyleSheet("font-weight: bold;");
    return lbl;
}

void add_section_header__(QGridLayout *lyt, const QString &text) {
    auto lbl = bold_label__(text);
    lbl->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Maximum);
    lyt->addWidget(lbl, lyt->rowCount(), 0);
}

void add_row__(QGridLayout *lyt, QWidget *left, QWidget *right) {
    left->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Maximum);
    right->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Maximum);
    int row = lyt->rowCount();
    lyt->addWidget(left, row, 0);
    lyt->addWidget(right, row, 1);
}

void add_row__(QGridLayout *lyt, const QString &left, const QString &right) {
    add_row__(lyt, label__(left), label__(right));
}

QString to_string__(bool b) {
    return b ? QStringLiteral("yes") : QStringLiteral("no");
}

} // unnamed namespace

namespace woinc { namespace ui { namespace qt {

ProjectPropertiesDialog::ProjectPropertiesDialog(Project in_project, DiskUsage in_disk_usage, QWidget *parent)
    : QDialog(parent, Qt::Dialog)
    , project_(std::move(in_project))
    , disk_usage_(std::move(in_disk_usage))
{
    setWindowTitle(QStringLiteral("Properties of project %1").arg(project_.name));

    auto *glyt = new QGridLayout;

    add_section_header__(glyt, QStringLiteral("General"));
    add_row__(glyt, QStringLiteral("URL"), project_.project_url);
    add_row__(glyt, QStringLiteral("User name"), project_.account);
    add_row__(glyt, QStringLiteral("Team name"), project_.team);
    add_row__(glyt, QStringLiteral("Resource share"), QString::number(project_.resource_share.first));
    if (project_.min_rpc_time > time(nullptr))
        add_row__(glyt, QStringLiteral("Scheduler RPC deferred for"), seconds_as_time_string(project_.min_rpc_time - time(nullptr)));
    if (project_.download_backoff > 0)
        add_row__(glyt, QStringLiteral("File downloads deferred for"), seconds_as_time_string(project_.download_backoff));
    if (project_.upload_backoff > 0)
        add_row__(glyt, QStringLiteral("File uploads deferred for"), seconds_as_time_string(project_.upload_backoff));
    {
        auto usage_iter = std::find_if(disk_usage_.projects.begin(), disk_usage_.projects.end(),
                                       [&](auto &pu) { return pu.name == project_.name; });
        assert(usage_iter != disk_usage_.projects.end());
        auto usage = usage_iter == disk_usage_.projects.end() ? -1 : usage_iter->usage;
        auto factor = normalization_values(usage);
        add_row__(glyt, QStringLiteral("Disk usage"),
                  QStringLiteral("%1 %2").arg(QString::number(usage/factor.first, 'f', 2)).arg(factor.second));
    }
    add_row__(glyt, QStringLiteral("Computer ID"), QString::number(project_.hostid));
    if (project_.non_cpu_intensive)
        add_row__(glyt, QStringLiteral("Non CPU intensive"), to_string__(true));
    add_row__(glyt, QStringLiteral("Suspended via GUI"), to_string__(project_.suspended_via_gui));
    add_row__(glyt, QStringLiteral("Don't request tasks"), to_string__(project_.dont_request_more_work));
    if (project_.scheduler_rpc_in_progress)
        add_row__(glyt, QStringLiteral("Scheduler call in progress"), to_string__(true));
    if (project_.trickle_up_pending)
        add_row__(glyt, QStringLiteral("Trickle-up pending"), to_string__(true));
    add_row__(glyt, QStringLiteral("Host location"),
              project_.venue.isEmpty() ? QStringLiteral("default") : project_.venue);
    if (project_.attached_via_acct_mgr)
        add_row__(glyt, QStringLiteral("Added via account manager"), to_string__(true));
    if (project_.detach_when_done)
        add_row__(glyt, QStringLiteral("Remove when tasks done"), to_string__(true));
    if (project_.ended)
        add_row__(glyt, QStringLiteral("Ended"), to_string__(true));
    add_row__(glyt, QStringLiteral("Tasks completed"), QString::number(project_.njobs_success));
    add_row__(glyt, QStringLiteral("Tasks failed"), QString::number(project_.njobs_error));


    add_section_header__(glyt, QStringLiteral("Credit"));
    add_row__(glyt, QStringLiteral("User"),
              QStringLiteral("%1 total, %2 average").arg(QString::number(project_.user_total_credit, 'f', 0))
                                                    .arg(QString::number(project_.user_expavg_credit, 'f', 2)));
    add_row__(glyt, QStringLiteral("Host"),
              QStringLiteral("%1 total, %2 average").arg(QString::number(project_.host_total_credit, 'f', 0))
                                                    .arg(QString::number(project_.host_expavg_credit, 'f', 2)));


    if (!project_.non_cpu_intensive || project_.last_rpc_time)
        add_section_header__(glyt, QStringLiteral("Scheduling"));
    if (!project_.non_cpu_intensive) {
        add_row__(glyt, QStringLiteral("Scheduling priority"), QString::number(project_.sched_priority, 'f', 2));
        // TODO see BOINC
    }
    if (project_.last_rpc_time)
        add_row__(glyt, QStringLiteral("Last scheduler reply"), time_t_as_string(project_.last_rpc_time));

    auto *vertical_filler = new QWidget(this);
    vertical_filler->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    glyt->addWidget(vertical_filler, glyt->rowCount(), 0);

    auto *prop_widget = new QWidget(this);
    prop_widget->setLayout(glyt);

    auto *scrolling_prop_widget = new QScrollArea(this);
    scrolling_prop_widget->setWidgetResizable(true);
    scrolling_prop_widget->setWidget(prop_widget);


    auto *close_btn = new QPushButton("Close", this);
    close_btn->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    connect(close_btn, &QPushButton::released, this, &ProjectPropertiesDialog::accept);

    auto *btn_lyt = new QHBoxLayout;
    btn_lyt->setAlignment(Qt::AlignRight);
    btn_lyt->setContentsMargins(0, 0, 0, 0);
    btn_lyt->addWidget(close_btn);

    auto *btn_panel = new QWidget(this);
    btn_panel->setLayout(btn_lyt);


    auto *lyt = new QVBoxLayout;
    lyt->addWidget(scrolling_prop_widget);
    lyt->addWidget(btn_panel);
    setLayout(lyt);
}

}}}
