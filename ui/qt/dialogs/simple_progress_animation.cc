/* ui/qt/dialogs/simple_progress_animation.cc --
   Written and Copyright (C) 2021-2023 by vmc.

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

#include "qt/dialogs/simple_progress_animation.h"

#include <QLabel>
#include <QTimer>
#include <QVBoxLayout>

#include "qt/dialogs/utils.h"

namespace woinc { namespace ui { namespace qt {

SimpleProgressAnimation::SimpleProgressAnimation(QWidget *parent)
    : QWidget(parent), timer_(new QTimer(this)), label_(new QLabel(this))
{
    label_->setStyleSheet("font-weight: bold");
    label_->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    label_->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    setLayout(add_widgets__(new QVBoxLayout, label_));

    connect(timer_, &QTimer::timeout, [this]() {
        label_->setText(base_msg_ + QStringLiteral(".").repeated(counter_));
        counter_ = (counter_ + 1) % 4;

        if (timer_->isSingleShot()) {
            timer_->setInterval(500);
            timer_->setSingleShot(false);
            timer_->start();
        }
    });
}

SimpleProgressAnimation::~SimpleProgressAnimation() = default;

void SimpleProgressAnimation::start(QString base_msg) {
    base_msg_ = std::move(base_msg);
    base_msg_.append(' ');
    counter_ = 0;

    // don't show anything for an initial delay of 200ms
    label_->clear();
    timer_->setInterval(200);
    timer_->setSingleShot(true);
    timer_->start();
}

void SimpleProgressAnimation::stop() {
    timer_->stop();
    label_->clear();
}

}}}
