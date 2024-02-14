/****************************************************************************
** $Id: qsadebuggerinterface.cpp  beta3   edited Mar 6 11:51 $
**
** Copyright (C) 2001-2002 Trolltech AS.  All rights reserved.
**
** This file is part of the AbanQ Developer for Applications framework (QSA).
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

#include "qsadebuggerinterface.h"
#include "quickinterpreter.h"
#include "quickdebugger.h"
#include "qsavariableview.h"
#include "qsastackview.h"
#include "quickobjects.h"
#include "../ide/idewindow.h"
#include "../qsa/qsproject.h"
#include "qsinterpreter.h"
#include "qsproject.h"

#include <qaction.h>
#include <qpixmap.h>
#include <qapplication.h>
#include <viewmanager.h>
#include <qmainwindow.h>
#include <qdockwindow.h>
#include <qvariant.h>
#include <qprocess.h>
#include <qregexp.h>
#include <qtimer.h>
#if defined(Q_OS_WIN32)
#include <qt_windows.h>
#endif
#include <qinputdialog.h>
#include <qwidgetlist.h>

extern QWidgetList *qt_modal_stack;

extern QuickInterpreter *get_quick_interpreter(QSInterpreter *);
extern IdeWindow *get_workbench(QWidget *);

static QIconSet createIconSet(const QString &name)
{
  QIconSet ic(QPixmap::fromMimeSource("" + name));
  ic.setPixmap(QPixmap::fromMimeSource("d_" + name),
               QIconSet::Small, QIconSet::Disabled);
  return ic;
}

class EventFilter : public QObject
{
  Q_OBJECT

public:
  EventFilter(QObject *parent, IdeWindow *i) :
    QObject(parent), enabled(false), mw(i) {}

  void restoreModal() {
    mw->clearWFlags(Qt::WShowModal);
    if (qt_modal_stack && qt_modal_stack->findRef(mw) != -1)
      qt_modal_stack->remove();
  }
  void overrideModal() {
    mw->setWFlags(Qt::WShowModal);
    if (qt_modal_stack && qt_modal_stack->getFirst() != mw) {
      if (qt_modal_stack->findRef(mw) != -1)
        qt_modal_stack->remove();
      qt_modal_stack->insert(0, mw);
    }
  }
  void setEnabled(bool e = true) {
    enabled = e;
    if (enabled)
      overrideModal();
    else
      restoreModal();
  }
  bool isEnabled() const {
    return enabled;
  }
  bool eventFilter(QObject *, QEvent *);

private:
  bool enabled;
  IdeWindow *mw;
};

bool EventFilter::eventFilter(QObject *o, QEvent *e)
{
  if (!enabled || !o->isWidgetType())
    return false;

  QWidget *w = static_cast<QWidget *>(o);
  if (!w->isModal())
    return false;

  if (mw == get_workbench(w))
    return false;

  overrideModal();

  switch (e->type()) {
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseButtonDblClick:
    case QEvent::MouseMove:
    case QEvent::Wheel:
    case QEvent::KeyPress:
    case QEvent::KeyRelease:
    case QEvent::FocusIn:
    case QEvent::FocusOut:
    case QEvent::Enter:
    case QEvent::Leave:
    case QEvent::Close:
      return true;
  }
  return false;
}

QSADebuggerInterface::QSADebuggerInterface(IdeWindow *i)
  : actionCont(0),
    actionNext(0),
    actionStep(0),
    watchVars(0),
    callStack(0),
    mw(i),
    waiting(false),
    actionsCreated(false)
{
  ip = get_quick_interpreter(mw->project->interpreter());
  dbg = ip->debuggerEngine();

  eventFilter = new EventFilter(this, i);
  qApp->installEventFilter(eventFilter);
}

QSADebuggerInterface::~QSADebuggerInterface()
{
  if (watchVars)
    delete watchVars->parentWidget();
  if (callStack)
    delete callStack->parentWidget();
}

QStringList QSADebuggerInterface::featureList() const
{
  QStringList lst;
  lst << "AbanQ Developer Debugger Cont";
  lst << "AbanQ Developer Debugger Next";
  lst << "AbanQ Developer Debugger Step";
  return lst;
}

QAction *QSADebuggerInterface::create(const QString &action, QObject *parent)
{
  if (!actionsCreated) {
    connect(ip, SIGNAL(runtimeError()), this, SLOT(runtimeError()));
    connect(dbg, SIGNAL(stopped(bool &)), this, SLOT(debuggerStopped(bool &)));
    connect(dbg, SIGNAL(modeChanged(int)), this, SLOT(changedDebugMode(int)));

    connect(mw->project, SIGNAL(projectChanged()), this, SLOT(projectChanged()));
    connect(mw->project, SIGNAL(projectEvaluated()), this, SLOT(projectEvaluated()));
  }
  actionsCreated = true;

  if (action == "AbanQ Developer Debugger Cont") {
    if (!actionCont) {
      actionCont = new QAction(tr("Continue"), createIconSet("project.png"),
                               tr("C&ontinue..."), Qt::Key_F10, parent, "aqdev_continue");
      actionCont->setEnabled(false);
      connect(actionCont, SIGNAL(activated()), this, SLOT(debugContinue()));
    }
    return actionCont;
  } else if (action == "AbanQ Developer Debugger Next") {
    if (!actionNext) {
      actionNext = new QAction(tr("Step Over"), createIconSet("stepover.png"),
                               tr("Step &Over"), Qt::Key_F11, parent, "aqdev_next");
      actionNext->setEnabled(false);
      connect(actionNext, SIGNAL(activated()), this, SLOT(debugNext()));
    }
    return actionNext;
  } else if (action == "AbanQ Developer Debugger Step") {
    if (!actionStep) {
      actionStep = new QAction(tr("Step Into"), createIconSet("steptonext.png"),
                               tr("Step &Into"), Qt::Key_F12, parent, "aqdev_step");
      actionStep->setEnabled(false);
      connect(actionStep, SIGNAL(activated()), this, SLOT(debugStep()));
    }
    return actionStep;
  }
  return 0;
}

QString QSADebuggerInterface::group(const QString &name) const
{
  return "AbanQ Developer";
}

bool QSADebuggerInterface::location(const QString &name,
                                    ActionInterface::Location l) const
{
  Q_UNUSED(l)
  return false;
}

void QSADebuggerInterface::debuggerStopped(bool &ret)
{
  dbg->setMode(Debugger::Stop);

  if (waiting || dbg->sourceId() == -1 ||
      !ip->objectOfSourceId(dbg->sourceId())) {
    ret = false;
    return;
  }

  QApplication::restoreOverrideCursor();
  QuickInterpreter::enableTimers(false);
  waitAction();
  QuickInterpreter::enableTimers(true);

  ret = true;
}

void QSADebuggerInterface::waitAction()
{
  if (waiting)
    debugContinue();
  waiting = true;
  qApp->exec();
}

void QSADebuggerInterface::wakeupWait()
{
  if (!waiting)
    return;
  waiting = false;
  qApp->exit();
}

void QSADebuggerInterface::debugNext()
{
  dbg->setMode(Debugger::Next);
  wakeupWait();
}

void QSADebuggerInterface::debugStep()
{
  dbg->setMode(Debugger::Step);
  wakeupWait();
}

void QSADebuggerInterface::debugContinue()
{
  dbg->setMode(Debugger::Continue);
  wakeupWait();
}

void QSADebuggerInterface::runtimeError()
{
  dbg->setMode(Debugger::Stop);
}

void QSADebuggerInterface::projectChanged()
{
  dbg->setMode(Debugger::Disabled);
}

void QSADebuggerInterface::projectEvaluated()
{
  dbg->setMode(Debugger::Continue);
}

void QSADebuggerInterface::stackViewScopeChanged(int level)
{
  dbg->setScopeLevel(level);
  watchVars->evaluateAll();
  dbg->setScopeLevel(-1);
}

void QSADebuggerInterface::setupWatchView()
{
  if (!actionsCreated || !mw || watchVars)
    return;

  QDockWindow *dw = new QDockWindow(QDockWindow::OutsideDock, mw, 0);
  mw->addDockWindow(dw, Qt::DockRight);
  dw->setFixedExtentHeight(150);
  dw->setResizeEnabled(true);
  dw->setCloseMode(QDockWindow::Always);
  watchVars = new QSAVariableView(dw);
  watchVars->setMinimumWidth(300);
  dw->setWidget(watchVars);
  watchVars->show();
  watchVars->addWatch("this"); // ###
  watchVars->addWatch("Local Variables");
  dw->setCaption(tr("Watch Variables"));
  mw->setAppropriate(dw, true);
}

void QSADebuggerInterface::setupCallStack()
{
  if (!actionsCreated || !mw || callStack)
    return;

  QDockWindow *dw = new QDockWindow(QDockWindow::OutsideDock, mw, 0);
  mw->addDockWindow(dw, Qt::DockRight);
  dw->setResizeEnabled(true);
  dw->setFixedExtentHeight(150);
  dw->setCloseMode(QDockWindow::Always);
  callStack = new QSAStackView(mw, dw);
  dw->setWidget(callStack);
  callStack->show();
  callStack->setMinimumWidth(300);
  dw->setCaption(tr("Call Stack"));
  dw->hide();

  connect(callStack, SIGNAL(scopeChanged(int)),
          this, SLOT(stackViewScopeChanged(int)));
}

void QSADebuggerInterface::setupDocks(bool debugging)
{
  setupWatchView();
  setupCallStack();

  if (debugging) {
    if (callStack) {
      mw->setAppropriate(
        static_cast<QDockWindow *>(callStack->parentWidget()), true
      );
      callStack->parentWidget()->show();
      callStack->updateStack();
    }
    if (watchVars) {
      mw->setAppropriate(
        static_cast<QDockWindow *>(watchVars->parentWidget()), true
      );
      watchVars->parentWidget()->show();
      watchVars->evaluateAll();
    }
  } else {
    if (callStack) {
      mw->setAppropriate(
        static_cast<QDockWindow *>(callStack->parentWidget()), false
      );
      callStack->parentWidget()->hide();
      callStack->clear();
    }
    if (watchVars) {
      mw->setAppropriate(
        static_cast<QDockWindow *>(watchVars->parentWidget()), false
      );
      watchVars->parentWidget()->hide();
    }
  }
}

void QSADebuggerInterface::setActionsEnabled(bool enabled)
{
  if (actionsCreated) {
    actionStep->setEnabled(enabled);
    actionNext->setEnabled(enabled);
    actionCont->setEnabled(enabled);
  }
}

void QSADebuggerInterface::changedDebugMode(int m)
{
  switch (m) {
    case Debugger::Disabled:
      if (waiting) {
        dbg->setMode(Debugger::Stop);
        wakeupWait();
        dbg->setMode(Debugger::Disabled);
      }
      setActionsEnabled(false);
      setupDocks(false);
      eventFilter->setEnabled(false);
    case Debugger::Continue: // fallback
      break;
    case Debugger::Next:
    case Debugger::Step:
    case Debugger::Stop: {
      bool noError = !ip->hadError();
      setActionsEnabled(noError);
      setupDocks();
      eventFilter->setEnabled(noError);
    }
    break;
  }
}

#include "qsadebuggerinterface.moc"
