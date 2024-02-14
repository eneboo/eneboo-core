/****************************************************************************
** $Id: qsavariableview.cpp  beta3   edited Jan 22 14:20 $
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

#include "qsavariableview.h"
#include "quickinterpreter.h"
#include "quickdebugger.h"
#include "../ide/idewindow.h"
#include "../qsa/qsproject.h"
#include "../shared/globaldefs.h"
#include <qheader.h>
#include <qpainter.h>
#include <qlineedit.h>
#include <qregexp.h>

extern QuickInterpreter *get_quick_interpreter(QSInterpreter *);
extern IdeWindow *get_workbench(QWidget *);

#define ALLOW_VARIABLE_MODIFY

QSAVariableItem::QSAVariableItem(QListView *parent, QListViewItem *after)
  : QListViewItem(parent, after)
{
  setRenameEnabled(0, true);
#ifdef ALLOW_VARIABLE_MODIFY
  setRenameEnabled(1, true);
#endif
  nd = 0;
}

QSAVariableItem::QSAVariableItem(QListViewItem *parent, QListViewItem *after)
  : QListViewItem(parent, after)
{
  setRenameEnabled(0, true);
#ifdef ALLOW_VARIABLE_MODIFY
  setRenameEnabled(1, true);
#endif

  nd = 0;
}

QSAVariableItem::~QSAVariableItem()
{
  delete nd;
}

void QSAVariableItem::paintCell(QPainter *p, const QColorGroup &cg, int column, int width, int align)
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
  if (listView()->firstChild() != this) {
    if (nextSibling() != itemBelow() && itemBelow()->depth() < depth()) {
      int d = depth() - itemBelow()->depth();
      p->drawLine(-listView()->treeStepSize() * d, height() - 1, 0, height() - 1);
    }
  }
  p->drawLine(0, height() - 1, width, height() - 1);
  p->drawLine(width - 1, 0, width - 1, height());
  p->restore();
}

void QSAVariableItem::paintBranches(QPainter *p, const QColorGroup &cg,
                                    int w, int y, int h)
{
  QColorGroup g(cg);
  g.setColor(QColorGroup::Base, backgroundColor());
  QListViewItem::paintBranches(p, g, w, y, h);
}

QColor QSAVariableItem::backgroundColor()
{
  updateBackColor();
  return backColor;
}

void QSAVariableItem::updateBackColor()
{
  if (listView()->firstChild() == this) {
    backColor = *backColor1;
    return ;
  }

  QListViewItemIterator it(this);
  --it;
  if (it.current()) {
    if (((QSAVariableItem *) it.current())->backColor == *backColor1)
      backColor = *backColor2;
    else
      backColor = *backColor1;
  } else {
    backColor == *backColor1;
  }
}

static void fixValueType(QString &value, QString &type)
{
  int i = type.findRev(':');
  if (i == 0) {
    type.remove(0, 1);
  } else {
    value += type.left(i);
    type.remove(0, i + 1);
  }
}

void QSAVariableItem::evaluate(int col)
{
  IdeWindow *wb = get_workbench(listView());
  QuickInterpreter *ip = get_quick_interpreter(wb->project->interpreter());
  QuickDebugger *dbg = (wb ? wb->project->debugger() : 0);
  if (!dbg || dbg->mode() == Debugger::Disabled)
    return;

  if (col == 0) {
    if (text(0).isEmpty())
      return ;
    QString val;
    QString type;

    QListViewItem *i = firstChild();
    while (i) {
      QListViewItem *n = i->nextSibling();
      delete i;
      i = n;
    }

    enum { Name, Value, Type } state;
    state = Name;

    QString txt = text(0);
    if (txt == "Local Variables") {
      setRenameEnabled(0, false);
#ifdef ALLOW_VARIABLE_MODIFY
      setRenameEnabled(1, false);
#endif
      txt = "||Activation||";
    } else if (txt == "Global Variables") {
      setRenameEnabled(0, false);
#ifdef ALLOW_VARIABLE_MODIFY
      setRenameEnabled(1, false);
#endif
      txt = "||Global||";
    }

    if (dbg->watch(txt, type, val)) {
      setText(2, type);
      if (val[ 0 ] != '{') {
        setText(1, val);
#ifdef ALLOW_VARIABLE_MODIFY
        setRenameEnabled(1, true);
#endif
      } else {
        // parse the value and save it in a tree
        Node *n = new Node;
        Node *root = n;
        n->name = text(0);
        n->value = "";
        n->type = type;
        n->valueEditable = false;
#ifdef ALLOW_VARIABLE_MODIFY
        setRenameEnabled(1, false);
#endif

        for (int i = 0; i < (int) val.length(); ++i) {
          if (val[ i ] == '{') {
            Node *n2 = new Node;
            state = Name;
            n2->parent = n;
            n = n2;
            if (!n->parent->firstChild) {
              n->parent->firstChild = n->parent->lastChild = n;
            } else {
              n->parent->lastChild->nextSibling = n;
              n->parent->lastChild = n;
            }
            continue;
          } else if (val[ i ] == ',' && state == Type) {
            n->nextSibling = new Node;
            state = Name;
            n->nextSibling->parent = n->parent;
            n = n->nextSibling;
            continue;
          } else if (val[ i ] == '}') {
            n = n->parent;
            n->valueEditable = false;
            continue;
          } else if (val[ i ] == '=') {
            state = Value;
            continue;
          } else if (val[ i ] == ':') {
            state = Type;
            // no continue
          }

          switch (state) {
            case Name:
              n->name += val[ i ];
              break;
            case Value:
              n->value += val[ i ];
              break;
            case Type:
              n->type += val[ i ];
              break;
          }
        }

        // build up the listview
        n = root;
        QSAVariableItem *item = this;
        while (n) {
          item->setText(0, n->name);
          fixValueType(n->value, n->type);
          item->setText(1, n->value);
          item->setText(2, n->type);
          if (n->type == "FactoryObject")
            item->setVisible(false);
#ifdef ALLOW_VARIABLE_MODIFY
          item->setRenameEnabled(1, n->valueEditable);
#endif

          n->item = item;
          item->setNode(n);
          if (item->text(0) == "Local Variables" ||
              item->text(0) == "Global Variables") {
            item->setOpen(true);
            item->setText(2, "Scope");
          } else {
            item->setOpen(false);
          }
          if (n->firstChild) {
            item = new QSAVariableItem(n->item, 0);
            n = n->firstChild;
          } else if (n->nextSibling) {
            item = new QSAVariableItem(n->parent->item, n->item);
            n = n->nextSibling;
          } else {
            n = n->parent;
            while (n) {
              if (n->nextSibling) {
                item = new QSAVariableItem(n->parent->item, n->item);
                n = n->nextSibling;
                break;
              }
              n = n->parent;
            }
          }
        }

        // get rid of functions in the tree
        n = root;
        while (n) {
          if (n->name.startsWith("[[") ||
              n->type == "Function" ||
              n->type == "Type" ||
              n->type == "InternalFunction" ||
              n->type == "DeclaredFunction" ||
              n->type == "InternalFunctionType" ||
              n->value == "(Internal function)")
            n->item->setVisible(false);
          if (ip->staticGlobalObjects().findIndex(n->name) != -1)
            n->item->setVisible(false);
          if (n->firstChild) {
            n = n->firstChild;
          } else if (n->nextSibling) {
            n = n->nextSibling;
          } else {
            n = n->parent;
            while (n) {
              if (n->nextSibling) {
                n = n->nextSibling;
                break;
              }
              n = n->parent;
            }
          }
        }
      }
      setOpen(true);
    } else {
      setText(1, "");
      setText(2, "unknown");
    }
  } else if (col == 1) {
    if (text(1).isEmpty())
      return ;
    QString var = (text(0) == "Local Variables" ||
                   text(0) == "Global Variables") ?
                  QString::null : text(0);
    Node *parentNode = 0;
    if (node()) {
      Node *n = node()->parent;
      while (n) {
        if (!var.isEmpty())
          var.prepend(".");
        var.prepend(n->name);
        parentNode = n;
        n = n->parent;
      }
    }
    var.replace(QRegExp("Local Variables"), "||Activation||");
    var.replace(QRegExp("Global Variables"), "||Global||");
    dbg->setVariable(var, text(1));
    if (!parentNode)
      evaluate(0);
    else
      parentNode->item->evaluate(0);
  }
}

QSAVariableView::QSAVariableView(QWidget *parent, const char *)
  : QListView(parent, "qsa_debugger_variableview")
{
  init_colors();
  addColumn(tr("Variable"));
  addColumn(tr("Value"));
  addColumn(tr("Type"));
  header()->setStretchEnabled(true);
  header()->setResizeEnabled(true);
  setAllColumnsShowFocus(true);
  setSorting(-1);
  setRootIsDecorated(true);
  setHScrollBarMode(AlwaysOff);
  setVScrollBarMode(AlwaysOn);

  (void) new QSAVariableItem(this, 0);

  connect(this, SIGNAL(itemRenamed(QListViewItem *, int, const QString &)),
          this, SLOT(evaluate(QListViewItem *, int)));
}

void QSAVariableView::evaluate(QListViewItem *i, int col)
{
  if (!i)
    return ;
  if (!i->nextSibling()) {
    QListViewItem *ni = new QSAVariableItem(this, i);
    setCurrentItem(ni);
  }

  if (i->text(1) [ 0 ] != '"' && i->text(2) == "String")
    i->setText(1, "\"" + i->text(1) + "\"");

  ((QSAVariableItem *) i)->evaluate(col);
}

void QSAVariableView::evaluateAll()
{
  QListViewItem *i = firstChild();
  while (i) {
    ((QSAVariableItem *) i)->evaluate(0);
    i = i->nextSibling();
  }
}

void QSAVariableView::addWatch(const QString &var)
{
  QListViewItem *i = firstChild();
  QListViewItem *last = 0;
  while (i) {
    ((QSAVariableItem *) i)->evaluate(0);
    last = i;
    i = i->nextSibling();
  }
  if (!last)
    return;
  last->setText(0, var);
  evaluate(last, 0);
}

void QSAVariableView::keyPressEvent(QKeyEvent *e)
{
  static QString legalChars = "abcdefghijklmnopqrstuvwxyzABSCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_";
  if (legalChars.find(e->text() [ 0 ]) == -1) {
    if (e->key() == Key_Delete) {
      if (currentItem() && !currentItem()->text(0).isEmpty() &&
          currentItem()->text(0) != "Local Variables")
        delete currentItem();
    }
    return ;
  }
  QListViewItem *i = firstChild();
  QListViewItem *last = 0;
  while (i) {
    last = i;
    i = i->nextSibling();
  }
  if (!last)
    return ;
  if (last && !last->text(0).isEmpty()) {
    QListViewItem *ni = new QSAVariableItem(this, i);
    setCurrentItem(ni);
    last = ni;
  } else {
    setCurrentItem(last);
  }
#ifdef ALLOW_VARIABLE_MODIFY
  last->startRename(0);
#endif

  QLineEdit *l = (QLineEdit *) child(0, "QLineEdit");
  if (l)
    l->setText(e->text());
}
