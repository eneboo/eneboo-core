/****************************************************************************
**
** Implementation of Qt/FB central server
**
** Created : 991025
**
** Copyright (C) 1992-2008 Trolltech ASA.  All rights reserved.
**
** This file is part of the kernel module of the Qt GUI Toolkit.
**
** This file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free
** Software Foundation and appearing in the files LICENSE.GPL2
** and LICENSE.GPL3 included in the packaging of this file.
** Alternatively you may (at your option) use any later version
** of the GNU General Public License if such license has been
** publicly approved by Trolltech ASA (or its successors, if any)
** and the KDE Free Qt Foundation.
**
** Please review the following information to ensure GNU General
** Public Licensing requirements will be met:
** http://trolltech.com/products/qt/licenses/licensing/opensource/.
** If you are unsure which license is appropriate for your use, please
** review the following information:
** http://trolltech.com/products/qt/licenses/licensing/licensingoverview
** or contact the sales department at sales@trolltech.com.
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with
** the Software.
**
** This file is provided "AS IS" with NO WARRANTY OF ANY KIND,
** INCLUDING THE WARRANTIES OF DESIGN, MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE. Trolltech reserves all rights not granted
** herein.
**
**********************************************************************/

#ifndef QWSUTILS_QWS_H
#define QWSUTILS_QWS_H

#ifndef QT_H
#include "qwssocket_qws.h"
#endif // QT_H

/********************************************************************
 *
 * Convenient socket functions
 *
 ********************************************************************/
#ifndef QT_NO_QWS_MULTIPROCESS
inline int qws_read_uint( QWSSocket *socket )
{
    if ( !socket || socket->size() < sizeof( int ) )
	return -1;

    int i;
    socket->readBlock( (char*)&i, sizeof( i ) );

    return i;
}

inline void qws_write_uint( QWSSocket *socket, int i )
{
    if ( !socket )
	return;

    socket->writeBlock( (char*)&i, sizeof( i ) );
}

#endif // QT_NO_QWS_MULTIPROCESS

#endif // QWSUTILS_QWS_H
