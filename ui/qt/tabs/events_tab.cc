/* ui/qt/tabs/events_tab.cc --
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

#include "qt/tabs/events_tab.h"

#include <deque>
#ifndef NDEBUG
#include <iostream>
#endif

#include <QFontMetrics>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QScrollBar>

#include "qt/tabs/proxy_models.h"
#include "qt/tabs/tab_model_updater.h"
#include "qt/utils.h"

namespace {

enum {
    INDEX_PROJECT,
    INDEX_TIMESTAMP,
    INDEX_MESSAGE
};

enum {
    COLUMN_COUNT = 3,
    MAX_NUM_EVENTS = 2000
};

}

namespace woinc { namespace ui { namespace qt { namespace events_tab_internals {

// ------- TabModel -------

TabModel::TabModel(QObject *parent) : QAbstractTableModel(parent) {
    events_.reserve(MAX_NUM_EVENTS);
}

int TabModel::rowCount(const QModelIndex &parent) const {
    return parent.isValid() ? 0 : static_cast<int>(events_.size());
}

int TabModel::columnCount(const QModelIndex &parent) const {
    return parent.isValid() ? 0 : COLUMN_COUNT;
}

QVariant TabModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid())
        return QVariant();

    if (role == Qt::TextAlignmentRole)
        return Qt::AlignLeft + Qt::AlignVCenter;

    assert(index.row() >= 0);

    auto row = static_cast<Events::size_type>(index.row());
    assert(row < events_.size());

    const auto &event = events_.at(row);

    // TODO how to choose a proper color when themes are used?
    if (role == Qt::ForegroundRole && event.user_alert)
        return QBrush(QColor::fromRgbF(1, 0, 0, 1));

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
            case INDEX_PROJECT:   return event.project_name;
            case INDEX_TIMESTAMP: return time_t_as_string(event.timestamp);
            case INDEX_MESSAGE:   return event.message;
        }

        assert(false);
    }

    return QVariant();
}

QVariant TabModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation != Qt::Horizontal)
        return QVariant();

    if (role == Qt::TextAlignmentRole)
        return Qt::AlignLeft + Qt::AlignVCenter;

    if (role == Qt::DisplayRole) {
        switch (section) {
            case INDEX_PROJECT:   return QString::fromUtf8("Project");
            case INDEX_TIMESTAMP: return QString::fromUtf8("Time");
            case INDEX_MESSAGE:   return QString::fromUtf8("Message");
        }
        assert(false);
    }

    return QVariant();
}

void TabModel::select_host(QString /*host*/) {
    events_.clear();
    append_events({});
}

void TabModel::unselect_host(QString /*host*/) {
    events_.clear();
    append_events({});
}

void TabModel::append_events(Events new_events) {
    std::deque<Event> events;
    events.insert(events.end(), events_.begin(), events_.end());
    events.insert(events.end(), new_events.begin(), new_events.end());

    while (events.size() > MAX_NUM_EVENTS)
        events.pop_front();

    update_tab_model(*this,
                     events_,
                     std::move(events),
                     [](const Event &a, const Event &b) { return a.seqno - b.seqno; });

    emit updated();
}

// ------- TableView -------

TableView::TableView(TabModel *model, QWidget *parent)
    : QTableView(parent), tab_model_(model)
{
    auto proxy_model = new RowBackgroundProxyModel(this);
    proxy_model->setSourceModel(tab_model_);

    setModel(proxy_model);


    verticalHeader()->hide();

    setShowGrid(false);
    setSortingEnabled(false);
    setWordWrap(false);

    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::NoSelection);

    horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    horizontalHeader()->setStretchLastSection(true);

    QFontMetrics font_metrics(font());
    setColumnWidth(0, font_metrics.boundingRect(QString::fromUtf8("XXXXXXXXXXXXXXXXXXXX")).width());
    setColumnWidth(1, font_metrics.boundingRect(QString::fromUtf8("XXXXXXXXXXXXXXXXXXXXXXXXXX")).width());

    connect(tab_model_, &TabModel::updated, this, &TableView::scrollToBottom);
    scroll_to_bottom_ = true;
    connect(verticalScrollBar(), &QScrollBar::valueChanged, this, &TableView::adjust_scrolling_);
}

void TableView::adjust_scrolling_(int value) {
    if (value == verticalScrollBar()->maximum()) {
        if (!scroll_to_bottom_) {
            connect(tab_model_, &TabModel::updated, this, &TableView::scrollToBottom);
            scroll_to_bottom_ = true;
        }
    } else {
        if (scroll_to_bottom_) {
            disconnect(tab_model_, &TabModel::updated, this, &TableView::scrollToBottom);
            scroll_to_bottom_ = false;
        }
    }
}

} // internals

// ------- EventsTab -------

EventsTab::EventsTab(QWidget *parent) : QWidget(parent) {
    using namespace woinc::ui::qt::events_tab_internals;

    auto tab_model = new TabModel(this);
    auto view = new TableView(tab_model, this);

    auto layout = new QHBoxLayout;
    layout->addWidget(view);
    setLayout(layout);

    connect(this, &EventsTab::events_appended, tab_model, &TabModel::append_events);
    connect(this, &EventsTab::host_selected,   tab_model, &TabModel::select_host);
    connect(this, &EventsTab::host_unselected, tab_model, &TabModel::unselect_host);
}

void EventsTab::append_events(Events events) {
    emit events_appended(events);
}

void EventsTab::select_host(QString host) {
    emit host_selected(host);
}

void EventsTab::unselect_host(QString host) {
    emit host_unselected(host);
}

}}}
