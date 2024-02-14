/****************************************************************************
** $Id: qsastackview.h  beta3   edited Mar 4 15:37 $
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

#ifndef QSASTACKVIEW_H
#define QSASTACKVIEW_H

#include <qlistview.h>

class IdeWindow;

class QSAStackItem : public QListViewItem
{
public:
  QSAStackItem(QListView *parent, QListViewItem *after);

  void paintCell(QPainter *p, const QColorGroup &cg, int column, int width, int align);
  void updateBackColor();

  void setSourceId(int id) {
    sid = id;
  }
  int sourceId() const {
    return sid;
  }
  void setLevel(int l) {
    lvl = l;
  }
  int level() const {
    return lvl;
  }

private:
  QColor backgroundColor();
  QColor backColor;
  int sid;
  int lvl;
};

class QSAStackView : public QListView
{
  Q_OBJECT

public:
  QSAStackView(IdeWindow *w, QWidget *parent, const char *name = 0);
  void updateStack();

signals:
  void scopeChanged(int level);

private slots:
  void changeScope(QListViewItem *i);

private:
  IdeWindow *wb;
};

#endif
