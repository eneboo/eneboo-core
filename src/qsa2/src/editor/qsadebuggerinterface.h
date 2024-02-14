/****************************************************************************
** $Id: qsadebuggerinterface.h  beta3   edited Mar 5 10:15 $
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

#ifndef QSADEBUGGERINTERFACE_H
#define QSADEBUGGERINTERFACE_H

#include <actioninterface.h>
#include <qobject.h>

class QSAVariableView;
class QSAStackView;
class QuickInterpreter;
class QuickDebugger;
class IdeWindow;
class EventFilter;

class QSADebuggerInterface : public QObject
{
  Q_OBJECT

public:
  QSADebuggerInterface(IdeWindow *i);
  virtual ~QSADebuggerInterface();

  QStringList featureList() const;
  QAction *create(const QString &, QObject *parent = 0);
  QString group(const QString &) const;
  bool location(const QString &name, ActionInterface::Location l) const;

private slots:
  void debuggerStopped(bool &ret);
  void debugNext();
  void debugStep();
  void debugContinue();
  void runtimeError();
  void projectChanged();
  void projectEvaluated();
  void stackViewScopeChanged(int level);
  void changedDebugMode(int m);

private:
  void waitAction();
  void wakeupWait();
  void setupWatchView();
  void setupCallStack();
  void setupDocks(bool debugging = true);
  void setActionsEnabled(bool enabled = true);

private:
  QAction *actionCont;
  QAction *actionNext;
  QAction *actionStep;
  QSAVariableView *watchVars;
  QSAStackView *callStack;
  IdeWindow *mw;
  QuickInterpreter *ip;
  QuickDebugger *dbg;
  EventFilter *eventFilter;
  bool waiting;
  bool actionsCreated;
};

#endif
