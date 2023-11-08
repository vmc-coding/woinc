/* ui/qt/dialogs/add_project_wizard.cc --
   Written and Copyright (C) 2020, 2023 by vmc.

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
#include <QStackedLayout>
#include <QStringList>
#include <QTextEdit>
#include <QTimer>
#include <QVBoxLayout>

#ifndef NDEBUG
#include <iostream>
#endif

#include "qt/controller.h"
#include "qt/dialogs/simple_progress_animation.h"
#include "qt/dialogs/utils.h"

namespace {

enum {
    POLLING_INTERVAL_MSECS = 500,
    POLLING_TIMEOUT_SECS = 30,
    STILL_LOADING = -204
};

const char * const FIELD_ATTACH_PROJECT_URL = "project_url";
const char * const FIELD_LOGIN_EMAIL = "email";
const char * const FIELD_LOGIN_PASSWORD = "password";
const char * const FIELD_LOGIN_ACCOUNT_KEY = "account_key";

QString all_category() {
    return QStringLiteral("All");
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
    : QWizardPage(parent), controller_(controller), host_(std::move(host)), progress_animation_(new SimpleProgressAnimation(this))
{
    setLayout(new QStackedLayout);

    // progress indicator

    layout()->addWidget(progress_animation_);

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

    layout()->addWidget(as_combined_widget__(new QVBoxLayout,
                                             bold_label__(QStringLiteral("Choose a project")),
                                             new QLabel(QStringLiteral("To choose a project, click its name or type its URL below.")),
                                             as_horizontal_widget__(
                                                 project_selection_wdgt,
                                                 project_details_wdgt),
                                             project_url_wdgt));

    // connect the widgets

    connect(categories_cb, &QComboBox::currentTextChanged, [this, projects_list_wdgt](QString category) {
        auto all = all_category();
        QStringList projects;
        for (auto &&project : all_projects_)
            if (category == all || project.general_area == category.toStdString())
                projects.append(QString::fromStdString(project.name));
        projects_list_wdgt->clear();
        projects_list_wdgt->addItems(std::move(projects));
    });

#if __cplusplus >= 202002
    connect(projects_list_wdgt, &QListWidget::itemSelectionChanged, [=, this]() {
#else
    connect(projects_list_wdgt, &QListWidget::itemSelectionChanged, [=]() {
#endif
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

    connect(this, &ChooseProjectPage::all_project_list_loaded, [this, categories_cb]() {
        categories_cb->addItems(extract_categories__(all_projects_));
        progress_animation_->stop();
        static_cast<QStackedLayout*>(layout())->setCurrentIndex(1);
    });

    connect(project_url_value, &QLineEdit::textChanged, this, &ChooseProjectPage::completeChanged);
}

void ChooseProjectPage::initializePage() {
    try {
        progress_animation_->start(QStringLiteral("Loading project list"));
        controller_.load_all_projects_list(
            host_,
            [this](AllProjectsList apl) {
                all_projects_ = std::move(apl);
                emit all_project_list_loaded();
            },
            [this](QString error) {
                QMessageBox::critical(this, QStringLiteral("Error"), error.isEmpty() ? QStringLiteral("Failed to load the projects list") : error, QMessageBox::Ok);
                close();
            });
    } catch (const std::exception &err) {
        QMessageBox::critical(this, QStringLiteral("Error"), QString::fromStdString(err.what()), QMessageBox::Ok);
        close();
    } catch (...) {
        QMessageBox::critical(this, QStringLiteral("Error"), QStringLiteral("Unhandled error occurred, please inform a dev about it"), QMessageBox::Ok);
        close();
    }
}

bool ChooseProjectPage::isComplete() const {
    return !field(FIELD_ATTACH_PROJECT_URL).toString().trimmed().isEmpty();
}

// ----- ProjectAccountPage -----

ProjectAccountPage::ProjectAccountPage(Controller &controller, QString host, QWidget *parent)
    : QWizardPage(parent), controller_(controller), host_(host), poll_config_timer_(new QTimer(this)), progress_animation_(new SimpleProgressAnimation(this))
{
    setLayout(new QStackedLayout);

    // progress indicator

    layout()->addWidget(progress_animation_);

    // create all widgets of the wizard page

    auto *title_lbl = new QLabel(QStringLiteral("Identify your account at ").append(QString::fromStdString(config_.name)));
    title_lbl->setStyleSheet("font-weight: bold;");

    auto *email_lbl = new QLabel(QStringLiteral("Email address:"));
    auto *email_value = new QLineEdit;

    auto *password_lbl = new QLabel(QStringLiteral("Password:"));
    auto *password_value = new QLineEdit;
    password_value->setEchoMode(QLineEdit::Password);

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
    account_key_value->setEchoMode(QLineEdit::Password);

    auto *account_key_wdgt = new QGroupBox;
    account_key_wdgt->setContentsMargins(0, 0, 0, 0);
    account_key_wdgt->setLayout(add_widgets__(new QHBoxLayout, account_key_lbl, account_key_value));

    auto *vertical_filler = new QWidget;
    vertical_filler->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

    layout()->addWidget(as_vertical_widget__(title_lbl,
                                             mail_pwd_wdgt,
                                             or_lbl,
                                             account_key_wdgt,
                                             vertical_filler));

    registerField(FIELD_LOGIN_EMAIL, email_value);
    registerField(FIELD_LOGIN_PASSWORD, password_value);
    registerField(FIELD_LOGIN_ACCOUNT_KEY, account_key_value);

    // connections

    connect(this, &ProjectAccountPage::project_config_loaded, [this]() {
        poll_config_timer_->stop();
        progress_animation_->stop();
        static_cast<QStackedLayout*>(layout())->setCurrentIndex(1);
    });

    connect(email_value, &QLineEdit::textChanged, this, &ProjectAccountPage::completeChanged);
    connect(password_value, &QLineEdit::textChanged, this, &ProjectAccountPage::completeChanged);
    connect(account_key_value, &QLineEdit::textChanged, this, &ProjectAccountPage::completeChanged);

    poll_config_timer_->setSingleShot(true);
    poll_config_timer_->setInterval(POLLING_INTERVAL_MSECS);

    connect(poll_config_timer_, &QTimer::timeout, [this]() {
        controller_.poll_project_config(
            host_,
            [this](ProjectConfig config) {
                if (config.error_num == 0) {
                    config_ = std::move(config);
                    emit project_config_loaded();
                } else if (config.error_num != STILL_LOADING) {
                    on_error_(QStringLiteral("Loading the project configuration failed."));
                } else if (std::chrono::steady_clock::now() - polling_start_ > std::chrono::seconds(POLLING_TIMEOUT_SECS)) {
                    on_error_(QStringLiteral("Timeout while loading the project configuration."));
                } else {
                    poll_config_timer_->start();
                }
            },
            [this](QString error) {
                on_error_(error.isEmpty() ? QStringLiteral("Timeout while loading the project configuration.")
                          : QStringLiteral("Loading the project configuration failed."));
            });
    });
}

void ProjectAccountPage::initializePage() {
    // it has to be queued so we're on this page when calling back, not on the stage switching to this page;
    // connecting in the c'tor would be too early, because the wizard is not yet set
    connect(this, &ProjectAccountPage::go_back, wizard(), &QWizard::back, Qt::QueuedConnection);

    progress_animation_->start(QStringLiteral("Loading the project configuration"));
    controller_.start_loading_project_config(
        host_,
        field(FIELD_ATTACH_PROJECT_URL).toString().trimmed(),
        [this](bool result) {
            if (result) {
                polling_start_ = std::chrono::steady_clock::now();
                poll_config_timer_->start();
            } else {
                on_error_(QStringLiteral("Loading the project configuration failed,"));
            }
        },
        [this](QString) { on_error_(QStringLiteral("Loading the project configuration failed.")); });
}

void ProjectAccountPage::cleanupPage() {
    QWizardPage::cleanupPage();
    poll_config_timer_->stop();
    progress_animation_->stop();
    disconnect(this, &ProjectAccountPage::go_back, wizard(), &QWizard::back);
    static_cast<QStackedLayout*>(layout())->setCurrentIndex(0);
}

bool ProjectAccountPage::isComplete() const {
    return (!field(FIELD_LOGIN_EMAIL).toString().trimmed().isEmpty() &&
            !field(FIELD_LOGIN_PASSWORD).toString().trimmed().isEmpty())
        || !field(FIELD_LOGIN_ACCOUNT_KEY).toString().trimmed().isEmpty();
}

void ProjectAccountPage::on_error_(QString error) {
    poll_config_timer_->stop();
    progress_animation_->stop();
    QMessageBox::critical(this, QStringLiteral("Error"), error, QMessageBox::Ok);
    emit go_back();
}

// ----- AttachProjectPage -----

AttachProjectPage::AttachProjectPage(Controller &controller, QString host, QWidget *parent)
    : QWizardPage(parent), controller_(controller), host_(host), poll_timer_(new QTimer(this)), progress_animation_(new SimpleProgressAnimation(this))
{
    setCommitPage(true);

    setLayout(add_widgets__(new QVBoxLayout, progress_animation_));

    poll_timer_->setSingleShot(true);
    poll_timer_->setInterval(POLLING_INTERVAL_MSECS);

    connect(poll_timer_, &QTimer::timeout, this, &AttachProjectPage::poll_account_key_);

    connect(this, &AttachProjectPage::account_key_to_be_loaded, this, &AttachProjectPage::load_account_key_, Qt::QueuedConnection);
    connect(this, &AttachProjectPage::project_to_be_attached, this, &AttachProjectPage::attach_project_, Qt::QueuedConnection);
}

void AttachProjectPage::initializePage() {
    // don't connect in the c'tor, the wizard isn't set yet!
    connect(this, &AttachProjectPage::project_attached, wizard(), &QWizard::next, Qt::QueuedConnection);
    connect(this, &AttachProjectPage::failed, wizard(), &QWizard::back, Qt::QueuedConnection);

    project_url_ = field(FIELD_ATTACH_PROJECT_URL).toString().trimmed();
    auto account_key = field(FIELD_LOGIN_ACCOUNT_KEY).toString().trimmed();

    if (account_key.isEmpty())
        emit account_key_to_be_loaded(field(FIELD_LOGIN_EMAIL).toString().trimmed(),
                                      field(FIELD_LOGIN_PASSWORD).toString().trimmed());
    else
        emit project_to_be_attached(std::move(account_key));
}

void AttachProjectPage::cleanupPage() {
    disconnect(this, &AttachProjectPage::project_attached, wizard(), &QWizard::next);
    disconnect(this, &AttachProjectPage::failed, wizard(), &QWizard::back);
    QWizardPage::cleanupPage();
    poll_timer_->stop();
    progress_animation_->stop();
}

void AttachProjectPage::load_account_key_(QString email, QString password) {
    progress_animation_->start(QStringLiteral("Looking up the account key"));
    controller_.start_account_lookup(
        host_,
        project_url_,
        email,
        password,
        [this](bool result) {
            if (result) {
                polling_start_ = std::chrono::steady_clock::now();
                poll_timer_->start();
            } else {
                on_error_(QStringLiteral("Looking up the account key failed."));
            }
        },
        [this](QString) { on_error_(QStringLiteral("Looking up the account key failed.")); });
}

void AttachProjectPage::poll_account_key_() {
    controller_.poll_account_lookup(
        host_,
        [this](AccountOut account) {
            if (account.error_num == 0) {
                progress_animation_->stop();
                emit project_to_be_attached(QString::fromStdString(std::move(account.authenticator)));
            } else if (account.error_num != STILL_LOADING) {
                on_error_(QStringLiteral("Looking up the account key failed."));
            } else if (std::chrono::steady_clock::now() - polling_start_ > std::chrono::seconds(POLLING_TIMEOUT_SECS)) {
                on_error_(QStringLiteral("Timeout while looking up the account key."));
            } else {
                poll_timer_->start();
            }
        },
        [this](QString error) {
            on_error_(error.isEmpty() ? QStringLiteral("Timeout while looking up the account key.")
                      : QStringLiteral("Looking up the account key failed."));
        });
}

void AttachProjectPage::on_error_(QString error) {
    poll_timer_->stop();
    progress_animation_->stop();
    QMessageBox::critical(this, QStringLiteral("Error"), error, QMessageBox::Ok);
    emit failed();
}

void AttachProjectPage::attach_project_(QString account_key) {
    progress_animation_->start(QStringLiteral("Attaching project"));

    controller_.attach_project(
        host_,
        project_url_,
        std::move(account_key),
        [this](bool result) {
            if (result) {
                progress_animation_->stop();
                emit project_attached();
            } else {
                on_error_(QStringLiteral("Faild to attach the project."));
            }
        },
        [this](QString error) { on_error_(error.isEmpty() ? QStringLiteral("Timeout while attaching the project.") : std::move(error)); });
}

// ----- CompletionPage -----

CompletionPage::CompletionPage(QWidget *parent) : QWizardPage(parent) {}

void CompletionPage::initializePage() {
    setup_success_page_();
}

void CompletionPage::cleanupPage() {
    QWizardPage::cleanupPage();
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
    addPage(new AttachProjectPage(controller, host, this));
    addPage(new CompletionPage(this));
}

}}}
