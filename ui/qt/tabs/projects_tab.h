/* ui/qt/tabs/projects_tab.h --
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

#ifndef WOINC_UI_QT_TABS_PROJECTS_TAB_H_
#define WOINC_UI_QT_TABS_PROJECTS_TAB_H_

#include <map>

#include <QAbstractButton>
#include <QAbstractTableModel>
#include <QSet> // used in signals
#include <QString>
#include <QTableView>
#include <QVariant>
#include <QVector> // used in signals
#include <QWidget>

#include <woinc/defs.h>

#include "qt/types.h"

namespace woinc { namespace ui { namespace qt {

// forward declaration for usage as friend later on
template<typename TAB_MODEL, typename TARGET_CONTAINER, typename SOURCE_CONTAINER, typename DATA_ITEM_COMP>
void update_tab_model(TAB_MODEL &model,
                      TARGET_CONTAINER &current_data,
                      SOURCE_CONTAINER new_data,
                      DATA_ITEM_COMP comp);

namespace projects_tab_internals {

typedef QSet<typename Projects::size_type> SelectedRows;

struct SelectedProject {
    QString host;
    QString project_url;
    bool allow_more_work = false;
    bool suspended = false;

    // only valid if there is a single project selected
    QVariant project;
};

typedef QVector<SelectedProject> SelectedProjects;

class ButtonPanel : public QWidget {
    Q_OBJECT

    public:
        ButtonPanel(QWidget *parent = nullptr);
        virtual ~ButtonPanel() = default;

    public slots:
        void update_disk_usage(DiskUsage disk_usage);
        void update_selected_projects(SelectedProjects selected_projects);

    signals:
        void project_op_clicked(QString host, QString project_url, ProjectOp op);

    private:
        enum class Command {
            UPDATE,
            SUSPEND,
            RESUME,
            NO_NEW_TASKS,
            ALLOW_NEW_TASKS,
            RESET,
            REMOVE,
            PROPERTIES
        };

        std::map<Command, QAbstractButton*> cmd_btns_;

        SelectedProjects selected_projects_;
        DiskUsage disk_usage_;
};

class TabModel : public QAbstractTableModel {
    Q_OBJECT

    public:
        TabModel(QObject *parent = nullptr);
        virtual ~TabModel() = default;

        TabModel(const TabModel &) = delete;
        TabModel &operator=(const TabModel &) = delete;

        TabModel(TabModel &&) = delete;
        TabModel &operator=(TabModel &&) = delete;

    public: // from QAbstractTableModel
        int rowCount(const QModelIndex &parent = QModelIndex()) const final;
        int columnCount(const QModelIndex &parent = QModelIndex()) const final;
        QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const final;
        QVariant headerData(int section, Qt::Orientation orientation, int role) const final;

    public:
        template<typename TAB_MODEL, typename TARGET_CONTAINER, typename SOURCE_CONTAINER, typename DATA_ITEM_COMP>
        friend void woinc::ui::qt::update_tab_model(TAB_MODEL &model,
                                                    TARGET_CONTAINER &current_data,
                                                    SOURCE_CONTAINER new_data,
                                                    DATA_ITEM_COMP comp);

    public slots:
        void select_host(QString host);
        void unselect_host(QString host);
        void select_projects(SelectedRows rows);
        void update_projects(Projects projects);

    signals:
        void projects_updated();
        void projects_selected(SelectedProjects selected_projects);

    private:
        QVariant data_as_display_role_(const QModelIndex &index) const;
        QVariant data_as_sort_role_(const QModelIndex &index) const;

    private:
        QString selected_host_;
        Projects projects_;
};

class TableView : public QTableView {
    Q_OBJECT

    public:
        TableView(TabModel *tab_model, QWidget *parent = nullptr);
        virtual ~TableView() = default;

    public slots:
        void update_project_selection();

    signals:
        void project_selection_changed(SelectedRows selected_rows);

    protected slots:
        void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected) final;
};

} // namespace projects_tab_internals

class ProjectsTab : public QWidget {
    Q_OBJECT

    public:
        ProjectsTab(QWidget *parent = nullptr);
        virtual ~ProjectsTab() = default;

    signals:
        void project_op_clicked(QString host, QString project_url, ProjectOp op);

        // delegated signals from the model to the internal widgets

        void host_selected(QString host);
        void host_unselected(QString host);

        void disk_usage_updated(woinc::ui::qt::DiskUsage disk_usage);
        void projects_updated(woinc::ui::qt::Projects projects);
};

}}}

#endif
