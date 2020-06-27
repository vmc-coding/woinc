/* ui/qt/tabs/tab_model_updater.h --
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

#ifndef WOINC_UI_QT_TAB_MODEL_UPDATER_H_
#define WOINC_UI_QT_TAB_MODEL_UPDATER_H_

#include <cassert>
#include <iterator>

#include <QModelIndex>

namespace woinc { namespace ui { namespace qt {

template<typename TAB_MODEL, typename TARGET_CONTAINER, typename SOURCE_CONTAINER, typename DATA_ITEM_COMP>
void update_tab_model(TAB_MODEL &model,
                      TARGET_CONTAINER &current_data,
                      SOURCE_CONTAINER new_data,
                      DATA_ITEM_COMP comp) {
    if (new_data.empty()) {
        if (!current_data.empty()) {
            emit model.beginRemoveRows(QModelIndex(), 0, static_cast<int>(current_data.size() - 1));
            current_data.clear();
            emit model.endRemoveRows();
        }
        return;
    }

    auto current_data_iter = current_data.begin();
    auto new_data_iter = new_data.begin();

    auto do_remove = [&]() {
        auto end_iter = current_data_iter;

        while (++end_iter != current_data.end() && comp(*end_iter, *new_data_iter) < 0)
            ;

        int start_idx = static_cast<int>(std::distance(current_data.begin(), current_data_iter));
        int end_idx = static_cast<int>(start_idx + std::distance(current_data_iter, end_iter) - 1);
        assert(end_idx >= start_idx);
        assert(static_cast<size_t>(end_idx) < current_data.size());

        emit model.beginRemoveRows(QModelIndex(), start_idx, end_idx);
        current_data_iter = current_data.erase(current_data_iter, end_iter);
        emit model.endRemoveRows();
    };

    auto do_update = [&]() {
        if (*current_data_iter != *new_data_iter) {
            *current_data_iter = *new_data_iter;
            int row_idx = static_cast<int>(std::distance(current_data.begin(), current_data_iter));
            emit model.dataChanged(model.createIndex(row_idx, 0),
                                   model.createIndex(row_idx, model.columnCount() - 1));
        }
        current_data_iter ++;
        new_data_iter ++;
    };

    auto do_insert = [&]() {
        int row_idx = static_cast<int>(std::distance(current_data.begin(), current_data_iter));
        emit model.beginInsertRows(QModelIndex(), row_idx, row_idx);
        current_data_iter = current_data.insert(current_data_iter, std::move(*new_data_iter));
        emit model.endInsertRows();
        current_data_iter ++;
        new_data_iter ++;
    };

    // adapt changes from the new data set to the current data set
    while (current_data_iter != current_data.end() && new_data_iter != new_data.end()) {
        int result = comp(*current_data_iter, *new_data_iter);
        if (result < 0) {
            do_remove();
        } else if (result == 0) {
            do_update();
        } else {
            do_insert();
        }
    }

    // if we reached the end of new data before the end of current data, remove all remaining items
    if (current_data_iter != current_data.end()) {
        int start_idx = static_cast<int>(std::distance(current_data.begin(), current_data_iter));
        int end_idx = static_cast<int>(current_data.size() - 1);
        assert(end_idx >= start_idx);

        model.beginRemoveRows(QModelIndex(), start_idx, end_idx);
        current_data_iter = current_data.erase(current_data_iter, current_data.end());
        model.endRemoveRows();
    }

    assert(current_data_iter == current_data.end());

    // append all remaining new items to the data set
    if (new_data_iter != new_data.end()) {
        assert(current_data.size() < new_data.size());
        emit model.beginInsertRows(QModelIndex(),
                                   static_cast<int>(current_data.size()),
                                   static_cast<int>(new_data.size() - 1));

        while (new_data_iter != new_data.end()) {
            current_data.push_back(std::move(*new_data_iter));
            new_data_iter ++;
        }

        emit model.endInsertRows();
    }

    assert(current_data.size() == new_data.size());
}

}}}

#endif
