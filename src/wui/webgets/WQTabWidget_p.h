/***************************************************************************
WQTabWidget_p.h
-------------------
begin                : 06/09/2008
copyright            : (C) 2003-2008 by InfoSiAL S.L.
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

#ifndef WQTABWIDGET_P_H_
#define WQTABWIDGET_P_H_

#include "AQUi/WQWidget_p.h"

class WQTabWidgetPrivate : public WQWidgetPrivate
{
  AQ_DECLARE_PUBLIC( WQTabWidget )

public :

  WQTabWidgetPrivate();
  ~WQTabWidgetPrivate();

  void init();

  WTabWidget * ww_;
  QTabWidget * qTabWid_;
  WTabWidget * tbw_;
};

#endif /*WQTABWIDGET_P_H_*/