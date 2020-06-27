/* ui/qt/tabs/notices_tab.h --
   Written and Copyright (C) 2018-2020 by vmc.

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

#ifndef WOINC_UI_QT_NOTICES_TAB_H_
#define WOINC_UI_QT_NOTICES_TAB_H_

#include <QTextBrowser>

#include "qt/types.h"

struct QNetworkAccessManager;

namespace woinc { namespace ui { namespace qt {

class NoticesTab : public QTextBrowser {
    Q_OBJECT

    public:
        NoticesTab(QWidget *parent = nullptr);
        virtual ~NoticesTab() = default;

        QVariant loadResource(int type, const QUrl &name) final;

    public slots:
        void select_host(QString host);
        void unselect_host(QString host);
        void append_notices(Notices notices);
        void refresh_notices(Notices notices);

    private:
        void update_();

    private:
        Notices notices_;
        QNetworkAccessManager *network_manager_;
};

}}}

#endif
