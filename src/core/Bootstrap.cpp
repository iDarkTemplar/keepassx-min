/*
 *  Copyright (C) 2020 KeePassXC Team <team@keepassxc.org>
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

#include "Bootstrap.h"
#include "config-keepassx.h"
#include "core/Translator.h"

#if defined(HAVE_RLIMIT_CORE)
#include <sys/resource.h>
#endif

#if defined(HAVE_PR_SET_DUMPABLE)
#include <sys/prctl.h>
#endif

namespace Bootstrap
{
	/**
	 * Perform early application bootstrapping that does not rely on a QApplication
	 * being present.
	 */
	void bootstrap(const QString &uiLanguage)
	{
#ifdef QT_NO_DEBUG
		disableCoreDumps();
#endif

		Translator::installTranslators(uiLanguage);
	}

	// LCOV_EXCL_START
	void disableCoreDumps()
	{
		// default to true
		// there is no point in printing a warning if this is not implemented on the platform
		bool success = true;

#if defined(HAVE_RLIMIT_CORE)
		struct rlimit limit;
		limit.rlim_cur = 0;
		limit.rlim_max = 0;
		success = success && (setrlimit(RLIMIT_CORE, &limit) == 0);
#endif

#if defined(HAVE_PR_SET_DUMPABLE)
		success = success && (prctl(PR_SET_DUMPABLE, 0) == 0);
#endif

		if (!success)
		{
			qWarning("Unable to disable core dumps.");
		}
	}
} // namespace Bootstrap
