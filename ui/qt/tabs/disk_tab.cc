/* ui/qt/tabs/disk_tab.cc --
   Written and Copyright (C) 2018-2024 by vmc.

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

#include "qt/tabs/disk_tab.h"

#include <cassert>
#ifndef NDEBUG
#include <iostream>
#endif

#include <QGraphicsLayout>
#include <QHBoxLayout>

#if QT_VERSION >= 0x060000
#include <QChartView>
#include <QLegendMarker>
#include <QPieSeries>
#else
#include <QtCharts/QChartView>
#include <QtCharts/QLegendMarker>
#include <QtCharts/QPieSeries>
QT_CHARTS_USE_NAMESPACE
#endif

#include "qt/utils.h"

namespace {

// Why the fuck do we need to guess the dimensions instead of just computing them?
QRect guess_legend_dimensions(QLegend *legend) {
    int width = 0;
    int height = 0;

    QFontMetrics font_metrics(legend->font());

    for (auto marker : legend->markers()) {
        auto br = font_metrics.boundingRect(marker->label());
        width = std::max(width, br.width());

        // guess the height + margin
        if (height == 0)
            height = 2 * br.height();
        height += static_cast<int>(1.5 * br.height());
    }

    // guess the width of the marker icon + some margin
    width += font_metrics.boundingRect(QString::fromUtf8("XXXXXX")).width();

    return QRect(0, 0, width, height);
}

}

namespace woinc { namespace ui { namespace qt {

DiskTab::DiskTab(QWidget *parent) : QWidget(parent) {
    total_chart_view_ = new QChartView(this);
    total_chart_view_->setRenderHint(QPainter::Antialiasing);

    total_chart_view_->setChart(new QChart());
    total_chart_view_->chart()->setTitle(QString::fromUtf8("Total disk usage"));
    total_chart_view_->chart()->legend()->detachFromChart();
    total_chart_view_->chart()->legend()->setVisible(true);
    //total_chart_view_->chart()->legend()->setBackgroundVisible(true);
    total_chart_view_->chart()->layout()->setContentsMargins(0, 0, 0, 0);

    projects_chart_view_ = new QChartView(this);
    projects_chart_view_->setRenderHint(QPainter::Antialiasing);

    projects_chart_view_->setChart(new QChart());
    projects_chart_view_->chart()->setTitle(QString::fromUtf8("Disk usage by BOINC projects"));
    projects_chart_view_->chart()->legend()->detachFromChart();
    projects_chart_view_->chart()->legend()->setVisible(true);
    //projects_chart_view_->chart()->legend()->setBackgroundVisible(true);
    projects_chart_view_->chart()->layout()->setContentsMargins(0, 0, 0, 0);

    auto layout = new QHBoxLayout;
    layout->addWidget(total_chart_view_);
    layout->addWidget(projects_chart_view_);

    setLayout(layout);
}

void DiskTab::paintEvent(QPaintEvent *event) {
    QWidget::paintEvent(event);

    {
        auto legend = total_chart_view_->chart()->legend();
        QRectF rect(guess_legend_dimensions(legend));
        legend->setGeometry(QRectF(0, event->rect().height() - rect.height(), rect.width(), rect.height()));
    }

    {
        auto legend = projects_chart_view_->chart()->legend();
        QRectF rect(guess_legend_dimensions(legend));
        legend->setGeometry(0, event->rect().height() - rect.height(), rect.width(), rect.height());
    }
}

void DiskTab::select_host(QString host) {
    selected_host_ = std::move(host);
    update_disk_usage({});
}

void DiskTab::unselect_host(QString /*host*/) {
    selected_host_.clear();
    update_disk_usage({});
}

void DiskTab::update_disk_usage(DiskUsage disk_usage) {
    update_total_chart_(disk_usage);
    update_projects_chart_(disk_usage);
}

void DiskTab::update_total_chart_(const DiskUsage &usage) {
    QPieSeries *series = new QPieSeries();
    series->setPieSize(.9);

    series->append(QString::fromUtf8("used by BOINC: ")
                   + size_as_string(usage.used),
                   usage.used);
    series->append(QString::fromUtf8("free, available to BOINC: ")
                   + size_as_string(usage.available),
                   usage.available);

    if (usage.not_available > 0)
        series->append(QString::fromUtf8("free, not available to BOINC: ")
                       + size_as_string(usage.not_available),
                       usage.not_available);

    series->append(QString::fromUtf8("used by other programs: ")
                   + size_as_string(usage.others),
                   usage.others);

    // TODO compare series with existing series and only update if there are changes
    bool update = true;

    if (update) {
        total_chart_view_->chart()->removeAllSeries();
        total_chart_view_->chart()->addSeries(series);
    } else {
        delete series;
    }
}

void DiskTab::update_projects_chart_(const DiskUsage &usage) {
    QPieSeries *series = new QPieSeries();
    series->setPieSize(.9);

    for (const auto &project : usage.projects)
        if (project.usage > 0)
            series->append(project.name + QString::fromUtf8(": ") + size_as_string(project.usage), project.usage);

    if (series->count() == 0)
        series->append(QString::fromUtf8("no projects: 0 bytes used"), 1);

    // TODO compare series with existing series and only update if there are changes
    bool update = true;

    if (update) {
        projects_chart_view_->chart()->removeAllSeries();
        projects_chart_view_->chart()->addSeries(series);
    } else {
        delete series;
    }
}

}}}
