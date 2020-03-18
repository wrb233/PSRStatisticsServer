#ifndef PSRSTATISTICSSERVER_H
#define PSRSTATISTICSSERVER_H

#include "common.h"
#include "NotificationCallback.h"
#include "DataModel.h"
#include "IncrementCommit.h"

class PSRStatisticsServer : public QtApplication

{
public:
	QMap<ObId, Switch> switchs;//����-��բMap
	QMap<ObId, Busbar> busbars;//ĸ��Map
	QMap<ObId, Transformer> transformers;//��ѹ��Map
	QMap<ObId, Line> lines;//��·Map

	QMap<ObId, Switch> switchsTemp;//����-��բMap��ʱ��Ϊ���ÿ����ʱ������ʱ�洢
	QMap<ObId, Busbar> busbarsTemp;//ĸ��Map��Ϊ���ÿ����ʱ������ʱ�洢
	QMap<ObId, Transformer> transformersTemp;//��ѹ��Map��Ϊ���ÿ����ʱ������ʱ�洢
	QMap<ObId, Line> linesTemp;//��·Map��Ϊ���ÿ����ʱ������ʱ�洢

	QList<ObId> faultMeasureTypeObIds;//�洢�����¹�MeasureType����obid

	//���캯��
	PSRStatisticsServer(int &argc, char **argv,
		SignalHandler sigtermHandler,
		const OptionList& optionList,
		EnvironmentScope environmentScope);

	//��������
	~PSRStatisticsServer();

	//��ʼ�������ź���������
	void initFaultMeasureType();

	//��ʼ�����ݽṹ
	void initDataInfo();

	//��ʼ��ע��ص�����
	void initNotificationCallback();

	//ͳ�ƿ���-��բ
	void statisticSwitch(QString psrName, QString type);

	//ͳ��ĸ��
	void statisticBusbar(QString psrName, QString type);

	//ͳ�Ʊ�ѹ��
	void statisticTransformer(QString psrName, QString type);

	//ͳ����·
	void statisticLine(QString psrName, QString type);

private:
	//��ʱ��Ԫ�ؽ���
	QString createTableByDate(QString psrName, int index);

	//��ʼ������-��բ������Ϣ
	void initSwitchDataInfo();

	//��ʼ�����ص�բ-��������
	void initSwitchCommonInfo(OType oType, int type);

	//��ʼ��ĸ��������Ϣ
	void initBusbarDataInfo();

	//��ʼ����ѹ��������Ϣ
	void initTransformerDataInfo();

	//��ʼ����ѹ��-��������
	void initTransformerCommonInfo(OType oType, int type);

	//��ʼ����·������Ϣ
	void initLineDataInfo();

	//��ȡ���ֵ
	float getMax(float a, float b, float c);

	//��ȡ��Сֵ
	float getMin(float a, float b, float c);
};


#endif