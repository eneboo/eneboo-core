PRJ_MODULE = SQLApi

exists(../../settings.pro) {
	include(../../settings.pro)
}
else {
	error("settings.pro missing, unable to build")
}

TEMPLATE = lib

CONFIG += warn_off create_prl link_prl
!mac:CONFIG += plugin

win32 {
	CONFIG += dll exceptions shared
	DLLDESTDIR = $$PREFIX/bin
	DEFINES += OS_WIN=1
	enable_win64:DEFINES += _OFF_T_DEFINED
}
shared {
	win32:DEFINES	+= AQSQLITE_DLL
} else {
	win32:DEFINES += AQSQLITE_NO_DLL
}
unix {
	CONFIG -= x11
	DEFINES += OS_UNIX=1
}

DESTDIR = $$PREFIX/lib

TARGET = sqlapi

LIBS += -L$$PREFIX/lib

INCLUDEPATH += $$ROOT/src/qt/include ./

VERSION = 1.00

SOURCES += sqlite3.c \
           shell.c \
	         opcodes.c \         
           dataset.cpp qry_dat.cpp sqlitedataset.cpp

HEADERS += aqsqliteglobal.h \
           sqlite3ext.h \
           sqlite3.h \
           opcodes.h \
           config_sqlite.h \
 	         dataset.h qry_dat.h sqlitedataset.h

