/* ui/qt/dialogs/add_project_wizard.cc --
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

#include "qt/dialogs/add_project_wizard.h"

#include <chrono>
#include <exception>
#include <set>

#include <QAbstractButton>
#include <QComboBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QScrollArea>
#include <QStringList>
#include <QTextEdit>
#include <QTimer>
#include <QVBoxLayout>

#ifndef NDEBUG
#include <iostream>
#endif

#include "qt/controller.h"

namespace {

const char * const FIELD_ATTACH_PROJECT_URL = "project_url";
const char * const FIELD_LOGIN_EMAIL = "email";
const char * const FIELD_LOGIN_PASSWORD = "password";
const char * const FIELD_LOGIN_ACCOUNT_KEY = "account_key";

QString all_category() {
    return QStringLiteral("All");
}

QLabel *topic__(QString text, QWidget *parent = nullptr) {
    auto *lbl = new QLabel(std::move(text), parent);
    lbl->setStyleSheet("font-weight: bold");
    return lbl;
}

template<typename Widget>
QLayout *add_widgets__(QLayout *layout, Widget widget) {
    layout->addWidget(widget);
    return layout;
}

template<typename Widget, typename... Widgets>
QLayout *add_widgets__(QLayout *layout, Widget widget, Widgets... others) {
    layout->addWidget(widget);
    return add_widgets__(layout, others...);
}

template<typename... Widgets>
QWidget *as_combined_widget__(QLayout *layout, Widgets... widgets) {
    layout->setContentsMargins(0, 0, 0, 0);

    add_widgets__(layout, widgets...);

    // TODO don't use a dummy widget, just add the layout to another layout
    auto widget = new QWidget;
    widget->setLayout(layout);
    widget->setContentsMargins(0, 0, 0, 0);

    return widget;
}

template<typename... Widgets>
QWidget *as_vertical_widget__(Widgets... widgets) {
    return as_combined_widget__(new QVBoxLayout, widgets...);
}

template<typename... Widgets>
QWidget *as_horizontal_widget__(Widgets... widgets) {
    return as_combined_widget__(new QHBoxLayout, widgets...);
}

QStringList extract_categories__(const woinc::AllProjectsList &projects) {
    std::set<std::string> categories;
    for (auto &&project : projects)
        categories.insert(project.general_area);
    QStringList clist;
    clist.append(all_category());
    for (auto &&c : categories)
        clist.append(QString::fromStdString(c));
    return clist;
}

}

namespace woinc { namespace ui { namespace qt {

namespace add_project_wizard_internals {

// ----- ChooseProjectPage -----

ChooseProjectPage::ChooseProjectPage(Controller &controller, QString host, QWidget *parent)
    : QWizardPage(parent), controller_(controller), host_(std::move(host))
{
    // create all widgets of the wizard page

    // left side

    auto *categories_lbl = new QLabel(QStringLiteral("Categories:"));
    auto *categories_cb = new QComboBox;
    auto *categories_wdgt = as_vertical_widget__(categories_lbl, categories_cb);

    auto *projects_list_wdgt = new QListWidget;
    projects_list_wdgt->setSortingEnabled(true);

    auto *project_selection_wdgt = as_vertical_widget__(categories_wdgt, projects_list_wdgt);
    project_selection_wdgt->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::MinimumExpanding);

    // right side

    auto *project_desc_wdgt = new QTextEdit;
    project_desc_wdgt->setReadOnly(true);

    auto *research_area_lbl = new QLabel(QStringLiteral("Research area:"));
    auto *research_area_value = new QLabel;

    auto *organization_lbl = new QLabel(QStringLiteral("Organization:"));
    auto *organization_value = new QLabel;

    auto *website_lbl = new QLabel(QStringLiteral("Web site:"));
    auto *website_value = new QLabel;
    website_value->setOpenExternalLinks(true);
    website_value->setTextInteractionFlags(Qt::TextBrowserInteraction);

    auto *supported_systems_lbl = new QLabel(QStringLiteral("Supported systems:"));
    auto *supported_systems_value = new QLabel;

    auto *project_details_lbls_wdgt = as_vertical_widget__(research_area_lbl,
                                                           organization_lbl,
                                                           website_lbl,
                                                           supported_systems_lbl);
    project_details_lbls_wdgt->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);

    auto *project_details_values_wdgt = new QScrollArea;
    project_details_values_wdgt->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    project_details_values_wdgt->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    project_details_values_wdgt->setWidgetResizable(true);
    project_details_values_wdgt->setFrameShape(QFrame::NoFrame);
    project_details_values_wdgt->setContentsMargins(0, 0, 0, 0);
    {
        auto *tmp_wdgt = new QWidget;
        tmp_wdgt->setLayout(add_widgets__(new QVBoxLayout,
                                          research_area_value,
                                          organization_value,
                                          website_value,
                                          supported_systems_value));
        // TODO a better aproach would be to have the scrollarea also containing
        // the lables as kind of 'locked' columns on the left side
        project_details_values_wdgt->setWidget(tmp_wdgt);
    }

    auto *project_details_bottom_wdgt = as_horizontal_widget__(project_details_lbls_wdgt,
                                                               project_details_values_wdgt);
    project_details_bottom_wdgt->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);

    auto *project_details_wdgt = new QGroupBox(QStringLiteral("Project details"));
    project_details_wdgt->setLayout(add_widgets__(new QVBoxLayout,
                                                  project_desc_wdgt,
                                                  project_details_bottom_wdgt));

    // bottom widget

    auto *project_url_lbl = new QLabel(QStringLiteral("Project URL:"));
    auto *project_url_value = new QLineEdit;
    auto *project_url_wdgt = as_horizontal_widget__(project_url_lbl, project_url_value);
    registerField(FIELD_ATTACH_PROJECT_URL, project_url_value);

    // setup the layout of the wizard page

    setLayout(add_widgets__(new QVBoxLayout,
                            topic__(QStringLiteral("Choose a project")),
                            new QLabel(QStringLiteral("To choose a project, click its name or type its URL below.")),
                            as_horizontal_widget__(
                                project_selection_wdgt,
                                project_details_wdgt),
                            project_url_wdgt));

    // connect the widgets

    connect(categories_cb, &QComboBox::currentTextChanged, [=](QString category) {
        auto all = all_category();
        QStringList projects;
        for (auto &&project : all_projects_)
            if (category == all || project.general_area == category.toStdString())
                projects.append(QString::fromStdString(project.name));
        projects_list_wdgt->clear();
        projects_list_wdgt->addItems(std::move(projects));
    });

    connect(projects_list_wdgt, &QListWidget::itemSelectionChanged, [=]() {
        auto selected = projects_list_wdgt->selectedItems();
        assert(selected.size() <= 1);
        auto project_iter = selected.isEmpty() ? all_projects_.end() :
            std::find_if(all_projects_.begin(), all_projects_.end(), [&](auto &&project) {
                return project.name == selected[0]->text().toStdString();
            });
        if (project_iter == all_projects_.end()) {
            project_desc_wdgt->clear();
            research_area_value->clear();
            organization_value->clear();
            website_value->clear();
            supported_systems_value->clear();
            project_url_value->clear();
        } else {
            project_desc_wdgt->setText(QString::fromStdString(project_iter->description));
            research_area_value->setText(QString::fromStdString(project_iter->specific_area));
            organization_value->setText(QString::fromStdString(project_iter->home));
            website_value->setText(QStringLiteral("<a href=\"%1\">%1</a>").arg(QString::fromStdString(project_iter->web_url)));
            QStringList platforms;
            for (auto &&platform : project_iter->platforms)
                platforms << QString::fromStdString(platform);
            supported_systems_value->setText(platforms.join(", "));
            project_url_value->setText(QString::fromStdString(project_iter->url));
        }
    });

    connect(this, &ChooseProjectPage::all_project_list_loaded, [=]() {
        categories_cb->addItems(extract_categories__(all_projects_));
    });
}

void ChooseProjectPage::initializePage() {
    // TODO show loading animation
    QTimer::singleShot(0, [&]() {
        try {
            auto all_projects_future = controller_.load_all_projects_list(host_);
            all_projects_future.wait();
            all_projects_ = all_projects_future.get();
            emit all_project_list_loaded();
        } catch (std::exception &err) {
            QMessageBox::critical(this, QStringLiteral("Error"), QString::fromStdString(err.what()), QMessageBox::Ok);
            close();
        } catch (...) {
            QMessageBox::critical(this, QStringLiteral("Error"), QStringLiteral("Unhandled error occurred, please inform a dev about it"), QMessageBox::Ok);
            close();
        }
    });
}

// ----- ProjectAccountPage -----

ProjectAccountPage::ProjectAccountPage(Controller &controller, QString host, QWidget *parent)
    : QWizardPage(parent), controller_(controller), host_(host), poll_config_timer_(new QTimer(this))
{
    connect(this, &ProjectAccountPage::project_config_loaded, this, &ProjectAccountPage::show_project_config_);
    connect(poll_config_timer_, &QTimer::timeout, [=]() {
        QString error;

        try {
#ifndef NDEBUG
            std::cout << "Poll project config of " << field(FIELD_ATTACH_PROJECT_URL).toString().toStdString() << std::endl;
#endif
            // TODO should we stop the timer before this blocking wait?
            config_ = controller_.poll_project_config(host_).get();

            if (config_.error_num == 0) {
                poll_config_timer_->stop();
                emit project_config_loaded();
            } else if (config_.error_num != -204 || --remaining_pollings_ == 0) { // -204 is still loading.. terrible API
                error = QStringLiteral("Failed to load the project configuration");
            }
        } catch (std::exception &err) {
            error = QString::fromStdString(err.what());
        } catch (...) {
            error = QStringLiteral("Unhandled error occurred, please inform a dev about it");
        }

        if (!error.isEmpty()) {
            poll_config_timer_->stop();
            QMessageBox::critical(this, QStringLiteral("Error"), error, QMessageBox::Ok);
            close();
        }
    });
}

// TODO show loading animation
void ProjectAccountPage::initializePage() {
    // it has to be queued so we're on this page when calling back, not on the stage switching to this page;
    // connecting in the c'tor would be to early, because wizard's not yet set
    connect(this, &ProjectAccountPage::go_back, wizard(), &QWizard::back, Qt::QueuedConnection);

    try { // start loading the config
        auto project_url = field(FIELD_ATTACH_PROJECT_URL).toString();

        auto load_config_future = controller_.start_loading_project_config(host_, project_url);

        // TODO this blocks the event queue until we get a result;
        // a better approach would be to poll instead of blocking waiting
        load_config_future.wait();

        // TODO do we get an error message from the client?
        if (!load_config_future.get())
            throw std::runtime_error("Error loading the project config");

#ifndef NDEBUG
        std::cout << "Started loading project config of " << project_url.toStdString() << std::endl;
#endif

        // start polling the config for an arbitrary value of one minute
        remaining_pollings_ = 60;
        poll_config_timer_->start(1000);
    } catch (std::exception &err) {
        on_error_(QString::fromStdString(err.what()));
    } catch (...) {
        on_error_(QStringLiteral("Unhandled error occurred, please inform a dev about it"));
    }
}

void ProjectAccountPage::cleanupPage() {
    poll_config_timer_->stop();
}

void ProjectAccountPage::show_project_config_() {
    auto *title_lbl = new QLabel(QStringLiteral("Identify your account at ").append(QString::fromStdString(config_.name)));
    title_lbl->setStyleSheet("font-weight: bold;");

    auto *email_lbl = new QLabel(QStringLiteral("Email address:"));
    auto *email_value = new QLineEdit;

    auto *password_lbl = new QLabel(QStringLiteral("Password:"));
    auto *password_value = new QLineEdit;

    auto *mail_pwd_lyt = new QGridLayout;
    mail_pwd_lyt->addWidget(email_lbl, 0, 0);
    mail_pwd_lyt->addWidget(email_value, 0, 1);
    mail_pwd_lyt->addWidget(password_lbl, 1, 0);
    mail_pwd_lyt->addWidget(password_value, 1, 1);

    auto *mail_pwd_wdgt = new QGroupBox;
    mail_pwd_wdgt->setContentsMargins(0, 0, 0, 0);
    mail_pwd_wdgt->setLayout(mail_pwd_lyt);

    auto *or_lbl = new QLabel(QStringLiteral("OR"));
    or_lbl->setStyleSheet("font-weight: bold;");
    or_lbl->setAlignment(Qt::AlignCenter);

    auto *account_key_lbl = new QLabel(QStringLiteral("Account key:"));
    auto *account_key_value = new QLineEdit;

    auto *account_key_wdgt = new QGroupBox;
    account_key_wdgt->setContentsMargins(0, 0, 0, 0);
    account_key_wdgt->setLayout(add_widgets__(new QHBoxLayout, account_key_lbl, account_key_value));

    auto *vertical_filler = new QWidget;
    vertical_filler->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

    setLayout(add_widgets__(new QVBoxLayout,
                            title_lbl,
                            mail_pwd_wdgt,
                            or_lbl,
                            account_key_wdgt,
                            vertical_filler));

    registerField(FIELD_LOGIN_EMAIL, email_value);
    registerField(FIELD_LOGIN_PASSWORD, password_value);
    registerField(FIELD_LOGIN_ACCOUNT_KEY, account_key_value);
}

void ProjectAccountPage::on_error_(QString error) {
    poll_config_timer_->stop();
    QMessageBox::critical(this, QStringLiteral("Error"), error, QMessageBox::Ok);
    emit go_back();
}

// ----- BackgroundLoginPage -----

BackgroundLoginPage::BackgroundLoginPage(Controller &controller, QString host, QWidget *parent)
    : QWizardPage(parent), controller_(controller), host_(host), poll_timer_(new QTimer(this))
{
    setCommitPage(true);
}

void BackgroundLoginPage::initializePage() {
    if (!connected_) {
        connect(this, &BackgroundLoginPage::account_key_to_be_loaded, this, &BackgroundLoginPage::load_account_key_, Qt::QueuedConnection);
        connect(this, &BackgroundLoginPage::log_in, this, &BackgroundLoginPage::log_in_, Qt::QueuedConnection);
        // don't connect in the c'tor, the wizard isn't set yet!
        connect(this, &BackgroundLoginPage::logged_in, wizard(), &QWizard::next, Qt::QueuedConnection);
        connect(this, &BackgroundLoginPage::login_failed, wizard(), &QWizard::back, Qt::QueuedConnection);
        connect(poll_timer_, &QTimer::timeout, this, &BackgroundLoginPage::poll_account_key_);
        connected_ = true;
    }

    project_url_ = field(FIELD_ATTACH_PROJECT_URL).toString();
    auto account_key = field(FIELD_LOGIN_ACCOUNT_KEY).toString();

    if (account_key.isEmpty())
        emit account_key_to_be_loaded(field(FIELD_LOGIN_EMAIL).toString(), field(FIELD_LOGIN_PASSWORD).toString());
    else
        emit log_in(std::move(account_key));
}

void BackgroundLoginPage::cleanupPage() {
    poll_timer_->stop();
}

// TODO show loading animation
void BackgroundLoginPage::load_account_key_(QString email, QString password) {
    try { // start the account lookup
        auto load_future = controller_.start_account_lookup(host_, project_url_, email, password);

        // TODO this blocks the event queue until we get a result;
        // a better approach would be to poll instead of blocking waiting
        load_future.wait();

        // TODO do we get an error message from the client?
        if (!load_future.get())
            throw std::runtime_error("Error looking up the account");

#ifndef NDEBUG
        std::cout << "Started looking up the account info" << std::endl;
#endif

        // start polling the config for an arbitrary value of one minute
        remaining_pollings_ = 60;
        poll_timer_->start(1000);
    } catch (std::exception &err) {
        on_error_(QString::fromStdString(err.what()));
    } catch (...) {
        on_error_(QStringLiteral("Unhandled error occurred, please inform a dev about it"));
    }
}

void BackgroundLoginPage::poll_account_key_() {
    QString error;

    try {
#ifndef NDEBUG
        std::cout << "Poll account info" << std::endl;
#endif
        // TODO should we stop the timer before this blocking wait?
        auto account = controller_.poll_account_lookup(host_).get();

        if (account.error_num == 0) {
            poll_timer_->stop();
            emit log_in(QString::fromStdString(account.authenticator));
        } else if (account.error_num != -204 || --remaining_pollings_ == 0) { // -204 is still loading.. terrible API
            error = QStringLiteral("Failed to lookup the account");
        }
    } catch (std::exception &err) {
        error = QString::fromStdString(err.what());
    } catch (...) {
        error = QStringLiteral("Unhandled error occurred, please inform a dev about it");
    }

    if (!error.isEmpty())
        on_error_(std::move(error));
}

void BackgroundLoginPage::on_error_(QString error) {
    poll_timer_->stop();
    QMessageBox::critical(this, QStringLiteral("Error"), error, QMessageBox::Ok);
    emit login_failed();
}

void BackgroundLoginPage::log_in_(QString account_key) {
#ifndef NDEBUG
    std::cout << "Attach with account key " << account_key.toStdString() << "\n";
#endif
    QString error;

    try {
        if (controller_.attach_project(host_, std::move(project_url_), std::move(account_key)).get())
            emit logged_in();
        else
            error = QStringLiteral("Failed to attach the project");
    } catch (std::exception &err) {
        error = QString::fromStdString(err.what());
    } catch (...) {
        error = QStringLiteral("Unhandled error occurred, please inform a dev about it");
    }

    if (!error.isEmpty())
        on_error_(std::move(error));
}

// ----- CompletionPage -----

CompletionPage::CompletionPage(QWidget *parent) : QWizardPage(parent) {}

void CompletionPage::initializePage() {
    setup_success_page_();
}

void CompletionPage::cleanupPage() {
    delete layout();
}

void CompletionPage::setup_success_page_() {
    auto *title_lbl = new QLabel(QStringLiteral("Project"));
    title_lbl->setStyleSheet("font-weight: bold;");

    auto *success_lbl = new QLabel(QStringLiteral("This project has been successfully added."));
    auto *finish_lbl = new QLabel(QStringLiteral("Click Finish to close."));

    setLayout(add_widgets__(new QVBoxLayout, title_lbl, success_lbl, finish_lbl));
}

} // namespace add_project_wizard_internals

// ----- AddProjectWizard -----

AddProjectWizard::AddProjectWizard(Controller &controller, QString host, QWidget *parent) : QWizard(parent)
{
    using namespace woinc::ui::qt::add_project_wizard_internals;

    setOptions(options() | QWizard::NoCancelButtonOnLastPage);

    addPage(new ChooseProjectPage(controller, host, this));
    addPage(new ProjectAccountPage(controller, host, this));
    addPage(new BackgroundLoginPage(controller, host, this));
    addPage(new CompletionPage(this));
}

}}}
