#include "PSRStatisticsServer.h"
#include <QDebug>
#include "PSRQTimer.h"
#include "DBUtil.h"
#include "version.h"
#include "FileConfig.h"
#include "OTypeAndATypeInit.h"

PSRStatisticsServer *app;//����appȫ�ֱ���
Database *m_OMSDatabase;//ʵʱ���ݿ�ָ��ȫ�ֱ���
DBPOOLHANDLE dbPoolHandle;//������ʷ��ȫ�ֱ���
Logger psrStatisticsServerLog = Logger::getInstance(LOG4CPLUS_TEXT("psrStatisticsServerLog"));//ȫ����־

int main(int argc, char *argv[])
{ 
	//��ӡ�汾��Ϣ
	if (echoVersion(argc, argv)) 
	{
		return 0;
	}

	//���ô�ӡ��־
	QString processName = "psrStatisticsServerLog";
	char* cpsenv = getenv("CPS_ENV");
	QString cfgPath = QString::fromUtf8(cpsenv) + "/etc/logConfig/" + processName + ".properties"; 
	QFile cfgFile(cfgPath);  
	if (!cfgFile.exists()) 
	{    
		createDefaultLogConfigFile(cfgPath, "psrStatisticsServerLog"); 
	} 

	log4cplus::initialize();  
	LogLog::getLogLog()->setInternalDebugging(true); 
	Logger root = Logger::getRoot(); 
	ConfigureAndWatchThread configureThread(cfgPath.toStdString().c_str(), 5 * 1000);

	//��ȡͳ��Ӧ��ָ��,ͬʱ����ʵʱ��ָ��
	OptionList optionList;
	app = new PSRStatisticsServer(argc, argv, NULL,optionList,RUNTIME_SCOPE);

	//��ȡ��ʷ��ָ��
	if (!connectDB(argc, argv))
	{
		std::cout<<"failed"<<std::endl;
		return 0;
	}

	//��ʼ��OType��AType
	if (!initOTypeAndAType())
	{
		LOG4CPLUS_ERROR(psrStatisticsServerLog, "Init OType And AType error");
		std::exit(0);
	}

	//��ʼ�������ź���������
	app->initFaultMeasureType();

	//��ʼ��������Ϣ
	app->initDataInfo();

	//��ʼ��ע��ص�
	app->initNotificationCallback();

	//������ʱ��
	PSRQTimer psrQTimer;

	//TEST
	//app->statisticSwitch("HourSwitchStat", "hour");
	//app->statisticBusbar("HourBusbarStat", "hour");
	//app->statisticTransformer("HourTransformStat", "hour");
	//app->statisticLine("HourLineStat", "hour");
	//app->statisticSwitch("DaySwitchStat", "day");
	//app->statisticBusbar("DayBusbarStat", "day");
	//app->statisticTransformer("DayTransformStat", "day");
	//app->statisticLine("DayLineStat", "day");
	//TEST

	//��ʼ����app
	app->svc();
	
	delete app;

	return 0;
}