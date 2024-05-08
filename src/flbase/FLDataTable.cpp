/***************************************************************************
                           FLDataTable.cpp
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

#include "FLDataTable.h"
#include "FLSqlCursor.h"
#include "FLTableMetaData.h"
#include "FLFieldMetaData.h"
#include "FLRelationMetaData.h"
#include "FLUtil.h"
#include "FLApplication.h"
#include "FLSqlQuery.h"
#include "FLSqlDatabase.h"
#include "FLManager.h"
#include "FLObjectFactory.h"
#include "FLSettings.h"

void FLCheckBox::drawButton(QPainter *p)
{
  QRect rect, wrect(this->rect());
  rect.setRect((wrect.width() - 13) / 2, (wrect.height() - 13) / 2, 13, 13);

  if (state() == QButton::On)
  {
    QBrush bu(green, SolidPattern);
    p->fillRect(0, 0, wrect.width() - 1, wrect.height() - 1, bu);
  }

  QRect irect = QStyle::visualRect(rect, this);
  p->fillRect(irect, Qt::white);
  p->drawRect(irect);

  if (state() == QButton::On)
  {
    QPointArray a(7 * 2);
    int i, xx, yy;
    xx = irect.x() + 3;
    yy = irect.y() + 5;

    for (i = 0; i < 3; i++)
    {
      a.setPoint(2 * i, xx, yy);
      a.setPoint(2 * i + 1, xx, yy + 2);
      xx++;
      yy++;
    }

    yy -= 2;
    for (i = 3; i < 7; i++)
    {
      a.setPoint(2 * i, xx, yy);
      a.setPoint(2 * i + 1, xx, yy + 2);
      xx++;
      yy--;
    }

    p->drawLineSegments(a);
  }
}

bool FLCheckBox::hitButton(const QPoint &pos) const
{
  return this->rect().contains(pos);
}

FLDataTable::FLDataTable(QWidget *parent, const char *name, bool popup)
    : QDataTable(parent, name),
      rowSelected(-1), colSelected(-1), cursor_(0), readonly_(false),
      editonly_(false), insertonly_(false), persistentFilter_(QString::null),
      refreshing_(false), refresh_timer_(false), popup_(popup), showAllPixmaps_(false),
      changingNumRows_(false), onlyTable_(false), paintFieldMtd_(0),
      timerViewRepaint_(0)
{
  if (!name)
    setName("FLDataTable");
  pixOk_ = QPixmap::fromMimeSource("unlock.png");
  pixNo_ = QPixmap::fromMimeSource("lock.png");
}

FLDataTable::~FLDataTable()
{
  if (timerViewRepaint_)
    timerViewRepaint_->stop();
  if (cursor_ && !cursor_->aqWasDeleted())
  {
    cursor_->restoreEditionFlag(this);
    cursor_->restoreBrowseFlag(this);
  }
}

void FLDataTable::selectRow(int r, int c)
{
  if (!cursor_ || cursor_->aqWasDeleted() || !cursor_->metadata())
    return;

  if (r < 0)
  {
    if (cursor_->isValid())
    {
      rowSelected = cursor_->at();
      colSelected = currentColumn();
    }
    else
    {
      rowSelected = 0;
      colSelected = 0;
    }
  }
  else
  {
    rowSelected = r;
    colSelected = c;
  }

  QObject *snd = const_cast<QObject *>(sender());
  if (!snd || (snd && !snd->isA("FLSqlCursor")))
  {
    QWidget *sndw = ::qt_cast<QWidget *>(snd);
    if (sndw)
    {
      if (refreshing_ || !sndw->hasFocus() || !sndw->isVisible())
      {
        setCurrentCell(rowSelected, colSelected);
        return;
      }
    }
    syncNumRows();
    cursor_->seek(rowSelected);
  }
  setCurrentCell(rowSelected, colSelected);
}

void FLDataTable::setFLSqlCursor(FLSqlCursor *c)
{
  if (c && c->metadata())
  {
    if (!cursor_)
    {
      disconnect(this, SIGNAL(currentChanged(int, int)), this, SLOT(selectRow(int, int)));
      disconnect(this, SIGNAL(clicked(int, int, int, const QPoint &)), this, SLOT(selectRow(int, int)));
      connect(this, SIGNAL(currentChanged(int, int)), this, SLOT(selectRow(int, int)));
      connect(this, SIGNAL(clicked(int, int, int, const QPoint &)), this, SLOT(selectRow(int, int)));
    }
    else
    {
      disconnect(cursor_, SIGNAL(currentChanged(int)), this, SLOT(selectRow(int)));
      if (!popup_)
        disconnect(cursor_, SIGNAL(cursorUpdated()), this, SLOT(refresh()));
      disconnect(cursor_, SIGNAL(destroyed(QObject *)), this, SLOT(cursorDestroyed(QObject *)));
    }

    bool curChg = false;
    if (cursor_ && cursor_ != c)
    {
      cursor_->restoreEditionFlag(this);
      cursor_->restoreBrowseFlag(this);
      curChg = true;
    }
    cursor_ = c;
    if (cursor_)
    {

      bool result_ = false;
      FLTableMetaData *mtd = cursor_->metadata();
      QString label_ = "FLDataTable::setFLSqlCursor (" + mtd->name() + "): ";

      QString id_mod_ = cursor_->db()->managerModules()->idModuleOfFile(mtd->name() + QString::fromLatin1(".mtd"));
      QString fun_module_ = "sys";

      if (!id_mod_.isEmpty())
      {
        fun_module_ = id_mod_;
      }
      QString fun_name_ = fun_module_ + ".useDelegateCommit";

      FLSqlCursorInterface *cI = FLSqlCursorInterface::sqlCursorInterface(cursor_);
      QVariant v = aqApp->call(fun_name_, QSArgumentList(cI), 0).variant();
      if (v.isValid())
      {
        result_ = v.toBool();
        qWarning(label_ + fun_name_ + " retorna " + (result_ ? "true" : "false"));
      }
      // else
      //{
      //   qWarning(label_ + "No hay respuesta de " + fun_name_ + "(cursor). Asumiento false");
      // }

      cursor_->isDelegateCommit = result_;

      if (curChg)
      {
        setFLReadOnly(readonly_);
        setEditOnly(editonly_);
        setInsertOnly(insertonly_);
        setOnlyTable(onlyTable_);
      }
      disconnect(cursor_, SIGNAL(currentChanged(int)), this, SLOT(selectRow(int)));
      if (!popup_)
        disconnect(cursor_, SIGNAL(cursorUpdated()), this, SLOT(refresh()));
      connect(cursor_, SIGNAL(currentChanged(int)), this, SLOT(selectRow(int)));
      if (!popup_)
        connect(cursor_, SIGNAL(cursorUpdated()), this, SLOT(refresh()));
      connect(cursor_, SIGNAL(destroyed(QObject *)), this, SLOT(cursorDestroyed(QObject *)));
    }

    QDataTable::setSqlCursor(static_cast<QSqlCursor *>(c), true, false);
  }
}

static inline Qt::BrushStyle nametoBrushStyle(const QString &style)
{
  if (style.isEmpty() || style == "SolidPattern")
    return Qt::SolidPattern;
  if (style == "NoBrush")
    return Qt::NoBrush;
  if (style == "Dense1Pattern")
    return Qt::Dense1Pattern;
  if (style == "Dense2Pattern")
    return Qt::Dense2Pattern;
  if (style == "Dense3Pattern")
    return Qt::Dense3Pattern;
  if (style == "Dense4Pattern")
    return Qt::Dense4Pattern;
  if (style == "Dense5Pattern")
    return Qt::Dense5Pattern;
  if (style == "Dense6Pattern")
    return Qt::Dense6Pattern;
  if (style == "Dense7Pattern")
    return Qt::Dense7Pattern;
  if (style == "HorPattern")
    return Qt::HorPattern;
  if (style == "VerPattern")
    return Qt::VerPattern;
  if (style == "CrossPattern")
    return Qt::CrossPattern;
  if (style == "BDiagPattern")
    return Qt::BDiagPattern;
  if (style == "FDiagPattern")
    return Qt::FDiagPattern;
  if (style == "DiagCrossPattern")
    return Qt::DiagCrossPattern;
  return Qt::SolidPattern;
}

static inline Qt::PenStyle nametoPenStyle(const QString &style)
{
  if (style.isEmpty() || style == "SolidLine")
    return Qt::SolidLine;
  if (style == "NoPen")
    return Qt::NoPen;
  if (style == "DashLine")
    return Qt::DashLine;
  if (style == "DotLine")
    return Qt::DotLine;
  if (style == "DashDotLine")
    return Qt::DashDotLine;
  if (style == "DashDotDotLine")
    return Qt::DashDotDotLine;
  return Qt::SolidLine;
}

static inline bool setNewBrushStyle(QBrush &brush, Qt::BrushStyle st)
{
  if (st != brush.style())
  {
    brush.setStyle(st);
    return true;
  }
  return false;
}

static inline bool setNewPenStyle(QPen &pen, Qt::PenStyle st)
{
  if (st != pen.style())
  {
    pen.setStyle(st);
    return true;
  }
  return false;
}

static inline bool setNewBrushColor(QBrush &brush, const QColor &c)
{
  if (c != brush.color())
  {
    brush.setColor(c);
    return true;
  }
  return false;
}

static inline bool setNewPenColor(QPen &pen, const QColor &c)
{
  if (c != pen.color())
  {
    pen.setColor(c);
    return true;
  }
  return false;
}

bool FLDataTable::getCellStyle(QBrush &brush, QPen &pen,
                               QSqlField *field, FLFieldMetaData *fieldTMD,
                               int row, bool selected, const QColorGroup &cg)
{
  if (brush.style() == Qt::NoBrush)
    brush.setStyle(Qt::SolidPattern);
  // if (pen.style() == Qt::NoPen)
  //   pen.setStyle(Qt::SolidLine);

  if (!fieldTMD->visible())
  {
    brush.setColor(Qt::gray);
    brush.setStyle(Qt::DiagCrossPattern);
    return true;
  }

  bool initNewBrush = false;

  if (!selected)
  {
    if ((row % 2))
      initNewBrush = setNewBrushColor(brush, cg.brush(QColorGroup::Midlight).color());
    else
      setNewBrushColor(brush, cg.brush(QColorGroup::Base).color());
    setNewPenColor(pen, cg.text());
  }
  else
  {
    if (focusStyle() == QTable::SpreadSheet)
      initNewBrush = setNewBrushColor(brush, cg.brush(QColorGroup::Highlight).color());
    else
      setNewBrushColor(brush, cg.brush(QColorGroup::Highlight).color());
    setNewPenColor(pen, cg.highlightedText());
  }

  bool newBrush = false;
  bool newBrushStyle = false;
  bool newPen = false;
  bool newPenStyle = false;
  int type = fieldTMD->type();

  if (!functionGetColor_.isEmpty() && cursor_->modeAccess() == FLSqlCursor::BROWSE)
  {
    QString bgColorName;
    QString fgColorName;
    QString brushStyle;
    QString penStyle;
    QSArgumentList arglist;
    QSArgument ret;

    arglist.append(QSArgument(fieldTMD->name()));
    arglist.append(QSArgument(field->value()));
    arglist.append(QSArgument(FLSqlCursorInterface::sqlCursorInterface(cursor_)));
    arglist.append(QSArgument(selected));
    arglist.append(QSArgument(type));
    bool bakrv = cursor_->d->rawValues_;
    cursor_->d->rawValues_ = true;
    ret = aqApp->call(functionGetColor_, arglist, 0);
    cursor_->d->rawValues_ = bakrv;

    if (ret.type() == QSArgument::Variant && ret.variant().type() == QVariant::List)
    {
      QValueList<QVariant> list(ret.variant().toList());

      if (!list.isEmpty())
      {
        if (list.size() >= 1)
        {
          bgColorName = list[0].toString();
          if (list.size() >= 2)
          {
            fgColorName = list[1].toString();
            if (list.size() >= 3)
            {
              brushStyle = list[2].toString();
              if (list.size() >= 4)
                penStyle = list[3].toString();
            }
          }
        }

        if (!bgColorName.isEmpty())
        {
          QColor bgColor(bgColorName);
          if (bgColor.isValid())
            newBrush = setNewBrushColor(brush, bgColor);
        }
        if (!brushStyle.isEmpty())
          newBrushStyle = setNewBrushStyle(brush, nametoBrushStyle(brushStyle));

        if (!fgColorName.isEmpty())
        {
          QColor fgColor(fgColorName);
          if (fgColor.isValid())
            newPen = setNewPenColor(pen, fgColor);
        }
        if (!penStyle.isEmpty())
          newPenStyle = setNewPenStyle(pen, nametoPenStyle(penStyle));
      }
    }
  }

  if (field->isNull() && type != QVariant::Bool)
    return initNewBrush || newBrush || newBrushStyle;

  switch (type)
  {
  case QVariant::Double:
  {
    if (!newPen)
    {
      double fValue = field->value().toDouble();
      if (fValue < 0.0)
        setNewPenColor(pen, Qt::red);
    }
  }
  break;

  case QVariant::Int:
  {
    if (!newPen)
    {
      double fValue = field->value().toInt();
      if (fValue < 0)
        setNewPenColor(pen, Qt::red);
    }
  }
  break;

  case QVariant::Bool:
  {
    if (!newBrush)
    {
      newBrush = setNewBrushColor(brush,
                                  field->value().toBool() ? Qt::green : Qt::red);
    }
    if (!newBrushStyle)
    {
      newBrushStyle = setNewBrushStyle(brush,
                                       selected ? Qt::Dense4Pattern : Qt::SolidPattern);
    }
    if (!newPen && !selected)
      setNewPenColor(pen, Qt::darkBlue);
  }
  break;
  }

  return initNewBrush || newBrush || newBrushStyle;
}

void FLDataTable::paintCell(QPainter *p, int row, int col, const QRect &cr,
                            bool selected, const QColorGroup &cg)
{
  FLTableMetaData *tMD;
  if (!cursor_ || cursor_->aqWasDeleted() || !(tMD = cursor_->metadata()))
    return;
  QSqlField *field = cursor_->field(indexOf(col));
  QString fName(field->name());
  FLFieldMetaData *fieldTMD = paintFieldMtd(fName, tMD);
  if (!fieldTMD)
    return;

  int type = fieldTMD->type();

  if (!showAllPixmaps_ && type == QVariant::Pixmap && row != rowSelected)
  {
    QTable::paintCell(p, row, col, cr, selected, cg);
    return;
  }
  qWarning("FLDataTable::paintCell() : row: %d, col: %d,  rowSelected: %d", row , col, rowSelected);
  if (row != cursor_->QSqlCursor::at() || !cursor_->isValid())
  {
    if (!cursor_->QSqlCursor::seek(row))
    {
#ifdef FL_DEBUG
      qWarning(tr("FLDataTable::paintCell() : Posici�n no v�lida %1 %2").arg(row).arg(tMD->name()));
#endif
      return;
    }
  }

  if (fieldTMD->isCheck())
  {
    if (showGrid())
    {
      int x2 = cr.width() - 1;
      int y2 = cr.height() - 1;
      QPen pen(p->pen());
      int gridColor = style().styleHint(QStyle::SH_Table_GridLineColor, this);
      if (gridColor != -1)
      {
        const QPalette &pal = palette();
        if (cg != colorGroup() && cg != pal.disabled() && cg != pal.inactive())
          p->setPen(cg.mid());
        else
          p->setPen((QRgb)gridColor);
      }
      else
      {
        p->setPen(cg.mid());
      }
      p->drawLine(x2, 0, x2, y2);
      p->drawLine(0, y2, x2, y2);
      p->setPen(pen);
    }
    paintField(p, field, cr, selected);
    return;
  }

  QBrush bB(p->brush());
  QPen bP(p->pen());
  QBrush stBru(bB);
  QPen stPen(bP);

  bool newBrush = getCellStyle(stBru, stPen, field, fieldTMD, row, selected, cg);

  p->setBrush(stBru);
  p->setPen(stPen);

  if (newBrush)
  {
    if (stBru.style() != Qt::SolidPattern)
    {
      p->fillRect(0, 0, cr.width() - 1, cr.height() - 1,
                  selected ? cg.brush(QColorGroup::Highlight) : cg.brush(QColorGroup::Base));
    }
    QColorGroup cgAux(cg);
    cgAux.setBrush(QColorGroup::Base, stBru);
    cgAux.setBrush(QColorGroup::Highlight, stBru);
    QTable::paintCell(p, row, col, cr, selected, cgAux);
  }
  else
    QTable::paintCell(p, row, col, cr, selected, cg);

  lastTextPainted_ = QString::null;
  paintField(p, field, cr, selected);
  p->setBrush(bB);
  p->setPen(bP);

  if (!widthCols_.isEmpty())
  {
    QMap<QString, int>::const_iterator it(widthCols_.find(fName));
    if (it != widthCols_.end())
    {
      int wH = (*it);
      if (wH == -1)
        return;
      if (wH > 0 && wH != columnWidth(col))
      {
        QTable::setColumnWidth(col, wH);
        if (col == 0 && popup_)
        {
          QWidget *pw = parentWidget();
          if (pw && pw->width() < wH)
          {
            resize(wH, pw->height());
            pw->resize(wH, pw->height());
          }
        }
        delayedViewportRepaint();
        widthCols_.replace(fName, -1);
      }
      if (wH > 0)
        return;
    }
  }

  if (!fieldTMD->visible())
    return;

  int wC = columnWidth(col);
  int wH = fontMetrics().width(fieldTMD->alias() + QString::fromLatin1("W"));
  if (wH < wC)
    wH = wC;
  wC = fontMetrics().width(lastTextPainted_) + fontMetrics().maxWidth();
  if (wC > wH)
  {
    QTable::setColumnWidth(col, wC);
    if (col == 0 && popup_)
    {
      QWidget *pw = parentWidget();
      if (pw && pw->width() < wC)
      {
        resize(wC, pw->height());
        pw->resize(wC, pw->height());
      }
    }
    delayedViewportRepaint();
  }
}

void FLDataTable::paintField(QPainter *p, const QSqlField *field,
                             const QRect &cr, bool selected)
{
  if (!field)
    return;

  FLTableMetaData *tMD = cursor_->metadata();
  FLFieldMetaData *fieldTMD = paintFieldMtd(field->name(), tMD);
  if (!fieldTMD || !fieldTMD->visible())
    return;

  int type = fieldTMD->type();

  if (field->isNull() && type != QVariant::Bool)
    return;

  QString text;

  switch (type)
  {
  case QVariant::Double:
  {
    double fValue = field->value().toDouble();
    text = aqApp->localeSystem().toString(fValue, 'f', fieldTMD->partDecimal());
    p->drawText(2, 2, cr.width() - 4, cr.height() - 4,
                Qt::AlignRight | Qt::AlignVCenter, text);
  }
  break;

  case FLFieldMetaData::Unlock:
  {
    if (field->value().toBool())
    {
      p->drawPixmap((cr.width() - pixOk_.width()) / 2, 2, pixOk_,
                    0, 0, cr.width() - 4, cr.height() - 4);
    }
    else
    {
      p->drawPixmap((cr.width() - pixNo_.width()) / 2, 2, pixNo_,
                    0, 0, cr.width() - 4, cr.height() - 4);
    }
  }
  break;

  case QVariant::DateTime:
  case QVariant::String:
  {
    text = field->value().toString();
    if (fieldTMD->hasOptionsList())
    {
      QStringList ol(fieldTMD->optionsList());
      if (!ol.contains(text))
      {
        QVariant defVal(fieldTMD->defaultValue());
        if (defVal.isValid())
          text = defVal.toString();
        else
          text = ol.first();
      }
      text = FLUtil::translate("MetaData", text);
    }
    p->drawText(2, 2, cr.width() - 4, cr.height() - 4, fieldAlignment(field), text);
  }
  break;

  case QVariant::Int:
  {
    int fValue = field->value().toInt();
    text = aqApp->localeSystem().toString(fValue);
    p->drawText(2, 2, cr.width() - 4, cr.height() - 4,
                Qt::AlignRight | Qt::AlignVCenter, text);
  }
  break;

  case FLFieldMetaData::Serial:
  case QVariant::UInt:
    text = aqApp->localeSystem().toString(field->value().toUInt());
    p->drawText(2, 2, cr.width() - 4, cr.height() - 4,
                Qt::AlignRight | Qt::AlignVCenter, text);
    break;

  case QVariant::Pixmap:
  {
    QCString cs = cursor_->db()->manager()->fetchLargeValue(field->value().toString()).toCString();
    if (cs.isEmpty())
      return;

    QPixmap pix;

    if (!QPixmapCache::find(cs.left(100), pix))
    {
      pix.loadFromData(cs);
      QPixmapCache::insert(cs.left(100), pix);
    }
    if (!pix.isNull())
      p->drawPixmap(2, 2, pix, 0, 0, cr.width() - 4,
                    cr.height() - 4);
  }
  break;

  case QVariant::ByteArray:
    p->drawText(2, 2, cr.width() - 4, cr.height() - 4,
                Qt::AlignAuto | Qt::AlignTop, QString::fromLatin1("ByteArray"));
    break;

  case QVariant::Date:
  {
    QDate d = field->value().toDate();

    text = d.toString("dd-MM-yyyy");
    p->drawText(2, 2, cr.width() - 4, cr.height() - 4,
                fieldAlignment(field), text);
  }
  break;

  case QVariant::Time:
  {
    QTime t = field->value().toTime();

    text = t.toString("hh:mm:ss");
    p->drawText(2, 2, cr.width() - 4, cr.height() - 4,
                fieldAlignment(field), text);
  }
  break;

  case QVariant::StringList:
    text = field->value().toString();
    p->drawText(2, 2, cr.width() - 4, cr.height() - 4,
                Qt::AlignAuto | Qt::AlignTop, text.left(255) + "...");
    break;

  case QVariant::Bool:
  {
    if (fieldTMD->isCheck())
    {
      int row = rowAt(cr.center().y()), col = columnAt(cr.center().x());
      int curAt = cursor_->at();
      FLCheckBox *chk = ::qt_cast<FLCheckBox *>(cellWidget(row, col));
      if (!chk)
      {
        chk = new FLCheckBox(this, row);
        setCellWidget(row, col, chk);
      }
      else
        disconnect(chk, SIGNAL(toggled(bool)), this, SLOT(setChecked(bool)));
      if (cursor_->QSqlCursor::seek(row))
      {
        chk->setChecked(primarysKeysChecked_.contains(cursor_->QSqlCursor::value(tMD->primaryKey())));
        cursor_->QSqlCursor::seek(curAt);
      }
      connect(chk, SIGNAL(toggled(bool)), this, SLOT(setChecked(bool)));
    }
    else
    {
      text = field->value().toBool() ? tr("S�") : tr("No");
      p->drawText(2, 2, cr.width() - 4, cr.height() - 4,
                  fieldAlignment(field), text);
    }
  }
  break;
  }
  lastTextPainted_ = text;
}

bool FLDataTable::eventFilter(QObject *o, QEvent *e)
{
  int r = currentRow(), c = currentColumn(), nr = numRows(), nc = numCols();

  switch (e->type())
  {
  case QEvent::KeyPress:
  {
    QKeyEvent *ke = static_cast<QKeyEvent *>(e);

    if (ke->key() == Qt::Key_Escape && popup_ && parentWidget())
    {
      parentWidget()->hide();
      return true;
    }

    if (ke->key() == Qt::Key_Insert)
      return true;

    if (ke->key() == Qt::Key_F2)
      return true;

    if (ke->key() == Qt::Key_Up && r == 0)
      return true;

    if (ke->key() == Qt::Key_Left && c == 0)
      return true;

    if (ke->key() == Qt::Key_Down && r == nr - 1)
      return true;

    if (ke->key() == Qt::Key_Right && c == nc - 1)
      return true;

    if ((ke->key() == Qt::Key_Enter || ke->key() == Qt::Key_Return) && r > -1)
    {
      emit recordChoosed();
      return true;
    }
    //-->Aulla : Desactiva atajos de teclado de FLTable
    if (!FLSettings::readBoolEntry("ebcomportamiento/FLTableShortCut", false))
    {

      if (ke->key() == Qt::Key_Space)
      {
        FLCheckBox *chk = ::qt_cast<FLCheckBox *>(cellWidget(r, c));
        if (chk)
          chk->animateClick();
      }

      if (ke->key() == Qt::Key_A && !popup_)
      {
        if (cursor_ && !cursor_->aqWasDeleted() && !readonly_ && !editonly_ && !onlyTable_)
        {
          cursor_->insertRecord();
          return true;
        }
        else
          return false;
      }

      if (ke->key() == Qt::Key_C && !popup_)
      {
        if (cursor_ && !cursor_->aqWasDeleted() && !readonly_ && !editonly_ && !onlyTable_)
        {
          cursor_->copyRecord();
          return true;
        }
        else
          return false;
      }

      if (ke->key() == Qt::Key_M && !popup_)
      {
        if (insertonly_)
          return false;
        else if (cursor_ && !cursor_->aqWasDeleted() && !readonly_ && !onlyTable_)
        {
          cursor_->editRecord();
          return true;
        }
        else
          return false;
      }

      if (ke->key() == Key_Delete && !popup_)
      {
        if (insertonly_)
          return false;
        else if (cursor_ && !cursor_->aqWasDeleted() && !readonly_ && !editonly_ && !onlyTable_)
        {
          cursor_->deleteRecord();
          return true;
        }
        else
          return false;
      }

      if (ke->key() == Qt::Key_V && !popup_)
      {
        if (cursor_ && !cursor_->aqWasDeleted() && !onlyTable_)
        {
          cursor_->browseRecord();
          return true;
        }
      }

    } //<--Aulla : Desactiva atajos de teclado de FLTable

    return false;
  }
  break;

  case QEvent::Paint:
  {
    if (o == this && cursor_ && !cursor_->aqWasDeleted() && !persistentFilter_.isEmpty() &&
        !cursor_->curFilter().contains(persistentFilter_))
    {
      cursor_->setFilter(persistentFilter_);
      refresh();
      return true;
    }
  }
  break;
  }
  return QDataTable::eventFilter(o, e);
}

void FLDataTable::contentsContextMenuEvent(QContextMenuEvent *e)
{
  QTable::contentsContextMenuEvent(e);

  if (!cursor_ || cursor_->aqWasDeleted() || !cursor_->isValid() || !cursor_->metadata())
    return;

  FLTableMetaData *mtd = cursor_->metadata();
  QString priKey(mtd->primaryKey());

  FLFieldMetaData *field = mtd->field(priKey);
  if (!field)
    return;

  const FLFieldMetaData::FLRelationMetaDataList *relList = field->relationList();
  if (!relList)
    return;

  FLSqlDatabase *db = cursor_->db();
  QVariant priKeyVal(cursor_->valueBuffer(priKey));
  QGuardedPtr<QPopupMenu> popup = new QPopupMenu(this);

  FLRelationMetaData *rel;
  QPtrListIterator<FLRelationMetaData> it(*relList);
  while ((rel = it.current()) != 0)
  {
    ++it;
    QGuardedPtr<QPopupMenu> subPopup = 0;
    FLSqlCursor *cur = new FLSqlCursor(rel->foreignTable(), true,
                                       db->connectionName(), 0, 0, popup);
    if (cur->metadata())
    {
      mtd = cur->metadata();
      field = mtd->field(rel->foreignField());
      if (!field)
        continue;

      subPopup = new QPopupMenu(popup);

      QVBox *frame = new QVBox(popup, 0, WType_Popup);
      frame->setFrameStyle(QFrame::PopupPanel | QFrame::Raised);
      frame->setLineWidth(1);

      FLDataTable *dt = new FLDataTable(frame, 0, true);
      dt->setFLSqlCursor(cur);

      QString filter(db->manager()->formatAssignValue(field, priKeyVal, false));
      cur->setFilter(filter);
      dt->setFilter(filter);
      dt->QDataTable::refresh();

      QHeader *horizHeader = dt->horizontalHeader();
      for (int i = 0; i < dt->numCols(); ++i)
      {
        field = mtd->field(mtd->fieldAliasToName(horizHeader->label(i)));
        if (!field)
          continue;
        if (!field->visibleGrid())
          dt->hideColumn(i);
      }
      for (int i = 0; i < dt->numCols(); ++i)
      {
        field = mtd->field(mtd->fieldAliasToName(horizHeader->label(i)));
        if (!field)
          continue;
        horizHeader->setLabel(i, field->alias());
      }

      subPopup->insertItem(frame);
      popup->insertItem(mtd->alias(), subPopup);
    }
  }

  popup->exec(e->globalPos());
  delete popup;
  e->accept();
}

void FLDataTable::contentsMouseDoubleClickEvent(QMouseEvent *e)
{
  if (e->button() != LeftButton)
    return;

  int tmpRow = rowAt(e->pos().y());
  int tmpCol = columnAt(e->pos().x());
  QTableItem *itm = item(tmpRow, tmpCol);

  if (itm && !itm->isEnabled())
    return;

  emit doubleClicked(tmpRow, tmpCol, e->button(), e->pos());
  emit recordChoosed();
}

void FLDataTable::refresh()
{
  if (popup_)
    QDataTable::refresh();
  if (!refreshing_ && cursor_ && !cursor_->aqWasDeleted() && cursor_->metadata())
  {
    refreshing_ = true;
    cursor_->setFilter(persistentFilter_);
    FLSqlCursor *sndCursor = ::qt_cast<FLSqlCursor *>(sender());
    if (sndCursor)
    {
      setFilter(cursor_->curFilter());
      QDataTable::refresh();
      cursor_->QSqlCursor::seek(cursor_->atFrom());
      selectRow();
    }
    else
    {
      setFilter(cursor_->curFilter());
      QDataTable::refresh();
      selectRow();
    }
  }
  refreshing_ = false;
}

void FLDataTable::setFocus()
{
  if (!cursor_ || cursor_->aqWasDeleted())
    return;
  if (!hasFocus())
  {
    setPaletteBackgroundColor(qApp->palette().color(QPalette::Active, QColorGroup::Base));
    QDataTable::refresh();
  }
  else
    syncNumRows();
  QWidget::setFocus();
}

void FLDataTable::setQuickFocus()
{
  setPaletteBackgroundColor(qApp->palette().color(QPalette::Active, QColorGroup::Base));
  QWidget::setFocus();
}

void FLDataTable::focusOutEvent(QFocusEvent *)
{
  setPaletteBackgroundColor(qApp->palette().color(QPalette::Active, QColorGroup::Background));
}

void FLDataTable::setFLReadOnly(const bool mode)
{
  if (!cursor_ || cursor_->aqWasDeleted())
    return;
  cursor_->setEdition(!mode, this);
  readonly_ = mode;
}

void FLDataTable::setEditOnly(const bool mode)
{
  if (!cursor_ || cursor_->aqWasDeleted())
    return;
  editonly_ = mode;
}

void FLDataTable::setInsertOnly(const bool mode)
{
  if (!cursor_ || cursor_->aqWasDeleted())
    return;
  cursor_->setEdition(!mode, this);
  insertonly_ = mode;
}

void FLDataTable::setOnlyTable(bool on)
{
  if (!cursor_ || cursor_->aqWasDeleted())
    return;
  cursor_->setEdition(!on, this);
  cursor_->setBrowse(!on, this);
  onlyTable_ = on;
}

void FLDataTable::ensureRowSelectedVisible()
{
  if (rowSelected > -1)
  {
    if (!isUpdatesEnabled() || !viewport()->isUpdatesEnabled())
      return;
    int cw = columnWidth(colSelected);
    int margin = visibleHeight() / 2;
    int y = rowPos(rowSelected) + rowHeight(rowSelected) / 2;
    if (cw < visibleWidth())
      ensureVisible(columnPos(colSelected) + cw / 2, y, cw / 2, margin);
    else
      ensureVisible(columnPos(colSelected), y, 0, margin);
  }
}

void FLDataTable::setChecked(bool on)
{
  FLCheckBox *chk = ::qt_cast<FLCheckBox *>(sender());

  if (!chk || !cursor_ || cursor_->aqWasDeleted() || !cursor_->metadata())
    return;

  int curAt = cursor_->at();
  int posAt = chk->row();

  if (cursor_->QSqlCursor::seek(posAt))
  {
    QVariant primaryKeyValue(cursor_->QSqlCursor::value(cursor_->metadata()->primaryKey()));
    setPrimaryKeyChecked(primaryKeyValue, on);
    cursor_->QSqlCursor::seek(curAt);
  }
}

const QValueList<QVariant> FLDataTable::primarysKeysChecked() const
{
  return primarysKeysChecked_;
}

void FLDataTable::clearChecked()
{
  primarysKeysChecked_.clear();
}

void FLDataTable::setPrimaryKeyChecked(const QVariant &primaryKeyValue, bool on)
{
  if (on)
  {
    if (!primarysKeysChecked_.contains(primaryKeyValue))
    {
      primarysKeysChecked_.append(primaryKeyValue);
      emit primaryKeyToggled(primaryKeyValue, true);
    }
  }
  else
  {
    if (primarysKeysChecked_.contains(primaryKeyValue))
    {
      primarysKeysChecked_.remove(primaryKeyValue);
      emit primaryKeyToggled(primaryKeyValue, false);
    }
  }
}

void FLDataTable::setPersistentFilter(const QString &pFilter)
{
  persistentFilter_ = pFilter;
}

void FLDataTable::setColumnWidth(const QString &field, int w)
{
  widthCols_.insert(field, w);
}

void FLDataTable::handleError(const QSqlError &)
{
}

int FLDataTable::indexOf(uint i) const
{
  return QDataTable::indexOf(i);
}

QString FLDataTable::fieldName(int col) const
{
  if (!cursor_ || cursor_->aqWasDeleted())
    return QString::null;
  QSqlField *field = cursor_->field(indexOf(col));
  if (!field)
    return QString::null;
  return field->name();
}

void FLDataTable::syncNumRows()
{
  if (changingNumRows_)
    return;
  if (numRows() != cursor_->size())
  {
    changingNumRows_ = true;
    setNumRows(cursor_->size());
    changingNumRows_ = false;
  }
}

void FLDataTable::drawContents(QPainter *p, int cx, int cy, int cw, int ch)
{
  if (!cursor_ || cursor_->aqWasDeleted())
  {
    QDataTable::drawContents(p, cx, cy, cw, ch);
    return;
  }

  int curRow = rowSelected;
  QDataTable::drawContents(p, cx, cy, cw, ch);

  if (!persistentFilter_.isEmpty() && !cursor_->curFilter().contains(persistentFilter_))
  {
    cursor_->setFilter(persistentFilter_);
    refresh();
    return;
  }

  rowSelected = curRow;
  cursor_->QSqlCursor::seek(rowSelected);
}

FLFieldMetaData *FLDataTable::paintFieldMtd(const QString &f, FLTableMetaData *t)
{
  if (paintFieldMtd_ && paintFieldName_ == f)
    return paintFieldMtd_;
  paintFieldName_ = f;
  paintFieldMtd_ = t->field(f);
  return paintFieldMtd_;
}

void FLDataTable::delayedViewportRepaint()
{
  if (!timerViewRepaint_)
  {
    timerViewRepaint_ = new QTimer(this);
    connect(timerViewRepaint_, SIGNAL(timeout()), this, SLOT(repaintViewportSlot()));
  }
  if (!timerViewRepaint_->isActive())
  {
    setUpdatesEnabled(false);
    timerViewRepaint_->start(50, true);
  }
}

void FLDataTable::repaintViewportSlot()
{
  QWidget *vw = viewport();
  setUpdatesEnabled(true);
  if (vw && !vw->aqWasDeleted())
    vw->repaint(false);
}

void FLDataTable::cursorDestroyed(QObject *obj)
{
  if (!obj || obj != cursor_)
    return;
  cursor_ = 0;
}
