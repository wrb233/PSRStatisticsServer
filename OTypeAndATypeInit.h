#ifndef OTYPEANDATYPEINIT_H
#define OTYPEANDATYPEINIT_H

#include "common.h"

extern AType AT_BreakerType;
extern AType AT_ChildrenList;
extern AType AT_CommitSyncEnd;
extern AType AT_ControlResult;
extern AType AT_DPCPointLink;
extern AType AT_DPSPointLink;
extern AType AT_FaultState;
extern AType AT_KeyName;
extern AType AT_Limit;
extern AType AT_MeasLink;
extern AType AT_MeasurementTypeLink;
extern AType AT_MVPointLink;
extern AType AT_ObjOrder;
extern AType AT_ParamMva;
extern AType AT_ParentLink;
extern AType AT_PSRLink;
extern AType AT_RTEnergized;
extern AType AT_State;
extern AType AT_Value;

extern OType OT_Analog;
extern OType OT_BSCPoint;
extern OType OT_DCSwitch;
extern OType OT_Disconnector;
extern OType OT_Discrete;
extern OType OT_DistributionTransformer;
extern OType OT_DPCPoint;
extern OType OT_DPSPoint;
extern OType OT_Feeder;
extern OType OT_IncrementCommit;
extern OType OT_LoadBreakSwitch;
extern OType OT_MeasurementType;
extern OType OT_MVPoint;
extern OType OT_PMSBreaker;
extern OType OT_PMSBusbar;
extern OType OT_PMSDoubleWindingTransformer;
extern OType OT_PMSTerminal;
extern OType OT_PMSThreeWindingTransformer;
extern OType OT_TransformerWinding;

bool initOTypeAndAType();

#endif