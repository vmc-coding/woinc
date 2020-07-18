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

namespace add_project_wizard_internals {

class ChooseProjectPage: public QWizardPage {
    Q_OBJECT

    public:
        ChooseProjectPage(AllProjectsList all_projects, QWidget *parent = nullptr);
};

class ProjectAccountPage: public QWizardPage {
    Q_OBJECT

    public:
        ProjectAccountPage(QWidget *parent = nullptr);
};

} // namespace add_project_wizard_internals

struct Controller;

class AddProjectWizard: public QWizard {
    Q_OBJECT

    public:
        AddProjectWizard(AllProjectsList all_projects, QWidget *parent = nullptr);
};

}}}

#endif
