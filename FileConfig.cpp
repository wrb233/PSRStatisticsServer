#include "FileConfig.h"

//获取配置文件PSRStatisticsServer.ini中的定时器周期
int getTimerValue()
{
	char* cpsenv = getenv("CPS_ENV");
	QString cfgPath = QString::fromUtf8(cpsenv) + "/etc/" + "PSRStatisticsServer.ini"; 
	QFile cfgFile(cfgPath);  
	if (!cfgFile.exists())
	{
		QSettings *configIniWrite = new QSettings(cfgPath, QSettings::IniFormat);   
		configIniWrite->setValue("/timer/second", "3600");
		delete configIniWrite;
	}

	QSettings *configIniRead = new QSettings(cfgPath, QSettings::IniFormat); 
	QString timerSecond = configIniRead->value("/timer/second").toString();  
	
	if (timerSecond=="")
	{
		timerSecond = "3600";
	}

	delete configIniRead;
	
	return timerSecond.toInt();
	
}

//创建默认log4Cplus配置文件
void createDefaultLogConfigFile(QString fileName,QString LoggerName)
{ 
	QStringList  content;  
	content.clear(); 
	content.append("log4cplus.rootLogger=ERROR, STDOUT"); 
	content.append("log4cplus.logger."+LoggerName +"=ERROR,R");  
	content.append("log4cplus.appender.STDOUT=log4cplus::ConsoleAppender");  
	content.append("log4cplus.appender.STDOUT.layout=log4cplus::PatternLayout");  
	content.append("log4cplus.appender.STDOUT.layout.ConversionPattern=%d{%m/%d/%y %H:%M:%S} [%t] %-5p %c{2} %%%x%% - %m [%l]%n");  

	content.append("log4cplus.appender.R=log4cplus::RollingFileAppender"); 
	char* cpsenv = getenv("CPS_ENV"); 
	QString logFilePath = QString::fromUtf8(cpsenv) + "/data/log/" + LoggerName + ".log";  
	content.append("log4cplus.appender.R.File = "+logFilePath);  

	content.append("log4cplus.appender.R.MaxFileSize=5000KB"); 
	content.append("log4cplus.appender.R.MaxBackupIndex=10");  
	content.append("log4cplus.appender.R.layout=log4cplus::PatternLayout"); 
	content.append("log4cplus.appender.R.layout.ConversionPattern=%d{%m/%d/%y %H:%M:%S} [%t] %-5p %c{2} %%%x%% - %m [%l]%n"); 

	QFile file(fileName);  
	if (file.open(QFile::WriteOnly | QFile::Text))   { 
		QTextStream out(&file);  
		for (int i = 0; i < content.size(); i++) {  
			out << content.at(i) << endl;  
		} 
	}  
} 