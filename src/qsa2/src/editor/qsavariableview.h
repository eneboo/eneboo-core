/****************************************************************************
** $Id: qsavariableview.h  beta3   edited Dec 10 13:07 $
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

#ifndef QSAVARIABLEVIEW_H
#define QSAVARIABLEVIEW_H

#include <qlistview.h>

class QKeyEvent;

class QSAVariableItem : public QListViewItem
{
public:
  struct Node {
    Node() : valueEditable(TRUE), parent(0), nextSibling(0), firstChild(0), lastChild(0) {}
    QString name;
    QString value;
    QString type;
    bool valueEditable;
    Node *parent;
    Node *nextSibling;
    Node *firstChild;
    Node *lastChild;
    QSAVariableItem *item;
  };

  QSAVariableItem(QListView *parent, QListViewItem *after);
  QSAVariableItem(QListViewItem *parent, QListViewItem *after);
  ~QSAVariableItem();

  void paintCell(QPainter *p, const QColorGroup &cg, int column, int width, int align);
  void updateBackColor();

  void evaluate(int col);
  void paintBranches(QPainter *p, const QColorGroup &cg,
                     int w, int y, int h);

  void setNode(Node *n) {
    nd = n;
  }
  Node *node() const {
    return nd;
  }

private:
  QColor backgroundColor();
  QColor backColor;
  Node *nd;

};

class QSAVariableView : public QListView
{
  Q_OBJECT

public:
  QSAVariableView(QWidget *parent, const char *name = 0);
  void addWatch(const QString &var);

public slots:
  void evaluateAll();

protected:
  void keyPressEvent(QKeyEvent *e);

private slots:
  void evaluate(QListViewItem *i, int col);
};

#endif
