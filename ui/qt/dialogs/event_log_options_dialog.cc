/* ui/qt/dialogs/event_log_options_dialog.cc --
   Written and Copyright (C) 2023 by vmc.

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

#include "qt/dialogs/event_log_options_dialog.h"

#ifndef NDEBUG
#include <iostream>
#endif

#include <QCheckBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QScrollArea>
#include <QStackedLayout>
#include <QVBoxLayout>

#include "qt/dialogs/simple_progress_animation.h"
#include "qt/dialogs/utils.h"

namespace woinc { namespace ui { namespace qt {

EventLogOptionsDialog::EventLogOptionsDialog(QWidget *parent)
    : QDialog(parent),
    progress_animation_(new SimpleProgressAnimation(this))
{
    // setup the widgets of the dialog

    auto *main_widget = new QWidget;
    main_widget->setLayout(new QVBoxLayout);

    setLayout(new QStackedLayout);
    layout()->addWidget(progress_animation_);
    layout()->addWidget(main_widget);

    update(woinc::LogFlags());
}

void EventLogOptionsDialog::update(woinc::LogFlags log_flags) {
    log_flags_ = std::move(log_flags);

    auto *main_widget = layout()->itemAt(1)->widget();
    // transfer the layout to a temp widget and let RAII do the cleanup
    QWidget().setLayout(main_widget->layout());
    main_widget->setLayout(new QVBoxLayout);

    { // info labels at the top
        auto *header = label__("These flags enable various types of diagnostic messages in the Event Log.");
        auto *more_info = new QLabel;
        more_info->setText("<a href=\"https://boinc.berkeley.edu/wiki/Client_configuration#Logging_flags\">More info ...</a>");
        more_info->setTextFormat(Qt::RichText);
        more_info->setTextInteractionFlags(Qt::TextBrowserInteraction);
        more_info->setOpenExternalLinks(true);

        main_widget->layout()->addWidget(header);
        main_widget->layout()->addWidget(more_info);
    }

    { // the log flag checkboxes
        auto *grid = new QGridLayout;

        for (const auto &flag : log_flags_.flags()) {
            auto *chk = new QCheckBox(QString::fromStdString(flag.name));
            chk->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
            chk->setChecked(flag.value);
            QObject::connect(chk, &QCheckBox::stateChanged, [this, flag](int state) mutable {
                log_flags_.at(flag.name) = state == Qt::Checked;
            });

            if (grid->count() == 1)
                grid->addWidget(chk, 0, 1);
            else
                grid->addWidget(chk);
        }

        auto *inner_widget = new QWidget;
        inner_widget->setLayout(grid);

        // TODO c/should we use a FlowLayout instead?
        auto *scroll_area = new QScrollArea;
        scroll_area->setWidgetResizable(false);
        scroll_area->setWidget(inner_widget);

        main_widget->layout()->addWidget(scroll_area);
    }

    { // buttons at the bottom
        auto *btn_widget = new QWidget;
        btn_widget->setLayout(new QHBoxLayout);
        main_widget->layout()->addWidget(btn_widget);

        auto *btn_save = new QPushButton(QStringLiteral("Save"));
        btn_save->setDefault(true);
        connect(btn_save, &QPushButton::released, [this]() {
            emit save(log_flags_);
        });
        btn_widget->layout()->addWidget(btn_save);

        auto *btn_defaults = new QPushButton(QStringLiteral("Defaults"));
        connect(btn_defaults, &QPushButton::released, [this]() {
            log_flags_.set_defaults();
            update(log_flags_);
        });
        btn_widget->layout()->addWidget(btn_defaults);

        auto *btn_cancel = new QPushButton(QStringLiteral("Cancel"));
        connect(btn_cancel, &QPushButton::released, this, &QDialog::reject);
        btn_widget->layout()->addWidget(btn_cancel);

        auto *btn_apply = new QPushButton(QStringLiteral("Apply"));
        connect(btn_apply, &QPushButton::released, [this]() {
            emit apply(log_flags_);
        });
        btn_widget->layout()->addWidget(btn_apply);
    }
}

void EventLogOptionsDialog::show_progress_animation(QString base_msg) {
    progress_animation_->start(std::move(base_msg));
    static_cast<QStackedLayout*>(layout())->setCurrentIndex(0);
}

void EventLogOptionsDialog::stop_progress_animation() {
    progress_animation_->stop();
    static_cast<QStackedLayout*>(layout())->setCurrentIndex(1);
}

}}}
