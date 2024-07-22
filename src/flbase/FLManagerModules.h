/***************************************************************************
                    FLManagerModules.h  -  description
                          -------------------
 begin                : mie dic 24 2003
 copyright            : (C) 2003-2004 by InfoSiAL, S.L.
 email                : mail@infosial.com
***************************************************************************/

/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#ifndef FLMANAGERMODULES_H
#define FLMANAGERMODULES_H

#include "AQGlobal.h"

class FLAction;
class FLInfoMod;
class FLSqlDatabase;
class FLApplication;
struct AQStaticBdInfo;

/**
Gestor de m�dulos.

Esta clase permite realizar las funciones b�sicas de manejo de ficheros
de texto que forman parte de los m�dulos de aplicaci�n, utilizando como
soporte de almacenamiento la base de datos y el sistema de cach�s de texto
para optimizar las lecturas.

Gestiona la carga y descarga de m�dulos. Mantiene cual es el m�dulo activo.
El m�dulo activo se puede establecer en cualquier momento con
FLManagerModules::setActiveIdModule().

Los m�dulos se engloban en �reas (FACTURACION, FINANCIERA, PRODUCCION, etc..) y
cada m�dulo tiene varios ficheros de texto XML y scripts. Toda la estructura de
m�dulos se almacena en las tablas flareas, flmodulos, flserial y flfiles, sirviendo esta
clase como interfaz para el manejo de dicha estructura en el entorno de trabajo
de AbanQ.

@author InfoSiAL S.L.
*/
class AQ_EXPORT FLManagerModules
{

  friend class FLSqlDatabase;
  friend class FLApplication;

protected:

  /**
  constructor
  */
  FLManagerModules(FLSqlDatabase *db);

public:

  /**
  constructor
  */
  explicit FLManagerModules();

  /**
  destructor
  */
  ~FLManagerModules();

public:

  /**
  Acciones de inicializaci�n del sistema de m�dulos.
  */
  void init();

  /**
  Acciones de finalizaci�n del sistema de m�dulos.
  */
  void finish();

  /**
  Obtiene el contenido de un fichero almacenado la base de datos.

  Este m�todo busca el contenido del fichero solicitado en la
  base de datos, exactamente en la tabla flfiles, si no lo encuentra
  intenta obtenerlo del sistema de ficheros.

  @param n Nombre del fichero.
  @return QString con el contenido del fichero o vac�a en caso de error.
  */
  QString content(const QString &n, const bool only_fs = false);

  /**
  Obtiene el contenido de un fichero de script, proces�ndolo para cambiar las conexiones que contenga,
  de forma que al acabar la ejecuci�n de la funci�n conectada se reanude el gui�n de pruebas.
  Tambien realiza procesos de formateo del c�digo para optimizarlo.

  @param n Nombre del fichero.
  @return QString con el contenido del fichero o vac�a en caso de error.
  */
  QString byteCodeToStr(const QByteArray &byteCode) const;
  QString contentCode(const QString &n);

  /**
  Obtiene el contenido de un fichero almacenado en el sistema de ficheros.

  @param pN Ruta y nombre del fichero en el sistema de ficheros
  @return QString con el contenido del fichero o vac�a en caso de error.
  */
  static QString contentFS(const QString &pN);

  /**
  Obtiene el contenido de un fichero, utilizando la cach� de memoria y disco.

  Este m�todo primero busca el contenido del fichero solicitado en la
  cach� interna, si no est� lo obtiene con el m�todo FLManagerModules::content().

  @param n Nombre del fichero.
  @return QString con el contenido del fichero o vac�a en caso de error.
  */
  QString contentCached(const QString &n, QString *shaKey = 0);

  /**
  Almacena el contenido de un fichero en un m�dulo dado.

  @param n Nombre del fichero.
  @param idM Identificador del m�dulo al que se asociar� el fichero
  @param content Contenido del fichero.
  */
  void setContent(const QString &n, const QString &idM, const QString &content);

  /**
  Crea un formulario a partir de su fichero de descripci�n.

  Utiliza el m�todo FLManagerModules::contentCached() para obtener el texto XML que describe
  el formulario.

  @param n Nombre del fichero que contiene la descricpci�n del formulario.
  @return QWidget correspondiente al formulario construido.
  */
  QWidget *createUI(const QString &n, QObject *connector = 0, QWidget *parent = 0, const char *name = 0);

  /**
  Crea el formulario maestro de una acci�n a partir de su fichero de descripci�n.

  Utiliza el m�todo FLManagerModules::createUI() para obtener el formulario construido.

  @param a Objeto FLAction.
  @return QWidget correspondiente al formulario construido.
  */
  QWidget *createForm(const FLAction *a, QObject *connector = 0, QWidget *parent = 0, const char *name = 0);

  /**
  Esta funci�n es igual a la anterior, s�lo se diferencia en que carga
  la descripci�n de interfaz del formulario de edici�n de registros.
  */
  QWidget *createFormRecord(const FLAction *a, QObject *connector = 0, QWidget *parent = 0, const char *name = 0);

  /**
  Para establecer el m�dulo activo.

  Autom�ticamente tambi�n establece cual es el �rea correspondiente al m�dulo,
  ya que un m�dulo s�lo puede pertenecer a una sola �rea.

  @param id Identificador del m�dulo
  */
  void setActiveIdModule(const QString &id);

  /**
  Para obtener el area del m�dulo activo.

  @return Identificador del area
  */
  QString activeIdArea() const {
    return activeIdArea_;
  }

  /**
  Para obtener el m�dulo activo.

  @return Identificador del m�dulo
  */
  QString activeIdModule() const {
    return activeIdModule_;
  }

  /**
  Obtiene la lista de identificadores de area cargadas en el sistema.

  @return Lista de identificadores de areas
  */
  QStringList listIdAreas();

  /**
  Obtiene la lista de identificadores de m�dulos cargados en el sistema de una area dada.

  @param idA Identificador del �rea de la que se quiere obtener la lista m�dulos
  @return Lista de identificadores de m�dulos
  */
  QStringList listIdModules(const QString &idA);

  /**
  Obtiene la lista de identificadores de todos los m�dulos cargados en el sistema.

  @return Lista de identificadores de m�dulos
  */
  QStringList listAllIdModules();

  /**
  Obtiene la descripci�n de un �rea a partir de su identificador.

  @param idA Identificador del �rea.
  @return Texto de descripci�n del �rea, si lo encuentra o idA si no lo encuentra.
  */
  QString idAreaToDescription(const QString &idA);

  /**
  Obtiene la descripci�n de un m�dulo a partir de su identificador.

  @param idM Identificador del m�dulo.
  @return Texto de descripci�n del m�dulo, si lo encuentra o idM si no lo encuentra.
  */
  QString idModuleToDescription(const QString &idM);

  /**
  Para obtener el icono asociado a un m�dulo.

  @param idM Identificador del m�dulo del que obtener el icono
  @return QPixmap con el icono
  */
  QPixmap iconModule(const QString &idM);

  /**
  Para obtener la versi�n de un m�dulo.

  @param idM Identificador del m�dulo del que se quiere saber su versi�n
  @return Cadena con la versi�n
  */
  QString versionModule(const QString &idM);

  /**
  Para obtener la clave sha local.

  @return Clave sha de la versi�n de los m�dulos cargados localmente
  */
  QString shaLocal();

  /**
  Para obtener la clave sha global.

  @return Clave sha de la versi�n de los m�dulos cargados globalmente
  */
  QString shaGlobal();

  /**
  Establece el valor de la clave sha local con el del global.
  */
  void setShaLocalFromGlobal();

  /**
  Obtiene la clave sha asociada a un fichero almacenado.

  @param n Nombre del fichero
  @return Clave sh asociada al ficheros
  */
  QString shaOfFile(const QString &n);

  /**
  Carga en el diccionario de claves las claves sha1 de los ficheros
  */
  void loadKeyFiles();

  /**
  Carga la lista de todos los identificadores de m�dulos
  */
  void loadAllIdModules();

  /**
  Carga la lista de todos los identificadores de areas
  */
  void loadIdAreas();
  
  /**
  Comprueba las firmas para un modulo dado
  */
  void checkSignatures(FLInfoMod *);

  /**
  Para obtener el identificador del m�dulo al que pertenece un fichero dado.

  @param n Nombre del fichero incluida la extensi�n
  @return Identificador del m�dulo al que pertenece el fichero
  */
  QString idModuleOfFile(const QString &n);

protected:

  /**
  Guarda el estado del sistema de m�dulos
  */
  void writeState();

  /**
  Lee el estado del sistema de m�dulos
  */
  void readState();

private:

  /**
  Mantiene el identificador del area a la que pertenece el m�dulo activo.
  */
  QString activeIdArea_;

  /**
  Mantiene el identificador del m�dulo activo.
  */
  QString activeIdModule_;

  /**
  Mantiene la clave sha correspondiente a la version de los m�dulos cargados localmente
  */
  QString shaLocal_;

  /**
  Diccionario de claves de ficheros, para optimizar lecturas
  */
  QDict<QString> *dictKeyFiles;

  /**
  Lista de todos los identificadores de m�dulos cargados, para optimizar lecturas
  */
  QStringList *listAllIdModules_;

  /**
  Lista de todas los identificadores de areas cargadas, para optimizar lecturas
  */
  QStringList *listIdAreas_;

  /**
  Diccionario con informaci�n de los m�dulos
  */
  QDict<FLInfoMod> *dictInfoMods;

  /**
  Diccionario de identificadores de modulo de ficheros, para optimizar lecturas
  */
  QDict<QString> *dictModFiles;

  /**
  Base de datos a utilizar por el manejador
  */
  FLSqlDatabase *db_;

  /**
  Uso interno.
  Obtiene el contenido de un fichero mediante la carga estatica desde el disco local

  @param n Nombre del fichero.
  @return QString con el contenido del fichero o vac�a en caso de error.
  */
  QString contentStatic(const QString &n);

  /**
  Uso interno.
  Muestra cuadro de dialogo para configurar la carga estatica desde el disco local
  */
  void staticLoaderSetup();

  /**
  Uso interno.
  Informacion para la carga estatica desde el disco local
  */
  AQStaticBdInfo *staticBdInfo_;

  /**
  Uso interno
  */
  QString rootDir_,
          scriptsDir_,
          tablesDir_,
          formsDir_,
          reportsDir_,
          queriesDir_,
          transDir_;
};

#endif
