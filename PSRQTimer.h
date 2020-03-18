#ifndef PSRQTIMER_H
#define PSRQTIMER_H

#include "common.h"
#include "PSRStatisticsServer.h"
#include "FileConfig.h"

class PSRQTimer:public QObject
{
	Q_OBJECT
public:
	PSRQTimer();
	~PSRQTimer();
public slots:
	void timerDone();
private:
	QDateTime lastTime;
};

#endif