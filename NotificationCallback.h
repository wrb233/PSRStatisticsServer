#ifndef NOTIFICATIONCALLBACK_H
#define NOTIFICATIONCALLBACK_H

#include "common.h"
#include "PSRStatisticsServer.h"

void notificationSwitchOptCount(const Notification* notif, void* clientdata);

void notificationSwitchRuntime(const Notification* notif, void* clientdata);

void notificationSwitchAccident(const Notification* notif, void* clientdata);

void notificationBusbarLimit(const Notification* notif, void* clientdata);

void notificationTransformerShift(const Notification* notif, void* clientdata);

//������·���ɼ����ʵ�ֵ�ı�
void notificationLineValue(const Notification* notif, void* clientdata);

//��������ʱ���ص����³�ʼ��
void notificationIncrementCommit(const Notification* notif, void* clientdata);

//��·�Ϳ��ش���״̬�Ƿ�仯�Ļص�����
void notificationLineAndSwitchRTEnergized(const Notification* notif, void* clientdata);

//��·�����ߣ������������ɹ�ʧ�ܴ����Ļص�����:ʹ��ObjOrder�������Զ���FA����
void notificationFeederObjOrder(const Notification* notif, void* clientdata);


#endif