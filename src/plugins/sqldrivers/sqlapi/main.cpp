/***************************************************************************
                         main.cpp  -  description
                            -------------------
   begin                : mie Dic 3 2003
   copyright            : (C) 2003-2004 by InfoSiAL S.L.
   email                : mail@infosial.com
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <qsqldriverplugin.h>
#include "sqlapi.h"

class SqlapiPlugin : public QSqlDriverPlugin
{
public:

    SqlapiPlugin();
    QSqlDriver* create( const QString & );
    QStringList keys() const;
};

SqlapiPlugin::SqlapiPlugin() : QSqlDriverPlugin() {}

QSqlDriver* SqlapiPlugin::create( const QString &name )
{
    if ( name == "FLsqlapi" ) {
        SqlApiDriver * driver = new SqlApiDriver();
        return driver;
    }
    return 0;
}

QStringList SqlapiPlugin::keys() const
{
    QStringList l;
    l.append( "FLsqlapi" );
    return l;
}

Q_EXPORT_PLUGIN( SqlapiPlugin )
