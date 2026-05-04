/*
 *  Copyright (C) 2017 Weslly Honorato <﻿weslly@protonmail.com>
 *  Copyright (C) 2017 KeePassXC Team <team@keepassxc.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef KEEPASSX_SETUPTOTPDIALOG_H
#define KEEPASSX_SETUPTOTPDIALOG_H

#include <QDialog>

#include "core/Database.h"
#include "gui/DatabaseWidget.h"

namespace Ui {

class TotpSetupDialog;

} // namespace Ui

class TotpSetupDialog: public QDialog
{
	Q_OBJECT

public:
	explicit TotpSetupDialog(QWidget *parent = nullptr, Entry *entry = nullptr);
	~TotpSetupDialog();

	void init();

Q_SIGNALS:
	void totpUpdated();

private Q_SLOTS:
	void toggleCustom(bool status);
	void saveSettings();

private:
	QScopedPointer<Ui::TotpSetupDialog> m_ui;
	Entry *m_entry;
};

#endif // KEEPASSX_SETUPTOTPDIALOG_H
