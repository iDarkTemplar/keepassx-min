/*
 *  Copyright (C) 2018 KeePassXC Team <team@keepassxc.org>
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

#ifndef KEEPASSXC_PASSWORDEDITWIDGET_H
#define KEEPASSXC_PASSWORDEDITWIDGET_H

#include "KeyComponentWidget.h"

#include "core/PasswordHealth.h"

namespace Ui {

class PasswordEditWidget;

} // namespace Ui

class PasswordEditWidget: public KeyComponentWidget
{
	Q_OBJECT

public:
	explicit PasswordEditWidget(QWidget *parent = nullptr);
	Q_DISABLE_COPY(PasswordEditWidget);
	~PasswordEditWidget();

	bool addToCompositeKey(QSharedPointer<CompositeKey> key) override;
	void setPasswordVisible(bool visible);
	bool isPasswordVisible() const;
	bool isEmpty() const;
	PasswordHealth::Quality getPasswordQuality() const;
	bool validate(QString &errorMessage) const override;

protected:
	QWidget* componentEditWidget() override;
	void initComponentEditWidget(QWidget *widget) override;
	void initComponent() override;

private Q_SLOTS:
	void setPassword(const QString &password);

private:
	const QScopedPointer<Ui::PasswordEditWidget> m_compUi;
	QPointer<QWidget> m_compEditWidget;
};

#endif // KEEPASSXC_PASSWORDEDITWIDGET_H
