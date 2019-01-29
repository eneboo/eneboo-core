/***************************************************************************
                                  ebcomportamiento.qs
                            -------------------
   begin                : 01 mar 2013
   copyright            : (C) 2013 by Aulla Sistemas
   email                : aullasistemas@gmail.com
***************************************************************************/
/***************************************************************************
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; version 2 of the License.               *
 ***************************************************************************/
/***************************************************************************
   Este  programa es software libre. Puede redistribuirlo y/o modificarlo
   bajo  los  términos  de  la  Licencia  Pública General de GNU   en  su
   versión 2, publicada  por  la  Free  Software Foundation.
 ***************************************************************************/

function main() {
	var mng = aqApp.db().managerModules();
	this.w_ = mng.createUI("ebcomportamiento.ui");
	var w = this.w_;
	var botonAceptar = w.child("pbnAceptar");
	var botonCancelar = w.child("pbnCancelar");
	var botonCambiarColor = w.child("pbnCO");
	connect(botonAceptar, "clicked()", this, "guardar_clicked");
	connect(botonCancelar, "clicked()", this, "cerrar_clicked");
	connect(botonCambiarColor, "clicked()", this, "seleccionarColor_clicked");
	cargarConfiguracion();
	this.initEventFilter();
	w_.show();
}

function cargarConfiguracion() {
	var w = this.w_;
	w.child("leNombreVertical").text = leerValorGlobal("verticalName");
	w.child("cbFLTableDC").checked = leerValorLocal("FLTableDoubleClick");
	w.child("cbFLTableSC").checked = leerValorLocal("FLTableShortCut");
	w.child("cbFLTableCalc").checked = leerValorLocal("FLTableExport2Calc");
	w.child("cbDebuggerMode").checked = leerValorLocal("isDebuggerMode");
	w.child("cbSLConsola").checked = leerValorLocal("SLConsola");
	w.child("cbSLInterface").checked = leerValorLocal("SLInterface");
	w.child("leCallFunction").text = leerValorLocal("ebCallFunction");
	w.child("leMaxPixImages").text = leerValorLocal("maxPixImages");
	w.child("cbFLLarge").checked = leerValorGlobal("FLLargeMode");
	w.child("cbPosInfo").checked = leerValorGlobal("PosInfo");
	w.child("cb_snapshot").checked = leerValorLocal("show_snapshot_button");
	w.child("leCO").hide();
	if (leerValorLocal("colorObligatorio") == "")
		w.child("leCO").paletteBackgroundColor = "#FFE9AD";
	else
		w.child("leCO").paletteBackgroundColor = leerValorLocal("colorObligatorio");
	w.child("leCO").show();
	w.child("cbActionsMenuRed").checked = leerValorLocal("ActionsMenuRed");
}

function leerValorGlobal(valorName):String {
	var util:FLUtil = new FLUtil();
	var valor:String = "";
	if (!util.sqlSelect("flsettings", "valor", "flkey='" + valorName + "'"))
		valor = "";
	else
		valor = util.sqlSelect("flsettings", "valor", "flkey='" + valorName + "'");
	return valor;
}

function grabarValorGlobal(valorName, value) {
	var util:FLUtil = new FLUtil();
	if (!util.sqlSelect("flsettings", "flkey", "flkey='" + valorName + "'"))
		util.sqlInsert("flsettings", "flkey,valor" , valorName + "," + value);
	else
		util.sqlUpdate("flsettings", "valor", value, "flkey = '" + valorName + "'");
}

function leerValorLocal(valor_name):String {
	var util:FLUtil = new FLUtil();
	var valor:String;
	var settings = new AQSettings;
	switch (valor_name) {
		case "isDebuggerMode":
			{
			valor = settings.readBoolEntry("application/" + valor_name );
			break;
			}
		case "SLInterface":
		case "SLConsola":
		case "FLLargeMode":
		case "PosInfo":
		case "FLTableDoubleClick":
		case "FLTableShortCut":
		case "FLTableExport2Calc":
		case "show_snapshot_button":
			{
			valor = settings.readBoolEntry("ebcomportamiento/" + valor_name );
			break;
			}			
		default:
			{
			valor = util.readSettingEntry("ebcomportamiento/" + valor_name, "");
			break;
			}
	}
	return valor;
}

function grabarValorLocal(valor_name, value) {
	if (valor_name == "maxPixImages" && value < 1 )
		value = 600;
	var settings = new AQSettings;
	switch (valor_name) {
		case "isDebuggerMode":
			settings.writeEntry("application/" + valor_name, value);
		break;
		default:
			settings.writeEntry("ebcomportamiento/" + valor_name, value);
		break;
	}
}

function initEventFilter() {
	var w = this.w_;

	w.eventFilterFunction = this.name + ".eventFilter";
	w.allowedEvents = [ AQS.Close ];
	w.installEventFilter(w);
}

function eventFilter(o, e) {
	switch (e.type) {
		case AQS.Close:
			this.cerrar_clicked();
		break;
	}
}

function cerrar_clicked() {
	this.w_.close();
}

function guardar_clicked() {
	var w = this.w_;
    			
	grabarValorGlobal("verticalName", w.child("leNombreVertical").text);
	grabarValorLocal("FLTableDoubleClick", w.child("cbFLTableDC").checked);
	grabarValorLocal("FLTableShortCut", w.child("cbFLTableSC").checked);
	grabarValorLocal("FLTableExport2Calc", w.child("cbFLTableCalc").checked);
	grabarValorLocal("isDebuggerMode", w.child("cbDebuggerMode").checked);
	grabarValorLocal("SLConsola", w.child("cbSLConsola").checked);
	grabarValorLocal("SLInterface", w.child("cbSLInterface").checked);
	grabarValorLocal("ebCallFunction", w.child("leCallFunction").text);
	grabarValorLocal("maxPixImages", w.child("leMaxPixImages").text);
	grabarValorLocal("colorObligatorio", w.child("leCO").paletteBackgroundColor + "");
	grabarValorLocal("ActionsMenuRed", w.child("cbActionsMenuRed").checked);
	grabarValorGlobal("FLLargeMode", w.child("cbFLLarge").checked);
	grabarValorGlobal("PosInfo", w.child("cbPosInfo").checked);
	grabarValorLocal("show_snapshot_button"), w.child("cb_snapshot").checked);
	cerrar_clicked();
}

function seleccionarColor_clicked() {
	var w = this.w_;
	const colorActual:QColor = w.child("leCO").paletteBackgroundColor;
	w.child("leCO").hide();
	w.child("leCO").paletteBackgroundColor = AQS.ColorDialog_getColor(colorActual);
	w_.hide();
	w_.show();
	if (w.child("leCO").paletteBackgroundColor == "#000000")
		w.child("leCO").paletteBackgroundColor = colorActual;
	w.child("leCO").show();
}

function fixPath(ruta:String):String
{
var rutaFixed:String;
    if (sys.osName() == "WIN32")
            {
           var barra = "\\";
        while (ruta != rutaFixed)
                    {
                    rutaFixed = ruta;
                    ruta = ruta.replace("/",barra);
                    }
        if (!rutaFixed.endsWith(barra)) rutaFixed +="\\";
            } else
                rutaFixed= ruta;
return rutaFixed;
}

