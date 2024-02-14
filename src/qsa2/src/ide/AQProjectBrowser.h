/***************************************************************************
 AQProjectBrowser.h
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

#ifndef AQPROJECTBROWSER_H_
#define AQPROJECTBROWSER_H_

#include "projectcontainer.h"

class QSProject;
class QSScript;
class AQScriptListItem;

class AQProjectBrowser : public QSProjectContainer
{
  Q_OBJECT

public:
  AQProjectBrowser(QSProject *project, QWidget *parent = 0);

  QSScript *currentScript() const;
  void setCurrentScript(QSScript *script);
  void addScript(QSScript *script);

signals:
  void scriptDoubleClicked(QSScript *);
  void scriptDoubleClicked(QSScript *, const QString &, const QString &);

private slots:
  void itemDoubleClicked(QListViewItem *i);
  void updateItems();
  void setCurrentItem(const QString &expr);

private:
  void setCurrentItem(QSScript *script);
  AQScriptListItem *findItem(QSScript *script) const;

  QSProject *project_;
};

#endif /* AQPROJECTBROWSER_H_ */
