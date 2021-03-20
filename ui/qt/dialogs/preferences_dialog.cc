/* ui/qt/dialogs/preferences_dialog.cc --
   Written and Copyright (C) 2019-2020 by vmc.

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

#include "qt/dialogs/preferences_dialog.h"

#include <cassert>
#include <functional>
#include <limits>
#include <type_traits>

#ifndef NDEBUG
#include <iostream>
#endif

#include <QCheckBox>
#include <QFontMetrics>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QString>
#include <QTabWidget>
#include <QTimeEdit>
#include <QVBoxLayout>

#include "qt/dialogs/utils.h"

namespace {

const double DEFAULT_TIMES_END_HOUR = 23.983;
const double DEFAULT_TIMES_START_HOUR = 0;
const double DEFAULT_DAILY_XFER_LIMIT_MB = 10000;
const double DEFAULT_DISK_MAX_USED_GB = 100;
const double DEFAULT_DISK_MAX_USED_PCT = 90;
const double DEFAULT_DISK_MIN_FREE_GB = 1.0;
const double DEFAULT_MAX_BYTES_SEC_DOWN = 100*1024;
const double DEFAULT_MAX_BYTES_SEC_UP = 100*1024;
const double DEFAULT_SUSPEND_CPU_USAGE = 25;
const int DEFAULT_DAILY_XFER_PERIOD_DAYS = 30;

// gcc < 8 doesn't like lambdas when used as default parameters for function paramaters (multiple definitions when linking)
// and std::identity is not available until c++20 , so let's define our own functor for this ..
template<typename T>
struct Identity {
    constexpr T &&operator()(T &&t) const noexcept {
        return std::forward<T>(t);
    }
};

template<typename T, std::enable_if_t<std::is_integral<T>::value, int> = 0>
constexpr bool parse__(const QString &input, T &t) {
    bool b = bool(); // initialized for gcc < 7
    auto parsed = input.toInt(&b);
    if (b)
        t = parsed;
    return b;
}

template<typename T, std::enable_if_t<std::is_floating_point<T>::value, int> = 0>
constexpr bool parse__(const QString &input, T &t) {
    bool b = bool(); // initialized for gcc < 7
    auto parsed = input.toDouble(&b);
    if (b)
        t = parsed;
    return b;
}

template<typename T>
constexpr bool validate__(const QString &input, T min, T max) {
    T value = T(); // initialize for gcc < 7
    return parse__(input, value) && value >= min && value <= max;
}

int text_width__(const QFontMetrics &metrics, int num_chars) {
    return metrics.boundingRect(QString(num_chars, 'X')).width();
}

QLineEdit *integer_input__(int min, int max, int width) {
    auto in = new QLineEdit;
    in->setValidator(new woinc::ui::qt::pref_dialog_internals::IntValidator(min, max, in));
    in->setFixedWidth(width);
    return in;
}

QLineEdit *integer_input__(int min, int max, int width,
                           std::function<void(int)> onTextEdited) {
    auto in = integer_input__(min, max, width);

    QObject::connect(in, &QLineEdit::textEdited, [&](const QString &str) {
        onTextEdited(str.toInt());
    });

    return in;
}

QLineEdit *floating_point_input__(double min, double max, int width) {
    auto in = new QLineEdit;
    in->setValidator(new woinc::ui::qt::pref_dialog_internals::DoubleValidator(min, max, in));
    in->setFixedWidth(width);
    return in;
}

QLineEdit *floating_point_input__(double min, double max, int width,
                                  std::function<void(double)> onTextEdited,
                                  const QString &text = QString()) {
    auto in = floating_point_input__(min, max, width);
    in->setText(text);

    QObject::connect(in, &QLineEdit::textEdited, [=](const QString &str) {
        // TODO round to hundreds; see BOINC why this is a good idea
        onTextEdited(str.toDouble());
    });

    return in;
}

QWidget *floating_point_input_widget__(const char *prefix, const char *postfix,
                                       double min, double max, int in_width,
                                       std::function<void(double)> onTextEdited,
                                       const QString &text = QString()) {
    return as_horizontal_widget__(label__(prefix),
                                  floating_point_input__(min, max, in_width, onTextEdited, text),
                                  label__(postfix));
}

template<typename T, typename DEFAULT_PROVIDER = std::function<T(T)>, typename CONVERTER = std::function<T(T)>>
void as_checked_input__(QCheckBox *chk, QLineEdit *in, T &value, bool &mask,
                        DEFAULT_PROVIDER default_provider = Identity<T>(),
                        CONVERTER converter = Identity<T>()) {
    in->setEnabled((mask = chk->isChecked()));
    T orig_value = value; // TODO why do we need this? we have as mask for it ..
    QObject::connect(chk, &QCheckBox::stateChanged, [=, &value, &mask](int state) {
        if (state == Qt::Checked) {
            value = default_provider(value);
            in->setEnabled((mask = true));
            in->setText(QString::number(converter(value)));
        } else {
            value = orig_value;
            in->setEnabled((mask = false));
            in->setText(QString());
        }
    });
}

QTime double_to_qtime(double t) {
    int h = static_cast<int>(t);
    return QTime(h, static_cast<int>(60 * (t - h) + .5));
}

double qtime_to_double(const QTime &t) {
    return t.hour() + t.minute() / 60.;
}

QWidget *time_of_day_widget__(const QString &prefix, const QString &infix,
                              double &from, bool &from_mask, double &to, bool &to_mask,
                              bool expend = true) {
    auto chk = new QCheckBox(prefix);
    if (expend)
        chk->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Maximum);

    auto in_from = new QTimeEdit(double_to_qtime(from > 0 ? from : DEFAULT_TIMES_START_HOUR));
    auto in_to = new QTimeEdit(double_to_qtime(to > 0 ? to : DEFAULT_TIMES_END_HOUR));

    QObject::connect(in_from, &QTimeEdit::timeChanged, [&](const QTime &time) {
        from = qtime_to_double(time);
    });
    QObject::connect(in_to, &QTimeEdit::timeChanged, [&](const QTime &time) {
        to = qtime_to_double(time);
    });

    auto wdgt = as_horizontal_widget__(chk, in_from, label__(infix), in_to);
    if (expend)
        wdgt->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Maximum);

    QObject::connect(chk, &QCheckBox::stateChanged, [=, &from, &from_mask, &to, &to_mask](int state) {
        if (state == Qt::Checked) {
            in_from->setEnabled((from_mask = true));
            in_to->setEnabled((to_mask = true));
        } else {
            in_from->setEnabled((from_mask = false));
            in_to->setEnabled((to_mask = false));
        }
    });

    in_from->setEnabled((from_mask = false));
    in_to->setEnabled((to_mask = false));
    chk->setChecked(from > 0 || to > 0);

    return wdgt;
}

template<typename KEY, typename VALUE>
const VALUE &find_or_default__(const std::map<KEY, VALUE> &map,
                               const KEY &key,
                               const VALUE &value) {
    auto i = map.find(key);
    return i == map.end() ? value : i->second;
}

QWidget *time_of_weekday_widget__(woinc::DAY_OF_WEEK day,
                                  woinc::GlobalPreferences::TimeSpans &spans) {
    QString prefix;
    switch (day) {
        case woinc::DAY_OF_WEEK::SUNDAY:    prefix = QString::fromUtf8("Sunday");    break;
        case woinc::DAY_OF_WEEK::MONDAY:    prefix = QString::fromUtf8("Monday");    break;
        case woinc::DAY_OF_WEEK::TUESDAY:   prefix = QString::fromUtf8("Tuesday");   break;
        case woinc::DAY_OF_WEEK::WEDNESDAY: prefix = QString::fromUtf8("Wednesday"); break;
        case woinc::DAY_OF_WEEK::THURSDAY:  prefix = QString::fromUtf8("Thursday");  break;
        case woinc::DAY_OF_WEEK::FRIDAY:    prefix = QString::fromUtf8("Friday");    break;
        case woinc::DAY_OF_WEEK::SATURDAY:  prefix = QString::fromUtf8("Saturday");  break;
        default: prefix = QString::fromUtf8("Unknown");
    }

    auto chk = new QCheckBox(prefix);
    chk->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Maximum);

    const woinc::GlobalPreferences::TimeSpan default_span{ DEFAULT_TIMES_START_HOUR, DEFAULT_TIMES_END_HOUR };

    auto in_from = new QTimeEdit(double_to_qtime(find_or_default__(spans, day, default_span).start));
    auto in_to = new QTimeEdit(double_to_qtime(find_or_default__(spans, day, default_span).end));

    QObject::connect(in_from, &QTimeEdit::timeChanged, [=, &spans](const QTime &time) {
        assert(spans.find(day) != spans.end());
        spans[day].start = qtime_to_double(time);
    });
    QObject::connect(in_to, &QTimeEdit::timeChanged, [=, &spans](const QTime &time) {
        assert(spans.find(day) != spans.end());
        spans[day].end = qtime_to_double(time);
    });

    auto wdgt = as_horizontal_widget__(chk, in_from, label__("to"), in_to);
    wdgt->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Maximum);

    QObject::connect(chk, &QCheckBox::stateChanged, [=, &spans](int state) {
        if (state == Qt::Checked) {
            if (spans.find(day) == spans.end())
                spans.emplace(day, default_span);
            in_from->setEnabled(true);
            in_to->setEnabled(true);
        } else {
            spans.erase(day);
            in_from->setEnabled(false);
            in_to->setEnabled(false);
        }
    });

    in_from->setEnabled(false);
    in_to->setEnabled(false);
    chk->setChecked(spans.find(day) != spans.end());

    return wdgt;
}

QCheckBox *checkbox__(const QString &text, bool &value, bool &mask,
                      std::function<bool(int)> predicate = [](int state) { return state == Qt::Checked; }) {
    auto chk = new QCheckBox(text);

    mask = true;
    QObject::connect(chk, &QCheckBox::stateChanged, [=, &value](int state) {
        value = predicate(state);
    });

    return chk;
}

template<typename T>
using Limits = std::numeric_limits<T>;

} // unnamed namespace

namespace woinc { namespace ui { namespace qt { namespace pref_dialog_internals {

// ----- IntValidator -----

IntValidator::IntValidator(int min, int max, QWidget *parent)
    : QValidator(parent), min_(min), max_(max)
{}

QValidator::State IntValidator::validate(QString &input, int &/*pos*/) const {
    return input.isEmpty() || validate__(input, min_, max_) ? Acceptable : Invalid;
}

// ----- DoubleValidator -----

DoubleValidator::DoubleValidator(double min, double max, QWidget *parent)
    : QValidator(parent), min_(min), max_(max)
{}

QValidator::State DoubleValidator::validate(QString &input, int &/*pos*/) const {
    return input.isEmpty() || validate__(input, min_, max_) ? Acceptable : Invalid;
}

// ----- Tab -----

Tab::Tab(QWidget *parent) : QWidget(parent) {
    auto layout = new QVBoxLayout;
    layout->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
    setLayout(layout);

    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
}

// ----- ComputingTab -----

ComputingTab::ComputingTab(GlobalPreferences &prefs, GlobalPreferencesMask &mask, QWidget *parent) : Tab(parent) {
    add_widgets__(layout(),
                  usage_limits_group_(prefs, mask),
                  when_to_suspend_group_(prefs, mask),
                  other_group_(prefs, mask));
}

QWidget *ComputingTab::usage_limits_group_(GlobalPreferences &prefs, GlobalPreferencesMask &mask) {
    QFontMetrics metrics(font());

    mask.max_ncpus_pct = true;
    auto wdgt_max_cpus = floating_point_input_widget__("Use at most", "% of the CPUs",
                                                       0, 100, text_width__(metrics, 7),
                                                       [&](double d) { prefs.max_ncpus_pct = d; },
                                                       QString::number(prefs.max_ncpus_pct));

    mask.cpu_usage_limit = true;
    auto wdgt_max_cpu_time = floating_point_input_widget__("Use at most", "% of CPU time",
                                                           0, 100, text_width__(metrics, 7),
                                                           [&](double d) { prefs.cpu_usage_limit = d; },
                                                           QString::number(prefs.cpu_usage_limit));

    return as_group__(qstring__("Usage limits"), wdgt_max_cpus, wdgt_max_cpu_time);
}

QWidget *ComputingTab::when_to_suspend_group_(GlobalPreferences &prefs, GlobalPreferencesMask &mask) {
    QFontMetrics metrics(font());

    // create widgets

    auto chk_suspend_on_battery = checkbox__(qstring__("Suspend when computer is on battery"),
                                             prefs.run_on_batteries, mask.run_on_batteries,
                                             [](int state) { return state == Qt::Unchecked; });
    auto chk_suspend_in_use = new QCheckBox(qstring__("Suspend when computer is in use"));
    auto chk_suspend_gpu_in_use = new QCheckBox(qstring__("Suspend GPU computing when computer is in use"));

    auto in_in_use_minutes = floating_point_input__(0, 9999.99, text_width__(metrics, 7),
                                                    [&](double d) { prefs.idle_time_to_run = d; });
    auto wdgt_in_use_minutes = as_horizontal_widget__(label__("'In use' means mouse/keyboard input in last"),
                                                      in_in_use_minutes, label__("minutes"));

    auto chk_above_usage = new QCheckBox(qstring__("Suspend when non-BOINC CPU usage is above"));
    auto in_above_usage = floating_point_input__(0, 9999.99, text_width__(metrics, 7),
                                                 [&](double d) { prefs.suspend_cpu_usage = d; });
    auto wdgt_above_usage = as_horizontal_widget__(chk_above_usage, in_above_usage, label__("%"));
    as_checked_input__(chk_above_usage, in_above_usage, prefs.suspend_cpu_usage, mask.suspend_cpu_usage,
                       [](double t) { return t > 0 ? t : DEFAULT_SUSPEND_CPU_USAGE; });

    // setup widgets

    mask.run_if_user_active = true;
    const auto orig_idle_time_to_run = prefs.idle_time_to_run;
    connect(chk_suspend_in_use, &QCheckBox::stateChanged, [=, &prefs, &mask](int state) {
        prefs.run_if_user_active = state == Qt::Unchecked;

        chk_suspend_gpu_in_use->setEnabled(prefs.run_if_user_active);
        chk_suspend_gpu_in_use->setChecked(!(chk_suspend_gpu_in_use->isEnabled() && prefs.run_gpu_if_user_active));

        in_in_use_minutes->setEnabled((mask.idle_time_to_run = !prefs.run_if_user_active || !prefs.run_gpu_if_user_active));
        in_in_use_minutes->setText(in_in_use_minutes->isEnabled() ? QString::number(prefs.idle_time_to_run) : QString());
        if (!in_in_use_minutes->isEnabled())
            prefs.idle_time_to_run = orig_idle_time_to_run;
    });

    mask.run_gpu_if_user_active = true;
    connect(chk_suspend_gpu_in_use, &QCheckBox::stateChanged, [=, &prefs, &mask](int state) {
        prefs.run_gpu_if_user_active = state == Qt::Unchecked;

        in_in_use_minutes->setEnabled((mask.idle_time_to_run = !prefs.run_if_user_active || !prefs.run_gpu_if_user_active));
        in_in_use_minutes->setText(in_in_use_minutes->isEnabled() ? QString::number(prefs.idle_time_to_run) : QString());
        if (!in_in_use_minutes->isEnabled())
            prefs.idle_time_to_run = orig_idle_time_to_run;
    });

    // initialize widgets

    in_in_use_minutes->setEnabled((mask.idle_time_to_run = !prefs.run_if_user_active || !prefs.run_gpu_if_user_active));

    chk_suspend_on_battery->setChecked(!prefs.run_on_batteries);
    chk_suspend_in_use->setChecked(!prefs.run_if_user_active);
    chk_suspend_gpu_in_use->setChecked(!prefs.run_gpu_if_user_active);
    chk_above_usage->setChecked(prefs.suspend_cpu_usage > 0);

    // create group

    return as_group__(qstring__("When to suspend"), chk_suspend_on_battery, chk_suspend_in_use,
                      chk_suspend_gpu_in_use, wdgt_in_use_minutes, wdgt_above_usage,
                      italic_label__(qstring__("To suspend by time of day, see the \"Daily Schedules\" section.")));
}

QWidget *ComputingTab::other_group_(GlobalPreferences &prefs, GlobalPreferencesMask &mask) {
    QFontMetrics metrics(font());

    mask.work_buf_min_days = true;
    auto wdgt_store_at_least = floating_point_input_widget__("Store at least", "days of work",
                                                             0, 10, text_width__(metrics, 5),
                                                             [&](double d) { prefs.work_buf_min_days = d; },
                                                             QString::number(prefs.work_buf_min_days));

    mask.work_buf_additional_days = true;
    auto wdgt_additional_work = floating_point_input_widget__("Store up to an additional", "days of work",
                                                              0, 10, text_width__(metrics, 5),
                                                              [&](double d) { prefs.work_buf_additional_days = d; },
                                                              QString::number(prefs.work_buf_additional_days));

    mask.cpu_scheduling_period_minutes = true;
    auto wdgt_switch_tasks = floating_point_input_widget__("Switch between tasks every", "minutes",
                                                           0, 9999.99, text_width__(metrics, 7),
                                                           [&](double d) { prefs.cpu_scheduling_period_minutes = d; },
                                                           QString::number(prefs.cpu_scheduling_period_minutes));

    mask.disk_interval = true;
    auto wdgt_request_checkpoints = floating_point_input_widget__("Request tasks to checkpoint at most every", "seconds",
                                                                  0, 9999.99, text_width__(metrics, 7),
                                                                  [&](double d) { prefs.disk_interval = d; },
                                                                  QString::number(prefs.disk_interval));

    return as_group__(qstring__("Other"), wdgt_store_at_least, wdgt_additional_work, wdgt_switch_tasks, wdgt_request_checkpoints);
}

// ----- NetworkTab -----

NetworkTab::NetworkTab(GlobalPreferences &prefs, GlobalPreferencesMask &mask, QWidget *parent) : Tab(parent) {
    add_widgets__(layout(), usage_limits_group_(prefs, mask), other_group_(prefs, mask));
}

QWidget *NetworkTab::usage_limits_group_(GlobalPreferences &prefs, GlobalPreferencesMask &mask) {
    QFontMetrics metrics(font());

    auto chk_limit_download_kb = new QCheckBox(qstring__("Limit download rate to"));
    auto in_limit_download_kb = floating_point_input__(0, Limits<double>::max() / 1024, text_width__(metrics, 7),
                                                       [&](double d) { prefs.max_bytes_sec_down = d * 1024;} );
    auto wdgt_limit_download_kb = as_horizontal_widget__(chk_limit_download_kb, in_limit_download_kb, label__("KB/second"));

    as_checked_input__(chk_limit_download_kb, in_limit_download_kb, prefs.max_bytes_sec_down, mask.max_bytes_sec_down,
                       [](auto t) { return t > 0 ? t : DEFAULT_MAX_BYTES_SEC_DOWN; },
                       [](auto t) { return t / 1024; });


    auto chk_limit_upload_kb = new QCheckBox(qstring__("Limit upload rate to"));
    auto in_limit_upload_kb = floating_point_input__(0, Limits<double>::max() / 1024, text_width__(metrics, 7),
                                                     [&](double d) { prefs.max_bytes_sec_up = d * 1024;} );
    auto wdgt_limit_upload_kb = as_horizontal_widget__(chk_limit_upload_kb, in_limit_upload_kb, label__("KB/second"));

    as_checked_input__(chk_limit_upload_kb, in_limit_upload_kb, prefs.max_bytes_sec_up, mask.max_bytes_sec_up,
                       [](auto t) { return t > 0 ? t : DEFAULT_MAX_BYTES_SEC_UP; },
                       [](auto t) { return t / 1024; });


    auto chk_limit_usage = new QCheckBox(qstring__("Limit usage to"));
    auto in_limit_mb = floating_point_input__(0, Limits<double>::max(), text_width__(metrics, 7),
                                              [&](double d) { prefs.daily_xfer_limit_mb = d ;} );
    auto in_limit_days = integer_input__(0, Limits<int>::max(), text_width__(metrics, 7),
                                         [&](int i) { prefs.daily_xfer_period_days = i ;} );
    auto wdgt_limit_usage = as_horizontal_widget__(chk_limit_usage, in_limit_mb, label__("MB every"),
                                                   in_limit_days, label__("days"));

    as_checked_input__(chk_limit_usage, in_limit_mb, prefs.daily_xfer_limit_mb, mask.daily_xfer_limit_mb,
                       [](auto t) { return t > 0 ? t : DEFAULT_DAILY_XFER_LIMIT_MB; });
    as_checked_input__(chk_limit_usage, in_limit_days, prefs.daily_xfer_period_days, mask.daily_xfer_period_days,
                       [](auto t) { return t > 0 ? t : DEFAULT_DAILY_XFER_PERIOD_DAYS; });


    chk_limit_download_kb->setChecked(prefs.max_bytes_sec_down);
    chk_limit_upload_kb->setChecked(prefs.max_bytes_sec_up);
    chk_limit_usage->setChecked(prefs.daily_xfer_limit_mb > 0 || prefs.daily_xfer_period_days > 0);


    return as_group__(qstring__("Usage limits"), wdgt_limit_download_kb, wdgt_limit_upload_kb, wdgt_limit_usage,
                      italic_label__(qstring__("To limit transfers by time of day, see the \"Daily Schedules\" section.")));
}

QWidget *NetworkTab::other_group_(GlobalPreferences &prefs, GlobalPreferencesMask &mask) {
    auto chk_skip_image_verification = checkbox__(qstring__("Skip data verification for image files"),
                                                  prefs.dont_verify_images, mask.dont_verify_images);
    auto chk_confirm_connecting = checkbox__(qstring__("Confirm before connecting to Internet"),
                                             prefs.confirm_before_connecting, mask.confirm_before_connecting);
    auto chk_disconnect_when_done = checkbox__(qstring__("Disconnect when done"),
                                               prefs.hangup_if_dialed, mask.hangup_if_dialed);

    chk_skip_image_verification->setChecked(prefs.dont_verify_images);
    chk_confirm_connecting->setChecked(prefs.confirm_before_connecting);
    chk_disconnect_when_done->setChecked(prefs.hangup_if_dialed);

    return as_group__(qstring__("Other"), chk_skip_image_verification, chk_confirm_connecting, chk_disconnect_when_done);
}

// ----- DiskAndMemoryTab -----

DiskAndMemoryTab::DiskAndMemoryTab(GlobalPreferences &prefs, GlobalPreferencesMask &mask, QWidget *parent)
    : Tab(parent)
{
    add_widgets__(layout(), disk_group_(prefs, mask), memory_group_(prefs, mask));
}

QWidget *DiskAndMemoryTab::disk_group_(GlobalPreferences &prefs, GlobalPreferencesMask &mask) {
    QFontMetrics metrics(font());

    auto chk_disk_max_used_gb = new QCheckBox(qstring__("Use no more than"));
    auto in_disk_max_used_gb = floating_point_input__(0, Limits<double>::max(), text_width__(metrics, 7),
                                                      [&](double d) { prefs.disk_max_used_gb = d; });
    auto wdgt_disk_max_used_gb = as_horizontal_widget__(chk_disk_max_used_gb, in_disk_max_used_gb, label__("GB"));
    as_checked_input__(chk_disk_max_used_gb, in_disk_max_used_gb, prefs.disk_max_used_gb, mask.disk_max_used_gb,
                       [](double d) { return d > 0 ? d : DEFAULT_DISK_MAX_USED_GB; });


    auto chk_disk_min_free_gb = new QCheckBox(qstring__("Leave at least"));
    auto in_disk_min_free_gb = floating_point_input__(0, Limits<double>::max(), text_width__(metrics, 7),
                                                      [&](double d) { prefs.disk_min_free_gb = d ;});
    auto wdgt_disk_min_free_gb = as_horizontal_widget__(chk_disk_min_free_gb, in_disk_min_free_gb, label__("GB free"));
    as_checked_input__(chk_disk_min_free_gb, in_disk_min_free_gb, prefs.disk_min_free_gb, mask.disk_min_free_gb,
                       [](double d) { return d > 0 ? d : DEFAULT_DISK_MIN_FREE_GB; });


    auto chk_disk_max_used_prct = new QCheckBox(qstring__("Use no more than"));
    auto in_disk_max_used_prct = floating_point_input__(0, 100, text_width__(metrics, 7),
                                                        [&](double d) { prefs.disk_max_used_pct = d; });
    auto wdgt_disk_max_used_prct = as_horizontal_widget__(chk_disk_max_used_prct, in_disk_max_used_prct, label__("% of total"));
    as_checked_input__(chk_disk_max_used_prct, in_disk_max_used_prct, prefs.disk_max_used_pct, mask.disk_max_used_pct,
                       [](double d) { return d >= 0 && d < 100 ? d : DEFAULT_DISK_MAX_USED_PCT; });


    chk_disk_max_used_gb->setChecked(prefs.disk_max_used_gb > 0);
    chk_disk_min_free_gb->setChecked(prefs.disk_min_free_gb > 0);
    chk_disk_max_used_prct->setChecked(prefs.disk_max_used_pct < 100);


    return as_group__(qstring__("Disk"),
                      label__("BOINC will use the most restrictive of these settings:"),
                      wdgt_disk_max_used_gb, wdgt_disk_min_free_gb, wdgt_disk_max_used_prct);
}

QWidget *DiskAndMemoryTab::memory_group_(GlobalPreferences &prefs, GlobalPreferencesMask &mask) {
    QFontMetrics metrics(font());

    mask.ram_max_used_busy_pct = true;
    auto wdgt_mem_in_use_prct = floating_point_input_widget__("When computer is in use, use at most", "%",
                                                              0, 100, text_width__(metrics, 7),
                                                              [&](double d) { return prefs.ram_max_used_busy_pct = d; },
                                                              QString::number(prefs.ram_max_used_busy_pct));

    mask.ram_max_used_idle_pct = true;
    auto wdgt_mem_not_in_use_prct = floating_point_input_widget__("When computer is not in use, use at most", "%",
                                                                  0, 100, text_width__(metrics, 7),
                                                                  [&](double d) { return prefs.ram_max_used_idle_pct = d; },
                                                                  QString::number(prefs.ram_max_used_idle_pct));

    auto chk_leave_in_mem = checkbox__(qstring__("Leave non-GPU tasks in memory while suspended"),
                                       prefs.leave_apps_in_memory, mask.leave_apps_in_memory);
    chk_leave_in_mem->setChecked(prefs.leave_apps_in_memory);

    mask.vm_max_used_pct = true;
    auto wdgt_max_swap_prct = floating_point_input_widget__("Page/swap file: use at most", "%",
                                                            0, 100, text_width__(metrics, 7),
                                                            [&](double d) { return prefs.vm_max_used_pct = d; },
                                                            QString::number(prefs.vm_max_used_pct));

    return as_group__(qstring__("Memory"), wdgt_mem_in_use_prct, wdgt_mem_not_in_use_prct,
                      chk_leave_in_mem, wdgt_max_swap_prct);
}

// ----- DailySchedulesTab -----

DailySchedulesTab::DailySchedulesTab(GlobalPreferences &prefs, GlobalPreferencesMask &mask, QWidget *parent)
    : Tab(parent)
{
    add_widgets__(layout(), computing_group_(prefs, mask), network_group_(prefs, mask));
}

QWidget *DailySchedulesTab::computing_group_(GlobalPreferences &prefs, GlobalPreferencesMask &mask) {
    auto grid_lyt = new QGridLayout;

    grid_lyt->addWidget(time_of_weekday_widget__(woinc::DAY_OF_WEEK::MONDAY, prefs.cpu_times)   , 0, 0);
    grid_lyt->addWidget(time_of_weekday_widget__(woinc::DAY_OF_WEEK::TUESDAY, prefs.cpu_times)  , 1, 0);
    grid_lyt->addWidget(time_of_weekday_widget__(woinc::DAY_OF_WEEK::WEDNESDAY, prefs.cpu_times), 2, 0);
    grid_lyt->addWidget(time_of_weekday_widget__(woinc::DAY_OF_WEEK::THURSDAY, prefs.cpu_times) , 3, 0);
    grid_lyt->addWidget(time_of_weekday_widget__(woinc::DAY_OF_WEEK::FRIDAY, prefs.cpu_times)   , 0, 1);
    grid_lyt->addWidget(time_of_weekday_widget__(woinc::DAY_OF_WEEK::SATURDAY, prefs.cpu_times) , 1, 1);
    grid_lyt->addWidget(time_of_weekday_widget__(woinc::DAY_OF_WEEK::SUNDAY, prefs.cpu_times)   , 2, 1);

    auto grid = new QWidget;
    grid->setLayout(grid_lyt);

    return as_group__(qstring__("Computing"),
                      time_of_day_widget__(qstring__("Compute only between"), qstring__("and"),
                                           prefs.start_hour, mask.start_hour,
                                           prefs.end_hour, mask.end_hour, false),
                      as_group__("Day-of-week override", grid));
}

QWidget *DailySchedulesTab::network_group_(GlobalPreferences &prefs, GlobalPreferencesMask &mask) {
    auto grid_lyt = new QGridLayout;

    grid_lyt->addWidget(time_of_weekday_widget__(woinc::DAY_OF_WEEK::MONDAY, prefs.net_times)   , 0, 0);
    grid_lyt->addWidget(time_of_weekday_widget__(woinc::DAY_OF_WEEK::TUESDAY, prefs.net_times)  , 1, 0);
    grid_lyt->addWidget(time_of_weekday_widget__(woinc::DAY_OF_WEEK::WEDNESDAY, prefs.net_times), 2, 0);
    grid_lyt->addWidget(time_of_weekday_widget__(woinc::DAY_OF_WEEK::THURSDAY, prefs.net_times) , 3, 0);
    grid_lyt->addWidget(time_of_weekday_widget__(woinc::DAY_OF_WEEK::FRIDAY, prefs.net_times)   , 0, 1);
    grid_lyt->addWidget(time_of_weekday_widget__(woinc::DAY_OF_WEEK::SATURDAY, prefs.net_times) , 1, 1);
    grid_lyt->addWidget(time_of_weekday_widget__(woinc::DAY_OF_WEEK::SUNDAY, prefs.net_times)   , 2, 1);

    auto grid = new QWidget;
    grid->setLayout(grid_lyt);

    return as_group__(qstring__("Network"),
                      time_of_day_widget__(qstring__("Transfer files only between"), qstring__("and"),
                                           prefs.net_start_hour, mask.net_start_hour,
                                           prefs.net_end_hour, mask.net_end_hour, false),
                      as_group__("Day-of-week override", grid));
}

// ----- TabsWidget -----

TabsWidget::TabsWidget(GlobalPreferences &prefs, GlobalPreferencesMask &mask, QWidget *parent) : QWidget(parent) {
    setLayout(new QVBoxLayout);

    auto tabs = new QTabWidget;
    layout()->addWidget(tabs);

    tabs->addTab(new ComputingTab(prefs, mask), QString::fromUtf8("Computing"));
    tabs->addTab(new NetworkTab(prefs, mask), QString::fromUtf8("Network"));
    tabs->addTab(new DiskAndMemoryTab(prefs, mask), QString::fromUtf8("Disk and memory"));
    tabs->addTab(new DailySchedulesTab(prefs, mask), QString::fromUtf8("Daily schedules"));

    auto btn_widget = new QWidget;
    btn_widget->setLayout(new QHBoxLayout);
    layout()->addWidget(btn_widget);

    auto btn_save = new QPushButton(QString::fromUtf8("Save"));
    connect(btn_save, &QPushButton::released, this, &TabsWidget::save);
    btn_widget->layout()->addWidget(btn_save);

    auto btn_cancel = new QPushButton(QString::fromUtf8("Cancel"));
    connect(btn_cancel, &QPushButton::released, this, &TabsWidget::cancel);
    btn_widget->layout()->addWidget(btn_cancel);
}

} // namespace pref_dialog_internals

// ----- PreferencesDialog -----

PreferencesDialog::PreferencesDialog(GlobalPreferences prefs, QWidget *parent)
    : QDialog(parent, Qt::Dialog), prefs_(std::move(prefs))
{
    setWindowTitle(QString::fromUtf8("Preferences"));

    setLayout(new QVBoxLayout);

    auto widget = new pref_dialog_internals::TabsWidget(prefs_, mask_, this);
    layout()->addWidget(widget);

    connect(widget, &pref_dialog_internals::TabsWidget::cancel, this, &PreferencesDialog::reject);
    connect(widget, &pref_dialog_internals::TabsWidget::save, this, &PreferencesDialog::save_);
}

void PreferencesDialog::save_() {
    emit save(prefs_, mask_);
    close();
}

}}}
