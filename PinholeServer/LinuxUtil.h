#pragma once

#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)

bool ShutdownOrReboot(bool doReboot);
bool getX11User(uid_t* uid, gid_t* gid, QString* name, QString* device);
bool isX11Running();
bool pickUser(uid_t* uid, gid_t* gid, QString& user);
bool ChangeToInteractiveUser(bool useX11);

#endif

