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
#include <math.h>
#include <time.h>

#define LIMIT_RESULT 1000

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
          //Autom?ticamente marcaremos campo como null
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
    userIdApi="";
    
    active = false;
    _in_transaction = false;    // for transaction

    error = "Unknown database error";//S_NO_CONNECTION;
    host = "localhost";
    port = "";
    db = "sqlite.db";
    login = "root";
    passwd, "";
    AQProc = new QProcess(0);
  }

  SqliteDatabase::~SqliteDatabase()
  {
    disconnect();
    if ( AQProc && AQProc->isRunning() ) {
      qWarning("AQProc->kill()!!");
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

    SqliteDataset *ds = new SqliteDataset((SqliteDatabase *)this);
    ds->sql = "select * from pg_stat_activity where datname = '" + db + "'";
    if (!ds->gestionar_consulta_paginada(0)) {
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
    debug_sql = false;
    debug_paginacion = true;
    debug_aqextension = false;
    last_pos_fetched = 0;
    last_invalid_pos = 0;
    bloque_last = 0;
    bloque_pos = 0;
  }


  SqliteDataset::SqliteDataset(SqliteDatabase *newDb): Dataset(newDb)
  {
    haveError = false;
    db = newDb;
    errmsg = NULL;
    autorefresh = false;
    debug_sql = false;
    debug_paginacion = true;
    debug_aqextension = false;
    last_pos_fetched = 0;
    last_invalid_pos = 0;
    bloque_last = 0;
    bloque_pos = 0;
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
    //qWarning("GUARDANDO QUERY VIA API " + fichero_datos + ", cadena:" + cadena);
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
    qWarning("No hay token disponible. Solicitando ...");
    QString folder = getenv("TPM");
    if (folder.isEmpty()) {
      folder = getenv("TMPDIR");
      if (folder.isEmpty()) {
        folder = "/tmp/";
      }
    }
    //qWarning("folder:" + folder + ", user:" + user + ", passwd:" + passwd);
    //QString passwd_md5 = (QString(passwd).utf8());
    //QString fichero_salida_pass = folder + "datar.md5";

    //QString passwd_md5 = lanzar_llamada_aqextension(QString("md5"), passwd, fichero_salida_pass);
    //passwd_md5 = passwd_md5.left(passwd_md5.length() - 1);
    //qWarning("El password "  + passwd + " pasa a ser " + passwd_md5);
    
    QString fichero_salida = folder + "data_token.txt";
    QString url = ((SqliteDatabase *)db)->urlApi; 
    QString cadena = "{\n";

    url = url.replace("/api", "");

    cadena += "\"metodo\": \"POST\",\n";
    cadena += "\"url\": \"" + url + "/login\",\n";
    cadena += "\"data\": {\n";
    cadena += "\"username\": \"" + user + "\",\n";
    cadena += "\"password\": \"" + passwd + "\"\n";
    cadena += "},\n";
    cadena += "\"fsalida\":\"" + fichero_salida + "\",\n";
    //cadena += "\"only_key\":\"token\",\n";
    cadena += "\"close_when_finish\":false\n";
    cadena += "}";
    qWarning("Fichero salida token : " + fichero_salida);
    QString fichero_datos = generar_fichero_aqextension(cadena);
    if (fichero_datos == "") {
      qWarning("Error al generar fichero de datos");
      return false;
    }
    QString data_received = lanzar_llamada_aqextension(QString("cliente_web"), fichero_datos, fichero_salida);
    QString token = data_received.right(data_received.length() - (data_received.find("\"token\": \"") + 10));
    //qWarning("Token(1): " + token);
    token = token.left(token.find("\""));

    QString user_id = data_received.right(data_received.length() - (data_received.find("\"user\": \"") + 9));
    user_id = user_id.left(user_id.find("\""));


    if (token == "error") {
      qWarning("Error al solicitar login");
      return false;
    }
    qWarning("User: " + user_id);
    qWarning("Token(2): " + token);
    ((SqliteDatabase *)db)->userIdApi = user_id;
    ((SqliteDatabase *)db)->tokenApi = token;
    return true;
  }

  QString SqliteDataset::lanzar_llamada_aqextension(const QString &accion, const QString &fichero_datos, const QString &fichero_salida)
  {
    bool usar_py = false;

    QString path_exec = "";
    QString comando_txt = "";
    if (usar_py) {
      path_exec = qApp->applicationDirPath() + "/aqextension.py";
      comando_txt = "python3 " + path_exec + " " + accion + " " + fichero_datos;
    } else {
      path_exec = qApp->applicationDirPath() + "/aqextension";
      comando_txt = path_exec + " " + accion + " " + fichero_datos;
    }

    if (QFile::exists(fichero_salida)) {
      if (debug_aqextension) {
        qWarning("Eliminando fichero salida " + fichero_salida);
      }
      QFile::remove(fichero_salida);
    }
    QProcess *AQProc = ((SqliteDatabase *)db)->AQProc;
    if (debug_aqextension) {
      qWarning("Comando: " + comando_txt);
    }
    if (!AQProc->isRunning()) {
      if (debug_aqextension) {
        qWarning("PROCESO PARADO! :(");
      }
      AQProc->clearArguments();
      if (usar_py) {
        AQProc->addArgument("python3");
      }
      AQProc->addArgument(path_exec);
      AQProc->addArgument(accion);
      AQProc->addArgument(fichero_datos);

      if ( !AQProc->start() ) {
        return "error";
      }
    } else {
      // Solo pasarle argumento ...
      if (debug_aqextension) {
        qWarning("PROCESO EN EJECUCION! :)");
      }
      AQProc->writeToStdin(fichero_datos + "\n");
    }

    if (debug_aqextension) {
      qWarning("Esperando a que se procese la llamada");
    }
    while (AQProc->isRunning()) {
      //Esperamos a que termine
      qApp->processEvents();
      if(QFile::exists(fichero_salida)) {
        //qWarning("Fichero salida encontrado");
        break;
      }
    }
    if (debug_aqextension) {
      qWarning("Fin de la espera");
    }
  
  if (!QFile::exists(fichero_salida)) {
    qWarning("No existe fichero salida " + fichero_salida + ", fichero datos: " + fichero_datos);
    QString error_str = AQProc->readStderr().data();
    qWarning("Error devuelto: " + error_str);
    qWarning("Reiniciando proceso");
    AQProc->tryTerminate();
    ((SqliteDatabase *)db)->AQProc = new QProcess();
    return "error";
  }

  QString salida = "";
  QFile fi_salida(fichero_salida);

  if (fi_salida.open(IO_ReadOnly)) {
    QTextStream t(&fi_salida);
    salida = t.read();
    fi_salida.close();
  } else {
    qWarning("No se ha podido leer el fichero " +  fichero_salida + ", aunque existe");
    salida = "error";
  }

  if (salida == "") {
    //QFile::remove(fichero_salida);
    //QFile::remove(fichero_datos);
  }
  //QFile::remove(fichero_salida);

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
    if ((db == NULL) || (result.record_header.size() == 0) || (num_rows() < frecno)) return;
    if (fields_object->size() == 0) // Filling columns name
      for (int i = 0; i < result.record_header.size(); i++) {
        (*fields_object)[i].props = result.record_header[i];
        (*edit_object)[i].props = result.record_header[i];
      }

    //Filling result
    if (num_rows() != 0) {
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
    SqliteDataset::sql = sql;
    if (gestionar_consulta_paginada(0))
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
    sql = query;
    
    result.record_header.clear();
    result.records.clear();
    result.total_records = 0;
    pila_paginacion.clear();
    lista_bloques.clear();
    bool res = true;
    res = gestionar_consulta_paginada(0); 
    if (!res) {
      db->setErr(SQLITE_ERROR,sql);
      return false;
    }
   
    lista_bloques[0] = true;
  

  active = true;
  ds_state = dsSelect;
  this->first();
  return true;
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
    pila_paginacion.clear();
    lista_bloques.clear();
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
    return result.total_records;

    //return result.records.size();
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

int SqliteDataset::resuelve_bloque(const int posicion) {
  
    int parte_entera = num_rows() > 0 ? floor(posicion / LIMIT_RESULT): 0;
    return parte_entera;
}

void SqliteDataset::lista_bloques_pila_paginacion()
{
    if (!debug_paginacion) {
      return;
    }
      //Muestro lista de bloques en pantalla
    qWarning("\tPila de bloques:");
    for (list<int>::iterator it = pila_paginacion.begin(); it != pila_paginacion.end(); ++it) {
      qWarning("\t\t- %d", *it);
    }
}


bool SqliteDataset::fetch_rows(int pos) {
    
    
    int codigo_bloque = resuelve_bloque(pos);
    
    if (lista_bloques.count(codigo_bloque) == 0) { // si no esta en la lista, lo meto el primero
      lista_bloques[codigo_bloque] = false;
    } else { // si esta en la pila, no hago nada
      return true;
    }
    
  /*     if (pila_paginacion.size() == 0) { // Si no hay bloques en la pila, salgo
        qWarning(":( estoy perdido %d" , pos);
        return false;
    } */


      if (debug_paginacion) {
        qWarning(" + Bloque %d en proceso", codigo_bloque);
      }
      
      bool fetch_result = gestionar_consulta_paginada(codigo_bloque * LIMIT_RESULT); // Aqui realizo la carga del bloque
      // eliminamos codigo_bloque de pila_paginacion
      lista_bloques[codigo_bloque] = fetch_result;
      qWarning("Bloque %d resuelto", codigo_bloque);
      //pila_paginacion.remove(codigo_bloque);
      
      if (debug_paginacion) {
        qWarning(" - Bloque %d (%d : %s) Procesado %s , rango: %d - %d", codigo_bloque, pos, result.records.count(pos) == 1 ? "OK" : "KO", fetch_result ? "OK" : "FALLO", codigo_bloque * LIMIT_RESULT, (codigo_bloque * LIMIT_RESULT) + LIMIT_RESULT - 1);
        //lista_bloques_pila_paginacion();
      }

      return fetch_result;      
}







  QString SqliteDataset::generarJsonQuery(const QString &qry, const QString &fichero_salida,const int offset)
  {
    QString url = ((SqliteDatabase *)db)->urlApi; 
    QString token = ((SqliteDatabase *)db)->tokenApi;



    QString cadena = "{\n";
    cadena += "\"metodo\": \"GET\",\n";
    cadena += "\"url\": \"" + url + "/delegate_qry\",\n";
    cadena += "\"params\":{\n";
    cadena += "\"sql\":\"" + qry + "\",\n";
    cadena += "\"is_query\":" + QString(qry.lower().startsWith("select") ? "true" : "false") + ",\n";
    cadena += "\"offset\":" + QString::number(offset) + ",\n";
    cadena += "\"limit\":" + QString::number(LIMIT_RESULT) + "\n";
    cadena += "},\n";
    cadena += "\"headers\": { \"Authorization\": \"Token " + token + "\"},\n";
    cadena += "\"codificacion\": \"UTF-8\",\n";
    //cadena += "\"tipo_payload\": \"STRING\",\n";
    cadena += "\"fsalida\":\"" + fichero_salida + "\",\n";
    cadena += "\"enable_debug\":true,\n";
    cadena += "\"only_key\":\"data\",\n";
    cadena += "\"close_when_finish\":false\n";
    cadena += "}";

    return cadena;
  }

  bool SqliteDataset::gestionar_consulta_paginada(const int offset)
  {
    
    QString current_sql = sql;
    
    if (((SqliteDatabase *)db)->tokenApi.isEmpty()) {
      
      QString user = ((SqliteDatabase *)db)->userApi;
      QString password = ((SqliteDatabase *)db)->passwordApi;
      if (!hacer_login_usuario(user, password)) {
        qWarning("Error al hacer login. QUERY Cancelada");
        db->setErr(SQLITE_ERROR, "auth error");
        return false;
      }
      }

    

    QString folder = getenv("TPM");
    if (folder.isEmpty()) {
      folder = getenv("TMPDIR");
      if (folder.isEmpty()) {
        folder = "/tmp/";
      }
    }

    QString separador_campos = "|^|";
    QString separador_lineas = "|^^|";
    QString separador_total = "@";


    QString timestamp = QDateTime::currentDateTime().toString("ddMMyyyyhhmmsszzz");
    QString fichero_salida =  folder + "delegate_qry_" + timestamp + ".txt";
    QString cadena = generarJsonQuery(current_sql, fichero_salida, offset);    
    QString fichero_datos = generar_fichero_aqextension(cadena);
    QString salida = lanzar_llamada_aqextension(QString("cliente_web"), fichero_datos, fichero_salida);

  if (salida == "error") {
      qWarning("Error al lanzar llamada aqextension. SQL:" + QString(current_sql));
      return false;
    }
  if (debug_sql) {
    qWarning("Procesando respuesta");
  }
  
  if (result.total_records == 0) {
    QStringList lista_arrobas = QStringList::split(separador_total, salida);
    for (QStringList::Iterator it = lista_arrobas.begin(); it != lista_arrobas.end(); ++it) {
      result.total_records = QString(*it).toInt();
      // TODO: forwardonly.
      if (debug_sql) {
        qWarning("PAGINACIÓN: TOTAL RECORDS: %d", result.total_records);
      }
      break;
    }
  }

  QString salida_datos = salida.right(salida.length() - (salida.find(separador_total) + 1));
  QStringList lista_registros(QStringList::split(separador_lineas, salida_datos));
  
  bool first = true;
  int posicion_idx = offset;
  //qWarning("PROCESANDO LINEAS RECIBIDAS (%d)", lista_registros.count());
  for (QStringList::Iterator it = lista_registros.begin(); it != lista_registros.end(); ++it) {
    
    //qWarning("PROCESANDO LINEA");
    QString registro = *it;

    QStringList lista_valores(QStringList::split(separador_campos, registro));

    if (first == true) { //cabecera ...
      // Cargamos registro de cabecera:
      //qWarning("PROCESANDO CABECERA. columnas %d", lista_valores.count()); 
      for (QStringList::Iterator it2 = lista_valores.begin(); it2 != lista_valores.end(); ++it2) {
        const int col_numero = result.record_header.size() + 1;
        const QString datos_columna = *it2;
        QStringList columna = QStringList::split("|", datos_columna);
        for (QStringList::Iterator it3 = columna.begin(); it3 != columna.end(); ++it3) {
          QString nombre_columna = *it3;
          //qWarning("Especificando nombre col : %d", col_numero);
          //qWarning(nombre_columna);
          if (nombre_columna.endsWith(" ")) {
            nombre_columna = nombre_columna.left(nombre_columna.length() - 1);
          }
          
          result.record_header[col_numero].name = nombre_columna.utf8();
          break;
        }
        
      }
      //qWarning("CABECERA CARGADA");
      first = false;
      continue;
    } else { // valores ...



    //qWarning("PROCESANDO VALORES LINEA N? %d" , sz);
    // Creamos listado con valores
    sql_record rec;
    for (int i = 0; i < lista_valores.size(); i++) {  
      const std::string valor = lista_valores[i];
      field_value v;
      if (valor == NULL || valor == "|^N^|") {
          //Automáticamente marcaremos campo como null
          v.set_asString("");
          v.set_isNull(); 
        } else {
          v.set_asString(valor); // entra siempre como string ...
        }
       rec[i] = v;
 
      }

    result.records[posicion_idx] = rec;
    posicion_idx += 1;
    }

  }
  if (debug_sql) {
    qWarning("PAGINACIÓN: CURRENT:" + QString::number(result.records.size()));
  }
  return true;
  }

/*   bool SqliteDataset::seek(int pos)
  {
      //qWarning("??: %d , last: %d, invalid: %d" , pos, last_pos_fetched, last_invalid_pos);
      
      bloque_pos = resuelve_bloque(pos);

      if (ds_state == dsSelect) {
        if (last_invalid_pos == 0 || (pos < last_invalid_pos || pos > last_invalid_pos + 120)) { 

          
          bool found = result.records.count(pos) == 1;

          
          if (!found) {
              last_pos_fetched = pos; 
              bloque_last = bloque_pos;
              //qWarning(" >> %d", pos);
              found = fetch_rows(pos);
              if (pos != last_pos_fetched) {
                  int bloque_f = resuelve_bloque(pos);
                  qWarning(" - Nuevo invalid pos: %d (bloque %d), valid: %d (bloque %d)", pos, bloque_f, last_pos_fetched, bloque_last);
                  last_invalid_pos = pos;
                  found = false;
              } 
          }
          if (found) {   
            int seek_pos = pos;
            int bloque_pos_now = resuelve_bloque(pos);
            if (bloque_pos_now == bloque_last) {
              //qWarning("OK! %d", seek_pos);
              Dataset::seek(seek_pos);
              fill_fields();
              return true;
            }  
            qWarning(" - Descartada por bloque %d (bloque: %d), last: %d (bloque: %d)", pos, bloque_pos_now, last_pos_fetched, bloque_last);
          } else {
            qWarning(" - No se encuentra pos %d", pos);
          }
        } else {
          qWarning(" -  %d (bloque: %d) Descartada por cercania a %d, last: %d (bloque: %d)", pos, last_invalid_pos, bloque_pos, last_pos_fetched, bloque_last);
        } // ds_state == dsSelect
      }

    return false;
  }   */

  bool SqliteDataset::seek(int pos)
  {
    if (ds_state == dsSelect) {
      if (result.records.count(pos) == 1 || fetch_rows(pos)) {   
          Dataset::seek(pos);
          fill_fields();
          return true;
      }     
    } 
    return false;
  }  

  long SqliteDataset::nextid(const char *seq_name)
  {
    if (handle()) return db->nextid(seq_name);
    else return DB_UNEXPECTED_RESULT;
  }

}//namespace
