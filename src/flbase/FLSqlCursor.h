/***************************************************************************
                      FLSqlCursor.h  -  description
                         -------------------
begin                : Fri Jul 27 2001
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

#ifndef FLSQLCURSOR_H
#define FLSQLCURSOR_H

#include <qvaluelist.h>
#include <qvariant.h>
#include <qsqlcursor.h>
#include <qsqlindex.h>
#include <qobject.h>
#include <qmessagebox.h>
#include <qdatetime.h>
#include <qregexp.h>
#include <qptrstack.h>
#include <qptrqueue.h>
#include <qdict.h>
#include <qvaluestack.h>
#include <qlabel.h>
#include <qguardedptr.h>

#include "AQGlobal.h"

// #define AQ_MD5_CHECK

class FLTableMetaData;
class FLRelationMetaData;
class FLAction;
class FLSqlSavePoint;
class FLSqlDatabase;
class FLSqlCursor;
class FLAccessControl;

struct AQBoolFlagState
{
  AQBoolFlagState() : modifier_(0), prevValue_(false), prev_(0), next_(0)
  {
#ifdef FL_DEBUG
    ++count_;
#endif
  }
#ifdef FL_DEBUG
  ~AQBoolFlagState()
  {
#ifdef FL_DEBUG
    --count_;
#endif
  }
  void dumpDebug() const
  {
    qWarning("%p <- (%p : [%p, %d]) -> %p", prev_, this, modifier_, prevValue_, next_);
  }
  static long count_;
#endif
  void *modifier_;
  bool prevValue_;
  AQBoolFlagState *prev_;
  AQBoolFlagState *next_;
};

struct AQBoolFlagStateList
{
  AQBoolFlagStateList() : cur_(0) {}
  ~AQBoolFlagStateList()
  {
    clear();
  }
#ifdef FL_DEBUG
  void dumpDebug() const
  {
    AQBoolFlagState *it = cur_;
    qWarning("current %p", it);
    while (it)
    {
      it->dumpDebug();
      it = it->prev_;
    }
    qWarning("AQBoolFlagState count %d", AQBoolFlagState::count_);
  }
#endif
  void clear()
  {
    while (cur_)
    {
      AQBoolFlagState *pv = cur_->prev_;
      delete cur_;
      cur_ = pv;
    }
  }
  bool isEmpty() const
  {
    return (cur_ == 0);
  }
  AQBoolFlagState *find(void *m) const
  {
    AQBoolFlagState *it = cur_;
    while (it && it->modifier_ != m)
      it = it->prev_;
    return it;
  }
  void append(AQBoolFlagState *i)
  {
    if (!cur_)
    {
      cur_ = i;
      cur_->next_ = 0;
      cur_->prev_ = 0;
      return;
    }
    cur_->next_ = i;
    i->prev_ = cur_;
    cur_ = i;
    cur_->next_ = 0;
  }
  void erase(AQBoolFlagState *i, bool del = true)
  {
    if (!cur_)
      return;
    if (cur_ == i)
    {
      if (cur_->prev_)
      {
        cur_ = cur_->prev_;
        cur_->next_ = 0;
      }
      else
        cur_ = 0;
    }
    else
    {
      if (i->next_)
        i->next_->prev_ = i->prev_;
      if (i->prev_)
        i->prev_->next_ = i->next_;
    }
    if (!del)
    {
      i->next_ = 0;
      i->prev_ = 0;
    }
    else
      delete i;
  }
  void pushOnTop(AQBoolFlagState *i)
  {
    if (cur_ == i)
      return;
    erase(i, false);
    append(i);
  }
  AQBoolFlagState *cur_;
};

class FLSqlCursorPrivate
{
public:
  FLSqlCursorPrivate();
  ~FLSqlCursorPrivate();

  /**
  Buffer con un registro del cursor.

  Seg�n el modo de acceso FLSqlCursor::Mode establecido para el cusor, este buffer contendr
  el registro activo de dicho cursor listo para insertar,editar,borrar o navegar.
  */
  QSqlRecord *buffer_;

  /**
  Copia del buffer.

  Aqui se guarda una copia del FLSqlCursor::buffer_ actual mediante el metodo FLSqlCursor::updateBufferCopy().
  */
  QSqlRecord *bufferCopy_;

  /**
  Metadatos de la tabla asociada al cursor.
  */
  QGuardedPtr<FLTableMetaData> metadata_;

  /**
  Mantiene el modo de acceso actual del cursor, ver FLSqlCursor::Mode.
  */
  int modeAccess_;

  /**
  Cursor relacionado con este.
  */
  FLSqlCursor *cursorRelation_;

  /**
  Relaci�n que determina como se relaciona con el cursor relacionado.
  */
  FLRelationMetaData *relation_;

  /**
  Esta bandera cuando es TRUE indica que se abra el formulario de edici�n de regitros en
  modo edici�n, y cuando es FALSE se consulta la bandera FLSqlCursor::browse. Por defecto esta
  bandera est� a TRUE
  */
  bool edition_;
  AQBoolFlagStateList *editionStates_;

  /**
  Esta bandera cuando es TRUE y la bandera FLSqlCuror::edition es FALSE, indica que se
  abra el formulario de edici�n de registro en modo visualizaci�n, y cuando es FALSE no hace
  nada. Por defecto esta bandera est� a TRUE
  */
  bool browse_;
  AQBoolFlagStateList *browseStates_;

  /**
  Filtro principal para el cursor.

  Este filtro persiste y se aplica al cursor durante toda su existencia,
  los filtros posteriores, siempre se ejecutaran unidos con 'AND' a este.
  */
  QString mainFilter_;

  /**
  Accion asociada al cursor, esta accion pasa a ser propiedad de FLSqlCursor, que ser� el
  encargado de destruirla
  */
  FLAction *action_;

  /**
  Cuando esta propiedad es TRUE siempre se pregunta al usuario si quiere cancelar
  cambios al editar un registro del cursor.
  */
  bool askForCancelChanges_;

  /**
  Indica si estan o no activos los chequeos de integridad referencial
  */
  bool activatedCheckIntegrity_;

  /**
  Indica si estan o no activas las acciones a realiar antes y despu�s del Commit
  */
  bool activatedCommitActions_;

  /**
  Contexto de ejecuci�n de scripts.

  El contexto de ejecuci�n ser� un objeto formulario el cual tiene asociado un script.
  Ese objeto formulario corresponde a aquel cuyo origen de datos es este cursor.
  El contexto de ejecuci�n es autom�ticamente establecido por las clases FLFormXXXX.
  */
  QObject *ctxt_;

  /**
  Cr�nometro interno
  */
  QTimer *timer_;

  /**
  Cuando el cursor proviene de una consulta indica si ya se han agregado al mismo
  la definic�n de los campos que lo componen
  */
  bool populated_;

  /**
  Cuando el cursor proviene de una consulta contiene la sentencia sql
  */
  QString query_;

  /**
  Cuando el cursor proviene de una consulta contiene la clausula order by
  */
  QString queryOrderBy_;

  /**
  Base de datos sobre la que trabaja
  */
  FLSqlDatabase *db_;

  /**
  Pila de los niveles de transacci�n que han sido iniciados por este cursor
  */
  QValueStack<int> transactionsOpened_;

  /**
  Filtro persistente para incluir en el cursor los registros recientemente insertados aunque estos no
  cumplan los filtros principales. Esto es necesario para que dichos registros sean v�lidos dentro del
  cursor y as� poder posicionarse sobre ellos durante los posibles refrescos que puedan producirse en
  el proceso de inserci�n. Este filtro se agrega a los filtros principales mediante el operador OR.
  */
  QString persistentFilter_;

  /**
   Guarda una copia de persistentFilter_ para cuando termina el cursor restaurarlo.
  */

  QString persistentFilterBeforeDelegate_;

  /**
  Cursor propietario
  */
  FLSqlCursor *cursor_;

  /**
  Nombre del cursor
  */
  QString curName_;

  /**
  Auxiliares para la comprobacion de riesgos de bloqueos
  */
  bool inLoopRisksLocks_;
  bool inRisksLocks_;
  QLabel *modalRisksLocks_;
  QTimer *timerRisksLocks_;

  /**
  Para el control de acceso din�mico en funci�n del contenido de los registros
  */
  void doAcl();
  void undoAcl();
  FLAccessControl *acTable_;
  QString acPermTable_;
  QString acPermBackupTable_;
  QStringList acosTable_;
  QStringList acosBackupTable_;
  QString acosCondName_;
  int acosCond_;
  QVariant acosCondVal_;
  int lastAt_;
  bool aclDone_;
  QStringList fieldsNamesUnlock_;
  int idAc_, idAcos_, idCond_;
  QString id_;

  /** Uso interno */
  bool isQuery_;
  bool isSysTable_;
  QMap<QString, QVariant> mapCalcFields_;
  bool rawValues_;

  void msgBoxWarning(const QString &text,
                     bool throwException = true, bool gui = true);

#ifdef AQ_MD5_CHECK
  bool needUpdate();
  QString md5Tuples_;
#endif
};

/**
Esta clase es una extensi�n de la clase QSqlCursor
para la necesidades especificas de AbanQ.

Hereda de QSqlCursor, y a�ade ciertas funcionalidades.
Esta clase se utiliza para gestionar los registros de
una sola tabla. Tambien mantiene una copia de los metadatos
(ver FLTableMetaData) de dicha tabla.

Tambi�n puede ser utilizada para manejar conjuntos de registros
provenientes de una consulta, si la definici�n de los metadatos
corresponden a una consulta. En este caso las operaciones sobre
el cursor se realizar�n sobre la tabla principal de la consulta,
ver FLTableMetaData::query().

El funcionamiento de esta clase radica en el modo de acceso
en el que se encuentre, INSERTAR,MODIFICAR,BORRAR,NAVEGAR.
Se mantiene siempre un registro completo del cursor, en un buffer,
dependiendo del modo de acceso, este buffer contendr� un registro
listo para INSERTAR en blanco, para MODIFICAR con el contenido
del registro actual del cursor, para BORRAR con el contenio del registro
actual listo para borrar, o para NAVEGAR con el contenido del registro
actual en m�do de s�lo lectura.

Las acciones asociadas a INSERTAR,MODIFICAR, o BORRAR, son llevadas
a cabo cuando se env�a el contenido del buffer de nuevo al cursor, con FLSqlCursor::commitBuffer , esta acci�n lleva implicito un chequeo de integridad para que se realize, ver FLSqlCursor::checkIntegrity.

@author InfoSiAL S.L.
*/
class AQ_EXPORT FLSqlCursor : public QObject, public QSqlCursor
{

  Q_OBJECT

public:
  /**
  constructor.

  @param cR Cursor relacionado con este. Al indicar que este cursor depende de otro,
     el contenido del cursor est� en funci�n del valor actual del campo por el que se
     establece la relaci�n del cursor relacionado
  @param r Relaci�n que indica como se relacionan los dos cursores
  @param connectionName Nombre de la conexion
  */
  FLSqlCursor(const QString &name = QString::null, bool autopopulate = true,
              const QString &connectionName = "default", FLSqlCursor *cR = 0,
              FLRelationMetaData *r = 0, QObject *parent = 0);

  /**
  Sobrecargado por conveniencia
  */
  FLSqlCursor(const QString &name, bool autopopulate, QSqlDatabase *db, FLSqlCursor *cR = 0,
              FLRelationMetaData *r = 0, QObject *parent = 0);

  /**
  destructor.
  */
  ~FLSqlCursor();

  /**
  Constantes para indicar el tipo de acceso al cursor
  */
  enum Mode
  {
    /** Insertar, en este modo el buffer se prepara para crear un nuevo registro */
    INSERT = 0,
    /** Edici�n, en este modo el buffer se prepara para editar el registro activo */
    EDIT = 1,
    /** Borrar, en este modo el buffer se prepara para borrar el registro activo */
    DEL = 2,
    /** Navegacion, en este modo solo se puede visualizar el buffer */
    BROWSE = 3
  };

  /**
  Constantes para indicar el tipo de condicion para evaluar si se aplica o no el control de acceso
  */
  enum AcosConditionEval
  {
    /** evalua un valor fijo */
    VALUE = 0,
    /** evalua una expresion regular */
    REGEXP = 1,
    /** evalua el valor devuelto por una funcion de script */
    FUNCTION = 2
  };

  /**
  Para obtener los metadatos de la tabla.

  @return Objeto FLTableMetaData con los metadatos de la tabla asociada al cursor
  */
  FLTableMetaData *metadata() const
  {
    return d->metadata_;
  }

  /**
  Para obtener el modo de acceso actual del cursor.

  @return Constante FLSqlCursor::Mode que define en que modo de acceso esta preparado
      el buffer del cursor
  */
  int modeAccess() const
  {
    return d->modeAccess_;
  }

  /**
  Para obtener el filtro principal del cursor.

  @return Cadena de texto con el filtro principal
  */
  QString mainFilter() const
  {
    return d->mainFilter_;
  }

  /**
  Para obtener la accion asociada al cursor.

  @return  Objeto FLAction
  */
  FLAction *action() const
  {
    return d->action_;
  }

  /**
  Establece la accion asociada al cursor.

  @param a Objeto FLAction
  */
  void setAction(FLAction *a)
  {
    d->action_ = a;
  }

  /**
  Establece el filtro principal del cursor.

  @param f Cadena con el filtro, corresponde con una clausura WHERE
  @param doRefresh Si TRUE tambien refresca el cursor
  */
  void setMainFilter(const QString &f, bool doRefresh = true);

  /**
  Establece el modo de acceso para el cursor.

  @param m Constante FLSqlCursor::Mode que indica en que modo de acceso
       se quiere establecer el cursor
  */
  void setModeAccess(const int m)
  {
    d->modeAccess_ = m;
  }

  /**
  Devuelve el nombre de la conexi�n que el cursor usa

  @return Nombre de la conexi�n
  */
  QString connectionName() const;

  /**
  Establece el valor de un campo del buffer de forma at�mica y fuera de transacci�n.

  Invoca a la funci�n, cuyo nombre se pasa como par�metro, del script del contexto del cursor
  (ver FLSqlCursor::ctxt_) para obtener el valor del campo. El valor es establecido en el campo de forma
  at�mica, bloqueando la fila durante la actualizaci�n. Esta actualizaci�n se hace fuera de la transacci�n
  actual, dentro de una transacci�n propia, lo que implica que el nuevo valor del campo est� inmediatamente
  disponible para las siguientes transacciones.

  @param fN Nombre del campo
  @param functionName Nombre de la funci�n a invocar del script
  */
  void setAtomicValueBuffer(const QString &fN, const QString &functionName);

  /**
  Establece el valor de un campo del buffer con un valor.

  @param fN Nombre del campo
  @param v Valor a establecer para el campo
  */
  void setValueBuffer(const QString &fN, const QVariant &v);

  /**
  Devuelve el valor de un campo del buffer.

  @param fN Nombre del campo
  */
  QVariant valueBuffer(const QString &fN);

  /**
  Devuelve el valor de un campo del buffer copiado antes de sufrir cambios.

  @param fN Nombre del campo
  */
  QVariant valueBufferCopy(const QString &fN) const;

  /**
  Establece el valor de FLSqlCursor::edition.

  @param b TRUE o FALSE
  */
  void setEdition(bool b, void *m = 0);
  void restoreEditionFlag(void *m);

  /**
  Establece el valor de FLSqlCursor::browse.

  @param b TRUE o FALSE
  */
  void setBrowse(bool b, void *m = 0);
  void restoreBrowseFlag(void *m);

  /**
  Establece el contexto de ejecuci�n de scripts

  Ver FLSqlCursor::ctxt_.

  @param c Contexto de ejecucion
  */
  void setContext(QObject *c);

  /**
  Para obtener el contexto de ejecuci�n de scripts.

  Ver FLSqlCursor::ctxt_.

  @return Contexto de ejecuci�n
  */
  QObject *context() const
  {
    return d->ctxt_;
  }

  /**
  Dice si un campo est� deshabilitado.

  Un campo estar� deshabilitado, porque esta clase le dar� un valor autom�ticamente.
  Estos campos son los que est�n en una relaci�n con otro cursor, por lo que
  su valor lo toman del campo for�neo con el que se relacionan.

  @param fN Nombre del campo a comprobar
  @return TRUE si est� deshabilitado y FALSE en caso contrario
  */
  bool fieldDisabled(const QString &fN);

  /**
  Indica si hay una transaccion en curso.

  @return TRUE si hay una transaccion en curso, FALSE en caso contrario
  */
  bool inTransaction();

  /**
  Inicia un nuevo nivel de transacci�n.

  Si ya hay una transacci�n en curso simula un nuevo nivel de anidamiento de
  transacci�n mediante un punto de salvaguarda.

  @param  lock Actualmente no se usa y no tiene ning�n efecto. Se mantiene por compatibilidad hacia atr�s
  @return TRUE si la operaci�n tuvo exito
  */
  bool transaction(bool lock = false);

  /**
  Deshace las operaciones de una transacci�n y la acaba.

  @return TRUE si la operaci�n tuvo exito
  */
  bool rollback();

  /**
  Hace efectiva la transacci�n y la acaba.

  @param notify Si TRUE emite la se�al cursorUpdated y pone el cursor en modo BROWSE,
          si FALSE no hace ninguna de estas dos cosas y emite la se�al de autoCommit
  @return TRUE si la operaci�n tuvo exito
  */
  bool commit(bool notify = true);

  /**
  Abre el formulario asociado a la tabla origen en el modo indicado.

  @param m Modo de apertura (FLSqlCursor::Mode)
  @param cont Indica que se abra el formulario de edici�n de registros con el bot�n de
         aceptar y continuar
  */
  void openFormInMode(int m, bool cont = true);

  /**
  Copia el contenido del FLSqlCursor::buffer_ actual en FLSqlCursor::bufferCopy_.

  Al realizar esta copia se podra comprobar posteriormente si el buffer actual y la copia realizada
  difieren mediante el metodo FLSqlCursor::isModifiedBuffer().
  */
  void updateBufferCopy();

  /**
  Indica si el contenido actual del buffer difiere de la copia guardada.

  Ver FLSqlCursor::bufferCopy_ .

  @return TRUE si el buffer y la copia son distintas, FALSE en caso contrario
  */
  bool isModifiedBuffer();

  /**
  Establece el valor de FLSqlCursor::askForCancelChanges_ .

  @param a Valor a establecer (TRUE o FALSE)
  */
  void setAskForCancelChanges(bool a)
  {
    d->askForCancelChanges_ = a;
  }

  /**
  Activa o desactiva los chequeos de integridad referencial.

  @param a TRUE los activa y FALSE los desactiva
  */
  void setActivatedCheckIntegrity(bool a)
  {
    d->activatedCheckIntegrity_ = a;
  }
  bool activatedCheckIntegrity() const
  {
    return d->activatedCheckIntegrity_;
  }

  /**
  Activa o desactiva las acciones a realizar antes y despu�s de un commit

  @param a TRUE las activa y FALSE las desactiva
  */
  void setActivatedCommitActions(bool a)
  {
    d->activatedCommitActions_ = a;
  }
  bool activatedCommitActions() const
  {
    return d->activatedCommitActions_;
  }

  /**
  Se comprueba la integridad referencial al intentar borrar, tambien se comprueba la no duplicidad de
  claves primarias y si hay nulos en campos que no lo permiten cuando se inserta o se edita.
  Si alguna comprobacion falla devuelve un mensaje describiendo el fallo.
  */
  QString msgCheckIntegrity();

  /**
  Realiza comprobaciones de intregidad.

  Se comprueba la integridad referencial al intentar borrar, tambien se comprueba la no duplicidad de
  claves primarias y si hay nulos en campos que no lo permiten cuando se inserta o se edita.
  Si alguna comprobacion falla muestra un cuadro de di�logo con el tipo de fallo encontrado y el metodo
  devuelve FALSE.

  @param showError Si es TRUE muestra el cuadro de dialogo con el error que se produce al no
           pasar las comprobaciones de integridad
  @return TRUE si se ha podido entregar el buffer al cursor, y FALSE si ha fallado alguna comprobacion
      de integridad
  */
  bool checkIntegrity(bool showError = true);

  /**
  Devuelve el cursor relacionado con este.
  */
  FLSqlCursor *cursorRelation() const
  {
    return d->cursorRelation_;
  }
  FLRelationMetaData *relation() const
  {
    return d->relation_;
  }

  /**
  Desbloquea el registro actual del cursor.

  @param fN Nombre del campo
  @param v Valor para el campo unlock
  */
  void setUnLock(const QString &fN, bool v);

  /**
  Para comprobar si el registro actual del cursor est� bloqueado.

  @return TRUE si est� bloqueado, FALSE en caso contrario.
  */
  bool isLocked();

  /**
  Devuelve si el contenido de un campo en el buffer es nulo.

  @param i Posicion del campo en el buffer
  */
  bool bufferIsNull(int i) const;

  /**
  Devuelve si el contenido de un campo en el buffer es nulo.

  @param name Nombre del campo en el buffer
  */
  bool bufferIsNull(const QString &name) const;

  /**
  Establece que el contenido de un campo en el buffer sea nulo.

  @param i Posicion del campo en el buffer
  */
  void bufferSetNull(int i) const;

  /**
  Establece que el contenido de un campo en el buffer sea nulo.

  @param name Nombre del campo en el buffer
  */
  void bufferSetNull(const QString &name) const;

  /**
  Devuelve si el contenido de un campo en el bufferCopy en nulo.

  @param i Posicion del campo en el bufferCopy
  */
  bool bufferCopyIsNull(int i) const;

  /**
  Devuelve si el contenido de un campo en el bufferCopy en nulo.

  @param name Nombre del campo en el bufferCopy
  */
  bool bufferCopyIsNull(const QString &name) const;

  /**
  Establece que el contenido de un campo en el bufferCopy sea nulo.

  @param i Posicion del campo en el bufferCopy
  */
  void bufferCopySetNull(int i) const;

  /**
  Establece que el contenido de un campo en el bufferCopy sea nulo.

  @param name Nombre del campo en el bufferCopy
  */
  void bufferCopySetNull(const QString &name) const;

  /**
  Obtiene la posici�n del registro actual, seg�n la clave primaria contenida en el buffer.

  La posici�n del registro actual dentro del cursor se calcula teniendo en cuenta el
  filtro actual ( FLSqlCursor::curFilter() ) y el campo o campos de ordenamiento
  del mismo ( QSqlCursor::sort() ).
  Este m�todo es �til, por ejemplo, para saber en que posici�n dentro del cursor
  se ha insertado un registro.

  @return Posici�n del registro dentro del cursor, o 0 si no encuentra coincidencia.
  */
  int atFrom();

  /**
  Obtiene la posici�n dentro del cursor del primer registro que en el campo indicado
  empieze con el valor solicitado. Supone que los registros est�n ordenados por dicho
  campo, para realizar una b�squeda binaria.

  La posici�n del registro actual dentro del cursor se calcula teniendo en cuenta el
  filtro actual ( FLSqlCursor::curFilter() ) y el campo o campos de ordenamiento
  del mismo ( QSqlCursor::sort() ).
  Este m�todo es �til, por ejemplo, para saber en que posici�n dentro del cursor
  se encuentra un registro con un cierto valor en un campo.

  @param  fN  Nombre del campo en el que buscar el valor
  @param  v   Valor a buscar ( mediante like 'v%' )
  @param  orderAsc TRUE (por defecto) si el orden es ascendente, FALSE si es descendente
  @return Posici�n del registro dentro del cursor, o 0 si no encuentra coincidencia.
  */
  int atFromBinarySearch(const QString &fN, const QString &v, bool orderAsc = true);

  /**
  Redefinido por conveniencia
  */
  bool exec(const QString &query);

#ifdef FL_DEBUG
  static long countRefCursor;
#endif

  /**
  Para obtener la base de datos sobre la que trabaja
  */
  FLSqlDatabase *db() const
  {
    return d->db_;
  }

  /**
  Para obtener el nombre del cursor (generalmente el nombre de la tabla)
  */
  QString curName() const
  {
    return d->curName_;
  }

  /**
  Para obtener el filtro por defecto en campos asociados

  @param  fieldName Nombre del campo que tiene campos asociados.
                    Debe ser el nombre de un campo de este cursor.
  @param  tableMD   Metadatos a utilizar como tabla for�nea.
                    Si es cero usa la tabla for�nea definida por la relaci�n M1 de 'fieldName'
  */
  QString filterAssoc(const QString &fieldName, FLTableMetaData *tableMD = 0);

protected:
  /**
  Redefinida
  */
  QVariant calculateField(const QString &name);

  /**
  Redefinicion del m�todo afterSeek() de QSqlCursor.
  */
  void afterSeek();

  /**
  Gestiona delegateCommit
  */

  bool lastDelegateCommitResult;
  bool isDelegateCommit;

public slots:

  /**
  Refresca el contenido del cursor.

  Si no se ha indicado cursor relacionado obtiene el cursor completo, segun la consulta
  por defecto. Si se ha indicado que depende de otro cursor con el que se relaciona,
  el contenido del cursor depender� del valor del campo que determina la relaci�n.
  Si se indica el nombre de un campo se considera que el buffer s�lo ha cambiado en ese
  campo y as� evitar repeticiones en el refresco.

  @param fN Nombre del campo de buffer que ha cambiado
  */
  void refresh(const QString &fN = QString::null);

  /**
  Actualiza el conjunto de registros con un retraso.

  Acepta un lapsus de tiempo en milisegundos, activando el cron�metro interno para
  que realize el refresh definitivo al cumplirse dicho lapsus.

  @param msec Cantidad de tiempo del lapsus, en milisegundos.
  */
  void refreshDelayed(int msec = 50);

  /**
  Refresca el buffer segun el modo de acceso establecido.

  Lleva informacion del cursor al buffer para editar o navegar, o prepara el buffer para
  insertar o borrar.

  Si existe un campo contador se invoca a la funci�n "calculateCounter" del script del
  contexto (ver FLSqlCursor::ctxt_) establecido para el cursor. A esta funci�n se le pasa
  como argumento el nombre del campo contador y debe devolver el valor que debe contener
  ese campo.

  @return TRUE si se ha podido realizar el refresco, FALSE en caso contrario
  */
  bool refreshBuffer();

  /**
  Pasa el cursor a modo Edit

  @return True si el cursor est� en modo Edit o estaba en modo Insert y ha pasado con �xito a modo Edit
  */
  bool setEditMode();

  /**
  Redefinicion del m�todo seek() de QSqlCursor.

  Este m�todo simplemente invoca al m�todo seek() original de QSqlCursor() y refresca
  el buffer con el metodo FLSqlCursor::refreshBuffer().

  @param emit Si TRUE emite la se�al FLSqlCursor::currentChanged()
  */
  bool seek(int i, bool relative = false, bool emite = false);

  /**
  Redefinicion del m�todo next() de QSqlCursor.

  Este m�todo simplemente invoca al m�todo next() original de QSqlCursor() y refresca el
  buffer con el metodo FLSqlCursor::refreshBuffer().

  @param emit Si TRUE emite la se�al FLSqlCursor::currentChanged()
  */
  bool next(bool emite = true);

  /**
  Redefinicion del m�todo prev() de QSqlCursor.

  Este m�todo simplemente invoca al m�todo prev() original de QSqlCursor() y refresca
  el buffer con el metodo FLSqlCursor::refreshBuffer().

  @param emit Si TRUE emite la se�al FLSqlCursor::currentChanged()
  */
  bool prev(bool emite = true);

  /**
  Redefinicion del m�todo first() de QSqlCursor.

  Este m�todo simplemente invoca al m�todo first() original de QSqlCursor() y refresca el
  buffer con el metodo FLSqlCursor::refreshBuffer().

  @param emit Si TRUE emite la se�al FLSqlCursor::currentChanged()
  */
  bool first(bool emite = true);

  /**
  Redefinicion del m�todo last() de QSqlCursor.

  Este m�todo simplemente invoca al m�todo last() original de QSqlCursor() y refresca el
  buffer con el metodo FLSqlCursor::refreshBuffer().

  @param emit Si TRUE emite la se�al FLSqlCursor::currentChanged()
  */
  bool last(bool emite = true);

  /**
  Redefinicion del m�todo del() de QSqlCursor.

  Este m�todo invoca al m�todo del() original de QSqlCursor() y comprueba si hay borrado
  en cascada, en caso afirmativo borrar tambi�n los registros relacionados en cardinalidad 1M.
  */
  int del(bool invalidate = true);

  /**
  Redefinicion del m�todo select() de QSqlCursor
  */
  bool select(const QString &filter, const QSqlIndex &sort = QSqlIndex());

  /**
  Redefinicion del m�todo sort() de QSqlCursor
  */
  void setSort(const QSqlIndex &sort);

  /**
  Obtiene el filtro base
  */
  QString baseFilter();

  /**
  Obtiene el filtro actual
  */
  QString curFilter();

  /**
  Redefinicion del m�todo setFilter() de QSqlCursor
  */
  void setFilter(const QString &filter);

  /**
  Abre el formulario de edicion de registro definido en los metadatos (FLTableMetaData) listo
  para insertar un nuevo registro en el cursor.
  */
  void insertRecord();

  /**
  Abre el formulario de edicion de registro definido en los metadatos (FLTableMetaData) listo
  para editar el registro activo del cursor.
  */
  void editRecord();

  /**
  Abre el formulario de edicion de registro definido en los metadatos (FLTableMetaData) listo
  para s�lo visualizar el registro activo del cursor.
  */
  void browseRecord();

  /**
  Borra, pidiendo confirmacion, el registro activo del cursor.
  */
  void deleteRecord();

  /**
  Realiza la accion de insertar un nuevo registro, y copia el valor de los campos del registro
  actual.
  */
  void copyRecord();

  /**
  Realiza la acci�n asociada a elegir un registro del cursor, por defecto se abre el formulario de
  edici�n de registro,llamando al m�todo FLSqlCursor::editRecord(), si la bandera FLSqlCursor::edition
  indica TRUE, si indica FALSE este m�todo no hace nada
  */
  void chooseRecord();

  /**
  Manda el contenido del buffer al cursor, o realiza la acci�n oportuna para el cursor.

  Todos los cambios realizados en el buffer se hacen efectivos en el cursor al invocar este m�todo.
  La manera de efectuar estos cambios viene determinada por el modo de acceso establecido para
  el cursor, ver FLSqlCursor::Mode, si el modo es editar o insertar actualiza con los nuevos valores de
  los campos del registro, si el modo es borrar borra el registro, y si el modo es navegacion no hace nada.
  Antes de nada tambien comprueba la integridad referencial invocando al m�todo FLSqlCursor::checkIntegrity().

  Si existe un campo calculado se invoca a la funci�n "calculateField" del script del
  contexto (ver FLSqlCursor::ctxt_) establecido para el cursor. A esta funci�n se le pasa
  como argumento el nombre del campo calculado y debe devolver el valor que debe contener
  ese campo, p.e. si el campo es el total de una factura y de tipo calculado la funci�n
  "calculateField" debe devolver la suma de lineas de las facturas mas/menos impuestos y
  descuentos.

  @param  emite       True para emitir se�al cursorUpdated
  @param  checkLocks  True para comprobar riesgos de bloqueos para esta tabla y el registro actual
  @return TRUE si se ha podido entregar el buffer al cursor, y FALSE si ha fallado la entrega
  */
  bool commitBuffer(bool emite = true, bool checkLocks = false);

  /**
  Manda el contenido del buffer del cursor relacionado a dicho cursor.

  Hace efectivos todos los cambios en el buffer del cursor relacionado posicon�ndose en el registro
  correspondiente que recibe los cambios.
  */
  bool commitBufferCursorRelation();

  /**
  @return El nivel actual de anidamiento de transacciones, 0 no hay transaccion
  */
  int transactionLevel();

  /**
  @return La lista con los niveles de las transacciones que ha iniciado este cursor y continuan abiertas
  */
  QStringList transactionsOpened();

  /**
  Deshace transacciones abiertas por este cursor.

  @param count  Cantidad de transacciones a deshacer, -1 todas.
  @param msg    Cadena de texto que se muestra en un cuadro de di�logo antes de deshacer las transacciones.
                Si es vac�a no muestra nada.
  */
  void rollbackOpened(int count = -1, const QString &msg = QString::null);

  /**
  Termina transacciones abiertas por este cursor.

  @param count  Cantidad de transacciones a terminar, -1 todas.
  @param msg    Cadena de texto que se muestra en un cuadro de di�logo antes de terminar las transacciones.
                Si es vac�a no muestra nada.
  */
  void commitOpened(int count = -1, const QString &msg = QString::null);

  /**
  Entra en un bucle de comprobacion de riesgos de bloqueos para esta tabla y el registro actual

  El bucle continua mientras existan bloqueos, hasta que se vuelva a llamar a este m�todo con
  'terminate' activado o cuando el usuario cancele la operaci�n.

  @param  terminate True terminar� el bucle de comprobaciones si est� activo
  */
  void checkRisksLocks(bool terminate = false);

  /**
  Establece el acceso global para la tabla, ver FLSqlCursor::setAcosCondition().

  Este ser� el permiso a aplicar a todos los campos por defecto

  @param  ac Permiso global; p.e.: "r-", "-w"
  */
  void setAcTable(const QString &ac);

  /**
  Establece la lista de control de acceso (ACOs) para los campos de la tabla, , ver FLSqlCursor::setAcosCondition().

  Esta lista de textos deber� tener en sus componentes de orden par los nombres de los campos,
  y en los componentes de orden impar el permiso a aplicar a ese campo,
  p.e.: "nombre", "r-", "descripcion", "--", "telefono", "rw",...

  Los permisos definidos aqui sobreescriben al global.

  @param acos Lista de cadenas de texto con los nombre de campos y permisos.
  */
  void setAcosTable(const QStringList &acos);

  /**
  Establece la condicion que se debe cumplir para aplicar el control de acceso.

  Para cada registro se evalua esta condicion y si se cumple, aplica la regla
  de control de acceso establecida con FLSqlCursor::setAcTable y FLSqlCursor::setAcosTable.

  Ejemplos:

    setAcosCondition( "nombre", VALUE, "pepe" ); // valueBuffer( "nombre" ) == "pepe"
    setAcosCondition( "nombre", REGEXP, "pe*" ); // QRegExp( "pe*" ).exactMatch( valueBuffer( "nombre" ).toString() )
    setAcosCondition( "sys.checkAcos", FUNCTION, true ); // call( "sys.checkAcos" ) == true

  @param  cond      Tipo de evaluacion;
                    VALUE compara con un valor fijo
                    REGEXP compara con una expresion regular
                    FUNCTION compara con el valor devuelto por una funcion de script

  @param  condName  Si es vacio no se evalua la condicion y la regla no se aplica nunca.
                    Para VALUE y REGEXP nombre de un campo.
                    Para FUNCTION nombre de una funcion de script.  A la funci�n se le pasa como
                    argumento el objeto cursor.

  @param  condVal   Valor que hace que la condicion sea cierta
  */
  void setAcosCondition(const QString &condName, AcosConditionEval cond, const QVariant &condVal);

  /**
  Comprueba si hay una colisi�n de campos editados por dos sesiones simult�neamente.

  @return Lista con los nombres de los campos que colisionan
  */
  QStringList concurrencyFields();

  /**
  Cambia el cursor a otra conexi�n de base de datos
  */
  void changeConnection(const QString &connName);

  /**
  Lanza llamada sengun proceda el deletateCommit o commitBuffer del cursorRelation.
  */
  bool doCommitBuffer();

  /**
  Lanza commit del cursor o reposiciona el cusor, sengun proceda.
  */

  bool doCommit();

  /**
   Retorna si se cumplen las condiciones para usar delegateCommit.
  */

  bool useDelegateCommit();

private:
  friend class FLDataTable;
  friend class FLSqlDatabase;

  /**
  Privado
  */
  FLSqlCursorPrivate *d;

  /**
  C�digo de inicializaci�n com�n para los constructores
  */
  void init(const QString &name, bool autopopulate,
            FLSqlCursor *cR, FLRelationMetaData *r);

  /**
  Si el cursor viene de una consulta, realiza el proceso de agregar la defici�n
  de los campos al mismo
  */
  void populateCursor();

  /**
  Cuando el cursor viene de una consulta, realiza el proceso que marca como
  no generados (no se tienen en cuenta en INSERT, EDIT, DEL) los campos del buffer
  que no pertenecen a la tabla principal
  */
  void setNotGenerateds();

  /**
  Uso interno
  */
  void setExtraFieldAttributes();
  void clearMapCalcFields();
  QVariant valueBufferRaw(const QString &fN);

signals:

  /**
  Indica que se ha cargado un nuevo buffer
  */
  void newBuffer();

  /**
  Indica ha cambiado un campo del buffer, junto con la se�al se env�a el nombre del campo que
  ha cambiado.
  */
  void bufferChanged(const QString &);

  /**
  Indica que se ha actualizado el cursor
  */
  void cursorUpdated();

  /**
  Indica que se ha elegido un registro, mediante doble clic sobre �l o bien pulsando la tecla Enter
  */
  void recordChoosed();

  /**
  Indica que la posicion del registro activo dentro del cursor ha cambiado
  */
  void currentChanged(int);

  /**
  Indica que se ha realizado un commit autom�tico para evitar bloqueos
  */
  void autoCommit();

  /**
  Indica que se ha realizado un commitBuffer
  */
  void bufferCommited();

  /**
  Indica que se ha cambiado la conexi�n de base de datos del cursor. Ver changeConnection
  */
  void connectionChanged();

  /**
  Indica que se ha realizado un commit
  */
  void commited();

private slots:

  /** Uso interno */
  void clearPersistentFilter();

  //** Uso interno */

  void restorePersistentFilterBeforeDelegate();

  //** Uso interno */

  void setPersistentFilterDelegate(const QString &filter)
};

#endif
