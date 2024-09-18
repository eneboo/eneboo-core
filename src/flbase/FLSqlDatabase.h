/***************************************************************************
                             FLSqlDatabase.h
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

#ifndef FLSQLDATABASE_H
#define FLSQLDATABASE_H

#include <qsqldatabase.h>
#include <qsqlresult.h>
#include <qptrstack.h>
#include <qptrqueue.h>
#include <qguardedptr.h>
#include <qvaluestack.h>
#include "AQGlobal.h"

class FLTableMetaData;
class FLSqlCursor;
class FLManager;
class FLManagerModules;
class FLSqlSavePoint;

/**
Gesti�n de bases de datos.

Proporciona una capa de abstracci�n para manejar distintos tipos de bases de datos.
A trav�s de controladores internos (drivers) espec�ficos se gestiona el acceso a cada
uno de los tipos de la bases de datos, ver FLSqlDriver.

Ofrece m�todos para cargar controladores, conectar a la base de datos y realizar operaciones
espec�ficas de forma unificada.

El sistemas de persistencia utiliza esta clase para poder trabajar de forma homog�nea sobre cualquier
tipo de base de datos de la que exista un controlador.

@author InfoSiAL S.L.
*/
class AQ_EXPORT FLSqlDatabase
{

public:

  /**
  Enumeraci�n de opciones de conexi�n
  */
  enum ConnOptions {
    /** Usuario */
    USER = 0,
    /** Contrase�a */
    PASSWORD = 1,
    /** Puerto */
    PORT = 2,
    /** Servidor */
    HOST = 3
  };

  /**
  constructor
  */
  FLSqlDatabase();

  /**
  destructor
  */
  ~FLSqlDatabase();

  /** Almacena los campos cacheados */
  typedef std::map<std::string, std::variant<bool, int, std::string>> cachedFields_;
  typedef std::map<std::string, cachedFields_> cachedFieldMap_;
  typedef std::map<std::string, cachedFieldsMap_> cachedFieldsTable_;

  /**
  @return Lista de los alias de los controladores actualmente disponibles.
  */
  static QStringList driverAliases();

  /**
  @return Alias establecido por defecto
  */
  static QString defaultAlias();

  /**
  @param alias Alias de un controlador
  @return Alias el nombre interno de un controlador a partir de su alias
  */
  static QString driverAliasToDriverName(const QString &alias);

  /**
  @param name Nombre interno de un controlador
  @return Alias de un controlador a partir de su nombre interno
  */
  static QString driverNameToDriverAlias(const QString &name);

  /**
  Obtiene si un controlador necesita una opci�n de conexi�n, a partir de su alias.

  @param alias Alias del controlador
  @param connOption Tipo de opci�n a comprobar, del tipo enumeraci�n FLSqlDatabase::ConnOptions
  @return True si la opci�n es necesaria para que el controlador pueda establecer la conexi�n, false en caso contrario
  */
  static bool needConnOption(const QString &alias, const int connOption);

  /**
  Obtiene el puerto de conexi�n usado habitualmente por un controlador

  @param alias Alias del controlador
  @return Numero de puerto
  */
  static QString defaultPort(const QString &alias);

  /**
  Carga un controlador.

  @param driverName Nombre interno del controlador que se desa cargar
  @param connName Nombre de la conexion
  @return True si la carga tuvo �xito, false en caso contrario
  */
  bool loadDriver(const QString &driverName, const QString &connName = "default");

  /**
  Conecta con una base de datos.

  Para poder conectar con una base de datos es imprescindible haber cargado con anterioridad el driver
  correspondiente utilizando FLSqlDatabase::loadDriver().

  A partir de la llamada a este m�todo la base de datos actual para a ser a la que conectamos. Internamente,
  si es posible, crea dos conexiones paralelas a la misma base de datos; FLSqlDatabase::db() y FLSqlDatabase::dbAux().

  @param database Nombre de la base de datos a la que conectar
  @param user  Usuario
  @param password Contrase�a
  @param host  Servidor de la base de datos
  @param port  Puerto TCP de conexi�n
  @param connName Nombre de la conexion
  @param connectOptions Contiene opciones auxiliares de conexi�n a la base de datos.
                        El formato de la cadena de opciones es una lista separada por punto y coma
                        de nombres de opci�n o la opci�n = valor. Las opciones dependen del uso del
                        driver de base de datos.
                        Si a las opciones se a�ade 'nogui' desactiva interactiveGUI_
                        Si a las opciones se a�ade 'noexceptions' desactiva qsaExceptions_
  @return True si la conexi�n tuvo �xito, false en caso contrario
  */
  bool connectDB(const QString &database, const QString &user = QString::null,
                 const QString &password = QString::null,
                 const QString &host = QString::null,
                 int port = -1, const QString &connName = "default",
                 const QString &connectOptions = QString::null);

  /**
  Conecta con una base de datos utilizando los datos de conexi�n actuales
  */
  bool connectDB();

  /**
  Crea una tabla en la base de datos actual.

  @param tmd Metadatos con la descripci�n de la tabla a crear
  @return True si se pudo crear la tabla, false en caso contrario
  */
  bool createTable(FLTableMetaData *tmd);

  /**
  @return True si la base de datos actual es capaz de regenerar tablas de forma din�mica
  */
  bool canRegenTables();

  /**
   @return True si la base de datos actual soporta la funci�n unaccent
  */
  bool canUnaccent();

  /**
  Devuelve el contenido del valor de de un campo formateado para ser reconocido
  por la base de datos actual en condiciones LIKE, dentro de la clausura WHERE de SQL.

  Este m�todo toma como parametros los metadatos del campo definidos con
  FLFieldMetaData. Adem�s de TRUE y FALSE como posibles valores de un campo
  l�gico tambi�n acepta los valores S� y No (o su traducci�n al idioma correspondiente).
  Las fechas son adaptadas al forma AAAA-MM-DD, que es el formato reconocido por PostgreSQL .

  @param t Tipo de datos del valor
  @param v Valor que se quiere formatear para el campo indicado
  @param upper Si TRUE convierte a may�sculas el valor (si es de tipo cadena)
  @return Valor del campo debidamente formateado
  */
  QString formatValueLike(int t, const QVariant &v, const bool upper = false);

  /**
  Devuelve el contenido del valor de de un campo formateado para ser reconocido
  por la base de datos actual, dentro de la clausura WHERE de SQL.

  Este m�todo toma como parametros los metadatos del campo definidos con
  FLFieldMetaData. Adem�s de TRUE y FALSE como posibles valores de un campo
  l�gico tambi�n acepta los valores S� y No (o su traducci�n al idioma correspondiente).
  Las fechas son adaptadas al forma AAAA-MM-DD, que es el formato reconocido por PostgreSQL .

  @param t Tipo de datos del valor
  @param v Valor que se quiere formatear para el campo indicado
  @param upper Si TRUE convierte a may�sculas el valor (si es de tipo cadena)
  @return Valor del campo debidamente formateado
  */

  QString formatValue(int t, const QVariant &v, const bool upper = false);

  /**
  Obtiene el siguiente valor de la secuencia para campos del tipo serial.

  @param table Nombre la tabla del campo serial
  @param field Nombre del campo serial
  @return Siguiente valor de la secuencia
  */
  QVariant nextSerialVal(const QString &table, const QString &field);

  /**
  Obtiene la posici�n del registro actual.

  La posici�n del registro actual dentro del cursor se calcula teniendo en cuenta el
  filtro actual ( FLSqlCursor::curFilter() ) y el campo o campos de ordenamiento
  del mismo ( QSqlCursor::sort() ).
  Este m�todo es �til, por ejemplo, para saber en que posici�n dentro del cursor
  se ha insertado un registro.

  @param cur Cursor sobre el que calcular la posici�n del registro.
  @return Posici�n del registro dentro del cursor.
  */
  int atFrom(FLSqlCursor *cur);

  /**
  @return Conexi�n principal a la base de datos actual
  */
  QSqlDatabase *db() const {
    return db_;
  }

  /**
  @return Conexi�n auxiliar a la base de datos actual
  */
  QSqlDatabase *dbAux() const {
    return dbAux_;
  }

  /**
  @return Nombre de la base de datos actual
  */
  QString database() const {
    return remote_database_ == "" ? database_ : remote_database_;
  }

    /**
  @return Nombre de la base de datos actual
  */
  QString database2() const {
    return database_;
  }

  /**
  @return Usuario utilizado para conectar a la base de datos actual
  */
  QString user() const {
    return remote_user_ == "" ? user_ : remote_user_;
  }

  /**
  @return Usuario utilizado para conectar a la base de datos actual
  */
  QString user2() const {
    return user_;
  }

  /**
  @return Contrase�a utilizada para conectar a la base de datos actual
  */
  QString password() const {
    return password_;
  }

  /**
  @return Direcci�n del servidor donde se encuentra la base de datos actual
  */
  QString host() const {
    return host_;
  }

  /**
  @return Puerto TCP utlizado para conectar al servidor de la base de datos actual
  */
  int port() const {
    return port_;
  }

  /**
  @return Nombre interno del driver utilizado para conectar a la base de datos actual
  */
  QString driverName() const {
    return driverName_;
  }

  /**
  Modifica la estructura de una tabla dada, preservando los datos. La nueva
  estructura y la vieja se pasan en cadenas de caracteres con la descripcion XML.

  @param n Nombre de la tabla a reconstruir
  @param mtd1 Descripcion en XML de la vieja estructura
  @param mtd2 Descripcion en XML de la nueva estructura
  @param key Clave sha1 de la vieja estructura
  @return TRUE si la modificaci�n tuvo �xito
  */
  bool alterTable(const QString &mtd1, const QString &mtd2, const QString &key = QString::null);

  /**
  @return Manejador general
  */
  FLManager *manager();

  /**
  @return Manejador de m�dulos
  */
  FLManagerModules *managerModules();

  /**
  @return Nombre de la conexi�n
  */
  QString connectionName() const {
    return connectionName_;
  }

  /**
  @return Si tiene capacidad para crear puntos de salvaguarda
  */
  bool canSavePoint();

  /**
  Crea un punto de salvaguarda

  @param n Nombre que se le asignar� al punto de salvaguarda
  @return TRUE si la acci�n tuvo �xito
  */
  bool savePoint(const QString &n);

  /**
  Libera un punto de salvaguarda

  @param n Nombre del punto de salvaguarda a liberar
  @return TRUE si la acci�n tuvo �xito
  */
  bool releaseSavePoint(const QString &n);

  /**
  Deshace operaciones hasta el punto de salvaguarda

  @param n Nombre del punto de salvaguarda
  @return TRUE si la acci�n tuvo �xito
  */
  bool rollbackSavePoint(const QString &n);

  /**
  @return Si soporta transacciones
  */
  bool canTransaction() const;

  /**
  @return True si la base de datos soporta la sentencia OVER
  */
  bool canOverPartition() const;


  /**
  Ejecuta tareas de limpieza y optimizaci�n de la base de datos
  */
  void Mr_Proper();

  /**
  @return True si la base de datos actual puede detectar si sus transacciones est�n bloqueando a las de otra conexi�n
  */
  bool canDetectLocks() const;

  /**
  Para obtener informaci�n sobre el estado de los bloqueos existentes en la base de datos.

  Si hay bloqueos devuelve una lista de cadenas de texto en forma de registros de informaci�n. En esta lista
  la primera cadena de texto contiene los nombres de los campos de informaci�n incluidos y separados con "@",
  las siguientes cadenas son una por cada bloqueo con la informaci�n correspondiente.
  Si hay registros bloqueados produciendo situaciones de espera, se incluye informaci�n de los mismos cuando
  la cadena empieza por "##", indicando el nombre del campo clave primaria y el valor para el registro bloqueado.

  Ejemplo:

  "relation@locktype@pid"
  "stocks@RowExclusiveLock@8229"
  "##idstock=203"
  "secuencias@ExclusiveLock@8393"

  @return Lista con informaci�n de los bloqueos, si es vacia no hay bloqueos.
  */
  QStringList locksStatus();

  /**
  Comprueba si las transacciones de la base de datos actual est�n bloqueando a otras conexiones.

  Si hay bloqueos devuelve una lista de los mismos con el formato descrito en FLSqlDatabase::locksStatus()

  @return Lista con informaci�n de los bloqueos, si es vacia no hay bloqueos.
  */
  QStringList detectLocks();

  /**
  Comprueba si hay riesgo de caer en un bloqueo en espera con otras conexiones.

  Si hay riesgo devuelve una lista de los bloqueos candidatos con el mismo formato descrito en FLSqlDatabase::locksStatus()

  @param  table           El nombre de una tabla para solo comprobar los riesgos sobre ella, o vacio
                          para comprobarlos en todas las tablas.
  @param  primaryKeyValue El valor de la clave primaria de un registro para solo comprobar los riesgos sobre el,
                          o vacio para comprobarlos en todos. ( No funciona con claves compuestas ).
  @return Lista con informaci�n de los bloqueos, si es vacia no hay bloqueos.
  */
  QStringList detectRisksLocks(const QString &table = QString::null, const QString &primaryKeyValue = QString::null);

  /**
  Regenera una tabla si su estructura actual en la base de datos difiere de la estructura definida en los metadatos
  pasados como par�metro.

  @param  n   Nombre de la tabla de la base de datos
  @param  tmd Metadatos con la descripci�n de la tabla
  @return True si se necesitaba regenerar la tabla y la regenaci�n tuvo �xito
  */
  bool regenTable(const QString &n, FLTableMetaData *tmd);

  /**
  Devuelve la suma md5 con el total de registros insertados, borrados y modificados en la base de datos hasta ahora

  Util para saber si la base de datos ha sido modificada desde un momento dado
  */
  QString md5TuplesState() const;

  /**
  Devuelve la suma md5 con el total de registros insertados, borrados y modificados en una tabla hasta ahora

  Util para saber si una tabla ha sido modificada desde un momento dado
  */
  QString md5TuplesStateTable(const QString &table) const;

  /** Ver propiedad interactiveGUI_ */
  bool interactiveGUI() const;
  void setInteractiveGUI(bool on = true);

  /** Ver propiedad qsaExceptions_ */
  bool qsaExceptions() const;
  void setQsaExceptions(bool on = true) {
    qsaExceptions_ = on;
  }

  /**
  Indica si la estructura de los metadatos de una tabla no coincide con la estructura de la tabla
  en la base de datos
  */
  bool mismatchedTable(const QString &table,
                       const FLTableMetaData *tmd) const;

  /**
  Indica si existe la tabla
  */
  bool existsTable(const QString &n) const;

  /**
  @return El nivel actual de anidamiento de transacciones, 0 no hay transaccion
  */
  int transactionLevel() const {
    return transaction_;
  }

  /**
  @return El �ltimo cursor activo en esta base de datos con una transacci�n abierta
  */
  FLSqlCursor *lastActiveCursor() const {
    return lastActiveCursor_;
  }

  // Some Wrappers to QSqlDatabase
  bool isOpen() const;
  bool isOpenError() const;
  QStringList tables() const;
  QStringList tables(QSql::TableType type) const;
  QSqlError lastError() const;
  QString connectOptions() const;
  
  QValueStack<int> cursorsOpened;

  void setRemoteUser(const QString &user) {
    remote_user_ = user;
  }

  void setRemoteDatabase(const QString &database) {
    remote_database_ = database;
  }

  cachedFieldsMap_ cachedFieldsTable(const QString &table);
  void setCachedFieldsTable(const QString &table, const QString &pkValue, const cachedFields_ fields);
  bool useCachedFields(const QString &tableName) const;




private:




  /**
  Cierra la conexi�n actual de la base de datos
  */
  void closeDB();

  /** Conexi�n principal a la base de datos actual */
  QSqlDatabase *db_;

  /** Usuario utilizado para conectar a la base de datos actual */
  QSqlDatabase *dbAux_;

  /** Usuario utilizado para conectar a la base de datos actual */
  QString user_;
  QString remote_user_;

  /** Nombre de la base de datos actual */
  QString database_;
  QString remote_database_;

  /** Contrase�a utilizada para conectar a la base de datos actual */
  QString password_;

  /** Direcci�n del servidor donde se encuentra la base de datos actual */
  QString host_;

  /** Puerto TCP utlizado para conectar al servidor de la base de datos actual */
  int port_;

  /**  Nombre interno del driver utilizado para conectar a la base de datos actual */
  QString driverName_;

  /** Nombre de la conexion, ver FLSqlConnections */
  QString connectionName_;

  /** Manejador general */
  FLManager *manager_;

  /** Manejador de m�dulos */
  FLManagerModules *managerModules_;

  /** Indica si el driver de la base de datos puede interactuar con el GUI, por defecto activado */
  bool interactiveGUI_;

  /** Indica si el driver puede lanzar excepciones a los scripts QSA, por defecto activado */
  bool qsaExceptions_;

private:
  friend class FLSqlCursor;

  int transaction_;
  QPtrStack<FLSqlSavePoint> *stackSavePoints_;
  QPtrQueue<FLSqlSavePoint> *queueSavePoints_;
  FLSqlSavePoint *currentSavePoint_;
  QGuardedPtr<FLSqlCursor> lastActiveCursor_;

  bool doTransaction(FLSqlCursor *cur);
  bool doCommit(FLSqlCursor *cur, bool notify = true);
  bool doRollback(FLSqlCursor *cur);
  void initInternal();
  void finishInternal();
};

#endif
