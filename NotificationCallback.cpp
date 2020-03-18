#include "NotificationCallback.h"

extern PSRStatisticsServer *app;//����qtӦ���ⲿȫ�ֱ���
extern Database *m_OMSDatabase;//ʵʱ���ݿ�ָ���ⲿȫ�ֱ���
extern Logger psrStatisticsServerLog;//log4c��־

//��������-��բ��������
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

	//���������DPSң�ŵ����
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

		//���������һ���豸Discrete�����
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

			//������ڸ��ڵ�
			if ((ObId)parentLinkData!=0)
			{
				//�����DPCPoint�������һ���豸�ǿ���
				if (app->switchs.contains((ObId)parentLinkData))
				{
					//�����DPC����ͳ�Ƶ�
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
//��������-��բ����ʱ��,  ͬʱ��������ң�ű����������������ÿ��غͱ�ѹ����
void notificationSwitchRuntime(const Notification* notif, void* clientdata)

{	
	ObId dpsPointObId = notif->getDataItems()->getObId();

	IntegerData stateCommonData = *(IntegerData*)(notif->getData());
	
	//��ȡ���ش���ʱ��ʱ��
	time_t t = notif->getSequenceTime().tv_sec;
	QDateTime currentTime = QDateTime::fromTime_t(t);

	//�ֱ��ӡ����ʵʱ���ʱ��͵��ﱾӦ�õ�ʱ�䣬���ں������ⶨλ����
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

	//���������һ���豸Discrete����߱�ѹ���豸����
	if ((ObId)psrLinkData!=0)
	{
		bool isFaultType = false;

		if ((int)stateCommonData==1)//���dps�Ƕ�����ͳ�Ʊ�����������
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

			if (app->faultMeasureTypeObIds.contains((ObId)measureTypeLinkData))//������������ǹ��ϡ��¹ʻ�FTU��������
			{
				isFaultType = true;

				if (app->transformers.contains((ObId)psrLinkData))//���������һ���豸�Ǳ�ѹ��
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

		//������ڸ��ڵ�
		if ((ObId)parentLinkData!=0)
		{
			//�����DPSPoint�������һ���豸�ǿ���
			if (app->switchs.contains((ObId)parentLinkData))
			{
				// ************�����DPS���Ǳ�������ͳ�Ƶ�. ͳ�Ʊ�����������
				if (isFaultType)
				{
					app->switchs[(ObId)parentLinkData].hourProtectionsCount +=1;
					app->switchs[(ObId)parentLinkData].dayProtectionsCount +=1;
				}


				//****************�����DPS���ǿ���ͳ�Ƶ�. ͳ�ƿ�������ʱ��
				if (app->switchs[(ObId)parentLinkData].relatePoints["CB_DPSPoint"]==dpsPointObId)
				{
					//������ڿ���DPS�����ǿ���-λ�����͵�DPS��
					if (app->switchs[(ObId)parentLinkData].lastSwitchOnTime!="")
					{
						IntegerData stateData = *(IntegerData*)(notif->getData());

						//��������ʱ��
						QString timeFormat = "yyyy-MM-dd-hh-mm-ss";

						Switch* psrSwitch = &(app->switchs[(ObId)parentLinkData]);
						//���������բ
						if ((int)stateData==0)
						{
							//�����ر��λ�����Ϊ�Ͽ�
							psrSwitch->isSwitchOn = false;

							int lastToCurrent = QDateTime::fromString(psrSwitch->lastSwitchOnTime, timeFormat).secsTo(currentTime);//�����ϴαպϵ���ǰʱ�������						

							QStringList currentTimeList = currentTime.toString(timeFormat).split("-");// ��ǰʱ��ָ�
							QString currentHourStartTime = currentTimeList[0]+"-"+currentTimeList[1]+"-"+currentTimeList[2]+"-"+currentTimeList[3]+"-00-00";//��ǰʱ���Ӧ������ʱ��
							int startHourToCurrent = QDateTime::fromString(currentHourStartTime, timeFormat).secsTo(currentTime);//��ǰ����ʱ�䵽��ǰʱ�������

							//����ϴαպ�ʱ�䵽��ǰʱ��� ���� ���㵽��ǰʱ���, ����ʱ�ۼ�����ͳ��
							if (lastToCurrent>startHourToCurrent)
							{
								psrSwitch->hourRunTime = startHourToCurrent;
							}else{
								psrSwitch->hourRunTime += lastToCurrent;
							}

							QString currentDayStartTime = currentTimeList[0]+"-"+currentTimeList[1]+"-"+currentTimeList[2]+"-00-00-00";//��ǰʱ���Ӧ�ĵ�����ʼʱ��
							int startDayToCurrent = QDateTime::fromString(currentDayStartTime, timeFormat).secsTo(currentTime);//������ʼʱ�䵽��ǰʱ�������

							//����ϴαպ�ʱ�䵽��ǰʱ��� ���� ������ʼʱ�䵽��ǰʱ���, �������ۼ�����ͳ��
							if (lastToCurrent>startDayToCurrent)
							{
								psrSwitch->dayRunTime = startDayToCurrent;
							}else{
								psrSwitch->dayRunTime += lastToCurrent;
							}
						}else if ((int)stateData==1)//������غ�բ
						{
							//�����ر��λ�����Ϊ�պ�
							psrSwitch->isSwitchOn = true;

							//�����ϴο��رպ�ʱ�����
							psrSwitch->lastSwitchOnTime = currentTime.toString(timeFormat);
						}
					}
				}
			}
		}
	}
}

//�������ع�����բ����
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

			//ͳ�ƿ��ع���ͣ����Ϣ
			if(!app->switchs[pmsBreakerObId].isAccidentOff)
			{
				//��ȡ����ʵʱ�ⴥ��ʱ��ʱ��
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

	//ͳ����·����ͣ����Ϣ
	LinkData parentLinkData;
	try
	{
		m_OMSDatabase->read(pmsBreakerObId, AT_ParentLink, &parentLinkData);
	}
	catch (Exception& e)
	{
		LOG4CPLUS_ERROR(psrStatisticsServerLog, "error: read error!!!"+QString::number(pmsBreakerObId).toStdString());
	}

	//������ڸ��ڵ�
	if ((ObId)parentLinkData!=0)
	{
		if (app->lines.contains((ObId)parentLinkData))
		{
			IntegerData faultStateData = *(IntegerData*)(notif->getData());
			if ((int)faultStateData==1)
			{
				//ͳ����·����ͣ����Ϣ
				if(!app->lines[(ObId)parentLinkData].isAccidentOff)
				{
					//��ȡ����ʵʱ�ⴥ��ʱ��ʱ��
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

//����ĸ��Խ�޴���
void notificationBusbarLimit(const Notification* notif, void* clientdata)
{
	ObId mvPointObId = notif->getDataItems()->getObId();

	LinkData psrLinkData;
	m_OMSDatabase->read(mvPointObId, AT_PSRLink, &psrLinkData);
	//���������һ���豸Analog�����
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

		//������ڸ��ڵ�
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

			//������ڸ��ڵ�
			if ((ObId)ppLinkData!=0)
			{
				//�����MVPoint�������һ���豸��ĸ��
				if (app->busbars.contains((ObId)ppLinkData))
				{
					IntegerData limitData = *(IntegerData*)(notif->getData());
					IntegerData limitOldData = *(IntegerData*)(notif->getOldData());

					switch((int)limitData) 
					{
					case 1://Խ��������
					case 2://Խ������
					case 3://Խ����
					case 5://Խ����
					case 6://Խ������
					case 7://Խ��������
						{
							if ((int)limitOldData==4)//����������Χ
							{
								app->busbars[(ObId)ppLinkData].hourLimitCount +=1;
								app->busbars[(ObId)ppLinkData].dayLimitCount +=1;
							}
							break;
						}
					case 9://����ֵԽ��������
					case 10://����ֵԽ������
					case 11://����ֵԽ����
					case 13://����ֵԽ����
					case 14://����ֵԽ������
					case 15://����ֵԽ��������
						{
							if ((int)limitOldData==12)//����ֵ����������Χ
							{
								app->busbars[(ObId)ppLinkData].hourLimitCount +=1;
								app->busbars[(ObId)ppLinkData].dayLimitCount +=1;
							}
							break;
						}
					case 17://ȡ��ֵԽ��������
					case 18://ȡ��ֵԽ������
					case 19://ȡ��ֵԽ����
					case 21://ȡ��ֵԽ����
					case 22://ȡ��ֵԽ������
					case 23://ȡ��ֵԽ��������
						{
							if ((int)limitOldData==20)//ȡ��ֵ����������Χ
							{
								app->busbars[(ObId)ppLinkData].hourLimitCount +=1;
								app->busbars[(ObId)ppLinkData].dayLimitCount +=1;
							}
							break;
						}
					case 25://ȡ���Ӽ���ֵԽ��������
					case 26://ȡ���Ӽ���ֵԽ������
					case 27://ȡ���Ӽ���ֵԽ����
					case 29://ȡ���Ӽ���ֵԽ����
					case 30://ȡ���Ӽ���ֵԽ������
					case 31://ȡ���Ӽ���ֵԽ��������
						{
							if ((int)limitOldData==28)//ȡ���Ӽ���ֵ����������Χ
							{
								app->busbars[(ObId)ppLinkData].hourLimitCount +=1;
								app->busbars[(ObId)ppLinkData].dayLimitCount +=1;
							}
							break;
						}
					case 33://��ЧֵԽ��������
					case 34://��ЧֵԽ������
					case 35://��ЧֵԽ����
					case 37://��ЧֵԽ����
					case 38://��ЧֵԽ������
					case 39://��ЧֵԽ��������
						{
							if ((int)limitOldData==36)//��Чֵ����������Χ
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

//������ѹ����������
void notificationTransformerShift(const Notification* notif, void* clientdata)
{
	ObId bscPointObId = notif->getDataItems()->getObId();

	IntegerData controlResultData = *(IntegerData*)(notif->getData());

	//������ƽ���ǵ����ɹ������ʧ��
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

		//���������ң������
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

			//���������һ���豸Analog�����
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

				//������ڸ��ڵ�PMSTerminal
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

					//������ڸ��ڵ��Ǳ�ѹ����
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

						//������ڸ��ڵ��Ǳ�ѹ��
						if ((ObId)pppLinkData!=0)
						{
							//�����MVPoint�������һ���豸�Ǳ�ѹ��
							if (app->transformers.contains((ObId)pppLinkData))
							{
								//�����MVPoint����ͳ�Ƶ�
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

//������·���ɼ����ʵ�ֵ�ı�
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

	//���������һ���豸Analog�����
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

		//������ڸ��ڵ�terminal��
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

			//������ڸ��ڵ�PMSBreaker
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

				//������ڸ��ڵ�Feeder
				if ((ObId)pppLinkData!=0)
				{
					//�����MVPoint�������һ���豸����·
					if (app->lines.contains((ObId)pppLinkData))
					{
						//�����mvpoint����ͳ�Ƶ�
						if (app->lines[(ObId)pppLinkData].relatePoints["P_MVPoint"]==mvPointObId)
						{
							//��ȡ����ʵʱ�ⴥ��ʱ��ʱ��
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

//��������ʱ���ص����³�ʼ��
void notificationIncrementCommit(const Notification* notif, void* clientdata)
{
	//��ʼ�����ݽṹ
	app->initDataInfo();
}

//��·�Ϳ��ش���״̬�Ƿ�仯�Ļص�����
void notificationLineAndSwitchRTEnergized(const Notification* notif, void* clientdata)
{
	ObId pmsTerminal = notif->getDataItems()->getObId();

	//��ȡ��·�򿪹ش���״̬�ı�ʱ��ʱ��
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

	//������ڸ��ڵ�
	if ((ObId)parentLinkData!=0)
	{
		//�����PMSTerminal�㸸�ڵ��ǿ���
		if (app->switchs.contains((ObId)parentLinkData))
		{
			//�������·�򿪹���ͳ�Ƶ�
			if (app->switchs[(ObId)parentLinkData].relatePoints["0_PMSTerminal"]==pmsTerminal
				||app->switchs[(ObId)parentLinkData].relatePoints["1_PMSTerminal"]==pmsTerminal)
			{
				//������ڿ��صĶ���
				if (app->switchs[(ObId)parentLinkData].lastPowerOnTime!="")
				{
					const Data* p = notif->getData();
					int rtEnergizedData = (IntegerData) (*p);

					//��������ʱ��
					QString timeFormat = "yyyy-MM-dd-hh-mm-ss";

					Switch* psrSwitch = &(app->switchs[(ObId)parentLinkData]);
					//����ö��Ӷϵ�
					if ((int)rtEnergizedData==0)
					{
						//���ԭ���Ǵ���Ļ�
						if (psrSwitch->isPowerOn)
						{
							//ͳ��ͣ�����
							psrSwitch->hourPowerOffCount +=1;
							psrSwitch->dayPowerOffCount +=1;


							psrSwitch->isPowerOn = false;

							int lastToCurrent = QDateTime::fromString(psrSwitch->lastPowerOnTime, timeFormat).secsTo(currentTime);//�����ϴ��͵絽��ǰʱ�������						

							QStringList currentTimeList = currentTime.toString(timeFormat).split("-");// ��ǰʱ��ָ�
							QString currentHourStartTime = currentTimeList[0]+"-"+currentTimeList[1]+"-"+currentTimeList[2]+"-"+currentTimeList[3]+"-00-00";//��ǰʱ���Ӧ������ʱ��
							int startHourToCurrent = QDateTime::fromString(currentHourStartTime, timeFormat).secsTo(currentTime);//��ǰ����ʱ�䵽��ǰʱ�������

							//����ϴ��͵�ʱ�䵽��ǰʱ��� ���� ���㵽��ǰʱ���, ����ʱ�ۼ��͵�ͳ��
							if (lastToCurrent>startHourToCurrent)
							{
								psrSwitch->hourPowerOnTime = startHourToCurrent;
							}else{
								psrSwitch->hourPowerOnTime += lastToCurrent;
							}

							QString currentDayStartTime = currentTimeList[0]+"-"+currentTimeList[1]+"-"+currentTimeList[2]+"-00-00-00";//��ǰʱ���Ӧ�ĵ�����ʼʱ��
							int startDayToCurrent = QDateTime::fromString(currentDayStartTime, timeFormat).secsTo(currentTime);//������ʼʱ�䵽��ǰʱ�������

							//����ϴ��͵�ʱ�䵽��ǰʱ��� ���� ������ʼʱ�䵽��ǰʱ���, �������ۼ��͵�ͳ��
							if (lastToCurrent>startDayToCurrent)
							{
								psrSwitch->dayPowerOnTime = startDayToCurrent;
							}else{
								psrSwitch->dayPowerOnTime += lastToCurrent;
							}

						}
					}else if ((int)rtEnergizedData==1)//����ö��ӱ�Ϊ����
					{
						IntegerData rtEnergizedData0;
						IntegerData rtEnergizedData1;
						try
						{
							m_OMSDatabase->read(psrSwitch->relatePoints["0_PMSTerminal"], AT_RTEnergized, &rtEnergizedData0);
							m_OMSDatabase->read(psrSwitch->relatePoints["1_PMSTerminal"], AT_RTEnergized, &rtEnergizedData1);
							//���������
							if ((int)rtEnergizedData0==1&&(int)rtEnergizedData1==1)
							{
								//ͳ���͵����
								psrSwitch->hourPowerOnCount +=1;
								psrSwitch->dayPowerOnCount +=1;

								//�����ر��λ�����Ϊ�͵�
								psrSwitch->isPowerOn = true;

								//�����ϴο����͵�ʱ�����
								psrSwitch->lastPowerOnTime = currentTime.toString(timeFormat);

								//ͳ�ƹ���ͣ��ʱ��
								if (psrSwitch->isAccidentOff)
								{
									psrSwitch->isAccidentOff = false;

									int lastToCurrent = QDateTime::fromString(psrSwitch->lastAccidentOffTime, timeFormat).secsTo(currentTime);//�����ϴι���ʱ�䵽��ǰʱ�������						

									QStringList currentTimeList = currentTime.toString(timeFormat).split("-");// ��ǰʱ��ָ�
									QString currentHourStartTime = currentTimeList[0]+"-"+currentTimeList[1]+"-"+currentTimeList[2]+"-"+currentTimeList[3]+"-00-00";//��ǰʱ���Ӧ������ʱ��
									int startHourToCurrent = QDateTime::fromString(currentHourStartTime, timeFormat).secsTo(currentTime);//��ǰ����ʱ�䵽��ǰʱ�������

									//����ϴι���ʱ�䵽��ǰʱ��� ���� ���㵽��ǰʱ���, ����ʱ�ۼ��͵�ͳ��
									if (lastToCurrent>startHourToCurrent)
									{
										psrSwitch->hourAccidentOffTime = startHourToCurrent;
									}else{
										psrSwitch->hourAccidentOffTime += lastToCurrent;
									}

									QString currentDayStartTime = currentTimeList[0]+"-"+currentTimeList[1]+"-"+currentTimeList[2]+"-00-00-00";//��ǰʱ���Ӧ�ĵ�����ʼʱ��
									int startDayToCurrent = QDateTime::fromString(currentDayStartTime, timeFormat).secsTo(currentTime);//������ʼʱ�䵽��ǰʱ�������

									//����ϴ��͵�ʱ�䵽��ǰʱ��� ���� ������ʼʱ�䵽��ǰʱ���, �������ۼ��͵�ͳ��
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

		//�������·��PMSBreaker����״̬�ı�*****************************************************
		LinkData grandparentLinkData;
		try
		{
			m_OMSDatabase->read((ObId)parentLinkData, AT_ParentLink, &grandparentLinkData);
		}
		catch (Exception& e)
		{
			LOG4CPLUS_ERROR(psrStatisticsServerLog, "error: read error!!!"+QString::number((ObId)parentLinkData).toStdString());
		}

		//�������ү�ڵ�
		if ((ObId)grandparentLinkData!=0)
		{
			//�����ү�ڵ�����·
			if (app->lines.contains((ObId)grandparentLinkData))
			{
				//�������·�򿪹���ͳ�Ƶ�
				if (app->lines[(ObId)grandparentLinkData].relatePoints["0_PMSTerminal"]==pmsTerminal
					||app->lines[(ObId)grandparentLinkData].relatePoints["1_PMSTerminal"]==pmsTerminal)
				{
					//������ڿ��صĶ���
					if (app->lines[(ObId)grandparentLinkData].lastPowerOnTime!="")
					{
						const Data* p = notif->getData();
						int rtEnergizedData = (IntegerData) (*p);

						//��������ʱ��
						QString timeFormat = "yyyy-MM-dd-hh-mm-ss";

						Line* line = &(app->lines[(ObId)grandparentLinkData]);
						//����ö��Ӷϵ�
						if ((int)rtEnergizedData==0)
						{
							//���ԭ���Ǵ���Ļ�
							if (line->isPowerOn)
							{
								//ͳ��ͣ�����
								line->hourPowerOffCount +=1;
								line->dayPowerOffCount +=1;


								line->isPowerOn = false;

								int lastToCurrent = QDateTime::fromString(line->lastPowerOnTime, timeFormat).secsTo(currentTime);//�����ϴ��͵絽��ǰʱ�������						

								QStringList currentTimeList = currentTime.toString(timeFormat).split("-");// ��ǰʱ��ָ�
								QString currentHourStartTime = currentTimeList[0]+"-"+currentTimeList[1]+"-"+currentTimeList[2]+"-"+currentTimeList[3]+"-00-00";//��ǰʱ���Ӧ������ʱ��
								int startHourToCurrent = QDateTime::fromString(currentHourStartTime, timeFormat).secsTo(currentTime);//��ǰ����ʱ�䵽��ǰʱ�������

								//����ϴ��͵�ʱ�䵽��ǰʱ��� ���� ���㵽��ǰʱ���, ����ʱ�ۼ��͵�ͳ��
								if (lastToCurrent>startHourToCurrent)
								{
									line->hourPowerOnTime = startHourToCurrent;
								}else{
									line->hourPowerOnTime += lastToCurrent;
								}

								QString currentDayStartTime = currentTimeList[0]+"-"+currentTimeList[1]+"-"+currentTimeList[2]+"-00-00-00";//��ǰʱ���Ӧ�ĵ�����ʼʱ��
								int startDayToCurrent = QDateTime::fromString(currentDayStartTime, timeFormat).secsTo(currentTime);//������ʼʱ�䵽��ǰʱ�������

								//����ϴ��͵�ʱ�䵽��ǰʱ��� ���� ������ʼʱ�䵽��ǰʱ���, �������ۼ��͵�ͳ��
								if (lastToCurrent>startDayToCurrent)
								{
									line->dayPowerOnTime = startDayToCurrent;
								}else{
									line->dayPowerOnTime += lastToCurrent;
								}

							}
						}else if ((int)rtEnergizedData==1)//����ö��ӱ�Ϊ����
						{
							IntegerData rtEnergizedData0;
							IntegerData rtEnergizedData1;
							try
							{
								m_OMSDatabase->read(line->relatePoints["0_PMSTerminal"], AT_RTEnergized, &rtEnergizedData0);
								m_OMSDatabase->read(line->relatePoints["1_PMSTerminal"], AT_RTEnergized, &rtEnergizedData1);
								//���������
								if ((int)rtEnergizedData0==1&&(int)rtEnergizedData1==1)
								{
									//ͳ���͵����
									line->hourPowerOnCount +=1;
									line->dayPowerOnCount +=1;

									//�����ر��λ�����Ϊ�͵�
									line->isPowerOn = true;

									//�����ϴο����͵�ʱ�����
									line->lastPowerOnTime = currentTime.toString(timeFormat);

									//ͳ�ƹ���ͣ��ʱ��
									if (line->isAccidentOff)
									{
										line->isAccidentOff = false;

										int lastToCurrent = QDateTime::fromString(line->lastAccidentOffTime, timeFormat).secsTo(currentTime);//�����ϴι���ʱ�䵽��ǰʱ�������						

										QStringList currentTimeList = currentTime.toString(timeFormat).split("-");// ��ǰʱ��ָ�
										QString currentHourStartTime = currentTimeList[0]+"-"+currentTimeList[1]+"-"+currentTimeList[2]+"-"+currentTimeList[3]+"-00-00";//��ǰʱ���Ӧ������ʱ��
										int startHourToCurrent = QDateTime::fromString(currentHourStartTime, timeFormat).secsTo(currentTime);//��ǰ����ʱ�䵽��ǰʱ�������

										//����ϴι���ʱ�䵽��ǰʱ��� ���� ���㵽��ǰʱ���, ����ʱ�ۼ��͵�ͳ��
										if (lastToCurrent>startHourToCurrent)
										{
											line->hourAccidentOffTime = startHourToCurrent;
										}else{
											line->hourAccidentOffTime += lastToCurrent;
										}

										QString currentDayStartTime = currentTimeList[0]+"-"+currentTimeList[1]+"-"+currentTimeList[2]+"-00-00-00";//��ǰʱ���Ӧ�ĵ�����ʼʱ��
										int startDayToCurrent = QDateTime::fromString(currentDayStartTime, timeFormat).secsTo(currentTime);//������ʼʱ�䵽��ǰʱ�������

										//����ϴ��͵�ʱ�䵽��ǰʱ��� ���� ������ʼʱ�䵽��ǰʱ���, �������ۼ��͵�ͳ��
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

//��·�����ߣ������������ɹ�ʧ�ܴ����Ļص�����:ʹ��ObjOrder�������Զ���FA����
void notificationFeederObjOrder(const Notification* notif, void* clientdata)
{
	ObId feederObId = notif->getDataItems()->getObId();
	if (app->lines.contains(feederObId))
	{
		Line* line = &(app->lines[feederObId]);
		IntegerData objOrderData = *(IntegerData*)(notif->getData());
		switch((int)objOrderData)
		{
		case 0://������߶���
			{
				line->hourFeederActionTimes +=1;
				line->dayFeederActionTimes +=1;
				break;
			}
		case 1://������߶����ɹ�
			{
				line->hourFeederSuccessTimes +=1;
				line->dayFeederSuccessTimes +=1;
				break;
			}
		case 2://������߶���ʧ��
			{
				line->hourFeederFaultTimes +=1;
				line->dayFeederFaultTimes +=1;
				break;
			}
		}		
	}
}