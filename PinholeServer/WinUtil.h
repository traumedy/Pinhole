#pragma once

#if defined(Q_OS_WIN)

class QProcessEnvironment;

bool ShutdownOrReboot(bool reboot);
bool OverrideCreateProcess();
void ElevateNextCreateProcess();
bool CreateSharedMemory(const QString& name, const QByteArray& data);
bool IsUserLoggedIn();
bool GetInteractiveUsername(QString& userstring, QString& sidstring);
QString SubstituteEnvironmentstring(const QString& envString, const QProcessEnvironment& procEnv);

#endif

