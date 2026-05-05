/*
 *  Copyright (C) 2019 KeePassXC Team <team@keepassxc.org>
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

#ifndef KEEPASSXC_REPORTSWIDGETSTATISTICS_H
#define KEEPASSXC_REPORTSWIDGETSTATISTICS_H

#include <QIcon>
#include <QWidget>

#include <memory>

class Database;
class QStandardItemModel;

namespace Ui {

class ReportsWidgetStatistics;

} // namespace Ui

class ReportsWidgetStatistics: public QWidget
{
	Q_OBJECT

public:
	explicit ReportsWidgetStatistics(QWidget *parent = nullptr);
	~ReportsWidgetStatistics();

	void loadSettings(QSharedPointer<Database> db);
	void saveSettings();

protected:
	void showEvent(QShowEvent *event) override;

private Q_SLOTS:
	void calculateStats();

private:
	QScopedPointer<Ui::ReportsWidgetStatistics> m_ui;

	bool m_statsCalculated = false;
	QIcon m_errIcon;
	std::unique_ptr<QStandardItemModel> m_referencesModel;
	QSharedPointer<Database> m_db;

	void addStatsRow(const QString &name, const QString &value, bool bad = false, const QString &badMsg = QString());
};

#endif // KEEPASSXC_REPORTSWIDGETSTATISTICS_H
