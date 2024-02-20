/* ui/qt/tabs/statistics_tab.cc --
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

#include "qt/tabs/statistics_tab.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <limits>
#include <set>
#include <utility>

#ifndef NDEBUG
#include <iostream>
#endif

#include <QDateTime>
#include <QGraphicsLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QPushButton>
#include <QScrollArea>

#if QT_VERSION >= 0x060000
#include <QChartView>
#include <QDateTimeAxis>
#include <QLineSeries>
#include <QValueAxis>
#else
#include <QtCharts/QChartView>
#include <QtCharts/QDateTimeAxis>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
QT_CHARTS_USE_NAMESPACE
#endif

namespace {

using namespace woinc::ui::qt::statistics_tab_internals;

enum {
    SECONDS_PER_DAY = 24 * 3600,
    MILLISECONDS_PER_DAY = SECONDS_PER_DAY * 1000,
    LIMIT_BEFORE_SWITCH_TO_GRID = 3
};

typedef std::map<time_t, double> Stats;

QString to_string__(StatisticType type) {
    switch (type) {
        case StatisticType::USER_TOTAL: return QString::fromUtf8("User total");
        case StatisticType::USER_AVG  : return QString::fromUtf8("User average");
        case StatisticType::HOST_TOTAL: return QString::fromUtf8("Host total");
        case StatisticType::HOST_AVG  : return QString::fromUtf8("Host average");
    }
    return QString();
}

Stats to_stats__(const woinc::ui::qt::DailyStatistics &daily_stats, const StatsSelector &selector) {
    Stats stats;
    for (auto &&ds : daily_stats)
        stats.emplace(ds.day * 1000 /*ms since epoch*/, selector(ds));
    return stats;
}

void create_statistics_chart_axes__(QChart &chart) {
    auto x_range = std::make_pair(std::numeric_limits<qreal>::max(), std::numeric_limits<qreal>::min());
    auto y_range = std::make_pair(std::numeric_limits<qreal>::max(), std::numeric_limits<qreal>::min());

    for (const auto si : chart.series()) {
        assert(dynamic_cast<QXYSeries *>(si));
        auto series = static_cast<QXYSeries *>(si);

        auto points = series->points();
        assert(!points.empty());

        x_range.first  = std::min(x_range.first,  points.front().x());
        x_range.second = std::max(x_range.second, points.back().x());

        auto tmp_y_range = std::minmax_element(points.cbegin(), points.cend(),
                                               [](const auto &a, const auto &b) { return a.y() < b.y(); });

        y_range.first  = std::min(y_range.first,  tmp_y_range.first->y());
        y_range.second = std::max(y_range.second, tmp_y_range.second->y());
    }

    auto x_axis = new QDateTimeAxis;
    auto y_axis = new QValueAxis;

    x_axis->setFormat("dd.MMMyy");
    x_axis->setRange(QDateTime::fromMSecsSinceEpoch(static_cast<qint64>(x_range.first)  - MILLISECONDS_PER_DAY),
                     QDateTime::fromMSecsSinceEpoch(static_cast<qint64>(x_range.second) + MILLISECONDS_PER_DAY));

    auto gap = std::max((y_range.second - y_range.first) * 0.05, 1.);
    y_range.first -= gap;
    y_range.second += gap;

    y_axis->setRange(std::max(y_range.first, 0.), y_range.second);
    y_axis->setLabelFormat("%d");

    chart.addAxis(x_axis, Qt::AlignBottom);
    chart.addAxis(y_axis, Qt::AlignLeft);

    for (const auto si : chart.series()) {
        si->attachAxis(x_axis);
        si->attachAxis(y_axis);
    }
}

QLineSeries *create_line_series__(const Stats &stats) {
    auto series = new QLineSeries;
    series->setPointsVisible();

    for (const auto &s : stats)
        series->append(static_cast<qreal>(s.first), s.second);

    return series;
}

void update_chart__(QChart *chart, const Stats &stats, const QString &title = QString()) {
    chart->removeAllSeries();
    for (auto axis : chart->axes()) {
        chart->removeAxis(axis);
        delete axis;
    }

    chart->setTitle(title);
    chart->addSeries(create_line_series__(stats));
    create_statistics_chart_axes__(*chart);
}

void update_chart__(QChart *chart, const std::vector<Stats> &stats, const QString &title = QString()) {
    chart->removeAllSeries();
    for (auto axis : chart->axes()) {
        chart->removeAxis(axis);
        delete axis;
    }

    chart->setTitle(title);
    for (const auto &s : stats)
        chart->addSeries(create_line_series__(s));
    create_statistics_chart_axes__(*chart);
}

Stats sum_stats(const std::vector<Stats> &all_stats) {
    Stats summed_stats;

    for (const auto &stats : all_stats)
        for (const auto &day_entry : stats)
            summed_stats[day_entry.first] = 0;

    // TODO maybe we should interpolate instead of just using the last known value to fill the gaps
    for (const auto &stats : all_stats) {
        auto value = stats.begin()->second;
        for (auto &day_entry : summed_stats) {
            auto iter = stats.find(day_entry.first);
            if (iter != stats.end())
                value = iter->second;
            day_entry.second += value;
        }
    }

    return summed_stats;
}

} // unnamed namespace

namespace woinc { namespace ui { namespace qt { namespace statistics_tab_internals {

// -------- TabState ---------

TabState::TabState(QObject *parent) : QObject(parent) {}

void TabState::init() {
    change_statistics_type(StatisticType::USER_TOTAL);
    change_view_mode(StatisticViewMode::ALL_PROJECTS_SEPARATE);
}

StatisticType TabState::statistic_type() const {
    return statistic_type_;
}

StatisticViewMode TabState::view_mode() const {
    return view_mode_;
}

QStringList TabState::selected_projects() const {
    QStringList list;
    if (view_mode_ == StatisticViewMode::ONE_PROJECT) {
        if (!selected_project_.isEmpty())
            list << selected_project_;
    } else {
#ifndef NDEBUG
        for (auto &&p : selected_projects_)
            assert(stats_.find(p) != stats_.cend());
#endif
        list << selected_projects_;
    }
    return list;
}

ProjectStatistics TabState::selected_project_statistics() const {
    ProjectStatistics selected_stats;

    for (auto &&s : selected_projects()) {
        assert(stats_.find(s) != stats_.cend());
        selected_stats.emplace(s, stats_.at(s));
    }

    return selected_stats;
}

void TabState::update_projects(Projects projects) {
    projects_ = std::move(projects);

    // handle removed projects

    std::set<QString> project_urls;
    for (auto &&p : projects_)
        project_urls.insert(p.project_url);

    for (auto iter = stats_.begin(); iter != stats_.end();) {
        if (project_urls.count(iter->second.project.project_url) > 0) {
            ++ iter;
        } else {
            if (selected_project_ == iter->first)
                selected_project_.clear();
            selected_projects_.removeOne(iter->first);
            emit project_removed(iter->first);
            iter = stats_.erase(iter);
        }
    }
}

void TabState::update_statistics(Statistics new_stats) {
    std::set<QString> old_projects;
    for (auto &&s : stats_)
        old_projects.insert(s.first);

    stats_.clear();

    for (auto &&ps : new_stats) {
        QString project_url = ps.project.project_url;
        auto piter = std::find_if(projects_.cbegin(), projects_.cend(),
                                  [&](auto &&p) { return p.project_url == project_url; });
        if (piter != projects_.cend())
            stats_.emplace(piter->name, std::move(ps));
    }

    std::set<QString> new_projects;
    for (auto &&s : stats_)
        new_projects.insert(s.first);

    for (auto &&s : old_projects) {
        if (new_projects.count(s) > 0)
            continue;
        if (selected_project_ == s)
            selected_project_.clear();
        selected_projects_.removeOne(s);
        emit project_removed(s);
    }

    for (auto &&s : new_projects)
        if (old_projects.count(s) == 0)
            emit project_added(s);

    emit statistics_updated();
}

void TabState::change_project_selection(QStringList projects) {
    if (view_mode_ == StatisticViewMode::ONE_PROJECT) {
        assert(projects.size() <= 1);
        selected_project_ = projects.empty() ? QString() : projects.front();
    } else {
#ifndef NDEBUG
        for (auto &&p : projects)
            assert(stats_.find(p) != stats_.cend());
#endif
        selected_projects_ = std::move(projects);
    }

    emit project_selection_changed();
}

void TabState::change_statistics_type(StatisticType type) {
    std::swap(statistic_type_, type);
    emit statistic_type_changed(type, statistic_type_);
}

void TabState::change_view_mode(StatisticViewMode mode) {
    std::swap(view_mode_, mode);
    emit view_mode_changed(mode, view_mode_);
}

// -------- CommandsWidget ---------

CommandsWidget::CommandsWidget(const TabState &state, QWidget *parent)
    : QWidget(parent), state_(state)
{
    setup_buttons_();
    setup_connections_();
    setup_layout_();
}

void CommandsWidget::setup_buttons_() {
#define WOINC_ADD_BTN(BTN, LABEL) buttons_.emplace(BTN, new QPushButton(LABEL, this))

    WOINC_ADD_BTN(Command::SHOW_USER_TOTAL, "Show user total");
    WOINC_ADD_BTN(Command::SHOW_USER_AVG  , "Show user average");
    WOINC_ADD_BTN(Command::SHOW_HOST_TOTAL, "Show host total");
    WOINC_ADD_BTN(Command::SHOW_HOST_AVG  , "Show host average");

    WOINC_ADD_BTN(Command::PREV_PROJECT       , "< Previous project");
    WOINC_ADD_BTN(Command::NEXT_PROJECT       , "Next project >");
    WOINC_ADD_BTN(Command::TOGGLE_PROJECT_LIST, "Hide project list");

    WOINC_ADD_BTN(Command::SHOW_ONE_PROJECT   , "One project");
    WOINC_ADD_BTN(Command::SHOW_ALL_SEPERATE  , "All projects (separate)");
    WOINC_ADD_BTN(Command::SHOW_ALL_TOGETHER  , "All projects (together)");
    WOINC_ADD_BTN(Command::SHOW_ALL_SUM       , "All projects (sum)");

#undef WOINC_ADD_BTN
}

void CommandsWidget::setup_connections_() {
#define WOINC_CONNECT_BTN(BTN, SIGNAL) connect(buttons_[BTN], &QPushButton::released, this, [this]() { emit SIGNAL; })

    WOINC_CONNECT_BTN(Command::SHOW_USER_TOTAL, statistic_type_changed(StatisticType::USER_TOTAL));
    WOINC_CONNECT_BTN(Command::SHOW_USER_AVG  , statistic_type_changed(StatisticType::USER_AVG));
    WOINC_CONNECT_BTN(Command::SHOW_HOST_TOTAL, statistic_type_changed(StatisticType::HOST_TOTAL));
    WOINC_CONNECT_BTN(Command::SHOW_HOST_AVG  , statistic_type_changed(StatisticType::HOST_AVG));

    WOINC_CONNECT_BTN(Command::PREV_PROJECT       , previous_project_wanted());
    WOINC_CONNECT_BTN(Command::NEXT_PROJECT       , next_project_wanted());
    WOINC_CONNECT_BTN(Command::TOGGLE_PROJECT_LIST, change_project_list_visibility_(!show_project_list_));

    WOINC_CONNECT_BTN(Command::SHOW_ONE_PROJECT , view_mode_changed(StatisticViewMode::ONE_PROJECT));
    WOINC_CONNECT_BTN(Command::SHOW_ALL_SEPERATE, view_mode_changed(StatisticViewMode::ALL_PROJECTS_SEPARATE));
    WOINC_CONNECT_BTN(Command::SHOW_ALL_TOGETHER, view_mode_changed(StatisticViewMode::ALL_PROJECTS_TOGETHER));
    WOINC_CONNECT_BTN(Command::SHOW_ALL_SUM     , view_mode_changed(StatisticViewMode::ALL_PROJECTS_SUM));

#undef WOINC_CONNECT_BTN

    connect(this, &CommandsWidget::statistic_type_changed, &state_, &TabState::change_statistics_type);
    connect(this, &CommandsWidget::view_mode_changed,      &state_, &TabState::change_view_mode);

    connect(&state_, &TabState::statistic_type_changed, this, &CommandsWidget::change_statistics_type_);
    connect(&state_, &TabState::view_mode_changed,      this, &CommandsWidget::change_view_mode_);
}

void CommandsWidget::setup_layout_() {
    auto cmds_layout = new QVBoxLayout;
    cmds_layout->addWidget(buttons_[Command::SHOW_USER_TOTAL]);
    cmds_layout->addWidget(buttons_[Command::SHOW_USER_AVG]);
    cmds_layout->addWidget(buttons_[Command::SHOW_HOST_TOTAL]);
    cmds_layout->addWidget(buttons_[Command::SHOW_HOST_AVG]);

    auto cmds = new QGroupBox("Commands");
    cmds->setLayout(cmds_layout);

    auto projects_layout = new QVBoxLayout;
    projects_layout->addWidget(buttons_[Command::PREV_PROJECT]);
    projects_layout->addWidget(buttons_[Command::NEXT_PROJECT]);
    projects_layout->addWidget(buttons_[Command::TOGGLE_PROJECT_LIST]);

    auto projects = new QGroupBox("Project");
    projects->setLayout(projects_layout);

    auto modes_layout = new QVBoxLayout;
    modes_layout->addWidget(buttons_[Command::SHOW_ONE_PROJECT]);
    modes_layout->addWidget(buttons_[Command::SHOW_ALL_SEPERATE]);
    modes_layout->addWidget(buttons_[Command::SHOW_ALL_TOGETHER]);
    modes_layout->addWidget(buttons_[Command::SHOW_ALL_SUM]);

    auto modes = new QGroupBox("Mode view");
    modes->setLayout(modes_layout);

    auto layout = new QVBoxLayout;
    layout->addWidget(cmds);
    layout->addWidget(projects);
    layout->addWidget(modes);
    setLayout(layout);
}

void CommandsWidget::change_statistics_type_(StatisticType /*old_type*/, StatisticType new_type) {
    buttons_[Command::SHOW_USER_TOTAL]->setDisabled(new_type == StatisticType::USER_TOTAL);
    buttons_[Command::SHOW_USER_AVG  ]->setDisabled(new_type == StatisticType::USER_AVG);
    buttons_[Command::SHOW_HOST_TOTAL]->setDisabled(new_type == StatisticType::HOST_TOTAL);
    buttons_[Command::SHOW_HOST_AVG  ]->setDisabled(new_type == StatisticType::HOST_AVG);
}

void CommandsWidget::change_view_mode_(StatisticViewMode /*old_mode*/, StatisticViewMode new_mode) {
    buttons_[Command::PREV_PROJECT]->setDisabled(new_mode != StatisticViewMode::ONE_PROJECT);
    buttons_[Command::NEXT_PROJECT]->setDisabled(new_mode != StatisticViewMode::ONE_PROJECT);

    buttons_[Command::SHOW_ONE_PROJECT ]->setDisabled(new_mode == StatisticViewMode::ONE_PROJECT);
    buttons_[Command::SHOW_ALL_SEPERATE]->setDisabled(new_mode == StatisticViewMode::ALL_PROJECTS_SEPARATE);
    buttons_[Command::SHOW_ALL_TOGETHER]->setDisabled(new_mode == StatisticViewMode::ALL_PROJECTS_TOGETHER);
    buttons_[Command::SHOW_ALL_SUM     ]->setDisabled(new_mode == StatisticViewMode::ALL_PROJECTS_SUM);
}

void CommandsWidget::change_project_list_visibility_(bool visibility) {
    show_project_list_ = visibility;
    auto btn = buttons_[Command::TOGGLE_PROJECT_LIST];

    if (show_project_list_) {
        btn->setText(QString::fromUtf8("Hide project list"));
        emit project_list_unhidden();
    } else {
        btn->setText(QString::fromUtf8("Show project list"));
        emit project_list_hidden();
    }
}

// -------- ChartsWidget ---------

ChartsWidget::ChartsWidget(const TabState &state, QWidget *parent)
    : QWidget(parent), state_(state), header_(new QLabel), charts_(new QWidget)
{
    header_->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
    header_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);

    charts_->setLayout(new QGridLayout);
    charts_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    auto layout = new QVBoxLayout;
    layout->addWidget(header_);
    layout->addWidget(charts_);
    setLayout(layout);

    connect(&state_, &TabState::project_selection_changed, this, &ChartsWidget::update_view_);
    connect(&state_, &TabState::statistic_type_changed,    this, &ChartsWidget::change_statistics_type_);
    connect(&state_, &TabState::view_mode_changed,         this, &ChartsWidget::change_view_mode_);
    connect(&state_, &TabState::statistics_updated,        this, &ChartsWidget::update_view_);
}

void ChartsWidget::change_statistics_type_(StatisticType /*old_type*/, StatisticType new_type) {
    decltype(&DailyStatistic::user_total_credit) select_ptr = nullptr;

    switch (new_type) {
        case StatisticType::USER_TOTAL: select_ptr = &DailyStatistic::user_total_credit ; break;
        case StatisticType::USER_AVG  : select_ptr = &DailyStatistic::user_expavg_credit; break;
        case StatisticType::HOST_TOTAL: select_ptr = &DailyStatistic::host_total_credit ; break;
        case StatisticType::HOST_AVG  : select_ptr = &DailyStatistic::host_expavg_credit; break;
    }

    assert(select_ptr);
    stats_selector_ = std::mem_fn(select_ptr);

    update_view_();
}

void ChartsWidget::change_view_mode_(StatisticViewMode old_mode, StatisticViewMode new_mode) {
    bool one_project_before = old_mode == StatisticViewMode::ONE_PROJECT;
    bool one_project_after = new_mode == StatisticViewMode::ONE_PROJECT;

    if (one_project_before != one_project_after)
        reset_charts_();

    update_view_();
}

void ChartsWidget::update_view_() {
    auto stats = state_.selected_project_statistics();

    setUpdatesEnabled(false);
    update_header_(stats);
    update_chart_views_(stats);
    update_charts_(stats);
    setUpdatesEnabled(true);
}

void ChartsWidget::update_header_(const ProjectStatistics &stats) {
    QStringList lines;
    lines << QString("<b>%1</b>").arg(to_string__(state_.statistic_type()));

    if (state_.view_mode() == StatisticViewMode::ONE_PROJECT) {
        assert(stats.size() <= 1);
        auto siter = stats.cbegin();

        auto last_updated = std::chrono::system_clock::now() -
            (stats.empty() ? std::chrono::system_clock::now() :
             std::chrono::system_clock::from_time_t(siter->second.daily_statistics.back().day));
        auto last_updated_days = static_cast<int>(std::floor(
                std::chrono::duration_cast<std::chrono::seconds>(last_updated).count() / SECONDS_PER_DAY));

        lines << QString("<i>Project: %1</i>").arg(stats.empty() ? QString() : siter->second.project.name);
        lines << QString("<i>Account: %1</i>").arg(stats.empty() ? QString() : siter->second.project.account);
        lines << QString("<i>Team: %1</i>")   .arg(stats.empty() ? QString() : siter->second.project.team);
        lines << QString("<i>Last update: %1 days ago</i>").arg(last_updated_days);
    }

    assert(header_);
    header_->setText(lines.join("<br>"));
}

void ChartsWidget::update_chart_views_(const ProjectStatistics &stats) {
    assert(charts_->layout());

    const int needed = stats.empty() ? 0 :
        state_.view_mode() == StatisticViewMode::ALL_PROJECTS_SEPARATE ? static_cast<int>(stats.size()) : 1;
    int current = charts_->layout()->count();

    if (current == needed)
        return;

    // reset the widget due to layout change
    if (current > 0) {
        reset_charts_();
        current = 0;
    }

    assert(dynamic_cast<QGridLayout *>(charts_->layout()));
    QGridLayout *grid = static_cast<QGridLayout *>(charts_->layout());

    int column_step = stats.size() > LIMIT_BEFORE_SWITCH_TO_GRID ? 1 : 0;
    int row = 0, column = 0;

    while (current++ < needed) {
        auto chart = new QChart;
        chart->legend()->hide();
        chart->layout()->setContentsMargins(0, 0, 0, 0);

        auto chart_view = new QChartView(chart);
        chart_view->setRenderHint(QPainter::Antialiasing);
        chart_view->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);

        grid->addWidget(chart_view, row, column);

        column = (column + column_step) % 2;
        row += 1 - column;
    }

    assert(grid->count() == needed);
}

void ChartsWidget::update_charts_(const ProjectStatistics &project_statistics) {
    if (project_statistics.empty())
        return;

    std::map<QString, Stats> selected_stats;

    for (auto &&ps : project_statistics)
        selected_stats[ps.first] = to_stats__(ps.second.daily_statistics, stats_selector_);

    if (state_.view_mode() == StatisticViewMode::ALL_PROJECTS_SEPARATE) {
        assert(static_cast<size_t>(charts_->layout()->count()) == selected_stats.size());

        int item_idx = 0;
        for (auto &&stats : selected_stats) {
            assert(item_idx < charts_->layout()->count());
            assert(dynamic_cast<QChartView *>(charts_->layout()->itemAt(item_idx)->widget()));

            auto chart_view = static_cast<QChartView *>(charts_->layout()->itemAt(item_idx)->widget());
            update_chart__(chart_view->chart(), stats.second, stats.first);

            item_idx ++;
        }
    } else {
        assert(static_cast<size_t>(charts_->layout()->count()) == 1);
        assert(dynamic_cast<QChartView *>(charts_->layout()->itemAt(0)->widget()));

        auto chart_view = static_cast<QChartView *>(charts_->layout()->itemAt(0)->widget());
        auto view_mode = state_.view_mode();

        if (view_mode == StatisticViewMode::ONE_PROJECT) {
            assert(selected_stats.size() == 1);
            update_chart__(chart_view->chart(), selected_stats.cbegin()->second);
        } else {
            std::vector<Stats> stats;
            stats.reserve(selected_stats.size());

            std::transform(selected_stats.cbegin(), selected_stats.cend(),
                           std::back_inserter(stats),
                           [](const auto &it) { return it.second; });

            if (view_mode == StatisticViewMode::ALL_PROJECTS_TOGETHER)
                update_chart__(chart_view->chart(), stats);
            else
                update_chart__(chart_view->chart(), sum_stats(stats));
        }
    }
}

void ChartsWidget::reset_charts_() {
    auto new_charts = new QWidget;
    new_charts->setLayout(new QGridLayout);
    new_charts->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    delete layout()->replaceWidget(charts_, new_charts);
    delete std::exchange(charts_, new_charts);
}

// -------- ProjectsWidget ---------

ProjectsWidget::ProjectsWidget(const TabState &state, QWidget *parent)
    : QListWidget(parent), state_(state)
{
    setSelectionMode(QAbstractItemView::MultiSelection);

    connect(this, &ProjectsWidget::selection_changed, &state_, &TabState::change_project_selection);

    connect(&state_, &TabState::project_added,     this, &ProjectsWidget::add_project);
    connect(&state_, &TabState::project_removed,   this, &ProjectsWidget::remove_project);
    connect(&state_, &TabState::view_mode_changed, this, &ProjectsWidget::change_view_mode_);

    connect(this, &ProjectsWidget::itemSelectionChanged, this, &ProjectsWidget::item_selection_changed_);
}

QSize ProjectsWidget::sizeHint() const {
    QFontMetrics font_metrics(font());

    int width = font_metrics.boundingRect(QString::fromUtf8("XXXXXXXXXX")).width();

    for (int i = 0; i < count(); ++i) {
        auto br = font_metrics.boundingRect(item(i)->text() + "XX");
        width = std::max(width, br.width());
    }

    return QSize(width, QListWidget::sizeHint().height());
}

void ProjectsWidget::select_previous_project() {
    move_selection_(-1, -1);
}

void ProjectsWidget::select_next_project() {
    move_selection_(0, 1);
}

void ProjectsWidget::add_project(QString project) {
    addItem(project);
    sortItems();
    updateGeometry();
}

void ProjectsWidget::remove_project(QString project) {
    for (auto item : findItems(project, Qt::MatchExactly)) {
        removeItemWidget(item);
        delete item;
    }
    updateGeometry();
}

void ProjectsWidget::change_view_mode_(StatisticViewMode old_mode, StatisticViewMode new_mode) {
    bool one_project_before = old_mode == StatisticViewMode::ONE_PROJECT;
    bool one_project_after  = new_mode == StatisticViewMode::ONE_PROJECT;

    if (one_project_before == one_project_after)
        return;

    ignore_selection_change_ = true;

    clearSelection();

    setSelectionMode(one_project_after ? QAbstractItemView::SingleSelection : QAbstractItemView::MultiSelection);

    for (auto &&name : state_.selected_projects())
        for (auto item : findItems(name, Qt::MatchExactly))
            item->setSelected(true);

    ignore_selection_change_ = false;

    emit itemSelectionChanged();
}

void ProjectsWidget::item_selection_changed_() {
    if (ignore_selection_change_)
        return;

    QStringList projects;
    for (auto item : selectedItems())
        projects << item->text();

    emit selection_changed(std::move(projects));
}

void ProjectsWidget::move_selection_(int default_if_empty, int step) {
    auto selected_idxs = selectedIndexes();

    assert(state_.view_mode() == StatisticViewMode::ONE_PROJECT);
    assert(selected_idxs.size() <= 1);

    ignore_selection_change_ = true;

    for (auto item : selectedItems())
        item->setSelected(false);

    if (count() > 0) {
        auto idx = selected_idxs.empty() ? default_if_empty : (selected_idxs.front().row() + step);
        idx = (idx + count()) % count();

        assert(idx >= 0);
        assert(idx < count());

        item(idx)->setSelected(true);
    }

    ignore_selection_change_ = false;

    emit itemSelectionChanged();
}

} // namespace statistics_tab_internals

// -------- StatisticsTab ---------

StatisticsTab::StatisticsTab(QWidget *parent)
    : QWidget(parent)
{
    using namespace statistics_tab_internals;

    auto state = new TabState(this);

    auto cmds = new CommandsWidget(*state, this);
    auto charts = new ChartsWidget(*state, this);
    auto projects = new ProjectsWidget(*state, this);

    auto scrollable_cmds = new QScrollArea(this);
    scrollable_cmds->setWidget(cmds);

    scrollable_cmds->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Ignored);
    charts->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Ignored);
    projects->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Ignored);

    auto layout = new QHBoxLayout;
    layout->addWidget(scrollable_cmds);
    layout->addWidget(charts);
    layout->addWidget(projects);
    setLayout(layout);

    connect(cmds, &CommandsWidget::previous_project_wanted, projects, &ProjectsWidget::select_previous_project);
    connect(cmds, &CommandsWidget::next_project_wanted,     projects, &ProjectsWidget::select_next_project);
    connect(cmds, &CommandsWidget::project_list_unhidden,   projects, &ProjectsWidget::show);
    connect(cmds, &CommandsWidget::project_list_hidden,     projects, &ProjectsWidget::hide);

    connect(this, &StatisticsTab::projects_updated,   state, &TabState::update_projects);
    connect(this, &StatisticsTab::statistics_updated, state, &TabState::update_statistics);

    state->init();
}

}}}
