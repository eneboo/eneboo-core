/****************************************************************************
** $Id: qsmember.cpp  1.1.5   edited 2006-02-23T15:39:57$
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

#include "qsmember.h"

// ### AbanQ
#include "qsnodes.h"

QSMember::~QSMember()
{
  setSuper(0);
}

void QSMember::setSuper(const QSMember *super)
{
  if (super_ == super)
    return;

  if (super_) {
    Q_ASSERT(super_->type() == QSMember::ScriptFunction);
    Q_ASSERT(super_->scriptFunction);

    // ### Leak risk ?
    if (super_->scriptFunction->deref()) {
      delete super_->scriptFunction;
      delete super_;
    }
  }

  super_ = super;

  if (super_) {
    Q_ASSERT(super_->type() == QSMember::ScriptFunction);
    Q_ASSERT(super_->scriptFunction);

    super_->scriptFunction->ref();
  }
}

// ### AbanQ

/*!
  Returns a human readable description of this member's type.
 */
QString QSMember::typeName() const
{
  switch (typ) {
    case Variable:
      return QString::fromLatin1("member variable");
    case NativeFunction:
      return QString::fromLatin1("C++ function");
    case NativeMemberFunction:
      return QString::fromLatin1("C++ member function");
    case ScriptFunction:
      return QString::fromLatin1("Function");
    case Object:
      return QString::fromLatin1("Object");
    case Identifier:
      return QString::fromLatin1("Identifier");
    case Custom:
      return QString::fromLatin1("Custom");
    default:
      return QString::fromLatin1("<undefined>");
  }
}
