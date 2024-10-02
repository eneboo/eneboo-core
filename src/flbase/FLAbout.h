/***************************************************************************
                              FLAbout.h
                          -------------------
 begin                : Sat Jan 26 2002
 copyright            : (C) 2002-2005 by InfoSiAL S.L.
 email                : mail@infosial.com
***************************************************************************/
/***************************************************************************
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; version 2 of the License.               *
 ***************************************************************************/
/***************************************************************************
   Este  programa es software libre. Puede redistribuirlo y/o modificarlo
   bajo  los  t�rminos  de  la  Licencia  P�blica General de GNU   en  su
   versi�n 2, publicada  por  la  Free  Software Foundation.
 ***************************************************************************/

#ifndef FLABOUT_H
#define FLABOUT_H

#include "FLWidgetAbout.h"

/**
Implementaci�n del cuadro de di�logo Acerca de..

@author InfoSiAL S.L.
*/
class FLAbout: public FLWidgetAbout
{
  Q_OBJECT

public:

  /**
  constructor.

  @param v Versi�n de la aplicaci�n.
  */
  FLAbout(const QString &v, QWidget *parent = 0, const char *name = 0);

  /**
  destructor.
  */
  ~FLAbout();


public slots:

  /**
  Copia la informaci�n de compilaci�n en el portapapeles
  */
  void copy2Clipboard();
  void mostrarKitDigital();
};
#endif
