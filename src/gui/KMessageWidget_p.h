/* This file is part of the KDE libraries
 *
 * Copyright (C) 2026 i.Dark_Templar <darktemplar@dark-templar-archives.net>
 * Copyright (c) 2011 Aurélien Gâteau <agateau@kde.org>
 * Copyright (c) 2014 Dominik Haumann <dhaumann@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#include "KMessageWidget.h"

#include "core/Global.h"
#include "gui/Icons.h"

#include <QAction>
#include <QBoxLayout>
#include <QEvent>
#include <QLabel>
#include <QPainter>
#include <QStyle>
#include <QTimeLine>
#include <QToolButton>

//---------------------------------------------------------------------
// KMessageWidgetPrivate
//---------------------------------------------------------------------
class KMessageWidgetPrivate: public QObject
{
	Q_OBJECT

public:
	void init(KMessageWidget *);

	KMessageWidget *q;
	QFrame *content;
	QLabel *iconLabel;
	QLabel *textLabel;
	QToolButton *closeButton;
	QTimeLine *timeLine;
	QIcon icon;
	QPixmap closeButtonPixmap;

	KMessageWidget::MessageType messageType;
	bool wordWrap;
	QList<QToolButton*> buttons;
	QPixmap contentSnapShot;

	void createLayout();
	void updateSnapShot();
	void updateLayout();

	int bestContentHeight() const;

public slots:
	void slotTimeLineChanged(qreal);
	void slotTimeLineFinished();
};
