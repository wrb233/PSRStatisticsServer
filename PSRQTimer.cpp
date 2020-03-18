#include "PSRQTimer.h"

extern PSRStatisticsServer *app;//声明qt应用外部全局变量
extern DBPOOLHANDLE dbPoolHandle;//历史库外部全局变量
extern Logger psrStatisticsServerLog;//log4c日志

PSRQTimer::PSRQTimer()
	: QObject()
{
	QTimer *timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), this, SLOT(timerDone()));
	lastTime = QDateTime::currentDateTime();// 定时器上次执行时间
	timer->start(1000);//每秒钟进行轮询
}

PSRQTimer::~PSRQTimer()
{

}

void PSRQTimer::timerDone()
{
	//定义时间字符串格式
	QString timeFormat = "yyyy-MM-dd-hh-mm-ss";

	//获取当前时间，并转换为字符串格式
	QDateTime currentTime = QDateTime::currentDateTime();
	QString currentTimeString = currentTime.toString(timeFormat);
	QStringList currentTimeList = currentTimeString.split("-");
	
	//获取上次执行时间的字符串格式
	QString lastTimeString = lastTime.toString(timeFormat);
	QStringList lastTimeList = lastTimeString.split("-");

	qDebug()<<currentTimeString;
	qDebug()<<lastTimeString; 

	//如果跨时，进行时统计
	if (lastTimeList[3]!=currentTimeList[3])
	{

		//检查数据库连接状态
		CPS_ORM_UpdatePool(dbPoolHandle);
		if(!CPS_ORM_GetDBPoolState(dbPoolHandle))
		{
			if(CPS_ORM_Connect(dbPoolHandle))
			{
				CPS_ORM_SetAutoCommit(dbPoolHandle,1);
				CPS_ORM_SetStreamAutoCommit(dbPoolHandle,1);
			}

			if(CPS_ORM_GetDBPoolState(dbPoolHandle)==0)
			{
				LOG4CPLUS_ERROR(psrStatisticsServerLog, "***********Database dbPoolHandle State Error********");
			}
		}

		app->statisticSwitch("HourSwitchStat", "hour");
		app->statisticBusbar("HourBusbarStat", "hour");
		app->statisticTransformer("HourTransformStat", "hour");
		app->statisticLine("HourLineStat", "hour");

		//如果跨日，进行日统计
		if (lastTimeList[2]!=currentTimeList[2])
		{
			app->statisticSwitch("DaySwitchStat", "day");
			app->statisticBusbar("DayBusbarStat", "day");
			app->statisticTransformer("DayTransformStat", "day");
			app->statisticLine("DayLineStat", "day");
		}
	}

	//将当前时间赋给上次执行时间
	lastTime = currentTime;
}