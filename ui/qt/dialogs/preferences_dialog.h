/* ui/qt/dialogs/preferences_dialog.h --
   Written and Copyright (C) 2019 by vmc.

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

#ifndef WOINC_UI_QT_PREFERENCES_DIALOG_H_
#define WOINC_UI_QT_PREFERENCES_DIALOG_H_

#include <QDialog>
#include <QValidator>
#include <QWidget>

#include <woinc/types.h>

namespace woinc { namespace ui { namespace qt { namespace pref_dialog_internals {

class IntValidator : public QValidator {
    Q_OBJECT

    public:
        IntValidator(int min, int max, QWidget *parent);
        virtual ~IntValidator() = default;

        QValidator::State validate(QString &input, int &pos) const final;

    private:
        int min_;
        int max_;
};

class DoubleValidator : public QValidator {
    Q_OBJECT

    public:
        DoubleValidator(double min, double max, QWidget *parent);
        virtual ~DoubleValidator() = default;

        QValidator::State validate(QString &input, int &pos) const final;

    private:
        double min_;
        double max_;
};

class Tab : public QWidget {
    Q_OBJECT

    public:
        Tab(QWidget *parent = nullptr);
        virtual ~Tab() = default;
};

class ComputingTab : public Tab {
    Q_OBJECT

    public:
        ComputingTab(GlobalPreferences &prefs, GlobalPreferencesMask &mask, QWidget *parent = nullptr);
        virtual ~ComputingTab() = default;

    private:
        QWidget *usage_limits_group_(GlobalPreferences &prefs, GlobalPreferencesMask &mask);
        QWidget *when_to_suspend_group_(GlobalPreferences &prefs, GlobalPreferencesMask &mask);
        QWidget *other_group_(GlobalPreferences &prefs, GlobalPreferencesMask &mask);
};

class NetworkTab : public Tab {
    Q_OBJECT

    public:
        NetworkTab(GlobalPreferences &prefs, GlobalPreferencesMask &mask, QWidget *parent = nullptr);
        virtual ~NetworkTab() = default;

    private:
        QWidget *usage_limits_group_(GlobalPreferences &prefs, GlobalPreferencesMask &mask);
        QWidget *other_group_(GlobalPreferences &prefs, GlobalPreferencesMask &mask);
};

class DiskAndMemoryTab : public Tab {
    Q_OBJECT

    public:
        DiskAndMemoryTab(GlobalPreferences &prefs, GlobalPreferencesMask &mask, QWidget *parent = nullptr);
        virtual ~DiskAndMemoryTab() = default;

    private:
        QWidget *disk_group_(GlobalPreferences &prefs, GlobalPreferencesMask &mask);
        QWidget *memory_group_(GlobalPreferences &prefs, GlobalPreferencesMask &mask);
};

class DailySchedulesTab : public Tab {
    Q_OBJECT

    public:
        DailySchedulesTab(GlobalPreferences &prefs, GlobalPreferencesMask &mask, QWidget *parent = nullptr);
        virtual ~DailySchedulesTab() = default;

    private:
        QWidget *computing_group_(GlobalPreferences &prefs, GlobalPreferencesMask &mask);
        QWidget *network_group_(GlobalPreferences &prefs, GlobalPreferencesMask &mask);
};

class TabsWidget : public QWidget {
    Q_OBJECT

    public:
        TabsWidget(GlobalPreferences &prefs, GlobalPreferencesMask &mask, QWidget *parent);

    signals:
        void save();
        void cancel();
};

} // namespace pref_dialog_internals

struct Controller;

class PreferencesDialog : public QDialog {
    Q_OBJECT

    public:
        PreferencesDialog(Controller &controller, GlobalPreferences prefs, QWidget *parent = nullptr);
        virtual ~PreferencesDialog() = default;

        PreferencesDialog(const PreferencesDialog&) = delete;
        PreferencesDialog(PreferencesDialog &&) = delete;

        PreferencesDialog &operator=(const PreferencesDialog&) = delete;
        PreferencesDialog &operator=(PreferencesDialog &&) = delete;

    private slots:
        void save();

    private:
        Controller &controller_;
        GlobalPreferences prefs_;
        GlobalPreferencesMask mask_;
};

}}}

#endif
