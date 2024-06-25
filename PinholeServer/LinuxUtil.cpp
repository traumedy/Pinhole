#include "LinuxUtil.h"
#include "Logger.h"
#include "../common/PinholeCommon.h"

#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)

#include <QElapsedTimer>

#include <errno.h>
#include <unistd.h>
#include <grp.h>
#include <utmp.h>
#include <pwd.h>
#if 1
#include <sys/reboot.h>
#else
#include <linux/reboot.h>
#endif

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QThread>
#include <QDirIterator>

bool ShutdownOrReboot(bool doReboot)
{
	// Commit buffer cache to disk
	sync();
#if 1
	if (0 != reboot(doReboot ? RB_AUTOBOOT : RB_POWER_OFF))
#else
	if (0 != _reboot(LINUX_REBOOT_MAGIC1, LINUX_REBOOT_MAGIC2, doReboot ? LINUX_REBOOT_CMD_RESTART : LINUX_REBOOT_CMD_POWER_OFF))
#endif
	{
		Logger(LOG_ERROR) << "Error " << errno << (doReboot ? " rebooting" : " shutting down") << " the system: " << strerror(errno);
		return false;
	}
	return true;
}


bool getX11User(uid_t* uid, gid_t* gid, QString* name, QString* device)
{
	QFile file(UTMP_FILE);
	if (!file.open(QIODevice::ReadOnly))
		return false;

	struct utmp ut_entry;

	while (file.read((char*)&ut_entry, sizeof(struct utmp)) == sizeof(ut_entry))
	{
		if (ut_entry.ut_type != USER_PROCESS)
			continue;

		if (ut_entry.ut_host[0] != ':')
			continue;

		// Usernames may not be 0 terminated
		char tmpUser[UT_NAMESIZE + 1] = { 0 };
		strncpy(tmpUser, ut_entry.ut_user, UT_NAMESIZE);

		if (nullptr != name)
			*name = QString::fromUtf8(tmpUser);

		struct passwd* ppasswd = getpwnam(tmpUser);
		if (nullptr == ppasswd)
			return false;
		if (nullptr != uid)
			*uid = ppasswd->pw_uid;
		if (nullptr != gid)
			*gid = ppasswd->pw_gid;
		if (nullptr != device)
			*device = QString::fromUtf8(ut_entry.ut_host);
			
		return true;
	}

	return false;
}


// Returns true if X11 is running
bool isX11Running()
{
	return getX11User(nullptr, nullptr, nullptr, nullptr);
}


// Picks 'some user' that exists on the system
bool pickUser(uid_t* uid, gid_t* gid, QString& user)
{
	QStringList users;
	QDirIterator dirit("/home", QDirIterator::NoIteratorFlags);
	while (dirit.hasNext())
	{
		QFileInfo fileInfo(dirit.next());
		if (fileInfo.isDir()&& fileInfo.fileName()[0] != ".")
		{
			users.push_back(fileInfo.fileName());
		}
	}

	if (users.isEmpty())
	{
		Logger(LOG_WARNING) << "No user directories";
		return false;
	}

	// Try not to use root unless no choice
	if (users.length() == 1)
		user = users[0];
	else if (users[0] == "root")
		user = users[1];
	else
		user = users[0];

	struct passwd* pw = getpwnam(user.toLocal8Bit().data());
	if (nullptr == pw)
	{
		Logger(LOG_WARNING) << QString("Failed to get info for user '%1'")
			.arg(user);
		return false;
	}

	if (nullptr != uid)
		*uid = pw->pw_uid;
	if (nullptr != gid)
		*gid = pw->pw_gid;

	return true;
}


// Changes the current process to another user and lowers priveleges
bool ChangeToInteractiveUser(bool useX11)
{
	// Query interactive user info
	gid_t safeGid;
	uid_t safeUid;
	QString user;

	if (useX11)
	{
		if (!getX11User(&safeUid, &safeGid, &user, nullptr))
		{
			Logger(LOG_WARNING) << "Failed to get X11 user";
				user.clear();
		}
	}

	if (user.isEmpty())
	{
		if (!pickUser(&safeUid, &safeGid, user))
			return false;
	}

	// Retrieve group membership
	gid_t gids[64];
	int count = sizeof(gids) / sizeof(gids[0]);
	if (0 >= ::getgrouplist(user.toLocal8Bit().data(), safeGid, gids, &count))
	{
		Logger(LOG_WARNING) << QString("getgrouplist(%1) failed: ").arg(user) << errno;
		gids[0] = (gid_t)-1;
		count = 0;
	}
	// Set group membership
	if (0 != ::setgroups(count, gids))
	{
		Logger(LOG_WARNING) << QString("setgroups(%1) failed: ").arg(count) << errno;
	}
	// Set primary group
	if (0 != ::setgid(safeGid))
	{
		Logger(LOG_WARNING) << QString("setgid(%1) failed: ").arg(safeGid) << errno;
	}
	// Change user
	if (0 != ::setuid(safeUid))
	{
		Logger(LOG_WARNING) << QString("setuid(%1) failed: ").arg(safeUid) << errno;
	}
	//::umask(0);
	return true;
}




#endif

