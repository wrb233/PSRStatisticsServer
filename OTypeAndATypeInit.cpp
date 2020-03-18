#include "OTypeAndATypeInit.h"

extern Database *m_OMSDatabase;//实时数据库指针外部全局变量

AType AT_BreakerType;
AType AT_ChildrenList;
AType AT_CommitSyncEnd;
AType AT_ControlResult;
AType AT_DPCPointLink;
AType AT_DPSPointLink;
AType AT_FaultState;
AType AT_KeyName;
AType AT_Limit;
AType AT_MeasLink;
AType AT_MeasurementTypeLink;
AType AT_MVPointLink;
AType AT_ObjOrder;
AType AT_ParamMva;
AType AT_ParentLink;
AType AT_PSRLink;
AType AT_RTEnergized;
AType AT_State;
AType AT_Value;

OType OT_Analog;
OType OT_BSCPoint;
OType OT_DCSwitch;
OType OT_Disconnector;
OType OT_Discrete;
OType OT_DistributionTransformer;
OType OT_DPCPoint;
OType OT_DPSPoint;
OType OT_Feeder;
OType OT_IncrementCommit;
OType OT_LoadBreakSwitch;
OType OT_MeasurementType;
OType OT_MVPoint;
OType OT_PMSBreaker;
OType OT_PMSBusbar;
OType OT_PMSDoubleWindingTransformer;
OType OT_PMSTerminal;
OType OT_PMSThreeWindingTransformer;
OType OT_TransformerWinding;

bool initOTypeAndAType()
{
	try
	{
		AT_BreakerType = m_OMSDatabase->matchAType("BreakerType");
		AT_ChildrenList = m_OMSDatabase->matchAType("ChildrenList");
		AT_CommitSyncEnd = m_OMSDatabase->matchAType("CommitSyncEnd");
		AT_ControlResult = m_OMSDatabase->matchAType("ControlResult");
		AT_DPCPointLink = m_OMSDatabase->matchAType("DPCPointLink");
		AT_DPSPointLink = m_OMSDatabase->matchAType("DPSPointLink");
		AT_FaultState = m_OMSDatabase->matchAType("FaultState");
		AT_KeyName = m_OMSDatabase->matchAType("KeyName");
		AT_Limit = m_OMSDatabase->matchAType("Limit");
		AT_MeasLink = m_OMSDatabase->matchAType("MeasLink");
		AT_MeasurementTypeLink = m_OMSDatabase->matchAType("MeasurementTypeLink");
		AT_MVPointLink = m_OMSDatabase->matchAType("MVPointLink");
		AT_ObjOrder = m_OMSDatabase->matchAType("ObjOrder");
		AT_ParamMva = m_OMSDatabase->matchAType("ParamMva");//额定视在功率AType
		AT_ParentLink = m_OMSDatabase->matchAType("ParentLink");
		AT_PSRLink = m_OMSDatabase->matchAType("PSRLink");
		AT_RTEnergized = m_OMSDatabase->matchAType("RTEnergized");//是否带电
		AT_State = m_OMSDatabase->matchAType("State");
		AT_Value = m_OMSDatabase->matchAType("Value");
		
		OT_Analog = m_OMSDatabase->matchOType("Analog");
		OT_BSCPoint = m_OMSDatabase->matchOType("BSCPoint");
		OT_DCSwitch = m_OMSDatabase->matchOType("DCSwitch");
		OT_Disconnector = m_OMSDatabase->matchOType("Disconnector");
		OT_Discrete = m_OMSDatabase->matchOType("Discrete");
		OT_DistributionTransformer = m_OMSDatabase->matchOType("DistributionTransformer");
		OT_DPCPoint = m_OMSDatabase->matchOType("DPCPoint");
		OT_DPSPoint = m_OMSDatabase->matchOType("DPSPoint");
		OT_Feeder = m_OMSDatabase->matchOType("Feeder");
		OT_IncrementCommit = m_OMSDatabase->matchOType("IncrementCommit");
		OT_LoadBreakSwitch = m_OMSDatabase->matchOType("LoadBreakSwitch");
		OT_MeasurementType = m_OMSDatabase->matchOType("MeasurementType");
		OT_MVPoint = m_OMSDatabase->matchOType("MVPoint");
		OT_PMSBreaker = m_OMSDatabase->matchOType("PMSBreaker");
		OT_PMSBusbar = m_OMSDatabase->matchOType("PMSBusbar");
		OT_PMSDoubleWindingTransformer = m_OMSDatabase->matchOType("PMSDoubleWindingTransformer");
		OT_PMSTerminal = m_OMSDatabase->matchOType("PMSTerminal");
		OT_PMSThreeWindingTransformer = m_OMSDatabase->matchOType("PMSThreeWindingTransformer");
		OT_TransformerWinding = m_OMSDatabase->matchOType("TransformerWinding");
	}
	catch (Exception& e)
	{
		return false;
	}
	return true;
}