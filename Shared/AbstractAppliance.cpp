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

#include "AbstractAppliance.h"

using namespace Appliance;

AbstractAppliance::AbstractAppliance()
{
}

bool AbstractAppliance::addToContainer(Container * container)
{
    // remove from previous, if exists
    if (m_containerPtr) {
        qWarning("AbstractAppliance::addToContainer: removing from previous container");
        removeFromContainer();
    }

    // add the appliance to the container
    m_containerPtr = container;
    if (m_containerPtr) {
        m_containerPtr->applianceSetScene(m_pScene.data());
        setContainerTopbar();
        m_containerPtr->applianceSetSidebar(m_pSidebar.data());
        m_containerPtr->applianceSetCentralwidget(m_pCentral.data());
    }
    return true;
}

void AbstractAppliance::removeFromContainer()
{
    // sanity check
    if (!m_containerPtr) {
        qWarning("AbstractAppliance::removeFromContainer: not on container");
        return;
    }

    // do the clearance
    clearContainer();
    m_containerPtr = 0;
}

Container * AbstractAppliance::container() const
{
    return m_containerPtr.data();
}

bool AbstractAppliance::isFloating() const
{
    return m_containerPtr.isNull();
}

void AbstractAppliance::sceneSet(QGraphicsScene * scene)
{
    m_pScene = scene;
    if (m_containerPtr)
        m_containerPtr->applianceSetScene(m_pScene.data());
}

void AbstractAppliance::sceneClear()
{
    sceneSet(0);
}

void AbstractAppliance::topbarAddWidget(QWidget * widget, int index)
{
    if (!widget)
        return;
    WidgetPointer wPtr(widget);
    if (index < 0)
        m_pTopbar.prepend(wPtr);
    else if (index >= m_pTopbar.size())
        m_pTopbar.append(wPtr);
    else
        m_pTopbar.insert(index, wPtr);
    setContainerTopbar();
}

void AbstractAppliance::topbarRemoveWidget(QWidget * widget)
{
    // remove given + any dead widgets
    bool removed = false;
    QList<WidgetPointer>::iterator it = m_pTopbar.begin();
    while (it != m_pTopbar.end()) {
        WidgetPointer wPtr = *it;
        if (!wPtr || wPtr.data() == widget) {
            it = m_pTopbar.erase(it);
            removed = true;
        } else
            ++it;
    }

    // set bar only if something changed
    if (removed)
        setContainerTopbar();
}

void AbstractAppliance::sidebarSetWidget(QWidget * widget)
{
    m_pSidebar = widget;
    if (m_containerPtr)
        m_containerPtr->applianceSetSidebar(m_pSidebar.data());
}

void AbstractAppliance::sidebarClearWidget()
{
    sidebarSetWidget(0);
}

void AbstractAppliance::centralwidgetSet(QWidget * widget)
{
    m_pCentral = widget;
    if (m_containerPtr)
        m_containerPtr->applianceSetCentralwidget(m_pCentral.data());
}

void AbstractAppliance::centralwidgetClear()
{
    centralwidgetSet(0);
}

void AbstractAppliance::clearContainer()
{
    if (m_containerPtr) {
        m_containerPtr->applianceSetScene(0);
        m_containerPtr->applianceSetTopbar(QList<QWidget *>());
        m_containerPtr->applianceSetSidebar(0);
        m_containerPtr->applianceSetCentralwidget(0);
    }
}

void AbstractAppliance::setContainerTopbar()
{
    if (!m_containerPtr)
        return;
    QList<QWidget *> widgets;
    foreach (WidgetPointer wpointer, m_pTopbar)
        if (wpointer)
            widgets.append(wpointer.data());
    m_containerPtr->applianceSetTopbar(widgets);
}