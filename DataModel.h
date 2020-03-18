#pragma once

#include "common.h"

class Switch
{
public:
	ObId obId;
	int otype;
	QMap<QString,ObId> relatePoints;//存储关联的MVPoint点、DPCPoint、DPSPoint点的ObId
	QString statisticDate;//记录统计时间

	float threePUnbalanceRate;//三相有功不平衡率
	float threeQUnbalanceRate;//三相无功不平衡率

	long hourOptCount;//时累计操作次数
	long dayOptCount;//日累计操作次数

	long hourAccidentTripCount;//时累计故障次数
	long dayAccidentTripCount;//日累计故障次数

	QString lastSwitchOnTime;//最近一次开关闭合时间
	bool isSwitchOn;//开关是否闭合
	long hourRunTime;//时累计运行时长
	long dayRunTime;//日累计运行时长

	long hourReplaceCount;//时累计替代次数
	long dayReplaceCount;//日累计替代次数
	QString replaceTime;//替代时间

	//2017.01.20新增 begin
	long hourPowerOnCount;//时送电次数
	long dayPowerOnCount;//日送电次数

	long hourPowerOffCount;//时总停电次数
	long dayPowerOffCount;//日总停电次数

	QString lastPowerOnTime;//最近一次送电时间
	bool isPowerOn;//开关是否送电
	long hourPowerOnTime;//时累计送电时长
	long dayPowerOnTime;//日累计送电时长
	long hourPowerOffTime;//时累计停电时长
	long dayPowerOffTime;//日累计停电时长
	//2017.01.20新增 end

	//2017.03.14 新增begin
	long hourAccidentOffCount;//时故障停电次数
	long dayAccidentOffCount;//日故障停电次数

	long hourArtificialOffCount;//时人工停电次数
	long dayArtificialOffCount;//日人工停电次数

	long hourAccidentOffTime;//时故障停电时长
	long dayAccidentOffTime;//日故障停电时长

	long hourArtificialOffTime;//时人工停电时长
	long dayArtificialOffTime;//日人工停电时长

	QString lastAccidentOffTime;//最近一次故障停电时间
	bool isAccidentOff;//开关是否处于故障停电状态
	//2017.03.14 新增end

	//2017.08.22 新增begin
	long hourProtectionsCount;//时累计 保护动作次数
	long dayProtectionsCount;//日累计保护动作次数
	//2017.08.22 新增end

	Switch()
	{
		this->obId = 0;
		this->otype = 0;
		this->statisticDate = "";
		this->threePUnbalanceRate = 0.0;
		this->threeQUnbalanceRate = 0.0;
		this->hourOptCount = 0;
		this->dayOptCount = 0;
		this->hourAccidentTripCount = 0;
		this->dayAccidentTripCount = 0;
		this->lastSwitchOnTime = "";
		this->isSwitchOn = false;
		this->hourRunTime = 0;
		this->dayRunTime = 0;
		this->hourReplaceCount = 0;
		this->dayReplaceCount = 0;
		this->replaceTime = "";

		//2017.01.20新增 begin
		this->hourPowerOnCount = 0;
		this->dayPowerOnCount = 0;
		this->hourPowerOffCount = 0;
		this->dayPowerOffCount = 0;
		this->lastPowerOnTime = "";
		this->isPowerOn = true;
		this->hourPowerOnTime = 0;
		this->dayPowerOnTime = 0;
		this->hourPowerOffTime = 0;
		this->dayPowerOffTime = 0;
		//2017.01.20新增 end

		//2017.03.14 begin
		this->hourAccidentOffCount = 0;
		this->dayAccidentOffCount = 0;
		this->hourArtificialOffCount = 0;
		this->dayArtificialOffCount = 0;
		this->hourAccidentOffTime = 0;
		this->dayAccidentOffTime = 0;
		this->hourArtificialOffTime = 0;
		this->dayArtificialOffTime = 0;
		this->lastAccidentOffTime = "";
		this->isAccidentOff = false;
		//2017.03.14 end

		//2017.08.22 begin
		this->hourProtectionsCount = 0;
		this->dayProtectionsCount = 0;
		//2017.08.22 end
	}
};

class Busbar
{
public:
	ObId obId;
	int otype;
	QMap<QString,ObId> relatePoints;//存储关联的MVPoint点的ObId
	QString statisticDate;//记录统计时间
	float threeVUnbalanceRate;//三相电压不平衡率
	long hourLimitCount;//时累计越限次数
	long dayLimitCount;//日累计越限次数

	Busbar()
	{
		this->obId = 0;
		this->otype = 0;
		this->statisticDate = "";
		this->threeVUnbalanceRate = 0.0;
		this->hourLimitCount = 0;
		this->dayLimitCount = 0;
	}
};

class Transformer
{
public:
	ObId obId;
	int otype;
	QMap<QString,ObId> relatePoints;//存储关联的MVPoint点的ObId
	QString statisticDate;//记录统计时间
	long hourShiftCount;//时累计调档次数
	long dayShiftCount;//日累计调档次数
	float loadRate;//负载率

	//2017.08.22 新增begin
	long hourProtectionsCount;//时累计 保护动作次数
	long dayProtectionsCount;//日累计保护动作次数
	//2017.08.22 新增end

	Transformer()
	{
		this->obId = 0;
		this->otype = 0;
		this->hourShiftCount = 0;
		this->dayShiftCount = 0;
		this->loadRate = 0.0;

		//2017.08.22 begin
		this->hourProtectionsCount = 0;
		this->dayProtectionsCount = 0;
		//2017.08.22 end
	}
};

class Line
{
public:
	ObId obId;
	int otype;
	QMap<QString,ObId> relatePoints;//存储关联的MVPoint点的ObId
	QString statisticDate;//记录统计时间
	float threeVUnbalanceRate;//三相电压不平衡率
	float loadRate;//负载率
	float latestLoad;//最近一次负载值
	float hourSumLoad;//时负载加和
	long  hourLoadChangeCount;//时负载变化次数
	float hourAvgLoad;//时负载平均值
	float hourMaxLoad;//时负载最大值
	QString hourMaxLoadTime;//时负载最大值时间
	float hourMinLoad;//时负载最小值
	QString hourMinLoadTime;//时负载最小值时间

	float daySumLoad;//日负载加和
	long  dayLoadChangeCount;//日负载变化次数
	float dayAvgLoad;//日负载平均值
	float dayMaxLoad;//日负载最大值
	QString dayMaxLoadTime;//日负载最大值时间
	float dayMinLoad;//日负载最小值
	QString dayMinLoadTime;//日负载最小值时间

	//2017.01.20新增 begin
	long hourPowerOnCount;//时送电次数
	long dayPowerOnCount;//日送电次数

	long hourPowerOffCount;//时停电次数
	long dayPowerOffCount;//日停电次数

	QString lastPowerOnTime;//最近一次送电时间
	bool isPowerOn;//开关是否送电
	long hourPowerOnTime;//时累计送电时长
	long dayPowerOnTime;//日累计送电时长
	long hourPowerOffTime;//时累计停电时长
	long dayPowerOffTime;//日累计停电时长
	//2017.01.20新增 end

	//2017.03.14 新增begin
	long hourAccidentOffCount;//时故障停电次数
	long dayAccidentOffCount;//日故障停电次数

	long hourArtificialOffCount;//时人工停电次数
	long dayArtificialOffCount;//日人工停电次数

	long hourAccidentOffTime;//时故障停电时长
	long dayAccidentOffTime;//日故障停电时长

	long hourArtificialOffTime;//时人工停电时长
	long dayArtificialOffTime;//日人工停电时长

	QString lastAccidentOffTime;//最近一次故障停电时间
	bool isAccidentOff;//开关是否处于故障停电状态
	//2017.03.14 新增end

	//2017.07.25新增begin
	long hourFeederActionTimes;//时馈线动作次数
	long dayFeederActionTimes;//日馈线动作次数

	long hourFeederSuccessTimes;//时馈线动作成功次数
	long dayFeederSuccessTimes;//日馈线动作成功次数

	long hourFeederFaultTimes;//时馈线动作失败次数
	long dayFeederFaultTimes;//日馈线动作失败次数
	//2017.07.25end

	Line()
	{
		this->obId = 0;
		this->otype = 0;
		this->statisticDate = "";
		this->threeVUnbalanceRate = 0.0;
		this->loadRate = 0.0;
		this->latestLoad = 0.0;
		this->hourSumLoad = 0.0;
		this->hourLoadChangeCount = 0;
		this->hourAvgLoad = 0.0;
		this->hourMaxLoad = 0.0;
		this->hourMaxLoadTime = "";
		this->hourMinLoad = 0.0;
		this->hourMinLoadTime = "";
		this->daySumLoad = 0.0;
		this->dayLoadChangeCount = 0;
		this->dayAvgLoad = 0.0;
		this->dayMaxLoad = 0.0;
		this->dayMaxLoadTime = "";
		this->dayMinLoad = 0.0;
		this->dayMaxLoadTime = "";

		//2017.01.20新增 begin
		this->hourPowerOnCount = 0;
		this->dayPowerOnCount = 0;
		this->hourPowerOffCount = 0;
		this->dayPowerOffCount = 0;
		this->lastPowerOnTime = "";
		this->isPowerOn = true;
		this->hourPowerOnTime = 0;
		this->dayPowerOnTime = 0;
		this->hourPowerOffTime = 0;
		this->dayPowerOffTime = 0;
		//2017.01.20新增 end

		//2017.03.14 begin
		this->hourAccidentOffCount = 0;
		this->dayAccidentOffCount = 0;
		this->hourArtificialOffCount = 0;
		this->dayArtificialOffCount = 0;
		this->hourAccidentOffTime = 0;
		this->dayAccidentOffTime = 0;
		this->hourArtificialOffTime = 0;
		this->dayArtificialOffTime = 0;
		this->lastAccidentOffTime = "";
		this->isAccidentOff = false;
		//2017.03.14 end

		//2017.07.25新增begin
		this->hourFeederActionTimes = 0;
		this->dayFeederActionTimes = 0;
		this->hourFeederSuccessTimes = 0;
		this->dayFeederSuccessTimes = 0;
		this->hourFeederFaultTimes = 0;
		this->dayFeederFaultTimes = 0;
		//2017.07.25end
	}
};

