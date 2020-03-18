#pragma once

#include "common.h"

class Switch
{
public:
	ObId obId;
	int otype;
	QMap<QString,ObId> relatePoints;//�洢������MVPoint�㡢DPCPoint��DPSPoint���ObId
	QString statisticDate;//��¼ͳ��ʱ��

	float threePUnbalanceRate;//�����й���ƽ����
	float threeQUnbalanceRate;//�����޹���ƽ����

	long hourOptCount;//ʱ�ۼƲ�������
	long dayOptCount;//���ۼƲ�������

	long hourAccidentTripCount;//ʱ�ۼƹ��ϴ���
	long dayAccidentTripCount;//���ۼƹ��ϴ���

	QString lastSwitchOnTime;//���һ�ο��رպ�ʱ��
	bool isSwitchOn;//�����Ƿ�պ�
	long hourRunTime;//ʱ�ۼ�����ʱ��
	long dayRunTime;//���ۼ�����ʱ��

	long hourReplaceCount;//ʱ�ۼ��������
	long dayReplaceCount;//���ۼ��������
	QString replaceTime;//���ʱ��

	//2017.01.20���� begin
	long hourPowerOnCount;//ʱ�͵����
	long dayPowerOnCount;//���͵����

	long hourPowerOffCount;//ʱ��ͣ�����
	long dayPowerOffCount;//����ͣ�����

	QString lastPowerOnTime;//���һ���͵�ʱ��
	bool isPowerOn;//�����Ƿ��͵�
	long hourPowerOnTime;//ʱ�ۼ��͵�ʱ��
	long dayPowerOnTime;//���ۼ��͵�ʱ��
	long hourPowerOffTime;//ʱ�ۼ�ͣ��ʱ��
	long dayPowerOffTime;//���ۼ�ͣ��ʱ��
	//2017.01.20���� end

	//2017.03.14 ����begin
	long hourAccidentOffCount;//ʱ����ͣ�����
	long dayAccidentOffCount;//�չ���ͣ�����

	long hourArtificialOffCount;//ʱ�˹�ͣ�����
	long dayArtificialOffCount;//���˹�ͣ�����

	long hourAccidentOffTime;//ʱ����ͣ��ʱ��
	long dayAccidentOffTime;//�չ���ͣ��ʱ��

	long hourArtificialOffTime;//ʱ�˹�ͣ��ʱ��
	long dayArtificialOffTime;//���˹�ͣ��ʱ��

	QString lastAccidentOffTime;//���һ�ι���ͣ��ʱ��
	bool isAccidentOff;//�����Ƿ��ڹ���ͣ��״̬
	//2017.03.14 ����end

	//2017.08.22 ����begin
	long hourProtectionsCount;//ʱ�ۼ� ������������
	long dayProtectionsCount;//���ۼƱ�����������
	//2017.08.22 ����end

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

		//2017.01.20���� begin
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
		//2017.01.20���� end

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
	QMap<QString,ObId> relatePoints;//�洢������MVPoint���ObId
	QString statisticDate;//��¼ͳ��ʱ��
	float threeVUnbalanceRate;//�����ѹ��ƽ����
	long hourLimitCount;//ʱ�ۼ�Խ�޴���
	long dayLimitCount;//���ۼ�Խ�޴���

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
	QMap<QString,ObId> relatePoints;//�洢������MVPoint���ObId
	QString statisticDate;//��¼ͳ��ʱ��
	long hourShiftCount;//ʱ�ۼƵ�������
	long dayShiftCount;//���ۼƵ�������
	float loadRate;//������

	//2017.08.22 ����begin
	long hourProtectionsCount;//ʱ�ۼ� ������������
	long dayProtectionsCount;//���ۼƱ�����������
	//2017.08.22 ����end

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
	QMap<QString,ObId> relatePoints;//�洢������MVPoint���ObId
	QString statisticDate;//��¼ͳ��ʱ��
	float threeVUnbalanceRate;//�����ѹ��ƽ����
	float loadRate;//������
	float latestLoad;//���һ�θ���ֵ
	float hourSumLoad;//ʱ���ؼӺ�
	long  hourLoadChangeCount;//ʱ���ر仯����
	float hourAvgLoad;//ʱ����ƽ��ֵ
	float hourMaxLoad;//ʱ�������ֵ
	QString hourMaxLoadTime;//ʱ�������ֵʱ��
	float hourMinLoad;//ʱ������Сֵ
	QString hourMinLoadTime;//ʱ������Сֵʱ��

	float daySumLoad;//�ո��ؼӺ�
	long  dayLoadChangeCount;//�ո��ر仯����
	float dayAvgLoad;//�ո���ƽ��ֵ
	float dayMaxLoad;//�ո������ֵ
	QString dayMaxLoadTime;//�ո������ֵʱ��
	float dayMinLoad;//�ո�����Сֵ
	QString dayMinLoadTime;//�ո�����Сֵʱ��

	//2017.01.20���� begin
	long hourPowerOnCount;//ʱ�͵����
	long dayPowerOnCount;//���͵����

	long hourPowerOffCount;//ʱͣ�����
	long dayPowerOffCount;//��ͣ�����

	QString lastPowerOnTime;//���һ���͵�ʱ��
	bool isPowerOn;//�����Ƿ��͵�
	long hourPowerOnTime;//ʱ�ۼ��͵�ʱ��
	long dayPowerOnTime;//���ۼ��͵�ʱ��
	long hourPowerOffTime;//ʱ�ۼ�ͣ��ʱ��
	long dayPowerOffTime;//���ۼ�ͣ��ʱ��
	//2017.01.20���� end

	//2017.03.14 ����begin
	long hourAccidentOffCount;//ʱ����ͣ�����
	long dayAccidentOffCount;//�չ���ͣ�����

	long hourArtificialOffCount;//ʱ�˹�ͣ�����
	long dayArtificialOffCount;//���˹�ͣ�����

	long hourAccidentOffTime;//ʱ����ͣ��ʱ��
	long dayAccidentOffTime;//�չ���ͣ��ʱ��

	long hourArtificialOffTime;//ʱ�˹�ͣ��ʱ��
	long dayArtificialOffTime;//���˹�ͣ��ʱ��

	QString lastAccidentOffTime;//���һ�ι���ͣ��ʱ��
	bool isAccidentOff;//�����Ƿ��ڹ���ͣ��״̬
	//2017.03.14 ����end

	//2017.07.25����begin
	long hourFeederActionTimes;//ʱ���߶�������
	long dayFeederActionTimes;//�����߶�������

	long hourFeederSuccessTimes;//ʱ���߶����ɹ�����
	long dayFeederSuccessTimes;//�����߶����ɹ�����

	long hourFeederFaultTimes;//ʱ���߶���ʧ�ܴ���
	long dayFeederFaultTimes;//�����߶���ʧ�ܴ���
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

		//2017.01.20���� begin
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
		//2017.01.20���� end

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

		//2017.07.25����begin
		this->hourFeederActionTimes = 0;
		this->dayFeederActionTimes = 0;
		this->hourFeederSuccessTimes = 0;
		this->dayFeederSuccessTimes = 0;
		this->hourFeederFaultTimes = 0;
		this->dayFeederFaultTimes = 0;
		//2017.07.25end
	}
};

