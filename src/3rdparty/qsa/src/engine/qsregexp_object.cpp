/****************************************************************************
**
** Copyright (C) 1992-2006 Trolltech AS. All rights reserved.
**
** This file is part of the QSA of the Qt Toolkit.
**
** For QSA Commercial License Holders (non-open source):
** 
** Licensees holding a valid Qt Script for Applications (QSA) License Agreement
** may use this file in accordance with the rights, responsibilities and
** obligations contained therein.  Please consult your licensing agreement or
** contact sales@trolltech.com if any conditions of the Licensing Agreement are
** not clear to you.
** 
** Further information about QSA licensing is available at:
** http://www.trolltech.com/products/qsa/licensing.html or by contacting
** info@trolltech.com.
** 
** 
** For Open Source Edition:  
** 
** This file may be used under the terms of the GNU General Public License
** version 2 as published by the Free Software Foundation and appearing in the
** file LICENSE.GPL included in the packaging of this file.  Please review the
** following information to ensure GNU General Public Licensing requirements
** will be met:  http://www.trolltech.com/products/qt/opensource.html 
** 
** If you are unsure which license is appropriate for your use, please review
** the following information:
** http://www.trolltech.com/products/qsa/licensing.html or contact the 
** sales department at sales@trolltech.com.

**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include "qsregexp_object.h"
#include "qsoperations.h"
#include "qsenv.h"
#include "qstypes.h"
#include "qsinternal.h"
#include "qsarray_object.h"
#include "qslexer.h"
#include <qregexp.h>

class QSRegExpShared : public QSWritable
{
public:
    QSRegExpShared( const QString &pat, bool ic, bool glob )
        : reg( pat, ic ? Qt::CaseInsensitive : Qt::CaseSensitive ),
            source( pat ),
            global( glob ),
            ignoreCase( ic )
    {}

    QRegExp reg;
    QString source;
    bool global;
    bool ignoreCase;
};

enum RegExpMemberType {
    REMT_Intermediate,
    REMT_Empty,
    REMT_MLength,
    REMT_CTexts,
    REMT_Source,
    REMT_Global,
    REMT_IgnoreCase,
    REMT_LastIndex
};

QSRegExpClass::QSRegExpClass( QSClass *b )
: QSWritableClass( b )
{
    int a = AttributeNonWritable;
    addMember( REG_USTR( QLatin1String( "valid" ) ), QSMember( QSMember::Custom, REMT_Intermediate, a ) );
    addMember( REG_USTR( QLatin1String( "empty" ) ), QSMember( QSMember::Custom, REMT_Empty, a ) );
    addMember( REG_USTR( QLatin1String( "matchedLength" ) ), QSMember( QSMember::Custom, REMT_MLength, a ) );
    addMember( REG_USTR( QLatin1String( "capturedTexts" ) ), QSMember( QSMember::Custom, REMT_CTexts, a ) );
    addMember( REG_USTR( QLatin1String( "source" ) ), QSMember( QSMember::Custom, REMT_Source, a ) );
    addMember( QSLexer::global_, QSMember( QSMember::Custom, REMT_Global, AttributePublic ) );
    addMember( REG_USTR( QLatin1String( "ignoreCase" ) ), QSMember( QSMember::Custom, REMT_IgnoreCase, AttributePublic ) );

    addMember( REG_USTR( QLatin1String( "toString" ) ), QSMember( &QSRegExpClass::toStringScript ) );
    addMember( REG_USTR( QLatin1String( "search" ) ), QSMember( &search ) );
    addMember( REG_USTR( QLatin1String( "searchRev" ) ), QSMember( &searchRev ) );
    addMember( REG_USTR( QLatin1String( "exactMatch" ) ), QSMember( &exactMatch ) );
    addMember( REG_USTR( QLatin1String( "pos" ) ), QSMember( &pos ) );
    addMember( REG_USTR( QLatin1String( "cap" ) ), QSMember( &cap ) );
}

/*!
 * Convenience function that retrieves a pointer to the QRegExp
 * object contained in \a obj. Only to be used if \a obj is of
 * type RegExp.
 */

QRegExp * QSRegExpClass::regExp( const QSObject *obj )
{
    Q_ASSERT( obj->typeName() == QLatin1String( "RegExp" ) );
    return &( static_cast<QSRegExpShared *>( obj->shVal() ) ->reg );
}

/*!
  \internal
  \overload
*/

QRegExp * QSRegExpClass::regExp( QSEnv *e )
{
    QSObject t = e->thisValue();
    Q_ASSERT( t.isA( e->regexpClass() ) );
    return &( static_cast<QSRegExpShared *>( t.shVal() ) ->reg );
}

/*! \reimp */
QSObject QSRegExpClass::cast( const QSList &args ) const
{
    return construct( args );
}

QSObject QSRegExpClass::construct( const QSList &args ) const
{
    // ### regexp arguments
    QString p( args.isEmpty() ? QString() : args[ 0 ].toString() );

    if ( args.size() < 2 ) {
        QSRegExpShared * sh = new QSRegExpShared( p, false, false );
        return env() ->createShared( this, sh );
    }

    QString flags = args[ 1 ].toString();
    bool ignoreCase = ( flags.indexOf( QLatin1String( "i" ) ) >= 0 );
    bool global = ( flags.indexOf( QLatin1String( "g" ) ) >= 0 );
    QSRegExpShared *sh = new QSRegExpShared( p, ignoreCase, global ); // ### apply flags
    return env() ->createShared( this, sh );
}

QSObject QSRegExpClass::fetchValue( const QSObject *objPtr,
                                    const QSMember &mem ) const
{
    if ( mem.type() != QSMember::Custom )
        return QSWritableClass::fetchValue( objPtr, mem );

    QRegExp *re = regExp( objPtr );
    switch ( mem.index() ) {
    case REMT_Intermediate:
        return createBoolean( re->isValid() );
    case REMT_Empty:
        return createBoolean( re->isEmpty() );
    case REMT_MLength:
        return createNumber( re->matchedLength() );
    case REMT_Source:
        return createString( source( objPtr ) );
    case REMT_Global:
        return createBoolean( isGlobal( objPtr ) );
    case REMT_IgnoreCase:
        return createBoolean( isIgnoreCase( objPtr ) );
    case REMT_CTexts: {
            QSArray array( env() );
            QStringList ct = re->capturedTexts();
            QStringList::ConstIterator it = ct.begin();
            int i = 0;
            for ( ; it != ct.end(); ++it, ++i )
                array.put( NEW_USTRNUM( i ), createString( *it ) );
            array.put( QSLexer::length_, createNumber( i ) );
            return array;
        }
    default:
            return createUndefined();
    }
}

void QSRegExpClass::write( QSObject *objPtr, const QSMember &mem,
                           const QSObject &val ) const
{
    if ( mem.type() != QSMember::Custom ) {
        QSWritableClass::write( objPtr, mem, val );
        return ;
    }

    Q_ASSERT( objPtr->objectType() == objPtr->objectType() ->env() ->regexpClass() );

    switch ( mem.index() ) {
    case REMT_Source:
        ( ( QSRegExpShared* ) objPtr->shVal() ) ->source = val.toString();
        break;
    case REMT_Global:
        ( ( QSRegExpShared* ) objPtr->shVal() ) ->global = val.toBoolean();
        break;
    case REMT_IgnoreCase: {
            bool ic = val.toBoolean();
            ( ( QSRegExpShared* ) objPtr->shVal() ) ->ignoreCase = ic;
            ( ( QSRegExpShared* ) objPtr->shVal() ) ->reg.setCaseSensitivity( ic ? Qt::CaseInsensitive : Qt::CaseSensitive );
        }
        break;
    default:
        QSWritableClass::write( objPtr, mem, val );
    }
}

#if 0
bool QSRegExpClass::hasStaticProperty( const QString &p ) const
{
    return ( p.length() == 2 && p[ 0 ] == '$' && p[ 1 ].isDigit() ) ||
           ( p.length() == 3 && p[ 0 ] == '$' && p[ 1 ].isDigit() &&
             p[ 2 ].isDigit() );
}

QSObject QSRegExpClass::getStatic( const QString &p ) const
{
    if ( ( p.length() == 2 && p[ 0 ] == '$' && p[ 1 ].isDigit() ) ||
            ( p.length() == 3 && p[ 0 ] == '$' && p[ 1 ].isDigit() &&
              p[ 2 ].isDigit() ) ) {
        int i = p.mid( 1 ).toInt();
        if ( i < ( int ) lastCaptures.count() )
            return createString( lastCaptures[ i ] );
        else
            return createString( "" );
    }

    return createUndefined();
}
#endif

QString QSRegExpClass::toString( const QSObject *obj ) const
{
    return QLatin1String( "/" ) + regExp( obj ) ->pattern() + QLatin1String( "/" );
}

QSObject QSRegExpClass::exec( QSEnv *env )
{
    QSObject obj = env->thisValue();
    QRegExp *re = regExp( &obj );
    QString s = env->arg( 0 ).toString();
    uint length = s.length();
    int i = obj.get( QSLexer::lastindex_ ).toInt32();
    QSObject tmp = obj.get( QSLexer::global_ );
    if ( !tmp.toBoolean() )
        i = 0;
    if ( i < 0 || i > ( int ) length ) {
        obj.put( QSLexer::lastindex_, 0 );
        return env->createNull();
    }
    i = s.indexOf( *re, i );
    obj.env() ->regexpClass() ->lastCaptures = re->capturedTexts();
    // ### complete
    return env->createString( NEW_USTR( re->cap( 0 ) ) );
}

QSObject QSRegExpClass::test( QSEnv *env )
{
    QSObject obj = env->thisValue();
    QRegExp *re = regExp( &obj );
    QString s = env->arg( 0 ).toString();
    uint length = s.length();
    int i = obj.get( QSLexer::lastindex_ ).toInt32();
    QSObject tmp = obj.get( QSLexer::global_ );
    if ( !tmp.toBoolean() )
        i = 0;
    if ( i < 0 || i > ( int ) length ) {
        obj.put( QSLexer::lastindex_, 0 );
        return env->createBoolean( false );
    }
    i = s.indexOf( *re, i );
    obj.env() ->regexpClass() ->lastCaptures = re->capturedTexts();
    return env->createBoolean( i >= 0 );
}

QSObject QSRegExpClass::toStringScript( QSEnv *env )
{
    QSObject that = env->thisValue();
    Q_ASSERT( that.objectType() == env->regexpClass() );
    QString pattern = QLatin1String( "/" ) + source( &that ) + QLatin1String( "/" );
    if ( isIgnoreCase( &that ) )
        pattern += 'i';
    if ( isGlobal( &that ) )
        pattern += 'g';
    return env->createString( NEW_USTR( pattern ) );
}

QSObject QSRegExpClass::search( QSEnv *env )
{
    int start = env->numArgs() >= 2 ? env->arg( 1 ).toInteger() : 0;
    return env->createNumber( env->arg( 0 ).toString().indexOf( *regExp( env ), start ) );
}

QSObject QSRegExpClass::searchRev( QSEnv *env )
{
    int start = env->numArgs() >= 2 ? env->arg( 1 ).toInteger() : -1;
    return env->createNumber( env->arg( 0 ).toString().lastIndexOf( *regExp( env ), start ) );
}

QSObject QSRegExpClass::exactMatch( QSEnv *env )
{
    return env->createBoolean( regExp( env ) ->exactMatch( env->arg( 0 ).toString() ) );
}

QSObject QSRegExpClass::pos( QSEnv *env )
{
    return env->createNumber( regExp( env ) ->pos( env->numArgs() >= 1 ?
                              env->arg( 0 ).toInteger() : 0 ) );
}

QSObject QSRegExpClass::cap( QSEnv *env )
{
    return env->createString( NEW_USTR( regExp( env ) ->cap( env->numArgs() >= 1 ?
                                        env->arg( 0 ).toInteger() : 0 ) ) );
}

QString QSRegExpClass::source( const QSObject *re )
{
    Q_ASSERT( re->objectType() == re->objectType() ->env() ->regexpClass() );
    return ( ( QSRegExpShared* ) re->shVal() ) ->source;
}

bool QSRegExpClass::isGlobal( const QSObject *re )
{
    Q_ASSERT( re->objectType() == re->objectType() ->env() ->regexpClass() );
    return ( ( QSRegExpShared* ) re->shVal() ) ->global;
}

bool QSRegExpClass::isIgnoreCase( const QSObject *re )
{
    Q_ASSERT( re->objectType() == re->objectType() ->env() ->regexpClass() );
    return ( ( QSRegExpShared* ) re->shVal() ) ->ignoreCase;
}