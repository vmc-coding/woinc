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

#include <set>

#include <QComboBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QScrollArea>
#include <QStringList>
#include <QTextEdit>
#include <QVBoxLayout>

#ifndef NDEBUG
#include <iostream>
#endif

namespace {

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

ChooseProjectPage::ChooseProjectPage(AllProjectsList all_projects, QWidget *parent) : QWizardPage(parent)
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
        // the lables as kind of 'locked' columns at the beginning
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
        for (auto &&project : all_projects)
            if (category == all || project.general_area == category.toStdString())
                projects.append(QString::fromStdString(project.name));
        projects_list_wdgt->clear();
        projects_list_wdgt->addItems(std::move(projects));
    });

    connect(projects_list_wdgt, &QListWidget::itemSelectionChanged, [=]() {
        auto selected = projects_list_wdgt->selectedItems();
        assert(selected.size() <= 1);
        auto project_iter = selected.isEmpty() ? all_projects.end() :
            std::find_if(all_projects.begin(), all_projects.end(), [&](auto &&project) {
                return project.name == selected[0]->text().toStdString();
            });
        if (project_iter == all_projects.end()) {
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

    // initialize the widgets

    categories_cb->addItems(extract_categories__(std::move(all_projects)));
}

// ----- ProjectAccountPage -----

ProjectAccountPage::ProjectAccountPage(QWidget *parent) : QWizardPage(parent)
{

}

} // namespace add_project_wizard_internals

// ----- AddProjectWizard -----

AddProjectWizard::AddProjectWizard(AllProjectsList all_projects, QWidget *parent) : QWizard(parent)
{
    using namespace woinc::ui::qt::add_project_wizard_internals;

    addPage(new ChooseProjectPage(all_projects, this));
    addPage(new ProjectAccountPage(this));
}

}}}
