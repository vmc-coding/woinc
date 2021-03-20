/* ui/qt/dialogs/task_properties_dialog.cc --
   Written and Copyright (C) 2020, 2021 by vmc.

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

#include "qt/dialogs/task_properties_dialog.h"

#ifndef NDEBUG
#include <iostream>
#endif

#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLocale>
#include <QPushButton>
#include <QScrollArea>
#include <QString>
#include <QVBoxLayout>

#include "qt/utils.h"

namespace {

void add_row__(QGridLayout *lyt, QWidget *left, QWidget *right) {
    left->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Maximum);
    right->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Maximum);
    int row = lyt->rowCount();
    lyt->addWidget(left, row, 0);
    lyt->addWidget(right, row, 1);
}

void add_row__(QGridLayout *lyt, const QString &left, const QString &right) {
    add_row__(lyt, new QLabel(left), new QLabel(right));
}

} // unnamed namespace

namespace woinc { namespace ui { namespace qt {

TaskPropertiesDialog::TaskPropertiesDialog(Task in_task, QWidget *parent)
    : QDialog(parent, Qt::Dialog)
    , task_(std::move(in_task))
{
    setWindowTitle(QStringLiteral("Properties of task %1").arg(task_.name));

    auto *glyt = new QGridLayout;

    add_row__(glyt, QStringLiteral("Application"), task_.application);
    add_row__(glyt, QStringLiteral("Name"), task_.wu_name);
    add_row__(glyt, QStringLiteral("State"), task_.status);
    if (task_.received_time > 0)
        add_row__(glyt, QStringLiteral("Received"), time_t_as_string(task_.received_time));
    add_row__(glyt, QStringLiteral("Report deadline"), time_t_as_string(task_.deadline));
	if  (!task_.resources.isEmpty())
        add_row__(glyt, QStringLiteral("Resources"), task_.resources);
    if (task_.estimated_computation_size > 0)
        add_row__(glyt, QStringLiteral("Estimated computation size"),
                  QStringLiteral("%1 GFLOPs").arg(QLocale::system().toString(static_cast<qulonglong>(task_.estimated_computation_size / 1e9))));
    //if (!task_.keywords.isEmpty())
        //add_row__(glyt, QStringLiteral("Keywords"), tbd);
    if (task_.active_task) {
        add_row__(glyt, QStringLiteral("CPU time"), seconds_as_time_string(task_.current_cpu_time));
        add_row__(glyt, QStringLiteral("CPU time since checkpoint"), seconds_as_time_string(task_.current_cpu_time - task_.checkpoint_cpu_time));
        if (task_.elapsed_seconds > 0)
            add_row__(glyt, QStringLiteral("Elapsed time"), seconds_as_time_string(task_.elapsed_seconds));
        add_row__(glyt, QStringLiteral("Estimated time remaining"), seconds_as_time_string(task_.remaining_seconds));
        add_row__(glyt, QStringLiteral("Fraction done"), QString::asprintf("%.3f%%", task_.progress * 100));
        add_row__(glyt, QStringLiteral("Virtual memory size"), size_as_string(task_.virtual_mem_size));
        add_row__(glyt, QStringLiteral("Working set size"), size_as_string(task_.working_set_size));
        if (task_.slot >= 0)
            add_row__(glyt, QStringLiteral("Directory"), QStringLiteral("slots/%1").arg(task_.slot));
        if (task_.pid)
            add_row__(glyt, QStringLiteral("Process ID"), QStringLiteral("%1").arg(task_.pid));
        if (task_.progress_rate > 0) {
            QString rate;
            if (task_.progress_rate * 3600 < 1)
                rate = QString::asprintf("%.3f%% %s", task_.progress_rate * 100 * 3600, "per hour");
            else if (task_.progress_rate * 60 < 1)
                rate = QString::asprintf("%.3f%% %s", task_.progress_rate * 100 * 60, "per minute");
            else
                rate = QString::asprintf("%.3f%% %s", task_.progress_rate * 100, "per second");
            add_row__(glyt, QStringLiteral("Progress rate"), rate);
        }
    } else {
        add_row__(glyt, QStringLiteral("CPU time"), seconds_as_time_string(task_.final_cpu_seconds));
        add_row__(glyt, QStringLiteral("Elapsed time"), seconds_as_time_string(task_.final_elapsed_seconds));
    }

    if (!task_.executable.isEmpty())
        add_row__(glyt, QStringLiteral("Executable"), task_.executable);

    auto *vertical_filler = new QWidget(this);
    vertical_filler->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    glyt->addWidget(vertical_filler, glyt->rowCount(), 0);

    auto *prop_widget = new QWidget;
    //prop_widget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    prop_widget->setLayout(glyt);

    auto *scrolling_prop_widget = new QScrollArea;
    scrolling_prop_widget->setWidgetResizable(false);
    scrolling_prop_widget->setWidget(prop_widget);


    auto *close_btn = new QPushButton("Close");
    close_btn->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    connect(close_btn, &QPushButton::released, this, &TaskPropertiesDialog::accept);

    auto *btn_lyt = new QHBoxLayout;
    btn_lyt->setAlignment(Qt::AlignRight);
    btn_lyt->setContentsMargins(0, 0, 0, 0);
    btn_lyt->addWidget(close_btn);

    auto *btn_panel = new QWidget;
    btn_panel->setLayout(btn_lyt);


    auto *lyt = new QVBoxLayout;
    lyt->addWidget(scrolling_prop_widget);
    lyt->addWidget(btn_panel);
    setLayout(lyt);
}

}}}
