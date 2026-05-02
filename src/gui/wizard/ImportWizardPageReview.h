/*
 *  Copyright (C) 2023 KeePassXC Team <team@keepassxc.org>
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

#ifndef KEEPASSXC_IMPORTWIZARDPAGEREVIEW_H
#define KEEPASSXC_IMPORTWIZARDPAGEREVIEW_H

#include <QPointer>
#include <QWizardPage>

class CsvImportWidget;
class Database;

namespace Ui {

class ImportWizardPageReview;

} // namespace Ui

class ImportWizardPageReview: public QWizardPage
{
	Q_OBJECT

public:
	explicit ImportWizardPageReview(QWidget *parent = nullptr);
	Q_DISABLE_COPY(ImportWizardPageReview)
	~ImportWizardPageReview();

	void initializePage() override;
	bool validatePage() override;

	QSharedPointer<Database> database();

private:
	bool isCsvImport() const;
	void setupCsvImport(const QString &filename);
	QSharedPointer<Database> importOPUX(const QString &filename);
	QSharedPointer<Database> importBitwarden(const QString &filename, const QString &password);
	QSharedPointer<Database> importOPVault(const QString &filename, const QString &password);
	QSharedPointer<Database> importProtonPass(const QString &filename);
	QSharedPointer<Database> importKeepassxc2(const QString &filename, const QString &password, const QString &keyfile);

	void setupDatabasePreview();

	QScopedPointer<Ui::ImportWizardPageReview> m_ui;

	QSharedPointer<Database> m_db;
	QPointer<CsvImportWidget> m_csvWidget;
};

#endif
