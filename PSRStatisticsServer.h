#ifndef PSRSTATISTICSSERVER_H
#define PSRSTATISTICSSERVER_H

#include "common.h"
#include "NotificationCallback.h"
#include "DataModel.h"
#include "IncrementCommit.h"

class PSRStatisticsServer : public QtApplication

{
public:
	QMap<ObId, Switch> switchs;//开关-刀闸Map
	QMap<ObId, Busbar> busbars;//母线Map
	QMap<ObId, Transformer> transformers;//变压器Map
	QMap<ObId, Line> lines;//线路Map

	QMap<ObId, Switch> switchsTemp;//开关-刀闸Map临时，为配置库更新时，做临时存储
	QMap<ObId, Busbar> busbarsTemp;//母线Map，为配置库更新时，做临时存储
	QMap<ObId, Transformer> transformersTemp;//变压器Map，为配置库更新时，做临时存储
	QMap<ObId, Line> linesTemp;//线路Map，为配置库更新时，做临时存储

	QList<ObId> faultMeasureTypeObIds;//存储所有事故MeasureType类型obid

	//构造函数
	PSRStatisticsServer(int &argc, char **argv,
		SignalHandler sigtermHandler,
		const OptionList& optionList,
		EnvironmentScope environmentScope);

	//析构函数
	~PSRStatisticsServer();

	//初始化故障信号量测类型
	void initFaultMeasureType();

	//初始化数据结构
	void initDataInfo();

	//初始化注册回调函数
	void initNotificationCallback();

	//统计开关-刀闸
	void statisticSwitch(QString psrName, QString type);

	//统计母线
	void statisticBusbar(QString psrName, QString type);

	//统计变压器
	void statisticTransformer(QString psrName, QString type);

	//统计线路
	void statisticLine(QString psrName, QString type);

private:
	//以时间元素建表
	QString createTableByDate(QString psrName, int index);

	//初始化开关-刀闸数据信息
	void initSwitchDataInfo();

	//初始化开关刀闸-公共部分
	void initSwitchCommonInfo(OType oType, int type);

	//初始化母线数据信息
	void initBusbarDataInfo();

	//初始化变压器数据信息
	void initTransformerDataInfo();

	//初始化变压器-公共部分
	void initTransformerCommonInfo(OType oType, int type);

	//初始化线路数据信息
	void initLineDataInfo();

	//获取最大值
	float getMax(float a, float b, float c);

	//获取最小值
	float getMin(float a, float b, float c);
};


#endif