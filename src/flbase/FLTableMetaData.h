/***************************************************************************
                           FLTableMetaData.h
                          -------------------
 begin                : Mon Jul 2 2001
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

#ifndef FLTABLEMETADATA_H
#define FLTABLEMETADATA_H

#include <qobject.h>
#include <qdict.h>
#include <qstringlist.h>

#include "AQGlobal.h"

class FLFieldMetaData;
class FLRelationMetaData;
class FLCompoundKey;
class FLTableMetaDataPrivate;

/**
Mantiene la definicion de una tabla.

Esta clase mantienen la definicion de
ciertas caracteristicas de una tabla de la base
de datos.

Adicionalmente puede ser utilizada para la definici�n de
los metadatos de una consulta, ver FLTableMetaData::query().

@author InfoSiAL S.L.
*/
class AQ_EXPORT FLTableMetaData : public QObject
{
  Q_OBJECT

public:

#ifdef FL_DEBUG
  static long count_;
#endif

  /**
  constructor

  @param n Nombre de la tabla a definir
  @param a Alias de la tabla, utilizado en formularios
  @param q (Opcional) Nombre de la consulta de la que define sus metadatos
  */
  FLTableMetaData(const QString &n, const QString &a, const QString &q = QString::null);
  FLTableMetaData(const FLTableMetaData *other);

  /**
  destructor
  */
  ~FLTableMetaData();

  /**
  Tipo de datos lista de campos
  */
  typedef QDict<FLFieldMetaData> FLFieldMetaDataList;

  /**
  Obtiene el nombre de la tabla

  @return El nombre de la tabla que se describe
  */
  QString name() const;

  /**
  Establece el nombre de la tabla

  @param n Nombre de la tabla
  */
  void setName(const QString &n);

  /**
  Establece el alias

  @param a Alias
  */
  void setAlias(const QString &a);

  /**
  Establece el nombre de la consulta

  @param q Nombre de la consulta
  */
  void setQuery(const QString &q);

  /**
  Obtiene el alias asociado a la tabla
  */
  QString alias() const;

  /**
  Obtiene el nombre de la consulta de la que define sus metadatos.

  El nombre corresponder� a la definici�n de una consulta mediante
  (fichero .qry). Si el nombre de la consulta est� definido entonces
  el nombre de la tabla correponder� a la tabla principal de la consulta
  cuando esta referencie a varias tablas.
  */
  QString query() const;

  /**
  Obtiene si define los metadatos de una consulta
  */
  bool isQuery() const;

  /**
  A�ade la descripci�n de un campo a lista de descripciones de campos.

  @param f Objeto FLFieldMetaData con la descripci�n del campo a a�adir
  */
  void addFieldMD(FLFieldMetaData *f);

  /**
  Elimina la descripci�n de un campo de la lista de descripciones de campos.

  @param fN Nombre del campo a eliminar
  */
  void removeFieldMD(const QString &fN);

  /**
  Establece la clave compuesta de esta tabla.

  @param cK Objeto FLCompoundKey con la descripci�n de la clave compuesta
  */
  void setCompoundKey(FLCompoundKey *cK);

  /**
  Obtiene el nombre del campo que es clave primaria para esta tabla.

  @param prefixTable Si es TRUE se a�ade un prefijo con el nombre de la tabla; nombretabla.nombrecampo
  */
  QString primaryKey(bool prefixTable = false) const;

  /**
  Obtiene el alias de un campo a partir de su nombre.

  @param fN Nombre del campo
  */
  QString fieldNameToAlias(const QString &fN) const;

  /**
  Obtiene el nombre de un campo a partir de su alias.

  @param aN Nombre del alias del campo
  */
  QString fieldAliasToName(const QString &aN) const;

  /**
  Obtiene el tipo de un campo a partir de su nombre.

  @param fN Nombre del campo
  */
  int fieldType(const QString &fN) const;

  /**
  Obtiene si un campo es clave primaria partir de su nombre.

  @param fN Nombre del campo
  */
  bool fieldIsPrimaryKey(const QString &fN) const;

  /**
  Obtiene si un campo es �ndice a partir de su nombre.

  @param fN Nombre del campo
  */
  bool fieldIsIndex(const QString &fN) const;

  /**
  Obtiene si un campo es contador.

  @param fN Nombre del campo
  @author Andr�s Ot�n Urbano (baxas@eresmas.com)
  */
  bool fieldIsCounter(const QString &fN) const;

  /**
  Obtiene si un campo puede ser nulo

  @param fN Nombre del campo
  */
  bool fieldAllowNull(const QString &fN) const;

  /**
  Obtiene si un campo es �nico a partir de su nombre.

  @param fN Nombre del campo
  */
  bool fieldIsUnique(const QString &fN) const;

    /**
  Obtiene si un campo es searchable a partir de su nombre.

  @param fN Nombre del campo
  */
  bool fieldIsSearchable(const QString &fN) const;

  /**
  Obtiene el nombre de la tabla for�nea relacionada con un campo de esta tabla mediante
  una relacion M1 (muchos a uno).

  @param fN Campo de la relacion M1 de esta tabla, que se supone que esta relacionado
        con otro campo de otra tabla
  @return El nombre de la tabla relacionada M1, si hay relacion para el campo, o una cadena
      vacia sin el campo no est� relacionado
  */
  QString fieldTableM1(const QString &fN) const;

  /**
  Obtiene el nombre del campo de la tabla for�nea relacionado con el indicado mediante
  una relacion M1 (muchos auno).

  @param fN Campo de la relacion M1 de esta tabla, que se supone que esta relacionado
        con otro campo de otra tabla
  @return El nombre del campo for�neo relacionado con el indicado
  */
  QString fieldForeignFieldM1(const QString &fN) const;

  /**
  Obtiene el objeto relaci�n que definen dos campos.

  @param fN Nombre del campo de esta tabla que forma parte de la relaci�n
  @param fFN Nombre del campo for�neo a esta tabla que forma parte de la relaci�n
  @param  fTN Nombre de la tabla for�nea
  @return Devuelve un objeto FLRelationMetaData con la informaci�n de la relaci�n, siempre y
      cuando esta exista. Si no existe devuelve 0
  */
  FLRelationMetaData *relation(const QString &fN, const QString &fFN, const QString &fTN) const;

  /**
  Obtiene la longitud de un campo a partir de su nombre.

  @param fN Nombre del campo
  */
  int fieldLength(const QString &fN) const;

  /**
  Obtiene el n�mero de d�gitos de la parte entera de un campo a partir de su nombre.

  @param fN Nombre del campo
  */
  int fieldPartInteger(const QString &fN) const;

  /**
  Obtiene el n�mero de d�gitos de la parte decimal de un campo a partir de su nombre.

  @param fN Nombre del campo
  */
  int fieldPartDecimal(const QString &fN) const;

  /**
  Obtiene si un campo es calculado.

  @param fN Nombre del campo
  */
  bool fieldCalculated(const QString &fN) const;

  /**
  Obtiene si un campo es visible.

  @param fN Nombre del campo
  */
  bool fieldVisible(const QString &fN) const;

  /** Obtiene los metadatos de un campo.

  @param fN Nombre del campo
  @return Un objeto FLFieldMetaData con lainformaci�n o metadatos de un campo dado
  */
  FLFieldMetaData *field(const QString &fN) const;

  /**
  Para obtener la lista de definiciones de campos.

  @return Objeto con la lista de deficiones de campos de la tabla
  */
  const FLFieldMetaDataList *fieldList() const;

  /**
  Para obtener una cadena con los nombres de los campos separados por comas.

  @param prefixTable Si es TRUE se a�ade un prefijo a cada campo con el nombre de la tabla; nombretabla.nombrecampo
  @return Cadena de caracteres con los nombres de los campos separados por comas
  */
  QString fieldList(bool prefixTable) const;

  /**
  Obtiene la lista de campos de una clave compuesta, a partir del nombre de
  un campo del que se quiere averiguar si est� en esa clave compuesta.

  @param fN Nombre del campo del que se quiere averiguar si pertenece a una clave compuesta.
  @return Si el campo pertenece a una clave compuesta, devuelve la lista de campos
      que forman dicha clave compuesta, incluido el campo consultado. En el caso
      que el campo consultado no pertenezca a ninguna clave compuesta devuelve 0
  */
  const FLFieldMetaDataList *fieldListOfCompoundKey(const QString &fN) const;

  /**
  Obtiene una cadena de texto que contiene los nombres de los campos separados por comas.

  El orden de los campos de izquierda a derecha es el correspondiente al orden en que
  se han a�adido con el m�todo addFieldMD() o addFieldName()
  */
  QString fieldsNames() const;

  /**
  Lista de nombres de campos de la tabla que son del tipo FLFieldMetaData::Unlock
  */
  QStringList fieldsNamesUnlock() const;

  /**
  @return El indicador FLTableMetaData::concurWarn_
  */
  bool concurWarn() const;

  /**
  Establece el indicador FLTableMetaData::concurWarn_
  */
  void setConcurWarn(bool b = true);

  /**
  @return El indicador FLTableMetaData::detectLocks_
  */
  bool detectLocks() const;

  /**
  Establece el indicador FLTableMetaData::detectLocks_
  */
  void setDetectLocks(bool b = true);

  /**
  Establece el nombre de funci�n a llamar para Full Text Search
  */
  QString FTSFunction();
  void setFTSFunction(QString ftsfun);

    /**
  Establece los campos que ser�n cacheados
  */
  bool useCachedFields();
  QStringList cachedFields();
  void setCachedFields(QString cachedFields);

  /**
  Indica si lo metadatos est�n en cach� (FLManager::cacheMetaData_)
  */
  bool inCache() const;

  /**
  Establece si lo metadatos est�n en cach� (FLManager::cacheMetaData_)
  */
  void setInCache(bool b = true);

private:

  /**
  Privado
  */
  FLTableMetaDataPrivate *d;

  void copy(const FLTableMetaData *other);
};

class FLTableMetaDataPrivate
{
public:

  FLTableMetaDataPrivate(const QString &n, const QString &a, const QString &q);
  FLTableMetaDataPrivate();

  ~FLTableMetaDataPrivate();

  /**
  A�ade el nombre de un campo a la cadena de nombres de campos, ver fieldsNames()

  @param n Nombre del campo
  */
  void addFieldName(const QString &n);

  /**
  Elimina el nombre de un campo a la cadena de nombres de campos, ver fieldsNames()

  @param n Nombre del campo
  */
  void removeFieldName(const QString &n);

  /**
  Formatea el alias del campo indicado para evitar duplicados

  @param  f   Campo objeto cuyo alias se desea formatear
  */
  void formatAlias(FLFieldMetaData *f);

  /**
  Limpia la lista de definiciones de campos
  */
  void clearFieldList();

  /**
  Nombre de la tabla
  */
  QString name_;

  /**
  Alias de la tabla
  */
  QString alias_;

  /**
  Lista de campos que tiene esta tabla
  */
  FLTableMetaData::FLFieldMetaDataList *fieldList_;

  /**
  Clave compuesta que tiene esta tabla
  */
  FLCompoundKey *compoundKey_;

  /**
  Nombre de la consulta (fichero .qry) de la que define los metadatos
  */
  QString query_;

  /**
  Cadena de texto con los nombre de los campos separados por comas
  */
  QString fieldsNames_;

  /**
  Mapas alias<->nombre
  */
  QMap<QString, QString> aliasFieldMap_;
  QMap<QString, QString> fieldAliasMap_;

  /**
  Lista de nombres de campos de la tabla que son del tipo FLFieldMetaData::Unlock
  */
  QStringList fieldsNamesUnlock_;

  /**
  Clave primaria
  */
  QString primaryKey_;

  /**
  Indica si se debe avisar de colisi�n de concurrencia entre sesiones.

  Si este flag es true y dos o mas sesiones/usuarios est�n modificando los
  mismos campos,al validar un formulario (FLFormRecordDB::validateForm)
  mostrar� un aviso de advertencia.

  Ver tambi�n FLSqlCursor::concurrencyFields().
  */
  bool concurWarn_;

  /**
  Indica si se deben comprobar riesgos de bloqueos para esta tabla

  Si este flag es true FLSqlCursor::commitBuffer() chequear� siempre
  los riesgos de bloqueo para esta tabla.

  Ver tambi�n FLSqlDatabase::detectRisksLocks
  */
  bool detectLocks_;
  
  /**
  Indica el nombre de funci�n a llamar para la b�squeda con Full Text Search
  */
  QString ftsfun_;

  /**
  Indica los campos que ser�n cacheados
  */
  QStringList cF_;
  
  /**
  Indica si lo metadatos est�n en cach� (FLManager::cacheMetaData_)
  */
  bool inCache_;
};

inline void FLTableMetaData::setCompoundKey(FLCompoundKey *cK)
{
  d->compoundKey_ = cK;
}

inline bool FLTableMetaData::isQuery() const
{
  return !d->query_.isEmpty();
}

inline QString FLTableMetaData::name() const
{
  return d->name_;
}

inline QString FLTableMetaData::alias() const
{
  return d->alias_;
}

inline QString FLTableMetaData::query() const
{
  return d->query_;
}

inline const FLTableMetaData::FLFieldMetaDataList *FLTableMetaData::fieldList() const
{
  return d->fieldList_;
}

inline QString FLTableMetaData::fieldsNames() const
{
  return d->fieldsNames_;
}

inline void FLTableMetaData::setAlias(const QString &a)
{
  d->alias_ = a;
}

inline void FLTableMetaData::setQuery(const QString &q)
{
  d->query_ = q;
}

inline QStringList FLTableMetaData::fieldsNamesUnlock() const
{
  return d->fieldsNamesUnlock_;
}

inline bool FLTableMetaData::concurWarn() const
{
  return d->concurWarn_;
}

inline void FLTableMetaData::setConcurWarn(bool b)
{
  d->concurWarn_ = b;
}

inline bool FLTableMetaData::detectLocks() const
{
  return d->detectLocks_;
}

inline void FLTableMetaData::setDetectLocks(bool b)
{
  d->detectLocks_ = b;
}

inline bool FLTableMetaData::useCachedFields()
{
  return d->cF_.count() > 0;
}

inline QStringList FLTableMetaData::cachedFields()
{
  return d->cF_;
}

inline void FLTableMetaData::setCachedFields(QString cachedFields)
{
   d->cF_ = QStringList::split(",", cachedFields);
}

inline bool FLTableMetaData::inCache() const
{
  return d->inCache_;
}

inline void FLTableMetaData::setInCache(bool b)
{
  d->inCache_ = b;
}



inline QString FLTableMetaData::FTSFunction()
{
  return d->ftsfun_;
}


inline void FLTableMetaData::setFTSFunction(QString ftsfun)
{
  d->ftsfun_ = ftsfun;  
}

#endif
