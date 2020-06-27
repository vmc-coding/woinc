/* ui/qt/tabs/transfers_tab.cc --
   Written and Copyright (C) 2018-2019 by vmc.

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

#include "qt/tabs/transfers_tab.h"

#include <cassert>

#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QTextStream>
#include <QVBoxLayout>

#include "qt/utils.h"
#include "qt/tabs/delegates.h"
#include "qt/tabs/proxy_models.h"
#include "qt/tabs/tab_model_updater.h"

namespace {

enum {
    INDEX_PROJECT,
    INDEX_FILE,
    INDEX_PROGRESS,
    INDEX_SIZE,
    INDEX_ELAPSED,
    INDEX_SPEED,
    INDEX_STATUS
};

enum {
    COLUMN_COUNT = 7,
};

int compare(const QString &a, const QString &b) {
    return a.localeAwareCompare(b);
}

int compare(const woinc::ui::qt::FileTransfer &a,
            const woinc::ui::qt::FileTransfer &b) {
    int result = compare(a.project, b.project);
    if (!result)
        result = compare(a.file, b.file);
    return result;
}

QString size_to_string(double file_size, double bytes_send) {
    auto fu = woinc::ui::qt::normalization_values(std::max(file_size, bytes_send));

    if (file_size > 0) {
        if (fu.first > 1.) {
            return QString::asprintf("%0.2f/%0.2f %s", bytes_send/fu.first, file_size/fu.first, fu.second);
        } else {
            return QString::asprintf("%d/%d %s", static_cast<int>(bytes_send), static_cast<int>(file_size), fu.second);
        }
    } else {
        return QString::asprintf("%0.2f %s", bytes_send/fu.first, fu.second);
    }
}

} // unnamed namespace

namespace woinc { namespace ui { namespace qt { namespace transfers_tab_internals {

// ------- ButtonPanel -------

ButtonPanel::ButtonPanel(QWidget *parent) : QWidget(parent) {
    setLayout(new QVBoxLayout);

    auto cmds_layout = new QVBoxLayout;
    cmds_layout->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

    cmd_btns_.emplace(Command::RETRY, new QPushButton("Retry now"));
    cmd_btns_.emplace(Command::ABORT, new QPushButton("Abort transfer"));

    cmds_layout->addWidget(cmd_btns_[Command::RETRY]);
    cmds_layout->addWidget(cmd_btns_[Command::ABORT]);

#define WOINC_EXECUTE_OP(OP) do { \
    for (const auto &transfer : selected_transfers_) \
        emit file_transfer_op_clicked(selected_host_, \
                                      OP, \
                                      transfer.project_url, \
                                      transfer.filename); \
    } while (0)

    connect(cmd_btns_[Command::RETRY], &QPushButton::released, this,
            [&]() { WOINC_EXECUTE_OP(FILE_TRANSFER_OP::RETRY); });

    connect(cmd_btns_[Command::ABORT], &QPushButton::released, this,
            [&]() {
                QString msg;
                QTextStream ts(&msg);
                ts << "Are you sure you want to abort ";

                auto note = QString::fromUtf8("\nNOTE: Aborting a transfer will invalidate a task and you will not receive credit for it.");

                if (selected_transfers_.size() == 1) {
                    auto &transfer = selected_transfers_.front();
                    ts << "this transfer '" << transfer.filename << "'?";
                } else {
                    ts << "these " << selected_transfers_.size() << " transfers?";
                }

                ts << note;

                if (QMessageBox::question(this, QString::fromUtf8("Abort file transfer"), msg,
                                          QMessageBox::No | QMessageBox::Yes) == QMessageBox::Yes) {
                    WOINC_EXECUTE_OP(FILE_TRANSFER_OP::ABORT);
                }
            });

    for (auto &entry : cmd_btns_) {
        entry.second->setEnabled(has_transfers_);
        entry.second->setMinimumSize(cmd_btns_[Command::ABORT]->minimumSizeHint());
    }

    auto cmds = new QGroupBox("Commands");
    cmds->setLayout(cmds_layout);
    cmds->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Maximum);
    layout()->addWidget(cmds);
}

void ButtonPanel::select_host(QString host) {
    selected_host_ = std::move(host);
    has_transfers_ = false;
    selected_transfers_.clear();
    update_();
}

void ButtonPanel::unselect_host(QString /*host*/) {
    selected_host_.clear();
    has_transfers_ = false;
    selected_transfers_.clear();
    update_();
}

void ButtonPanel::update_transfers(FileTransfers transfers) {
    has_transfers_ = !transfers.empty();
    update_();
}

void ButtonPanel::update_selected_transfers(SelectedTransfers selected_transfers) {
    selected_transfers_ = std::move(selected_transfers);
    update_();
}

void ButtonPanel::update_() {
    for (auto &entry : cmd_btns_)
        entry.second->setEnabled(has_transfers_ && !selected_transfers_.empty());
}

// ------- TabModel -------

TabModel::TabModel(QObject *parent) : QAbstractTableModel(parent) {}

int TabModel::rowCount(const QModelIndex &parent) const {
    return parent.isValid() ? 0 : static_cast<int>(file_transfers_.size());
}

int TabModel::columnCount(const QModelIndex &parent) const {
    return parent.isValid() ? 0 : COLUMN_COUNT;
}

QVariant TabModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid())
        return QVariant();

    assert(index.row() >= 0);
    assert(static_cast<size_t>(index.row()) < file_transfers_.size());

    if (role == Qt::DisplayRole) {
        const auto &file_transfer = file_transfers_.at(static_cast<size_t>(index.row()));
        switch (index.column()) {
            case INDEX_PROJECT:  return file_transfer.project;
            case INDEX_FILE:     return file_transfer.file;
            case INDEX_PROGRESS: return file_transfer.size > 0 ? file_transfer.bytes_xferred / file_transfer.size : 0;
            case INDEX_SIZE:     return size_to_string(file_transfer.size, file_transfer.bytes_xferred);
            case INDEX_ELAPSED:  return seconds_as_time_string(static_cast<long>(file_transfer.elapsed));
            case INDEX_SPEED:    return QString::asprintf("%0.2f KBps", file_transfer.speed / 1024);
            case INDEX_STATUS:   return file_transfer.status;
        }
        assert(false);
    } else if (role == Qt::TextAlignmentRole) {
        auto column = index.column();
        assert(column >= 0);
        assert(column < COLUMN_COUNT);

        if (column == INDEX_PROGRESS)
            return Qt::AlignRight + Qt::AlignVCenter;
        else
            return Qt::AlignLeft + Qt::AlignVCenter;
    }

    return QVariant();
}

QVariant TabModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation != Qt::Horizontal)
        return QVariant();

    assert(section >= 0);
    assert(section < COLUMN_COUNT);

    if (role == Qt::DisplayRole) {
        switch (section) {
            case INDEX_PROJECT:  return QString::fromUtf8("Project");
            case INDEX_FILE:     return QString::fromUtf8("File");
            case INDEX_PROGRESS: return QString::fromUtf8("Progress");
            case INDEX_SIZE:     return QString::fromUtf8("Size");
            case INDEX_ELAPSED:  return QString::fromUtf8("Elapsed");
            case INDEX_SPEED:    return QString::fromUtf8("Speed");
            case INDEX_STATUS:   return QString::fromUtf8("Status");
        }
        assert(false);
    } else if (role == Qt::TextAlignmentRole) {
        if (section == INDEX_PROGRESS)
            return Qt::AlignCenter;
        else
            return Qt::AlignLeft + Qt::AlignVCenter;
    }

    return QVariant();
}

SelectedTransfer TabModel::selected_transfer(size_t row) const {
    assert(row < file_transfers_.size());
    return { file_transfers_[row].project_url, file_transfers_[row].file };
}

void TabModel::update(FileTransfers file_transfers) {
    update_tab_model(*this,
                     file_transfers_,
                     std::move(file_transfers),
                     [](const FileTransfer &a, const FileTransfer &b) { return compare(a,b); });
}

// ------- TableView -------

TableView::TableView(TabModel *tab_model, QWidget *parent)
    : QTableView(parent), tab_model_(tab_model), proxy_model_(new RowBackgroundProxyModel(this))
{
    verticalHeader()->hide();

    setShowGrid(false);
    setSortingEnabled(true);
    setWordWrap(false);

    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::ExtendedSelection);

    horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    horizontalHeader()->setStretchLastSection(true);

    setItemDelegateForColumn(INDEX_PROJECT, new HtmlEntityDelegate(this));
    setItemDelegateForColumn(INDEX_FILE, new HtmlEntityDelegate(this));
    setItemDelegateForColumn(INDEX_PROGRESS, new ProgressBarDelegate(this, 2 /* precision */));

    proxy_model_->setSourceModel(tab_model_);
    setModel(proxy_model_);
}

void TableView::selectionChanged(const QItemSelection &selected, const QItemSelection &deselected) {
    QTableView::selectionChanged(selected, deselected);

    std::set<int> selected_rows;
    for (auto &&index : selectedIndexes())
        selected_rows.insert(proxy_model_->mapToSource(index).row());

    assert(std::all_of(selected_rows.begin(), selected_rows.end(), [](int i) { return i >= 0; }));

    SelectedTransfers selected_transfers;
    selected_transfers.reserve(selected_rows.size());

    for (auto &&row : selected_rows)
        selected_transfers.push_back(tab_model_->selected_transfer(static_cast<size_t>(row)));

    emit selection_changed(selected_transfers);
}

} // namespace transfers_tab_internals

// ------- TransfersTab -------

TransfersTab::TransfersTab(QWidget *parent) : QWidget(parent) {
    using namespace woinc::ui::qt::transfers_tab_internals;

    auto btn_panel = new ButtonPanel(this);

    auto scrollable_btn_panel = new QScrollArea(this);
    scrollable_btn_panel->setWidget(btn_panel);
    scrollable_btn_panel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);

    auto tab_model = new TabModel(this);
    auto view = new TableView(tab_model, this);

    auto layout = new QHBoxLayout;
    layout->addWidget(scrollable_btn_panel);
    layout->addWidget(view);
    setLayout(layout);

    connect(this, &TransfersTab::file_transfers_updated, btn_panel, &ButtonPanel::update_transfers);
    connect(this, &TransfersTab::file_transfers_updated, tab_model, &TabModel::update);

    connect(this, &TransfersTab::host_selected,   btn_panel, &ButtonPanel::select_host);
    connect(this, &TransfersTab::host_unselected, btn_panel, &ButtonPanel::unselect_host);

    connect(btn_panel, &ButtonPanel::file_transfer_op_clicked, this, &TransfersTab::file_transfer_op_clicked);
    connect(view, &TableView::selection_changed, btn_panel, &ButtonPanel::update_selected_transfers);
}

}}}
