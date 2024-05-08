/***************************************************************************
                            FLDataTable.h
                          -------------------
 begin                : Sun Jul 1 2001
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

#ifndef FLDATATABLE_H
#define FLDATATABLE_H

#include <qdatatable.h>
#include <qpainter.h>
#include <qpixmapcache.h>
#include <qdatetime.h>
#include <qcolor.h>
#include <qcheckbox.h>

class FLSqlCursor;
class FLFieldMetaData;
class FLTableMetaData;

/**
Clase que es una redefinicion de la clase QDataTable,
especifica para las necesidades de AbanQ.

@author InfoSiAL S.L.
*/
class FLDataTable: public QDataTable
{
  Q_OBJECT

  Q_PROPERTY(QString functionGetColor READ functionGetColor WRITE setFunctionGetColor)
  Q_PROPERTY(bool onlyTable READ onlyTable WRITE setOnlyTable)
  Q_PROPERTY(bool aqReadOnly READ flReadOnly WRITE setFLReadOnly)
  Q_PROPERTY(bool editOnly READ editOnly WRITE setEditOnly)
  Q_PROPERTY(bool insertOnly READ insertOnly WRITE setInsertOnly)

public:

  /**
  constructor
  */
  FLDataTable(QWidget *parent = 0, const char *name = 0, bool popup = false);

  /**
  desctructor
  */
  ~FLDataTable();

  /**
  Establece el cursor
  */
  void setFLSqlCursor(FLSqlCursor *c);

  /**
  Establece un filtro persistente que siempre se aplica al cursor antes
  de hacer un refresh
  */
  void setPersistentFilter(const QString &pFilter);

  /**
  Devuelve el cursor
  */
  FLSqlCursor *cursor() const {
    return cursor_;
  }

  /**
  Establece la tabla a s�lo lectura o no
  */
  void setFLReadOnly(const bool mode);
  bool flReadOnly() const {
    return readonly_;
  }

  /**
  Establece la tabla a s�lo edici�n o no
  */
  void setEditOnly(const bool mode);
  bool editOnly() const {
    return editonly_;
  }

  /**
  Establece la tabla a s�lo insercion o no
  */
  void setInsertOnly(const bool mode);
  bool insertOnly() const {
    return insertonly_;
  }

  /**
  Obtiene la lista con las claves primarias de los registros seleccionados por chequeo
  */
  const QValueList<QVariant> primarysKeysChecked() const;

  /**
  Limpia la lista con las claves primarias de los registros seleccionados por chequeo
  */
  void clearChecked();

  /**
  Establece el estado seleccionado por chequeo para un regsitro, indicando el valor de su clave primaria
  */
  void setPrimaryKeyChecked(const QVariant &primaryKeyValue, bool on);

  /**
  Ver FLDataTable::showAllPixmaps_
  */
  void setShowAllPixmaps(bool s) {
    showAllPixmaps_ = s;
  }

  /**
  Ver FLDataTable::functionGetColor_
  */
  void setFunctionGetColor(const QString &f) {
    functionGetColor_ = f;
  }
  QString functionGetColor() const {
    return functionGetColor_;
  }

  /**
  Ver FLDataTable::onlyTable_
  */
  void setOnlyTable(bool on = true);
  bool onlyTable() const {
    return onlyTable_;
  }

  /**
  Redefinida por conveniencia
  */
  int indexOf(uint i) const;

  /**
  @return El nombre del campo en la tabla de una columna dada
  */
  QString fieldName(int col) const;

protected:

  /**
  Filtrado de eventos
  */
  bool eventFilter(QObject *o, QEvent *e);

  /**
  Redefinido por conveniencia para pintar la celda
  */
  void paintCell(QPainter *p, int row, int col, const QRect &cr,
                 bool selected, const QColorGroup &cg);

  /**
  Redefinido por conveniencia para pintar el campo
  */
  void paintField(QPainter *p, const QSqlField *field, const QRect &cr, bool selected);

  /**
  Redefinido por conveniencia, para evitar que aparezca el menu contextual
  con las opciones para editar registros
  */
  void contentsContextMenuEvent(QContextMenuEvent *e);

  /**
  Redefine por conveniencia, el comportamiento al hacer doble clic sobre una
  celda
  */
  void contentsMouseDoubleClickEvent(QMouseEvent *e);

  /**
  Redefinida por conveniencia
  */
  void focusOutEvent(QFocusEvent *);

  /**
  Redefinida por conveniencia
  */
  void handleError(const QSqlError &);

  /**
  Redefinida por conveniencia
  */
  void drawContents(QPainter *p, int cx, int cy, int cw, int ch);

private:

  /**
  Numero de la fila (registro) seleccionada actualmente
  */
  int rowSelected;

  /**
  Numero de la columna (campo) seleccionada actualmente
  */
  int colSelected;

  /**
  Cursor, con los registros
  */
  FLSqlCursor *cursor_;

  /**
  Almacena la tabla est� en modo s�lo lectura
  */
  bool readonly_;

  /**
  Almacena la tabla est� en modo s�lo edici�n
  */
  bool editonly_;

  /**
  Indica si la tabla est� en modo s�lo inserci�n
  */
  bool insertonly_;

  /**
  Texto del �ltimo campo dibujado en la tabla
  */
  QString lastTextPainted_;

  /**
  Pixmap precargados
  */
  QPixmap pixOk_;
  QPixmap pixNo_;

  /**
  Lista con las claves primarias de los registros seleccionados por chequeo
  */
  QValueList<QVariant> primarysKeysChecked_;

  /**
  Filtro persistente para el cursor
  */
  QString persistentFilter_;

  /**
  Indicador para evitar refrescos anidados
  */
  bool refreshing_;
  bool refresh_timer_;

  /**
  Indica si el componente es emergente ( su padre es un widget del tipo Popup )
  */
  bool popup_;

  /**
  Indica el ancho de las columnas establecidas expl�citamente con FLDataTable::setColumnWidth
  */
  QMap<QString, int> widthCols_;

  /**
  Indica si se deben mostrar los campos tipo pixmap en todas las filas
  */
  bool showAllPixmaps_;

  /**
  Nombre de la funci�n de script a invocar para obtener el color de las filas y celdas
  */
  QString functionGetColor_;

  /**
  Indica que no se realicen operaciones con la base de datos (abrir formularios). Modo "s�lo tabla".
  */
  bool onlyTable_;

  /** Uso interno */
  bool changingNumRows_;
  void syncNumRows();

  /** Uso interno */
  bool getCellStyle(QBrush &brush, QPen &pen,
                    QSqlField *field, FLFieldMetaData *fieldTMD,
                    int row, bool selected, const QColorGroup &cg);
  QString paintFieldName_;
  FLFieldMetaData *paintFieldMtd_;
  FLFieldMetaData *paintFieldMtd(const QString &f, FLTableMetaData *t);
  QTimer *timerViewRepaint_;
  /** Almacena el �ltimo row que di� false en seek */
  int last_seek_invalid;


public slots:

  /**
  Selecciona la fila completa en la que est� la celda activa.

  @param r Fila de la celda activa
  @param c Columna de la celda activa
  */
  void selectRow(int r = -1, int c = -1);

  /**
  Redefinida por conveniencia
  */
  void setFocus();

  /**
  Redefinida por conveniencia
  */
  void refresh();

  /**
  Hace que la fila seleccionada est� visible
  */
  void ensureRowSelectedVisible();

  /**
  Foco r�pido sin refrescos para optimizar
  */
  void setQuickFocus();

  /**
  Establece el ancho de una columna

  @param  field Nombre del campo de la base de datos correspondiente a la columna
  @param  w     Ancho de la columna
  */
  void setColumnWidth(const QString &field, int w);

protected slots:

  /**
  Activado cuando se hace click en el chequeo de la columna de selecci�n
  */
  void setChecked(bool on);

  /** Uso interno */
  void delayedViewportRepaint();
  void repaintViewportSlot();
  void cursorDestroyed(QObject *);

signals:

  /**
  Indica que se ha elegido un registro
  */
  void recordChoosed();

  /**
  Indica que ha cambiado el estado del campo de selecci�n de un registro. Es decir
  se ha incluido o eliminado su clave primaria de la lista de claves primarias seleccionadas.
  Esta se�al se emite cuando el usuario hace click en el control de chequeo y cuando se cambia
  program�ticamente el chequeo mediante el m�todo FLDataTable::setPrimaryKeyChecked.

  @param  primaryKeyValue El valor de la clave primaria del registro correspondiente
  @param  on  El nuevo estado; TRUE chequeo activado, FALSE chequeo desactivado
  */
  void primaryKeyToggled(const QVariant &primaryKeyValue, bool on);
};

typedef FLDataTable FLDataTableDB;

// Uso interno
class FLCheckBox: public QCheckBox
{
public:

  FLCheckBox(QWidget *parent, int row = 0, const char *name = 0) : QCheckBox(parent, name), row_(row) {}

  int row() const {
    return row_;
  }

protected:

  void drawButton(QPainter *p);
  bool hitButton(const QPoint &pos) const;

private:

  int row_;
};

#endif
