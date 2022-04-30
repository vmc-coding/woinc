/* ui/qt/dialogs/add_project_wizard.h --
   Written and Copyright (C) 2020, 2021 by vmc.

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

#include <chrono>
#include <future>
#include <memory>

#include <QWidget>
#include <QWizard>
#include <QWizardPage>
#include <QVariant>

#include <woinc/types.h>

struct QLabel;
struct QTimer;

namespace woinc { namespace ui { namespace qt {

struct Controller;
struct SimpleProgressAnimation;

namespace add_project_wizard_internals {

class ChooseProjectPage: public QWizardPage {
    Q_OBJECT

    public:
        ChooseProjectPage(Controller &controller, QString host, QWidget *parent = nullptr);

        void initializePage() final;
        bool isComplete() const final;

    signals:
        void all_project_list_loaded();

    private:
        Controller &controller_;
        QString host_;
        AllProjectsList all_projects_;

        SimpleProgressAnimation *progress_animation_;
};

class ProjectAccountPage: public QWizardPage {
    Q_OBJECT

    public:
        ProjectAccountPage(Controller &controller, QString host, QWidget *parent = nullptr);

        void initializePage() final;
        void cleanupPage() final;
        bool isComplete() const final;

    signals:
        void project_config_loaded();
        void go_back();

    private slots:
        void on_error_(QString error);

    private:
        Controller &controller_;
        QString host_;
        ProjectConfig config_;

        std::chrono::steady_clock::time_point polling_start_;
        QTimer *poll_config_timer_;

        SimpleProgressAnimation *progress_animation_;
};

class AttachProjectPage : public QWizardPage {
    Q_OBJECT

    public:
        AttachProjectPage(Controller &controller, QString host, QWidget *parent = nullptr);

        void initializePage() final;
        void cleanupPage() final;

    signals:
        void account_key_to_be_loaded(QString email, QString password);
        void project_to_be_attached(QString account_key);
        void project_attached();
        void failed();

    private slots:
        void load_account_key_(QString email, QString password);
        void poll_account_key_();
        void attach_project_(QString account_key);
        void on_error_(QString error);

    private:
        Controller &controller_;
        QString host_;

        std::chrono::steady_clock::time_point polling_start_;
        QTimer *poll_timer_;

        QString project_url_;

        SimpleProgressAnimation *progress_animation_;
};

class CompletionPage: public QWizardPage {
    Q_OBJECT

    public:
        CompletionPage(QWidget *parent = nullptr);

        void initializePage() final;
        void cleanupPage() final;

    private:
        void setup_success_page_();
};

} // namespace add_project_wizard_internals

class AddProjectWizard: public QWizard {
    Q_OBJECT

    public:
        AddProjectWizard(Controller &controller, QString host, QWidget *parent = nullptr);
};

}}}

#endif
