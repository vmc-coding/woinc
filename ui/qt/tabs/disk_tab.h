/* ui/qt/tabs/disk_tab.h --
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

#ifndef WOINC_UI_QT_TABS_DISK_TAB_H_
#define WOINC_UI_QT_TABS_DISK_TAB_H_

#include <QWidget>
#include <QtCharts/QChartView>

#include "qt/types.h"

namespace woinc { namespace ui { namespace qt {

class DiskTab : public QWidget {
    Q_OBJECT

    public:
        DiskTab(QWidget *parent = nullptr);
        virtual ~DiskTab() = default;

    protected:
        void paintEvent(QPaintEvent *event) override;

    public slots:
        void select_host(QString host);
        void unselect_host(QString host);
        void update_disk_usage(DiskUsage disk_usage);

    private:
        void update_total_chart_(const DiskUsage &disk_usage);
        void update_projects_chart_(const DiskUsage &disk_usage);

    private:
        QString selected_host_;
        DiskUsage disk_usage_;

        QtCharts::QChartView *total_chart_view_ = nullptr;
        QtCharts::QChartView *projects_chart_view_ = nullptr;
};

}}}

#endif
