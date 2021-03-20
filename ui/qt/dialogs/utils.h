/* ui/qt/dialogs/utils.h --
   Written and Copyright (C) 2021 by vmc.

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

// TODO C++17: Use [[maybe_unused]] instead of template<typename AVOID_UNUSED_WARNING = void>

namespace {

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

template<typename... Widgets>
QGroupBox *as_group__(const QString &title, Widgets... widgets) {
    auto layout = new QVBoxLayout;

    add_widgets__(layout, widgets...);

    auto group = new QGroupBox(title);
    group->setLayout(layout);
    group->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Maximum);

    return group;
}

template<typename AVOID_UNUSED_WARNING = void>
QString qstring__(const char *text) {
    return QString::fromUtf8(text);
}

template<typename AVOID_UNUSED_WARNING = void>
QLabel *label__(const char *text, QWidget *parent = nullptr) {
    return new QLabel(qstring__(text), parent);
}

template<typename AVOID_UNUSED_WARNING = void>
QLabel *label__(const QString &text, QWidget *parent = nullptr) {
    return new QLabel(text, parent);
}

template<typename AVOID_UNUSED_WARNING = void>
QLabel *bold_label__(const QString &text, QWidget *parent = nullptr) {
    auto *lbl = label__(text, parent);
    lbl->setStyleSheet("font-weight: bold");
    return lbl;
}

template<typename AVOID_UNUSED_WARNING = void>
QLabel *italic_label__(const QString &text, QWidget *parent = nullptr) {
    auto lbl = label__(text, parent);
    lbl->setStyleSheet("font-style: italic;");
    return lbl;
}

}
