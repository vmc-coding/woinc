/* ui/qt/dialogs/add_project_wizard.h --
   Written and Copyright (C) 2020 by vmc.

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

#ifndef WOINC_UI_QT_ADD_PROJECT_WIZARD_H_
#define WOINC_UI_QT_ADD_PROJECT_WIZARD_H_

#include <QWizard>
#include <QWizardPage>

#include <woinc/types.h>

namespace woinc { namespace ui { namespace qt {

struct Controller;

namespace add_project_wizard_internals {

class ChooseProjectPage: public QWizardPage {
    Q_OBJECT

    public:
        ChooseProjectPage(Controller &controller, QString host, QWidget *parent = nullptr);

        void initializePage() final;

    signals:
        void all_project_list_loaded();

    private:
        Controller &controller_;
        QString host_;
        AllProjectsList all_projects_;
};

class ProjectAccountPage: public QWizardPage {
    Q_OBJECT

    public:
        ProjectAccountPage(Controller &controller, QString host, QWidget *parent = nullptr);

        void initializePage() final;

    private:
        Controller &controller_;
        QString host_;
        ProjectConfig config_;
};

} // namespace add_project_wizard_internals

class AddProjectWizard: public QWizard {
    Q_OBJECT

    public:
        AddProjectWizard(Controller &controller, QString host, QWidget *parent = nullptr);
};

}}}

#endif
