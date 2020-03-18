#include "PSRQTimer.h"

extern PSRStatisticsServer *app;//����qtӦ���ⲿȫ�ֱ���
extern DBPOOLHANDLE dbPoolHandle;//��ʷ���ⲿȫ�ֱ���
extern Logger psrStatisticsServerLog;//log4c��־

PSRQTimer::PSRQTimer()
	: QObject()
{
	QTimer *timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), this, SLOT(timerDone()));
	lastTime = QDateTime::currentDateTime();// ��ʱ���ϴ�ִ��ʱ��
	timer->start(1000);//ÿ���ӽ�����ѯ
}

PSRQTimer::~PSRQTimer()
{

}

void PSRQTimer::timerDone()
{
	//����ʱ���ַ�����ʽ
	QString timeFormat = "yyyy-MM-dd-hh-mm-ss";

	//��ȡ��ǰʱ�䣬��ת��Ϊ�ַ�����ʽ
	QDateTime currentTime = QDateTime::currentDateTime();
	QString currentTimeString = currentTime.toString(timeFormat);
	QStringList currentTimeList = currentTimeString.split("-");
	
	//��ȡ�ϴ�ִ��ʱ����ַ�����ʽ
	QString lastTimeString = lastTime.toString(timeFormat);
	QStringList lastTimeList = lastTimeString.split("-");

	qDebug()<<currentTimeString;
	qDebug()<<lastTimeString; 

	//�����ʱ������ʱͳ��
	if (lastTimeList[3]!=currentTimeList[3])
	{

		//������ݿ�����״̬
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

		//������գ�������ͳ��
		if (lastTimeList[2]!=currentTimeList[2])
		{
			app->statisticSwitch("DaySwitchStat", "day");
			app->statisticBusbar("DayBusbarStat", "day");
			app->statisticTransformer("DayTransformStat", "day");
			app->statisticLine("DayLineStat", "day");
		}
	}

	//����ǰʱ�丳���ϴ�ִ��ʱ��
	lastTime = currentTime;
}