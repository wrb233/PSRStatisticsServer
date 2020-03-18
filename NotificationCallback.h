#ifndef NOTIFICATIONCALLBACK_H
#define NOTIFICATIONCALLBACK_H

#include "common.h"
#include "PSRStatisticsServer.h"

void notificationSwitchOptCount(const Notification* notif, void* clientdata);

void notificationSwitchRuntime(const Notification* notif, void* clientdata);

void notificationSwitchAccident(const Notification* notif, void* clientdata);

void notificationBusbarLimit(const Notification* notif, void* clientdata);

void notificationTransformerShift(const Notification* notif, void* clientdata);

//监听线路负荷即功率的值改变
void notificationLineValue(const Notification* notif, void* clientdata);

//增量更新时，回调重新初始化
void notificationIncrementCommit(const Notification* notif, void* clientdata);

//线路和开关带电状态是否变化的回调函数
void notificationLineAndSwitchRTEnergized(const Notification* notif, void* clientdata);

//线路（馈线）动作次数，成功失败次数的回调函数:使用ObjOrder与馈线自动化FA交互
void notificationFeederObjOrder(const Notification* notif, void* clientdata);


#endif