PRJ_MODULE      = Eneboo-src
exists(../settings.pro) {
  include(../settings.pro)
}
else {
  error("settings.pro missing, unable to build")
}

TEMPLATE = subdirs

CONFIG += warn_off
CONFIG += ordered


enable_hoard:SUBDIRS += hoard
enable_qwt:SUBDIRS += qwt
enable_digidoc:SUBDIRS += libdigidoc/openssl/crypto libdigidoc/openssl/ssl libdigidoc/libxml2 libdigidoc

SUBDIRS += libxslt
SUBDIRS += kudesigner teddy
SUBDIRS += lrelease barcode kugar advance flmail libpq sqlite sqlapi libmysql dbf flbase plugins
enable_flfcgi:SUBDIRS += flfcgi
SUBDIRS += fllite

