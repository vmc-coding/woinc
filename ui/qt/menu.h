/* ui/qt/menu.h --
   Written and Copyright (C) 2017-2021 by vmc.

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

#ifndef WOINC_QT_MENU_H_
#define WOINC_QT_MENU_H_

#include <QMenu>

#include "qt/defs.h"
#include "qt/types.h"

namespace woinc { namespace ui { namespace qt {

// helper class while we support only one selected host
class HostAwareMenu : public QMenu {
    Q_OBJECT

    public:
        HostAwareMenu(const QString &title, QWidget *parent = nullptr);
#ifndef NDEBUG
        virtual ~HostAwareMenu();
        void connected();
#else
        virtual ~HostAwareMenu() = default;
#endif

    public slots:
        void select_host(QString host);
        void unselect_host(QString host);

    protected:
        virtual void host_selected_();
        virtual void host_unselected_();

    protected:
        QString selected_host_;
#ifndef NDEBUG
        bool connected_ = false;
#endif
};

class FileMenu : public QMenu {
    Q_OBJECT

    public:
        FileMenu(const QString &title, QWidget *parent = nullptr);
        virtual ~FileMenu() = default;

    signals:
        void to_quit();
        void computer_to_be_selected();
};

class ViewMenu : public QMenu {
    Q_OBJECT

    public:
        ViewMenu(const QString &title, QWidget *parent = nullptr);
        virtual ~ViewMenu() = default;

    signals:
        void current_tab_to_be_changed(TAB tab);
};

class ActivityMenu : public HostAwareMenu {
    Q_OBJECT

    public:
        ActivityMenu(const QString &title, QWidget *parent = nullptr);
        virtual ~ActivityMenu() = default;

    public slots:
        void update_run_modes(RunModes modes);

    private:
        QAction *run_always_;
        QAction *run_auto_;
        QAction *run_never_;

        QAction *gpu_always_;
        QAction *gpu_auto_;
        QAction *gpu_never_;

        QAction *network_always_;
        QAction *network_auto_;
        QAction *network_never_;

    signals:
        void run_mode_set(QString host, RunMode mode);
        void gpu_mode_set(QString host, RunMode mode);
        void network_mode_set(QString host, RunMode mode);
};

class OptionsMenu : public HostAwareMenu {
    Q_OBJECT

    public:
        OptionsMenu(const QString &title, QWidget *parent = nullptr);
        virtual ~OptionsMenu() = default;

    signals:
        void computation_preferences_to_be_shown(QString host);
        void config_files_to_be_read(QString host);
};

class ToolsMenu : public HostAwareMenu {
    Q_OBJECT

    public:
        ToolsMenu(const QString &title, QWidget *parent = nullptr);
        virtual ~ToolsMenu() = default;

    signals:
        void add_project_wizard_to_be_shown(QString host);
};

class HelpMenu : public QMenu {
    Q_OBJECT

    public:
        HelpMenu(const QString &title, QWidget *parent = nullptr);
        virtual ~HelpMenu() = default;
};

}}}
#endif
