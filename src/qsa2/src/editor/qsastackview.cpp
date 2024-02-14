/****************************************************************************
** $Id: qsastackview.cpp  beta3   edited Mar 4 15:37 $
**
** Copyright (C) 2001-2002 Trolltech AS.  All rights reserved.
**
** This file is part of the Qt Script for Applications framework (QSA).
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.
**
** Licensees holding a valid QSA Beta Evaluation Version license may use
** this file in accordance with the QSA Beta Evaluation Version License
** Agreement provided with the Software.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** See http://www.trolltech.com/pricing.html or email sales@trolltech.com for
**   information about QSA Commercial License Agreements.
** See http://www.trolltech.com/gpl/ for GPL licensing information.
**
** Contact info@trolltech.com if any conditions of this licensing are
** not clear to you.
**
*****************************************************************************/

#include "qsastackview.h"
#include "quickinterpreter.h"
#include "quickdebugger.h"
#include "../ide/idewindow.h"
#include "../shared/globaldefs.h"
#include "../qsa/qsproject.h"
#include <qheader.h>
#include <qpainter.h>

extern QuickInterpreter *get_quick_interpreter(QSInterpreter *);
extern IdeWindow *get_workbench(QWidget *);

QSAStackItem::QSAStackItem(QListView *parent, QListViewItem *after)
  : QListViewItem(parent, after), sid(-1), lvl(-1)
{}

void QSAStackItem::paintCell(QPainter *p, const QColorGroup &cg, int column, int width, int align)
{
  QColorGroup g(cg);
  g.setColor(QColorGroup::Base, backgroundColor());
  g.setColor(QColorGroup::Foreground, Qt::black);
  g.setColor(QColorGroup::Text, Qt::black);
  p->save();

  QListViewItem::paintCell(p, g, column, width, align);
  p->setPen(QPen(cg.dark(), 1));
  if (column == 0)
    p->drawLine(0, 0, 0, height() - 1);
  p->drawLine(0, height() - 1, width, height() - 1);
  p->drawLine(width - 1, 0, width - 1, height());
  p->restore();
}

QColor QSAStackItem::backgroundColor()
{
  updateBackColor();
  return backColor;
}

void QSAStackItem::updateBackColor()
{
  if (listView()->firstChild() == this) {
    backColor = *backColor1;
    return ;
  }

  QListViewItemIterator it(this);
  --it;
  if (it.current()) {
    if (((QSAStackItem *) it.current())->backColor == *backColor1)
      backColor = *backColor2;
    else
      backColor = *backColor1;
  } else {
    backColor == *backColor1;
  }
}

QSAStackView::QSAStackView(IdeWindow *w, QWidget *parent, const char *)
  : QListView(parent, "qsa_debugger_stackview"), wb(w)
{
  setSorting(-1);
  init_colors();
  addColumn(tr("Line"));
  addColumn(tr("Function"));
  setResizeMode(LastColumn);
  setAllColumnsShowFocus(TRUE);
  connect(this, SIGNAL(clicked(QListViewItem *)),
          this, SLOT(changeScope(QListViewItem *)));
  setHScrollBarMode(AlwaysOff);
  setVScrollBarMode(AlwaysOn);
}

void QSAStackView::updateStack()
{
  clear();

  IdeWindow *wb = get_workbench(this);
  QuickDebugger *dbg = (wb ? wb->project->debugger() : 0);
  if (!dbg || dbg->mode() == Debugger::Disabled)
    return;

  QValueList<QuickDebuggerStackFrame> lst = dbg->backtrace();
  QSAStackItem *last = 0;
  int l = 0;
  for (QValueList<QuickDebuggerStackFrame>::ConstIterator it = lst.begin();
       it != lst.end(); ++it, ++l) {
    last = new QSAStackItem(this, last);
    last->setText(1, (*it).function);
    last->setText(0, QString::number((*it).line < 0 ? 0 : (*it).line));
    last->setSourceId((*it).sourceId);
    last->setLevel(l);
  }
}

void QSAStackView::changeScope(QListViewItem *i)
{
  if (!i)
    return ;

  QuickDebugger *dbg = (wb ? wb->project->debugger() : 0);
  if (!dbg)
    return;

  QSAStackItem *item = static_cast<QSAStackItem *>(i);
  QuickInterpreter *ip = get_quick_interpreter(wb->project->interpreter());
  QObject *sidObj = ip->objectOfSourceId(item->sourceId());

  if (!sidObj)
    return;

  wb->showStackFrame(sidObj, i->text(0).toInt());
  emit scopeChanged(item->level());
}
