/***************************************************************************
 idewindow.ui.h
 -------------------
 begin                : 09/04/2011
 copyright            : (C) 2003-2011 by InfoSiAL S.L.
 email                : mail@infosial.com
 ***************************************************************************/
/***************************************************************************
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; version 2 of the License.               *
 ***************************************************************************/
/***************************************************************************
 Este  programa es software libre. Puede redistribuirlo y/o modificarlo
 bajo  los  t?rminos  de  la  Licencia  P?blica General de GNU   en  su
 versi?n 2, publicada  por  la  Free  Software Foundation.
 ***************************************************************************/

/****************************************************************************
** $Id: idewindow.ui.h  1.1.5   edited 2006-06-12T22:23:52$
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

#include <stdlib.h>
#include <qtimer.h>
#include <qstyle.h>
#include <qprinter.h>
#include <qpainter.h>
#include <qpaintdevicemetrics.h>
#include <qsimplerichtext.h>

#include "../editor/qsaeditorinterface.h"
#include <quickdebugger.h>
#include <quickinterpreter.h>
#include <qsettings.h>

extern QuickInterpreter *get_quick_interpreter(QSInterpreter *);

#define AQ_SETTINGS_KEY_BD(B)                                 \
  QSettings config;                                           \
  config.setPath("InfoSiAL", "AbanQ", QSettings::User);  \
  QString key("/AbanQ/Workbench/" + QString(B) + "/");

static inline QString aqFormatTabLabel(QSScript *s)
{
  if (s->fileName().isEmpty() || s->fileName().startsWith("#"))
    return s->name();
  else
    return s->baseFileName();
}

static inline QString aqFormatTabToolTip(QSScript *s)
{
  if (s->fileName().isEmpty() || s->fileName().startsWith("#"))
    return s->name();
  else
    return s->fileName();
}

static QTextEdit *debugoutput = 0;
static void (*qt_default_message_handler)(QtMsgType, const char *msg);

void debugMessageOutput(QtMsgType type, const char *msg)
{
  // So that we don't override others defaults...
  if (qt_default_message_handler) {
    qt_default_message_handler(type, msg);
  } else {
    fprintf(stderr, "%s\n", msg);
    fflush(stderr);
  }

  if (type == QtDebugMsg) {
    if (debugoutput) {
      if (QStyleSheet::mightBeRichText(msg))
        debugoutput->append(msg);
      else
        debugoutput->append(QStyleSheet::convertFromPlainText(msg));
    }
  } else if (type == QtFatalMsg) {
    fprintf(stderr, msg);
    abort();
  }
  qApp->flush();
}

#ifdef _WIN32
#define STDCALL __stdcall
#else
#define STDCALL
#endif
extern "C"
{
  char *STDCALL  AStyleMain(const char *textIn,
                            const char *options,
                            void(STDCALL *errorHandler)(int, char *),
                            char * (STDCALL *memoryAlloc)(unsigned long));
}

static void  STDCALL aqAstyleErrorHandler(int errorNumber, char *errorMessage)
{
  QString msg(QString::fromLatin1("AStyle error ") +
              QString::number(errorNumber) +
              QString::fromLatin1("\n") +
              QString::fromLatin1(errorMessage));
  qDebug(msg);
}

static char *STDCALL aqAstyleMemoryAlloc(unsigned long memoryNeeded)
{
  char *buffer = new(std::nothrow) char [memoryNeeded];
  return buffer;
}

static inline QString fix_string(const QString &s)
{
  QString ns(s);
  ns.replace("&", "&amp;");
  ns.replace("<", "&lt;");
  ns.replace(">", "&gt;");
  return ns;
}

static inline QIconSet createIconSet(const QString &name, bool disabled = true)
{
  QIconSet ic(QPixmap::fromMimeSource(name));
  if (disabled) {
    if (name.startsWith("designer")) {
      QString prefix = "designer_";
      int right = name.length() - prefix.length();
      ic.setPixmap(QPixmap::fromMimeSource(prefix + "d_" + name.right(right)),
                   QIconSet::Small, QIconSet::Disabled);
    } else {
      ic.setPixmap(QPixmap::fromMimeSource(QString::fromLatin1("d_") + name),
                   QIconSet::Small, QIconSet::Disabled);
    }
  }
  return ic;
}

void IdeWindow::saveScript(QSScript *s)
{
  Q_ASSERT(s);

  if ((dbg->mode() != Debugger::Disabled && dbg->mode() != Debugger::Continue) ||
      project->interpreter()->isRunning()) {
    dbg->setMode(Debugger::Disabled);
    if (project->scriptsModified())
      QTimer::singleShot(0, this, SLOT(evaluateProject()));
    return;
  }

  QString fileName(s->fileName());
  if (fileName.isEmpty())
    return;

  QSEditor *editor = project->editor(s);
  if (!editor || !editor->isModified())
    return;
  QPtrList<QSEditor> editors = project->editors();
  for (QSEditor *ed = editors.first(); ed; ed = editors.next()) {
    if (ed->script()->fileName() == fileName)
      ed->commit();
  }

  if (fileName.startsWith("#")) {
    QString saveFunction(fileName.section('@', 0, 0).mid(1));
    QSArgumentList l;
    l << QVariant(s->name());
    project->interpreter()->call(saveFunction, l);
    tabWidget->setTabLabel(editor, aqFormatTabLabel(s));
    tabWidget->setTabToolTip(editor, aqFormatTabToolTip(s));
    return;
  } else {
    QPtrList<QSScript> scripts(project->scripts());
    QPtrListIterator<QSScript> it(scripts);
    QSScript *scr;
    while ((scr = it())) {
      if (scr != s && !scr->baseFileName().isEmpty() &&
          scr->baseFileName() == s->baseFileName())
        scr->setCode(s->code());
    }
  }

  QFile file(fileName);
  if (!file.open(IO_WriteOnly)) {
    QMessageBox::information(
      this, QString::fromLatin1("Save script failed"),
      QString::fromLatin1("The file '%1' could not be opened for\n"
                          "writing. Script '%2' was not saved.")
      .arg(fileName).arg(s->name()),
      QMessageBox::Ok
    );
    return;
  }

  tabWidget->setTabLabel(editor, aqFormatTabLabel(s));
  tabWidget->setTabToolTip(editor, aqFormatTabToolTip(s));
  QTextStream stream(&file);
  stream.setEncoding(QTextStream::Latin1);
  stream << s->code().remove("var form = this; //auto-added\n");
}

void IdeWindow::scriptSave()
{
  QSScript *script = 0;
  QSEditor *activeEditor = static_cast<QSEditor *>(tabWidget->currentPage());
  if (activeEditor)
    script = activeEditor->script();
  else if (projectBrowser)
    script = projectBrowser->currentScript();
  saveScript(script);
}

void IdeWindow::scriptPrint()
{
  QSEditor *activeEditor = static_cast<QSEditor *>(tabWidget->currentPage());
  if (!activeEditor)
    return;

  QString scriptName = activeEditor->script()->name();
  QTextEdit *te = activeEditor->textEdit();
  QString printString = te->text();

  // fix formatting
  printString.replace('&', QString::fromLatin1("&amp;"));
  printString.replace('<', QString::fromLatin1("&lt;"));
  printString.replace('>', QString::fromLatin1("&gt;"));
  printString.replace('\n', QString::fromLatin1("<br>\n"));
  printString.replace('\t', QString::fromLatin1("        "));
  printString.replace(' ', QString::fromLatin1("&nbsp;"));

  printString = QString::fromLatin1("<html><body>") +
                printString +
                QString::fromLatin1("</body></html>");

  QPrinter printer(QPrinter::HighResolution);
  printer.setFullPage(true);
  if (printer.setup(this)) {
    QPainter p(&printer);
    // Check that there is a valid device to print to.
    if (!p.device()) return;
    QPaintDeviceMetrics metrics(p.device());
    int dpiy = metrics.logicalDpiY();
    int margin = (int)((2 / 2.54) * dpiy); // 2 cm margins
    QRect body(margin, margin, metrics.width() - 2 * margin,
               metrics.height() - 2 * margin);
    QFont font(te->QWidget::font());
    font.setPointSize(10);   // we define 10pt to be a nice base size for printing

    QSimpleRichText richText(printString, font,
                             te->context(),
                             te->styleSheet(),
                             te->mimeSourceFactory(),
                             body.height());
    richText.setWidth(&p, body.width());
    QRect view(body);
    int page = 1;
    do {
      richText.draw(&p, body.left(), body.top(), view, colorGroup());
      view.moveBy(0, body.height());
      p.translate(0 , -body.height());
      p.setFont(font);
      QString renderText = scriptName + QString::fromLatin1(", ") +
                           QString::number(page);
      p.drawText(view.right() - p.fontMetrics().width(renderText),
                 view.bottom() + p.fontMetrics().ascent() + 5,
                 renderText);
      if (view.top()  >= richText.height())
        break;
      printer.newPage();
      page++;
    } while (true);
  }
}

void IdeWindow::editUndo()
{
  QSEditor *editor = static_cast<QSEditor *>(tabWidget->currentPage());
  if (!editor)
    return;
  editor->undo();
}

void IdeWindow::editRedo()
{
  QSEditor *editor = static_cast<QSEditor *>(tabWidget->currentPage());
  if (!editor)
    return;
  editor->redo();
}

void IdeWindow::editCut()
{
  QSEditor *editor = static_cast<QSEditor *>(tabWidget->currentPage());
  if (!editor)
    return;
  editor->cut();
}

void IdeWindow::editCopy()
{
  QSEditor *editor = static_cast<QSEditor *>(tabWidget->currentPage());
  if (!editor)
    return;
  editor->copy();
}

void IdeWindow::editPaste()
{
  QSEditor *editor = static_cast<QSEditor *>(tabWidget->currentPage());
  if (!editor)
    return;
  editor->paste();
}

void IdeWindow::editFindNext()
{
  findText->radioForward->setChecked(true);
  editFind();
}

void IdeWindow::editFindPrev()
{
  findText->radioBackward->setChecked(true);
  editFind();
}

void IdeWindow::editFind()
{
  if (findText->comboFind->currentText().isEmpty()) {
    findText->show();
    return;
  }
  QSEditor *editor = static_cast<QSEditor *>(tabWidget->currentPage());
  if (!editor)
    return;
  QString findStr = findText->comboFind->currentText();
  findText->comboFind->insertItem(findStr);
  bool caseSen = findText->checkCase->isChecked();
  bool wholeWords = findText->checkWhole->isChecked();
  bool startAtCur = !findText->checkStart->isChecked();
  bool forw = findText->radioForward->isChecked();
  if (!editor->find(findStr, caseSen, wholeWords, forw, startAtCur))
    editor->find(findStr, caseSen, wholeWords, forw, !startAtCur);
}

void IdeWindow::editReplace()
{
  editReplace(false);
}

void IdeWindow::editReplaceAll()
{
  editReplace(true);
}

void IdeWindow::editReplace(bool all)
{
  QSEditor *editor = static_cast<QSEditor *>(tabWidget->currentPage());
  if (!editor)
    return;
  QString findStr = replaceText->comboFind->currentText();
  replaceText->comboFind->insertItem(findStr);
  QString replaceStr = replaceText->comboReplace->currentText();
  replaceText->comboFind->insertItem(replaceStr);
  bool caseSen = replaceText->checkCase->isChecked();
  bool wholeWords = replaceText->checkWhole->isChecked();
  bool startAtCur = !replaceText->checkStart->isChecked();
  bool forw = replaceText->radioForward->isChecked();
  if (!editor->replace(findStr, replaceStr, caseSen, wholeWords, forw, startAtCur, all) && !all)
    editor->replace(findStr, replaceStr, caseSen, wholeWords, forw, !startAtCur, all);
}

void IdeWindow::editSelectAll()
{
  QSEditor *editor = static_cast<QSEditor *>(tabWidget->currentPage());
  editor->selectAll();
}

void IdeWindow::editPreferences()
{
  qsaEditorSyntax->reInit();
  preferencesContainer->show();
}

void IdeWindow::editGotoLine()
{
  QSEditor *editor = static_cast<QSEditor *>(tabWidget->currentPage());
  gotoLine->hide();
  editor->setFocus();
  int val = gotoLine->spinLine->value();
  int max = editor->textEdit()->lines();
  editor->gotoLine(val > max ? max : val);
}

void IdeWindow::helpAbout()
{
  QMessageBox box(this);
  box.setText(QString::fromLatin1("<center><img src=\"splash.png\">"
                                  "<p>Version " QSA_VERSION_STRING  "</p>"
                                  "<p>Copyright (C) 2001-2006 Trolltech ASA. All rights reserved.</p>"
                                  "</center><p></p>"
                                  "<p>QSA Commercial Edition license holders: This program is"
                                  " licensed to you under the terms of the QSA Commercial License"
                                  " Agreement. For details, see the file LICENSE that came with"
                                  " this software distribution.</p><p></p>"
                                  "<p>QSA Free Edition users: This program is licensed to you"
                                  " under the terms of the GNU General Public License Version 2."
                                  " For details, see the file LICENSE.GPL that came with this"
                                  " software distribution.</p><p>The program is provided AS IS"
                                  " with NO WARRANTY OF ANY KIND, INCLUDING THE WARRANTY OF"
                                  " DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE."
                                  "</p>")
             );
  box.setCaption(tr("About QSA Workbench"));
  box.setIcon(QMessageBox::NoIcon);
  box.exec();
}

void IdeWindow::addPage(QSScript *s)
{
  Q_ASSERT(s);
  QSEditor *editor = project->createEditor(s, tabWidget, 0);
  tabWidget->addTab(editor, aqFormatTabLabel(s));
  tabWidget->setTabToolTip(editor, aqFormatTabToolTip(s));
  int idx = tabWidget->indexOf(editor);
  tabWidget->setCurrentPage(idx);
  enableEditActions(true);

  if (projectBrowser)
    projectBrowser->addScript(s);

  editor->iface()->onBreakPointChange(this, SLOT(breakPointsChanged()));

  AQ_SETTINGS_KEY_BD(QObject::name())
  key.append("openScripts");
  QStringList openScripts(config.readListEntry(key));
  if (!openScripts.contains(s->name())) {
    openScripts << s->name();
    config.writeEntry(key, openScripts);
  }

  editor->setLastModificationTime(QDateTime());
  syncEditor(editor);
}

void IdeWindow::removePage(QSScript *s)
{
  Q_ASSERT(s);
  QSEditor *editor = project->editor(s);
  if (!editor) return;

  if (editor->isModified() &&
      QMessageBox::Yes == QMessageBox::question(
        this, tr("Guardar cambios"),
        tr("El fichero ha sido modificado\n¿ Quiere guardar los cambios ?"),
        QMessageBox::No, QMessageBox::Yes | QMessageBox::Default)) {
    saveScript(s);
  }
  AQ_SETTINGS_KEY_BD(QObject::name())
  key.append("openScripts");
  QStringList openScripts(config.readListEntry(key));
  openScripts.remove(s->name());
  config.writeEntry(key, openScripts);

  tabWidget->removePage(editor);
  delete editor;
  enableEditActions(tabWidget->count() > 0);
}

void IdeWindow::removePage()
{
  QSEditor *editor = static_cast<QSEditor *>(tabWidget->currentPage());
  if (!editor)
    return;

  QSScript *s = editor->script();
  if (editor->isModified() &&
      QMessageBox::Yes == QMessageBox::question(
        this, tr("Guardar cambios"),
        tr("El fichero ha sido modificado\n\n¿ Quiere guardar los cambios ?"),
        QMessageBox::No, QMessageBox::Yes | QMessageBox::Default)) {
    saveScript(s);
  }

  AQ_SETTINGS_KEY_BD(QObject::name())
  key.append("openScripts");
  QStringList openScripts(config.readListEntry(key));
  openScripts.remove(s->name());
  config.writeEntry(key, openScripts);

  tabWidget->removePage(editor);
  delete editor;
  enableEditActions(tabWidget->count() > 0);
}

void IdeWindow::showPage(QSScript *s)
{
  Q_ASSERT(s);
  QSEditor *editor = project->editor(s);
  int idx = -1;
  if (editor && (idx = tabWidget->indexOf(editor)) != -1) {
    tabWidget->setCurrentPage(idx);
    return;
  }
  addPage(s);
}

void IdeWindow::showFunction(QSScript *s, const QString &f)
{
  showPage(s);
  QSEditor *editor = static_cast<QSEditor *>(tabWidget->currentPage());
  if (!editor)
    return;
  editor->find("function " + f, false, true, true, false);
}

void IdeWindow::scrollTo(QSScript *s, const QString &txt, const QString &first)
{
  showPage(s);
  QSEditor *editor = static_cast<QSEditor *>(tabWidget->currentPage());
  if (!editor)
    return;
  editor->iface()->scrollTo(txt, first);
}

void IdeWindow::init()
{
#if (QT_VERSION >= 0x030200)
  QTabBar *tabBar = (QTabBar *)tabWidget->child(0, "QTabBar", false);
  int m = (tabBar ? style().pixelMetric(QStyle::PM_TabBarTabVSpace, (QWidget *)tabBar)
           + style().pixelMetric(QStyle::PM_TabBarBaseHeight, (QWidget *)tabBar) : 0);
  int s = tabWidget->height() - m;
  QToolButton *closeTabButton = new QToolButton(tabWidget);
  closeTabButton->setAutoRaise(true);
  closeTabButton->setFixedSize(s, s);
  closeTabButton->setIconSet(QIconSet(style().stylePixmap(QStyle::SP_TitleBarCloseButton)));
  connect(closeTabButton, SIGNAL(clicked()), this, SLOT(removePage()));
  QToolTip::add(closeTabButton, tr("Close tab"));
  tabWidget->setCornerWidget(closeTabButton, Qt::TopRight);
#endif

  tabWidget->removePage(tabWidget->page(0));

  connect(tabWidget, SIGNAL(currentChanged(QWidget *)),
          this, SLOT(currentTabChanged(QWidget *)));

  projectBrowserDock = 0;
  projectBrowser = 0;

  outputContainerDock = new QDockWindow(QDockWindow::InDock, this);
  outputContainer = new QSOutputContainer(outputContainerDock , 0, false);
  outputContainerDock->setResizeEnabled(true);
  outputContainerDock->setCloseMode(QDockWindow::Always);
  addDockWindow(outputContainerDock, DockBottom);
  outputContainerDock->setWidget(outputContainer);
  outputContainerDock->setCaption(tr("Output"));
  outputContainerDock->setFixedExtentHeight(100);
  outputContainer->show();


  findText = new QSFindText(this, 0, false);
  connect(editFindAction, SIGNAL(activated()), findText, SLOT(show()));
  connect(findText->pushFind, SIGNAL(clicked()), this, SLOT(editFind()));

  replaceText = new QSReplaceText(this, 0, false);
  connect(editReplaceAction, SIGNAL(activated()), replaceText, SLOT(show()));
  connect(replaceText->pushReplace, SIGNAL(clicked()), this, SLOT(editReplace()));
  connect(replaceText->pushReplaceAll, SIGNAL(clicked()), this, SLOT(editReplaceAll()));
  gotoLine = new QSGotoLine(this, 0, false);
  connect(editGotoLineAction, SIGNAL(activated()), gotoLine, SLOT(show()));
  connect(gotoLine->pushGoto, SIGNAL(clicked()), this, SLOT(editGotoLine()));

  preferencesContainer = new QSPreferencesContainer(this, 0);
  QBoxLayout *preferencesLayout = new QBoxLayout(preferencesContainer->frame, QBoxLayout::Down);
  qsaEditorSyntax = new PreferencesBase(preferencesContainer->frame, "qsaeditor_syntax");
  preferencesLayout->addWidget(qsaEditorSyntax);
  qsaEditorSyntax->setPath(QString::fromLatin1("/Trolltech/QSAScriptEditor/"));
  connect(preferencesContainer->pushOk, SIGNAL(clicked()), this, SLOT(savePreferences()));

  windowMenu->insertItem(tr("&Views"), createDockWindowMenu(NoToolBars));
  windowMenu->insertItem(tr("&Toolbars"), createDockWindowMenu(OnlyToolBars));

  setIcon(QPixmap::fromMimeSource(QString::fromLatin1("qsa.png")));
  setupActionIcons();

  enableEditActions(false);
}

void IdeWindow::setRunningState(bool running)
{
  projectStopAction->setEnabled(running);
  projectRunAction->setEnabled(!running);
  projectCallAction->setEnabled(!running);
}

void IdeWindow::projectRun()
{
  evaluateProject();

  QSScript *script = 0;
  QSEditor *activeEditor = static_cast<QSEditor *>(tabWidget->currentPage());
  if (activeEditor)
    script = activeEditor->script();
  else if (projectBrowser)
    script = projectBrowser->currentScript();

  QStringList functions;
  if (script && script->context()) {
    functions += project->interpreter()->functions(script->context());
    QString objName(QString::fromLatin1(script->context()->name()) + '.');
    for (QStringList::iterator it = functions.begin(); it != functions.end(); ++it)
      (*it).prepend(objName);
  }
  functions += project->interpreter()->functions();

  if (ip->hadError())
    return;

  bool ok = true;
  if (runFunction.isNull()
      || runFunction == QString::fromLatin1("")
      || functions.find(runFunction) == functions.end()) {
    runFunction = QInputDialog::getItem(QString::fromLatin1("Call function"),
                                        QString::fromLatin1("&Function: "), functions,
                                        functions.findIndex(runFunction),
                                        false, &ok, this);
  }
  if (ok && !runFunction.isEmpty()) {
    setRunningState(true);
    project->interpreter()->call(runFunction);
    setRunningState(false);
  }
}

void IdeWindow::projectCall()
{
  evaluateProject();

  QSScript *script = 0;
  QSEditor *activeEditor = static_cast<QSEditor *>(tabWidget->currentPage());
  if (activeEditor)
    script = activeEditor->script();
  else if (projectBrowser)
    script = projectBrowser->currentScript();

  QStringList functions;
  if (script && script->context()) {
    functions += project->interpreter()->functions(script->context());
    QString objName(QString::fromLatin1(script->context()->name()) + '.');
    for (QStringList::iterator it = functions.begin(); it != functions.end(); ++it)
      (*it).prepend(objName);
  }
  functions += project->interpreter()->functions();

  if (ip->hadError())
    return;

  bool ok = false;
  runFunction = QInputDialog::getItem(QString::fromLatin1("Call function"),
                                      QString::fromLatin1("&Function: "), functions,
                                      functions.findIndex(runFunction),
                                      false, &ok, this);
  if (ok && !runFunction.isEmpty()) {
    setRunningState(true);
    project->interpreter()->call(runFunction);
    setRunningState(false);
  }
}

void IdeWindow::currentTabChanged(QWidget *w)
{
  QSEditor *editor = static_cast<QSEditor *>(w);
  if (!editor)
    return;
  projectBrowser->setCurrentScript(editor->script());
  syncEditor(editor);
  textChanged();
}

void IdeWindow::setupActionIcons()
{
  fileSaveAction->setIconSet(createIconSet(QString::fromLatin1("filesave.png")));
  filePrintAction->setIconSet(createIconSet(QString::fromLatin1("designer_print.png")));
  fileCloseAction->setIconSet(createIconSet(QString::fromLatin1("editdelete.png")));
  fileExitAction->setIconSet(createIconSet(QString::fromLatin1("exit.png")));
  editUndoAction->setIconSet(createIconSet(QString::fromLatin1("undo.png")));
  editRedoAction->setIconSet(createIconSet(QString::fromLatin1("redo.png")));
  editCutAction->setIconSet(createIconSet(QString::fromLatin1("editcut.png")));
  editCopyAction->setIconSet(createIconSet(QString::fromLatin1("editcopy.png")));
  editPasteAction->setIconSet(createIconSet(QString::fromLatin1("editpaste.png")));
  editFindAction->setIconSet(createIconSet(QString::fromLatin1("searchfind.png")));
  helpAboutAction->setIconSet(createIconSet(QString::fromLatin1("qsa.png"), false));
  projectRunAction->setIconSet(createIconSet(QString::fromLatin1("playprev.png")));
  projectCallAction->setIconSet(createIconSet(QString::fromLatin1("play.png")));
  projectStopAction->setIconSet(createIconSet(QString::fromLatin1("stop.png")));
  projectEvaluateAction->setIconSet(createIconSet(QString::fromLatin1("sync.png")));
  editFormatCodeAction->setIconSet(createIconSet(QString::fromLatin1("designer_textjustify.png")));
}

void IdeWindow::enableEditActions(bool enable)
{
  fileSaveAction->setEnabled(enable);
  filePrintAction->setEnabled(enable);
  fileCloseAction->setEnabled(enable);
  editCutAction->setEnabled(enable);
  editCopyAction->setEnabled(enable);
  editPasteAction->setEnabled(enable);
  editFindAction->setEnabled(enable);
  editFindAgainAction->setEnabled(enable);
  editFindAgainBWAction->setEnabled(enable);
  editReplaceAction->setEnabled(enable);
  editGotoLineAction->setEnabled(enable);
  editSelectAllAction->setEnabled(enable);
  editFormatCodeAction->setEnabled(enable);
#if (QT_VERSION >= 0x030200)
  tabWidget->cornerWidget(Qt::TopRight)->setShown(enable);
#endif
}

void IdeWindow::enableProjectActions(bool enable)
{
  projectRunAction->setEnabled(enable);
  projectCallAction->setEnabled(enable);
}

void IdeWindow::textChanged()
{
  QSEditor *editor = static_cast<QSEditor *>(tabWidget->currentPage());
  if (!editor)
    return;

  editUndoAction->setEnabled(editor->isUndoAvailable());
  editRedoAction->setEnabled(editor->isRedoAvailable());

  if (editor->isModified()) {
    tabWidget->setTabLabel(editor, aqFormatTabLabel(editor->script()) + "*");
    projectEvaluateAction->setEnabled(true);
  } else {
    tabWidget->setTabLabel(editor, aqFormatTabLabel(editor->script()));
    projectEvaluateAction->setEnabled(false);
  }
}

void IdeWindow::setProject(QSProject *p)
{
  Q_ASSERT(p);

  project = p;
  ip = get_quick_interpreter(project->interpreter());
  dbg = ip->debuggerEngine();

  projectBrowserDock = new QDockWindow(QDockWindow::InDock, this);
  projectBrowser = new AQProjectBrowser(project, projectBrowserDock);
  projectBrowserDock->setResizeEnabled(true);
  projectBrowserDock->setCloseMode(QDockWindow::Always);
  addDockWindow(projectBrowserDock, DockLeft);
  projectBrowserDock->setWidget(projectBrowser);
  projectBrowserDock->setCaption(tr("Scripts"));
  projectBrowserDock->setFixedExtentWidth(250);
  projectBrowser->show();

  connect(projectBrowser, SIGNAL(scriptDoubleClicked(QSScript *)),
          this, SLOT(showPage(QSScript *)));
  connect(projectBrowser,
          SIGNAL(scriptDoubleClicked(QSScript *, const QString &, const QString &)),
          this, SLOT(scrollTo(QSScript *, const QString &, const QString &)));

  connect(dbg, SIGNAL(modeChanged(int)), this, SLOT(changedDebugMode(int)));
  connect(project, SIGNAL(editorTextChanged()), this, SLOT(textChanged()));

  projectChanged();

  AQ_SETTINGS_KEY_BD(QObject::name())
  key.append("openScripts");
  QStringList openScripts(config.readListEntry(key));

  QPtrList<QSScript> scripts = project->scripts();
  for (QSScript *script = scripts.first(); script; script = scripts.next()) {
    if (!script->fileName().isEmpty() && openScripts.contains(script->name()))
      addPage(script);
  }

  errorMode = project->interpreter()->errorMode();

  connect(ip, SIGNAL(parseError()), this, SLOT(parseError()));
  connect(ip, SIGNAL(runtimeError()), this, SLOT(runtimeError()));
  connect(project, SIGNAL(projectChanged()), this, SLOT(projectChanged()));
  connect(project, SIGNAL(projectEvaluated()), this, SLOT(projectEvaluated()));
}

void IdeWindow::savePreferences()
{
  qsaEditorSyntax->save();
  QPtrList<QSEditor> editors = project->editors();
  QSEditor *editor = editors.first();
  while (editor) {
    editor->readSettings();
    editor = editors.next();
  }
}

void IdeWindow::projectChanged()
{
  enableProjectActions(!project->scripts().isEmpty());
  dbg->setMode(Debugger::Disabled);
  projectEvaluateAction->setEnabled(true);
}

void IdeWindow::projectEvaluated()
{
  dbg->setMode(Debugger::Continue);
  projectEvaluateAction->setEnabled(false);
}

void IdeWindow::evaluateProject()
{
  saveAllScripts();
  project->commitEditorContents();
  QTimer::singleShot(0, project, SLOT(evaluate()));
}

void IdeWindow::saveAllScripts()
{
  QPtrList<QSScript> scripts = project->scripts();
  for (QSScript *script = scripts.first(); script; script = scripts.next()) {
    if (script->fileName().isEmpty())
      continue;
    saveScript(script);
  }
}

void IdeWindow::hideEvent(QHideEvent *)
{
  project->interpreter()->setErrorMode((QSInterpreter::ErrorMode)errorMode);
  if (debugoutput)
    qInstallMsgHandler(0);
  debugoutput = 0;
}

void IdeWindow::showEvent(QShowEvent *)
{
  project->interpreter()->setErrorMode(QSInterpreter::Nothing);
  if (debugoutput)
    qInstallMsgHandler(0);
  debugoutput = outputContainer->textEdit;
  qt_default_message_handler = qInstallMsgHandler(debugMessageOutput);
}

void IdeWindow::projectStop()
{
  project->interpreter()->stopExecution();
}

void IdeWindow::editFormatCode()
{
  QSEditor *ed = static_cast<QSEditor *>(tabWidget->currentPage());
  if (!ed)
    return;

  QString asOps;
  QChar sep('\n');
  asOps = "mode=java";
  asOps += sep;
  asOps += "style=k&r";
  asOps += sep;
  asOps += "indent=spaces=2";
  asOps += sep;
  asOps += "brackets=linux";
  asOps += sep;
  asOps += "convert-tabs";
  asOps += sep;
  asOps += "indent-namespaces";
  asOps += sep;
  asOps += "indent-switches";
  asOps += sep;
  asOps += "indent-preprocessor";
  asOps += sep;
  asOps += "unpad-paren";
  asOps += sep;
  asOps += "pad-oper";
  asOps += sep;
  asOps += "pad-header";
  asOps += sep;
  asOps += "align-pointer=name";
  asOps += sep;
  asOps += "max-instatement-indent=70";
  asOps += sep;
  asOps += "min-conditional-indent=0";
  asOps += sep;
  asOps += "indent-col1-comments";
  asOps += sep;

  char *asOut = AStyleMain(ed->text(), asOps,
                           aqAstyleErrorHandler,
                           aqAstyleMemoryAlloc);
  if (!asOut)
    return;

  QString txt(asOut);
  if (txt != ed->text()) {
    QTextEdit *ted = ed->textEdit();
    int par, idx;
    ted->getCursorPosition(&par, &idx);
    ed->setText(asOut);
    ted->setCursorPosition(par, idx);
  }

  delete [] asOut;
}

void IdeWindow::showErrorMessage(QObject *o, int line,
                                 const QString &errorMessage)
{
  QSScript *script = project->script(o);
  if (script)
    runtimeError(script, errorMessage, line);
}

void IdeWindow::showStackFrame(QObject *o, int line)
{
  QSScript *script = project->script(o);
  if (script) {
    showPage(script);
    QSEditor *editor = project->editor(script);
    QSAEditorInterface *eIface = editor->iface();
    eIface->clearStackFrame();
    eIface->setStackFrame(line - 1);
  }
}

void IdeWindow::showDebugStep(QObject *o, int line)
{
  QSScript *script = project->script(o);
  if (script) {
    showPage(script);
    QSEditor *editor = project->editor(script);
    QSAEditorInterface *eIface = editor->iface();
    eIface->clearStackFrame();
    eIface->setStep(line - 1);
  }
}

void IdeWindow::breakPointsChanged()
{
  QSEditor *editor = static_cast<QSEditor *>(tabWidget->currentPage());
  Q_ASSERT(editor);
  if (!editor)
    return;

  editor->iface()->breakPoints(bpsMap[editor->script()->name()]);
  setBreakPoints();
}

void IdeWindow::setDebugger(QSADebuggerInterface *debugger)
{
  if (debugger) {
    dbgIface = debugger;
    QStringList l(dbgIface->featureList());
    for (QStringList::const_iterator it = l.begin(); it != l.end(); ++it) {
      QAction *a = dbgIface->create(*it, dbgToolbar);
      if (a)
        a->addTo(dbgToolbar);
    }
  }
}

void IdeWindow::parseError()
{
  int sourceId = dbg->sourceId();
  QString msg = ip->errorMessages().first();
  int line = ip->errorLines().first();
  QSScript *script = project->script(ip->objectOfSourceId(sourceId));

  if (script)
    parseError(script, msg, line);
}

void IdeWindow::runtimeError()
{
  int sourceId = dbg->sourceId();
  QString msg = ip->errorMessages().first();
  int line = dbg->lineNumber();
  QSScript *script = project->script(ip->objectOfSourceId(sourceId));

  if (script)
    runtimeError(script, msg, line);
}

void IdeWindow::parseError(QSScript *s, const QString &msg, int line)
{
  QStringList error;
  error << QString::fromLatin1("<pre><font color=red><b>Parse Error:</b></font> ") << s->name()
        << QString::fromLatin1(" : <font color=blue>") << QString::number(line)
        << QString::fromLatin1("</font>\n") << QString::fromLatin1("<i>") << msg
        << QString::fromLatin1("</i>\n");
  error << QString::fromLatin1("</pre>");
  qDebug(error.join(QString::fromLatin1("")));

  showPage(s);
  QSEditor *editor = project->editor(s);
  QSAEditorInterface *eIface = editor->iface();
  eIface->clearError();
  eIface->clearStep();
  eIface->clearStackFrame();
  eIface->setError(line - 1);
}

void IdeWindow::runtimeError(QSScript *s, const QString &msg, int line)
{
  QStringList error;
  error << QString::fromLatin1("<pre><font color=red><b>Runtime Error:</b></font> ") << s->name()
        << QString::fromLatin1(" : <font color=blue>") << QString::number(line)
        << QString::fromLatin1("</font>\n") << QString::fromLatin1("<i>") << msg
        << QString::fromLatin1("</i>\n");
  QSStackTrace stackTrace = project->interpreter()->stackTrace();
  if (stackTrace.size()) {
    QString trace = stackTrace.toString();
    error << QString::fromLatin1("Callstack:\n");
    error << QString::fromLatin1("  ") << trace.replace(QString::fromLatin1("\n"),
                                                        QString::fromLatin1("\n  "));
  }
  error << QString::fromLatin1("</pre>");
  qDebug(error.join(QString::fromLatin1("")));

  showPage(s);
  QSEditor *editor = project->editor(s);
  QSAEditorInterface *eIface = editor->iface();
  eIface->clearError();
  eIface->clearStep();
  eIface->clearStackFrame();
  eIface->setError(line - 1);
}

void IdeWindow::setBreakPoints()
{
  QMap<QString, QValueList<uint> >::const_iterator it;
  for (it = bpsMap.begin(); it != bpsMap.end(); ++it) {
    QSScript *s = project->script(it.key());
    if (!s)
      continue;

    QValueList<uint> l(*it);
    QSEditor *e = project->editor(s);
    if (e)
      e->iface()->setBreakPoints(l);

    int sid = ip->sourceIdOfName(it.key());
    if (sid != -1) {
      dbg->clearAllBreakpoints(sid);
      for (QValueList<uint>::const_iterator it2 = l.begin(); it2 != l.end(); ++it2)
        dbg->setBreakpoint(sid, *it2);
    }
  }
}

void IdeWindow::changedDebugMode(int m)
{
  switch (m) {
    case Debugger::Disabled:
    case Debugger::Continue: {
      QPtrList<QSEditor> editors = project->editors();
      for (QSEditor *ed = editors.first(); ed; ed = editors.next()) {
        QSAEditorInterface *eIface = ed->iface();
        eIface->clearStackFrame();
        eIface->clearStep();
        eIface->clearError();
        eIface->setMode(EditorInterface::Editing);
      }
      setRunningState(false);
      setBreakPoints();
      break;
    }
    case Debugger::Next:
    case Debugger::Step: {
      QPtrList<QSEditor> editors = project->editors();
      for (QSEditor *ed = editors.first(); ed; ed = editors.next()) {
        QSAEditorInterface *eIface = ed->iface();
        eIface->setMode(EditorInterface::Editing);
      }
      setRunningState(false);
      break;
    }
    case Debugger::Stop: {
      setRunningState(true);
      if (ip->hadError())
        return;
      QPtrList<QSEditor> editors = project->editors();
      for (QSEditor *ed = editors.first(); ed; ed = editors.next()) {
        QSAEditorInterface *eIface = ed->iface();
        eIface->setMode(EditorInterface::Debugging);
      }
      showDebugStep(ip->objectOfSourceId(dbg->sourceId()),
                    dbg->lineNumber());
      break;
    }
  }
}

void IdeWindow::setWFlags(WFlags f)
{
  QWidget::setWFlags(f);
}

void IdeWindow::clearWFlags(WFlags f)
{
  QWidget::clearWFlags(f);
}

static inline QString scriptBaseName(QSScript *s)
{
  return s->baseFileName().section("::", 0, 0);
}

static inline QString scriptTextFileName(const QString &fileName)
{
  QString text;
  QFile fi(fileName);
  if (fi.open(IO_ReadOnly)) {
    QTextStream t(&fi);
    t.setEncoding(QTextStream::Latin1);
    text = t.read();
    fi.close();
    if (!text.left(45).lower().contains("var form"))
      text.prepend("var form = this; //auto-added\n");
  } else {
    QMessageBox::critical(
      0, qApp->tr("Error abriendo fichero"),
      qApp->tr("No se pudo abrir el fichero %1 para lectura:\n %2")
      .arg(fileName, fi.errorString()));
  }
  return text;
}

void IdeWindow::syncEditor(QSEditor *e)
{
  QString eBaseName(scriptBaseName(e->script()));
  QString eFileName(e->script()->fileName());
  QDateTime eLastModTime(e->lastModificationTime());
  QString eText(e->text());

  QPtrList<QSEditor> editors = project->editors();
  for (QSEditor *ed = editors.first(); ed; ed = editors.next()) {
    if (ed == e)
      continue;
    if (scriptBaseName(ed->script()) == eBaseName &&
        ed->script()->fileName() == eFileName &&
        ed->lastModificationTime() > eLastModTime &&
        ed->text() != eText) {
      e->setText(ed->text());
      e->setLastModificationTime(ed->lastModificationTime());
      eText = ed->text();
    }
  }

  if (eFileName.isEmpty())
    return;

  QFileInfo fi(eFileName);
  if (fi.lastModified() > eLastModTime) {
    QString text(scriptTextFileName(eFileName));
    if (!text.isEmpty() && text != eText) {
      QString msg(tr("El archivo\n%1\nasociado al editor ha cambiado en el disco.\n\n"
                     "�Quiere volver a cargar el archivo?"));
      int res = QMessageBox::question(
                  this, tr("Detectados cambios en disco"), msg.arg(eFileName),
                  QMessageBox::No, QMessageBox::Yes | QMessageBox::Default);
      if (res == QMessageBox::Yes)
        e->setText(text);
    }
    e->setLastModificationTime(fi.lastModified());
  }
}

void IdeWindow::enterEvent(QEvent *e)
{
  QSEditor *editor = static_cast<QSEditor *>(tabWidget->currentPage());
  if (editor)
    syncEditor(editor);
  QMainWindow::enterEvent(e);
}
