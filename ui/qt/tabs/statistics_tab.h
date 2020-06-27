/* ui/qt/tabs/statistics_tab.h --
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

#ifndef WOINC_UI_QT_TABS_STATISTICS_TAB_H_
#define WOINC_UI_QT_TABS_STATISTICS_TAB_H_

#include <functional>
#include <map>

#include <QAbstractButton>
#include <QLabel>
#include <QListWidget>
#include <QStringList>
#include <QWidget>

#include "qt/types.h"

namespace woinc { namespace ui { namespace qt { namespace statistics_tab_internals {

enum class StatisticType {
    USER_TOTAL,
    USER_AVG,
    HOST_TOTAL,
    HOST_AVG
};

enum class StatisticViewMode {
    ONE_PROJECT,
    ALL_PROJECTS_SEPARATE,
    ALL_PROJECTS_TOGETHER,
    ALL_PROJECTS_SUM
};

// TODO do we really need a map here? If so, let's use the master url as key (better safe than sorry ..);
// TODO rename this to avoid name clashes
typedef std::map<QString, woinc::ui::qt::ProjectStatistics> ProjectStatistics;

typedef decltype(std::mem_fn(&DailyStatistic::user_total_credit)) StatsSelector;

// to have the same state in the three widgets and avoiding circular dependendies we'll define a tab internal state;
// this state will also handle the updates from the model and dispatch signals to inform the widgets;
class TabState : public QObject {
    Q_OBJECT

    public:
        TabState(QObject *parent = nullptr);
        virtual ~TabState() = default;

        // propagates the initial state
        void init();

    signals:
        void project_added(QString project);
        void project_removed(QString project);
        void project_selection_changed();
        void statistic_type_changed(StatisticType old_value, StatisticType new_value);
        void view_mode_changed(StatisticViewMode old_value, StatisticViewMode new_value);
        void statistics_updated();

    public:
        StatisticType statistic_type() const;
        StatisticViewMode view_mode() const;

        QStringList selected_projects() const;
        ProjectStatistics selected_project_statistics() const;

    public slots:
        void update_projects(Projects projects);
        void update_statistics(Statistics stats);

        void change_project_selection(QStringList projects);
        void change_statistics_type(StatisticType type);
        void change_view_mode(StatisticViewMode mode);

    private:
        StatisticType statistic_type_ = StatisticType::USER_TOTAL;
        StatisticViewMode view_mode_ = StatisticViewMode::ALL_PROJECTS_SEPARATE;

        QString selected_project_;
        QStringList selected_projects_;

        Projects projects_;
        ProjectStatistics stats_;
};

class CommandsWidget : public QWidget {
    Q_OBJECT

    public:
        CommandsWidget(const TabState &state, QWidget *parent = nullptr);
        virtual ~CommandsWidget() = default;

    signals:
        void statistic_type_changed(StatisticType type);
        void previous_project_wanted();
        void next_project_wanted();
        void project_list_unhidden();
        void project_list_hidden();
        void view_mode_changed(StatisticViewMode mode);

    private:
        void setup_buttons_();
        void setup_connections_();
        void setup_layout_();

    private slots:
        void change_project_list_visibility_(bool visibility);
        void change_statistics_type_(StatisticType old_type, StatisticType new_type);
        void change_view_mode_(StatisticViewMode old_mode, StatisticViewMode new_mode);

    private:
        enum class Command {
            SHOW_USER_TOTAL,
            SHOW_USER_AVG,
            SHOW_HOST_TOTAL,
            SHOW_HOST_AVG,
            PREV_PROJECT,
            NEXT_PROJECT,
            TOGGLE_PROJECT_LIST,
            SHOW_ONE_PROJECT,
            SHOW_ALL_SEPERATE,
            SHOW_ALL_TOGETHER,
            SHOW_ALL_SUM
        };

        const TabState &state_;

        std::map<Command, QAbstractButton *> buttons_;
        bool show_project_list_ = true;
};

class ChartsWidget : public QWidget {
    Q_OBJECT

    public:
        ChartsWidget(const TabState &state, QWidget *parent = nullptr);
        virtual ~ChartsWidget() = default;

    private slots:
        void change_statistics_type_(StatisticType old_type, StatisticType new_type);
        void change_view_mode_(StatisticViewMode old_mode, StatisticViewMode new_mode);
        void update_view_();

    private:
        void update_header_(const ProjectStatistics &stats);
        void update_chart_views_(const ProjectStatistics &stats);
        void update_charts_(const ProjectStatistics &stats);

        void reset_charts_();

    private:
        const TabState &state_;

        StatsSelector stats_selector_ = std::mem_fn(&DailyStatistic::user_total_credit);

        QLabel *header_ = nullptr;
        QWidget *charts_ = nullptr;
};

class ProjectsWidget : public QListWidget {
    Q_OBJECT

    public:
        ProjectsWidget(const TabState &state, QWidget *parent = nullptr);
        virtual ~ProjectsWidget() = default;

        virtual QSize sizeHint() const;

    signals:
        void selection_changed(QStringList projects);

    public slots:
        void select_previous_project();
        void select_next_project();

    private slots:
        void add_project(QString project);
        void remove_project(QString project);
        void change_view_mode_(StatisticViewMode old_mode, StatisticViewMode new_mode);
        void item_selection_changed_();

    private:
        void move_selection_(int default_if_empty, int idx);

    private:
        const TabState &state_;

        bool ignore_selection_change_ = false;

        QString selected_project_;
        QStringList selected_projects_;
};

} // namspace statistics_tab_internals

class StatisticsTab : public QWidget {
    Q_OBJECT

    public:
        StatisticsTab(QWidget *parent = nullptr);
        virtual ~StatisticsTab() = default;

    signals:
        // delegated signals from the model to the internal widgets

        void projects_updated(woinc::ui::qt::Projects projects);
        void statistics_updated(woinc::ui::qt::Statistics statistics);
};

}}}

#endif
