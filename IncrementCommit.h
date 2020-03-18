#ifndef INCREMENTCOMMIT_H
#define INCREMENTCOMMIT_H

#include "common.h"
#include "NotificationCallback.h"

class IncrementCommit
{
public:
	//ע�������ύ
	bool NotfyIncrementCommit();
private:
	ObId FindAChildrenObId( OType childOt,ObId &parentObId );

	int FindAllChildrenObId( OType childOt,ObId &parentObId,QList<ObId> &obIdList );

};
#endif