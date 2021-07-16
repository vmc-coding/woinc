/* ui/qt/tabs/tasks_tab.cc --
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

#include "qt/tabs/tasks_tab.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <ctime>

#ifndef NDEBUG
#include <iostream>
#endif

#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QTextStream>

#include "qt/dialogs/task_properties_dialog.h"
#include "qt/tabs/delegates.h"
#include "qt/tabs/proxy_models.h"
#include "qt/tabs/tab_model_updater.h"
#include "qt/utils.h"

namespace {

enum {
    INDEX_PROJECT,
    INDEX_PROGRESS,
    INDEX_STATUS,
    INDEX_ELAPSED,
    INDEX_REMAINING,
    INDEX_DEADLINE,
    INDEX_APPLICATION,
    INDEX_TASK_NAME
};

enum {
    COLUMN_COUNT = 8
};

int compare(const QString &a, const QString &b) {
    return a.localeAwareCompare(b);
}

int compare(const woinc::ui::qt::Task &a, const woinc::ui::qt::Task &b) {
    int result = compare(a.project, b.project);
    if (!result)
        result = compare(a.name, b.name);
    return result;
}

} // unnamed namespace

namespace woinc { namespace ui { namespace qt { namespace tasks_tab_internals {

// ------- ButtonPanel -------

ButtonPanel::ButtonPanel(QWidget *parent) : QWidget(parent) {
    setLayout(new QVBoxLayout);

    auto cmds_layout = new QVBoxLayout;
    cmds_layout->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

#define WOINC_ADD_BTN(BTN, LABEL) cmd_btns_.emplace(BTN, new QPushButton(LABEL))

    WOINC_ADD_BTN(Command::SHOW_ACTIVE_ONLY, "Show active tasks");
    WOINC_ADD_BTN(Command::SHOW_ALL, "Show all tasks");
    WOINC_ADD_BTN(Command::SHOW_GRAPHICS, "Show graphics");
    WOINC_ADD_BTN(Command::SUSPEND, "Suspend");
    WOINC_ADD_BTN(Command::RESUME, "Resume");
    WOINC_ADD_BTN(Command::ABORT, "Abort");
    WOINC_ADD_BTN(Command::PROPERTIES, "Properties");

#undef WOINC_ADD_BTN

#define WOINC_CONNECT_BTN(BTN, VALUE) connect(cmd_btns_[BTN], &QPushButton::released, this, \
                                              [&]() { \
                                                  emit active_only_tasks_clicked(selected_host_, VALUE); \
                                                  cmd_btns_[Command::SHOW_ACTIVE_ONLY]->setVisible(!VALUE); \
                                                  cmd_btns_[Command::SHOW_ALL]->setVisible(VALUE); \
                                              })

    WOINC_CONNECT_BTN(Command::SHOW_ACTIVE_ONLY, true);
    WOINC_CONNECT_BTN(Command::SHOW_ALL, false);

#undef WOINC_CONNECT_BTN

#define WOINC_CONNECT_BTN(BTN, OP) connect(cmd_btns_[BTN], &QPushButton::released, this, \
                                           [&]() { \
                                               for (auto &&task : selected_tasks_) \
                                                   emit task_op_clicked(task.host, task.project_url, task.name, OP); \
                                           })

    WOINC_CONNECT_BTN(Command::SUSPEND, TaskOp::Suspend);
    WOINC_CONNECT_BTN(Command::RESUME, TaskOp::Resume);
#undef WOINC_CONNECT_BTN

    connect(cmd_btns_[Command::ABORT], &QPushButton::released, this,
            [&]() {
                QString msg;
                QTextStream ts(&msg);
                ts << "Are you sure you want to abort ";

                if (selected_tasks_.size() == 1) {
                    auto &task = selected_tasks_.front();
                    ts << "this task '" << task.name << "'?\n(Progress: "
                        << task.progress << "%, Status: " << task.status << ")";
                } else {
                    ts << "these " << selected_tasks_.size() << " tasks?";
                }

                if (QMessageBox::question(this, QString::fromUtf8("Abort task"), msg,
                                          QMessageBox::No | QMessageBox::Yes) == QMessageBox::Yes)
                    for (auto &&task : selected_tasks_)
                        emit task_op_clicked(task.host, task.project_url, task.name, TaskOp::Abort);
            });

    connect(cmd_btns_[Command::PROPERTIES], &QPushButton::released, this,
            [&]() {
                assert(selected_tasks_.size() == 1);
                auto dlg = new TaskPropertiesDialog(selected_tasks_.front(), this);
                dlg->setAttribute(Qt::WA_DeleteOnClose);
                dlg->open();
            });

    for (auto &entry : cmd_btns_) {
        entry.second->setEnabled(false);
        entry.second->setMinimumSize(cmd_btns_[Command::SHOW_ACTIVE_ONLY]->minimumSizeHint());
        cmds_layout->addWidget(entry.second);
    }

    cmd_btns_[Command::SHOW_ACTIVE_ONLY]->setEnabled(true);
    cmd_btns_[Command::SHOW_ALL]->setEnabled(true);
    cmd_btns_[Command::SHOW_ALL]->setVisible(false);
    cmd_btns_[Command::RESUME]->setVisible(false);

    auto cmds = new QGroupBox("Commands");
    cmds->setLayout(cmds_layout);
    cmds->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Maximum);
    layout()->addWidget(cmds);

    auto project_pages = new QGroupBox("Project web pages");
    project_pages->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
    project_pages->setLayout(new QVBoxLayout);
    project_pages->setVisible(false);

    layout()->addWidget(project_pages);
}

void ButtonPanel::update_selected_tasks(Tasks selected_tasks) {
    if (selected_tasks.empty()) {
        for (auto &&entry : cmd_btns_)
            if (entry.first != Command::SHOW_ACTIVE_ONLY && entry.first != Command::SHOW_ALL)
                entry.second->setEnabled(false);

        auto project_pages = layout()->itemAt(1);
        assert(project_pages);

        project_pages->widget()->setVisible(false);
    } else {
        bool suspended = selected_tasks.front().suspended;
        bool equal_suspend_status = std::all_of(selected_tasks.cbegin(), selected_tasks.cend(),
                                                [&](const auto &task) { return task.suspended == suspended; });

        if (equal_suspend_status) {
            cmd_btns_[Command::SUSPEND]->setVisible(!suspended);
            cmd_btns_[Command::SUSPEND]->setEnabled(!suspended);
            cmd_btns_[Command::RESUME]->setVisible(suspended);
            cmd_btns_[Command::RESUME]->setEnabled(suspended);
        } else {
            cmd_btns_[Command::SUSPEND]->setEnabled(false);
            cmd_btns_[Command::RESUME]->setEnabled(false);
        }

        cmd_btns_[Command::ABORT]->setEnabled(true);
        cmd_btns_[Command::PROPERTIES]->setEnabled(selected_tasks.size() == 1);
    }

    selected_tasks_ = std::move(selected_tasks);
}

void ButtonPanel::select_host(QString host) {
    selected_host_ = std::move(host);
}

void ButtonPanel::unselect_host(QString /*host*/) {
    selected_host_.clear();
}

// ------- TabModel -------

TabModel::TabModel(QObject *parent) : QAbstractTableModel(parent) {}

int TabModel::rowCount(const QModelIndex &parent) const {
    return parent.isValid() ? 0 : static_cast<int>(tasks_.size());
}

int TabModel::columnCount(const QModelIndex &parent) const {
    return parent.isValid() ? 0 : COLUMN_COUNT;
}

QVariant TabModel::data(const QModelIndex &index, int role) const {
    assert(index.isValid());

    if (!index.isValid())
        return QVariant();

    assert(index.row() >= 0);
    assert(static_cast<size_t>(index.row()) < tasks_.size());

    switch (role) {
        case Qt::DisplayRole:
            return data_as_display_role_(index);
        case Qt::TextAlignmentRole:
            switch (index.column()) {
                case INDEX_PROJECT:
                case INDEX_STATUS:
                case INDEX_APPLICATION:
                case INDEX_TASK_NAME:
                    return Qt::AlignLeft + Qt::AlignVCenter;
                case INDEX_DEADLINE:
                    return Qt::AlignRight + Qt::AlignVCenter;
                case INDEX_PROGRESS:
                case INDEX_ELAPSED:
                case INDEX_REMAINING:
                    return Qt::AlignCenter;
            }
            assert(false);
            break;
    }

    return QVariant();
}

QVariant TabModel::headerData(int section, Qt::Orientation orientation, int role) const {
    assert(section >= 0);
    assert(section < COLUMN_COUNT);

    if (orientation != Qt::Horizontal)
        return QVariant();

    if (role == Qt::DisplayRole) {
        switch (section) {
            case INDEX_PROJECT:     return QString("Project");
            case INDEX_PROGRESS:    return QString("Progress");
            case INDEX_STATUS:      return QString("Status");
            case INDEX_ELAPSED:     return QString("Elapsed");
            case INDEX_REMAINING:   return QString("Remaining (estimated)");
            case INDEX_DEADLINE:    return QString("Deadline");
            case INDEX_APPLICATION: return QString("Application");
            case INDEX_TASK_NAME:   return QString("Name");
        }
        assert(false);
    } else if (role == Qt::TextAlignmentRole) {
        switch (section) {
            case INDEX_REMAINING:
                return Qt::AlignLeft + Qt::AlignVCenter;
            default:
                return Qt::AlignCenter;
        }
    }

    return QVariant();
}

void TabModel::select_host(QString host) {
    selected_host_ = std::move(host);
    host_selected(selected_host_);
    update_tasks({});
}

void TabModel::unselect_host(QString host) {
    selected_host_.clear();
    host_unselected(std::move(host));
    update_tasks({});
}

void TabModel::select_tasks(SelectedRows selected_rows) {
    Tasks selected_tasks;
    selected_tasks.reserve(static_cast<Tasks::size_type>(selected_rows.size()));

    for (auto &&row : selected_rows) {
        assert(row < tasks_.size());
        selected_tasks.push_back(tasks_[row]);
    }

    emit tasks_selected(std::move(selected_tasks));
}

void TabModel::update_tasks(Tasks new_tasks) {
    update_tab_model(*this,
                     tasks_,
                     std::move(new_tasks),
                     [](const Task &a, const Task &b) { return compare(a,b); });

    emit tasks_updated();
}

QVariant TabModel::data_as_display_role_(const QModelIndex &index) const {
    const Task &task = tasks_.at(static_cast<size_t>(index.row()));

    switch (index.column()) {
        case INDEX_PROJECT:     return task.project;
        case INDEX_PROGRESS:    return task.progress;
        case INDEX_STATUS:      return task.status;
        case INDEX_ELAPSED:     return seconds_as_time_string(task.elapsed_seconds);
        case INDEX_REMAINING:   return seconds_as_time_string(task.remaining_seconds);
        case INDEX_DEADLINE:    return time_t_as_string(task.deadline);
        case INDEX_APPLICATION: return task.application;
        case INDEX_TASK_NAME:   return task.name;
    }

    assert(false);
    return QVariant();
}

// ------- TableView -------

TableView::TableView(TabModel *tab_model, QWidget *parent) : QTableView(parent) {
    verticalHeader()->hide();

    setShowGrid(false);
    setSortingEnabled(true);
    setWordWrap(false);

    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::ExtendedSelection);

    horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    horizontalHeader()->setStretchLastSection(true);

    setItemDelegateForColumn(INDEX_PROJECT, new HtmlEntityDelegate(this));
    setItemDelegateForColumn(INDEX_APPLICATION, new HtmlEntityDelegate(this));
    setItemDelegateForColumn(INDEX_PROGRESS, new ProgressBarDelegate(this));

    auto proxy_model = new RowBackgroundProxyModel(this);
    proxy_model->setSourceModel(tab_model);
    setModel(proxy_model);
}

void TableView::update_task_selection() {
    SelectedRows selected_rows;

    for (auto &&index : selectedIndexes()) {
        assert(dynamic_cast<QSortFilterProxyModel*>(model()));
        auto row = static_cast<QSortFilterProxyModel*>(model())->mapToSource(index).row();
        assert(row >= 0);
        selected_rows.insert(static_cast<SelectedRows::value_type>(row));
    }

    emit task_selection_changed(selected_rows);
}

void TableView::selectionChanged(const QItemSelection &selected, const QItemSelection &deselected) {
    QTableView::selectionChanged(selected, deselected);
    update_task_selection();
}

} // namespace tasks_tab_internals

// ------- TasksTab -------

TasksTab::TasksTab(QWidget *parent) : QWidget(parent) {
    using namespace woinc::ui::qt::tasks_tab_internals;

    auto btn_panel = new ButtonPanel(this);
    auto tab_model = new TabModel(this);
    auto table_view = new TableView(tab_model, this);

    auto scrollable_btn_panel = new QScrollArea(this);
    scrollable_btn_panel->setWidget(btn_panel);
    scrollable_btn_panel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);

    auto layout = new QHBoxLayout;
    layout->addWidget(scrollable_btn_panel);
    layout->addWidget(table_view);
    setLayout(layout);

    connect(this, &TasksTab::host_selected, tab_model, &TabModel::select_host);
    connect(this, &TasksTab::host_unselected, tab_model, &TabModel::unselect_host);
    connect(this, &TasksTab::tasks_updated, tab_model, &TabModel::update_tasks);

    connect(btn_panel, &ButtonPanel::active_only_tasks_clicked, this, &TasksTab::active_only_tasks_clicked);
    connect(btn_panel, &ButtonPanel::task_op_clicked, this, &TasksTab::task_op_clicked);

    connect(tab_model, &TabModel::host_selected,   btn_panel, &ButtonPanel::select_host);
    connect(tab_model, &TabModel::host_unselected, btn_panel, &ButtonPanel::unselect_host);
    connect(tab_model, &TabModel::tasks_selected,  btn_panel, &ButtonPanel::update_selected_tasks);

    connect(tab_model, &TabModel::tasks_updated, table_view, &TableView::update_task_selection);

    connect(table_view, &TableView::task_selection_changed, tab_model, &TabModel::select_tasks);
}

}}}
