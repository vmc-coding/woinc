/* ui/qt/tabs/transfers_tab.h --
   Written and Copyright (C) 2018-2020 by vmc.

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

#ifndef WOINC_UI_QT_TABS_TRANSFERS_TAB_H_
#define WOINC_UI_QT_TABS_TRANSFERS_TAB_H_

#include <set>

#include <QAbstractButton>
#include <QAbstractTableModel>
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

namespace transfers_tab_internals {

struct SelectedTransfer {
    QString project_url;
    QString filename;
};

typedef std::vector<SelectedTransfer> SelectedTransfers;

class ButtonPanel : public QWidget {
    Q_OBJECT

    public:
        ButtonPanel(QWidget *parent = nullptr);
        virtual ~ButtonPanel() = default;

        ButtonPanel(const ButtonPanel&) = delete;
        ButtonPanel(ButtonPanel &&) = delete;

        ButtonPanel &operator=(const ButtonPanel&) = delete;
        ButtonPanel &operator=(ButtonPanel &&) = delete;

    public slots:
        void select_host(QString host);
        void unselect_host(QString host);
        void update_transfers(FileTransfers transfers);
        void update_selected_transfers(SelectedTransfers selected_transfers);

    signals:
        void file_transfer_op_clicked(QString host, FILE_TRANSFER_OP op, QString project_url, QString filename);

    private:
        void update_();

    private:
        enum class Command {
            ABORT,
            RETRY
        };

        QString selected_host_;
        bool has_transfers_ = false;
        SelectedTransfers selected_transfers_;
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

    public:
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

        SelectedTransfer selected_transfer(size_t row) const;

    public slots:
        void update(FileTransfers file_transfers);

    private:
        FileTransfers file_transfers_;
};

class TableView : public QTableView {
    Q_OBJECT

    public:
        TableView(TabModel *tab_model, QWidget *parent = nullptr);
        virtual ~TableView() = default;

        TableView(const TableView &) = delete;
        TableView &operator=(const TableView &) = delete;

        TableView(TableView &&) = delete;
        TableView &operator=(TableView &&) = delete;

    signals:
        void selection_changed(SelectedTransfers selected_transfers);

    protected:
        void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected) final;

    private:
        TabModel *tab_model_ = nullptr;
        QSortFilterProxyModel *proxy_model_ = nullptr;
};

} // namespace transfers_tab_internals

class TransfersTab : public QWidget {
    Q_OBJECT

    public:
        TransfersTab(QWidget *parent = nullptr);
        virtual ~TransfersTab() = default;

    signals:
        void file_transfer_op_clicked(QString host, FILE_TRANSFER_OP op, QString project_url, QString filename);

        // delegated signals from the model to the internal widgets

        void host_selected(QString host);
        void host_unselected(QString host);

        void file_transfers_updated(woinc::ui::qt::FileTransfers transfers);
};

}}}

#endif
