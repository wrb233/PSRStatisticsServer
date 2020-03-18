#include "DBUtil.h"

extern DBPOOLHANDLE dbPoolHandle;//历史库外部全局变量

//连接历史库
bool connectDB(int argc, char *argv[])
{
	std::string user,password,dataserver;

	for(int i=0;i<argc;i++)
	{
		if(qstricmp(argv[i],"-u") == 0)
		{
			while (*(argv[i+1])!='\0')
			{
				user += *(argv[i+1]++);
			}
		}

		if(qstricmp(argv[i],"-p") == 0)
		{
			while (*(argv[i+1])!='\0')
			{
				password += *(argv[i+1]++);
			}
		}

		if(qstricmp(argv[i],"-d") == 0)
		{
			while (*(argv[i+1])!='\0')
			{
				dataserver += *(argv[i+1]++);
			}
		}
	}

	if(user == "")
		user = "hds";
	if(password == "")
		password = "hds";
	if(dataserver == "")
		dataserver = "hstserver1+hstserver2";

	CPS_ORM_Initialize();
	DBPoolConfig cfg;
	//cfg.m_bAutoCommit=false;
	cfg.m_nPoolSize=2;
	cfg.m_strUser=user;
	cfg.m_strPassword=password;	
	cfg.m_strTNS[0] = "hstserver1";	
	cfg.m_strTNS[1] = "hstserver2";	
	dbPoolHandle = CPS_ORM_NewDBPoolbyConfig(cfg);
	std::cout<<"connect to history database...";
	if(CPS_ORM_Connect(dbPoolHandle))
	{
		CPS_ORM_SetAutoCommit(dbPoolHandle,1);
		CPS_ORM_SetStreamAutoCommit(dbPoolHandle,1);
		return true;
	}
	return false;
}