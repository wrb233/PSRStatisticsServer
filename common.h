#ifndef COMMON_H
#define COMMON_H

#include <QMap>
#include <QObject>
#include <QTimer>
#include <QDebug>
#include <QDateTime>
#include <QStringList>
#include <QTextCodec>
#include <QSettings>
#include <QFile>

#include "oms/oms_database.h"
#include "oms/qt_application.h"
#include "oms/data_all.h" 
#include "oms/database.h"
#include "oms/database_defs.h"
#include "oms/object.h"
#include "oms/oms_time.h"
#include "oms/object_type.h"
#include "oms/object_attribute.h"
#include "oms/choice_literals.h" 
#include "ORMDatabase/ORM_database.h"
#include "ORMDatabase/ORM_interface.h"
#include "libORM/ormlib.h"
#include <tolerance/basic_fault_tolerance.h>
#include <log4cplus/logger.h>
#include <log4cplus/configurator.h>
#include <log4cplus/helpers/loglog.h>
#include <log4cplus/helpers/stringhelper.h>
#include <log4cplus/helpers/sleep.h>
#include <log4cplus/loggingmacros.h>

#include "OTypeAndATypeInit.h"

using namespace log4cplus;
using namespace log4cplus::helpers;

#endif