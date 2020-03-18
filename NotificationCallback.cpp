#include "NotificationCallback.h"

extern PSRStatisticsServer *app;//声明qt应用外部全局变量
extern Database *m_OMSDatabase;//实时数据库指针外部全局变量
extern Logger psrStatisticsServerLog;//log4c日志

//监听开关-刀闸操作次数
void notificationSwitchOptCount(const Notification* notif, void* clientdata)

{
	ObId dpcPointObId = notif->getDataItems()->getObId();
	LinkData dpsPointLinkData;
	try
	{
		m_OMSDatabase->read(dpcPointObId, AT_DPSPointLink, &dpsPointLinkData);
	}
	catch (Exception& e)
	{
		LOG4CPLUS_ERROR(psrStatisticsServerLog, "error: read error!!!"+QString::number(dpcPointObId).toStdString());
	}

	//如果关联的DPS遥信点存在
	if ((ObId)dpsPointLinkData!=0)
	{
		LinkData psrLinkData;
		try
		{
			m_OMSDatabase->read((ObId)dpsPointLinkData, AT_PSRLink, &psrLinkData);
		}
		catch (Exception& e)
		{
			LOG4CPLUS_ERROR(psrStatisticsServerLog, "error: read error!!!"+QString::number((ObId)dpsPointLinkData).toStdString());
		}

		//如果关联的一次设备Discrete点存在
		if ((ObId)psrLinkData!=0)
		{
			LinkData parentLinkData;
			try
			{
				m_OMSDatabase->read((ObId)psrLinkData, AT_ParentLink, &parentLinkData);
			}
			catch (Exception& e)
			{
				LOG4CPLUS_ERROR(psrStatisticsServerLog, "error: read error!!!"+QString::number((ObId)psrLinkData).toStdString());
			}

			//如果存在父节点
			if ((ObId)parentLinkData!=0)
			{
				//如果该DPCPoint点关联的一次设备是开关
				if (app->switchs.contains((ObId)parentLinkData))
				{
					//如果该DPC点是统计点
					if (app->switchs[(ObId)parentLinkData].relatePoints["CB_DPCPoint"]==dpcPointObId)
					{
						app->switchs[(ObId)parentLinkData].hourOptCount +=1;
						app->switchs[(ObId)parentLinkData].dayOptCount +=1;
					}
				}
			}
		}
	}
}
//监听开关-刀闸运行时长,  同时监听故障遥信保护动作次数（适用开关和变压器）
void notificationSwitchRuntime(const Notification* notif, void* clientdata)

{	
	ObId dpsPointObId = notif->getDataItems()->getObId();

	IntegerData stateCommonData = *(IntegerData*)(notif->getData());
	
	//获取开关触发时刻时间
	time_t t = notif->getSequenceTime().tv_sec;
	QDateTime currentTime = QDateTime::fromTime_t(t);

	//分别打印到达实时库的时间和到达本应用的时间，便于后期问题定位分析
	qDebug()<<currentTime;
	qDebug()<<QDateTime::currentDateTime();

	LinkData psrLinkData;
	try
	{
		m_OMSDatabase->read((ObId)dpsPointObId, AT_PSRLink, &psrLinkData);
	}
	catch (Exception& e)
	{
		LOG4CPLUS_ERROR(psrStatisticsServerLog, "error: read AT_PSRLink error!!!"+QString::number((ObId)dpsPointObId).toStdString());
	}

	//如果关联的一次设备Discrete点或者变压器设备存在
	if ((ObId)psrLinkData!=0)
	{
		bool isFaultType = false;

		if ((int)stateCommonData==1)//如果dps是动作才统计保护动作次数
		{
			LinkData measureTypeLinkData = 0;
			try
			{
				m_OMSDatabase->read((ObId)dpsPointObId, AT_MeasurementTypeLink, &measureTypeLinkData);
			}
			catch (Exception& e)
			{
				LOG4CPLUS_ERROR(psrStatisticsServerLog, "error: read AT_MeasurementTypeLink error!!!"+QString::number((ObId)dpsPointObId).toStdString());
			}

			if (app->faultMeasureTypeObIds.contains((ObId)measureTypeLinkData))//如果量测类型是故障、事故或FTU故障类型
			{
				isFaultType = true;

				if (app->transformers.contains((ObId)psrLinkData))//如果关联的一次设备是变压器
				{
					app->transformers[(ObId)psrLinkData].hourProtectionsCount +=1;
					app->transformers[(ObId)psrLinkData].dayProtectionsCount +=1;
				}
			}
		}

		LinkData parentLinkData;
		try
		{
			m_OMSDatabase->read((ObId)psrLinkData, AT_ParentLink, &parentLinkData);
		}
		catch (Exception& e)
		{
			LOG4CPLUS_ERROR(psrStatisticsServerLog, "error: read error!!!"+QString::number((ObId)psrLinkData).toStdString());
		}

		//如果存在父节点
		if ((ObId)parentLinkData!=0)
		{
			//如果该DPSPoint点关联的一次设备是开关
			if (app->switchs.contains((ObId)parentLinkData))
			{
				// ************如果该DPS点是保护动作统计点. 统计保护动作次数
				if (isFaultType)
				{
					app->switchs[(ObId)parentLinkData].hourProtectionsCount +=1;
					app->switchs[(ObId)parentLinkData].dayProtectionsCount +=1;
				}


				//****************如果该DPS点是开关统计点. 统计开关运行时长
				if (app->switchs[(ObId)parentLinkData].relatePoints["CB_DPSPoint"]==dpsPointObId)
				{
					//如果存在开关DPS点且是开关-位置类型的DPS点
					if (app->switchs[(ObId)parentLinkData].lastSwitchOnTime!="")
					{
						IntegerData stateData = *(IntegerData*)(notif->getData());

						//计算运行时间
						QString timeFormat = "yyyy-MM-dd-hh-mm-ss";

						Switch* psrSwitch = &(app->switchs[(ObId)parentLinkData]);
						//如果开关跳闸
						if ((int)stateData==0)
						{
							//将开关标记位，标记为断开
							psrSwitch->isSwitchOn = false;

							int lastToCurrent = QDateTime::fromString(psrSwitch->lastSwitchOnTime, timeFormat).secsTo(currentTime);//开关上次闭合到当前时间的秒数						

							QStringList currentTimeList = currentTime.toString(timeFormat).split("-");// 当前时间分割
							QString currentHourStartTime = currentTimeList[0]+"-"+currentTimeList[1]+"-"+currentTimeList[2]+"-"+currentTimeList[3]+"-00-00";//当前时间对应的整点时间
							int startHourToCurrent = QDateTime::fromString(currentHourStartTime, timeFormat).secsTo(currentTime);//当前整点时间到当前时间的秒数

							//如果上次闭合时间到当前时间差 大于 整点到当前时间差, 进行时累计运行统计
							if (lastToCurrent>startHourToCurrent)
							{
								psrSwitch->hourRunTime = startHourToCurrent;
							}else{
								psrSwitch->hourRunTime += lastToCurrent;
							}

							QString currentDayStartTime = currentTimeList[0]+"-"+currentTimeList[1]+"-"+currentTimeList[2]+"-00-00-00";//当前时间对应的当天起始时间
							int startDayToCurrent = QDateTime::fromString(currentDayStartTime, timeFormat).secsTo(currentTime);//当天起始时间到当前时间的秒数

							//如果上次闭合时间到当前时间差 大于 当天起始时间到当前时间差, 进行日累计运行统计
							if (lastToCurrent>startDayToCurrent)
							{
								psrSwitch->dayRunTime = startDayToCurrent;
							}else{
								psrSwitch->dayRunTime += lastToCurrent;
							}
						}else if ((int)stateData==1)//如果开关合闸
						{
							//将开关标记位，标记为闭合
							psrSwitch->isSwitchOn = true;

							//更新上次开关闭合时间变量
							psrSwitch->lastSwitchOnTime = currentTime.toString(timeFormat);
						}
					}
				}
			}
		}
	}
}

//监听开关故障跳闸次数
void notificationSwitchAccident(const Notification* notif, void* clientdata)
{
	ObId pmsBreakerObId = notif->getDataItems()->getObId();
	if (app->switchs.contains(pmsBreakerObId))
	{
		IntegerData faultStateData = *(IntegerData*)(notif->getData());
		if ((int)faultStateData==1)
		{
			app->switchs[pmsBreakerObId].hourAccidentTripCount +=1;
			app->switchs[pmsBreakerObId].dayAccidentTripCount +=1;

			//统计开关故障停电信息
			if(!app->switchs[pmsBreakerObId].isAccidentOff)
			{
				//获取到达实时库触发时刻时间
				time_t t = notif->getSequenceTime().tv_sec;
				QDateTime currentTime = QDateTime::fromTime_t(t);

				QString timeFormat = "yyyy-MM-dd-hh-mm-ss";
				QString currentTimeString = currentTime.toString(timeFormat);

				app->switchs[pmsBreakerObId].lastAccidentOffTime = currentTimeString;
				app->switchs[pmsBreakerObId].isAccidentOff = true;
				app->switchs[pmsBreakerObId].hourAccidentOffCount +=1;
				app->switchs[pmsBreakerObId].dayAccidentOffCount +=1;
			}
		}
	}

	//统计线路故障停电信息
	LinkData parentLinkData;
	try
	{
		m_OMSDatabase->read(pmsBreakerObId, AT_ParentLink, &parentLinkData);
	}
	catch (Exception& e)
	{
		LOG4CPLUS_ERROR(psrStatisticsServerLog, "error: read error!!!"+QString::number(pmsBreakerObId).toStdString());
	}

	//如果存在父节点
	if ((ObId)parentLinkData!=0)
	{
		if (app->lines.contains((ObId)parentLinkData))
		{
			IntegerData faultStateData = *(IntegerData*)(notif->getData());
			if ((int)faultStateData==1)
			{
				//统计线路故障停电信息
				if(!app->lines[(ObId)parentLinkData].isAccidentOff)
				{
					//获取到达实时库触发时刻时间
					time_t t = notif->getSequenceTime().tv_sec;
					QDateTime currentTime = QDateTime::fromTime_t(t);

					QString timeFormat = "yyyy-MM-dd-hh-mm-ss";
					QString currentTimeString = currentTime.toString(timeFormat);

					app->lines[(ObId)parentLinkData].lastAccidentOffTime = currentTimeString;
					app->lines[(ObId)parentLinkData].isAccidentOff = true;
					app->lines[(ObId)parentLinkData].hourAccidentOffCount +=1;
					app->lines[(ObId)parentLinkData].dayAccidentOffCount +=1;
				}
			}
		}
	}
}

//监听母线越限次数
void notificationBusbarLimit(const Notification* notif, void* clientdata)
{
	ObId mvPointObId = notif->getDataItems()->getObId();

	LinkData psrLinkData;
	m_OMSDatabase->read(mvPointObId, AT_PSRLink, &psrLinkData);
	//如果关联的一次设备Analog点存在
	if ((ObId)psrLinkData!=0)
	{
		LinkData parentLinkData;
		try
		{
			m_OMSDatabase->read((ObId)psrLinkData, AT_ParentLink, &parentLinkData);
		}
		catch (Exception& e)
		{
			LOG4CPLUS_ERROR(psrStatisticsServerLog, "error: read error!!!"+QString::number((ObId)psrLinkData).toStdString());
		}

		//如果存在父节点
		if ((ObId)parentLinkData!=0)
		{
			LinkData ppLinkData;
			try
			{
				m_OMSDatabase->read((ObId)parentLinkData, AT_ParentLink, &ppLinkData);
			}
			catch (Exception& e)
			{
				LOG4CPLUS_ERROR(psrStatisticsServerLog, "error: read error!!!"+QString::number((ObId)parentLinkData).toStdString());
			}

			//如果存在父节点
			if ((ObId)ppLinkData!=0)
			{
				//如果该MVPoint点关联的一次设备是母线
				if (app->busbars.contains((ObId)ppLinkData))
				{
					IntegerData limitData = *(IntegerData*)(notif->getData());
					IntegerData limitOldData = *(IntegerData*)(notif->getOldData());

					switch((int)limitData) 
					{
					case 1://越下下下限
					case 2://越下下限
					case 3://越下限
					case 5://越上限
					case 6://越上上限
					case 7://越上上上限
						{
							if ((int)limitOldData==4)//返回正常范围
							{
								app->busbars[(ObId)ppLinkData].hourLimitCount +=1;
								app->busbars[(ObId)ppLinkData].dayLimitCount +=1;
							}
							break;
						}
					case 9://检修值越下下下限
					case 10://检修值越下下限
					case 11://检修值越下限
					case 13://检修值越上限
					case 14://检修值越上上限
					case 15://检修值越上上上限
						{
							if ((int)limitOldData==12)//检修值返回正常范围
							{
								app->busbars[(ObId)ppLinkData].hourLimitCount +=1;
								app->busbars[(ObId)ppLinkData].dayLimitCount +=1;
							}
							break;
						}
					case 17://取代值越下下下限
					case 18://取代值越下下限
					case 19://取代值越下限
					case 21://取代值越上限
					case 22://取代值越上上限
					case 23://取代值越上上上限
						{
							if ((int)limitOldData==20)//取代值返回正常范围
							{
								app->busbars[(ObId)ppLinkData].hourLimitCount +=1;
								app->busbars[(ObId)ppLinkData].dayLimitCount +=1;
							}
							break;
						}
					case 25://取代加检修值越下下下限
					case 26://取代加检修值越下下限
					case 27://取代加检修值越下限
					case 29://取代加检修值越上限
					case 30://取代加检修值越上上限
					case 31://取代加检修值越上上上限
						{
							if ((int)limitOldData==28)//取代加检修值返回正常范围
							{
								app->busbars[(ObId)ppLinkData].hourLimitCount +=1;
								app->busbars[(ObId)ppLinkData].dayLimitCount +=1;
							}
							break;
						}
					case 33://无效值越下下下限
					case 34://无效值越下下限
					case 35://无效值越下限
					case 37://无效值越上限
					case 38://无效值越上上限
					case 39://无效值越上上上限
						{
							if ((int)limitOldData==36)//无效值返回正常范围
							{
								app->busbars[(ObId)ppLinkData].hourLimitCount +=1;
								app->busbars[(ObId)ppLinkData].dayLimitCount +=1;
							}
							break;
						}
					}
				}
			}
		}
	}
}

//监听变压器调档次数
void notificationTransformerShift(const Notification* notif, void* clientdata)
{
	ObId bscPointObId = notif->getDataItems()->getObId();

	IntegerData controlResultData = *(IntegerData*)(notif->getData());

	//如果控制结果是调档成功或调档失败
	if ((int)controlResultData==12||(int)controlResultData==13)
	{
		LinkData mvPointLinkData;
		try
		{
			m_OMSDatabase->read(bscPointObId, AT_MVPointLink, &mvPointLinkData);
		}
		catch (Exception& e)
		{
			LOG4CPLUS_ERROR(psrStatisticsServerLog, "error: read error!!!"+QString::number(bscPointObId).toStdString());
		}

		//如果关联的遥测点存在
		if ((ObId)mvPointLinkData!=0)
		{
			LinkData psrLinkData;
			try
			{
				m_OMSDatabase->read((ObId)mvPointLinkData, AT_PSRLink, &psrLinkData);
			}
			catch (Exception& e)
			{
				LOG4CPLUS_ERROR(psrStatisticsServerLog, "error: read error!!!"+QString::number((ObId)mvPointLinkData).toStdString());
			}

			//如果关联的一次设备Analog点存在
			if ((ObId)psrLinkData!=0)
			{
				LinkData parentLinkData;
				try
				{
					m_OMSDatabase->read((ObId)psrLinkData, AT_ParentLink, &parentLinkData);
				}
				catch (Exception& e)
				{
					LOG4CPLUS_ERROR(psrStatisticsServerLog, "error: read error!!!"+QString::number((ObId)psrLinkData).toStdString());
				}

				//如果存在父节点PMSTerminal
				if ((ObId)parentLinkData!=0)
				{
					LinkData ppLinkData;
					try
					{
						m_OMSDatabase->read((ObId)parentLinkData, AT_ParentLink, &ppLinkData);
					}
					catch (Exception& e)
					{
						LOG4CPLUS_ERROR(psrStatisticsServerLog, "error: read error!!!"+QString::number((ObId)parentLinkData).toStdString());
					}

					//如果存在父节点是变压器卷
					if ((ObId)ppLinkData!=0)
					{
						LinkData pppLinkData;
						try
						{
							m_OMSDatabase->read((ObId)ppLinkData, AT_ParentLink, &pppLinkData);
						}
						catch (Exception& e)
						{
							LOG4CPLUS_ERROR(psrStatisticsServerLog, "error: read error!!!"+QString::number((ObId)ppLinkData).toStdString());
						}

						//如果存在父节点是变压器
						if ((ObId)pppLinkData!=0)
						{
							//如果该MVPoint点关联的一次设备是变压器
							if (app->transformers.contains((ObId)pppLinkData))
							{
								//如果该MVPoint点是统计点
								if (app->transformers[(ObId)pppLinkData].relatePoints["TapPos_MVPoint"]==(ObId)mvPointLinkData)
								{
									app->transformers[(ObId)pppLinkData].hourShiftCount +=1;
									app->transformers[(ObId)pppLinkData].dayShiftCount +=1;
								}
							}
						}
					}
				}
			}
		}
	}
}

//监听线路负荷即功率的值改变
void notificationLineValue(const Notification* notif, void* clientdata)
{
	ObId mvPointObId = notif->getDataItems()->getObId();

	LinkData psrLinkData;
	try
	{
		m_OMSDatabase->read(mvPointObId, AT_PSRLink, &psrLinkData);
	}
	catch (Exception& e)
	{
		LOG4CPLUS_ERROR(psrStatisticsServerLog, "error: read error!!!"+QString::number(mvPointObId).toStdString());
	}

	//如果关联的一次设备Analog点存在
	if ((ObId)psrLinkData!=0)
	{
		LinkData parentLinkData;
		try
		{
			m_OMSDatabase->read((ObId)psrLinkData, AT_ParentLink, &parentLinkData);
		}
		catch (Exception& e)
		{
			LOG4CPLUS_ERROR(psrStatisticsServerLog, "error: read error!!!"+QString::number((ObId)psrLinkData).toStdString());
		}

		//如果存在父节点terminal点
		if ((ObId)parentLinkData!=0)
		{
			LinkData ppLinkData;
			try
			{
				m_OMSDatabase->read((ObId)parentLinkData, AT_ParentLink, &ppLinkData);
			}
			catch (Exception& e)
			{
				LOG4CPLUS_ERROR(psrStatisticsServerLog, "error: read error!!!"+QString::number((ObId)parentLinkData).toStdString());
			}

			//如果存在父节点PMSBreaker
			if ((ObId)ppLinkData!=0)
			{
				LinkData pppLinkData;
				try
				{
					m_OMSDatabase->read((ObId)ppLinkData, AT_ParentLink, &pppLinkData);
				}
				catch (Exception& e)
				{
					LOG4CPLUS_ERROR(psrStatisticsServerLog, "error: read error!!!"+QString::number((ObId)ppLinkData).toStdString());
				}

				//如果存在父节点Feeder
				if ((ObId)pppLinkData!=0)
				{
					//如果该MVPoint点关联的一次设备是线路
					if (app->lines.contains((ObId)pppLinkData))
					{
						//如果该mvpoint点是统计点
						if (app->lines[(ObId)pppLinkData].relatePoints["P_MVPoint"]==mvPointObId)
						{
							//获取到达实时库触发时刻时间
							time_t t = notif->getSequenceTime().tv_sec;
							QDateTime currentTime = QDateTime::fromTime_t(t);
							
							QString timeFormat = "yyyy-MM-dd-hh-mm-ss";
							QString currentTimeString = currentTime.toString(timeFormat);

							FloatData valueData = *(FloatData*)(notif->getData());

							app->lines[(ObId)pppLinkData].latestLoad = (float)valueData;
							app->lines[(ObId)pppLinkData].hourSumLoad += (float)valueData;
							app->lines[(ObId)pppLinkData].hourLoadChangeCount +=1;
							app->lines[(ObId)pppLinkData].hourAvgLoad = (app->lines[(ObId)pppLinkData].hourSumLoad)/app->lines[(ObId)pppLinkData].hourLoadChangeCount;
							if (app->lines[(ObId)pppLinkData].hourMaxLoad<(float)valueData)
							{
								app->lines[(ObId)pppLinkData].hourMaxLoad = (float)valueData;
								app->lines[(ObId)pppLinkData].hourMaxLoadTime = currentTimeString;
							}
							if (app->lines[(ObId)pppLinkData].hourMinLoad>(float)valueData)
							{
								app->lines[(ObId)pppLinkData].hourMinLoad = (float)valueData;
								app->lines[(ObId)pppLinkData].hourMinLoadTime = currentTimeString;
							}

							app->lines[(ObId)pppLinkData].daySumLoad += (float)valueData;
							app->lines[(ObId)pppLinkData].dayLoadChangeCount +=1;
							app->lines[(ObId)pppLinkData].dayAvgLoad = (app->lines[(ObId)pppLinkData].daySumLoad)/app->lines[(ObId)pppLinkData].dayLoadChangeCount;
							if (app->lines[(ObId)pppLinkData].dayMaxLoad<(float)valueData)
							{
								app->lines[(ObId)pppLinkData].dayMaxLoad = (float)valueData;
								app->lines[(ObId)pppLinkData].dayMaxLoadTime = currentTimeString;
							}
							if (app->lines[(ObId)pppLinkData].dayMinLoad>(float)valueData)
							{
								app->lines[(ObId)pppLinkData].dayMinLoad = (float)valueData;
								app->lines[(ObId)pppLinkData].dayMinLoadTime = currentTimeString;
							}
						}
					}
				}
			}
		}
	}
}

//增量更新时，回调重新初始化
void notificationIncrementCommit(const Notification* notif, void* clientdata)
{
	//初始化数据结构
	app->initDataInfo();
}

//线路和开关带电状态是否变化的回调函数
void notificationLineAndSwitchRTEnergized(const Notification* notif, void* clientdata)
{
	ObId pmsTerminal = notif->getDataItems()->getObId();

	//获取线路或开关带电状态改变时刻时间
	time_t t = notif->getSequenceTime().tv_sec;
	QDateTime currentTime = QDateTime::fromTime_t(t);

	LinkData parentLinkData;
	try
	{
		m_OMSDatabase->read(pmsTerminal, AT_ParentLink, &parentLinkData);
	}
	catch (Exception& e)
	{
		LOG4CPLUS_ERROR(psrStatisticsServerLog, "error: read error!!!"+QString::number(pmsTerminal).toStdString());
	}

	//如果存在父节点
	if ((ObId)parentLinkData!=0)
	{
		//如果该PMSTerminal点父节点是开关
		if (app->switchs.contains((ObId)parentLinkData))
		{
			//如果该线路或开关是统计点
			if (app->switchs[(ObId)parentLinkData].relatePoints["0_PMSTerminal"]==pmsTerminal
				||app->switchs[(ObId)parentLinkData].relatePoints["1_PMSTerminal"]==pmsTerminal)
			{
				//如果存在开关的端子
				if (app->switchs[(ObId)parentLinkData].lastPowerOnTime!="")
				{
					const Data* p = notif->getData();
					int rtEnergizedData = (IntegerData) (*p);

					//计算运行时间
					QString timeFormat = "yyyy-MM-dd-hh-mm-ss";

					Switch* psrSwitch = &(app->switchs[(ObId)parentLinkData]);
					//如果该端子断电
					if ((int)rtEnergizedData==0)
					{
						//如果原来是带电的话
						if (psrSwitch->isPowerOn)
						{
							//统计停电次数
							psrSwitch->hourPowerOffCount +=1;
							psrSwitch->dayPowerOffCount +=1;


							psrSwitch->isPowerOn = false;

							int lastToCurrent = QDateTime::fromString(psrSwitch->lastPowerOnTime, timeFormat).secsTo(currentTime);//开关上次送电到当前时间的秒数						

							QStringList currentTimeList = currentTime.toString(timeFormat).split("-");// 当前时间分割
							QString currentHourStartTime = currentTimeList[0]+"-"+currentTimeList[1]+"-"+currentTimeList[2]+"-"+currentTimeList[3]+"-00-00";//当前时间对应的整点时间
							int startHourToCurrent = QDateTime::fromString(currentHourStartTime, timeFormat).secsTo(currentTime);//当前整点时间到当前时间的秒数

							//如果上次送电时间到当前时间差 大于 整点到当前时间差, 进行时累计送电统计
							if (lastToCurrent>startHourToCurrent)
							{
								psrSwitch->hourPowerOnTime = startHourToCurrent;
							}else{
								psrSwitch->hourPowerOnTime += lastToCurrent;
							}

							QString currentDayStartTime = currentTimeList[0]+"-"+currentTimeList[1]+"-"+currentTimeList[2]+"-00-00-00";//当前时间对应的当天起始时间
							int startDayToCurrent = QDateTime::fromString(currentDayStartTime, timeFormat).secsTo(currentTime);//当天起始时间到当前时间的秒数

							//如果上次送电时间到当前时间差 大于 当天起始时间到当前时间差, 进行日累计送电统计
							if (lastToCurrent>startDayToCurrent)
							{
								psrSwitch->dayPowerOnTime = startDayToCurrent;
							}else{
								psrSwitch->dayPowerOnTime += lastToCurrent;
							}

						}
					}else if ((int)rtEnergizedData==1)//如果该端子变为带电
					{
						IntegerData rtEnergizedData0;
						IntegerData rtEnergizedData1;
						try
						{
							m_OMSDatabase->read(psrSwitch->relatePoints["0_PMSTerminal"], AT_RTEnergized, &rtEnergizedData0);
							m_OMSDatabase->read(psrSwitch->relatePoints["1_PMSTerminal"], AT_RTEnergized, &rtEnergizedData1);
							//如果都带电
							if ((int)rtEnergizedData0==1&&(int)rtEnergizedData1==1)
							{
								//统计送电次数
								psrSwitch->hourPowerOnCount +=1;
								psrSwitch->dayPowerOnCount +=1;

								//将开关标记位，标记为送电
								psrSwitch->isPowerOn = true;

								//更新上次开关送电时间变量
								psrSwitch->lastPowerOnTime = currentTime.toString(timeFormat);

								//统计故障停电时长
								if (psrSwitch->isAccidentOff)
								{
									psrSwitch->isAccidentOff = false;

									int lastToCurrent = QDateTime::fromString(psrSwitch->lastAccidentOffTime, timeFormat).secsTo(currentTime);//开关上次故障时间到当前时间的秒数						

									QStringList currentTimeList = currentTime.toString(timeFormat).split("-");// 当前时间分割
									QString currentHourStartTime = currentTimeList[0]+"-"+currentTimeList[1]+"-"+currentTimeList[2]+"-"+currentTimeList[3]+"-00-00";//当前时间对应的整点时间
									int startHourToCurrent = QDateTime::fromString(currentHourStartTime, timeFormat).secsTo(currentTime);//当前整点时间到当前时间的秒数

									//如果上次故障时间到当前时间差 大于 整点到当前时间差, 进行时累计送电统计
									if (lastToCurrent>startHourToCurrent)
									{
										psrSwitch->hourAccidentOffTime = startHourToCurrent;
									}else{
										psrSwitch->hourAccidentOffTime += lastToCurrent;
									}

									QString currentDayStartTime = currentTimeList[0]+"-"+currentTimeList[1]+"-"+currentTimeList[2]+"-00-00-00";//当前时间对应的当天起始时间
									int startDayToCurrent = QDateTime::fromString(currentDayStartTime, timeFormat).secsTo(currentTime);//当天起始时间到当前时间的秒数

									//如果上次送电时间到当前时间差 大于 当天起始时间到当前时间差, 进行日累计送电统计
									if (lastToCurrent>startDayToCurrent)
									{
										psrSwitch->dayAccidentOffTime = startDayToCurrent;
									}else{
										psrSwitch->dayAccidentOffTime += lastToCurrent;
									}
								}
							}
						}
						catch (Exception& e)
						{
							LOG4CPLUS_ERROR(psrStatisticsServerLog, "error: read error!!!");
						}
					}
				}
			}
		}

		//如果是线路的PMSBreaker带电状态改变*****************************************************
		LinkData grandparentLinkData;
		try
		{
			m_OMSDatabase->read((ObId)parentLinkData, AT_ParentLink, &grandparentLinkData);
		}
		catch (Exception& e)
		{
			LOG4CPLUS_ERROR(psrStatisticsServerLog, "error: read error!!!"+QString::number((ObId)parentLinkData).toStdString());
		}

		//如果存在爷节点
		if ((ObId)grandparentLinkData!=0)
		{
			//如果该爷节点是线路
			if (app->lines.contains((ObId)grandparentLinkData))
			{
				//如果该线路或开关是统计点
				if (app->lines[(ObId)grandparentLinkData].relatePoints["0_PMSTerminal"]==pmsTerminal
					||app->lines[(ObId)grandparentLinkData].relatePoints["1_PMSTerminal"]==pmsTerminal)
				{
					//如果存在开关的端子
					if (app->lines[(ObId)grandparentLinkData].lastPowerOnTime!="")
					{
						const Data* p = notif->getData();
						int rtEnergizedData = (IntegerData) (*p);

						//计算运行时间
						QString timeFormat = "yyyy-MM-dd-hh-mm-ss";

						Line* line = &(app->lines[(ObId)grandparentLinkData]);
						//如果该端子断电
						if ((int)rtEnergizedData==0)
						{
							//如果原来是带电的话
							if (line->isPowerOn)
							{
								//统计停电次数
								line->hourPowerOffCount +=1;
								line->dayPowerOffCount +=1;


								line->isPowerOn = false;

								int lastToCurrent = QDateTime::fromString(line->lastPowerOnTime, timeFormat).secsTo(currentTime);//开关上次送电到当前时间的秒数						

								QStringList currentTimeList = currentTime.toString(timeFormat).split("-");// 当前时间分割
								QString currentHourStartTime = currentTimeList[0]+"-"+currentTimeList[1]+"-"+currentTimeList[2]+"-"+currentTimeList[3]+"-00-00";//当前时间对应的整点时间
								int startHourToCurrent = QDateTime::fromString(currentHourStartTime, timeFormat).secsTo(currentTime);//当前整点时间到当前时间的秒数

								//如果上次送电时间到当前时间差 大于 整点到当前时间差, 进行时累计送电统计
								if (lastToCurrent>startHourToCurrent)
								{
									line->hourPowerOnTime = startHourToCurrent;
								}else{
									line->hourPowerOnTime += lastToCurrent;
								}

								QString currentDayStartTime = currentTimeList[0]+"-"+currentTimeList[1]+"-"+currentTimeList[2]+"-00-00-00";//当前时间对应的当天起始时间
								int startDayToCurrent = QDateTime::fromString(currentDayStartTime, timeFormat).secsTo(currentTime);//当天起始时间到当前时间的秒数

								//如果上次送电时间到当前时间差 大于 当天起始时间到当前时间差, 进行日累计送电统计
								if (lastToCurrent>startDayToCurrent)
								{
									line->dayPowerOnTime = startDayToCurrent;
								}else{
									line->dayPowerOnTime += lastToCurrent;
								}

							}
						}else if ((int)rtEnergizedData==1)//如果该端子变为带电
						{
							IntegerData rtEnergizedData0;
							IntegerData rtEnergizedData1;
							try
							{
								m_OMSDatabase->read(line->relatePoints["0_PMSTerminal"], AT_RTEnergized, &rtEnergizedData0);
								m_OMSDatabase->read(line->relatePoints["1_PMSTerminal"], AT_RTEnergized, &rtEnergizedData1);
								//如果都带电
								if ((int)rtEnergizedData0==1&&(int)rtEnergizedData1==1)
								{
									//统计送电次数
									line->hourPowerOnCount +=1;
									line->dayPowerOnCount +=1;

									//将开关标记位，标记为送电
									line->isPowerOn = true;

									//更新上次开关送电时间变量
									line->lastPowerOnTime = currentTime.toString(timeFormat);

									//统计故障停电时长
									if (line->isAccidentOff)
									{
										line->isAccidentOff = false;

										int lastToCurrent = QDateTime::fromString(line->lastAccidentOffTime, timeFormat).secsTo(currentTime);//开关上次故障时间到当前时间的秒数						

										QStringList currentTimeList = currentTime.toString(timeFormat).split("-");// 当前时间分割
										QString currentHourStartTime = currentTimeList[0]+"-"+currentTimeList[1]+"-"+currentTimeList[2]+"-"+currentTimeList[3]+"-00-00";//当前时间对应的整点时间
										int startHourToCurrent = QDateTime::fromString(currentHourStartTime, timeFormat).secsTo(currentTime);//当前整点时间到当前时间的秒数

										//如果上次故障时间到当前时间差 大于 整点到当前时间差, 进行时累计送电统计
										if (lastToCurrent>startHourToCurrent)
										{
											line->hourAccidentOffTime = startHourToCurrent;
										}else{
											line->hourAccidentOffTime += lastToCurrent;
										}

										QString currentDayStartTime = currentTimeList[0]+"-"+currentTimeList[1]+"-"+currentTimeList[2]+"-00-00-00";//当前时间对应的当天起始时间
										int startDayToCurrent = QDateTime::fromString(currentDayStartTime, timeFormat).secsTo(currentTime);//当天起始时间到当前时间的秒数

										//如果上次送电时间到当前时间差 大于 当天起始时间到当前时间差, 进行日累计送电统计
										if (lastToCurrent>startDayToCurrent)
										{
											line->dayAccidentOffTime = startDayToCurrent;
										}else{
											line->dayAccidentOffTime += lastToCurrent;
										}
									}
								}
							}
							catch (Exception& e)
							{
								LOG4CPLUS_ERROR(psrStatisticsServerLog, "error: read error!!!");
							}
						}
					}
				}
			}
		}
	}
}

//线路（馈线）动作次数，成功失败次数的回调函数:使用ObjOrder与馈线自动化FA交互
void notificationFeederObjOrder(const Notification* notif, void* clientdata)
{
	ObId feederObId = notif->getDataItems()->getObId();
	if (app->lines.contains(feederObId))
	{
		Line* line = &(app->lines[feederObId]);
		IntegerData objOrderData = *(IntegerData*)(notif->getData());
		switch((int)objOrderData)
		{
		case 0://如果馈线动作
			{
				line->hourFeederActionTimes +=1;
				line->dayFeederActionTimes +=1;
				break;
			}
		case 1://如果馈线动作成功
			{
				line->hourFeederSuccessTimes +=1;
				line->dayFeederSuccessTimes +=1;
				break;
			}
		case 2://如果馈线动作失败
			{
				line->hourFeederFaultTimes +=1;
				line->dayFeederFaultTimes +=1;
				break;
			}
		}		
	}
}