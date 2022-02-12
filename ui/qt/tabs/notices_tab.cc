/* ui/qt/tabs/notices_tab.cc --
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

#include "qt/tabs/notices_tab.h"

#include <map>
#ifndef NDEBUG
#include <iostream>
#endif

#include <QDateTime>
#include <QImage>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QUrl>

namespace woinc { namespace ui { namespace qt {

NoticesTab::NoticesTab(QWidget *parent) : QTextBrowser(parent), network_manager_(new QNetworkAccessManager(this)) {
    setOpenExternalLinks(true);
}

QVariant NoticesTab::loadResource(int type, const QUrl &name) {
    if (type == QTextDocument::ImageResource) {
        static std::map<QUrl, QImage> cache;

        auto iter = cache.find(name);
        if (iter != cache.end())
            return iter->second;

        cache.emplace(name, QImage());

        connect(network_manager_, &QNetworkAccessManager::finished, [&](QNetworkReply *reply) {
            if (reply->error() == QNetworkReply::NoError) {
                cache[reply->url()].loadFromData(reply->readAll());
                // force the widget to show the loaded image
                // TODO how to do this the right way?
                auto r = rect();
                resize(r.width()+1, r.height());
                resize(r.width(), r.height());
            }

            reply->deleteLater();
        });

        network_manager_->get(QNetworkRequest(name));
        return cache[name];
    }

    return QTextBrowser::loadResource(type, name);
}

void NoticesTab::select_host(QString /*host*/) {
    notices_.clear();
}

void NoticesTab::unselect_host(QString /*host*/) {
    notices_.clear();
}

void NoticesTab::append_notices(Notices notices) {
    notices_.insert(notices_.end(), notices.begin(), notices.end());
    update_();
}

void NoticesTab::refresh_notices(Notices notices) {
    notices_ = std::move(notices);
    update_();
}

void NoticesTab::update_() {
    QStringList txts;
    txts.reserve(static_cast<int>(notices_.size()));

    if (notices_.empty())
        txts << QStringLiteral("There are no notices at this time.");

    for (auto notice = notices_.rbegin(); notice != notices_.rend(); ++notice) {
        QString title;

        switch (notice->category) {
            case Notice::Category::CLIENT:
                title = notice->project_name.isEmpty()
                    ? QString::fromUtf8("Notice from BOINC")
                    : notice->project_name + ": " + notice->title;
                break;
            case Notice::Category::SCHEDULER:
                title = notice->project_name + ": Notice from server";
                break;
            default:
                title = notice->project_name.isEmpty()
                    ? notice->title
                    : notice->project_name + ": " + notice->title;
        }

        QString footer = QString::fromUtf8("<br><font size=-2 color=#8f8f8f>");
        footer += QDateTime::fromTime_t(static_cast<unsigned int>(notice->create_time)).toString(Qt::TextDate);
        if (!notice->link.isEmpty())
            footer += QString::fromUtf8(" &middot; <a href=\"") + notice->link + QString::fromUtf8("\">more...</a>");
        footer += "</font>";

        txts <<  "<b>" + title + "</b><br>" + notice->description + footer;
    }

    setHtml("<html><head></head><body>" + txts.join("<hr>") + "</body></html>");
}

}}}
