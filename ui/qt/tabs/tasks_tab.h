/* ui/qt/tabs/tasks_tab.h --
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

#ifndef WOINC_UI_QT_TABS_TASKS_TAB_H_
#define WOINC_UI_QT_TABS_TASKS_TAB_H_

#include <map>

#include <QWidget>

#include <QAbstractButton>
#include <QAbstractTableModel>
#include <QSet> // used in signals
#include <QSortFilterProxyModel>
#include <QTableView>
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

namespace tasks_tab_internals {

typedef QSet<typename Projects::size_type> SelectedRows;

class ButtonPanel : public QWidget {
    Q_OBJECT

    public:
        ButtonPanel(QWidget *parent = nullptr);
        virtual ~ButtonPanel() = default;

    public slots:
        void update_selected_tasks(Tasks selected_tasks);

    public slots:
        void select_host(QString host);
        void unselect_host(QString host);

    signals:
        void active_only_tasks_clicked(QString host, bool value);
        void task_op_clicked(QString host, QString project_url, QString name, TASK_OP op);

    private:
        enum class Command {
            SHOW_ACTIVE_ONLY,
            SHOW_ALL,
            SHOW_GRAPHICS,
            SUSPEND,
            RESUME,
            ABORT,
            PROPERTIES
        };

        QString selected_host_;
        Tasks selected_tasks_;
        std::map<Command, QAbstractButton*> cmd_btns_;
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
        int rowCount(const QModelIndex &parent = QModelIndex()) const override;
        int columnCount(const QModelIndex &parent = QModelIndex()) const override;
        QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
        QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    public:
        template<typename TAB_MODEL, typename TARGET_CONTAINER, typename SOURCE_CONTAINER, typename DATA_ITEM_COMP>
        friend void woinc::ui::qt::update_tab_model(TAB_MODEL &model,
                                                    TARGET_CONTAINER &current_data,
                                                    SOURCE_CONTAINER new_data,
                                                    DATA_ITEM_COMP comp);

    public slots:
        void select_host(QString host);
        void unselect_host(QString host);
        void select_tasks(SelectedRows rows);
        void update_tasks(Tasks tasks);

    signals:
        void host_selected(QString host);
        void host_unselected(QString host);
        void tasks_updated();
        void tasks_selected(Tasks selected_tasks);

    private:
        QVariant data_as_display_role_(const QModelIndex &index) const;

    private:
        QString selected_host_;
        Tasks tasks_;
};

class TableView : public QTableView {
    Q_OBJECT

    public:
        TableView(TabModel *tab_model, QWidget *parent = nullptr);
        virtual ~TableView() = default;

    public slots:
        void update_task_selection();

    signals:
        void task_selection_changed(SelectedRows rows);

    protected slots:
        void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected) final;
};

} // namespace tasks_tab_internals

class TasksTab : public QWidget {
    Q_OBJECT

    public:
        TasksTab(QWidget *parent = nullptr);
        virtual ~TasksTab() = default;

    signals:
        void active_only_tasks_clicked(QString host, bool value);
        void task_op_clicked(QString host, QString project_url, QString name, TASK_OP op);

        // delegated signals from the model to the internal widgets

        void host_selected(QString host);
        void host_unselected(QString host);

        void tasks_updated(woinc::ui::qt::Tasks tasks);
};

}}}

#endif
