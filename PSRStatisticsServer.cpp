#include "PSRStatisticsServer.h"

extern PSRStatisticsServer *app;//声明qt应用外部全局变量
extern Database *m_OMSDatabase;//实时数据库指针外部全局变量
extern DBPOOLHANDLE dbPoolHandle;//历史库外部全局变量
extern Logger psrStatisticsServerLog;//log4c日志

//构造函数
PSRStatisticsServer::PSRStatisticsServer(int &argc, char **argv,SignalHandler sigtermHandler,const OptionList& optionList,
	EnvironmentScope environmentScope) : QtApplication(argc, argv, sigtermHandler, optionList, environmentScope)

{
	//在构造函数中获取实时库对象
	m_OMSDatabase = getDatabase();

	//进程容错
	try
	{
		activateFaultTolerance();
	}catch(...)
	{
		LOG4CPLUS_ERROR(psrStatisticsServerLog, "Active faultTolerance error");
		std::exit(0);
	}

	//测试如果不是主机
	if (!getFaultTolerance()->amITheCurrentMachine())
	{
		LOG4CPLUS_ERROR(psrStatisticsServerLog, "I am not TheCurrentMachine");
	}
}


//析构函数
PSRStatisticsServer::~PSRStatisticsServer()

{

}

//初始化故障信号量测类型
void PSRStatisticsServer::initFaultMeasureType()
{
	//将开关和所有事故的Measuretype测量类型存储到QList中
	StringData keyNameData;
	//将开关类型和全部故障类型存储在数组中
	StringData keyNameDatas[] = {"INF_EVENT","FT_SJ","FT_YES","FT_DXJD","FT_DXJD_SJ","FT_JD_A","FT_JD_B","FT_JD_C","FT_XJJD"
		,"FT_XJJD_SJ","FT_XJJD_AB","FT_XJJD_BC","FT_XJJD_CA","FT_XJJD_ABC"
		,"FT_XJDL","FT_XJDL_SJ","FT_XJDL_AB","FT_XJDL_BC","FT_XJDL_CA","FT_XJDL_ABC","FT_CHZ"
		,"FT_GL_A","FT_GL_B","FT_GL_C","FT_JD","FT_DL","FT_DL_A","FT_DL_B","FT_DL_C"
	    ,"FTU_SJ","FTU_YES","FTU_DXJD","FTU_DXJD_SJ","FTU_JD_A","FTU_JD_B","FTU_JD_C","FTU_XJJD","FTU_XJJD_SJ"
	    ,"FTU_XJJD_AB","FTU_XJJD_BC","FTU_XJJD_CA","FTU_XJJD_ABC","FTU_XJDL","FTU_XJDL_SJ","FTU_XJDL_AB","FTU_XJDL_BC","FTU_XJDL_CA","FTU_XJDL_ABC"};
	int arrayLength = sizeof(keyNameDatas)/sizeof(StringData);
	//qDebug()<<arrayLength;
	for (int i=0;i<arrayLength;i++)
	{
		Condition condition(AT_KeyName, EQ, &keyNameDatas[i]);
		int num = m_OMSDatabase->find(OT_MeasurementType, &condition, 1);
		if (num>0)
		{
			ObId *measureTypeObId = new ObId[num];

			try
			{
				m_OMSDatabase->find(OT_MeasurementType, &condition, 1, measureTypeObId, num);
			}
			catch (Exception& e)
			{
				qDebug()<<"error:initFaultMeasureType(): init condition error!!!"<<e.getDescription().c_str();
				LOG4CPLUS_ERROR(psrStatisticsServerLog, "error:initFaultMeasureType(): init condition error!!!"+(std::string)keyNameDatas[i]);
			}

			faultMeasureTypeObIds.append(measureTypeObId[0]);

			//删除指针内存空间
			delete[] measureTypeObId;
		}
	}
}

//初始化数据结构
void PSRStatisticsServer::initDataInfo()
{
	initSwitchDataInfo();
	initBusbarDataInfo();
	initTransformerDataInfo();
	initLineDataInfo();
}

//初始化注册回调函数
void PSRStatisticsServer::initNotificationCallback()
{
	//监听开关-刀闸操作次数
	Request reqTrigerSwitchOptCount;
	reqTrigerSwitchOptCount.set(AT_ControlResult, OT_DPCPoint, NEW_NOTIFICATION, notificationSwitchOptCount, (void*)this,NOTIFY_ON_WRITE); 
	m_OMSDatabase->notify(&reqTrigerSwitchOptCount, 1);

	//监听开关-刀闸运行时长
	Request reqTrigerSwitchRuntime;
	reqTrigerSwitchRuntime.set(AT_State, OT_DPSPoint, NEW_NOTIFICATION, notificationSwitchRuntime, (void*)this,NOTIFY_ON_CHANGE); 
	m_OMSDatabase->notify(&reqTrigerSwitchRuntime, 1);

	//监听开关故障跳闸次数
	Request reqTrigerSwitchAccident;
	reqTrigerSwitchAccident.set(AT_FaultState, OT_PMSBreaker, NEW_NOTIFICATION, notificationSwitchAccident, (void*)this,NOTIFY_ON_WRITE); 
	m_OMSDatabase->notify(&reqTrigerSwitchAccident, 1);

	//监听母线越限次数
	Request reqTrigerBusbarLimit;
	reqTrigerBusbarLimit.set(AT_Limit, OT_MVPoint, NEW_NOTIFICATION, notificationBusbarLimit, (void*)this,NOTIFY_ON_CHANGE); 
	m_OMSDatabase->notify(&reqTrigerBusbarLimit, 1);

	//监听变压器调档次数
	Request reqTrigerTransformerShift;
	reqTrigerTransformerShift.set(AT_ControlResult, OT_BSCPoint, NEW_NOTIFICATION, notificationTransformerShift, (void*)this,NOTIFY_ON_CHANGE); 
	m_OMSDatabase->notify(&reqTrigerTransformerShift, 1);

	//监听线路负荷值变化
	Request reqTrigerLineValue;
	reqTrigerLineValue.set(AT_Value, OT_MVPoint, NEW_NOTIFICATION, notificationLineValue, (void*)this, NOTIFY_ON_CHANGE); 
	m_OMSDatabase->notify(&reqTrigerLineValue, 1);

	//监听线路和开关带电状态是否变化
	Request reqTrigerLineAndSwitchRTEnergized;
	reqTrigerLineAndSwitchRTEnergized.set(AT_RTEnergized, OT_PMSTerminal, NEW_NOTIFICATION, notificationLineAndSwitchRTEnergized, (void*)this, NOTIFY_ON_CHANGE); 
	m_OMSDatabase->notify(&reqTrigerLineAndSwitchRTEnergized, 1);

	//监听线路（馈线）动作次数，成功失败次数
	Request reqTrigerFeederAction;
	reqTrigerFeederAction.set(AT_ObjOrder, OT_Feeder, NEW_NOTIFICATION, notificationFeederObjOrder, (void*)this, NOTIFY_ON_WRITE); 
	m_OMSDatabase->notify(&reqTrigerFeederAction, 1);

	//监听增量更新
	IncrementCommit ic;
	ic.NotfyIncrementCommit();
}

//统计开关-刀闸
void PSRStatisticsServer::statisticSwitch(QString psrName, QString type)
{
	//如果表不存在，则新建
	QString tableName = createTableByDate(psrName, 0);

    FloatData P_A_MVPointValueData;
	FloatData P_B_MVPointValueData;
	FloatData P_C_MVPointValueData;
	ObId P_A_MVPointObId;
	ObId P_B_MVPointObId;
	ObId P_C_MVPointObId;
	FloatData Q_A_MVPointValueData;
	FloatData Q_B_MVPointValueData;
	FloatData Q_C_MVPointValueData;
	ObId Q_A_MVPointObId;
	ObId Q_B_MVPointObId;
	ObId Q_C_MVPointObId;
	float max;
	float min;
	float avg;

	//循环遍历所有开关-刀闸
	QMap<ObId, Switch>::iterator switchMapIterator;
	for (switchMapIterator=switchs.begin();switchMapIterator!=switchs.end();switchMapIterator++)
	{
		//计算三相有功不平衡率
		P_A_MVPointObId = switchMapIterator->relatePoints["P_A_MVPoint"];
		P_B_MVPointObId = switchMapIterator->relatePoints["P_B_MVPoint"];
		P_C_MVPointObId = switchMapIterator->relatePoints["P_C_MVPoint"];
		if (P_A_MVPointObId!=0&&P_B_MVPointObId!=0&&P_C_MVPointObId!=0)
		{
			try
			{
				m_OMSDatabase->read(P_A_MVPointObId, AT_Value, &P_A_MVPointValueData);
				m_OMSDatabase->read(P_B_MVPointObId, AT_Value, &P_B_MVPointValueData);
				m_OMSDatabase->read(P_C_MVPointObId, AT_Value, &P_C_MVPointValueData);
			}
			catch (Exception& e)
			{
				LOG4CPLUS_ERROR(psrStatisticsServerLog, "error: read error!!!"+QString::number(P_A_MVPointObId).toStdString());
			}

			max = getMax((float)P_A_MVPointValueData, (float)P_B_MVPointValueData, (float)P_C_MVPointValueData);
			min = getMin((float)P_A_MVPointValueData, (float)P_B_MVPointValueData, (float)P_C_MVPointValueData);
			avg = ((float)P_A_MVPointValueData+(float)P_B_MVPointValueData+(float)P_C_MVPointValueData)/3;

			if (avg!=0)
			{
				switchMapIterator->threePUnbalanceRate = (max-min)/avg;
			}
		}

		//计算三相无功不平衡率
		Q_A_MVPointObId = switchMapIterator->relatePoints["Q_A_MVPoint"];
		Q_B_MVPointObId = switchMapIterator->relatePoints["Q_B_MVPoint"];
		Q_C_MVPointObId = switchMapIterator->relatePoints["Q_C_MVPoint"];
		if (Q_A_MVPointObId!=0&&Q_B_MVPointObId!=0&&Q_C_MVPointObId!=0)
		{
			try
			{
				m_OMSDatabase->read(Q_A_MVPointObId, AT_Value, &Q_A_MVPointValueData);
				m_OMSDatabase->read(Q_B_MVPointObId, AT_Value, &Q_B_MVPointValueData);
				m_OMSDatabase->read(Q_C_MVPointObId, AT_Value, &Q_C_MVPointValueData);
			}
			catch (Exception& e)
			{
				LOG4CPLUS_ERROR(psrStatisticsServerLog, "error: read error!!!"+QString::number(Q_A_MVPointObId).toStdString());
			}

			max = this->getMax((float)Q_A_MVPointValueData, (float)Q_B_MVPointValueData, (float)Q_C_MVPointValueData);
			min = this->getMin((float)Q_A_MVPointValueData, (float)Q_B_MVPointValueData, (float)Q_C_MVPointValueData);
			avg = ((float)Q_A_MVPointValueData+(float)Q_B_MVPointValueData+(float)Q_C_MVPointValueData)/3;

			if (avg!=0)
			{
				switchMapIterator->threeQUnbalanceRate = (max-min)/avg;
			}
		}

		//计算运行时间
		QString timeFormat = "yyyy-MM-dd-hh-mm-ss";
		QString time24Format = "yyyy-MM-dd hh:mm:ss";
		QDateTime currentTime = QDateTime::currentDateTime();
		QStringList currentTimeList = currentTime.toString(timeFormat).split("-");// 当前时间分割

		QString currentHourStartTime = currentTimeList[0]+"-"+currentTimeList[1]+"-"+currentTimeList[2]+"-"+currentTimeList[3]+"-00-00";//当前时间对应的整点时间
		QDateTime currentHourStartDate = QDateTime::fromString(currentHourStartTime, timeFormat);

		QString currentDayStartTime = currentTimeList[0]+"-"+currentTimeList[1]+"-"+currentTimeList[2]+"-00-00-00";//当前时间对应的当天起始时间
		QDateTime currentDayStartDate = QDateTime::fromString(currentDayStartTime, timeFormat);
	
		//如果是小时级统计
		if (type=="hour")
		{
			//如果存在开关DPS点
			if (switchMapIterator->lastSwitchOnTime!="")
			{
				//如果此时开关闭合状态
				if (switchMapIterator->isSwitchOn)
				{
					int lastToCurrent = QDateTime::fromString(switchMapIterator->lastSwitchOnTime, timeFormat).secsTo(currentHourStartDate);//开关上次闭合到当前整点时间的秒数

					//如果时间差大于3600s即1小时
					if (lastToCurrent>3600)
					{
						switchMapIterator->hourRunTime = 3600;
					}else{
						switchMapIterator->hourRunTime += lastToCurrent;
						if (switchMapIterator->hourRunTime>3600)
						{
							switchMapIterator->hourRunTime = 3600;
						}
					}
				}	
			}

			//如果存在两个端子
			if (switchMapIterator->lastPowerOnTime!="")
			{
				//如果此时线路或开关是送电状态
				if (switchMapIterator->isPowerOn)
				{
					int lastToCurrent = QDateTime::fromString(switchMapIterator->lastPowerOnTime, timeFormat).secsTo(currentHourStartDate);//线路或开关上次送电到当前整点时间的秒数

					//如果时间差大于3600s即1小时
					if (lastToCurrent>3600)
					{
						switchMapIterator->hourPowerOnTime = 3600;
					}else{
						switchMapIterator->hourPowerOnTime += lastToCurrent;
						if (switchMapIterator->hourPowerOnTime>3600)
						{
							switchMapIterator->hourPowerOnTime = 3600;
						}
					}
				}	

				//线路或开关停电时长
				switchMapIterator->hourPowerOffTime = 3600 - switchMapIterator->hourPowerOnTime;

				//如果此时线路或开关是断电状态
				if (!switchMapIterator->isPowerOn)
				{
					//如果是故障断电
					if (switchMapIterator->isAccidentOff)
					{
						int lastToCurrent = QDateTime::fromString(switchMapIterator->lastAccidentOffTime, timeFormat).secsTo(currentHourStartDate);//线路或开关上次送电到当前整点时间的秒数

						//如果时间差大于3600s即1小时
						if (lastToCurrent>3600)
						{
							switchMapIterator->hourAccidentOffTime = 3600;
						}else{
							switchMapIterator->hourAccidentOffTime += lastToCurrent;
							if (switchMapIterator->hourAccidentOffTime>3600)
							{
								switchMapIterator->hourAccidentOffTime = 3600;
							}
						}
					}
				}	

				//人工停电时长
				switchMapIterator->hourArtificialOffTime = switchMapIterator->hourPowerOffTime - switchMapIterator->hourAccidentOffTime;
				
				//人工停电次数
				switchMapIterator->hourArtificialOffCount = switchMapIterator->hourPowerOffCount - switchMapIterator->hourAccidentOffCount;

				// 因设计机制问题，纠错处理
				if (switchMapIterator->hourArtificialOffTime < 0)
				{
					switchMapIterator->hourArtificialOffTime = 0;
					switchMapIterator->hourAccidentOffTime = switchMapIterator->hourPowerOffTime;
				}
				if (switchMapIterator->hourArtificialOffCount < 0)
				{
					switchMapIterator->hourArtificialOffCount = 0;
					switchMapIterator->hourAccidentOffCount = switchMapIterator->hourPowerOffCount;
				}
			}

			switchMapIterator->statisticDate = currentHourStartDate.toString(time24Format);

			//插入数据到历史表
			QString insertSql = "insert into "+tableName+" ("
				+"OBJECTID, "
				+"OTYPE, "
				+"THREEPUNBALANCERATE, "
				+"THREEQUNBALANCERATE, "
				+"OPTCOUNT, "
				+"ACCIDENTTRIPCOUNT, "
				+"CUMULATIVERUNTIME, "
				+"PowerOnCount, "
				+"PowerOffCount, "
				+"PowerOnTime, "
				+"PowerOffTime, "
				+"AccidentOffCount, "
				+"ArtificialOffCount, "
				+"AccidentOffTime, "
				+"ArtificialOffTime, "
				+"ProtectionsCount, "
				+"REPLACECOUNT, "
				+"REPLACETIME, "
				+"SDATE)"
				+"values ("
				+QString::number(switchMapIterator->obId)+", "
				+QString::number(switchMapIterator->otype)+", "
				+QString::number(switchMapIterator->threePUnbalanceRate)+", "
				+QString::number(switchMapIterator->threeQUnbalanceRate)+", "
				+QString::number(switchMapIterator->hourOptCount)+", "
				+QString::number(switchMapIterator->hourAccidentTripCount)+", "
				+QString::number(switchMapIterator->hourRunTime)+", "
				+QString::number(switchMapIterator->hourPowerOnCount)+", "
				+QString::number(switchMapIterator->hourPowerOffCount)+", "
				+QString::number(switchMapIterator->hourPowerOnTime)+", "
				+QString::number(switchMapIterator->hourPowerOffTime)+", "
				+QString::number(switchMapIterator->hourAccidentOffCount)+", "
				+QString::number(switchMapIterator->hourArtificialOffCount)+", "
				+QString::number(switchMapIterator->hourAccidentOffTime)+", "
				+QString::number(switchMapIterator->hourArtificialOffTime)+", "
				+QString::number(switchMapIterator->hourProtectionsCount)+", "
				+QString::number(switchMapIterator->hourReplaceCount)+", "
				+"to_timestamp('"+switchMapIterator->replaceTime+"', 'yyyy-mm-dd hh24:mi:ss')"+", "
				+"to_timestamp('"+switchMapIterator->statisticDate+"', 'yyyy-mm-dd hh24:mi:ss')"+")";

			qDebug()<<insertSql;

			//如果是主机
			if (getFaultTolerance()->amITheCurrentMachine())
			{
				if(CPS_ORM_DirectExecuteSql(dbPoolHandle,insertSql.toStdString())>=0)
				{
					qDebug()<<"INSERT HOUR DATA SUCCESS!";
				}
			}

			//归零
			switchMapIterator->hourOptCount = 0;
			switchMapIterator->hourAccidentTripCount = 0;
			switchMapIterator->hourRunTime = 0;
			switchMapIterator->hourReplaceCount = 0;

			switchMapIterator->hourPowerOffCount = 0;
			switchMapIterator->hourPowerOffTime = 0;
			switchMapIterator->hourPowerOnCount = 0;
			switchMapIterator->hourPowerOnTime = 0;

			switchMapIterator->hourAccidentOffCount = 0;
			switchMapIterator->hourAccidentOffTime = 0;
			switchMapIterator->hourArtificialOffCount = 0;
			switchMapIterator->hourArtificialOffTime = 0;
			switchMapIterator->hourProtectionsCount = 0;
		}else if (type=="day")//如果是日级统计
		{
			//如果存在开关DPS点
			if (switchMapIterator->lastSwitchOnTime!="")
			{
				//如果此时开关闭合状态
				if (switchMapIterator->isSwitchOn)
				{
					int lastToCurrent = QDateTime::fromString(switchMapIterator->lastSwitchOnTime, timeFormat).secsTo(currentDayStartDate);//开关上次闭合到当天起始时间的秒数

					//如果时间差大于3600*24s即1天
					if (lastToCurrent>3600*24)
					{
						switchMapIterator->dayRunTime = 3600*24;
					}else{
						switchMapIterator->dayRunTime += lastToCurrent;
						if (switchMapIterator->dayRunTime>3600*24)
						{
							switchMapIterator->dayRunTime = 3600*24;
						}
					}
				}
			}

			//如果存在两个端子
			if (switchMapIterator->lastPowerOnTime!="")
			{
				//如果此时线路或开关是送电状态
				if (switchMapIterator->isPowerOn)
				{
					int lastToCurrent = QDateTime::fromString(switchMapIterator->lastPowerOnTime, timeFormat).secsTo(currentDayStartDate);//线路或开关上次送电到当天零点时间的秒数

					//如果时间差大于3600*24s即1天
					if (lastToCurrent>3600*24)
					{
						switchMapIterator->dayPowerOnTime = 3600*24;;
					}else{
						switchMapIterator->dayPowerOnTime += lastToCurrent;
						if (switchMapIterator->dayPowerOnTime>3600*24)
						{
							switchMapIterator->dayPowerOnTime = 3600*24;
						}
					}
				}	

				//线路或开关停电时长
				switchMapIterator->dayPowerOffTime = 3600*24 - switchMapIterator->dayPowerOnTime;

				//如果此时线路或开关是断电状态
				if (!switchMapIterator->isPowerOn)
				{
					//如果是故障断电
					if (switchMapIterator->isAccidentOff)
					{
						int lastToCurrent = QDateTime::fromString(switchMapIterator->lastAccidentOffTime, timeFormat).secsTo(currentDayStartDate);//线路或开关上次送电到当天零点时间的秒数

						//如果时间差大于3600*24s即1天
						if (lastToCurrent>3600*24)
						{
							switchMapIterator->dayAccidentOffTime = 3600*24;
						}else{
							switchMapIterator->dayAccidentOffTime += lastToCurrent;
							if (switchMapIterator->dayAccidentOffTime>3600*24)
							{
								switchMapIterator->dayAccidentOffTime = 3600*24;
							}
						}
					}
				}	

				//人工停电时长
				switchMapIterator->dayArtificialOffTime = switchMapIterator->dayPowerOffTime - switchMapIterator->dayAccidentOffTime;
				//人工停电次数
				switchMapIterator->dayArtificialOffCount = switchMapIterator->dayPowerOffCount - switchMapIterator->dayAccidentOffCount;
				// 因设计机制问题，纠错处理
				if (switchMapIterator->dayArtificialOffTime < 0)
				{
					switchMapIterator->dayArtificialOffTime = 0;
					switchMapIterator->dayAccidentOffTime = switchMapIterator->dayPowerOffTime;
				}
				if (switchMapIterator->dayArtificialOffCount < 0)
				{
					switchMapIterator->dayArtificialOffCount = 0;
					switchMapIterator->dayAccidentOffCount = switchMapIterator->dayPowerOffCount;
				}
			}
			
			switchMapIterator->statisticDate = currentDayStartDate.addDays(-1).toString(time24Format);

			//插入数据到历史表
			QString insertSql = "insert into "+tableName+" ("
				+"OBJECTID, "
				+"OTYPE, "
				+"THREEPUNBALANCERATE, "
				+"THREEQUNBALANCERATE, "
				+"OPTCOUNT, "
				+"ACCIDENTTRIPCOUNT, "
				+"CUMULATIVERUNTIME, "
				+"PowerOnCount, "
				+"PowerOffCount, "
				+"PowerOnTime, "
				+"PowerOffTime, "
				+"AccidentOffCount, "
				+"ArtificialOffCount, "
				+"AccidentOffTime, "
				+"ArtificialOffTime, "
				+"ProtectionsCount, "
				+"REPLACECOUNT, "
				+"REPLACETIME, "
				+"SDATE)"
				+"values ("
				+QString::number(switchMapIterator->obId)+", "
				+QString::number(switchMapIterator->otype)+", "
				+QString::number(switchMapIterator->threePUnbalanceRate)+", "
				+QString::number(switchMapIterator->threeQUnbalanceRate)+", "
				+QString::number(switchMapIterator->dayOptCount)+", "
				+QString::number(switchMapIterator->dayAccidentTripCount)+", "
				+QString::number(switchMapIterator->dayRunTime)+", "
				+QString::number(switchMapIterator->dayPowerOnCount)+", "
				+QString::number(switchMapIterator->dayPowerOffCount)+", "
				+QString::number(switchMapIterator->dayPowerOnTime)+", "
				+QString::number(switchMapIterator->dayPowerOffTime)+", "
				+QString::number(switchMapIterator->dayAccidentOffCount)+", "
				+QString::number(switchMapIterator->dayArtificialOffCount)+", "
				+QString::number(switchMapIterator->dayAccidentOffTime)+", "
				+QString::number(switchMapIterator->dayArtificialOffTime)+", "
				+QString::number(switchMapIterator->dayProtectionsCount)+", "
				+QString::number(switchMapIterator->dayReplaceCount)+", "
				+"to_timestamp('"+switchMapIterator->replaceTime+"', 'yyyy-mm-dd hh24:mi:ss')"+", "
				+"to_timestamp('"+switchMapIterator->statisticDate+"', 'yyyy-mm-dd hh24:mi:ss')"+")";

			qDebug()<<insertSql;

			//如果是主机
			if (getFaultTolerance()->amITheCurrentMachine())
			{
				if(CPS_ORM_DirectExecuteSql(dbPoolHandle,insertSql.toStdString())>=0)
				{
					qDebug()<<"INSERT DAY DATA SUCCESS!";
				}
			}

			//归零
			switchMapIterator->dayOptCount = 0;
			switchMapIterator->dayAccidentTripCount = 0;
			switchMapIterator->dayRunTime = 0;
			switchMapIterator->dayReplaceCount = 0;

			switchMapIterator->dayPowerOffCount = 0;
			switchMapIterator->dayPowerOffTime = 0;
			switchMapIterator->dayPowerOnCount = 0;
			switchMapIterator->dayPowerOnTime = 0;

			switchMapIterator->dayAccidentOffCount = 0;
			switchMapIterator->dayAccidentOffTime = 0;
			switchMapIterator->dayArtificialOffCount = 0;
			switchMapIterator->dayArtificialOffTime = 0;

			switchMapIterator->dayProtectionsCount = 0;
		}
	}
}

//统计母线
void PSRStatisticsServer::statisticBusbar(QString psrName, QString type)
{
	//如果表不存在，则新建
	QString tableName = createTableByDate(psrName, 1);

	FloatData V_A_MVPointValueData;
	FloatData V_B_MVPointValueData;
	FloatData V_C_MVPointValueData;
	ObId V_A_MVPointObId;
	ObId V_B_MVPointObId;
	ObId V_C_MVPointObId;
	float max;
	float min;
	float avg;

	//循环遍历所有母线
	QMap<ObId, Busbar>::iterator busbarMapIterator;
	for (busbarMapIterator=busbars.begin();busbarMapIterator!=busbars.end();busbarMapIterator++)
	{
		//计算三相电压不平衡率
		V_A_MVPointObId = busbarMapIterator->relatePoints["V_A_MVPoint"];
		V_B_MVPointObId = busbarMapIterator->relatePoints["V_B_MVPoint"];
		V_C_MVPointObId = busbarMapIterator->relatePoints["V_C_MVPoint"];
		if (V_A_MVPointObId!=0&&V_B_MVPointObId!=0&&V_C_MVPointObId!=0)
		{
			try
			{
				m_OMSDatabase->read(V_A_MVPointObId, AT_Value, &V_A_MVPointValueData);
				m_OMSDatabase->read(V_B_MVPointObId, AT_Value, &V_B_MVPointValueData);
				m_OMSDatabase->read(V_C_MVPointObId, AT_Value, &V_C_MVPointValueData);
			}
			catch (Exception& e)
			{
				LOG4CPLUS_ERROR(psrStatisticsServerLog, "error: read error!!!"+QString::number(V_A_MVPointObId).toStdString());
			}

			max = getMax((float)V_A_MVPointValueData, (float)V_B_MVPointValueData, (float)V_C_MVPointValueData);
			min = getMin((float)V_A_MVPointValueData, (float)V_B_MVPointValueData, (float)V_C_MVPointValueData);
			avg = ((float)V_A_MVPointValueData+(float)V_B_MVPointValueData+(float)V_C_MVPointValueData)/3;

			if (avg!=0)
			{
				busbarMapIterator->threeVUnbalanceRate = (max-min)/avg;
			}
		}

		QString timeFormat = "yyyy-MM-dd-hh-mm-ss";
		QString time24Format = "yyyy-MM-dd hh:mm:ss";
		QDateTime currentTime = QDateTime::currentDateTime();
		QStringList currentTimeList = currentTime.toString(timeFormat).split("-");// 当前时间分割

		QString currentHourStartTime = currentTimeList[0]+"-"+currentTimeList[1]+"-"+currentTimeList[2]+"-"+currentTimeList[3]+"-00-00";//当前时间对应的整点时间
		QDateTime currentHourStartDate = QDateTime::fromString(currentHourStartTime, timeFormat);

		QString currentDayStartTime = currentTimeList[0]+"-"+currentTimeList[1]+"-"+currentTimeList[2]+"-00-00-00";//当前时间对应的当天起始时间
		QDateTime currentDayStartDate = QDateTime::fromString(currentDayStartTime, timeFormat);


		//如果是小时级统计
		if (type=="hour")
		{
			busbarMapIterator->statisticDate = currentHourStartDate.toString(time24Format);

			//插入数据到历史表
			QString insertSql = "insert into "+tableName+" ("
				+"OBJECTID, "
				+"SDATE, "
				+"OTYPE, "
				+"THREEVUNBALANCERATE, "
				+"LIMITCOUNT) "
				+"values ("
				+QString::number(busbarMapIterator->obId)+", "
				+"to_timestamp('"+busbarMapIterator->statisticDate+"', 'yyyy-mm-dd hh24:mi:ss')"+", "
				+QString::number(busbarMapIterator->otype)+", "
				+QString::number(busbarMapIterator->threeVUnbalanceRate)+", "
				+QString::number(busbarMapIterator->hourLimitCount)+")";
			qDebug()<<insertSql;

			//如果是主机
			if (getFaultTolerance()->amITheCurrentMachine())
			{
				if(CPS_ORM_DirectExecuteSql(dbPoolHandle,insertSql.toStdString())>=0)
				{
					qDebug()<<"INSERT HOUR DATA SUCCESS";
				}	
			}

			//归零
			busbarMapIterator->hourLimitCount = 0;
		}else if (type=="day")//如果是日级统计
		{
			busbarMapIterator->statisticDate = currentDayStartDate.addDays(-1).toString(time24Format);

			//插入数据到历史表
			QString insertSql = "insert into "+tableName+" ("
				+"OBJECTID, "
				+"SDATE, "
				+"OTYPE, "
				+"THREEVUNBALANCERATE, "
				+"LIMITCOUNT) "
				+"values ("
				+QString::number(busbarMapIterator->obId)+", "
				+"to_timestamp('"+busbarMapIterator->statisticDate+"', 'yyyy-mm-dd hh24:mi:ss')"+", "
				+QString::number(busbarMapIterator->otype)+", "
				+QString::number(busbarMapIterator->threeVUnbalanceRate)+", "
				+QString::number(busbarMapIterator->dayLimitCount)+")";
			qDebug()<<insertSql;

			//如果是主机
			if (getFaultTolerance()->amITheCurrentMachine())
			{
				if(CPS_ORM_DirectExecuteSql(dbPoolHandle,insertSql.toStdString())>=0)
				{
					qDebug()<<"INSERT DAY DATA SUCCESS";
				}	
			}

			//归零
			busbarMapIterator->dayLimitCount = 0;
		}
	}

}

//统计变压器
void PSRStatisticsServer::statisticTransformer(QString psrName, QString type)
{
	//如果表不存在，则新建
	QString tableName = createTableByDate(psrName, 2);
	FloatData paramMvaData;

	//循环遍历所有变压器
	QMap<ObId, Transformer>::iterator transformerMapIterator;
	for (transformerMapIterator=transformers.begin();transformerMapIterator!=transformers.end();transformerMapIterator++)
	{
		//计算除配变外的负载率
		/*OType oType = m_OMSDatabase->extractOType(transformerMapIterator->obId);
		OType distributionTransformerOType = m_OMSDatabase->matchOType("DistributionTransformer");
		if (oType!=distributionTransformerOType)
		{
			qDebug()<<transformerMapIterator->obId;
			m_OMSDatabase->read(transformerMapIterator->obId, paramMva, &paramMvaData);
			if ((float)paramMvaData>0)
			{
				ObId P_MVPointObId = transformerMapIterator->relatePoints["P_MVPoint"];
				if (P_MVPointObId!=0)
				{
					FloatData valueData;
					m_OMSDatabase->read(P_MVPointObId, pValue, &valueData);
					transformerMapIterator->loadRate = (float)valueData/(float)paramMvaData;
				}
			}
		}*/

		QString timeFormat = "yyyy-MM-dd-hh-mm-ss";
		QString time24Format = "yyyy-MM-dd hh:mm:ss";
		QDateTime currentTime = QDateTime::currentDateTime();
		QStringList currentTimeList = currentTime.toString(timeFormat).split("-");// 当前时间分割

		QString currentHourStartTime = currentTimeList[0]+"-"+currentTimeList[1]+"-"+currentTimeList[2]+"-"+currentTimeList[3]+"-00-00";//当前时间对应的整点时间
		QDateTime currentHourStartDate = QDateTime::fromString(currentHourStartTime, timeFormat);

		QString currentDayStartTime = currentTimeList[0]+"-"+currentTimeList[1]+"-"+currentTimeList[2]+"-00-00-00";//当前时间对应的当天起始时间
		QDateTime currentDayStartDate = QDateTime::fromString(currentDayStartTime, timeFormat);

		//如果是小时级统计
		if (type=="hour")
		{
			transformerMapIterator->statisticDate = currentHourStartDate.toString(time24Format);

			//插入数据到历史表
			QString insertSql = "insert into "+tableName+" ("
				+"OBJECTID, "
				+"SDATE, "
				+"OTYPE, "
				+"SHIFTCOUNT, "
				+"ProtectionsCount, "
				+"LOADRATE) "
				+"values ("
				+QString::number(transformerMapIterator->obId)+", "
				+"to_timestamp('"+transformerMapIterator->statisticDate+"', 'yyyy-mm-dd hh24:mi:ss')"+", "
				+QString::number(transformerMapIterator->otype)+", "
				+QString::number(transformerMapIterator->hourShiftCount)+", "
				+QString::number(transformerMapIterator->hourProtectionsCount)+", "
				+QString::number(transformerMapIterator->loadRate)+")";
			qDebug()<<insertSql;

			//如果是主机
			if (getFaultTolerance()->amITheCurrentMachine())
			{
				if(CPS_ORM_DirectExecuteSql(dbPoolHandle,insertSql.toStdString())>=0)
				{
					qDebug()<<"INSERT HOUR DATA SUCCESS";
				}	
			}

			//归零
			transformerMapIterator->hourShiftCount = 0;

			transformerMapIterator->hourProtectionsCount = 0;
		}else if (type=="day")//如果是日级统计
		{
			transformerMapIterator->statisticDate = currentDayStartDate.addDays(-1).toString(time24Format);

			//插入数据到历史表
			QString insertSql = "insert into "+tableName+" ("
				+"OBJECTID, "
				+"SDATE, "
				+"OTYPE, "
				+"SHIFTCOUNT, "
				+"ProtectionsCount, "
				+"LOADRATE) "
				+"values ("
				+QString::number(transformerMapIterator->obId)+", "
				+"to_timestamp('"+transformerMapIterator->statisticDate+"', 'yyyy-mm-dd hh24:mi:ss')"+", "
				+QString::number(transformerMapIterator->otype)+", "
				+QString::number(transformerMapIterator->dayShiftCount)+", "
				+QString::number(transformerMapIterator->dayProtectionsCount)+", "
				+QString::number(transformerMapIterator->loadRate)+")";
			qDebug()<<insertSql;

			//如果是主机
			if (getFaultTolerance()->amITheCurrentMachine())
			{
				if(CPS_ORM_DirectExecuteSql(dbPoolHandle,insertSql.toStdString())>=0)
				{
					qDebug()<<"INSERT DAY DATA SUCCESS";
				}	
			}

			//归零
			transformerMapIterator->dayShiftCount = 0;

			transformerMapIterator->dayProtectionsCount = 0;
		}
	}
}

//统计线路
void PSRStatisticsServer::statisticLine(QString psrName, QString type)
{
	//如果表不存在，则新建
	QString tableName = createTableByDate(psrName, 3);

	//循环遍历所有线路
	QMap<ObId, Line>::iterator lineMapIterator;

	FloatData V_A_MVPointValueData;
	FloatData V_B_MVPointValueData;
	FloatData V_C_MVPointValueData;
	FloatData P_MVPointValueData;
	ObId V_A_MVPointObId;
	ObId V_B_MVPointObId;
	ObId V_C_MVPointObId;
	ObId P_MVPointObId;
	float max;
	float min;
	float avg;

	for (lineMapIterator=lines.begin();lineMapIterator!=lines.end();lineMapIterator++)
	{
		//计算三相电压不平衡率
		V_A_MVPointObId = lineMapIterator->relatePoints["V_A_MVPoint"];
		V_B_MVPointObId = lineMapIterator->relatePoints["V_B_MVPoint"];
		V_C_MVPointObId = lineMapIterator->relatePoints["V_C_MVPoint"];
		if (V_A_MVPointObId!=0&&V_B_MVPointObId!=0&&V_C_MVPointObId!=0)
		{
			try
			{
				m_OMSDatabase->read(V_A_MVPointObId, AT_Value, &V_A_MVPointValueData);
				m_OMSDatabase->read(V_B_MVPointObId, AT_Value, &V_B_MVPointValueData);
				m_OMSDatabase->read(V_C_MVPointObId, AT_Value, &V_C_MVPointValueData);
			}
			catch (Exception& e)
			{
				LOG4CPLUS_ERROR(psrStatisticsServerLog, "error: read error!!!"+QString::number(V_A_MVPointObId).toStdString());
			}

			max = getMax((float)V_A_MVPointValueData, (float)V_B_MVPointValueData, (float)V_C_MVPointValueData);
			min = getMin((float)V_A_MVPointValueData, (float)V_B_MVPointValueData, (float)V_C_MVPointValueData);
			avg = ((float)V_A_MVPointValueData+(float)V_B_MVPointValueData+(float)V_C_MVPointValueData)/3;

			if (avg!=0)
			{
				lineMapIterator->threeVUnbalanceRate = (max-min)/avg;
			}
		}

		QString timeFormat = "yyyy-MM-dd-hh-mm-ss";
		QString time24Format = "yyyy-MM-dd hh:mm:ss";
		QDateTime currentTime = QDateTime::currentDateTime();
		QStringList currentTimeList = currentTime.toString(timeFormat).split("-");// 当前时间分割

		QString currentHourStartTime = currentTimeList[0]+"-"+currentTimeList[1]+"-"+currentTimeList[2]+"-"+currentTimeList[3]+"-00-00";//当前时间对应的整点时间
		QDateTime currentHourStartDate = QDateTime::fromString(currentHourStartTime, timeFormat);

		QString currentDayStartTime = currentTimeList[0]+"-"+currentTimeList[1]+"-"+currentTimeList[2]+"-00-00-00";//当前时间对应的当天起始时间
		QDateTime currentDayStartDate = QDateTime::fromString(currentDayStartTime, timeFormat);

		//如果是小时级统计
		if (type=="hour")
		{
			//如果存在两个端子
			if (lineMapIterator->lastPowerOnTime!="")
			{
				//如果此时线路或开关是送电状态
				if (lineMapIterator->isPowerOn)
				{
					int lastToCurrent = QDateTime::fromString(lineMapIterator->lastPowerOnTime, timeFormat).secsTo(currentHourStartDate);//线路或开关上次送电到当前整点时间的秒数

					//如果时间差大于3600s即1小时
					if (lastToCurrent>3600)
					{
						lineMapIterator->hourPowerOnTime = 3600;
					}else{
						lineMapIterator->hourPowerOnTime += lastToCurrent;
						if (lineMapIterator->hourPowerOnTime>3600)
						{
							lineMapIterator->hourPowerOnTime = 3600;
						}
					}
				}	

				//线路或开关停电时长
				lineMapIterator->hourPowerOffTime = 3600 - lineMapIterator->hourPowerOnTime;

				//如果此时线路或开关是断电状态
				if (!lineMapIterator->isPowerOn)
				{
					//如果是故障断电
					if (lineMapIterator->isAccidentOff)
					{
						int lastToCurrent = QDateTime::fromString(lineMapIterator->lastAccidentOffTime, timeFormat).secsTo(currentHourStartDate);//线路或开关上次送电到当前整点时间的秒数

						//如果时间差大于3600s即1小时
						if (lastToCurrent>3600)
						{
							lineMapIterator->hourAccidentOffTime = 3600;
						}else{
							lineMapIterator->hourAccidentOffTime += lastToCurrent;
							if (lineMapIterator->hourAccidentOffTime>3600)
							{
								lineMapIterator->hourAccidentOffTime = 3600;
							}
						}
					}
				}	

				//人工停电时长
				lineMapIterator->hourArtificialOffTime = lineMapIterator->hourPowerOffTime - lineMapIterator->hourAccidentOffTime;
				//人工停电次数
				lineMapIterator->hourArtificialOffCount = lineMapIterator->hourPowerOffCount - lineMapIterator->hourAccidentOffCount;
				// 因设计机制问题，纠错处理
				if (lineMapIterator->hourArtificialOffTime < 0)
				{
					lineMapIterator->hourArtificialOffTime = 0;
					lineMapIterator->hourAccidentOffTime = lineMapIterator->hourPowerOffTime;
				}
				if (lineMapIterator->hourArtificialOffCount < 0)
				{
					lineMapIterator->hourArtificialOffCount = 0;
					lineMapIterator->hourAccidentOffCount = lineMapIterator->hourPowerOffCount;
				}
			}

			lineMapIterator->statisticDate = currentHourStartDate.toString(time24Format);

			//插入数据到历史表
			QString insertSql = "insert into "+tableName+" ("
				+"OBJECTID, "
				+"SDATE, "
				+"OTYPE, "
				+"THREEVUNBALANCERATE, "
				+"LOADRATE, "
				+"PowerOnCount, "
				+"PowerOffCount, "
				+"PowerOnTime, "
				+"PowerOffTime, "
				+"AccidentOffCount, "
				+"ArtificialOffCount, "
				+"AccidentOffTime, "
				+"ArtificialOffTime, "
				+"FeederActionTimes, "
				+"FeederSuccessTimes, "
				+"FeederFaultTimes, "
				+"MAXLOAD, "
				+"MAXLOADTIME, "
				+"MINLOAD, "
				+"MINLOADTIME, "
				+"AVGLOAD)"
				+"values ("
				+QString::number(lineMapIterator->obId)+", "
				+"to_timestamp('"+lineMapIterator->statisticDate+"', 'yyyy-mm-dd hh24:mi:ss')"+", "
				+QString::number(lineMapIterator->otype)+", "
				+QString::number(lineMapIterator->threeVUnbalanceRate)+", "
				+QString::number(lineMapIterator->loadRate)+", "
				+QString::number(lineMapIterator->hourPowerOnCount)+", "
				+QString::number(lineMapIterator->hourPowerOffCount)+", "
				+QString::number(lineMapIterator->hourPowerOnTime)+", "
				+QString::number(lineMapIterator->hourPowerOffTime)+", "
				+QString::number(lineMapIterator->hourAccidentOffCount)+", "
				+QString::number(lineMapIterator->hourArtificialOffCount)+", "
				+QString::number(lineMapIterator->hourAccidentOffTime)+", "
				+QString::number(lineMapIterator->hourArtificialOffTime)+", "
				+QString::number(lineMapIterator->hourFeederActionTimes)+", "
				+QString::number(lineMapIterator->hourFeederSuccessTimes)+", "
				+QString::number(lineMapIterator->hourFeederFaultTimes)+", "
				+QString::number(lineMapIterator->hourMaxLoad)+", "
				+"to_timestamp('"+lineMapIterator->hourMaxLoadTime+"', 'yyyy-mm-dd hh24:mi:ss')"+", "
				+QString::number(lineMapIterator->hourMinLoad)+", "
				+"to_timestamp('"+lineMapIterator->hourMinLoadTime+"', 'yyyy-mm-dd hh24:mi:ss')"+", "
				+QString::number(lineMapIterator->hourAvgLoad)+")";
			qDebug()<<insertSql;

			//如果是主机
			if (getFaultTolerance()->amITheCurrentMachine())
			{
				if(CPS_ORM_DirectExecuteSql(dbPoolHandle,insertSql.toStdString())>=0)
				{
					qDebug()<<"INSERT HOUR DATA SUCCESS";
				}	
			}

			//归零
			lineMapIterator->hourAvgLoad = lineMapIterator->latestLoad;
			lineMapIterator->hourLoadChangeCount = 1;
			lineMapIterator->hourMaxLoad = lineMapIterator->latestLoad;
			lineMapIterator->hourMaxLoadTime = currentTime.toString(timeFormat);
			lineMapIterator->hourMinLoad = lineMapIterator->latestLoad;
			lineMapIterator->hourMinLoadTime = currentTime.toString(timeFormat);
			lineMapIterator->hourSumLoad = lineMapIterator->latestLoad;

			lineMapIterator->hourPowerOffCount = 0;
			lineMapIterator->hourPowerOffTime = 0;
			lineMapIterator->hourPowerOnCount = 0;
			lineMapIterator->hourPowerOnTime = 0;

			lineMapIterator->hourAccidentOffCount = 0;
			lineMapIterator->hourAccidentOffTime = 0;
			lineMapIterator->hourArtificialOffCount = 0;
			lineMapIterator->hourArtificialOffTime = 0;

			lineMapIterator->hourFeederActionTimes = 0;
			lineMapIterator->hourFeederSuccessTimes = 0;
			lineMapIterator->hourFeederFaultTimes = 0;
		}else if (type=="day")//如果是日级统计
		{
			//如果存在两个端子
			if (lineMapIterator->lastPowerOnTime!="")
			{
				//如果此时线路或开关是送电状态
				if (lineMapIterator->isPowerOn)
				{
					int lastToCurrent = QDateTime::fromString(lineMapIterator->lastPowerOnTime, timeFormat).secsTo(currentDayStartDate);//线路或开关上次送电到当前整点时间的秒数

					//如果时间差大于3600*24s即1天
					if (lastToCurrent>3600*24)
					{
						lineMapIterator->dayPowerOnTime = 3600*24;;
					}else{
						lineMapIterator->dayPowerOnTime += lastToCurrent;
						if (lineMapIterator->dayPowerOnTime>3600*24)
						{
							lineMapIterator->dayPowerOnTime = 3600*24;
						}
					}
				}	

				//线路或开关停电时长
				lineMapIterator->dayPowerOffTime = 3600*24 - lineMapIterator->dayPowerOnTime;

				//如果此时线路或开关是断电状态
				if (!lineMapIterator->isPowerOn)
				{
					//如果是故障断电
					if (lineMapIterator->isAccidentOff)
					{
						int lastToCurrent = QDateTime::fromString(lineMapIterator->lastAccidentOffTime, timeFormat).secsTo(currentDayStartDate);//线路或开关上次送电到当天零点时间的秒数

						//如果时间差大于3600*24s即1天
						if (lastToCurrent>3600*24)
						{
							lineMapIterator->dayAccidentOffTime = 3600*24;
						}else{
							lineMapIterator->dayAccidentOffTime += lastToCurrent;
							if (lineMapIterator->dayAccidentOffTime>3600*24)
							{
								lineMapIterator->dayAccidentOffTime = 3600*24;
							}
						}
					}
				}	

				//人工停电时长
				lineMapIterator->dayArtificialOffTime = lineMapIterator->dayPowerOffTime - lineMapIterator->dayAccidentOffTime;
				//人工停电次数
				lineMapIterator->dayArtificialOffCount = lineMapIterator->dayPowerOffCount - lineMapIterator->dayAccidentOffCount;
				// 因设计机制问题，纠错处理
				if (lineMapIterator->dayArtificialOffTime < 0)
				{
					lineMapIterator->dayArtificialOffTime = 0;
					lineMapIterator->dayAccidentOffTime = lineMapIterator->dayPowerOffTime;
				}
				if (lineMapIterator->dayArtificialOffCount < 0)
				{
					lineMapIterator->dayArtificialOffCount = 0;
					lineMapIterator->dayAccidentOffCount = lineMapIterator->dayPowerOffCount;
				}
			}

			lineMapIterator->statisticDate = currentDayStartDate.addDays(-1).toString(time24Format);

			//插入数据到历史表
			QString insertSql = "insert into "+tableName+" ("
				+"OBJECTID, "
				+"SDATE, "
				+"OTYPE, "
				+"THREEVUNBALANCERATE, "
				+"LOADRATE, "
				+"PowerOnCount, "
				+"PowerOffCount, "
				+"PowerOnTime, "
				+"PowerOffTime, "
				+"AccidentOffCount, "
				+"ArtificialOffCount, "
				+"AccidentOffTime, "
				+"ArtificialOffTime, "
				+"FeederActionTimes, "
				+"FeederSuccessTimes, "
				+"FeederFaultTimes, "
				+"MAXLOAD, "
				+"MAXLOADTIME, "
				+"MINLOAD, "
				+"MINLOADTIME, "
				+"AVGLOAD)"
				+"values ("
				+QString::number(lineMapIterator->obId)+", "
				+"to_timestamp('"+lineMapIterator->statisticDate+"', 'yyyy-mm-dd hh24:mi:ss')"+", "
				+QString::number(lineMapIterator->otype)+", "
				+QString::number(lineMapIterator->threeVUnbalanceRate)+", "
				+QString::number(lineMapIterator->loadRate)+", "
				+QString::number(lineMapIterator->dayPowerOnCount)+", "
				+QString::number(lineMapIterator->dayPowerOffCount)+", "
				+QString::number(lineMapIterator->dayPowerOnTime)+", "
				+QString::number(lineMapIterator->dayPowerOffTime)+", "
				+QString::number(lineMapIterator->dayAccidentOffCount)+", "
				+QString::number(lineMapIterator->dayArtificialOffCount)+", "
				+QString::number(lineMapIterator->dayAccidentOffTime)+", "
				+QString::number(lineMapIterator->dayArtificialOffTime)+", "
				+QString::number(lineMapIterator->dayFeederActionTimes)+", "
				+QString::number(lineMapIterator->dayFeederSuccessTimes)+", "
				+QString::number(lineMapIterator->dayFeederFaultTimes)+", "
				+QString::number(lineMapIterator->dayMaxLoad)+", "
				+"to_timestamp('"+lineMapIterator->dayMaxLoadTime+"', 'yyyy-mm-dd hh24:mi:ss')"+", "
				+QString::number(lineMapIterator->dayMinLoad)+", "
				+"to_timestamp('"+lineMapIterator->dayMinLoadTime+"', 'yyyy-mm-dd hh24:mi:ss')"+", "
				+QString::number(lineMapIterator->dayAvgLoad)+")";
			qDebug()<<insertSql;

			//如果是主机
			if (getFaultTolerance()->amITheCurrentMachine())
			{
				if(CPS_ORM_DirectExecuteSql(dbPoolHandle,insertSql.toStdString())>=0)
				{
					qDebug()<<"INSERT DAY DATA SUCCESS";
				}	
			}

			//归零
			lineMapIterator->dayAvgLoad = lineMapIterator->latestLoad;
			lineMapIterator->dayLoadChangeCount = 1;
			lineMapIterator->dayMaxLoad = lineMapIterator->latestLoad;
			lineMapIterator->dayMaxLoadTime = currentTime.toString(timeFormat);
			lineMapIterator->dayMinLoad = lineMapIterator->latestLoad;
			lineMapIterator->dayMinLoadTime = currentTime.toString(timeFormat);
			lineMapIterator->daySumLoad = lineMapIterator->latestLoad;

			lineMapIterator->dayPowerOffCount = 0;
			lineMapIterator->dayPowerOffTime = 0;
			lineMapIterator->dayPowerOnCount = 0;
			lineMapIterator->dayPowerOnTime = 0;

			lineMapIterator->dayAccidentOffCount = 0;
			lineMapIterator->dayAccidentOffTime = 0;
			lineMapIterator->dayArtificialOffCount = 0;
			lineMapIterator->dayArtificialOffTime = 0;

			lineMapIterator->dayFeederActionTimes = 0;
			lineMapIterator->dayFeederSuccessTimes = 0;
			lineMapIterator->dayFeederFaultTimes = 0;
		}
	}
}

//以时间元素建表
QString PSRStatisticsServer::createTableByDate(QString psrName, int index)
{
	//构造表名，以时间元素
	QDateTime dt = QDateTime::currentDateTime();
	QString timeFormat = "yyyy-MM-dd-hh-mm-ss";
	QString timeString = dt.toString(timeFormat);
	QStringList timeList = timeString.split("-");
	QString tableName = "H_"+psrName+"_"+timeList[0]+timeList[1];

	//如果表不存在，则新建表
	if (CPS_ORM_ExistTable(dbPoolHandle, tableName.toStdString())==0)
	{
		ORMColumnDescVector m_descVector;
		if(index==0)//创建开关-刀闸统计表
		{
			ORMColumnDesc switchColumnDesc[19];

			switchColumnDesc[0].m_strName = "ObjectId";
			switchColumnDesc[0].m_strType = "bigint(19)";
			switchColumnDesc[0].m_bNullOK = 0;
			switchColumnDesc[0].m_bIndexed = 1;
			switchColumnDesc[0].m_bKey = 0;
			switchColumnDesc[0].m_bUnique = 0;
			m_descVector.push_back(switchColumnDesc[0]);

			switchColumnDesc[1].m_strName = "SDate";
			switchColumnDesc[1].m_strType = "timestamp";
			switchColumnDesc[1].m_bNullOK = 0;
			switchColumnDesc[1].m_bIndexed = 1;
			switchColumnDesc[1].m_bKey = 0;
			switchColumnDesc[1].m_bUnique = 0;
			m_descVector.push_back(switchColumnDesc[1]);

			switchColumnDesc[2].m_strName = "OType";
			switchColumnDesc[2].m_strType = "int";
			switchColumnDesc[2].m_bNullOK = 1;
			switchColumnDesc[2].m_bIndexed = 0;
			switchColumnDesc[2].m_bKey = 0;
			switchColumnDesc[2].m_bUnique = 0;
			m_descVector.push_back(switchColumnDesc[2]);

			switchColumnDesc[3].m_strName = "ThreePUnbalanceRate";
			switchColumnDesc[3].m_strType = "double";
			switchColumnDesc[3].m_bNullOK = 1;
			switchColumnDesc[3].m_bIndexed = 0;
			switchColumnDesc[3].m_bKey = 0;
			switchColumnDesc[3].m_bUnique = 0;
			m_descVector.push_back(switchColumnDesc[3]);

			switchColumnDesc[4].m_strName = "ThreeQUnbalanceRate";
			switchColumnDesc[4].m_strType = "double";
			switchColumnDesc[4].m_bNullOK = 1;
			switchColumnDesc[4].m_bIndexed = 0;
			switchColumnDesc[4].m_bKey = 0;
			switchColumnDesc[4].m_bUnique = 0;
			m_descVector.push_back(switchColumnDesc[4]);

			switchColumnDesc[5].m_strName = "OptCount";
			switchColumnDesc[5].m_strType = "int";
			switchColumnDesc[5].m_bNullOK = 1;
			switchColumnDesc[5].m_bIndexed = 0;
			switchColumnDesc[5].m_bKey = 0;
			switchColumnDesc[5].m_bUnique = 0;
			m_descVector.push_back(switchColumnDesc[5]);

			switchColumnDesc[6].m_strName = "AccidentTripCount";
			switchColumnDesc[6].m_strType = "int";
			switchColumnDesc[6].m_bNullOK = 1;
			switchColumnDesc[6].m_bIndexed = 0;
			switchColumnDesc[6].m_bKey = 0;
			switchColumnDesc[6].m_bUnique = 0;
			m_descVector.push_back(switchColumnDesc[6]);

			switchColumnDesc[7].m_strName = "CumulativeRunTime";
			switchColumnDesc[7].m_strType = "int";
			switchColumnDesc[7].m_bNullOK = 1;
			switchColumnDesc[7].m_bIndexed = 0;
			switchColumnDesc[7].m_bKey = 0;
			switchColumnDesc[7].m_bUnique = 0;
			m_descVector.push_back(switchColumnDesc[7]);

			switchColumnDesc[8].m_strName = "ReplaceCount";
			switchColumnDesc[8].m_strType = "int";
			switchColumnDesc[8].m_bNullOK = 1;
			switchColumnDesc[8].m_bIndexed = 0;
			switchColumnDesc[8].m_bKey = 0;
			switchColumnDesc[8].m_bUnique = 0;
			m_descVector.push_back(switchColumnDesc[8]);

			switchColumnDesc[9].m_strName = "ReplaceTime";
			switchColumnDesc[9].m_strType = "timestamp";
			switchColumnDesc[9].m_bNullOK = 1;
			switchColumnDesc[9].m_bIndexed = 0;
			switchColumnDesc[9].m_bKey = 0;
			switchColumnDesc[9].m_bUnique = 0;
			m_descVector.push_back(switchColumnDesc[9]);

			switchColumnDesc[10].m_strName = "PowerOnCount";
			switchColumnDesc[10].m_strType = "int";
			switchColumnDesc[10].m_bNullOK = 1;
			switchColumnDesc[10].m_bIndexed = 0;
			switchColumnDesc[10].m_bKey = 0;
			switchColumnDesc[10].m_bUnique = 0;
			m_descVector.push_back(switchColumnDesc[10]);

			switchColumnDesc[11].m_strName = "PowerOffCount";
			switchColumnDesc[11].m_strType = "int";
			switchColumnDesc[11].m_bNullOK = 1;
			switchColumnDesc[11].m_bIndexed = 0;
			switchColumnDesc[11].m_bKey = 0;
			switchColumnDesc[11].m_bUnique = 0;
			m_descVector.push_back(switchColumnDesc[11]);

			switchColumnDesc[12].m_strName = "PowerOnTime";
			switchColumnDesc[12].m_strType = "int";
			switchColumnDesc[12].m_bNullOK = 1;
			switchColumnDesc[12].m_bIndexed = 0;
			switchColumnDesc[12].m_bKey = 0;
			switchColumnDesc[12].m_bUnique = 0;
			m_descVector.push_back(switchColumnDesc[12]);

			switchColumnDesc[13].m_strName = "PowerOffTime";
			switchColumnDesc[13].m_strType = "int";
			switchColumnDesc[13].m_bNullOK = 1;
			switchColumnDesc[13].m_bIndexed = 0;
			switchColumnDesc[13].m_bKey = 0;
			switchColumnDesc[13].m_bUnique = 0;
			m_descVector.push_back(switchColumnDesc[13]);

			switchColumnDesc[14].m_strName = "AccidentOffCount";
			switchColumnDesc[14].m_strType = "int";
			switchColumnDesc[14].m_bNullOK = 1;
			switchColumnDesc[14].m_bIndexed = 0;
			switchColumnDesc[14].m_bKey = 0;
			switchColumnDesc[14].m_bUnique = 0;
			m_descVector.push_back(switchColumnDesc[14]);

			switchColumnDesc[15].m_strName = "ArtificialOffCount";
			switchColumnDesc[15].m_strType = "int";
			switchColumnDesc[15].m_bNullOK = 1;
			switchColumnDesc[15].m_bIndexed = 0;
			switchColumnDesc[15].m_bKey = 0;
			switchColumnDesc[15].m_bUnique = 0;
			m_descVector.push_back(switchColumnDesc[15]);

			switchColumnDesc[16].m_strName = "AccidentOffTime";
			switchColumnDesc[16].m_strType = "int";
			switchColumnDesc[16].m_bNullOK = 1;
			switchColumnDesc[16].m_bIndexed = 0;
			switchColumnDesc[16].m_bKey = 0;
			switchColumnDesc[16].m_bUnique = 0;
			m_descVector.push_back(switchColumnDesc[16]);

			switchColumnDesc[17].m_strName = "ArtificialOffTime";
			switchColumnDesc[17].m_strType = "int";
			switchColumnDesc[17].m_bNullOK = 1;
			switchColumnDesc[17].m_bIndexed = 0;
			switchColumnDesc[17].m_bKey = 0;
			switchColumnDesc[17].m_bUnique = 0;
			m_descVector.push_back(switchColumnDesc[17]);

			switchColumnDesc[18].m_strName = "ProtectionsCount";
			switchColumnDesc[18].m_strType = "int";
			switchColumnDesc[18].m_bNullOK = 1;
			switchColumnDesc[18].m_bIndexed = 0;
			switchColumnDesc[18].m_bKey = 0;
			switchColumnDesc[18].m_bUnique = 0;
			m_descVector.push_back(switchColumnDesc[18]);
		}else if(index==1){//创建母线统计表sql
			ORMColumnDesc busbarColumnDesc[5];

			busbarColumnDesc[0].m_strName = "ObjectId";
			busbarColumnDesc[0].m_strType = "bigint(19)";
			busbarColumnDesc[0].m_bNullOK = 0;
			busbarColumnDesc[0].m_bIndexed = 1;
			busbarColumnDesc[0].m_bKey = 0;
			busbarColumnDesc[0].m_bUnique = 0;
			m_descVector.push_back(busbarColumnDesc[0]);

			busbarColumnDesc[1].m_strName = "SDate";
			busbarColumnDesc[1].m_strType = "timestamp";
			busbarColumnDesc[1].m_bNullOK = 0;
			busbarColumnDesc[1].m_bIndexed = 1;
			busbarColumnDesc[1].m_bKey = 0;
			busbarColumnDesc[1].m_bUnique = 0;
			m_descVector.push_back(busbarColumnDesc[1]);

			busbarColumnDesc[2].m_strName = "OType";
			busbarColumnDesc[2].m_strType = "int";
			busbarColumnDesc[2].m_bNullOK = 1;
			busbarColumnDesc[2].m_bIndexed = 0;
			busbarColumnDesc[2].m_bKey = 0;
			busbarColumnDesc[2].m_bUnique = 0;
			m_descVector.push_back(busbarColumnDesc[2]);

			busbarColumnDesc[3].m_strName = "ThreeVUnbalanceRate";
			busbarColumnDesc[3].m_strType = "double";
			busbarColumnDesc[3].m_bNullOK = 1;
			busbarColumnDesc[3].m_bIndexed = 0;
			busbarColumnDesc[3].m_bKey = 0;
			busbarColumnDesc[3].m_bUnique = 0;
			m_descVector.push_back(busbarColumnDesc[3]);

			busbarColumnDesc[4].m_strName = "LimitCount";
			busbarColumnDesc[4].m_strType = "int";
			busbarColumnDesc[4].m_bNullOK = 1;
			busbarColumnDesc[4].m_bIndexed = 0;
			busbarColumnDesc[4].m_bKey = 0;
			busbarColumnDesc[4].m_bUnique = 0;
			m_descVector.push_back(busbarColumnDesc[4]);
		}else if(index==2){//创建变压器统计表sql
			ORMColumnDesc transformColumnDesc[6];

			transformColumnDesc[0].m_strName = "ObjectId";
			transformColumnDesc[0].m_strType = "bigint(19)";
			transformColumnDesc[0].m_bNullOK = 0;
			transformColumnDesc[0].m_bIndexed = 1;
			transformColumnDesc[0].m_bKey = 0;
			transformColumnDesc[0].m_bUnique = 0;
			m_descVector.push_back(transformColumnDesc[0]);

			transformColumnDesc[1].m_strName = "SDate";
			transformColumnDesc[1].m_strType = "timestamp";
			transformColumnDesc[1].m_bNullOK = 0;
			transformColumnDesc[1].m_bIndexed = 1;
			transformColumnDesc[1].m_bKey = 0;
			transformColumnDesc[1].m_bUnique = 0;
			m_descVector.push_back(transformColumnDesc[1]);

			transformColumnDesc[2].m_strName = "OType";
			transformColumnDesc[2].m_strType = "int";
			transformColumnDesc[2].m_bNullOK = 1;
			transformColumnDesc[2].m_bIndexed = 0;
			transformColumnDesc[2].m_bKey = 0;
			transformColumnDesc[2].m_bUnique = 0;
			m_descVector.push_back(transformColumnDesc[2]);

			transformColumnDesc[3].m_strName = "ShiftCount";
			transformColumnDesc[3].m_strType = "int";
			transformColumnDesc[3].m_bNullOK = 1;
			transformColumnDesc[3].m_bIndexed = 0;
			transformColumnDesc[3].m_bKey = 0;
			transformColumnDesc[3].m_bUnique = 0;
			m_descVector.push_back(transformColumnDesc[3]);

			transformColumnDesc[4].m_strName = "LoadRate";
			transformColumnDesc[4].m_strType = "double";
			transformColumnDesc[4].m_bNullOK = 1;
			transformColumnDesc[4].m_bIndexed = 0;
			transformColumnDesc[4].m_bKey = 0;
			transformColumnDesc[4].m_bUnique = 0;
			m_descVector.push_back(transformColumnDesc[4]);

			transformColumnDesc[5].m_strName = "ProtectionsCount";
			transformColumnDesc[5].m_strType = "int";
			transformColumnDesc[5].m_bNullOK = 1;
			transformColumnDesc[5].m_bIndexed = 0;
			transformColumnDesc[5].m_bKey = 0;
			transformColumnDesc[5].m_bUnique = 0;
			m_descVector.push_back(transformColumnDesc[5]);
		}else if(index==3){//创建线路统计表sql
			ORMColumnDesc lineColumnDesc[21];

			lineColumnDesc[0].m_strName = "ObjectId";
			lineColumnDesc[0].m_strType = "bigint(19)";
			lineColumnDesc[0].m_bNullOK = 0;
			lineColumnDesc[0].m_bIndexed = 1;
			lineColumnDesc[0].m_bKey = 0;
			lineColumnDesc[0].m_bUnique = 0;
			m_descVector.push_back(lineColumnDesc[0]);

			lineColumnDesc[1].m_strName = "SDate";
			lineColumnDesc[1].m_strType = "timestamp";
			lineColumnDesc[1].m_bNullOK = 0;
			lineColumnDesc[1].m_bIndexed = 1;
			lineColumnDesc[1].m_bKey = 0;
			lineColumnDesc[1].m_bUnique = 0;
			m_descVector.push_back(lineColumnDesc[1]);

			lineColumnDesc[2].m_strName = "OType";
			lineColumnDesc[2].m_strType = "int";
			lineColumnDesc[2].m_bNullOK = 1;
			lineColumnDesc[2].m_bIndexed = 0;
			lineColumnDesc[2].m_bKey = 0;
			lineColumnDesc[2].m_bUnique = 0;
			m_descVector.push_back(lineColumnDesc[2]);

			lineColumnDesc[3].m_strName = "ThreeVUnbalanceRate";
			lineColumnDesc[3].m_strType = "double";
			lineColumnDesc[3].m_bNullOK = 1;
			lineColumnDesc[3].m_bIndexed = 0;
			lineColumnDesc[3].m_bKey = 0;
			lineColumnDesc[3].m_bUnique = 0;
			m_descVector.push_back(lineColumnDesc[3]);

			lineColumnDesc[4].m_strName = "LoadRate";
			lineColumnDesc[4].m_strType = "double";
			lineColumnDesc[4].m_bNullOK = 1;
			lineColumnDesc[4].m_bIndexed = 0;
			lineColumnDesc[4].m_bKey = 0;
			lineColumnDesc[4].m_bUnique = 0;
			m_descVector.push_back(lineColumnDesc[4]);

			lineColumnDesc[5].m_strName = "MaxLoad";
			lineColumnDesc[5].m_strType = "double";
			lineColumnDesc[5].m_bNullOK = 1;
			lineColumnDesc[5].m_bIndexed = 0;
			lineColumnDesc[5].m_bKey = 0;
			lineColumnDesc[5].m_bUnique = 0;
			m_descVector.push_back(lineColumnDesc[5]);

			lineColumnDesc[6].m_strName = "MaxLoadTime";
			lineColumnDesc[6].m_strType = "timestamp";
			lineColumnDesc[6].m_bNullOK = 1;
			lineColumnDesc[6].m_bIndexed = 0;
			lineColumnDesc[6].m_bKey = 0;
			lineColumnDesc[6].m_bUnique = 0;
			m_descVector.push_back(lineColumnDesc[6]);

			lineColumnDesc[7].m_strName = "MinLoad";
			lineColumnDesc[7].m_strType = "double";
			lineColumnDesc[7].m_bNullOK = 1;
			lineColumnDesc[7].m_bIndexed = 0;
			lineColumnDesc[7].m_bKey = 0;
			lineColumnDesc[7].m_bUnique = 0;
			m_descVector.push_back(lineColumnDesc[7]);

			lineColumnDesc[8].m_strName = "MinLoadTime";
			lineColumnDesc[8].m_strType = "timestamp";
			lineColumnDesc[8].m_bNullOK = 1;
			lineColumnDesc[8].m_bIndexed = 0;
			lineColumnDesc[8].m_bKey = 0;
			lineColumnDesc[8].m_bUnique = 0;
			m_descVector.push_back(lineColumnDesc[8]);

			lineColumnDesc[9].m_strName = "AvgLoad";
			lineColumnDesc[9].m_strType = "double";
			lineColumnDesc[9].m_bNullOK = 1;
			lineColumnDesc[9].m_bIndexed = 0;
			lineColumnDesc[9].m_bKey = 0;
			lineColumnDesc[9].m_bUnique = 0;
			m_descVector.push_back(lineColumnDesc[9]);

			lineColumnDesc[10].m_strName = "PowerOnCount";
			lineColumnDesc[10].m_strType = "int";
			lineColumnDesc[10].m_bNullOK = 1;
			lineColumnDesc[10].m_bIndexed = 0;
			lineColumnDesc[10].m_bKey = 0;
			lineColumnDesc[10].m_bUnique = 0;
			m_descVector.push_back(lineColumnDesc[10]);

			lineColumnDesc[11].m_strName = "PowerOffCount";
			lineColumnDesc[11].m_strType = "int";
			lineColumnDesc[11].m_bNullOK = 1;
			lineColumnDesc[11].m_bIndexed = 0;
			lineColumnDesc[11].m_bKey = 0;
			lineColumnDesc[11].m_bUnique = 0;
			m_descVector.push_back(lineColumnDesc[11]);

			lineColumnDesc[12].m_strName = "PowerOnTime";
			lineColumnDesc[12].m_strType = "int";
			lineColumnDesc[12].m_bNullOK = 1;
			lineColumnDesc[12].m_bIndexed = 0;
			lineColumnDesc[12].m_bKey = 0;
			lineColumnDesc[12].m_bUnique = 0;
			m_descVector.push_back(lineColumnDesc[12]);

			lineColumnDesc[13].m_strName = "PowerOffTime";
			lineColumnDesc[13].m_strType = "int";
			lineColumnDesc[13].m_bNullOK = 1;
			lineColumnDesc[13].m_bIndexed = 0;
			lineColumnDesc[13].m_bKey = 0;
			lineColumnDesc[13].m_bUnique = 0;
			m_descVector.push_back(lineColumnDesc[13]);

			lineColumnDesc[14].m_strName = "AccidentOffCount";
			lineColumnDesc[14].m_strType = "int";
			lineColumnDesc[14].m_bNullOK = 1;
			lineColumnDesc[14].m_bIndexed = 0;
			lineColumnDesc[14].m_bKey = 0;
			lineColumnDesc[14].m_bUnique = 0;
			m_descVector.push_back(lineColumnDesc[14]);

			lineColumnDesc[15].m_strName = "ArtificialOffCount";
			lineColumnDesc[15].m_strType = "int";
			lineColumnDesc[15].m_bNullOK = 1;
			lineColumnDesc[15].m_bIndexed = 0;
			lineColumnDesc[15].m_bKey = 0;
			lineColumnDesc[15].m_bUnique = 0;
			m_descVector.push_back(lineColumnDesc[15]);

			lineColumnDesc[16].m_strName = "AccidentOffTime";
			lineColumnDesc[16].m_strType = "int";
			lineColumnDesc[16].m_bNullOK = 1;
			lineColumnDesc[16].m_bIndexed = 0;
			lineColumnDesc[16].m_bKey = 0;
			lineColumnDesc[16].m_bUnique = 0;
			m_descVector.push_back(lineColumnDesc[16]);

			lineColumnDesc[17].m_strName = "ArtificialOffTime";
			lineColumnDesc[17].m_strType = "int";
			lineColumnDesc[17].m_bNullOK = 1;
			lineColumnDesc[17].m_bIndexed = 0;
			lineColumnDesc[17].m_bKey = 0;
			lineColumnDesc[17].m_bUnique = 0;
			m_descVector.push_back(lineColumnDesc[17]);

			lineColumnDesc[18].m_strName = "FeederActionTimes";
			lineColumnDesc[18].m_strType = "int";
			lineColumnDesc[18].m_bNullOK = 1;
			lineColumnDesc[18].m_bIndexed = 0;
			lineColumnDesc[18].m_bKey = 0;
			lineColumnDesc[18].m_bUnique = 0;
			m_descVector.push_back(lineColumnDesc[18]);

			lineColumnDesc[19].m_strName = "FeederSuccessTimes";
			lineColumnDesc[19].m_strType = "int";
			lineColumnDesc[19].m_bNullOK = 1;
			lineColumnDesc[19].m_bIndexed = 0;
			lineColumnDesc[19].m_bKey = 0;
			lineColumnDesc[19].m_bUnique = 0;
			m_descVector.push_back(lineColumnDesc[19]);

			lineColumnDesc[20].m_strName = "FeederFaultTimes";
			lineColumnDesc[20].m_strType = "int";
			lineColumnDesc[20].m_bNullOK = 1;
			lineColumnDesc[20].m_bIndexed = 0;
			lineColumnDesc[20].m_bKey = 0;
			lineColumnDesc[20].m_bUnique = 0;
			m_descVector.push_back(lineColumnDesc[20]);
		}

		//如果是主机
		if (getFaultTolerance()->amITheCurrentMachine())
		{
			if(CPS_ORM_CreateTable(dbPoolHandle, tableName.toStdString(), m_descVector)==1)
			{
				qDebug()<<"CreateTable:"+tableName;

				// 增加主键
				ACE_Time_Value tv = ACE_OS::gettimeofday();
				timespec_t st = tv;
				char tmp[512];
				sprintf(tmp, "ind_%lld_%ld", st.tv_sec, st.tv_nsec);
				std::string indexName = tmp;

				char sql[1024];
				ACE_OS::memset(sql,'\0',1024);
				char* primaryKeySql = "alter table %s add constraint %s primary key(OBJECTID,SDATE)";
				ACE_OS::sprintf(sql, primaryKeySql ,tableName.toStdString().c_str(),indexName.c_str());
				if(-1==CPS_ORM_DirectExecuteSql(dbPoolHandle, sql, 0))
				{
					qDebug()<<"CreateTablePrimaryKey error:"+tableName;
				}
			} 
		}
	}

	return tableName;
}


//初始化开关-刀闸数据信息
void PSRStatisticsServer::initSwitchDataInfo()
{
	//配置更新后，不启动应用，更新内存
	switchsTemp = switchs;
	switchs.clear();

	//初始化断路器PMSBreaker数据信息
	initSwitchCommonInfo(OT_PMSBreaker, 1);

	//初始化刀闸Disconnector数据信息
	initSwitchCommonInfo(OT_Disconnector, 2);

	//初始化负荷开关LoadBreakSwitch数据信息
	initSwitchCommonInfo(OT_LoadBreakSwitch, 3);

	//初始化直流开断设备DCSwitch数据信息
	initSwitchCommonInfo(OT_DCSwitch, 4);

	//循环遍历所有开关-刀闸, 配置更新后，不启动应用，更新内存********************
	QMap<ObId, Switch>::iterator switchMapIterator;
	for (switchMapIterator=switchs.begin();switchMapIterator!=switchs.end();switchMapIterator++)
	{
		ObId switchObId = switchMapIterator->obId;
		//如果临时存储中，包含更新后的obid，则将原内存中的统计量还原，用于增量提交处理
		if (switchsTemp.contains(switchObId))
		{
			//如果如果此次增量提交前后都有两个端子
			if (switchMapIterator->relatePoints.contains("0_PMSTerminal")&&switchsTemp[switchObId].relatePoints.contains("0_PMSTerminal")
				&&switchMapIterator->relatePoints.contains("1_PMSTerminal")&&switchsTemp[switchObId].relatePoints.contains("1_PMSTerminal"))
			{
				switchMapIterator->dayPowerOffCount = switchsTemp[switchObId].dayPowerOffCount;
				switchMapIterator->dayPowerOffTime = switchsTemp[switchObId].dayPowerOffTime;
				switchMapIterator->dayPowerOnCount = switchsTemp[switchObId].dayPowerOnCount;
				switchMapIterator->dayPowerOnTime = switchsTemp[switchObId].dayPowerOnTime;

				switchMapIterator->dayAccidentOffCount = switchsTemp[switchObId].dayAccidentOffCount;
				switchMapIterator->dayAccidentOffTime = switchsTemp[switchObId].dayAccidentOffTime;
				switchMapIterator->dayArtificialOffCount = switchsTemp[switchObId].dayArtificialOffCount;
				switchMapIterator->dayArtificialOffTime = switchsTemp[switchObId].dayArtificialOffTime;

				switchMapIterator->hourPowerOffCount = switchsTemp[switchObId].hourPowerOffCount;
				switchMapIterator->hourPowerOffTime = switchsTemp[switchObId].hourPowerOffTime;
				switchMapIterator->hourPowerOnCount = switchsTemp[switchObId].hourPowerOnCount;
				switchMapIterator->hourPowerOnTime = switchsTemp[switchObId].hourPowerOnTime;

				switchMapIterator->hourAccidentOffCount = switchsTemp[switchObId].hourAccidentOffCount;
				switchMapIterator->hourAccidentOffTime = switchsTemp[switchObId].hourAccidentOffTime;
				switchMapIterator->hourArtificialOffCount = switchsTemp[switchObId].hourArtificialOffCount;
				switchMapIterator->hourArtificialOffTime = switchsTemp[switchObId].hourArtificialOffTime;

				switchMapIterator->lastPowerOnTime = switchsTemp[switchObId].lastPowerOnTime;

				switchMapIterator->lastAccidentOffTime = switchsTemp[switchObId].lastAccidentOffTime;
				switchMapIterator->isAccidentOff = switchsTemp[switchObId].isAccidentOff;
			}

			//如果此次增量提交都含有开关DPS点，则恢复提交前统计相关信息
			if (switchMapIterator->relatePoints.contains("CB_DPSPoint")&&switchsTemp[switchObId].relatePoints.contains("CB_DPSPoint"))
			{
				switchMapIterator->dayAccidentTripCount = switchsTemp[switchObId].dayAccidentTripCount;
				switchMapIterator->dayRunTime = switchsTemp[switchObId].dayRunTime;

				switchMapIterator->hourAccidentTripCount = switchsTemp[switchObId].hourAccidentTripCount;
				switchMapIterator->hourRunTime = switchsTemp[switchObId].hourRunTime;

				switchMapIterator->lastSwitchOnTime = switchsTemp[switchObId].lastSwitchOnTime;
			}

			//如果此次增量提交都含有DPC开关操作次数统计点，则恢复提交前统计相关信息
			if (switchMapIterator->relatePoints.contains("CB_DPCPoint"))
			{
				switchMapIterator->dayOptCount = switchsTemp[switchObId].dayOptCount;

				switchMapIterator->hourOptCount = switchsTemp[switchObId].hourOptCount;
			}
			
			switchMapIterator->dayProtectionsCount = switchsTemp[switchObId].dayProtectionsCount;
			switchMapIterator->hourProtectionsCount = switchsTemp[switchObId].hourProtectionsCount;

			switchMapIterator->dayReplaceCount = switchsTemp[switchObId].dayReplaceCount;
			switchMapIterator->hourReplaceCount = switchsTemp[switchObId].hourReplaceCount;
			
			switchMapIterator->replaceTime = switchsTemp[switchObId].replaceTime;
		}
	}

	//清除临时存储空间
	switchsTemp.clear();
}

//初始化开关刀闸-公共部分
void PSRStatisticsServer::initSwitchCommonInfo(OType oType, int type)
{
	int num = m_OMSDatabase->find(oType, NULL, 0);
	if (num>0)
	{
		// 获取所有开关刀闸节点的ObId
		ObId *obIds = new ObId[num];
		m_OMSDatabase->find(oType, NULL, 0, obIds, num);
		
		ContainerData childrenListData;
		for (int i=0;i<num;i++)
		{
			//判断当前一次设备点是否需要添加
			bool isNeededInsert = false;

			//定义开关刀闸统计点
			Switch curSwitch;

			//初始化统计的obid 与类型
			curSwitch.obId = obIds[i];
			curSwitch.otype = type;

			//读取p开关刀闸的子节点
			try
			{
				m_OMSDatabase->read(obIds[i], AT_ChildrenList, &childrenListData);
			}
			catch (Exception& e)
			{
				LOG4CPLUS_ERROR(psrStatisticsServerLog, "error: read error!!!"+QString::number(obIds[i]).toStdString());
				continue;
			}
			
			int sonNum = childrenListData.getNumberOfElements();
			if (sonNum>0)
			{
				LinkData measurementTypeLinkData;
				LinkData measLinkData;
				LinkData dpcPointLinkData;
				StringData keyNameData;
				IntegerData stateData;

				QString timeFormat = "yyyy-MM-dd-hh-mm-ss";

				int pmsTerminalNum = 0;
				
				const ObId *childrenObId = childrenListData.getObIds();
				for (int i=0;i<sonNum;i++)
				{
					OType sonOType;
					try
					{
						sonOType = m_OMSDatabase->extractOType(childrenObId[i]);
					}
					catch (Exception& e)
					{
						LOG4CPLUS_ERROR(psrStatisticsServerLog, "error: extractOType error!!!"+QString::number(childrenObId[i]).toStdString());
						continue;
					}

					//获取遥信点信息
					if (OT_Discrete==sonOType)
					{
						try
						{
							m_OMSDatabase->read(childrenObId[i], AT_MeasurementTypeLink, &measurementTypeLinkData);
							m_OMSDatabase->read((ObId)measurementTypeLinkData, AT_KeyName, &keyNameData);
						}
						catch (Exception& e)
						{
							LOG4CPLUS_ERROR(psrStatisticsServerLog, "error: read error!!!"+QString::number((ObId)measurementTypeLinkData).toStdString());
						    continue;
						}
						
						if ((OMString)keyNameData=="CB"||(OMString)keyNameData=="DS")//如果是开关或者是刀闸
						{
							try
							{
								m_OMSDatabase->read(childrenObId[i], AT_MeasLink, &measLinkData);
							}
							catch (Exception& e)
							{
								LOG4CPLUS_ERROR(psrStatisticsServerLog, "error: read error!!!"+QString::number(childrenObId[i]).toStdString());
								continue;
							}
							
							//如果存在链接的DPS点，则存储
							if ((ObId)measLinkData!=0)
							{
								curSwitch.relatePoints.insert("CB_DPSPoint",(ObId)measLinkData);
								try
								{
									m_OMSDatabase->read((ObId)measLinkData, AT_State, &stateData);
								}
								catch (Exception& e)
								{
									LOG4CPLUS_ERROR(psrStatisticsServerLog, "error: read error!!!"+QString::number((ObId)measLinkData).toStdString());
								    continue;
								}

								curSwitch.lastSwitchOnTime = QDateTime::currentDateTime().toString(timeFormat);

								// 如果开关处于运行状态...
								if ((int)stateData==1)
								{
									//开关处于开启状态
									curSwitch.isSwitchOn = true;
								}
								//qDebug()<<(ObId)measLinkData;

								//获取遥控点obid
								try
								{
									m_OMSDatabase->read((ObId)measLinkData, AT_DPCPointLink, &dpcPointLinkData);
								}
								catch (Exception& e)
								{
									LOG4CPLUS_ERROR(psrStatisticsServerLog, "error: read error!!!"+QString::number((ObId)measLinkData).toStdString());
									continue;
								}

								//如果存储关联的遥控点，则存储
								if ((ObId)dpcPointLinkData!=0)
								{
									curSwitch.relatePoints.insert("CB_DPCPoint", (ObId)dpcPointLinkData);
								}
							}
						}
					}

					//获取遥测点信息,首先获取端子类型
					if (OT_PMSTerminal==sonOType)
					{
						//初始化开关刀闸带电状态以及端子信息*********************************************************
						ChoiceData rtEnergizedData;//是否带电
						try
						{
							m_OMSDatabase->read(childrenObId[i], AT_RTEnergized, &rtEnergizedData);

							//初始化上次送电时间
							curSwitch.lastPowerOnTime = QDateTime::currentDateTime().toString(timeFormat);

							//如果不带电
							if ((int)rtEnergizedData==0)
							{
								curSwitch.isPowerOn = false;
							}
							
							curSwitch.relatePoints.insert(QString("%1").arg(pmsTerminalNum)+"_PMSTerminal",childrenObId[i]);
							pmsTerminalNum++;//计算该开关刀闸下面有几个pmsTerminal
						}
						catch (Exception& e)
						{
							LOG4CPLUS_ERROR(psrStatisticsServerLog, "error: read error!!!"+QString::number(childrenObId[i]).toStdString());
							continue;
						}


						//初始化开关刀闸其他信息*********************************************************
						ContainerData childrenListAnalogData;
						try
						{
							m_OMSDatabase->read(childrenObId[i], AT_ChildrenList, &childrenListAnalogData);
						}
						catch (Exception& e)
						{
							LOG4CPLUS_ERROR(psrStatisticsServerLog, "error: read error!!!"+QString::number(childrenObId[i]).toStdString());
							continue;
						}
						
						int sonNum = childrenListAnalogData.getNumberOfElements();
						if (sonNum>0)
						{
							const ObId *childrenObId = childrenListAnalogData.getObIds();
							for (int i=0;i<sonNum;i++)
							{
								OType sonOType;
								try
								{
									sonOType = m_OMSDatabase->extractOType(childrenObId[i]);
								}
								catch (Exception& e)
								{
									LOG4CPLUS_ERROR(psrStatisticsServerLog, "error: extractOType error!!!"+QString::number(childrenObId[i]).toStdString());
									continue;
								}
								
								if (OT_Analog==sonOType)
								{
									try
									{
										m_OMSDatabase->read(childrenObId[i], AT_MeasurementTypeLink, &measurementTypeLinkData);
										m_OMSDatabase->read((ObId)measurementTypeLinkData, AT_KeyName, &keyNameData);
									}
									catch (Exception& e)
									{
										LOG4CPLUS_ERROR(psrStatisticsServerLog, "error: read error!!!"+QString::number((ObId)measurementTypeLinkData).toStdString());
										continue;
									}
									
									try
									{
										m_OMSDatabase->read(childrenObId[i], AT_MeasLink, &measLinkData);
									}
									catch (Exception& e)
									{
										LOG4CPLUS_ERROR(psrStatisticsServerLog, "error: read error!!!"+QString::number(childrenObId[i]).toStdString());
										continue;
									}

									//如果存在链接的MVPoint点，则存储
									if ((ObId)measLinkData!=0)
									{
										if ((OMString)keyNameData=="P_A")
										{
											curSwitch.relatePoints.insert("P_A_MVPoint",(ObId)measLinkData);
										}else if ((OMString)keyNameData=="P_B")
										{
											curSwitch.relatePoints.insert("P_B_MVPoint",(ObId)measLinkData);
										}else if ((OMString)keyNameData=="P_C")
										{
											curSwitch.relatePoints.insert("P_C_MVPoint",(ObId)measLinkData);
										}else if ((OMString)keyNameData=="Q_A")
										{
											curSwitch.relatePoints.insert("Q_A_MVPoint",(ObId)measLinkData);
										}else if ((OMString)keyNameData=="Q_B")
										{
											curSwitch.relatePoints.insert("Q_B_MVPoint",(ObId)measLinkData);
										}else if ((OMString)keyNameData=="Q_C")
										{
											curSwitch.relatePoints.insert("Q_C_MVPoint",(ObId)measLinkData);
										}
									}
								}
							}
						}
					}
				}
			}

			//判断是否需要添加该一次设备点
			if (curSwitch.relatePoints.contains("0_PMSTerminal")&&curSwitch.relatePoints.contains("1_PMSTerminal"))//如果存在两个端子
			{
				isNeededInsert = true;
			}

			if (curSwitch.relatePoints.contains("CB_DPSPoint"))
			{
				isNeededInsert = true;
			}

			if(curSwitch.relatePoints.contains("P_A_MVPoint")&&curSwitch.relatePoints.contains("P_B_MVPoint")&&curSwitch.relatePoints.contains("P_C_MVPoint"))
			{
				isNeededInsert = true;
			}

			if(curSwitch.relatePoints.contains("Q_A_MVPoint")&&curSwitch.relatePoints.contains("Q_B_MVPoint")&&curSwitch.relatePoints.contains("Q_C_MVPoint"))
			{
				isNeededInsert = true;
			}

			//存储pmsbreaker统计点到map中
			if (isNeededInsert)
			{
				switchs.insert(obIds[i], curSwitch);
			}
		}
		//清理内存
		delete[] obIds;
	}
}

//初始化母线数据信息
void PSRStatisticsServer::initBusbarDataInfo()
{
	//配置更新后，不启动应用，更新内存
	busbarsTemp = busbars;
	busbars.clear();

	//初始化母线PMSBusbar数据信息
	int pmsBusbarNum = m_OMSDatabase->find(OT_PMSBusbar, NULL, 0);
	if (pmsBusbarNum>0)
	{
		// 获取所有母线PMSBusbar节点的ObId
		ObId *pmsBusbarObIds = new ObId[pmsBusbarNum];
		m_OMSDatabase->find(OT_PMSBusbar, NULL, 0, pmsBusbarObIds, pmsBusbarNum);

		ContainerData childrenListData;
		for (int i=0;i<pmsBusbarNum;i++)
		{
			//判断当前一次设备点是否需要添加
			bool isNeededInsert = false;

			//定义母线统计点
			Busbar busbar;

			//初始化统计的obid 与类型
			busbar.obId = pmsBusbarObIds[i];
			busbar.otype = 1;//"PMSBusbar"

			//读取母线的子节点
			try
			{
				m_OMSDatabase->read(pmsBusbarObIds[i], AT_ChildrenList, &childrenListData);
			}
			catch (Exception& e)
			{
				LOG4CPLUS_ERROR(psrStatisticsServerLog, "error: read error!!!"+QString::number(pmsBusbarObIds[i]).toStdString());
				continue;
			}
			
			int sonNum = childrenListData.getNumberOfElements();
			if (sonNum>0) 
			{
				LinkData measurementTypeLinkData;
				LinkData measLinkData;
				StringData keyNameData;

				const ObId *childrenObId = childrenListData.getObIds();
				
				for (int i=0;i<sonNum;i++)
				{
					OType sonOType;
					try
					{
						sonOType = m_OMSDatabase->extractOType(childrenObId[i]);
					}
					catch (Exception& e)
					{
						LOG4CPLUS_ERROR(psrStatisticsServerLog, "error: extractOType error!!!"+QString::number(childrenObId[i]).toStdString());
						continue;
					}

					//获取遥测点信息,首先获取端子类型
					if (OT_PMSTerminal==sonOType)
					{
						ContainerData childrenListAnalogData;
						try
						{
							m_OMSDatabase->read(childrenObId[i], AT_ChildrenList, &childrenListAnalogData);
						}
						catch (Exception& e)
						{
							LOG4CPLUS_ERROR(psrStatisticsServerLog, "error: read error!!!"+QString::number(childrenObId[i]).toStdString());
							continue;
						}
						
						int sonNum = childrenListAnalogData.getNumberOfElements();
						if (sonNum>0)
						{
							const ObId *childrenObId = childrenListAnalogData.getObIds();
							for (int i=0;i<sonNum;i++)
							{
								OType sonOType;
								try
								{
									sonOType = m_OMSDatabase->extractOType(childrenObId[i]);
								}
								catch (Exception& e)
								{
									LOG4CPLUS_ERROR(psrStatisticsServerLog, "error: extractOType error!!!"+QString::number(childrenObId[i]).toStdString());
									continue;
								}
								
								if (OT_Analog==sonOType)
								{
									try
									{
										m_OMSDatabase->read(childrenObId[i], AT_MeasurementTypeLink, &measurementTypeLinkData);
										m_OMSDatabase->read((ObId)measurementTypeLinkData, AT_KeyName, &keyNameData);
									}
									catch (Exception& e)
									{
										LOG4CPLUS_ERROR(psrStatisticsServerLog, "error: read error!!!"+QString::number((ObId)measurementTypeLinkData).toStdString());
										continue;
									}

									try
									{
										m_OMSDatabase->read(childrenObId[i], AT_MeasLink, &measLinkData);
									}
									catch (Exception& e)
									{
										LOG4CPLUS_ERROR(psrStatisticsServerLog, "error: read error!!!"+QString::number(childrenObId[i]).toStdString());
										continue;
									}

									//如果存在链接的MVPoint点，则存储
									if ((ObId)measLinkData!=0)
									{
										//判断为添加该总线
										isNeededInsert = true;

										if ((OMString)keyNameData=="V_A")
										{
											busbar.relatePoints.insert("V_A_MVPoint",(ObId)measLinkData);
										}else if ((OMString)keyNameData=="V_B")
										{
											busbar.relatePoints.insert("V_B_MVPoint",(ObId)measLinkData);
										}else if ((OMString)keyNameData=="V_C")
										{
											busbar.relatePoints.insert("V_C_MVPoint",(ObId)measLinkData);
										}
									}
								}
							}
						}
					}
				}
			}

			//存储busbar统计点到map中
			if (isNeededInsert)
			{
				busbars.insert(pmsBusbarObIds[i], busbar);
			}
		}

		//清理内存
		delete[] pmsBusbarObIds;
	}

	//循环遍历所有母线, 配置更新后，不启动应用，更新内存
	QMap<ObId, Busbar>::iterator busbarMapIterator;
	for (busbarMapIterator=busbars.begin();busbarMapIterator!=busbars.end();busbarMapIterator++)
	{
		ObId busbarObId = busbarMapIterator->obId;
		//如果临时存储中，包含更新后的obid，则将原内存中的统计量还原
		if (busbarsTemp.contains(busbarObId))
		{
			busbarMapIterator->dayLimitCount = busbarsTemp[busbarObId].dayLimitCount;
			busbarMapIterator->hourLimitCount = busbarsTemp[busbarObId].hourLimitCount;
		}
	}

	//清除临时存储空间
	busbarsTemp.clear();
}

//初始化变压器数据信息
void PSRStatisticsServer::initTransformerDataInfo()
{
	//配置更新后，不启动应用，更新内存
	transformersTemp = transformers;
	transformers.clear();

	//初始化三卷变压器PMSThreeWindingTransformer数据信息
	initTransformerCommonInfo(OT_PMSThreeWindingTransformer, 1);

	//初始化两卷变压器PMSDoubleWindingTransformer数据信息
	initTransformerCommonInfo(OT_PMSDoubleWindingTransformer, 2);

	//初始化配变DistributionTransformer数据信息
	initTransformerCommonInfo(OT_DistributionTransformer, 3);

	//循环遍历所有变压器, 配置更新后，不启动应用，更新内存
	QMap<ObId, Transformer>::iterator transformerMapIterator;
	for (transformerMapIterator=transformers.begin();transformerMapIterator!=transformers.end();transformerMapIterator++)
	{
		ObId transformerObId = transformerMapIterator->obId;
		//如果临时存储中，包含更新后的obid，则将原内存中的统计量还原
		if (transformersTemp.contains(transformerObId))
		{
			//如果此次增量提交都含有调档操作次数统计点，则恢复提交前统计相关信息
			if (transformerMapIterator->relatePoints.contains("TapPos_MVPoint"))
			{
				transformerMapIterator->dayShiftCount = transformersTemp[transformerObId].dayShiftCount;
				transformerMapIterator->hourShiftCount = transformersTemp[transformerObId].hourShiftCount;
			}

			transformerMapIterator->dayProtectionsCount = transformersTemp[transformerObId].dayProtectionsCount;
			transformerMapIterator->hourProtectionsCount = transformersTemp[transformerObId].hourProtectionsCount;
		}
	}

	//清除临时存储空间
	transformersTemp.clear();
}

//初始化变压器-公共部分
void PSRStatisticsServer::initTransformerCommonInfo(OType oType, int type)
{
	//初始化变压器数据信息
	int num = m_OMSDatabase->find(oType, NULL, 0);
	if (num>0)
	{
		// 获取所有变压器节点的ObId
		ObId *obIds = new ObId[num];
		m_OMSDatabase->find(oType, NULL, 0, obIds, num);

		ContainerData childrenListData;
		for (int i=0;i<num;i++)
		{
			//判断当前一次设备点是否需要添加
			bool isNeededInsert = false;

			//定义变压器统计点
			Transformer transformer;

			//初始化统计的obid 与类型
			transformer.obId = obIds[i];
			transformer.otype = type;

			//读取变压器的子节点
			try
			{
				m_OMSDatabase->read(obIds[i], AT_ChildrenList, &childrenListData);
			}
			catch (Exception& e)
			{
				LOG4CPLUS_ERROR(psrStatisticsServerLog, "error: read error!!!"+QString::number(obIds[i]).toStdString());
				continue;
			}
			
			int sonNum = childrenListData.getNumberOfElements();
			if (sonNum>0) 
			{
				LinkData measurementTypeLinkData;
				LinkData measLinkData;
				StringData keyNameData;

				const ObId *childrenObId = childrenListData.getObIds();

				for (int i=0;i<sonNum;i++)
				{
					OType sonOType;
					try
					{
						sonOType = m_OMSDatabase->extractOType(childrenObId[i]);
					}
					catch (Exception& e)
					{
						LOG4CPLUS_ERROR(psrStatisticsServerLog, "error: extractOType error!!!"+QString::number(childrenObId[i]).toStdString());
						continue;
					}

					//获取遥测点信息,首先获取变压器卷类型
					if (OT_TransformerWinding==sonOType)
					{
						ContainerData childrenListPMSTerminalData;
						try
						{
							m_OMSDatabase->read(childrenObId[i], AT_ChildrenList, &childrenListPMSTerminalData);
						}
						catch (Exception& e)
						{
							LOG4CPLUS_ERROR(psrStatisticsServerLog, "error: read error!!!"+QString::number(childrenObId[i]).toStdString());
							continue;
						}

						int sonNum = childrenListPMSTerminalData.getNumberOfElements();
						if (sonNum>0)
						{
							const ObId *childrenObId = childrenListPMSTerminalData.getObIds();
							for (int i=0;i<sonNum;i++)
							{
								OType sonOType;
								try
								{
									sonOType = m_OMSDatabase->extractOType(childrenObId[i]);
								}
								catch (Exception& e)
								{
									LOG4CPLUS_ERROR(psrStatisticsServerLog, "error: read error!!!"+QString::number(childrenObId[i]).toStdString());
									continue;
								}

								//获取遥测点信息,首先获取端子类型
								if (OT_PMSTerminal==sonOType)
								{
									ContainerData childrenListAnalogData;
									try
									{
										m_OMSDatabase->read(childrenObId[i], AT_ChildrenList, &childrenListAnalogData);
									}
									catch (Exception& e)
									{
										LOG4CPLUS_ERROR(psrStatisticsServerLog, "error: read error!!!"+QString::number(childrenObId[i]).toStdString());
										continue;
									}

									int sonNum = childrenListAnalogData.getNumberOfElements();
									if (sonNum>0)
									{
										const ObId *childrenObId = childrenListAnalogData.getObIds();
										for (int i=0;i<sonNum;i++)
										{
											OType sonOType;
											try
											{
												sonOType = m_OMSDatabase->extractOType(childrenObId[i]);
											}
											catch (Exception& e)
											{
												LOG4CPLUS_ERROR(psrStatisticsServerLog, "error: read error!!!"+QString::number(childrenObId[i]).toStdString());
												continue;
											}

											if (OT_Analog==sonOType)
											{
												try
												{
													m_OMSDatabase->read(childrenObId[i], AT_MeasurementTypeLink, &measurementTypeLinkData);
													m_OMSDatabase->read((ObId)measurementTypeLinkData, AT_KeyName, &keyNameData);
												}
												catch (Exception& e)
												{
													LOG4CPLUS_ERROR(psrStatisticsServerLog, "error: read error!!!"+QString::number((ObId)measurementTypeLinkData).toStdString());
													continue;
												}

												try
												{
													m_OMSDatabase->read(childrenObId[i], AT_MeasLink, &measLinkData);
												}
												catch (Exception& e)
												{
													LOG4CPLUS_ERROR(psrStatisticsServerLog, "error: read error!!!"+QString::number(childrenObId[i]).toStdString());
													continue;
												}

												//如果存在链接的MVPoint点，则存储
												if ((ObId)measLinkData!=0)
												{
													if ((OMString)keyNameData=="TapPos")
													{
														transformer.relatePoints.insert("TapPos_MVPoint",(ObId)measLinkData);
													}else if ((OMString)keyNameData=="P")
													{
														transformer.relatePoints.insert("P_MVPoint",(ObId)measLinkData);
													}

												}

											}
										}
									}
								}
							}
						}
					}
				}
			}

			// 如果需要添加该变压器,负载率暂时不做，故不做必须包含判断
			if (transformer.relatePoints.contains("TapPos_MVPoint"))
			{
				isNeededInsert = true;
			}
			//存储变压器统计点到map中
			if (isNeededInsert)
			{
				transformers.insert(obIds[i], transformer);
			}
		}

		//清理内存
		delete[] obIds;
	}
}

//初始化线路数据信息
void PSRStatisticsServer::initLineDataInfo()
{
	//配置更新后，不启动应用，更新内存
	linesTemp = lines;
	lines.clear();

	//初始化线路Feeder数据信息
	LinkData measurementTypeLinkData;
	LinkData measLinkData;
	StringData keyNameData;
	IntegerData breakerTypeData;
	FloatData valuesData;

	int feederNum = m_OMSDatabase->find(OT_Feeder, NULL, 0);
	if (feederNum>0)
	{
		// 获取所有线路Feeder节点的ObId
		ObId *feederObIds = new ObId[feederNum];
		m_OMSDatabase->find(OT_Feeder, NULL, 0, feederObIds, feederNum);

		ContainerData childrenListData;
		for (int i=0;i<feederNum;i++)
		{
			//判断当前一次设备点是否需要添加
			bool isNeededInsert = false;

			//定义线路节点
			Line line;

			//初始化统计的obid 与类型
			line.obId = feederObIds[i];
			line.otype = 1;//"Feeder"

			//读取线路的子节点
			try
			{
				m_OMSDatabase->read(feederObIds[i], AT_ChildrenList, &childrenListData);
			}
			catch (Exception& e)
			{
				LOG4CPLUS_ERROR(psrStatisticsServerLog, "error: read error!!!"+QString::number(feederObIds[i]).toStdString());
				continue;
			}
			
			int sonNum = childrenListData.getNumberOfElements();
			if (sonNum>0) 
			{
				const ObId *childrenObId = childrenListData.getObIds();

				for (int j=0;j<sonNum;j++)
				{
					OType sonOType;
					try
					{
						sonOType = m_OMSDatabase->extractOType(childrenObId[j]);
					}
					catch (Exception& e)
					{
						LOG4CPLUS_ERROR(psrStatisticsServerLog, "error: extractOType error!!!"+QString::number(childrenObId[j]).toStdString());
						continue;
					}

					//获取遥测点信息,首先获取PMSBreaker出线开关类型
					if (OT_PMSBreaker==sonOType)
					{
						try
						{
							m_OMSDatabase->read(childrenObId[j], AT_BreakerType, &breakerTypeData);
						}
						catch (Exception& e)
						{
							LOG4CPLUS_ERROR(psrStatisticsServerLog, "error: read error!!!"+QString::number(childrenObId[j]).toStdString());
							continue;
						}
						
						//如果是出线开关则存储
						if ((int)breakerTypeData==6)
						{
							ContainerData childrenListPMSPerminalData;
							try
							{
								m_OMSDatabase->read(childrenObId[j], AT_ChildrenList, &childrenListPMSPerminalData);
							}
							catch (Exception& e)
							{
								LOG4CPLUS_ERROR(psrStatisticsServerLog, "error: read error!!!"+QString::number(childrenObId[j]).toStdString());
								continue;
							}
							
							int sonNum = childrenListPMSPerminalData.getNumberOfElements();
							if (sonNum>0)
							{
								const ObId *childrenObId = childrenListPMSPerminalData.getObIds();

								int pmsTerminalNum = 0;

								for (int k=0;k<sonNum;k++)
								{
									OType sonOType;
									try
									{
										sonOType = m_OMSDatabase->extractOType(childrenObId[k]);
									}
									catch (Exception& e)
									{
										LOG4CPLUS_ERROR(psrStatisticsServerLog, "error: extractOType error!!!"+QString::number(childrenObId[k]).toStdString());
										continue;
									}

									//获取遥测点信息,首先获取端子类型
									if (OT_PMSTerminal==sonOType)
									{
										//初始化开关刀闸带电状态以及端子信息*********************************************************
										ChoiceData rtEnergizedData;//是否带电
										try
										{
											m_OMSDatabase->read(childrenObId[k], AT_RTEnergized, &rtEnergizedData);

											//初始化上次送电时间
											QString timeFormat = "yyyy-MM-dd-hh-mm-ss";
											line.lastPowerOnTime = QDateTime::currentDateTime().toString(timeFormat);

											//如果不带电
											if ((int)rtEnergizedData==0)
											{
												line.isPowerOn = false;
											}

											line.relatePoints.insert(QString("%1").arg(pmsTerminalNum)+"_PMSTerminal",childrenObId[k]);
											pmsTerminalNum++;//计算该开关刀闸下面有几个pmsTerminal
										}
										catch (Exception& e)
										{
											LOG4CPLUS_ERROR(psrStatisticsServerLog, "error: read error!!!"+QString::number(childrenObId[k]).toStdString());
											continue;
										}

										//初始化线路其他信息**************************************
										ContainerData childrenListAnalogData;
										try
										{
											m_OMSDatabase->read(childrenObId[k], AT_ChildrenList, &childrenListAnalogData);
										}
										catch (Exception& e)
										{
											LOG4CPLUS_ERROR(psrStatisticsServerLog, "error: read error!!!"+QString::number(childrenObId[k]).toStdString());
											continue;
										}
										
										int sonNum = childrenListAnalogData.getNumberOfElements();
										if (sonNum>0)
										{
											const ObId *childrenObId = childrenListAnalogData.getObIds();
											for (int m=0;m<sonNum;m++)
											{
												OType sonOType;
												try
												{
													sonOType = m_OMSDatabase->extractOType(childrenObId[m]);
												}
												catch (Exception& e)
												{
													LOG4CPLUS_ERROR(psrStatisticsServerLog, "error: extractOType error!!!"+QString::number(childrenObId[m]).toStdString());
													continue;
												}
												
												if (OT_Analog==sonOType)
												{
													try
													{
														m_OMSDatabase->read(childrenObId[m], AT_MeasurementTypeLink, &measurementTypeLinkData);
														m_OMSDatabase->read((ObId)measurementTypeLinkData, AT_KeyName, &keyNameData);
													}
													catch (Exception& e)
													{
														LOG4CPLUS_ERROR(psrStatisticsServerLog, "error: read error!!!"+QString::number((ObId)measurementTypeLinkData).toStdString());
														continue;
													}

													try
													{
														m_OMSDatabase->read(childrenObId[m], AT_MeasLink, &measLinkData);
													}
													catch (Exception& e)
													{
														LOG4CPLUS_ERROR(psrStatisticsServerLog, "error: read error!!!"+QString::number(childrenObId[m]).toStdString());
														continue;
													}

													//如果存在链接的MVPoint点，则存储
													if ((ObId)measLinkData!=0)
													{
														if ((OMString)keyNameData=="P")
														{
															QString timeFormat = "yyyy-MM-dd-hh-mm-ss";
															QDateTime currentTime = QDateTime::currentDateTime();

															line.relatePoints.insert("P_MVPoint",(ObId)measLinkData);

															//初始化负荷最大值、最小值、平均值及其时间以及最近一次的负荷值
															m_OMSDatabase->read((ObId)measLinkData, AT_Value, &valuesData);
															line.latestLoad = (float)valuesData;
															line.dayAvgLoad = (float)valuesData;
															line.dayLoadChangeCount = 1;
															line.dayMaxLoad = (float)valuesData;
															line.dayMaxLoadTime = currentTime.toString(timeFormat);
															line.dayMinLoad = (float)valuesData;
															line.dayMinLoadTime = currentTime.toString(timeFormat);
															line.daySumLoad = (float)valuesData;
															line.hourAvgLoad = (float)valuesData;
															line.hourLoadChangeCount = 1;
															line.hourMaxLoad = (float)valuesData;
															line.hourMaxLoadTime = currentTime.toString(timeFormat);
															line.hourMinLoad = (float)valuesData;
															line.hourMinLoadTime = currentTime.toString(timeFormat);
															line.hourSumLoad = (float)valuesData;
														}else if ((OMString)keyNameData=="V_A")
														{
															line.relatePoints.insert("V_A_MVPoint",(ObId)measLinkData);
														}else if ((OMString)keyNameData=="V_B")
														{
															line.relatePoints.insert("V_B_MVPoint",(ObId)measLinkData);
														}else if ((OMString)keyNameData=="V_C")
														{
															line.relatePoints.insert("V_C_MVPoint",(ObId)measLinkData);
														}
													}
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}

			//如果需要插入
			if (line.relatePoints.contains("0_PMSTerminal")&&line.relatePoints.contains("1_PMSTerminal"))//如果存在两个端子
			{
				isNeededInsert = true;
			}

			if (line.relatePoints.contains("P_MVPoint"))
			{
				isNeededInsert = true;
			}else if (line.relatePoints.contains("V_A_MVPoint")&&line.relatePoints.contains("V_B_MVPoint")&&line.relatePoints.contains("V_C_MVPoint"))
			{
				isNeededInsert = true;
			}

			//存储线路统计点到map中
			if (isNeededInsert)
			{
				lines.insert(feederObIds[i], line);
			}
		}

		//清理内存
		delete[] feederObIds;
	}

	//循环遍历所有线路, 配置更新后，不启动应用，更新内存
	QMap<ObId, Line>::iterator lineMapIterator;
	for (lineMapIterator=lines.begin();lineMapIterator!=lines.end();lineMapIterator++)
	{
		ObId lineObId = lineMapIterator->obId;
		//如果临时存储中，包含更新后的obid，则将原内存中的统计量还原
		if (linesTemp.contains(lineObId))
		{
			//如果如果此次增量提交前后都有两个端子
			if (lineMapIterator->relatePoints.contains("0_PMSTerminal")&&linesTemp[lineObId].relatePoints.contains("0_PMSTerminal")
				&&lineMapIterator->relatePoints.contains("1_PMSTerminal")&&linesTemp[lineObId].relatePoints.contains("1_PMSTerminal"))
			{
				lineMapIterator->dayPowerOffCount = linesTemp[lineObId].dayPowerOffCount;
				lineMapIterator->dayPowerOffTime = linesTemp[lineObId].dayPowerOffTime;
				lineMapIterator->dayPowerOnCount = linesTemp[lineObId].dayPowerOnCount;
				lineMapIterator->dayPowerOnTime = linesTemp[lineObId].dayPowerOnTime;

				lineMapIterator->dayAccidentOffCount = linesTemp[lineObId].dayAccidentOffCount;
				lineMapIterator->dayAccidentOffTime = linesTemp[lineObId].dayAccidentOffTime;
				lineMapIterator->dayArtificialOffCount = linesTemp[lineObId].dayArtificialOffCount;
				lineMapIterator->dayArtificialOffTime = linesTemp[lineObId].dayArtificialOffTime;

				lineMapIterator->dayFeederActionTimes = linesTemp[lineObId].dayFeederActionTimes;
				lineMapIterator->dayFeederSuccessTimes = linesTemp[lineObId].dayFeederSuccessTimes;
				lineMapIterator->dayFeederFaultTimes = linesTemp[lineObId].dayFeederFaultTimes;

				lineMapIterator->hourPowerOffCount = linesTemp[lineObId].hourPowerOffCount;
				lineMapIterator->hourPowerOffTime = linesTemp[lineObId].hourPowerOffTime;
				lineMapIterator->hourPowerOnCount = linesTemp[lineObId].hourPowerOnCount;
				lineMapIterator->hourPowerOnTime = linesTemp[lineObId].hourPowerOnTime;

				lineMapIterator->hourAccidentOffCount = linesTemp[lineObId].hourAccidentOffCount;
				lineMapIterator->hourAccidentOffTime = linesTemp[lineObId].hourAccidentOffTime;
				lineMapIterator->hourArtificialOffCount = linesTemp[lineObId].hourArtificialOffCount;
				lineMapIterator->hourArtificialOffTime = linesTemp[lineObId].hourArtificialOffTime;

				lineMapIterator->hourFeederActionTimes = linesTemp[lineObId].hourFeederActionTimes;
				lineMapIterator->hourFeederSuccessTimes = linesTemp[lineObId].hourFeederSuccessTimes;
				lineMapIterator->hourFeederFaultTimes = linesTemp[lineObId].hourFeederFaultTimes;

				lineMapIterator->lastPowerOnTime = linesTemp[lineObId].lastPowerOnTime;

				lineMapIterator->lastAccidentOffTime = linesTemp[lineObId].lastAccidentOffTime;
				lineMapIterator->isAccidentOff = linesTemp[lineObId].isAccidentOff;
			}

			//如果此次增量提交都含有负荷统计点，则恢复提交前统计相关信息
			if (lineMapIterator->relatePoints.contains("P_MVPoint")&&linesTemp[lineObId].relatePoints.contains("P_MVPoint"))
			{
				lineMapIterator->dayAvgLoad = linesTemp[lineObId].dayAvgLoad;
				lineMapIterator->dayLoadChangeCount = linesTemp[lineObId].dayLoadChangeCount;
				lineMapIterator->dayMaxLoad = linesTemp[lineObId].dayMaxLoad;
				lineMapIterator->dayMaxLoadTime = linesTemp[lineObId].dayMaxLoadTime;
				lineMapIterator->dayMinLoad = linesTemp[lineObId].dayMinLoad;
				lineMapIterator->dayMinLoadTime = linesTemp[lineObId].dayMinLoadTime;
				lineMapIterator->daySumLoad = linesTemp[lineObId].daySumLoad;
				lineMapIterator->hourAvgLoad = linesTemp[lineObId].hourAvgLoad;
				lineMapIterator->hourLoadChangeCount = linesTemp[lineObId].hourLoadChangeCount;
				lineMapIterator->hourMaxLoad = linesTemp[lineObId].hourMaxLoad;
				lineMapIterator->hourMaxLoadTime = linesTemp[lineObId].hourMaxLoadTime;
				lineMapIterator->hourMinLoad = linesTemp[lineObId].hourMinLoad;
				lineMapIterator->hourMinLoadTime = linesTemp[lineObId].hourMinLoadTime;
				lineMapIterator->hourSumLoad = linesTemp[lineObId].hourSumLoad;
				lineMapIterator->latestLoad = linesTemp[lineObId].latestLoad;
			}
		}
	}

	//清除临时存储空间
	linesTemp.clear();
}

//获取最大值
float PSRStatisticsServer::getMax(float a, float b, float c)
{
	float max = a;
	if (b>max)
	{
		max = b;
	}
	if (c>max)
	{
		max = c;
	}

	return max;
}

//获取最小值
float PSRStatisticsServer::getMin(float a, float b, float c)
{
	float min = a;
	if (b<min)
	{
		min = b;
	}
	if (c<min)
	{
		min = c;
	}

	return min;
}