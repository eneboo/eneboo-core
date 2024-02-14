/****************************************************************************
** $Id: qsabrowser.cpp  1.1.5   edited 2006-02-23T15:39:57$
**
** Copyright (C) 2001-2006 Trolltech AS.  All rights reserved.
**
** This file is part of the Qt Script for Applications framework (QSA).
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.
**
** Licensees holding a valid Qt Script for Applications license may use
** this file in accordance with the Qt Script for Applications License
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

#include "qsabrowser.h"
#include "qsaeditor.h"
#include "qsavariableview.h"

#include "quickinterpreter.h"
#include "quickdebugger.h"

#include "qsinterpreter.h"
#include "qsproject.h"

extern QuickInterpreter *get_quick_interpreter(QSInterpreter *);

QSAEditorBrowser::QSAEditorBrowser(Editor *e)
  : EditorBrowser(e)
{
}

void QSAEditorBrowser::showHelp(const QString &str)
{
  QSAEditor *curEditor_ = ::qt_cast<QSAEditor *>(curEditor);

  if (curEditor_ && curEditor_->isDebugging()) {
    QString s = str.simplifyWhiteSpace();
    static QString legalChars = "abcdefghijklmnopqrstuvwxyzABSCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_";
    while (s.length() > 0) {
      if (legalChars.find(s[0]) == -1)
        s.remove(0, 1);
      else
        break;
    }
    while (s.length() > 0) {
      if (legalChars.find(s[(int)s.length() - 1 ]) == -1)
        s.remove(s.length() - 1, 1);
      else
        break;
    }

    if (s[(int)s.length() - 1 ] == ';')
      s.remove(s.length() - 1, 1);

    QString type, val;
    QuickInterpreter *ip = get_quick_interpreter(curEditor_->interpreter());
    ip->debuggerEngine()->watch(s, type, val);
    if (!val.isEmpty() && !type.isEmpty()) {
      QWidget *w = curEditor->topLevelWidget();
      QSAVariableView *o = ::qt_cast<QSAVariableView *>(
                             w->child("qsa_debugger_variableview", "QSAVariableView")
                           );
      if (o)
        o->addWatch(s);
    }
  }
}

bool QSAEditorBrowser::eventFilter(QObject *o, QEvent *e)
{
  if (!((QSAEditor *)curEditor)->isDebugging())
    return EditorBrowser::eventFilter(o, e);

  if (e->type() == QEvent::KeyPress) {
    QKeyEvent *ke = (QKeyEvent *)e;
    switch (ke->key()) {
      case Key_Up:
      case Key_Down:
      case Key_Left:
      case Key_Right:
      case Key_Prior:
      case Key_Next:
      case Key_Home:
      case Key_End:
      case Key_F9:
      case Key_F10:
      case Key_F11:
      case Key_F5:
        break;
      default:
        return TRUE;
    }
  }
  return EditorBrowser::eventFilter(o, e);
}
