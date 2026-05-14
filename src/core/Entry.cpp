/*
 *  Copyright (C) 2024 KeePassXC Team <team@keepassxc.org>
 *  Copyright (C) 2010 Felix Geyer <debfx@fobos.de>
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

#include "Entry.h"

#include "core/Config.h"
#include "core/Database.h"
#include "core/Global.h"
#include "core/Group.h"
#include "core/Metadata.h"
#include "core/PasswordHealth.h"
#include "core/Tools.h"
#include "core/Totp.h"

#include <QDir>
#include <QRegularExpression>
#include <QStringBuilder>
#include <QUrl>

const int Entry::DefaultIconNumber = 0;

namespace {

const QRegularExpression TagDelimiterRegex(QStringLiteral(R"([,;\t])"));

} // namespace

Entry::Entry()
	: m_attributes(new EntryAttributes(this))
	, m_attachments(new EntryAttachments(this))
	, m_customData(new CustomData(this))
	, m_modifiedSinceBegin(false)
	, m_updateTimeinfo(true)
{
	m_data.iconNumber = DefaultIconNumber;
	m_data.excludeFromReports = false;

	connect(m_attributes, &EntryAttributes::modified, this, &Entry::updateTotp);
	connect(m_attributes, &EntryAttributes::modified, this, &Entry::modified);
	connect(m_attributes, &EntryAttributes::defaultKeyModified, this, &Entry::emitDataChanged);
	connect(m_attachments, &EntryAttachments::modified, this, &Entry::modified);
	connect(m_customData, &CustomData::modified, this, &Entry::modified);

	connect(this, &Entry::modified, this, &Entry::updateTimeinfo);
	connect(this, &Entry::modified, this, &Entry::updateModifiedSinceBegin);
}

Entry::~Entry()
{
	setUpdateTimeinfo(false);
	if (m_group)
	{
		m_group->removeEntry(this);

		if (m_group->database())
		{
			m_group->database()->addDeletedObject(m_uuid);
		}
	}

	qDeleteAll(m_history);
}

template <class T>
inline bool Entry::set(T &property, const T &value)
{
	if (property != value)
	{
		property = value;
		emitModified();
		return true;
	}

	return false;
}

void Entry::updateTimeinfo()
{
	if (m_updateTimeinfo)
	{
		m_data.timeInfo.setLastModificationTime(Clock::currentDateTimeUtc());
		m_data.timeInfo.setLastAccessTime(Clock::currentDateTimeUtc());
	}
}

bool Entry::canUpdateTimeinfo() const
{
	return m_updateTimeinfo;
}

void Entry::setUpdateTimeinfo(bool value)
{
	m_updateTimeinfo = value;
}

const QUuid& Entry::uuid() const
{
	return m_uuid;
}

const QString Entry::uuidToHex() const
{
	return Tools::uuidToHex(m_uuid);
}

int Entry::iconNumber() const
{
	return m_data.iconNumber;
}

const QUuid& Entry::iconUuid() const
{
	return m_data.customIcon;
}

QString Entry::foregroundColor() const
{
	return m_data.foregroundColor;
}

QString Entry::backgroundColor() const
{
	return m_data.backgroundColor;
}

QString Entry::overrideUrl() const
{
	return m_data.overrideUrl;
}

QString Entry::tags() const
{
	return m_data.tags.join(QLatin1Char(','));
}

QStringList Entry::tagList() const
{
	return m_data.tags;
}

const TimeInfo& Entry::timeInfo() const
{
	return m_data.timeInfo;
}

const QSharedPointer<PasswordHealth> Entry::passwordHealth()
{
	if (!m_data.passwordHealth)
	{
		m_data.passwordHealth.reset(new PasswordHealth(password()));
	}
	return m_data.passwordHealth;
}

const QSharedPointer<PasswordHealth> Entry::passwordHealth() const
{
	if (!m_data.passwordHealth)
	{
		return QSharedPointer<PasswordHealth>::create(password());
	}
	return m_data.passwordHealth;
}

bool Entry::excludeFromReports() const
{
	return m_data.excludeFromReports
		|| (customData()->contains(CustomData::ExcludeFromReportsLegacy)
			&& customData()->value(CustomData::ExcludeFromReportsLegacy) == TRUE_STR);
}

void Entry::setExcludeFromReports(bool state)
{
	set(m_data.excludeFromReports, state);
}

QString Entry::title() const
{
	return m_attributes->value(EntryAttributes::TitleKey);
}

QString Entry::url() const
{
	return m_attributes->value(EntryAttributes::URLKey);
}

QStringList Entry::getAdditionalUrls() const
{
	QStringList urlList;

	for (const auto &key: m_attributes->keys())
	{
		if (key.startsWith(EntryAttributes::AdditionalUrlAttribute)
			|| (key == QStringLiteral("%1_RELYING_PARTY").arg(EntryAttributes::PasskeyAttribute)))
		{
			auto additionalUrl = m_attributes->value(key);
			if (!additionalUrl.isEmpty())
			{
				urlList << additionalUrl;
			}
		}
	}

	return urlList;
}

QString Entry::displayUrl() const
{
	return m_attributes->value(EntryAttributes::URLKey);
}

QString Entry::username() const
{
	return m_attributes->value(EntryAttributes::UserNameKey);
}

QString Entry::password() const
{
	return m_attributes->value(EntryAttributes::PasswordKey);
}

QString Entry::notes() const
{
	return m_attributes->value(EntryAttributes::NotesKey);
}

QString Entry::attribute(const QString &key) const
{
	return m_attributes->value(key);
}

int Entry::size() const
{
	int size = 0;
	size += attributes()->attributesSize();
	size += attachments()->attachmentsSize();
	size += customData()->dataSize();
	for (const QString &tag: tags().split(TagDelimiterRegex, Qt::SkipEmptyParts))
	{
		size += tag.toUtf8().size();
	}

	return size;
}

bool Entry::isExpired() const
{
	return willExpireInDays(0);
}

bool Entry::willExpireInDays(int days) const
{
	return m_data.timeInfo.expires() && (m_data.timeInfo.expiryTime() < Clock::currentDateTime().addDays(days));
}

void Entry::expireNow()
{
	setExpiryTime(Clock::currentDateTimeUtc());
	setExpires(true);
}

bool Entry::isRecycled() const
{
	const Database *db = database();
	if (!db)
	{
		return false;
	}

	return m_group->isRecycled();
}

EntryAttributes* Entry::attributes()
{
	return m_attributes;
}

const EntryAttributes* Entry::attributes() const
{
	return m_attributes;
}

EntryAttachments* Entry::attachments()
{
	return m_attachments;
}

const EntryAttachments* Entry::attachments() const
{
	return m_attachments;
}

CustomData* Entry::customData()
{
	return m_customData;
}

const CustomData* Entry::customData() const
{
	return m_customData;
}

bool Entry::hasTotp() const
{
	return !m_data.totpSettings.isNull();
}

bool Entry::hasValidTotp() const
{
	auto error = Totp::checkValidSettings(m_data.totpSettings);
	return error.isEmpty();
}

bool Entry::hasPasskey() const
{
	return m_attributes->hasPasskey();
}

QString Entry::totp(bool *isValid) const
{
	if (hasTotp())
	{
		return Totp::generateTotp(m_data.totpSettings, isValid);
	}

	if (isValid)
	{
		*isValid = false;
	}

	return {};
}

void Entry::setTotp(QSharedPointer<Totp::Settings> settings)
{
	beginUpdate();

	m_attributes->remove(Totp::ATTRIBUTE_OTP);
	m_attributes->remove(Totp::ATTRIBUTE_SEED);
	m_attributes->remove(Totp::ATTRIBUTE_SETTINGS);

	if (!settings || settings->key.isEmpty())
	{
		m_data.totpSettings.reset();
	}
	else
	{
		m_data.totpSettings = std::move(settings);
		auto text = Totp::writeSettings(m_data.totpSettings, title(), username());
		if (m_data.totpSettings->format != Totp::StorageFormat::LEGACY)
		{
			m_attributes->set(Totp::ATTRIBUTE_OTP, text, true);
		}
		else
		{
			m_attributes->set(Totp::ATTRIBUTE_SEED, m_data.totpSettings->key, true);
			m_attributes->set(Totp::ATTRIBUTE_SETTINGS, text);
		}
	}

	endUpdate();
}

void Entry::updateTotp()
{
	if (m_attributes->contains(Totp::ATTRIBUTE_SETTINGS))
	{
		m_data.totpSettings = Totp::parseSettings(
			m_attributes->value(Totp::ATTRIBUTE_SETTINGS),
			m_attributes->value(Totp::ATTRIBUTE_SEED));
	}
	else if (m_attributes->contains(Totp::ATTRIBUTE_OTP))
	{
		m_data.totpSettings = Totp::parseSettings(m_attributes->value(Totp::ATTRIBUTE_OTP));
	}
	else if (m_attributes->contains(Totp::KP2_TOTP_SECRET))
	{
		m_data.totpSettings = Totp::fromKeePass2Totp(
			m_attributes->value(Totp::KP2_TOTP_SECRET),
			m_attributes->value(Totp::KP2_TOTP_ALGORITHM),
			m_attributes->value(Totp::KP2_TOTP_LENGTH),
			m_attributes->value(Totp::KP2_TOTP_PERIOD));
	}
	else
	{
		m_data.totpSettings.reset();
	}
}

QSharedPointer<Totp::Settings> Entry::totpSettings() const
{
	return m_data.totpSettings;
}

QString Entry::totpSettingsString() const
{
	if (m_data.totpSettings)
	{
		return Totp::writeSettings(m_data.totpSettings, title(), username(), true);
	}

	return {};
}

QString Entry::path() const
{
	auto path = group()->hierarchy();
	path << title();
	return path.mid(1).join(QLatin1Char('/'));
}

void Entry::setUuid(const QUuid &uuid)
{
	Q_ASSERT(!uuid.isNull());
	set(m_uuid, uuid);
}

void Entry::setIcon(int iconNumber)
{
	Q_ASSERT(iconNumber >= 0);

	if ((m_data.iconNumber != iconNumber) || (!m_data.customIcon.isNull()))
	{
		m_data.iconNumber = iconNumber;
		m_data.customIcon = QUuid();

		emitModified();
		emitDataChanged();
	}
}

void Entry::setIcon(const QUuid &uuid)
{
	Q_ASSERT(!uuid.isNull());

	if (m_data.customIcon != uuid)
	{
		m_data.customIcon = uuid;
		m_data.iconNumber = 0;

		emitModified();
		emitDataChanged();
	}
}

void Entry::setForegroundColor(const QString &colorStr)
{
	set(m_data.foregroundColor, colorStr);
}

void Entry::setBackgroundColor(const QString &colorStr)
{
	set(m_data.backgroundColor, colorStr);
}

void Entry::setOverrideUrl(const QString &url)
{
	set(m_data.overrideUrl, url);
}

void Entry::setTags(const QString &tags)
{
	auto taglist = tags.split(TagDelimiterRegex, Qt::SkipEmptyParts);

	// Trim whitespace before/after tag text
	for (auto &tag: taglist)
	{
		tag = tag.trimmed();
	}

	// Remove duplicates
	taglist = Tools::asSet(taglist).values();
	// Sort alphabetically
	taglist.sort();
	set(m_data.tags, taglist);
}

void Entry::addTag(const QString &tag)
{
	auto cleanTag = tag.trimmed();
	cleanTag.remove(TagDelimiterRegex);

	auto taglist = m_data.tags;
	if (!taglist.contains(cleanTag))
	{
		taglist.append(cleanTag);
		taglist.sort();
		set(m_data.tags, taglist);
	}
}

void Entry::removeTag(const QString &tag)
{
	auto cleanTag = tag.trimmed();
	cleanTag.remove(TagDelimiterRegex);

	auto taglist = m_data.tags;
	if (taglist.removeAll(tag) > 0)
	{
		set(m_data.tags, taglist);
	}
}

void Entry::setTimeInfo(const TimeInfo &timeInfo)
{
	m_data.timeInfo = timeInfo;
}

void Entry::setTitle(const QString &title)
{
	m_attributes->set(EntryAttributes::TitleKey, title, m_attributes->isProtected(EntryAttributes::TitleKey));
}

void Entry::setUrl(const QString &url)
{
	bool remove = (url != m_attributes->value(EntryAttributes::URLKey))
		&& (m_attributes->value(EntryAttributes::RememberCmdExecAttr) == QStringLiteral("1")
			|| m_attributes->value(EntryAttributes::RememberCmdExecAttr) == QStringLiteral("0"));

	if (remove)
	{
		m_attributes->remove(EntryAttributes::RememberCmdExecAttr);
	}

	m_attributes->set(EntryAttributes::URLKey, url, m_attributes->isProtected(EntryAttributes::URLKey));
}

void Entry::setUsername(const QString &username)
{
	m_attributes->set(EntryAttributes::UserNameKey, username, m_attributes->isProtected(EntryAttributes::UserNameKey));
}

void Entry::setPassword(const QString &password)
{
	// Reset Password Health
	m_data.passwordHealth.reset();
	m_attributes->set(EntryAttributes::PasswordKey, password, m_attributes->isProtected(EntryAttributes::PasswordKey));
}

void Entry::setNotes(const QString &notes)
{
	m_attributes->set(EntryAttributes::NotesKey, notes, m_attributes->isProtected(EntryAttributes::NotesKey));
}

void Entry::setExpires(const bool &value)
{
	if (m_data.timeInfo.expires() != value)
	{
		m_data.timeInfo.setExpires(value);
		emitModified();
	}
}

void Entry::setExpiryTime(const QDateTime &dateTime)
{
	if (m_data.timeInfo.expiryTime() != dateTime)
	{
		m_data.timeInfo.setExpiryTime(dateTime);
		emitModified();
	}
}

QList<Entry*> Entry::historyItems()
{
	return m_history;
}

const QList<Entry*>& Entry::historyItems() const
{
	return m_history;
}

void Entry::addHistoryItem(Entry *entry)
{
	Q_ASSERT(!entry->parent());

	m_history.append(entry);
	emitModified();
}

void Entry::removeHistoryItems(const QList<Entry*> &historyEntries)
{
	if (historyEntries.isEmpty())
	{
		return;
	}

	for (Entry *entry: historyEntries)
	{
		Q_ASSERT(!entry->parent());
		Q_ASSERT(entry->uuid().isNull() || entry->uuid() == uuid());
		Q_ASSERT(m_history.contains(entry));

		m_history.removeOne(entry);
		delete entry;
	}

	emitModified();
}

void Entry::truncateHistory()
{
	const Database *db = database();

	if (!db)
	{
		return;
	}

	bool changed = false;
	int histMaxItems = db->metadata()->historyMaxItems();
	if (histMaxItems > -1)
	{
		int historyCount = 0;
		QMutableListIterator<Entry*> i(m_history);

		i.toBack();
		while (i.hasPrevious())
		{
			historyCount++;
			Entry *entry = i.previous();
			if (historyCount > histMaxItems)
			{
				delete entry;
				i.remove();
				changed = true;
			}
		}
	}

	int histMaxSize = db->metadata()->historyMaxSize();
	if (histMaxSize > -1)
	{
		int size_value = 0;

		QMutableListIterator<Entry*> i(m_history);
		i.toBack();
		while (i.hasPrevious())
		{
			Entry *historyItem = i.previous();

			// don't calculate size if it's already above the maximum
			if (size_value <= histMaxSize)
			{
				size_value += historyItem->size();
			}

			if (size_value > histMaxSize)
			{
				delete historyItem;
				i.remove();
				changed = true;
			}
		}
	}

	if (changed)
	{
		emitModified();
	}
}

bool Entry::equals(const Entry *other, CompareItemOptions options) const
{
	if (!other)
	{
		return false;
	}

	if (m_uuid != other->uuid())
	{
		return false;
	}

	if (!m_data.equals(other->m_data, options))
	{
		return false;
	}

	if (*m_customData != *other->m_customData)
	{
		return false;
	}

	if (*m_attributes != *other->m_attributes)
	{
		return false;
	}

	if (*m_attachments != *other->m_attachments)
	{
		return false;
	}

	if (!options.testFlag(CompareItemIgnoreHistory))
	{
		if (m_history.count() != other->m_history.count())
		{
			return false;
		}

		for (int i = 0; i < m_history.count(); ++i)
		{
			if (!m_history[i]->equals(other->m_history[i], options))
			{
				return false;
			}
		}
	}

	return true;
}

QStringList Entry::calculateDifference(const Entry *other)
{
	QStringList modifiedFields;

	if (*attributes() != *other->attributes())
	{
		bool foundAttribute = false;

		if (title() != other->title())
		{
			modifiedFields << tr("Title");
			foundAttribute = true;
		}

		if (username() != other->username())
		{
			modifiedFields << tr("Username");
			foundAttribute = true;
		}

		if (password() != other->password())
		{
			modifiedFields << tr("Password");
			foundAttribute = true;
		}

		if (url() != other->url())
		{
			modifiedFields << tr("URL");
			foundAttribute = true;
		}

		if (notes() != other->notes())
		{
			modifiedFields << tr("Notes");
			foundAttribute = true;
		}

		if (!foundAttribute)
		{
			modifiedFields << tr("Custom Attributes");
		}
	}

	if (iconNumber() != other->iconNumber() || iconUuid() != other->iconUuid())
	{
		modifiedFields << tr("Icon");
	}

	if (foregroundColor() != other->foregroundColor() || backgroundColor() != other->backgroundColor())
	{
		modifiedFields << tr("Color");
	}

	if ((timeInfo().expires() != other->timeInfo().expires())
		|| (timeInfo().expiryTime() != other->timeInfo().expiryTime()))
	{
		modifiedFields << tr("Expiration");
	}

	if (totp() != other->totp())
	{
		modifiedFields << tr("TOTP");
	}

	if (*customData() != *other->customData())
	{
		modifiedFields << tr("Custom Data");
	}

	if (*attachments() != *other->attachments())
	{
		modifiedFields << tr("Attachments");
	}

	if (tags() != other->tags())
	{
		modifiedFields << tr("Tags");
	}

	return modifiedFields;
}

Entry* Entry::clone(CloneFlags flags) const
{
	Entry *entry = new Entry();
	entry->setUpdateTimeinfo(false);
	if (flags & CloneNewUuid)
	{
		entry->m_uuid = QUuid::createUuid();
	}
	else
	{
		entry->m_uuid = m_uuid;
	}

	entry->m_data = m_data;
	entry->m_customData->copyDataFrom(m_customData);
	entry->m_attributes->copyDataFrom(m_attributes);
	entry->m_attachments->copyDataFrom(m_attachments);

	if (flags & CloneIncludeHistory)
	{
		for (Entry *historyItem: m_history)
		{
			Entry *historyItemClone = historyItem->clone(flags & ~CloneIncludeHistory & ~CloneNewUuid & ~CloneResetTimeInfo);
			historyItemClone->setUpdateTimeinfo(false);
			historyItemClone->setUuid(entry->uuid());
			historyItemClone->setUpdateTimeinfo(true);
			entry->addHistoryItem(historyItemClone);
		}
	}

	if (flags & CloneResetTimeInfo)
	{
		QDateTime now = Clock::currentDateTimeUtc();
		entry->m_data.timeInfo.setCreationTime(now);
		entry->m_data.timeInfo.setLastModificationTime(now);
		entry->m_data.timeInfo.setLastAccessTime(now);
		entry->m_data.timeInfo.setLocationChanged(now);
	}

	if (flags & CloneRenameTitle)
	{
		entry->setTitle(tr("%1 - Clone").arg(entry->title()));
	}

	entry->setUpdateTimeinfo(true);

	return entry;
}

void Entry::copyDataFrom(const Entry *other)
{
	setUpdateTimeinfo(false);
	m_data = other->m_data;
	m_customData->copyDataFrom(other->m_customData);
	m_attributes->copyDataFrom(other->m_attributes);
	m_attachments->copyDataFrom(other->m_attachments);
	setUpdateTimeinfo(true);
}

void Entry::beginUpdate()
{
	Q_ASSERT(!m_tmpHistoryItem);

	m_tmpHistoryItem = std::make_unique<Entry>();
	m_tmpHistoryItem->setUpdateTimeinfo(false);
	m_tmpHistoryItem->m_uuid = m_uuid;
	m_tmpHistoryItem->m_data = m_data;
	m_tmpHistoryItem->m_attributes->copyDataFrom(m_attributes);
	m_tmpHistoryItem->m_attachments->copyDataFrom(m_attachments);

	m_modifiedSinceBegin = false;
}

bool Entry::endUpdate()
{
	Q_ASSERT(m_tmpHistoryItem);

	if (m_modifiedSinceBegin)
	{
		m_tmpHistoryItem->setUpdateTimeinfo(true);
		addHistoryItem(m_tmpHistoryItem.release());
		truncateHistory();
	}

	m_tmpHistoryItem.reset();

	return m_modifiedSinceBegin;
}

void Entry::updateModifiedSinceBegin()
{
	m_modifiedSinceBegin = true;
}

void Entry::moveUp()
{
	if (m_group)
	{
		m_group->moveEntryUp(this);
	}
}

void Entry::moveDown()
{
	if (m_group)
	{
		m_group->moveEntryDown(this);
	}
}

Group* Entry::group()
{
	return m_group;
}

const Group* Entry::group() const
{
	return m_group;
}

void Entry::setGroup(Group *group, bool trackPrevious)
{
	Q_ASSERT(group);

	if (m_group == group)
	{
		return;
	}

	if (m_group)
	{
		m_group->removeEntry(this);

		if (m_group->database() && m_group->database() != group->database())
		{
			setPreviousParentGroup(nullptr);
			m_group->database()->addDeletedObject(m_uuid);

			// copy custom icon to the new database
			if ((!iconUuid().isNull())
				&& group->database()
				&& m_group->database()->metadata()->hasCustomIcon(iconUuid())
				&& (!group->database()->metadata()->hasCustomIcon(iconUuid())))
			{
				group->database()->metadata()->addCustomIcon(
					iconUuid(),
					m_group->database()->metadata()->customIcon(iconUuid()));
			}
		}
		else if (trackPrevious && m_group->database())
		{
			setPreviousParentGroup(m_group);
		}
	}

	QObject::setParent(group);

	m_group = group;
	group->addEntry(this);

	if (m_updateTimeinfo)
	{
		m_data.timeInfo.setLocationChanged(Clock::currentDateTimeUtc());
	}
}

void Entry::emitDataChanged()
{
	Q_EMIT entryDataChanged(this);
}

const Database* Entry::database() const
{
	if (m_group)
	{
		return m_group->database();
	}

	return nullptr;
}

Database* Entry::database()
{
	if (m_group)
	{
		return m_group->database();
	}

	return nullptr;
}

Group* Entry::previousParentGroup()
{
	if (!database() || !database()->rootGroup())
	{
		return nullptr;
	}

	return database()->rootGroup()->findGroupByUuid(m_data.previousParentGroupUuid);
}

const Group* Entry::previousParentGroup() const
{
	if (!database() || !database()->rootGroup())
	{
		return nullptr;
	}

	return database()->rootGroup()->findGroupByUuid(m_data.previousParentGroupUuid);
}

QUuid Entry::previousParentGroupUuid() const
{
	return m_data.previousParentGroupUuid;
}

void Entry::setPreviousParentGroupUuid(const QUuid &uuid)
{
	set(m_data.previousParentGroupUuid, uuid);
}

void Entry::setPreviousParentGroup(const Group *group)
{
	setPreviousParentGroupUuid(group ? group->uuid() : QUuid());
}

bool EntryData::operator==(const EntryData &other) const
{
	return equals(other, CompareItemDefault);
}

bool EntryData::operator!=(const EntryData &other) const
{
	return !(*this == other);
}

bool EntryData::equals(const EntryData &other, CompareItemOptions options) const
{
	if (::compare(iconNumber, other.iconNumber, options) != 0)
	{
		return false;
	}

	if (::compare(customIcon, other.customIcon, options) != 0)
	{
		return false;
	}

	if (::compare(foregroundColor, other.foregroundColor, options) != 0)
	{
		return false;
	}

	if (::compare(backgroundColor, other.backgroundColor, options) != 0)
	{
		return false;
	}

	if (::compare(overrideUrl, other.overrideUrl, options) != 0)
	{
		return false;
	}

	if (::compare(tags, other.tags, options) != 0)
	{
		return false;
	}
	if (!timeInfo.equals(other.timeInfo, options))
	{
		return false;
	}

	if (!totpSettings.isNull() && (!other.totpSettings.isNull()))
	{
		// Both have TOTP settings, compare them
		if (::compare(totpSettings->key, other.totpSettings->key, options) != 0)
		{
			return false;
		}

		if (::compare(totpSettings->digits, other.totpSettings->digits, options) != 0)
		{
			return false;
		}

		if (::compare(totpSettings->step, other.totpSettings->step, options) != 0)
		{
			return false;
		}
	}
	else if (totpSettings.isNull() != other.totpSettings.isNull())
	{
		// The existance of TOTP has changed between these entries
		return false;
	}

	if (::compare(excludeFromReports, other.excludeFromReports, options) != 0)
	{
		return false;
	}

	if (::compare(previousParentGroupUuid, other.previousParentGroupUuid, options) != 0)
	{
		return false;
	}

	return true;
}
