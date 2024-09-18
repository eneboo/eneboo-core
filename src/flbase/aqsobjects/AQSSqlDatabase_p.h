/***************************************************************************
 AQSSqlDatabase_p.h
 -------------------
 begin                : 29/03/2011
 copyright            : (C) 2003-2011 by InfoSiAL S.L.
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

#ifndef AQSSQLDATABASE_P_H_
#define AQSSQLDATABASE_P_H_

#include "AQSVoidPtr_p.h"
#include "AQObjects.h"

  /** Almacena los campos cacheados */
  typedef QDict<QVariant> cachedFields_;
  typedef std::map<std::string, cachedFields_> cachedFieldsMap_;
  typedef std::map<std::string, cachedFieldsMap_> cachedFieldsTable_;

class AQSSqlDatabase : public AQSVoidPtr
{
  Q_OBJECT

  AQ_DECLARE_AQS_VOID_AQOBJECT(SqlDatabase, VoidPtr);

  //@AQ_BEGIN_DEF_PUB_SLOTS@
public slots:
  bool createTable(FLTableMetaData *);
  bool canRegenTables();
  QString formatValueLike(int, const QVariant &, bool = false);
  QString formatValue(int, const QVariant &, bool = false);
  QVariant nextSerialVal(const QString &, const QString &);
  int atFrom(FLSqlCursor *);
  QString database() const;
  QString user() const;
  QString password() const;
  QString host() const;
  int port() const;
  QString driverName() const;
  bool alterTable(const QString &, const QString &, const QString& = QString::null);
  FLManager *manager();
  FLManagerModules *managerModules();
  QString connectionName() const;
  bool canSavePoint();
  bool savePoint(const QString &);
  bool releaseSavePoint(const QString &);
  bool rollbackSavePoint(const QString &);
  bool canTransaction() const;
  void Mr_Proper();
  bool canDetectLocks() const;
  QStringList locksStatus();
  QStringList detectLocks();
  QStringList detectRisksLocks(const QString& = QString::null, const QString& = QString::null);
  bool regenTable(const QString &, FLTableMetaData *);
  QStringList driverAliases();
  QString defaultAlias();
  QString driverAliasToDriverName(const QString &);
  QString driverNameToDriverAlias(const QString &);
  bool needConnOption(const QString &, int);
  QString defaultPort(const QString &);
  bool isOpen() const;
  bool isOpenError() const;
  QStringList tables() const;
  QStringList tables(uint) const;
  QSqlError *lastError() const;
  QString connectOptions() const;
  QSqlDatabase *db() const;
  QSqlDatabase *dbAux() const;
  bool interactiveGUI() const;
  void setInteractiveGUI(bool = true);
  bool qsaExceptions() const;
  void setQsaExceptions(bool = true);
  QString md5TuplesState() const;
  QString md5TuplesStateTable(const QString &) const;
  bool existsTable(const QString &) const;
  int transactionLevel() const;
  FLSqlCursor *lastActiveCursor() const;
  bool useCachedFields(const QString &) const;
  cachedFieldsMap_ cachedFieldsTable(const QString &table);
  void setCachedFieldsTable(const QString &table, QString &pkValue, cachedFields_ fields);

protected:
  static void *construct(const QSArgumentList &args) {
    return 0;
  }
  //@AQ_END_DEF_PUB_SLOTS@
};

//@AQ_BEGIN_IMP_PUB_SLOTS@
inline bool AQSSqlDatabase::createTable(FLTableMetaData *arg0)
{
  AQ_CALL_RET_V(createTable(arg0), bool);
}
inline bool AQSSqlDatabase::canRegenTables()
{
  AQ_CALL_RET_V(canRegenTables(), bool);
}
inline QString AQSSqlDatabase::formatValueLike(int arg0,  const QVariant &arg1,  bool arg2)
{
  AQ_CALL_RET_V(formatValueLike(arg0, arg1, arg2), QString);
}
inline QString AQSSqlDatabase::formatValue(int arg0,  const QVariant &arg1,  bool arg2)
{
  AQ_CALL_RET_V(formatValue(arg0, arg1, arg2), QString);
}
inline QVariant AQSSqlDatabase::nextSerialVal(const QString &arg0,  const QString &arg1)
{
  AQ_CALL_RET_V(nextSerialVal(arg0, arg1), QVariant);
}
inline int AQSSqlDatabase::atFrom(FLSqlCursor *arg0)
{
  AQ_CALL_RET_V(atFrom(arg0), int);
}
inline QString AQSSqlDatabase::database() const
{
  AQ_CALL_RET_V(database(), QString);
}
inline QString AQSSqlDatabase::user() const
{
  AQ_CALL_RET_V(user(), QString);
}
inline QString AQSSqlDatabase::password() const
{
  AQ_CALL_RET_V(password(), QString);
}
inline QString AQSSqlDatabase::host() const
{
  AQ_CALL_RET_V(host(), QString);
}
inline int AQSSqlDatabase::port() const
{
  AQ_CALL_RET_V(port(), int);
}
inline QString AQSSqlDatabase::driverName() const
{
  AQ_CALL_RET_V(driverName(), QString);
}
inline bool AQSSqlDatabase::alterTable(const QString &arg0,  const QString &arg1,  const QString &arg2)
{
  AQ_CALL_RET_V(alterTable(arg0, arg1, arg2), bool);
}
inline FLManager *AQSSqlDatabase::manager()
{
  AQ_CALL_RET(manager());
}
inline FLManagerModules *AQSSqlDatabase::managerModules()
{
  AQ_CALL_RET(managerModules());
}
inline QString AQSSqlDatabase::connectionName() const
{
  AQ_CALL_RET_V(connectionName(), QString);
}
inline bool AQSSqlDatabase::canSavePoint()
{
  AQ_CALL_RET_V(canSavePoint(), bool);
}
inline bool AQSSqlDatabase::savePoint(const QString &arg0)
{
  AQ_CALL_RET_V(savePoint(arg0), bool);
}
inline bool AQSSqlDatabase::releaseSavePoint(const QString &arg0)
{
  AQ_CALL_RET_V(releaseSavePoint(arg0), bool);
}
inline bool AQSSqlDatabase::rollbackSavePoint(const QString &arg0)
{
  AQ_CALL_RET_V(rollbackSavePoint(arg0), bool);
}
inline bool AQSSqlDatabase::canTransaction() const
{
  AQ_CALL_RET_V(canTransaction(), bool);
}
inline void AQSSqlDatabase::Mr_Proper()
{
  AQ_CALL_VOID(Mr_Proper());
}
inline bool AQSSqlDatabase::canDetectLocks() const
{
  AQ_CALL_RET_V(canDetectLocks(), bool);
}
inline QStringList AQSSqlDatabase::locksStatus()
{
  AQ_CALL_RET_V(locksStatus(), QStringList);
}
inline QStringList AQSSqlDatabase::detectLocks()
{
  AQ_CALL_RET_V(detectLocks(), QStringList);
}
inline QStringList AQSSqlDatabase::detectRisksLocks(const QString &arg0,  const QString &arg1)
{
  AQ_CALL_RET_V(detectRisksLocks(arg0, arg1), QStringList);
}
inline bool AQSSqlDatabase::regenTable(const QString &arg0,  FLTableMetaData *arg1)
{
  AQ_CALL_RET_V(regenTable(arg0, arg1), bool);
}
inline QStringList AQSSqlDatabase::driverAliases()
{
  AQ_CALL_RET_V(driverAliases(), QStringList);
}
inline QString AQSSqlDatabase::defaultAlias()
{
  AQ_CALL_RET_V(defaultAlias(), QString);
}
inline QString AQSSqlDatabase::driverAliasToDriverName(const QString &arg0)
{
  AQ_CALL_RET_V(driverAliasToDriverName(arg0), QString);
}
inline QString AQSSqlDatabase::driverNameToDriverAlias(const QString &arg0)
{
  AQ_CALL_RET_V(driverNameToDriverAlias(arg0), QString);
}
inline bool AQSSqlDatabase::needConnOption(const QString &arg0,  int arg1)
{
  AQ_CALL_RET_V(needConnOption(arg0, arg1), bool);
}
inline QString AQSSqlDatabase::defaultPort(const QString &arg0)
{
  AQ_CALL_RET_V(defaultPort(arg0), QString);
}
inline bool AQSSqlDatabase::isOpen() const
{
  AQ_CALL_RET_V(isOpen(), bool);
}
inline bool AQSSqlDatabase::isOpenError() const
{
  AQ_CALL_RET_V(isOpenError(), bool);
}
inline QStringList AQSSqlDatabase::tables() const
{
  AQ_CALL_RET_V(tables(), QStringList);
}
inline QStringList AQSSqlDatabase::tables(uint arg0) const
{
  AQ_CALL_RET_V(tables(static_cast<QSql::TableType>(arg0)), QStringList);
}
inline QSqlError *AQSSqlDatabase::lastError() const
{
  AQ_CALL_RET_PTR(lastError(), QSqlError);
}
inline QString AQSSqlDatabase::connectOptions() const
{
  AQ_CALL_RET_V(connectOptions(), QString);
}
inline QSqlDatabase *AQSSqlDatabase::db() const
{
  AQ_CALL_RET(db());
}
inline QSqlDatabase *AQSSqlDatabase::dbAux() const
{
  AQ_CALL_RET(dbAux());
}
inline bool AQSSqlDatabase::interactiveGUI() const
{
  AQ_CALL_RET_V(interactiveGUI(), bool);
}
inline void AQSSqlDatabase::setInteractiveGUI(bool arg0)
{
  AQ_CALL_VOID(setInteractiveGUI(arg0));
}
inline bool AQSSqlDatabase::qsaExceptions() const
{
  AQ_CALL_RET_V(qsaExceptions(), bool);
}
inline void AQSSqlDatabase::setQsaExceptions(bool arg0)
{
  AQ_CALL_VOID(setQsaExceptions(arg0));
}
inline QString AQSSqlDatabase::md5TuplesState() const
{
  AQ_CALL_RET_V(md5TuplesState(), QString);
}
inline QString AQSSqlDatabase::md5TuplesStateTable(const QString &arg0) const
{
  AQ_CALL_RET_V(md5TuplesStateTable(arg0), QString);
}
inline bool AQSSqlDatabase::existsTable(const QString &arg0) const
{
  AQ_CALL_RET_V(existsTable(arg0), bool);
}
inline int AQSSqlDatabase::transactionLevel() const
{
  AQ_CALL_RET_V(transactionLevel(), int);
}
inline FLSqlCursor *AQSSqlDatabase::lastActiveCursor() const
{
  AQ_CALL_RET(lastActiveCursor());
}

inline cachedFieldsMap_ AQSSqlDatabase::cachedFieldsTable(const QString &table)
{
  AQ_CALL_RET_V(cachedFieldsTable(table), cachedFieldsMap_);
}
inline void AQSSqlDatabase::setCachedFieldsTable(const QString &table, QString &pkValue, cachedFields_ fields)
{
  AQ_CALL_VOID(setCachedFieldsTable(table, pkValue, fields));
}
inline bool AQSSqlDatabase::useCachedFields(const QString &tableName) const
{
  AQ_CALL_RET_V(useCachedFields(tableName), bool);
}
//@AQ_END_IMP_PUB_SLOTS@

#endif /* AQSSQLDATABASE_P_H_ */
// @AQOBJECT_VOID@
