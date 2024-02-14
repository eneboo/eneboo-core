/***************************************************************************
 AQProjectBrowser.cpp
 -------------------
 begin                : 08/04/2011
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
 bajo  los  términos  de  la  Licencia  Pública General de GNU   en  su
 versión 2, publicada  por  la  Free  Software Foundation.
 ***************************************************************************/

#include <qapplication.h>
#include <qpixmap.h>
#include <qheader.h>
#include <qpainter.h>
#include <qlistview.h>
#include <qlineedit.h>
#include <qcombobox.h>
#include <qsproject.h>

#include "../kernel/quickclassparser.h"
#include "AQProjectBrowser.h"

static QColor *backColor1 = 0;
static QColor *backColor2 = 0;
static inline void aqInitColors()
{
  if (backColor1)
    return;
  QColorGroup myCg = qApp->palette().active();
  int h1, s1, v1;
  int h2, s2, v2;
  myCg.color(QColorGroup::Base).hsv(&h1, &s1, &v1);
  myCg.color(QColorGroup::Background).hsv(&h2, &s2, &v2);
  QColor c(h1, s1, (v1 + v2) / 2, QColor::Hsv);
  backColor1 = new QColor(c);
  backColor2 = new QColor(255, 255, 255);
}

class AQClassBrowserItem : public QListViewItem
{
public:
  enum Type { Class = 2, Form, Global, Function, Variable, Script };

  AQClassBrowserItem(QListView *parent, QSScript *scr,
                     QListViewItem *after = 0, Type t = Script)
    : QListViewItem(parent, after), scr_(scr), type_(t) {}

  AQClassBrowserItem(QListViewItem *parent, QSScript *scr,
                     QListViewItem *after = 0, Type t = Script)
    : QListViewItem(parent, after), scr_(scr), type_(t) {}

  QSScript *scr() const {
    return scr_;
  }

  virtual int rtti() const {
    return type_;
  };

protected:
  void paintCell(QPainter *p, const QColorGroup &cg,
                 int column, int width, int align) {
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

  void paintBranches(QPainter *p, const QColorGroup &cg,
                     int w, int y, int h) {
    QColorGroup g(cg);
    g.setColor(QColorGroup::Base, backgroundColor());
    QListViewItem::paintBranches(p, g, w, y, h);
  }

  void updateBackColor() {
    if (listView()->firstChild() == this) {
      backColor_ = *backColor1;
      return;
    }
    QListViewItemIterator it(this);
    do {
      --it;
    } while (it.current() && it.current() != parent()
             && it.current()->depth() != depth());
    if (it.current()) {
      if (static_cast<AQClassBrowserItem *>(it.current())->backColor_ == *backColor1)
        backColor_ = *backColor2;
      else
        backColor_ = *backColor1;
    } else
      backColor_ == *backColor1;
  }

  QColor backgroundColor() {
    updateBackColor();
    return backColor_;
  }

  QSScript *scr_;
  Type type_;
  QColor backColor_;
};

class AQScriptListItem : public AQClassBrowserItem
{
public:
  AQScriptListItem(QListView *parent, QSScript *scr)
    : AQClassBrowserItem(parent, scr) {
    if (scr->baseFileName().isEmpty()) {
      setText(0, scr->name());
      setPixmap(0, QPixmap::fromMimeSource(QString::fromLatin1("scriptobject.png")));
    } else {
      setText(0, scr->baseFileName());
      setPixmap(0, QPixmap::fromMimeSource(QString::fromLatin1("script.png")));
    }
  }

  void clear() {
    QListViewItem *item;
    while ((item = firstChild()))
      delete item;
  }

  void updateItem() {
    clear();

    QuickClassParser parser;
    parser.parse(scr_->code());
    QValueList<QuickClass> classes = parser.classes();
    QListViewItem *lastClass = 0;
    AQClassBrowserItem *currentClass = 0;

    for (QValueList<QuickClass>::ConstIterator it = classes.begin(); it != classes.end(); ++it) {
      if ((*it).type == QuickClass::Global) {
        currentClass = new AQClassBrowserItem(this, scr_, 0, AQClassBrowserItem::Global);
        currentClass->setText(0, QApplication::tr("Global"));
        currentClass->setOpen(false);
      } else {
        AQClassBrowserItem::Type t;
        switch ((*it).type) {
          case QuickClass::Global:
            t = AQClassBrowserItem::Global;
            break;
          case QuickClass::Class:
            t = AQClassBrowserItem::Class;
            break;
          default:
            qWarning("AQScriptListItem::updateItem: unhandled switch case");
            t = AQClassBrowserItem::Variable;
            break;
        }
        currentClass = new AQClassBrowserItem(this, scr_, lastClass, t);
        if ((*it).type != QuickClass::Class || (*it).access == "public")
          currentClass->setText(0, (*it).name);
        else
          currentClass->setText(0, (*it).name + " [" + (*it).access + "]");
        currentClass->setOpen(false);
        lastClass = currentClass;
      }
      currentClass->setPixmap(0, QPixmap::fromMimeSource("class.png"));

      QListViewItem *last = 0;
      QStringList vars = (*it).variables;
      for (QStringList::Iterator vit = vars.begin(); vit != vars.end(); ++vit) {
        last = new AQClassBrowserItem(currentClass, scr_, last, AQClassBrowserItem::Variable);
        QString var = *vit;
        if (var.startsWith("var"))
          var = var.mid(3).stripWhiteSpace();
        else if (var.startsWith("const"))
          var = var.mid(5).stripWhiteSpace() + " [const]";
        else if (var.startsWith("static"))
          var = var.mid(10).stripWhiteSpace() + " [static]";
        last->setText(0, var);
        last->setPixmap(0, QPixmap::fromMimeSource("variable.png"));
      }

      QValueList<LanguageInterface::Function> funcs = (*it).functions;
      for (QValueList<LanguageInterface::Function>::Iterator fit = funcs.begin();
           fit != funcs.end(); ++fit) {
        last = new AQClassBrowserItem(currentClass, scr_, last, AQClassBrowserItem::Function);
        if ((*it).type != QuickClass::Global && (*fit).access != "public")
          last->setText(0, (*fit).name + " [" + (*fit).access + "]");
        else
          last->setText(0, (*fit).name);
        last->setPixmap(0, QPixmap::fromMimeSource("function.png"));
      }
    }
  }
};

AQProjectBrowser::AQProjectBrowser(QSProject *project, QWidget *parent)
  : QSProjectContainer(parent, "aq_project_browser"),
    project_(project)
{
  aqInitColors();

  scriptsListView_->setResizeMode(QListView::AllColumns);
  scriptsListView_->setRootIsDecorated(true);
  scriptsListView_->setSorting(0);
  scriptsListView_->setHScrollBarMode(QScrollView::AlwaysOff);
  scriptsListView_->setVScrollBarMode(QScrollView::AlwaysOn);
  scriptsListView_->header()->hide();

  connect(scriptsListView_, SIGNAL(doubleClicked(QListViewItem *)),
          this, SLOT(itemDoubleClicked(QListViewItem *)));

  updateItems();
  connect(project_, SIGNAL(projectChanged()), this, SLOT(updateItems()));
  connect(leFind_, SIGNAL(textChanged(const QString &)),
          this, SLOT(setCurrentItem(const QString &)));
}

QSScript *AQProjectBrowser::currentScript() const
{
  QListViewItem *i = scriptsListView_->currentItem();
  return (i ? static_cast<AQScriptListItem *>(i)->scr() : 0);
}

void AQProjectBrowser::setCurrentScript(QSScript *script)
{
  setCurrentItem(script);
}

void AQProjectBrowser::addScript(QSScript *script)
{
  if (findItem(script))
    return;
  AQScriptListItem *item = new AQScriptListItem(scriptsListView_, script);
  item->updateItem();
}

void AQProjectBrowser::setCurrentItem(QSScript *script)
{
  QListViewItem *i = scriptsListView_->currentItem();
  if (i && static_cast<AQClassBrowserItem *>(i)->scr() == script)
    return;
  AQScriptListItem *item = findItem(script);
  if (!item)
    return;
  scriptsListView_->setCurrentItem(item);
  scriptsListView_->ensureItemVisible(item);
}

AQScriptListItem *AQProjectBrowser::findItem(QSScript *script) const
{
  if (script->baseFileName().isEmpty())
    return static_cast<AQScriptListItem *>(
             scriptsListView_->findItem(script->name(), 0)
           );
  else
    return static_cast<AQScriptListItem *>(
             scriptsListView_->findItem(script->baseFileName(), 0)
           );
}

static inline QString classSignature(QListViewItem *i)
{
  QString txt(i->text(0));
  QString cls(txt.left(txt.find('[')));
  if (cls == QString::fromLatin1("Global"))
    return QString::null;
  return QString::fromLatin1("class ") + cls;
}

static inline QString variableSignature(QListViewItem *i)
{
  QString txt(i->text(0));
  QString vr(txt.left(txt.find(';')));
  if (txt.endsWith(QString::fromLatin1("[const]"))) {
    vr.prepend(QString::fromLatin1("const "));
    return vr;
  }
  vr.prepend(QString::fromLatin1("var "));
  if (txt.endsWith(QString::fromLatin1("[static]")))
    vr.prepend(QString::fromLatin1("static "));
  return vr;
}

static inline QString functionSignature(QListViewItem *i)
{
  QString txt(i->text(0));
  QString fn(txt.left(txt.find('(')));
  return QString::fromLatin1("function ") + fn;
}

void AQProjectBrowser::itemDoubleClicked(QListViewItem *i)
{
  switch (i->rtti()) {
    case AQClassBrowserItem::Script:
      emit scriptDoubleClicked(static_cast<AQScriptListItem *>(i)->scr());
      break;

    case AQClassBrowserItem::Class:
      emit scriptDoubleClicked(static_cast<AQScriptListItem *>(i)->scr(),
                               classSignature(i), QString::null);
      break;

    case AQClassBrowserItem::Variable:
      emit scriptDoubleClicked(static_cast<AQScriptListItem *>(i)->scr(),
                               variableSignature(i),
                               (i->parent() == i->listView()->firstChild()
                                ? QString::null
                                : classSignature(i->parent())));
      break;

    case AQClassBrowserItem::Function:
      emit scriptDoubleClicked(static_cast<AQScriptListItem *>(i)->scr(),
                               functionSignature(i),
                               (i->parent() == i->listView()->firstChild()
                                ? QString::null
                                : classSignature(i->parent())));
      break;

    default:
      break;
  }
}

void AQProjectBrowser::updateItems()
{
  Q_ASSERT(project_);
  QPtrList<QSScript> scripts = project_->scripts();
  scriptsListView_->clear();
  for (QSScript *script = scripts.first(); script; script = scripts.next()) {
    if (!script->fileName().isEmpty()) {
      AQScriptListItem *item = new AQScriptListItem(scriptsListView_, script);
      item->updateItem();
    }
  }
}

void AQProjectBrowser::setCurrentItem(const QString &expr)
{
  AQScriptListItem *i = static_cast<AQScriptListItem *>(
                          scriptsListView_->findItem(expr, 0,
                                                     Qt::ExactMatch |
                                                     Qt::BeginsWith |
                                                     Qt::EndsWith |
                                                     Qt::Contains)
                        );
  if (!i)
    return;

  QString type(cbFind_->currentText());
  if (type == "Scripts" && i->rtti() != AQClassBrowserItem::Script)
    return;
  if (type == "Clases" && i->rtti() != AQClassBrowserItem::Class)
    return;
  if (type == "Funciones" && i->rtti() != AQClassBrowserItem::Function)
    return;
  if (type == "Variables" && i->rtti() != AQClassBrowserItem::Variable)
    return;

  scriptsListView_->setCurrentItem(i);
  scriptsListView_->ensureItemVisible(i);
}
