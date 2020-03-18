#include "PSRStatisticsServer.h"
#include <QDebug>
#include "PSRQTimer.h"
#include "DBUtil.h"
#include "version.h"
#include "FileConfig.h"
#include "OTypeAndATypeInit.h"

PSRStatisticsServer *app;//定义app全局变量
Database *m_OMSDatabase;//实时数据库指针全局变量
DBPOOLHANDLE dbPoolHandle;//定义历史库全局变量
Logger psrStatisticsServerLog = Logger::getInstance(LOG4CPLUS_TEXT("psrStatisticsServerLog"));//全局日志

int main(int argc, char *argv[])
{ 
	//打印版本信息
	if (echoVersion(argc, argv)) 
	{
		return 0;
	}

	//配置打印日志
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

	//获取统计应用指针,同时构造实时库指针
	OptionList optionList;
	app = new PSRStatisticsServer(argc, argv, NULL,optionList,RUNTIME_SCOPE);

	//获取历史库指针
	if (!connectDB(argc, argv))
	{
		std::cout<<"failed"<<std::endl;
		return 0;
	}

	//初始化OType和AType
	if (!initOTypeAndAType())
	{
		LOG4CPLUS_ERROR(psrStatisticsServerLog, "Init OType And AType error");
		std::exit(0);
	}

	//初始化故障信号量测类型
	app->initFaultMeasureType();

	//初始化数据信息
	app->initDataInfo();

	//初始化注册回调
	app->initNotificationCallback();

	//启动定时器
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

	//开始运行app
	app->svc();
	
	delete app;

	return 0;
}