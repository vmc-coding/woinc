/* ui/qt/tabs/events_tab.h --
   Written and Copyright (C) 2019 by vmc.

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

#ifndef WOINC_UI_QT_EVENTS_TAB_H_
#define WOINC_UI_QT_EVENTS_TAB_H_

#include <QAbstractTableModel>
#include <QTableView>
#include <QWidget>

#include "qt/types.h"

namespace woinc { namespace ui { namespace qt {

template<typename TAB_MODEL, typename TARGET_CONTAINER, typename SOURCE_CONTAINER, typename DATA_ITEM_COMP>
void update_tab_model(TAB_MODEL &model,
                      TARGET_CONTAINER &current_data,
                      SOURCE_CONTAINER new_data,
                      DATA_ITEM_COMP comp);

namespace events_tab_internals {

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
        void append_events(Events events);
        void select_host(QString host);
        void unselect_host(QString host);

    signals:
        void updated();

    private:
        // TODO use a deque instead of a vector as we want to only hold x events, not all of them
        Events events_;
};

class TableView : public QTableView {
    Q_OBJECT

    public:
        TableView(TabModel *model, QWidget *parent = nullptr);
        virtual ~TableView() = default;

        TableView(const TableView &) = delete;
        TableView &operator=(const TableView &) = delete;

        TableView(TableView &&) = delete;
        TableView &operator=(TableView &&) = delete;

    private slots:
        void adjust_scrolling_(int value);

    private:
        TabModel *tab_model_;
        bool scroll_to_bottom_ = false;
};

} // namespace events_tab_internals

class EventsTab : public QWidget {
    Q_OBJECT

    public:
        EventsTab(QWidget *parent = nullptr);
        virtual ~EventsTab() = default;

        EventsTab(const EventsTab &) = delete;
        EventsTab &operator=(const EventsTab &) = delete;

        EventsTab(EventsTab &&) = delete;
        EventsTab &operator=(EventsTab &&) = delete;

    public slots:
        void append_events(Events events);
        void select_host(QString host);
        void unselect_host(QString host);

    signals:
        void events_appended(Events events);
        void host_selected(QString host);
        void host_unselected(QString host);
};

}}}

#endif
