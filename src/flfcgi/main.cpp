/***************************************************************************
 main.cpp
 -------------------
 begin                : lun mar 19 13:44:14 CET 2001
 copyright            : (C) 2001-2005 by InfoSiAL, S.L.
 email                : mail@infosial.com
 ***************************************************************************/
/***************************************************************************
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; version 2 of the License.               *
 ***************************************************************************/
/***************************************************************************
 Este  programa es software libre. Puede redistribuirlo y/o modificarlo
 bajo  los  t�rminos  de  la  Licencia  P�blica General de GNU   en  su
 versi�n 2, publicada  por  la  Free  Software Foundation.
 ***************************************************************************/

// C Libraries ----
#include <stdlib.h>
#include <stdio.h>
// ----

#include <qsplashscreen.h>

#include "../flbase/FLConnectDBDialog.h"
#include "../flbase/FLDiskCache.h"
#include "../flbase/FLSqlDatabase.h"
#include "../flbase/FLSqlConnections.h"
#include "../flbase/FLSettings.h"

#include "../flbase/AQApplication.h"
#include "../../AQConfig.h"

#define NO_FCGI_DEFINES
#include "fcgi_stdio.h"

#include "fcgiobjectfactory.h"


static inline bool silentConnect(const QString &conn)
{
  if (conn.isEmpty())
    return false;

  QString user, namedb, db, host, port, password, connOpts;
  QStringList dat = QStringList::split(":", conn, true);
  int i = 0;
  for (QStringList::Iterator it = dat.begin(); it != dat.end(); ++it, ++i) {
    switch (i) {
      case 0:
        namedb = *it;
        break;
      case 1:
        user = *it;
        break;
      case 2:
        db = *it;
        break;
      case 3:
        host = *it;
        break;
      case 4:
        port = *it;
        break;
      case 5:
        password = *it;
        break;
      case 6:
        connOpts = *it;
        break;
    }
  }

  FLSqlDatabase *sqlDb = new FLSqlDatabase();
  QString driverName(FLSqlDatabase::driverAliasToDriverName(db));

  if (!sqlDb->loadDriver(driverName)) {
    delete sqlDb;
    return false;
  }
  
    if (driverName == "FLQPSQL7") {
    if (connOpts.isEmpty())
      connOpts = "connect_timeout=30";
    else
      connOpts += ";connect_timeout=30";
  }

  if (!sqlDb->connectDB(namedb, user, password, host, port.toInt(),
                        "default", connOpts)) {
    delete sqlDb;
    return false;
  }

  FLSqlConnections::addDatabase(sqlDb);
  return true;
}

void aq_main(int argc, char **argv) 
{
  QString formAlone, callFunction, arguments, strConn, silentConn;
  bool quitAfterCall = false, autoLogin_ = false, noMax = false;

  for (int i = 1; i < argc; i++) {
    if (qstrcmp(argv[i], "-f") == 0) {
      if (i != argc - 1) {
        i++;
        formAlone = QString::fromLatin1(argv[i]);
      }
    } else if (qstrcmp(argv[i], "-c") == 0) {
      if (i != argc - 1) {
        i++;
        callFunction = QString::fromLatin1(argv[i]);
      }
    } else if (qstrcmp(argv[i], "-a") == 0) {
      if (i != argc - 1) {
        i++;
        arguments = QString::fromLatin1(argv[i]);
      }
    } else if (qstrcmp(argv[i], "-sc") == 0) {
      if (i != argc - 1) {
        i++;
        strConn = QString::fromLatin1(argv[i]);
      }
    } else if (qstrcmp(argv[i], "-silentconn") == 0) {
      if (i != argc - 1) {
        i++;
        silentConn = QString::fromLatin1(argv[i]);
      }
    } else if (qstrcmp(argv[i], "-q") == 0) {
      quitAfterCall = true;
    } else if (qstrcmp(argv[i], "-auto") == 0) {
      autoLogin_ = true;
    } else if (qstrcmp(argv[i], "-nomax") == 0) {
      noMax = true;
    }
  }
  FLApplication *AbanQ = aqApp;
  if (silentConn.isEmpty()) {
    printf("Usage: eneboo-fcgi -silentconn dbname:user:PostgreSQL:host:5432:passwd\n");
    QTimer::singleShot(0, AbanQ, SLOT(quit()));
    return;
  }
  QFont appFont;

  QStringList paths(AbanQ->libraryPaths());
  QString pathApp(AQ_PREFIX + "/plugins");
  if (!paths.contains(pathApp))
    paths << pathApp;
  if (!paths.contains(AQ_LIB))
    paths << AQ_LIB;
  AbanQ->setLibraryPaths(paths);
  AbanQ->installTranslator(AbanQ->createSysTranslator(QString(QTextCodec::locale()).left(2), true));
  AbanQ->installTranslator(AbanQ->createSysTranslator("multilang"));
  
  int pointSize = 8;

  if (!silentConnect(silentConn)) {
    QTimer::singleShot(0, AbanQ, SLOT(quit()));
    return;
  }
  QString empty;

  AbanQ->initfcgi();
  AbanQ->addObjectFactory(new FCgiObjectFactory());

    int count = 0;
	QStringList argumentList;
    while (FCGI_Accept() >= 0)   {    
		QString ret = AbanQ->callfcgi(callFunction, argumentList);
    }
  AbanQ->endfcgi();
    
      
}

int main(int argc, char **argv)
{
  int retval = 0;
  AQApplication app(argc, argv);
  aq_main(argc, argv);
  retval = app.exec();
  exit(retval);
  return retval;
}




/* **************** Implementaci�n de objectfactory para FastCGI ******
 * ********************************************************************
 * 
 * ... probablemente esto haya que pasarlo a un fichero distinto
 * ... llamado fcgiobjectfactory.cpp ...
 */

FCgiObjectFactory::FCgiObjectFactory() :
  QSObjectFactory()
{
  registerClass("FLFastCgi", "FLFastCgiInterface");
 }

FCgiObjectFactory::~FCgiObjectFactory()
{
}

QObject *FCgiObjectFactory::create(const QString &className, const QSArgumentList &arguments, QObject *context)
{
  if (className == "FLFastCgi") {
    if (arguments.count() == 0) {
      return new FLFastCgiInterface();
    }
  }

  return NULL;
}


