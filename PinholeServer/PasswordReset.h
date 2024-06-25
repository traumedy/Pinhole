#pragma once

#include <QObject>

class PasswordReset : public QObject
{
	Q_OBJECT

public:
	PasswordReset(QObject *parent);
	~PasswordReset();
	bool exec();
};
