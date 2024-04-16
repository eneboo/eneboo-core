/**********************************************************************
 * Copyright (c) 2004, Leo Seib, Hannover
 *
 * Project:SQLiteDataset C++ Dynamic Library
 * Module: SQLiteDataset class realisation file
 * Author: Leo Seib      E-Mail: leoseib@web.de
 * Begin: 5/04/2002
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 **********************************************************************/

#if defined(AQ_WIN64)
#include "win64.h"
#endif

#include "cstdlib"
#include <iostream>
#include <string>

#include "sqlitedataset.h"
#include <unistd.h>

namespace dbiplus
{
  //************* Callback function ***************************

  int callback(void *res_ptr, int ncol, char **reslt, char **cols)
  {

    result_set *r = (result_set *)res_ptr;
    int sz = r->records.size();


    if (!r->record_header.size())
      for (int i = 0; i < ncol; i++) {
        r->record_header[i].name = cols[i];
      }


    sql_record rec;
    

    if (reslt != NULL) {
      for (int i = 0; i < ncol; i++) {
        field_value v;
        //Siempre nuevo obj.
        //printf("\nA) reslt[%d] = %s. field_value.value =%s, is_null: %s\n", i, reslt[i], v.get_asString().c_str(), v.get_isNull() ? "SI": "NO");

        if (reslt[i] == NULL) {
          //Automï¿½ticamente marcaremos campo como null
          v.set_asString("");
          v.set_isNull(); 
        } else {
          //printf("\n++++ name:%s, type:%d", r->record_header[i].name.c_str(), r->record_header[i].type);
          v.set_asString(reslt[i]);
        }

        //printf("\nB) reslt[%d] = %s. field_value.value =%s, is_null: %s\n", i, reslt[i], v.get_asString().c_str(), v.get_isNull() ? "SI": "NO");
        rec[i] = v;
        //printf("\nC) reslt[%d] = %s. field_value.value =%s, is_null: %s\n", i, reslt[i], rec[i].get_asString().c_str(), rec[i].get_isNull() ? "SI": "NO");
      }
      r->records[sz] = rec;
    }
    return 0;
  }


  //************* SqliteDatabase implementation ***************

  SqliteDatabase::SqliteDatabase(const QString &url, const QString &user, const QString &password)
  {
    urlApi = url;
    userApi = user;
    passwordApi = password;
    tokenApi = "";
    AQProc = new QProcess(0);
    
    active = false;
    _in_transaction = false;    // for transaction

    error = "Unknown database error";//S_NO_CONNECTION;
    host = "localhost";
    port = "";
    db = "sqlite.db";
    login = "root";
    passwd, "";
  }

  SqliteDatabase::~SqliteDatabase()
  {
    disconnect();
    if ( AQProc && AQProc->isRunning() ) {
      AQProc->tryTerminate();
      AQProc->kill();
    }
  }


  Dataset *SqliteDatabase::CreateDataset() const
  {
    return new SqliteDataset((SqliteDatabase *)this);
  }

  int SqliteDatabase::status(void)
  {
    if (active == false) return DB_CONNECTION_NONE;
    return DB_CONNECTION_OK;
  }

  int SqliteDatabase::setErr(int err_code, const char *qry)
  {
    switch (err_code) {
      case SQLITE_OK:
        error = "Successful result";
        break;
      case SQLITE_ERROR:
        error = "SQL error or missing database";
        break;
      case SQLITE_INTERNAL:
        error = "An internal logic error in SQLite";
        break;
      case SQLITE_PERM:
        error = "Access permission denied";
        break;
      case SQLITE_ABORT:
        error = "Callback routine requested an abort";
        break;
      case SQLITE_BUSY:
        error = "The database file is locked";
        break;
      case SQLITE_LOCKED:
        error = "A table in the database is locked";
        break;
      case SQLITE_NOMEM:
        error = "A malloc() failed";
        break;
      case SQLITE_READONLY:
        error = "Attempt to write a readonly database";
        break;
      case SQLITE_INTERRUPT:
        error = "Operation terminated by sqlite_interrupt()";
        break;
      case  SQLITE_IOERR:
        error = "Some kind of disk I/O error occurred";
        break;
      case  SQLITE_CORRUPT:
        error = "The database disk image is malformed";
        break;
      case SQLITE_NOTFOUND:
        error = "(Internal Only) Table or record not found";
        break;
      case SQLITE_FULL:
        error = "Insertion failed because database is full";
        break;
      case SQLITE_CANTOPEN:
        error = "Unable to open the database file";
        break;
      case SQLITE_PROTOCOL:
        error = "Database lock protocol error";
        break;
      case SQLITE_EMPTY:
        error = "(Internal Only) Database table is empty";
        break;
      case SQLITE_SCHEMA:
        error = "The database schema changed";
        break;
      case SQLITE_TOOBIG:
        error = "Too much data for one row of a table";
        break;
      case SQLITE_CONSTRAINT:
        error = "Abort due to contraint violation";
        break;
      case SQLITE_MISMATCH:
        error = "Data type mismatch";
        break;
      default :
        error = "Undefined SQLite error";
    }
    error += "\nQuery: ";
    error += qry;
    error += "\n";
    return err_code;
  }

  const char *SqliteDatabase::getErrorMsg()
  {
    return error.c_str();
  }

  int SqliteDatabase::connect()
  {
    disconnect();
    int result;
    result = sqlite3_open(db.c_str(), &conn);
    //char* err=NULL;
    if (result != SQLITE_OK) {
      return DB_CONNECTION_NONE;
    }
    if (sqlite3_exec(getHandle(),"PRAGMA empty_result_callbacks=ON",NULL,NULL,NULL) != SQLITE_OK) {
        return DB_CONNECTION_NONE;
      }
    active = true;
    return DB_CONNECTION_OK;

    //return DB_CONNECTION_NONE;
  };

  void SqliteDatabase::disconnect(void)
  {
    if (active == false) return;
    sqlite3_close(conn);
    active = false;
  };

  int SqliteDatabase::create()
  {
    return connect();
  };

  int SqliteDatabase::drop()
  {
    if (active == false)
      return DB_CONNECTION_NONE;
    disconnect();
    if (!unlink(db.c_str()))
      return DB_CONNECTION_NONE;
    return DB_COMMAND_OK;
  };


  long SqliteDatabase::nextid(const char *sname)
  {
    if (!active) return DB_UNEXPECTED_RESULT;
    int id, nrow, ncol;
    result_set res;
    char sqlcmd[512];
    sprintf(sqlcmd, "select nextid from %s where seq_name = '%s'", sequence_table.c_str(), sname);
    if (last_err = sqlite3_exec(getHandle(), sqlcmd, &callback, &res, NULL) != SQLITE_OK) {
      return DB_UNEXPECTED_RESULT;
    }
    if (res.records.size() == 0) {
      id = 1;
      sprintf(sqlcmd, "insert into %s (nextid,seq_name) values (%d,'%s')", sequence_table.c_str(), id, sname);
      if (last_err = sqlite3_exec(conn, sqlcmd, NULL, NULL, NULL) != SQLITE_OK) return DB_UNEXPECTED_RESULT;
      return id;
    } else {
      id = res.records[0][0].get_asInteger() + 1;
      sprintf(sqlcmd, "update %s set nextid=%d where seq_name = '%s'", sequence_table.c_str(), id, sname);
      if (last_err = sqlite3_exec(conn, sqlcmd, NULL, NULL, NULL) != SQLITE_OK) return DB_UNEXPECTED_RESULT;
      return id;
    }
    return DB_UNEXPECTED_RESULT;
  }


  // methods for transactions
  // ---------------------------------------------
  void SqliteDatabase::start_transaction()
  {
    if (active) {
      sqlite3_exec(conn, "begin", NULL, NULL, NULL);
      _in_transaction = true;
    }
  }

  void SqliteDatabase::commit_transaction()
  {
    if (active) {
      sqlite3_exec(conn, "commit", NULL, NULL, NULL);
      _in_transaction = false;
    }
  }

  void SqliteDatabase::rollback_transaction()
  {
    if (active) {
      sqlite3_exec(conn, "rollback", NULL, NULL, NULL);
      _in_transaction = false;
    }
  }



  //************* SqliteDataset implementation ***************

  SqliteDataset::SqliteDataset(): Dataset()
  {
    haveError = false;
    db = NULL;
    errmsg = NULL;
    autorefresh = false;
  }


  SqliteDataset::SqliteDataset(SqliteDatabase *newDb): Dataset(newDb)
  {
    haveError = false;
    db = newDb;
    errmsg = NULL;
    autorefresh = false;
  }

  SqliteDataset::~SqliteDataset()
  {
    //if (errmsg) sqlite_freemem(&errmsg);
  }


  void SqliteDataset::set_autorefresh(bool val)
  {
    autorefresh = val;
  }

  QString SqliteDataset::generar_fichero_aqextension(const QString &cadena)
  {

    QString timestamp = QDateTime::currentDateTime().toString("ddMMyyyyhhmmsszzz");
    QString folder = getenv("TPM");
    if (folder.isEmpty()) {
      folder = getenv("TMPDIR");
      if (folder.isEmpty()) {
        folder = "/tmp/";
      }
    }
    QString fichero_datos = folder + "data_api" + "_" + timestamp + ".json";

    // Guardar cadena en fichero data.
    qWarning("GUARDANDO QUERY VIA API " + fichero_datos + ", cadena:" + cadena);
    QFile fi(fichero_datos);
    if (fi.open(IO_WriteOnly)) {
      QTextStream t(&fi);
      t << cadena;
      fi.close();
    } else {
      qWarning("no se ha podido escribir en el fichero " +  fichero_datos);
      return "";
    }

    return fichero_datos;
  }

  bool SqliteDataset::hacer_login_usuario(const string &user, const string &passwd)
  {
    // Hacemos login con aqextension y guardamos el token devuelto ....

    QString folder = getenv("TPM");
    if (folder.isEmpty()) {
      folder = getenv("TMPDIR");
      if (folder.isEmpty()) {
        folder = "/tmp/";
      }
    }
    qWarning("folder:" + folder + ", user:" + user + ", passwd:" + passwd);
    //QString passwd_md5 = (QString(passwd).utf8());
    //QString fichero_salida_pass = folder + "datar.md5";

    //QString passwd_md5 = lanzar_llamada_aqextension(QString("md5"), passwd, fichero_salida_pass);
    //passwd_md5 = passwd_md5.left(passwd_md5.length() - 1);
    //qWarning("El password "  + passwd + " pasa a ser " + passwd_md5);
    
    QString fichero_salida = folder + "data_token.txt";
    QString url = ((SqliteDatabase *)db)->urlApi; 
    QString cadena = "{\n";
    cadena += "\"metodo\": \"POST\",\n";
    cadena += "\"url\": \"" + url + "/login\",\n";
    cadena += "\"params\": {\n";
    cadena += "\"username\": \"" + user + "\",\n";
    cadena += "\"password\": \"" + passwd + "\"\n";
    cadena += "}\n";
    cadena += "\"tipo_payload\": \"STRING\",\n";
    cadena += "\"fsalida\":\"" + fichero_salida + "\"\n";
    cadena += "}";
    
    QString fichero_datos = generar_fichero_aqextension(cadena);
    if (fichero_datos == "") {
      qWarning("Error al generar fichero de datos");
      return false;
    }
    QString data_received = lanzar_llamada_aqextension(QString("cliente_web"), fichero_datos, fichero_salida);
    QString token = data_received.right(data_received.find("'token': '") + 10).left(token.length() - 2);

    qWarning("Token: " + token);
    if (token == "error") {
      qWarning("Error al lanzar llamada aqextension");
      return false;
    }

    ((SqliteDatabase *)db)->tokenApi = token;
    return true;
  }

  QString SqliteDataset::lanzar_llamada_aqextension(const QString &accion, const QString &argumento, const QString &fichero_salida)
  {
    
    QString path_exec = qApp->applicationDirPath() + "/aqextension";
    QString AQExtensionCall = path_exec + " " + accion + " " + argumento + " " + fichero_salida;

    QProcess *AQProc = ((SqliteDatabase *)db)->AQProc;

    AQProc->clearArguments();
    AQProc->addArgument(path_exec);
    AQProc->addArgument(accion);
    AQProc->addArgument(argumento);
    AQProc->addArgument(fichero_salida);
    
    QString salida = "";
     // Lanzar llamada via aqextension
     qWarning("LLAMANDO " + AQExtensionCall);

    if ( !AQProc->launch(salida) ) {
      qWarning("No se ha lanzado el comando : " + salida);
      return "error";
    }

    while (AQProc->isRunning()) {
      //Esperamos a que termine
      qApp->processEvents();
    }

    QString error_str = AQProc->readStderr().data();
/*     
    QString out_str = AQProc->readStdout().data();

   qWarning("Valor devuelto stdout: " + out_str);
   qWarning("Valor devuelto error: " + error_str);
   qWarning("Valor salida: " + salida); */

   //Leer un fichero y cargar el contenido
  if (!QFile::exists(fichero_salida)) {
    qWarning("No existe el fichero " + fichero_salida);
    qWarning("Error " + error_str);
    return "error";
  }


  QFile fi_salida(fichero_salida);
  if (fi_salida.open(IO_ReadOnly)) {
    
    salida = fi_salida.readAll().data();
    fi_salida.close();
  } else {
    qWarning("no se ha podido leer el fichero " +  fichero_salida);
    return "error";
  }
    //Limpia caracteres raros inicio y fin...
  const QString marca_inicio = "\"\\\""; 
  const QString marca_fin = "\\\"\"";
  if (salida.startsWith(marca_inicio)) {
    // Eliminar del inicio de salida
    salida = salida.right(salida.length() - marca_inicio.length());
  }
  if (salida.endsWith(marca_fin)) {
    // Eliminar del final de salida
    salida = salida.left(salida.length() - marca_fin.length());
  }

  qWarning("lanzar_llamada_aqextension: Valor salida: " + salida);

  return salida;
}



  //--------- protected functions implementation -----------------//

  sqlite3 *SqliteDataset::handle()
  {
    if (db != NULL) {
      return ((SqliteDatabase *)db)->getHandle();
    } else return NULL;
  }

  void SqliteDataset::make_query(StringList &_sql)
  {
    string query;

    if (autocommit) db->start_transaction();


    if (db == NULL) {
      if (db->in_transaction()) db->rollback_transaction();
      return;
    }


    for (list<string>::iterator i = _sql.begin(); i != _sql.end(); i++) {
      query = *i;
      char *err = NULL;
      Dataset::parse_sql(query);
      if (db->setErr(sqlite3_exec(this->handle(), query.c_str(), NULL, NULL, &err), query.c_str()) != SQLITE_OK) {
        if (db->in_transaction()) db->rollback_transaction();
        return;
      }
    } // end of for


    if (db->in_transaction() && autocommit) db->commit_transaction();

    active = true;
    ds_state = dsSelect;
    if (autorefresh)
      refresh();
  }


  void SqliteDataset::make_insert()
  {
    make_query(insert_sql);
    last();
  }


  void SqliteDataset::make_edit()
  {
    make_query(update_sql);
  }


  void SqliteDataset::make_deletion()
  {
    make_query(delete_sql);
  }


  void SqliteDataset::fill_fields()
  {
    //cout <<"rr "<<result.records.size()<<"|" << frecno <<"\n";
    if ((db == NULL) || (result.record_header.size() == 0) || (result.records.size() < frecno)) return;
    if (fields_object->size() == 0) // Filling columns name
      for (int i = 0; i < result.record_header.size(); i++) {
        (*fields_object)[i].props = result.record_header[i];
        (*edit_object)[i].props = result.record_header[i];
      }

    //Filling result
    if (result.records.size() != 0) {
      for (int i = 0; i < result.records[frecno].size(); i++) {
        (*fields_object)[i].val = result.records[frecno][i];
        (*edit_object)[i].val = result.records[frecno][i];
      }
    } else
      for (int i = 0; i < result.record_header.size(); i++) {
        (*fields_object)[i].val = "";
        (*edit_object)[i].val = "";
      }

  }


  //------------- public functions implementation -----------------//

  int SqliteDataset::exec(const string &sql)
  {
    if (!handle()) return DB_ERROR;
    int res;
    exec_res.record_header.clear();
    exec_res.records.clear();
    if (res = db->setErr(sqlite3_exec(handle(), sql.c_str(), &callback, &exec_res, &errmsg), sql.c_str()) == SQLITE_OK)
      return res;
    else
      return DB_ERROR;
  }

  int SqliteDataset::exec()
  {
    return exec(sql);
  }

  const void *SqliteDataset::getExecRes()
  {
    return &exec_res;
  }


  bool SqliteDataset::query(const char *query)
  {
    if (db == NULL)
      return false;
    if (((SqliteDatabase *)db)->getHandle() == NULL)
      return false;
    std::string qry = query;
    int fs = qry.find("select");
    int fS = qry.find("SELECT");
    if (!(fs >= 0 || fS >= 0))
      return false;

    close();

    QString url = ((SqliteDatabase *)db)->urlApi; 
    
    
    if (((SqliteDatabase *)db)->tokenApi.isEmpty()) {
      qWarning("No hay token disponible. Solicitando ...");
      QString user = ((SqliteDatabase *)db)->userApi;
      QString password = ((SqliteDatabase *)db)->passwordApi;
      hacer_login_usuario(user, password);
    }
    QString token = ((SqliteDatabase *)db)->tokenApi;

    QString folder = getenv("TPM");
    if (folder.isEmpty()) {
      folder = getenv("TMPDIR");
      if (folder.isEmpty()) {
        folder = "/tmp/";
      }
    }


    QString timestamp = QDateTime::currentDateTime().toString("ddMMyyyyhhmmsszzz");
    QString separador_campos = "|^|";
    QString separador_lineas = "|^^|";
    QString fichero_salida =  folder + "delegate_qry_" + timestamp + ".txt";

    QString cadena = "{\n";
    cadena += "\"metodo\": \"GET\",\n";
    cadena += "\"url\": \"" + url + "/delegate_qry\",\n";
    cadena += "\"params\":{\n";
    cadena += "\"delegate_qry\":\"" + qry + "\",\n";
    cadena += "\"separador_campos\":\"" + separador_campos + "\",\n";
    cadena += "\"separador_lineas\":\"" + separador_lineas + "\"\n";
    cadena += "},\n";
    cadena += "\"headers\": { \"Authorization\": \"Token " + token + "\"},\n";
    cadena += "\"codificacion\": \"UTF-8\",\n";
    cadena += "\"tipo_payload\": \"STRING\",\n";
    cadena += "\"fsalida\":\"" + fichero_salida + "\"\n";
    cadena += "}";
    
    QString fichero_datos = generar_fichero_aqextension(cadena);
    if (fichero_datos == "") {
      qWarning("Error al generar fichero de datos");
      return false;
    }

  QString salida = lanzar_llamada_aqextension(QString("cliente_web"), fichero_datos, fichero_salida);

  QStringList lista_registros(QStringList::split(separador_lineas, salida));
  

  result.record_header.clear();
  bool first = true;
  qWarning("PROCESANDO LINEAS RECIBIDAS");
  for (QStringList::Iterator it = lista_registros.begin(); it != lista_registros.end(); ++it) {
    
    qWarning("PROCESANDO LINEA");
    QString registro = *it;

    QStringList lista_valores(QStringList::split(separador_campos, registro));

    if (first == true) { //cabecera ...
      // Cargamos registro de cabecera:
      qWarning("PROCESANDO CABECERA"); 
      for (int i = 0; i < lista_valores.size(); i++) {
        const QString datos_columna = lista_valores[i];
        QStringList columna = QStringList::split("|", datos_columna);
        qWarning("COLUMNA: %d -> %s (%s) ", i, *columna[0], *columna[1]);
        result.record_header[i].name = *columna[0];
      }
      first = false;
      continue;
    } else { // valores ...

    qWarning("PROCESANDO VALORES");

    int sz = result.records.size(); 
    
    // Creamos listado con valores
    sql_record rec;
    for (int i = 0; i < lista_valores.size(); i++) {  

      field_value v;
      const std::string valor = lista_valores[i];
    
      
      if (valor == NULL) {
          //Automáticamente marcaremos campo como null
          v.set_asString("");
          v.set_isNull(); 
        } else {
          v.set_asString(valor); // entra siempre como string ...
        }
       rec[i] = v;
 
      }

    result.records[sz] = rec;

    }
  }

  active = true;
  ds_state = dsSelect;
  this->first();
  return true;
  
/*     if (db->setErr(sqlite3_exec(handle(), query, &callback, &result, &errmsg), query) == SQLITE_OK) {
      active = true;
      ds_state = dsSelect;
      this->first();
      return true;
    } */

  }

  bool SqliteDataset::query(const string &q)
  {
    return query(q.c_str());
  }

  void SqliteDataset::open(const string &sql)
  {
    set_select_sql(sql);
    open();
  }

  void SqliteDataset::open()
  {
    if (select_sql.size()) {
      query(select_sql.c_str());
    } else {
      ds_state = dsInactive;
    }
  }


  void SqliteDataset::close()
  {
    Dataset::close();
    result.record_header.clear();
    result.records.clear();
    edit_object->clear();
    fields_object->clear();
    ds_state = dsInactive;
    active = false;
  }


  void SqliteDataset::cancel()
  {
    if ((ds_state == dsInsert) || (ds_state == dsEdit))
      if (result.record_header.size()) ds_state = dsSelect;
      else ds_state = dsInactive;
  }


  int SqliteDataset::num_rows()
  {
    return result.records.size();
  }


  bool SqliteDataset::eof()
  {
    return feof;
  }


  bool SqliteDataset::bof()
  {
    return fbof;
  }


  void SqliteDataset::first()
  {
    Dataset::first();
    this->fill_fields();
  }

  void SqliteDataset::last()
  {
    Dataset::last();
    fill_fields();
  }

  void SqliteDataset::prev(void)
  {
    Dataset::prev();
    fill_fields();
  }

  void SqliteDataset::next(void)
  {
    Dataset::next();
    if (!eof())
      fill_fields();
  }


  bool SqliteDataset::seek(int pos)
  {
    if (ds_state == dsSelect) {
      Dataset::seek(pos);
      fill_fields();
      return true;
    }
    return false;
  }



  long SqliteDataset::nextid(const char *seq_name)
  {
    if (handle()) return db->nextid(seq_name);
    else return DB_UNEXPECTED_RESULT;
  }

}//namespace
