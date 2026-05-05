/*
 *  Copyright (C) 2026 i.Dark_Templar <darktemplar@dark-templar-archives.net>
 *  Copyright (C) 2012 Felix Geyer <debfx@fobos.de>
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

#include "AboutDialog.h"
#include "ui_AboutDialog.h"

#include "config-keepassx.h"
#include "core/Tools.h"
#include "crypto/Crypto.h"
#include "gui/Icons.h"

#include <QClipboard>

AboutDialog::AboutDialog(QWidget *parent)
	: QDialog(parent)
	, m_ui(new Ui::AboutDialog())
{
	m_ui->setupUi(this);

	resize(minimumSize());
	setWindowFlags(Qt::Sheet);
	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

	m_ui->nameLabel->setText(m_ui->nameLabel->text().replace(QStringLiteral("${VERSION}"), QStringLiteral(KEEPASSXM_VERSION)));
	QFont nameLabelFont = m_ui->nameLabel->font();
	nameLabelFont.setPointSize(nameLabelFont.pointSize() + 4);
	m_ui->nameLabel->setFont(nameLabelFont);

	m_ui->iconLabel->setPixmap(icons()->applicationIcon().pixmap(48));

	QString debugInfo = Tools::debugInfo().append(QStringLiteral("\n")).append(Crypto::debugInfo());
	m_ui->debugInfo->setPlainText(debugInfo);

	setAttribute(Qt::WA_DeleteOnClose);
	connect(m_ui->buttonBox, &QDialogButtonBox::rejected, this, &AboutDialog::close);
	connect(m_ui->copyToClipboard, &QPushButton::clicked, this, &AboutDialog::copyToClipboard);

	m_ui->buttonBox->button(QDialogButtonBox::Close)->setDefault(true);
}

AboutDialog::~AboutDialog()
{
}

void AboutDialog::copyToClipboard()
{
	QClipboard *clipboard = QApplication::clipboard();
	clipboard->setText(m_ui->debugInfo->toPlainText());
}
