/***************************************************************************
                             FLSqlDriver.h
                          -------------------
 begin                : Thu Nov 22 2005
 copyright            : (C) 2002-2005 by InfoSiAL S.L.
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

#ifndef FLSQLDRIVER_H
#define FLSQLDRIVER_H

#include <qsqldriver.h>
#include <qsqlresult.h>

#include "AQGlobal.h"

class FLTableMetaData;
class FLSqlCursor;
class FLSqlDatabase;

/**
Clase de abstracci�n para controladores de bases de datos.

Mediante esta clase se definen de forma unificada los distintos controladores para la
gesti�n de sistemas de gesti�n de bases de datos espec�ficos.

Esta clase no deber�a utilizarse directamente, se recomienda utilizar FLSqlDatabase.

@author InfoSiAL S.L.
*/
class AQ_EXPORT FLSqlDriver : public QSqlDriver
{
  Q_OBJECT

public:

  /**
  constructor
  */
  FLSqlDriver(QObject *parent = 0, const char *name = 0);

  /**
  destructor
  */
  ~FLSqlDriver();

  /**
  Obtiene el nombre de la base de datos formateado correctamente para realizar una conexi�n

  @param name Nombre de la base de datos
  @return Cadena con el nombre debidamente formateado
  */
  virtual QString formatDatabaseName(const QString &name);

  /**
  Intentar realizar una conexi�n a una base de datos.

  Si la base de datos no existe intenta crearla.

  @param database Nombre de la base de datos a la que conectar
  @param user  Usuario
  @param password Contrase�a
  @param host  Servidor de la base de datos
  @param port  Puerto TCP de conexi�n
  @return True si la conexi�n tuvo �xito, false en caso contrario
  */
  virtual bool tryConnect(const QString &db, const QString &user = QString::null, const QString &password = QString::null,
                          const QString &host = QString::null, int port = -1);

  /**
  Sentencia SQL espec�fica de la base de datos que soporta el controlador, necesaria para crear
  la tabla solicitada.

  @param tmd Metadatos con la descripci�n de la tabla que se desea crear
  @return Sentencia SQL debidamente formateada para el tipo de base de datos soportada por el controlador
  */
  virtual QString sqlCreateTable(const FLTableMetaData *tmd);

  /** Ver FLSqlDatabase::formatValueLike() */
  virtual QString formatValueLike(int t, const QVariant &v, const bool upper = false);
  /** Ver FLSqlDatabase::formatValue() */
  virtual QString formatValue(int t, const QVariant &v, const bool upper = false);
  /** Ver FLSqlDatabase::nextSerialVal() */
  virtual QVariant nextSerialVal(const QString &table, const QString &field);
  /** Ver FLSqlDatabase::atFrom() */
  virtual int atFrom(FLSqlCursor *cur);
  /** Ver FLSqlDatabase::alterTable() */
  virtual bool alterTable(const QString &mtd1, const QString &mtd2, const QString &key = QString::null);
  /** Ver FLSqlDatabase::canSavePoint() */
  virtual bool canSavePoint();
  /** Ver FLSqlDatabase::savePoint() */
  virtual bool savePoint(const QString &n);
  /** Ver FLSqlDatabase::releaseSavePoint() */
  virtual bool releaseSavePoint(const QString &n);
  /** Ver FLSqlDatabase::rollbackSavePoint() */
  virtual bool rollbackSavePoint(const QString &n);
  /** Ver FLSqlDatabase::canOverPartition() */
  virtual bool canOverPartition();
  /** Ver FLSqlDatabase::Mr_Proper() */
  virtual void Mr_Proper() {}
  /** Ver FLSqlDatabase::locksStatus() */
  virtual QStringList locksStatus();
  /** Ver FLSqlDatabase::detectLocks() */
  virtual QStringList detectLocks();
  /** Ver FLSqlDatabase::detectRisksLocks() */
  virtual QStringList detectRisksLocks(const QString &table = QString::null,
                                       const QString &primaryKeyValue = QString::null);
  /** Ver FLSqlDatabase::regenTable() */
  virtual bool regenTable(const QString &n, FLTableMetaData *tmd);
  /** Ver FLSqlDatabase::md5TuplesState() */
  virtual QString md5TuplesState() const;
  /** Ver FLSqlDatabase::md5TuplesStateTable() */
  virtual QString md5TuplesStateTable(const QString &table) const;
  /** Ver FLSqlDatabase::mismatchedTable() */
  virtual bool mismatchedTable(const QString &table,
                               const FLTableMetaData *tmd) const;
  /** Ver FLSqlDatabase::existsTable() */
  virtual bool existsTable(const QString &n) const;

  /**
  Informa al driver de la base de datos que lo utiliza
  */
  void setFLSqlDatabase(FLSqlDatabase *db);

  FLSqlDatabase *db() const;

  QString userIdApi;
  QString urlApi;
  QString userApi;
  QString passwordApi;

protected:

  void msgBoxCritical(const QString &title, const QString &msg);

  virtual void setLastError(const QSqlError &e);

  FLSqlDatabase *db_;
};

class AQ_EXPORT FLSqlResult : public QSqlResult
{
public:
  virtual ~FLSqlResult();

protected:
  FLSqlResult(const QSqlDriver *db);

  virtual void setLastError(const QSqlError &e);
};

#endif
