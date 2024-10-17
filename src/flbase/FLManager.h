/***************************************************************************
                             FLManager.h
                          -------------------
 begin                : Sun Jul 15 2001
 copyright            : (C) 2001-2005 by InfoSiAL S.L.
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

#ifndef FLMANAGER_H
#define FLMANAGER_H

#include "FLTableMetaData.h"

class QDomElement;
class FLFieldMetaData;
class FLRelationMetaData;
class FLCompoundKey;
class FLSqlQuery;
class FLParameterQuery;
class FLGroupByQuery;
class FLAction;
class FLSqlDatabase;

/**
Esta clase sirve como administrador de la base de datos.

Encargada de abrir los formularios u obtener sus definiciones (ficheros .ui).
Tambien mantiene los metadatos de todas la tablas de la base de
datos, ofreciendo la posibilidad de recuperar los metadatos
mediante objetos FLTableMetaData de una tabla dada.

@author InfoSiAL S.L.
*/
class AQ_EXPORT FLManager
{

  friend class FLSqlDatabase;

protected:

  /**
  constructor
  */
  FLManager(FLSqlDatabase *db);

public:

  /**
  constructor
  */
  explicit FLManager();

  /**
  destructor
  */
  ~FLManager();

  /**
  Acciones de inicializaci�n.
  */
  void init();

  /**
  Acciones de finalizaci�n.
  */
  void finish();

  /**
  Para obtener definicion de una tabla de la base de datos, a partir de un fichero XML.

  El nombre de la tabla corresponde con el nombre del fichero mas la extensi�n ".mtd"
  que contiene en XML la descripci�n de la tablas. Este m�todo escanea el fichero
  y construye/devuelve el objeto FLTableMetaData correspondiente, adem�s
  realiza una copia de estos metadatos en una tabla de la misma base de datos
  para poder determinar cuando ha sido modificados y as�, si es necesario, reconstruir
  la tabla para que se adapte a la nuevos metadatos. NO SE HACEN
  CHEQUEOS DE ERRORES SINT�CTICOS EN EL XML.

  IMPORTANTE :Para que estos m�todos funcionen correctamente, es estrictamente
        necesario haber creado la base de datos en PostgreSQL con codificaci�n
        UNICODE; "createdb -E UNICODE abanq".

  @param n Nombre de la tabla de la base de datos de la que obtener los metadatos
  @param quick Si TRUE no realiza chequeos, usar con cuidado
  @return Un objeto FLTableMetaData con los metadatos de la tabla solicitada
  */
  FLTableMetaData *metadata(const QString &n, bool quick = false);
  FLTableMetaData *metadataDev(const QString &n, bool quick = false);

  /**
  Para obtener una consulta de la base de datos, a partir de un fichero XML.

  El nombre de la consulta corresponde con el nombre del fichero mas la extensi�n ".qry"
  que contiene en XML la descripci�n de la consulta. Este m�todo escanea el fichero
  y construye/devuelve el objeto FLSqlQuery. NO SE HACEN
  CHEQUEOS DE ERRORES SINT�CTICOS EN EL XML.

  @param n Nombre de la consulta de la base de datos que se quiere obtener
  @return Un objeto FLSqlQuery que representa a la consulta que se quiere obtener
  */
  FLSqlQuery *query(const QString &n, QObject *parent = 0);

  /**
  Obtiene la definici�n de una acci�n a partir de su nombre.

  Este m�todo busca en los [id_modulo].xml la acci�n que se le pasa
  como nombre y construye y devuelve el objeto FLAction correspondiente.
  NO SE HACEN CHEQUEOS DE ERRORES SINT�CTICOS EN EL XML.

  @param n Nombre de la accion
  @return Un objeto FLAction con la descripcion de la accion
  */
  FLAction *action(const QString &n);

  /**
  Comprueba si existe la tabla especificada en la base de datos.

  @param n      Nombre de la tabla que se quiere comprobar si existe
  @param cache  Si cierto consulta primero la cache de tablas, en caso contrario
                realiza una consulta a la base para obtener las tablas existentes
  @return TRUE si existe la tabla, FALSE en caso contrario
  */
  bool existsTable(const QString &n, bool cache = true) const;

  /**
  Esta funci�n es esencialmente igual a la anterior, se proporciona por conveniencia.

  Compara los metadatos de dos tablas,  la definici�n en XML de esas dos tablas se
  pasan como dos cadenas de caracteres.

  @param mtd1 Cadena de caracteres con XML que describe la primera tabla
  @param mtd2 Cadena de caracteres con XML que describe la primera tabla
  @return TRUE si las dos descripciones son iguales, y FALSE en caso contrario
  */
  bool checkMetaData(const QString &mtd1, const QString &mtd2) {
    return (!QString::compare(mtd1, mtd2));
  }

  /**
  Esta funci�n es esencialmente igual a la anterior, se proporciona por conveniencia.
  */
  bool checkMetaData(FLTableMetaData *tmd1, FLTableMetaData *tmd2);

  /**
  Modifica la estructura o metadatos de una tabla, preservando los posibles datos
  que pueda contener.

  Seg�n la definici�n existente en un momento dado de los metadatos en el fichero .mtd, este
  m�todo reconstruye la tabla con esos metadatos sin la p�rdida de informaci�n o datos,
  que pudieran existir en ese momento en la tabla.

  @param n Nombre de la tabla a reconstruir
  @return TRUE si la modificaci�n tuvo �xito
  */
  bool alterTable(const QString &n);

  /**
  Esta funci�n es esencialmente igual a la anterior, se proporciona por conveniencia.

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
  Crea una tabla en la base de datos.

  @param n Nombre de la tabla que se quiere crear
  @return Un objeto FLTableMetaData con los metadatos de la tabla que se ha creado, o
      0 si no se pudo crear la tabla o ya exist�a
  */
  FLTableMetaData *createTable(const QString &n);

  /**
  Crea una tabla en la base de datos.

  @param tmd Metadatos de la tabla
  @return Un objeto FLTableMetaData con los metadatos de la tabla que se ha creado, o
      0 si no se pudo crear la tabla o ya exist�a
  */
  FLTableMetaData *createTable(FLTableMetaData *tmd);

  /**
  Devuelve el contenido del valor de de un campo formateado para ser reconocido
  por la base de datos actual en condiciones LIKE, dentro de la clausura WHERE de SQL.

  Este m�todo toma como parametros los metadatos del campo definidos con
  FLFieldMetaData. Adem�s de TRUE y FALSE como posibles valores de un campo
  l�gico tambi�n acepta los valores S� y No (o su traducci�n al idioma correspondiente).
  Las fechas son adaptadas al forma AAAA-MM-DD, que es el formato reconocido por PostgreSQL .

  @param fMD Objeto FLFieldMetaData que describre los metadatos para el campo
  @param v Valor que se quiere formatear para el campo indicado
  @param upper Si TRUE convierte a may�sculas el valor (si es de tipo cadena)
  */
  QString formatValueLike(FLFieldMetaData *fMD, const QVariant &v, const bool upper = false);
  QString formatAssignValueLike(FLFieldMetaData *fMD, const QVariant &v, const bool upper = false);
  QString formatAssignValueLike(const QString &fieldName, FLFieldMetaData *fMD, const QVariant &v, const bool upper = false);
  QString formatAssignValueLike(const QString &fieldName, int t, const QVariant &v, const bool upper = false, const bool searchable = false);

  /**
  Este m�todo hace lo mismo que el anterior, y se suministra por conveniencia.

  @param t Tipo de datos del valor
  @param v Valor que se quiere formatear para el campo indicado
  @param upper Si TRUE convierte a may�sculas el valor (si es de tipo cadena)
  */
  QString formatValueLike(int t, const QVariant &v, const bool upper = false);

  /**
  Devuelve el contenido del valor de de un campo formateado para ser reconocido
  por la base de datos actual, dentro de la clausura WHERE de SQL.

  Este m�todo toma como parametros los metadatos del campo definidos con
  FLFieldMetaData. Adem�s de TRUE y FALSE como posibles valores de un campo
  l�gico tambi�n acepta los valores S� y No (o su traducci�n al idioma correspondiente).
  Las fechas son adaptadas al forma AAAA-MM-DD, que es el formato reconocido por PostgreSQL .

  @param fMD Objeto FLFieldMetaData que describre los metadatos para el campo
  @param v Valor que se quiere formatear para el campo indicado
  @param upper Si TRUE convierte a may�sculas el valor (si es de tipo cadena)
  */
  QString formatValue(FLFieldMetaData *fMD, const QVariant &v, const bool upper = false);
  QString formatAssignValue(FLFieldMetaData *fMD, const QVariant &v, const bool upper = false);
  QString formatAssignValue(const QString &fieldName, FLFieldMetaData *fMD, const QVariant &v, const bool upper = false);
  QString formatAssignValue(const QString &fieldName, int t, const QVariant &v, const bool upper = false);

  /**
  Este m�todo hace lo mismo que el anterior, y se suministra por conveniencia.

  @param t Tipo de datos del valor
  @param v Valor que se quiere formatear para el campo indicado
  @param upper Si TRUE convierte a may�sculas el valor (si es de tipo cadena)
  */
  QString formatValue(int t, const QVariant &v, const bool upper = false);

  /**
  Crea un objeto FLTableMetaData a partir de un elemento XML.

  Dado un elemento XML, que contiene la descripci�n de una
  tablas, construye y devuelve el objeto FLTableMetaData correspondiente.
  NO SE HACEN CHEQUEOS DE ERRORES SINT�CTICOS EN EL XML.

  @param mtd Elemento XML con la descripci�n de la tabla
  @param quick Si TRUE no realiza chequeos, usar con cuidado
  @return Objeto FLTableMetaData que contiene la descrici�n de la relaci�n
  */
  FLTableMetaData *metadata(QDomElement *mtd, bool quick = false);

  /**
  Crea un objeto FLFieldMetaData a partir de un elemento XML.

  Dado un elemento XML, que contiene la descripci�n de un
  campo de una tabla construye y agrega a una lista de descripciones
  de campos el objeto FLFieldMetaData correspondiente, que contiene
  dicha definici�n del campo. Tambien lo agrega a una lista de claves
  compuesta, si el campo construido pertenece a una clave compuesta.
  NO SE HACEN CHEQUEOS DE ERRORES SINT�CTICOS EN EL XML.

  @param field Elemento XML con la descripci�n del campo
  @param v Valor utilizado por defecto para la propiedad visible
  @param ed Valor utilizado por defecto para la propiedad editable
  @return Objeto FLFieldMetaData que contiene la descripci�n del campo
  */
  FLFieldMetaData *metadataField(QDomElement *field, bool v = true, bool ed = true);

  /**
  Crea un objeto FLRelationMetaData a partir de un elemento XML.

  Dado un elemento XML, que contiene la descripci�n de una
  relaci�n entre tablas, construye y devuelve el objeto FLRelationMetaData
  correspondiente, que contiene dicha definici�n de la relaci�n.
  NO SE HACEN CHEQUEOS DE ERRORES SINT�CTICOS EN EL XML.

  @param relation Elemento XML con la descripci�n de la relaci�n
  @return Objeto FLRelationMetaData que contiene la descripci�n de la relaci�n
  */
  FLRelationMetaData *metadataRelation(QDomElement *relation);

  /**
  Crea un objeto FLParameterQuery a partir de un elemento XML.

  Dado un elemento XML, que contiene la descripci�n de una
  par�metro de una consulta, construye y devuelve el objeto FLParameterQuery
  correspondiente.
  NO SE HACEN CHEQUEOS DE ERRORES SINT�CTICOS EN EL XML.

  @param parameter Elemento XML con la descripci�n del par�metro de una consulta
  @return Objeto FLParameterQuery que contiene la descrici�n del par�metro
  */
  FLParameterQuery *queryParameter(QDomElement *parameter);

  /**
  Crea un objeto FLGroupByQuery a partir de un elemento XML.

  Dado un elemento XML, que contiene la descripci�n de un nivel de agrupamiento
  de una consulta, construye y devuelve el objeto FLGroupByQuery correspondiente.
  NO SE HACEN CHEQUEOS DE ERRORES SINT�CTICOS EN EL XML.

  @param group Elemento XML con la descripci�n del nivel de agrupamiento de una consulta.
  @return Objeto FLGroupByQuery que contiene la descrici�n del nivel de agrupamiento
  */
  FLGroupByQuery *queryGroup(QDomElement *group);

  /**
  Crea una tabla del sistema.

  Este m�todo lee directamente de disco el fichero con la descripci�n de una tabla
  del sistema y la crea en la base de datos. Su uso normal es para inicializar
  el sistema con tablas iniciales.

  @param n Nombre de la tabla.
  @return Un objeto FLTableMetaData con los metadatos de la tabla que se ha creado, o
      0 si no se pudo crear la tabla o ya exist�a
  */
  FLTableMetaData *createSystemTable(const QString &n);

  /**
  Carga en la lista de tablas los nombres de las tablas de la base de datos
  */
  void loadTables();

  /**
  Limpieza la tabla flmetadata, actualiza el cotenido xml con el de los fichero .mtd
  actualmente cargados
  */
  void cleanupMetaData();

  /**
  Para saber si la tabla dada es una tabla de sistema.

  @param n Nombre de la tabla.
  @return TRUE si es una tabla de sistema
  */
  bool isSystemTable(const QString &n);

  /**
  Utilizado para almacenar valores grandes de campos en tablas separadas indexadas
  por claves SHA del contenido del valor.

  Se utiliza para optimizar consultas que incluyen campos con valores grandes,
  como por ejemplo im�genes, para manejar en las consulta SQL la referencia al valor
  que es de tama�o constante en vez del valor en s�. Esto disminuye el tr�fico al
  reducir considerablemente el tama�o de los registros obtenidos.

  Las consultas pueden utilizar una referencia y obtener su valor s�lo cuando se
  necesita mediante FLManager::fetchLargeValue().

  @param mtd Metadatos de la tabla que contiene el campo
  @param largeValue Valor de gran tama�o del campo
  @return Clave de referencia al valor
  */
  QString storeLargeValue(FLTableMetaData *mtd, const QString &largeValue);

  /**
  Obtiene el valor de gran tama�o segun su clave de referencia.

  @param refKey Clave de referencia. Esta clave se suele obtener mediante FLManager::storeLargeValue
  @return Valor de gran tama�o almacenado
  */
  QVariant fetchLargeValue(const QString &refKey) const;

  /**
  Uso interno. Indica el n�mero de veces que se ha llamado a FLManager::init().
  */
  int initCount() const {
    return initCount_;
  }

  bool initCacheLite();

  void checkTablaCache(FLTableMetaData *tmd);

  void insertMetadataCache(QString &name, FLTableMetaData *tmd);

  FLTableMetaData *getMetadataCache(const QString &name);

  bool isMandatoryQuery(QString &query);

  QString resolveMandatoryValues(QString &query);

private:

  /**
  Lista de las tablas de la base de datos, para optimizar lecturas
  */
  QStringList *listTables_;

  /**
  Diccionario de claves de metadatos, para optimizar lecturas
  */
  QDict<QString> *dictKeyMetaData_;


  /**
  Cach� de metadatos, para optimizar lecturas
  */
  QDict<FLTableMetaData> *cacheMetaData_;

  /**
  Cach� de definiciones de acciones, para optimizar lecturas
  */
  QDict<FLAction> *cacheAction_;

  /**
  Cach� de metadatos de talblas del sistema para optimizar lecturas
  */
  QDict<FLTableMetaData> *cacheMetaDataSys_;

  /**
  Base de datos a utilizar por el manejador
  */
  FLSqlDatabase *db_;
  /**
  Indica el n�mero de veces que se ha llamado a FLManager::init()
  */
  int initCount_;
};

#endif
