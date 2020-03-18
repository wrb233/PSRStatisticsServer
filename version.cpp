#define CPS_PSRSTATISTICSSERVER_VERSION_STR "0.4.5"
#define CPS_PSRSTATISTICSSERVER_REMARK_STR  "Modify statistic time. by yanglibin 2016-11-28,V0.1.3\n \
 Add poweroncount,poweroffcount,powerontime,powerofftime. by yanglibin 2017-02-21,V0.2.3\n \
 Add AccidentOffCount,ArtificialOffCount,AccidentOffTime,ArtificialOffTime. by yanglibin 2017-03-22,V0.3.3.\n \
 Add FeederActionTimes、FeederSuccessTimes、FeederFaultTimes 、ProtectionsCount by yanglibin 2017-08-23,V0.4.3.\n \
 Add database dbhandlepool  state check by yanglibin 2018-01-22,V0.4.4.\n \
 Modify dbhandlepool size to 2 by yanglibin 2018-05-10,V0.4.5."
#include "version.h"

//打印版本信息函数
bool echoVersion(int argc, char *argv[]){
	for(int i = 0;i < argc;i++){
		if(qstricmp(argv[i],"-v") == 0 || qstricmp(argv[i],"--version") == 0 ){
			std::cout << "Version: " << CPS_PSRSTATISTICSSERVER_VERSION_STR << std::endl;
			std::cout << "Remark : " << CPS_PSRSTATISTICSSERVER_REMARK_STR  << std::endl;	
			return true;
		}
	}
	return  false;
}