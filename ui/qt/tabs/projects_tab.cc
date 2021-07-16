/* ui/qt/tabs/projects_tab.cc --
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

#include "qt/tabs/projects_tab.h"

#include <algorithm>
#include <cassert>

#ifndef NDEBUG
#include <iostream>
#endif

#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLocale>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

#include "qt/dialogs/project_properties_dialog.h"
#include "qt/tabs/delegates.h"
#include "qt/tabs/proxy_models.h"
#include "qt/tabs/tab_model_updater.h"

namespace {

enum {
    INDEX_PROJECT,
    INDEX_ACCOUNT,
    INDEX_TEAM,
    INDEX_WORK_DONE,
    INDEX_AVG_WORK_DONE,
    INDEX_RESOURCE_SHARE,
    INDEX_STATUS
};

enum {
    COLUMN_COUNT = 7,
    PTM_SORT_RULE = Qt::UserRole
};

int compare__(const QString &a, const QString &b) {
    return a.localeAwareCompare(b);
}

int compare__(const woinc::ui::qt::Project &a,
              const woinc::ui::qt::Project &b) {
    return compare__(a.name, b.name);
}

}

namespace woinc { namespace ui { namespace qt { namespace projects_tab_internals {

// ------- ButtonPanel -------

ButtonPanel::ButtonPanel(QWidget *parent) : QWidget(parent) {
    setLayout(new QVBoxLayout);

    auto cmds_layout = new QVBoxLayout;
    cmds_layout->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

#define WOINC_ADD_BTN(BTN, LABEL) cmd_btns_.emplace(BTN, new QPushButton(LABEL))
#define WOINC_CONNECT_BTN(BTN, OP) connect(cmd_btns_[BTN], &QPushButton::released, this, \
                                       [&]() { \
                                           for (auto &&proj : selected_projects_) \
                                                emit project_op_clicked(proj.host, proj.project_url, OP); \
                                       })

    WOINC_ADD_BTN(Command::UPDATE, "Update");
    WOINC_ADD_BTN(Command::SUSPEND, "Suspend");
    WOINC_ADD_BTN(Command::RESUME, "Resume");
    WOINC_ADD_BTN(Command::NO_NEW_TASKS, "No new tasks");
    WOINC_ADD_BTN(Command::ALLOW_NEW_TASKS, "Allow new tasks");
    WOINC_ADD_BTN(Command::RESET, "Reset project");
    // TODO should we ask the user before removing it?
    WOINC_ADD_BTN(Command::REMOVE, "Remove");
    WOINC_ADD_BTN(Command::PROPERTIES, "Properties");

    WOINC_CONNECT_BTN(Command::UPDATE, ProjectOp::Update);
    WOINC_CONNECT_BTN(Command::SUSPEND, ProjectOp::Suspend);
    WOINC_CONNECT_BTN(Command::RESUME, ProjectOp::Resume);
    WOINC_CONNECT_BTN(Command::NO_NEW_TASKS, ProjectOp::Nomorework);
    WOINC_CONNECT_BTN(Command::ALLOW_NEW_TASKS, ProjectOp::Allowmorework);
    WOINC_CONNECT_BTN(Command::RESET, ProjectOp::Reset);
    WOINC_CONNECT_BTN(Command::REMOVE, ProjectOp::Detach);

    connect(cmd_btns_[Command::PROPERTIES], &QPushButton::released, this,
            [&]() {
                assert(selected_projects_.size() == 1);
                assert(!selected_projects_.first().project.isNull());
                if (selected_projects_.size() == 1 && !selected_projects_.first().project.isNull()) {
                    auto proj = selected_projects_.first().project.value<woinc::ui::qt::Project>();
                    (new ProjectPropertiesDialog(std::move(proj), disk_usage_, this))->open();
                } else {
                    QMessageBox::critical(this,
                                          QStringLiteral("Error"),
                                          QStringLiteral("Could not load project properties."),
                                          QMessageBox::Ok);
                }
            });
#undef WOINC_ADD_BTN
#undef WOINC_CONNECT_BTN

    for (auto &entry : cmd_btns_) {
        entry.second->setEnabled(false);
        entry.second->setMinimumSize(cmd_btns_[Command::ALLOW_NEW_TASKS]->minimumSizeHint());
        cmds_layout->addWidget(entry.second);
    }

    cmd_btns_[Command::RESUME]->setVisible(false);
    cmd_btns_[Command::ALLOW_NEW_TASKS]->setVisible(false);

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

void ButtonPanel::update_disk_usage(DiskUsage disk_usage) {
    disk_usage_ = std::move(disk_usage);
}

void ButtonPanel::update_selected_projects(SelectedProjects selected_projects) {
    if (selected_projects.empty()) {
        for (auto &entry : cmd_btns_)
            entry.second->setEnabled(false);

        auto project_pages = layout()->itemAt(1);
        assert(project_pages);

        project_pages->widget()->setVisible(false);

        selected_projects_.clear();
        return;
    }

    bool allow_more_work = selected_projects.front().allow_more_work;
    bool equal_task_status = std::all_of(selected_projects.cbegin(), selected_projects.cend(),
                                         [&](const auto &proj) {
                                             return proj.allow_more_work == allow_more_work;
                                         });

    bool suspended = selected_projects.front().suspended;
    bool equal_suspend_status = std::all_of(selected_projects.cbegin(), selected_projects.cend(),
                                            [&](const auto &proj) {
                                                return proj.suspended == suspended;
                                            });


    cmd_btns_[Command::UPDATE]->setEnabled(true);

    if (equal_suspend_status) {
        cmd_btns_[Command::SUSPEND]->setVisible(!suspended);
        cmd_btns_[Command::SUSPEND]->setEnabled(!suspended);
        cmd_btns_[Command::RESUME]->setVisible(suspended);
        cmd_btns_[Command::RESUME]->setEnabled(suspended);
    } else {
        cmd_btns_[Command::SUSPEND]->setEnabled(false);
        cmd_btns_[Command::RESUME]->setEnabled(false);
    }

    if (equal_task_status) {
        cmd_btns_[Command::NO_NEW_TASKS]->setVisible(allow_more_work);
        cmd_btns_[Command::NO_NEW_TASKS]->setEnabled(allow_more_work);
        cmd_btns_[Command::ALLOW_NEW_TASKS]->setVisible(!allow_more_work);
        cmd_btns_[Command::ALLOW_NEW_TASKS]->setEnabled(!allow_more_work);
    } else {
        cmd_btns_[Command::NO_NEW_TASKS]->setEnabled(false);
        cmd_btns_[Command::ALLOW_NEW_TASKS]->setEnabled(false);
    }

    cmd_btns_[Command::RESET]->setEnabled(true);
    cmd_btns_[Command::REMOVE]->setEnabled(true);
    cmd_btns_[Command::PROPERTIES]->setEnabled(selected_projects.size() == 1);

    selected_projects_ = std::move(selected_projects);
}

// ------- TabModel -------

TabModel::TabModel(QObject *parent) : QAbstractTableModel(parent) {}

int TabModel::rowCount(const QModelIndex &parent) const {
    return parent.isValid() ? 0 : static_cast<int>(projects_.size());
}

int TabModel::columnCount(const QModelIndex &parent) const {
    return parent.isValid() ? 0 : COLUMN_COUNT;
}

QVariant TabModel::data(const QModelIndex &index, int role) const {
    assert(index.isValid());

    if (!index.isValid())
        return QVariant();

    assert(index.row() >= 0);
    assert(static_cast<size_t>(index.row()) < projects_.size());

    switch (role) {
        case Qt::DisplayRole:
            return data_as_display_role_(index);
        case PTM_SORT_RULE: // otherwise we would get local aware string comp on the *work_done columns
            return data_as_sort_role_(index);
        case Qt::TextAlignmentRole:
            switch (index.column()) {
                case INDEX_PROJECT:
                case INDEX_ACCOUNT:
                case INDEX_TEAM:
                case INDEX_STATUS:
                    return Qt::AlignLeft + Qt::AlignVCenter;
                case INDEX_WORK_DONE:
                case INDEX_AVG_WORK_DONE:
                case INDEX_RESOURCE_SHARE:
                    return Qt::AlignRight + Qt::AlignVCenter;
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
            case INDEX_PROJECT:        return QString("Project");
            case INDEX_ACCOUNT:        return QString("Account");
            case INDEX_TEAM:           return QString("Team");
            case INDEX_WORK_DONE:      return QString("Work done");
            case INDEX_AVG_WORK_DONE:  return QString("Avg. work done");
            case INDEX_RESOURCE_SHARE: return QString("Resource share");
            case INDEX_STATUS:         return QString("Status");
        }
        assert(false);
    } else if (role == Qt::TextAlignmentRole) {
        switch (section) {
            case INDEX_PROJECT:
            case INDEX_ACCOUNT:
            case INDEX_TEAM:
            case INDEX_STATUS:
                return Qt::AlignLeft + Qt::AlignVCenter;
            case INDEX_WORK_DONE:
            case INDEX_AVG_WORK_DONE:
                return Qt::AlignRight + Qt::AlignVCenter;
            case INDEX_RESOURCE_SHARE:
                return Qt::AlignCenter;
        }
        assert(false);
    }

    return QVariant();
}

void TabModel::select_host(QString host) {
    selected_host_ = host;
    update_projects({});
}

void TabModel::unselect_host(QString /*host*/) {
    selected_host_.clear();
    update_projects({});
}

void TabModel::select_projects(SelectedRows selected_rows) {
    SelectedProjects selected_projects;
    selected_projects.reserve(selected_rows.size());

    for (auto &&row : selected_rows) {
        const auto &proj = projects_[row];
        selected_projects.push_back({
            selected_host_,
            proj.project_url,
            !proj.dont_request_more_work,
            proj.suspended_via_gui,
            selected_rows.size() == 1 ? QVariant::fromValue<woinc::ui::qt::Project>(proj) : QVariant()
        });
    }

    emit projects_selected(std::move(selected_projects));
}

void TabModel::update_projects(Projects new_projects) {
    update_tab_model(*this,
                     projects_,
                     std::move(new_projects),
                     [](const Project &a, const Project &b) { return compare__(a, b); });

    emit projects_updated();
}

QVariant TabModel::data_as_display_role_(const QModelIndex &index) const {
    const Project &project = projects_.at(static_cast<Projects::size_type>(index.row()));

    switch (index.column()) {
        case INDEX_PROJECT:         return project.name;
        case INDEX_ACCOUNT:         return project.account;
        case INDEX_TEAM:            return project.team;
        case INDEX_WORK_DONE:       return QLocale::system().toString(static_cast<int>(project.user_total_credit));
        case INDEX_AVG_WORK_DONE:   return QLocale::system().toString(project.user_expavg_credit, 'f', 2);
        case INDEX_RESOURCE_SHARE:  return QVariant::fromValue(project.resource_share);
        case INDEX_STATUS:          return project.status;
    }

    assert(false);
    return QVariant();
}

QVariant TabModel::data_as_sort_role_(const QModelIndex &index) const {
    const Project &project = projects_.at(static_cast<Projects::size_type>(index.row()));

    switch (index.column()) {
        case INDEX_PROJECT:         return project.name;
        case INDEX_ACCOUNT:         return project.account;
        case INDEX_TEAM:            return project.team;
        case INDEX_WORK_DONE:       return project.user_total_credit;
        case INDEX_AVG_WORK_DONE:   return project.user_expavg_credit;
        case INDEX_RESOURCE_SHARE:  return project.resource_share.first;
        case INDEX_STATUS:          return project.status;
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
    setItemDelegateForColumn(INDEX_ACCOUNT, new HtmlEntityDelegate(this));
    setItemDelegateForColumn(INDEX_TEAM, new HtmlEntityDelegate(this));
    setItemDelegateForColumn(INDEX_RESOURCE_SHARE, new ResourceShareBarDelegate(this));

    auto proxy_model_(new RowBackgroundProxyModel(this));
    proxy_model_->setSourceModel(tab_model);
    proxy_model_->setSortRole(PTM_SORT_RULE);
    setModel(proxy_model_);
}

void TableView::update_project_selection() {
    SelectedRows selected_rows;

    for (auto &&index : selectedIndexes()) {
        assert(dynamic_cast<QSortFilterProxyModel*>(model()));
        auto row = static_cast<QSortFilterProxyModel*>(model())->mapToSource(index).row();
        assert(row >= 0);
        selected_rows.insert(static_cast<SelectedRows::value_type>(row));
    }

    emit project_selection_changed(selected_rows);
}

void TableView::selectionChanged(const QItemSelection &selected, const QItemSelection &deselected) {
    QTableView::selectionChanged(selected, deselected);
    update_project_selection();
}

} // namespace projects_tab_internals

// ------- ProjectsTab -------

ProjectsTab::ProjectsTab(QWidget *parent)
    : QWidget(parent)
{
    using namespace woinc::ui::qt::projects_tab_internals;

    auto btn_panel = new ButtonPanel(this);
    auto tab_model = new TabModel(this);
    auto table_view = new TableView(tab_model, this);

    auto scrollable_btn_panel = new QScrollArea(this);
    scrollable_btn_panel->setWidget(btn_panel);
    scrollable_btn_panel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);

    auto layout = new QHBoxLayout();
    layout->addWidget(scrollable_btn_panel);
    layout->addWidget(table_view);
    setLayout(layout);

    connect(this, &ProjectsTab::disk_usage_updated, btn_panel, &ButtonPanel::update_disk_usage);

    connect(this, &ProjectsTab::host_selected,    tab_model, &TabModel::select_host);
    connect(this, &ProjectsTab::host_unselected,  tab_model, &TabModel::unselect_host);
    connect(this, &ProjectsTab::projects_updated, tab_model, &TabModel::update_projects);

    connect(table_view, &TableView::project_selection_changed, tab_model, &TabModel::select_projects);

    connect(tab_model, &TabModel::projects_updated, table_view, &TableView::update_project_selection);

    connect(tab_model, &TabModel::projects_selected, btn_panel, &ButtonPanel::update_selected_projects);

    connect(btn_panel, &ButtonPanel::project_op_clicked, this, &ProjectsTab::project_op_clicked);
}

}}}
