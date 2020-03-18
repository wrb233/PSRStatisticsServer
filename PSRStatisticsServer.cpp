#include "PSRStatisticsServer.h"

extern PSRStatisticsServer *app;//����qtӦ���ⲿȫ�ֱ���
extern Database *m_OMSDatabase;//ʵʱ���ݿ�ָ���ⲿȫ�ֱ���
extern DBPOOLHANDLE dbPoolHandle;//��ʷ���ⲿȫ�ֱ���
extern Logger psrStatisticsServerLog;//log4c��־

//���캯��
PSRStatisticsServer::PSRStatisticsServer(int &argc, char **argv,SignalHandler sigtermHandler,const OptionList& optionList,
	EnvironmentScope environmentScope) : QtApplication(argc, argv, sigtermHandler, optionList, environmentScope)

{
	//�ڹ��캯���л�ȡʵʱ�����
	m_OMSDatabase = getDatabase();

	//�����ݴ�
	try
	{
		activateFaultTolerance();
	}catch(...)
	{
		LOG4CPLUS_ERROR(psrStatisticsServerLog, "Active faultTolerance error");
		std::exit(0);
	}

	//���������������
	if (!getFaultTolerance()->amITheCurrentMachine())
	{
		LOG4CPLUS_ERROR(psrStatisticsServerLog, "I am not TheCurrentMachine");
	}
}


//��������
PSRStatisticsServer::~PSRStatisticsServer()

{

}

//��ʼ�������ź���������
void PSRStatisticsServer::initFaultMeasureType()
{
	//�����غ������¹ʵ�Measuretype�������ʹ洢��QList��
	StringData keyNameData;
	//���������ͺ�ȫ���������ʹ洢��������
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

			//ɾ��ָ���ڴ�ռ�
			delete[] measureTypeObId;
		}
	}
}

//��ʼ�����ݽṹ
void PSRStatisticsServer::initDataInfo()
{
	initSwitchDataInfo();
	initBusbarDataInfo();
	initTransformerDataInfo();
	initLineDataInfo();
}

//��ʼ��ע��ص�����
void PSRStatisticsServer::initNotificationCallback()
{
	//��������-��բ��������
	Request reqTrigerSwitchOptCount;
	reqTrigerSwitchOptCount.set(AT_ControlResult, OT_DPCPoint, NEW_NOTIFICATION, notificationSwitchOptCount, (void*)this,NOTIFY_ON_WRITE); 
	m_OMSDatabase->notify(&reqTrigerSwitchOptCount, 1);

	//��������-��բ����ʱ��
	Request reqTrigerSwitchRuntime;
	reqTrigerSwitchRuntime.set(AT_State, OT_DPSPoint, NEW_NOTIFICATION, notificationSwitchRuntime, (void*)this,NOTIFY_ON_CHANGE); 
	m_OMSDatabase->notify(&reqTrigerSwitchRuntime, 1);

	//�������ع�����բ����
	Request reqTrigerSwitchAccident;
	reqTrigerSwitchAccident.set(AT_FaultState, OT_PMSBreaker, NEW_NOTIFICATION, notificationSwitchAccident, (void*)this,NOTIFY_ON_WRITE); 
	m_OMSDatabase->notify(&reqTrigerSwitchAccident, 1);

	//����ĸ��Խ�޴���
	Request reqTrigerBusbarLimit;
	reqTrigerBusbarLimit.set(AT_Limit, OT_MVPoint, NEW_NOTIFICATION, notificationBusbarLimit, (void*)this,NOTIFY_ON_CHANGE); 
	m_OMSDatabase->notify(&reqTrigerBusbarLimit, 1);

	//������ѹ����������
	Request reqTrigerTransformerShift;
	reqTrigerTransformerShift.set(AT_ControlResult, OT_BSCPoint, NEW_NOTIFICATION, notificationTransformerShift, (void*)this,NOTIFY_ON_CHANGE); 
	m_OMSDatabase->notify(&reqTrigerTransformerShift, 1);

	//������·����ֵ�仯
	Request reqTrigerLineValue;
	reqTrigerLineValue.set(AT_Value, OT_MVPoint, NEW_NOTIFICATION, notificationLineValue, (void*)this, NOTIFY_ON_CHANGE); 
	m_OMSDatabase->notify(&reqTrigerLineValue, 1);

	//������·�Ϳ��ش���״̬�Ƿ�仯
	Request reqTrigerLineAndSwitchRTEnergized;
	reqTrigerLineAndSwitchRTEnergized.set(AT_RTEnergized, OT_PMSTerminal, NEW_NOTIFICATION, notificationLineAndSwitchRTEnergized, (void*)this, NOTIFY_ON_CHANGE); 
	m_OMSDatabase->notify(&reqTrigerLineAndSwitchRTEnergized, 1);

	//������·�����ߣ������������ɹ�ʧ�ܴ���
	Request reqTrigerFeederAction;
	reqTrigerFeederAction.set(AT_ObjOrder, OT_Feeder, NEW_NOTIFICATION, notificationFeederObjOrder, (void*)this, NOTIFY_ON_WRITE); 
	m_OMSDatabase->notify(&reqTrigerFeederAction, 1);

	//������������
	IncrementCommit ic;
	ic.NotfyIncrementCommit();
}

//ͳ�ƿ���-��բ
void PSRStatisticsServer::statisticSwitch(QString psrName, QString type)
{
	//��������ڣ����½�
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

	//ѭ���������п���-��բ
	QMap<ObId, Switch>::iterator switchMapIterator;
	for (switchMapIterator=switchs.begin();switchMapIterator!=switchs.end();switchMapIterator++)
	{
		//���������й���ƽ����
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

		//���������޹���ƽ����
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

		//��������ʱ��
		QString timeFormat = "yyyy-MM-dd-hh-mm-ss";
		QString time24Format = "yyyy-MM-dd hh:mm:ss";
		QDateTime currentTime = QDateTime::currentDateTime();
		QStringList currentTimeList = currentTime.toString(timeFormat).split("-");// ��ǰʱ��ָ�

		QString currentHourStartTime = currentTimeList[0]+"-"+currentTimeList[1]+"-"+currentTimeList[2]+"-"+currentTimeList[3]+"-00-00";//��ǰʱ���Ӧ������ʱ��
		QDateTime currentHourStartDate = QDateTime::fromString(currentHourStartTime, timeFormat);

		QString currentDayStartTime = currentTimeList[0]+"-"+currentTimeList[1]+"-"+currentTimeList[2]+"-00-00-00";//��ǰʱ���Ӧ�ĵ�����ʼʱ��
		QDateTime currentDayStartDate = QDateTime::fromString(currentDayStartTime, timeFormat);
	
		//�����Сʱ��ͳ��
		if (type=="hour")
		{
			//������ڿ���DPS��
			if (switchMapIterator->lastSwitchOnTime!="")
			{
				//�����ʱ���رպ�״̬
				if (switchMapIterator->isSwitchOn)
				{
					int lastToCurrent = QDateTime::fromString(switchMapIterator->lastSwitchOnTime, timeFormat).secsTo(currentHourStartDate);//�����ϴαպϵ���ǰ����ʱ�������

					//���ʱ������3600s��1Сʱ
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

			//���������������
			if (switchMapIterator->lastPowerOnTime!="")
			{
				//�����ʱ��·�򿪹����͵�״̬
				if (switchMapIterator->isPowerOn)
				{
					int lastToCurrent = QDateTime::fromString(switchMapIterator->lastPowerOnTime, timeFormat).secsTo(currentHourStartDate);//��·�򿪹��ϴ��͵絽��ǰ����ʱ�������

					//���ʱ������3600s��1Сʱ
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

				//��·�򿪹�ͣ��ʱ��
				switchMapIterator->hourPowerOffTime = 3600 - switchMapIterator->hourPowerOnTime;

				//�����ʱ��·�򿪹��Ƕϵ�״̬
				if (!switchMapIterator->isPowerOn)
				{
					//����ǹ��϶ϵ�
					if (switchMapIterator->isAccidentOff)
					{
						int lastToCurrent = QDateTime::fromString(switchMapIterator->lastAccidentOffTime, timeFormat).secsTo(currentHourStartDate);//��·�򿪹��ϴ��͵絽��ǰ����ʱ�������

						//���ʱ������3600s��1Сʱ
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

				//�˹�ͣ��ʱ��
				switchMapIterator->hourArtificialOffTime = switchMapIterator->hourPowerOffTime - switchMapIterator->hourAccidentOffTime;
				
				//�˹�ͣ�����
				switchMapIterator->hourArtificialOffCount = switchMapIterator->hourPowerOffCount - switchMapIterator->hourAccidentOffCount;

				// ����ƻ������⣬������
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

			//�������ݵ���ʷ��
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

			//���������
			if (getFaultTolerance()->amITheCurrentMachine())
			{
				if(CPS_ORM_DirectExecuteSql(dbPoolHandle,insertSql.toStdString())>=0)
				{
					qDebug()<<"INSERT HOUR DATA SUCCESS!";
				}
			}

			//����
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
		}else if (type=="day")//������ռ�ͳ��
		{
			//������ڿ���DPS��
			if (switchMapIterator->lastSwitchOnTime!="")
			{
				//�����ʱ���رպ�״̬
				if (switchMapIterator->isSwitchOn)
				{
					int lastToCurrent = QDateTime::fromString(switchMapIterator->lastSwitchOnTime, timeFormat).secsTo(currentDayStartDate);//�����ϴαպϵ�������ʼʱ�������

					//���ʱ������3600*24s��1��
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

			//���������������
			if (switchMapIterator->lastPowerOnTime!="")
			{
				//�����ʱ��·�򿪹����͵�״̬
				if (switchMapIterator->isPowerOn)
				{
					int lastToCurrent = QDateTime::fromString(switchMapIterator->lastPowerOnTime, timeFormat).secsTo(currentDayStartDate);//��·�򿪹��ϴ��͵絽�������ʱ�������

					//���ʱ������3600*24s��1��
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

				//��·�򿪹�ͣ��ʱ��
				switchMapIterator->dayPowerOffTime = 3600*24 - switchMapIterator->dayPowerOnTime;

				//�����ʱ��·�򿪹��Ƕϵ�״̬
				if (!switchMapIterator->isPowerOn)
				{
					//����ǹ��϶ϵ�
					if (switchMapIterator->isAccidentOff)
					{
						int lastToCurrent = QDateTime::fromString(switchMapIterator->lastAccidentOffTime, timeFormat).secsTo(currentDayStartDate);//��·�򿪹��ϴ��͵絽�������ʱ�������

						//���ʱ������3600*24s��1��
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

				//�˹�ͣ��ʱ��
				switchMapIterator->dayArtificialOffTime = switchMapIterator->dayPowerOffTime - switchMapIterator->dayAccidentOffTime;
				//�˹�ͣ�����
				switchMapIterator->dayArtificialOffCount = switchMapIterator->dayPowerOffCount - switchMapIterator->dayAccidentOffCount;
				// ����ƻ������⣬������
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

			//�������ݵ���ʷ��
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

			//���������
			if (getFaultTolerance()->amITheCurrentMachine())
			{
				if(CPS_ORM_DirectExecuteSql(dbPoolHandle,insertSql.toStdString())>=0)
				{
					qDebug()<<"INSERT DAY DATA SUCCESS!";
				}
			}

			//����
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

//ͳ��ĸ��
void PSRStatisticsServer::statisticBusbar(QString psrName, QString type)
{
	//��������ڣ����½�
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

	//ѭ����������ĸ��
	QMap<ObId, Busbar>::iterator busbarMapIterator;
	for (busbarMapIterator=busbars.begin();busbarMapIterator!=busbars.end();busbarMapIterator++)
	{
		//���������ѹ��ƽ����
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
		QStringList currentTimeList = currentTime.toString(timeFormat).split("-");// ��ǰʱ��ָ�

		QString currentHourStartTime = currentTimeList[0]+"-"+currentTimeList[1]+"-"+currentTimeList[2]+"-"+currentTimeList[3]+"-00-00";//��ǰʱ���Ӧ������ʱ��
		QDateTime currentHourStartDate = QDateTime::fromString(currentHourStartTime, timeFormat);

		QString currentDayStartTime = currentTimeList[0]+"-"+currentTimeList[1]+"-"+currentTimeList[2]+"-00-00-00";//��ǰʱ���Ӧ�ĵ�����ʼʱ��
		QDateTime currentDayStartDate = QDateTime::fromString(currentDayStartTime, timeFormat);


		//�����Сʱ��ͳ��
		if (type=="hour")
		{
			busbarMapIterator->statisticDate = currentHourStartDate.toString(time24Format);

			//�������ݵ���ʷ��
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

			//���������
			if (getFaultTolerance()->amITheCurrentMachine())
			{
				if(CPS_ORM_DirectExecuteSql(dbPoolHandle,insertSql.toStdString())>=0)
				{
					qDebug()<<"INSERT HOUR DATA SUCCESS";
				}	
			}

			//����
			busbarMapIterator->hourLimitCount = 0;
		}else if (type=="day")//������ռ�ͳ��
		{
			busbarMapIterator->statisticDate = currentDayStartDate.addDays(-1).toString(time24Format);

			//�������ݵ���ʷ��
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

			//���������
			if (getFaultTolerance()->amITheCurrentMachine())
			{
				if(CPS_ORM_DirectExecuteSql(dbPoolHandle,insertSql.toStdString())>=0)
				{
					qDebug()<<"INSERT DAY DATA SUCCESS";
				}	
			}

			//����
			busbarMapIterator->dayLimitCount = 0;
		}
	}

}

//ͳ�Ʊ�ѹ��
void PSRStatisticsServer::statisticTransformer(QString psrName, QString type)
{
	//��������ڣ����½�
	QString tableName = createTableByDate(psrName, 2);
	FloatData paramMvaData;

	//ѭ���������б�ѹ��
	QMap<ObId, Transformer>::iterator transformerMapIterator;
	for (transformerMapIterator=transformers.begin();transformerMapIterator!=transformers.end();transformerMapIterator++)
	{
		//����������ĸ�����
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
		QStringList currentTimeList = currentTime.toString(timeFormat).split("-");// ��ǰʱ��ָ�

		QString currentHourStartTime = currentTimeList[0]+"-"+currentTimeList[1]+"-"+currentTimeList[2]+"-"+currentTimeList[3]+"-00-00";//��ǰʱ���Ӧ������ʱ��
		QDateTime currentHourStartDate = QDateTime::fromString(currentHourStartTime, timeFormat);

		QString currentDayStartTime = currentTimeList[0]+"-"+currentTimeList[1]+"-"+currentTimeList[2]+"-00-00-00";//��ǰʱ���Ӧ�ĵ�����ʼʱ��
		QDateTime currentDayStartDate = QDateTime::fromString(currentDayStartTime, timeFormat);

		//�����Сʱ��ͳ��
		if (type=="hour")
		{
			transformerMapIterator->statisticDate = currentHourStartDate.toString(time24Format);

			//�������ݵ���ʷ��
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

			//���������
			if (getFaultTolerance()->amITheCurrentMachine())
			{
				if(CPS_ORM_DirectExecuteSql(dbPoolHandle,insertSql.toStdString())>=0)
				{
					qDebug()<<"INSERT HOUR DATA SUCCESS";
				}	
			}

			//����
			transformerMapIterator->hourShiftCount = 0;

			transformerMapIterator->hourProtectionsCount = 0;
		}else if (type=="day")//������ռ�ͳ��
		{
			transformerMapIterator->statisticDate = currentDayStartDate.addDays(-1).toString(time24Format);

			//�������ݵ���ʷ��
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

			//���������
			if (getFaultTolerance()->amITheCurrentMachine())
			{
				if(CPS_ORM_DirectExecuteSql(dbPoolHandle,insertSql.toStdString())>=0)
				{
					qDebug()<<"INSERT DAY DATA SUCCESS";
				}	
			}

			//����
			transformerMapIterator->dayShiftCount = 0;

			transformerMapIterator->dayProtectionsCount = 0;
		}
	}
}

//ͳ����·
void PSRStatisticsServer::statisticLine(QString psrName, QString type)
{
	//��������ڣ����½�
	QString tableName = createTableByDate(psrName, 3);

	//ѭ������������·
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
		//���������ѹ��ƽ����
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
		QStringList currentTimeList = currentTime.toString(timeFormat).split("-");// ��ǰʱ��ָ�

		QString currentHourStartTime = currentTimeList[0]+"-"+currentTimeList[1]+"-"+currentTimeList[2]+"-"+currentTimeList[3]+"-00-00";//��ǰʱ���Ӧ������ʱ��
		QDateTime currentHourStartDate = QDateTime::fromString(currentHourStartTime, timeFormat);

		QString currentDayStartTime = currentTimeList[0]+"-"+currentTimeList[1]+"-"+currentTimeList[2]+"-00-00-00";//��ǰʱ���Ӧ�ĵ�����ʼʱ��
		QDateTime currentDayStartDate = QDateTime::fromString(currentDayStartTime, timeFormat);

		//�����Сʱ��ͳ��
		if (type=="hour")
		{
			//���������������
			if (lineMapIterator->lastPowerOnTime!="")
			{
				//�����ʱ��·�򿪹����͵�״̬
				if (lineMapIterator->isPowerOn)
				{
					int lastToCurrent = QDateTime::fromString(lineMapIterator->lastPowerOnTime, timeFormat).secsTo(currentHourStartDate);//��·�򿪹��ϴ��͵絽��ǰ����ʱ�������

					//���ʱ������3600s��1Сʱ
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

				//��·�򿪹�ͣ��ʱ��
				lineMapIterator->hourPowerOffTime = 3600 - lineMapIterator->hourPowerOnTime;

				//�����ʱ��·�򿪹��Ƕϵ�״̬
				if (!lineMapIterator->isPowerOn)
				{
					//����ǹ��϶ϵ�
					if (lineMapIterator->isAccidentOff)
					{
						int lastToCurrent = QDateTime::fromString(lineMapIterator->lastAccidentOffTime, timeFormat).secsTo(currentHourStartDate);//��·�򿪹��ϴ��͵絽��ǰ����ʱ�������

						//���ʱ������3600s��1Сʱ
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

				//�˹�ͣ��ʱ��
				lineMapIterator->hourArtificialOffTime = lineMapIterator->hourPowerOffTime - lineMapIterator->hourAccidentOffTime;
				//�˹�ͣ�����
				lineMapIterator->hourArtificialOffCount = lineMapIterator->hourPowerOffCount - lineMapIterator->hourAccidentOffCount;
				// ����ƻ������⣬������
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

			//�������ݵ���ʷ��
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

			//���������
			if (getFaultTolerance()->amITheCurrentMachine())
			{
				if(CPS_ORM_DirectExecuteSql(dbPoolHandle,insertSql.toStdString())>=0)
				{
					qDebug()<<"INSERT HOUR DATA SUCCESS";
				}	
			}

			//����
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
		}else if (type=="day")//������ռ�ͳ��
		{
			//���������������
			if (lineMapIterator->lastPowerOnTime!="")
			{
				//�����ʱ��·�򿪹����͵�״̬
				if (lineMapIterator->isPowerOn)
				{
					int lastToCurrent = QDateTime::fromString(lineMapIterator->lastPowerOnTime, timeFormat).secsTo(currentDayStartDate);//��·�򿪹��ϴ��͵絽��ǰ����ʱ�������

					//���ʱ������3600*24s��1��
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

				//��·�򿪹�ͣ��ʱ��
				lineMapIterator->dayPowerOffTime = 3600*24 - lineMapIterator->dayPowerOnTime;

				//�����ʱ��·�򿪹��Ƕϵ�״̬
				if (!lineMapIterator->isPowerOn)
				{
					//����ǹ��϶ϵ�
					if (lineMapIterator->isAccidentOff)
					{
						int lastToCurrent = QDateTime::fromString(lineMapIterator->lastAccidentOffTime, timeFormat).secsTo(currentDayStartDate);//��·�򿪹��ϴ��͵絽�������ʱ�������

						//���ʱ������3600*24s��1��
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

				//�˹�ͣ��ʱ��
				lineMapIterator->dayArtificialOffTime = lineMapIterator->dayPowerOffTime - lineMapIterator->dayAccidentOffTime;
				//�˹�ͣ�����
				lineMapIterator->dayArtificialOffCount = lineMapIterator->dayPowerOffCount - lineMapIterator->dayAccidentOffCount;
				// ����ƻ������⣬������
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

			//�������ݵ���ʷ��
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

			//���������
			if (getFaultTolerance()->amITheCurrentMachine())
			{
				if(CPS_ORM_DirectExecuteSql(dbPoolHandle,insertSql.toStdString())>=0)
				{
					qDebug()<<"INSERT DAY DATA SUCCESS";
				}	
			}

			//����
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

//��ʱ��Ԫ�ؽ���
QString PSRStatisticsServer::createTableByDate(QString psrName, int index)
{
	//�����������ʱ��Ԫ��
	QDateTime dt = QDateTime::currentDateTime();
	QString timeFormat = "yyyy-MM-dd-hh-mm-ss";
	QString timeString = dt.toString(timeFormat);
	QStringList timeList = timeString.split("-");
	QString tableName = "H_"+psrName+"_"+timeList[0]+timeList[1];

	//��������ڣ����½���
	if (CPS_ORM_ExistTable(dbPoolHandle, tableName.toStdString())==0)
	{
		ORMColumnDescVector m_descVector;
		if(index==0)//��������-��բͳ�Ʊ�
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
		}else if(index==1){//����ĸ��ͳ�Ʊ�sql
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
		}else if(index==2){//������ѹ��ͳ�Ʊ�sql
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
		}else if(index==3){//������·ͳ�Ʊ�sql
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

		//���������
		if (getFaultTolerance()->amITheCurrentMachine())
		{
			if(CPS_ORM_CreateTable(dbPoolHandle, tableName.toStdString(), m_descVector)==1)
			{
				qDebug()<<"CreateTable:"+tableName;

				// ��������
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


//��ʼ������-��բ������Ϣ
void PSRStatisticsServer::initSwitchDataInfo()
{
	//���ø��º󣬲�����Ӧ�ã������ڴ�
	switchsTemp = switchs;
	switchs.clear();

	//��ʼ����·��PMSBreaker������Ϣ
	initSwitchCommonInfo(OT_PMSBreaker, 1);

	//��ʼ����բDisconnector������Ϣ
	initSwitchCommonInfo(OT_Disconnector, 2);

	//��ʼ�����ɿ���LoadBreakSwitch������Ϣ
	initSwitchCommonInfo(OT_LoadBreakSwitch, 3);

	//��ʼ��ֱ�������豸DCSwitch������Ϣ
	initSwitchCommonInfo(OT_DCSwitch, 4);

	//ѭ���������п���-��բ, ���ø��º󣬲�����Ӧ�ã������ڴ�********************
	QMap<ObId, Switch>::iterator switchMapIterator;
	for (switchMapIterator=switchs.begin();switchMapIterator!=switchs.end();switchMapIterator++)
	{
		ObId switchObId = switchMapIterator->obId;
		//�����ʱ�洢�У��������º��obid����ԭ�ڴ��е�ͳ������ԭ�����������ύ����
		if (switchsTemp.contains(switchObId))
		{
			//�������˴������ύǰ������������
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

			//����˴������ύ�����п���DPS�㣬��ָ��ύǰͳ�������Ϣ
			if (switchMapIterator->relatePoints.contains("CB_DPSPoint")&&switchsTemp[switchObId].relatePoints.contains("CB_DPSPoint"))
			{
				switchMapIterator->dayAccidentTripCount = switchsTemp[switchObId].dayAccidentTripCount;
				switchMapIterator->dayRunTime = switchsTemp[switchObId].dayRunTime;

				switchMapIterator->hourAccidentTripCount = switchsTemp[switchObId].hourAccidentTripCount;
				switchMapIterator->hourRunTime = switchsTemp[switchObId].hourRunTime;

				switchMapIterator->lastSwitchOnTime = switchsTemp[switchObId].lastSwitchOnTime;
			}

			//����˴������ύ������DPC���ز�������ͳ�Ƶ㣬��ָ��ύǰͳ�������Ϣ
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

	//�����ʱ�洢�ռ�
	switchsTemp.clear();
}

//��ʼ�����ص�բ-��������
void PSRStatisticsServer::initSwitchCommonInfo(OType oType, int type)
{
	int num = m_OMSDatabase->find(oType, NULL, 0);
	if (num>0)
	{
		// ��ȡ���п��ص�բ�ڵ��ObId
		ObId *obIds = new ObId[num];
		m_OMSDatabase->find(oType, NULL, 0, obIds, num);
		
		ContainerData childrenListData;
		for (int i=0;i<num;i++)
		{
			//�жϵ�ǰһ���豸���Ƿ���Ҫ���
			bool isNeededInsert = false;

			//���忪�ص�բͳ�Ƶ�
			Switch curSwitch;

			//��ʼ��ͳ�Ƶ�obid ������
			curSwitch.obId = obIds[i];
			curSwitch.otype = type;

			//��ȡp���ص�բ���ӽڵ�
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

					//��ȡң�ŵ���Ϣ
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
						
						if ((OMString)keyNameData=="CB"||(OMString)keyNameData=="DS")//����ǿ��ػ����ǵ�բ
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
							
							//����������ӵ�DPS�㣬��洢
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

								// ������ش�������״̬...
								if ((int)stateData==1)
								{
									//���ش��ڿ���״̬
									curSwitch.isSwitchOn = true;
								}
								//qDebug()<<(ObId)measLinkData;

								//��ȡң�ص�obid
								try
								{
									m_OMSDatabase->read((ObId)measLinkData, AT_DPCPointLink, &dpcPointLinkData);
								}
								catch (Exception& e)
								{
									LOG4CPLUS_ERROR(psrStatisticsServerLog, "error: read error!!!"+QString::number((ObId)measLinkData).toStdString());
									continue;
								}

								//����洢������ң�ص㣬��洢
								if ((ObId)dpcPointLinkData!=0)
								{
									curSwitch.relatePoints.insert("CB_DPCPoint", (ObId)dpcPointLinkData);
								}
							}
						}
					}

					//��ȡң�����Ϣ,���Ȼ�ȡ��������
					if (OT_PMSTerminal==sonOType)
					{
						//��ʼ�����ص�բ����״̬�Լ�������Ϣ*********************************************************
						ChoiceData rtEnergizedData;//�Ƿ����
						try
						{
							m_OMSDatabase->read(childrenObId[i], AT_RTEnergized, &rtEnergizedData);

							//��ʼ���ϴ��͵�ʱ��
							curSwitch.lastPowerOnTime = QDateTime::currentDateTime().toString(timeFormat);

							//���������
							if ((int)rtEnergizedData==0)
							{
								curSwitch.isPowerOn = false;
							}
							
							curSwitch.relatePoints.insert(QString("%1").arg(pmsTerminalNum)+"_PMSTerminal",childrenObId[i]);
							pmsTerminalNum++;//����ÿ��ص�բ�����м���pmsTerminal
						}
						catch (Exception& e)
						{
							LOG4CPLUS_ERROR(psrStatisticsServerLog, "error: read error!!!"+QString::number(childrenObId[i]).toStdString());
							continue;
						}


						//��ʼ�����ص�բ������Ϣ*********************************************************
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

									//����������ӵ�MVPoint�㣬��洢
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

			//�ж��Ƿ���Ҫ��Ӹ�һ���豸��
			if (curSwitch.relatePoints.contains("0_PMSTerminal")&&curSwitch.relatePoints.contains("1_PMSTerminal"))//���������������
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

			//�洢pmsbreakerͳ�Ƶ㵽map��
			if (isNeededInsert)
			{
				switchs.insert(obIds[i], curSwitch);
			}
		}
		//�����ڴ�
		delete[] obIds;
	}
}

//��ʼ��ĸ��������Ϣ
void PSRStatisticsServer::initBusbarDataInfo()
{
	//���ø��º󣬲�����Ӧ�ã������ڴ�
	busbarsTemp = busbars;
	busbars.clear();

	//��ʼ��ĸ��PMSBusbar������Ϣ
	int pmsBusbarNum = m_OMSDatabase->find(OT_PMSBusbar, NULL, 0);
	if (pmsBusbarNum>0)
	{
		// ��ȡ����ĸ��PMSBusbar�ڵ��ObId
		ObId *pmsBusbarObIds = new ObId[pmsBusbarNum];
		m_OMSDatabase->find(OT_PMSBusbar, NULL, 0, pmsBusbarObIds, pmsBusbarNum);

		ContainerData childrenListData;
		for (int i=0;i<pmsBusbarNum;i++)
		{
			//�жϵ�ǰһ���豸���Ƿ���Ҫ���
			bool isNeededInsert = false;

			//����ĸ��ͳ�Ƶ�
			Busbar busbar;

			//��ʼ��ͳ�Ƶ�obid ������
			busbar.obId = pmsBusbarObIds[i];
			busbar.otype = 1;//"PMSBusbar"

			//��ȡĸ�ߵ��ӽڵ�
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

					//��ȡң�����Ϣ,���Ȼ�ȡ��������
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

									//����������ӵ�MVPoint�㣬��洢
									if ((ObId)measLinkData!=0)
									{
										//�ж�Ϊ��Ӹ�����
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

			//�洢busbarͳ�Ƶ㵽map��
			if (isNeededInsert)
			{
				busbars.insert(pmsBusbarObIds[i], busbar);
			}
		}

		//�����ڴ�
		delete[] pmsBusbarObIds;
	}

	//ѭ����������ĸ��, ���ø��º󣬲�����Ӧ�ã������ڴ�
	QMap<ObId, Busbar>::iterator busbarMapIterator;
	for (busbarMapIterator=busbars.begin();busbarMapIterator!=busbars.end();busbarMapIterator++)
	{
		ObId busbarObId = busbarMapIterator->obId;
		//�����ʱ�洢�У��������º��obid����ԭ�ڴ��е�ͳ������ԭ
		if (busbarsTemp.contains(busbarObId))
		{
			busbarMapIterator->dayLimitCount = busbarsTemp[busbarObId].dayLimitCount;
			busbarMapIterator->hourLimitCount = busbarsTemp[busbarObId].hourLimitCount;
		}
	}

	//�����ʱ�洢�ռ�
	busbarsTemp.clear();
}

//��ʼ����ѹ��������Ϣ
void PSRStatisticsServer::initTransformerDataInfo()
{
	//���ø��º󣬲�����Ӧ�ã������ڴ�
	transformersTemp = transformers;
	transformers.clear();

	//��ʼ�������ѹ��PMSThreeWindingTransformer������Ϣ
	initTransformerCommonInfo(OT_PMSThreeWindingTransformer, 1);

	//��ʼ�������ѹ��PMSDoubleWindingTransformer������Ϣ
	initTransformerCommonInfo(OT_PMSDoubleWindingTransformer, 2);

	//��ʼ�����DistributionTransformer������Ϣ
	initTransformerCommonInfo(OT_DistributionTransformer, 3);

	//ѭ���������б�ѹ��, ���ø��º󣬲�����Ӧ�ã������ڴ�
	QMap<ObId, Transformer>::iterator transformerMapIterator;
	for (transformerMapIterator=transformers.begin();transformerMapIterator!=transformers.end();transformerMapIterator++)
	{
		ObId transformerObId = transformerMapIterator->obId;
		//�����ʱ�洢�У��������º��obid����ԭ�ڴ��е�ͳ������ԭ
		if (transformersTemp.contains(transformerObId))
		{
			//����˴������ύ�����е�����������ͳ�Ƶ㣬��ָ��ύǰͳ�������Ϣ
			if (transformerMapIterator->relatePoints.contains("TapPos_MVPoint"))
			{
				transformerMapIterator->dayShiftCount = transformersTemp[transformerObId].dayShiftCount;
				transformerMapIterator->hourShiftCount = transformersTemp[transformerObId].hourShiftCount;
			}

			transformerMapIterator->dayProtectionsCount = transformersTemp[transformerObId].dayProtectionsCount;
			transformerMapIterator->hourProtectionsCount = transformersTemp[transformerObId].hourProtectionsCount;
		}
	}

	//�����ʱ�洢�ռ�
	transformersTemp.clear();
}

//��ʼ����ѹ��-��������
void PSRStatisticsServer::initTransformerCommonInfo(OType oType, int type)
{
	//��ʼ����ѹ��������Ϣ
	int num = m_OMSDatabase->find(oType, NULL, 0);
	if (num>0)
	{
		// ��ȡ���б�ѹ���ڵ��ObId
		ObId *obIds = new ObId[num];
		m_OMSDatabase->find(oType, NULL, 0, obIds, num);

		ContainerData childrenListData;
		for (int i=0;i<num;i++)
		{
			//�жϵ�ǰһ���豸���Ƿ���Ҫ���
			bool isNeededInsert = false;

			//�����ѹ��ͳ�Ƶ�
			Transformer transformer;

			//��ʼ��ͳ�Ƶ�obid ������
			transformer.obId = obIds[i];
			transformer.otype = type;

			//��ȡ��ѹ�����ӽڵ�
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

					//��ȡң�����Ϣ,���Ȼ�ȡ��ѹ��������
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

								//��ȡң�����Ϣ,���Ȼ�ȡ��������
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

												//����������ӵ�MVPoint�㣬��洢
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

			// �����Ҫ��Ӹñ�ѹ��,��������ʱ�������ʲ�����������ж�
			if (transformer.relatePoints.contains("TapPos_MVPoint"))
			{
				isNeededInsert = true;
			}
			//�洢��ѹ��ͳ�Ƶ㵽map��
			if (isNeededInsert)
			{
				transformers.insert(obIds[i], transformer);
			}
		}

		//�����ڴ�
		delete[] obIds;
	}
}

//��ʼ����·������Ϣ
void PSRStatisticsServer::initLineDataInfo()
{
	//���ø��º󣬲�����Ӧ�ã������ڴ�
	linesTemp = lines;
	lines.clear();

	//��ʼ����·Feeder������Ϣ
	LinkData measurementTypeLinkData;
	LinkData measLinkData;
	StringData keyNameData;
	IntegerData breakerTypeData;
	FloatData valuesData;

	int feederNum = m_OMSDatabase->find(OT_Feeder, NULL, 0);
	if (feederNum>0)
	{
		// ��ȡ������·Feeder�ڵ��ObId
		ObId *feederObIds = new ObId[feederNum];
		m_OMSDatabase->find(OT_Feeder, NULL, 0, feederObIds, feederNum);

		ContainerData childrenListData;
		for (int i=0;i<feederNum;i++)
		{
			//�жϵ�ǰһ���豸���Ƿ���Ҫ���
			bool isNeededInsert = false;

			//������·�ڵ�
			Line line;

			//��ʼ��ͳ�Ƶ�obid ������
			line.obId = feederObIds[i];
			line.otype = 1;//"Feeder"

			//��ȡ��·���ӽڵ�
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

					//��ȡң�����Ϣ,���Ȼ�ȡPMSBreaker���߿�������
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
						
						//����ǳ��߿�����洢
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

									//��ȡң�����Ϣ,���Ȼ�ȡ��������
									if (OT_PMSTerminal==sonOType)
									{
										//��ʼ�����ص�բ����״̬�Լ�������Ϣ*********************************************************
										ChoiceData rtEnergizedData;//�Ƿ����
										try
										{
											m_OMSDatabase->read(childrenObId[k], AT_RTEnergized, &rtEnergizedData);

											//��ʼ���ϴ��͵�ʱ��
											QString timeFormat = "yyyy-MM-dd-hh-mm-ss";
											line.lastPowerOnTime = QDateTime::currentDateTime().toString(timeFormat);

											//���������
											if ((int)rtEnergizedData==0)
											{
												line.isPowerOn = false;
											}

											line.relatePoints.insert(QString("%1").arg(pmsTerminalNum)+"_PMSTerminal",childrenObId[k]);
											pmsTerminalNum++;//����ÿ��ص�բ�����м���pmsTerminal
										}
										catch (Exception& e)
										{
											LOG4CPLUS_ERROR(psrStatisticsServerLog, "error: read error!!!"+QString::number(childrenObId[k]).toStdString());
											continue;
										}

										//��ʼ����·������Ϣ**************************************
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

													//����������ӵ�MVPoint�㣬��洢
													if ((ObId)measLinkData!=0)
													{
														if ((OMString)keyNameData=="P")
														{
															QString timeFormat = "yyyy-MM-dd-hh-mm-ss";
															QDateTime currentTime = QDateTime::currentDateTime();

															line.relatePoints.insert("P_MVPoint",(ObId)measLinkData);

															//��ʼ���������ֵ����Сֵ��ƽ��ֵ����ʱ���Լ����һ�εĸ���ֵ
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

			//�����Ҫ����
			if (line.relatePoints.contains("0_PMSTerminal")&&line.relatePoints.contains("1_PMSTerminal"))//���������������
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

			//�洢��·ͳ�Ƶ㵽map��
			if (isNeededInsert)
			{
				lines.insert(feederObIds[i], line);
			}
		}

		//�����ڴ�
		delete[] feederObIds;
	}

	//ѭ������������·, ���ø��º󣬲�����Ӧ�ã������ڴ�
	QMap<ObId, Line>::iterator lineMapIterator;
	for (lineMapIterator=lines.begin();lineMapIterator!=lines.end();lineMapIterator++)
	{
		ObId lineObId = lineMapIterator->obId;
		//�����ʱ�洢�У��������º��obid����ԭ�ڴ��е�ͳ������ԭ
		if (linesTemp.contains(lineObId))
		{
			//�������˴������ύǰ������������
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

			//����˴������ύ�����и���ͳ�Ƶ㣬��ָ��ύǰͳ�������Ϣ
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

	//�����ʱ�洢�ռ�
	linesTemp.clear();
}

//��ȡ���ֵ
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

//��ȡ��Сֵ
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