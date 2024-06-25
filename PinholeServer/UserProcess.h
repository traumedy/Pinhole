#pragma once
// Subclass of QProcess to allow further control

#include <QProcess>

class Settings;

class UserProcess : public QProcess
{
	Q_OBJECT

public:
	UserProcess(Settings* settings, QObject *parent = nullptr);
	~UserProcess();
	void setElevated(bool set) { m_elevated = set; }

protected:
	void setupChildProcess() override;

private:
	bool m_elevated = false;
	Settings* m_settings = nullptr;
};
