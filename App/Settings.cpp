/***************************************************************************
 *                                                                         *
 *   This file is part of the Fotowall project,                            *
 *       http://www.enricoros.com/opensource/fotowall                      *
 *                                                                         *
 *   Copyright (C) 2009 by Enrico Ros <enrico.ros@gmail.com>               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "Settings.h"

#include "App.h"

#include <QStringList>

Settings::Settings()
    : QSettings()
    , m_firstTime(false)
{
    // build up the recent urls list
    foreach (const QString & urlString, value("Fotowall/RecentUrls").toStringList())
        if (App::validateFotowallUrl(urlString))
            m_recentFotowallUrls.append(QUrl(urlString));

    // find out if this is the first time
    m_firstTime = value("Fotowall/FirstTime", true).toBool();
    setValue("Fotowall/FirstTime", false);
}

Settings::~Settings()
{
    // save the recent urls list
    if (!m_recentFotowallUrls.isEmpty()) {
        QStringList urls;
        foreach (const QUrl & url, m_recentFotowallUrls)
            urls.append(url.toString());
        setValue("Fotowall/RecentUrls", urls);
    } else
        remove("Fotowall/RecentUrls");

    // flush to disk
    sync();
}

bool Settings::firstTime() const
{
    return m_firstTime;
}

QList<QUrl> Settings::recentFotowallUrls() const
{
    return m_recentFotowallUrls;
}

void Settings::addRecentFotowallUrl(const QUrl & fotowallUrl)
{
    // remove if already enlisted
    m_recentFotowallUrls.removeAll(fotowallUrl);

    // keep up to 10 urls
    //while (m_recentFotowallUrls.size() > 10)
    //    m_recentFotowallUrls.removeLast();

    // finally add the url
    m_recentFotowallUrls.prepend(fotowallUrl);
}

void Settings::addCommandlineUrl(const QString & url)
{
    m_commandlineUrls.append(url);
}

QStringList Settings::commandlineUrls() const
{
    return m_commandlineUrls;
}
