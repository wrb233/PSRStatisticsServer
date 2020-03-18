#include "IncrementCommit.h"

//声明外部全局变量
extern Database *m_OMSDatabase;
extern PSRStatisticsServer *app;

bool IncrementCommit::NotfyIncrementCommit()		//注册增量提交

{
	ObId faultTolerObId = 0;

	Request req;

	ObId localMacObId = app->getFaultTolerance()->getMachineObject();

	ObId incrementObId = FindAChildrenObId(OT_IncrementCommit,localMacObId);

	if (incrementObId > 0){

		req.set(incrementObId,AT_CommitSyncEnd,NULL,NEW_NOTIFICATION,notificationIncrementCommit,(void*)0,NOTIFY_ON_WRITE,faultTolerObId);

		m_OMSDatabase->notify(&req, 1);

	}

	return true;
}



ObId IncrementCommit::FindAChildrenObId( OType childOt,ObId &parentObId )

{
	QList<ObId> obIdList;

	FindAllChildrenObId(childOt,parentObId,obIdList);

	if (!obIdList.isEmpty())

		return obIdList.at(0);

	else

		return 0;

}



int IncrementCommit::FindAllChildrenObId( OType childOt,ObId &parentObId,QList<ObId> &obIdList )

{
	Request req;

	ContainerData childrenList;

	req.set(parentObId, AT_ChildrenList, &childrenList);

	m_OMSDatabase->read(&req);

	int childrenNum = 0;

	childrenNum = childrenList.getNumberOfElements();

	if (childrenNum > 0){

		for (int j = 0; j<childrenNum; j++ ){

			const ObId* obIds = childrenList.getObIds();

			ObId childId = *(obIds+j);							

			if(childId>0){

				OType ot;

				ot =m_OMSDatabase->extractOType(childId);

				if(ot == childOt){

					obIdList.append(childId);

				}

			}

		}					

	}

	return obIdList.size();
}
