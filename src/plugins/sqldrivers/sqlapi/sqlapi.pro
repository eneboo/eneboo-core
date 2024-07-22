PRJ_MODULE      = SqlDrivers-SqlApi
exists(../../../../settings.pro) {
        include(../../../../settings.pro)
}
else {
        error("settings.pro missing, unable to build")
}
TEMPLATE = lib
CONFIG += warn_off plugin
DESTDIR = $$PREFIX/plugins/sqldrivers
TARGET = qsqlapi
INCLUDEPATH += $$ROOT $$ROOT/src/flbase $$ROOT/src/sqlapi

mac {
load(qsa)
LIBS += -L$$PREFIX/lib -lflbase -lsqlapi -ladvance -lqui -lflmail -lqwt
enable_digidoc:LIBS += -llibdigidoc -lcrypto -lssl -lxml2
LIBS += -lxslt -lexslt -lxsltproc
} 
else {
LIBS += -L$$PREFIX/lib -lflbase -lsqlapi
}

HEADERS += sqlapi.h
SOURCES += sqlapi.cpp main.cpp
