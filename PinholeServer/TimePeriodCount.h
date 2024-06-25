#pragma once

// Counts the number of occurances in a time period

#include <QDateTime>

class TimePeriodCount
{
public:
	int getCount() const
	{
		return m_list.size();
	}

	void reset()
	{
		m_list.clear();
	}

	int increment(qint64 period)
	{
		QDateTime now = QDateTime::currentDateTime();

		// Remove entries older than period
		auto it = m_list.begin();
		while (it != m_list.end())
		{
			if (it->msecsTo(now) > period)
				it = m_list.erase(it);
			else
				++it;
		}

		m_list.append(now);

		return m_list.size();
	}

private:
	QList<QDateTime> m_list;
};

