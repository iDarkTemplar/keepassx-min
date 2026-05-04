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

#ifndef KEEPASSXC_DATABASESETTINGSWIDGETENCRYPTION_H
#define KEEPASSXC_DATABASESETTINGSWIDGETENCRYPTION_H

#include "DatabaseSettingsWidget.h"

#include "crypto/kdf/Kdf.h"

class Database;

namespace Ui {

class DatabaseSettingsWidgetEncryption;

} // namespace Ui

class DatabaseSettingsWidgetEncryption: public DatabaseSettingsWidget
{
	Q_OBJECT

public:
	explicit DatabaseSettingsWidgetEncryption(QWidget *parent = nullptr);
	Q_DISABLE_COPY(DatabaseSettingsWidgetEncryption);
	~DatabaseSettingsWidgetEncryption();

public Q_SLOTS:
	void initialize() override;
	void uninitialize() override;
	bool saveSettings() override;

protected:
	void showEvent(QShowEvent *event) override;

private Q_SLOTS:
	void benchmarkTransformRounds(int millisecs = Kdf::DEFAULT_ENCRYPTION_TIME);
	void memoryChanged(int value);
	void parallelismChanged(int value);
	void updateDecryptionTime(int value);
	void loadKdfAlgorithms();
	void loadKdfParameters();
	void updateKdfFields();
	void markDirty();

private:
	bool isAdvancedMode();
	void showBasicEncryption(int decryptionMillisecs = Kdf::DEFAULT_ENCRYPTION_TIME);

	enum FormatSelection
	{
		KDBX4,
		KDBX3
	};
	static const char *CD_DECRYPTION_TIME_PREFERENCE_KEY;

	bool m_isDirty = false;
	bool m_initWithAdvanced = false;
	const QScopedPointer<Ui::DatabaseSettingsWidgetEncryption> m_ui;
};

#endif // KEEPASSXC_DATABASESETTINGSWIDGETENCRYPTION_H
