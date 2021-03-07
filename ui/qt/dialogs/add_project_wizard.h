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

#include <future>

#include <QTimer>
#include <QWidget>
#include <QWizard>
#include <QWizardPage>
#include <QVariant>

#include <woinc/types.h>

// not really nice to do it here, but haven't found a better place for the moment
Q_DECLARE_METATYPE(woinc::ProjectListEntry)
Q_DECLARE_METATYPE(woinc::AllProjectsList)

struct QLabel;

namespace woinc { namespace ui { namespace qt {

struct Controller;

namespace add_project_wizard_internals {

template<typename SUBJECT, typename IMPL>
class Poller {
    public:
        Poller();

    protected:
        ~Poller() = default;

    public:
        void start(std::future<SUBJECT> &&future, int timeout_secs = 60);
        void stop();

    private:
        QTimer timer_;
        std::future<SUBJECT> future_;
        int remaining_tries_;
};

class PollerSignals : public QObject {
    Q_OBJECT

    public:
        PollerSignals(QObject *parent = nullptr) : QObject(parent) {}
        virtual ~PollerSignals() = default;

    signals:
        void timed_out();
        void failed(QString error);
        void loaded(QVariant subject);
};

struct AllProjectsListPoller : public PollerSignals, public Poller<AllProjectsList, AllProjectsListPoller>{
    AllProjectsListPoller(QObject *parent = nullptr) : PollerSignals(parent), Poller() {}
    virtual ~AllProjectsListPoller() = default;
};

class SimpleProgressAnimation : public QWidget {
    Q_OBJECT

    public:
        SimpleProgressAnimation(QWidget *parent = nullptr);
        virtual ~SimpleProgressAnimation() = default;

        void start(QString base_msg);
        void stop();

    private:
        QString base_msg_;
        QTimer timer_;
        int counter_;
        QLabel *label_;
};

class ChooseProjectPage: public QWizardPage {
    Q_OBJECT

    public:
        ChooseProjectPage(Controller &controller, QString host, QWidget *parent = nullptr);

        void initializePage() final;
        void cleanupPage() final;
        bool isComplete() const final;

    signals:
        void all_project_list_loaded();

    private:
        Controller &controller_;
        QString host_;
        AllProjectsList all_projects_;

        AllProjectsListPoller poller_;
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
        void show_project_config_();
        void on_error_(QString error);

    private:
        Controller &controller_;
        QString host_;
        ProjectConfig config_;

        int remaining_pollings_;
        QTimer *poll_config_timer_;
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
        bool connected_ = false;

        int remaining_pollings_;
        QTimer *poll_timer_;

        QString project_url_;
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
