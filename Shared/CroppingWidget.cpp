/***************************************************************************
 *                                                                         *
 *   This file is part of the Fotowall project,                            *
 *       http://www.enricoros.com/opensource/fotowall                      *
 *                                                                         *
 *   Copyright (C) 2009 by TANGUY Arnaud <arn.tanguy@gmail.com>            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "CroppingWidget.h"
#include <QRubberBand>
#include <QMouseEvent>
#include <QPainter>

CroppingWidget::CroppingWidget(QWidget *parent) : QWidget(parent), m_previewRatio(1)
{
    m_rubberBand = new QRubberBand(QRubberBand::Rectangle, this);
    m_rubberBand->setGeometry(0,0,0,0);
}

void CroppingWidget::setPixmap(QPixmap *pix)
{
    m_previewPixmap = pix->scaled(500, 500, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    m_previewRatio = (float)pix->width()/(float)m_previewPixmap.width();
    setFixedSize(m_previewPixmap.size());
}

void CroppingWidget::mousePressEvent(QMouseEvent *event)
{
    m_rubberBand->show();
    m_origin = event->pos();
    m_rubberBand->setGeometry(QRect(m_origin, QSize()));
}

void CroppingWidget::mouseMoveEvent(QMouseEvent *event)
{
    m_rubberBand->setGeometry(QRect(m_origin, event->pos()).normalized());
}

void CroppingWidget::mouseReleaseEvent(QMouseEvent *)
{
    // determine selection, for example using QRect::intersects()
    // and QRect::contains().
}

void CroppingWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.drawPixmap(m_previewPixmap.rect(), m_previewPixmap);
}

QRect CroppingWidget::getCroppingRect() const
{
    QRectF selectionRect = m_rubberBand->geometry();
    return QRect(selectionRect.x()*m_previewRatio, selectionRect.y()*m_previewRatio,
                 selectionRect.width()*m_previewRatio, selectionRect.height()*m_previewRatio);
}
