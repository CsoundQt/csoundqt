/*
    Copyright (C) 2008-2016 Andres Cabrera
    mantaraya36@gmail.com

    This file is part of CsoundQt.

    CsoundQt is free software; you can redistribute it
    and/or modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    CsoundQt is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with Csound; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
*/


// code based partly on CsoundHtmlWrapper by Michael Gogins https://github.com/gogins/gogins.github.io/tree/master/csound_html5

#include "csoundhtmlwrapper.h"
#include <QApplication>
#include <QDebug>
#include <QFile>

CsoundHtmlWrapper::CsoundHtmlWrapper(QObject *parent) :
    QObject(parent),
    csound_stop(true),
    csound_finished(true),
    csound(nullptr),
    m_csoundEngine(nullptr),
    csound_thread(nullptr),
    message_callback(nullptr)
{
}

void CsoundHtmlWrapper::setCsoundEngine(CsoundEngine *csEngine, CsoundOptions *options)
{
    m_csoundEngine = csEngine;
	if (m_csoundEngine) {
        csound = m_csoundEngine->getCsound();
        m_options = options;
        qDebug()<<"options for file: "<<m_options->fileName1;
    }
}

int CsoundHtmlWrapper::compileCsd(const QString &filename) {
    if (checkCsound()) {
        return -1;
    }
#if CS_APIVERSION>=4
    int result = csoundCompileCsd(csound, filename.toLocal8Bit()); // also this returns 0 even if there are errors in csd
    if (result!=CSOUND_SUCCESS) {
        qDebug()<<"Failed to compile.";
        m_csoundEngine->stop();
    }
    return result;
#else
    return csoundCompileCsd(csound, filename.toLocal8Bit().data());
#endif
}

int CsoundHtmlWrapper::compileCsdText(const QString &text) {

	if (checkCsound()) {
		return -1;
	}
	int result = -1;
	int version = csoundGetVersion();
	if (csoundGetVersion()<=6091 )  { // to work around bug in 6.09.1 where csoundCompileCsdText was always 1:

		QString newName = "/tmp/temp-html-csd.csd";//m_options->fileName1+"-temp.csd"; // TODO: handle :/example beginning
		QFile tempCsd(newName);
		if (!tempCsd.open(QIODevice::WriteOnly| QIODevice::Text)) {
			qDebug()<<"Csould not open file";
			return -1;
		}
		tempCsd.write(text.toLocal8Bit());
		tempCsd.close();
		m_csoundEngine->prepareCsound(m_options);

		result = m_csoundEngine->compileCsd(newName);
	} else {
		result = csoundCompileCsdText(csound, text.toLocal8Bit());
		if (result!=CSOUND_SUCCESS) { // later separate function in CsoundEngine compilationFailed()
			qDebug()  << "Csound compile failed! "  << result;
			m_csoundEngine->stop();
			//emit (errorLines(getErrorLines()));
		}
	}
	return result;
}

int CsoundHtmlWrapper::compileOrc(const QString &text) {
    if (checkCsound()) {
		return -1;
	}
    return csoundCompileOrc(csound, text.toLocal8Bit());
}

double CsoundHtmlWrapper::evalCode(const QString &text) {
    if (checkCsound()) {
		return -1;
	}
    return csoundEvalCode(csound, text.toLocal8Bit());
}

double CsoundHtmlWrapper::get0dBFS() {
    if (!csound) {
        return -1;
    }
    return csoundGet0dBFS(csound); //cs->Get0dBFS();
}

int CsoundHtmlWrapper::getApiVersion() {
    if (!csound) {
        return -1;
    }
    return csoundGetAPIVersion();
}

double CsoundHtmlWrapper::getControlChannel(const QString &name) {
    if (!csound) {
        return -1;
    }
    int result = 0;
    double value = csoundGetControlChannel(csound, name.toLocal8Bit(), &result);
    return value;
}

qint64 CsoundHtmlWrapper::getCurrentTimeSamples() { // FIXME: unknown type int64_t qint64
    if (!csound) {
        return -1;
    }
    return csoundGetCurrentTimeSamples(csound);
}

QString CsoundHtmlWrapper::getEnv(const QString &name) { // not sure, if it works... test with setGlobalEnv
    if (!csound) {
        return QString();
    }
    return csoundGetEnv(csound,name.toLocal8Bit());
}

int CsoundHtmlWrapper::getKsmps() {
    if (!csound) {
        return -1;
    }
    return csoundGetKsmps(csound);
}

int CsoundHtmlWrapper::getNchnls() {
    if (!csound) {
        return -1;
    }
    return csoundGetNchnls(csound);
}

int CsoundHtmlWrapper::getNchnlsInput() {
    if (!csound) {
        return -1;
    }
    return csoundGetNchnlsInput(csound);
}

QString CsoundHtmlWrapper::getOutputName() {
    if (!csound) {
        return QString();
    }
    return QString(csoundGetOutputName(csound));
}

double CsoundHtmlWrapper::getScoreOffsetSeconds() {
    if (!csound) {
        return -1;
    }
    return csoundGetScoreOffsetSeconds(csound);
}

double CsoundHtmlWrapper::getScoreTime() {
    if (!csound) {
        return -1;
    }
    return csoundGetScoreTime(csound);
}

int CsoundHtmlWrapper::getSr() {
    if (!csound) {
        return -1;
    }
    return csoundGetSr(csound);
}

QString CsoundHtmlWrapper::getStringChannel(const QString &name) {
    if (!csound) {
        return QString();
    }
    char buffer[0x100];
    csoundGetStringChannel(csound,name.toLocal8Bit(), buffer);
    return QString(buffer);
}

int CsoundHtmlWrapper::getVersion() {
    return csoundGetVersion();
}

bool CsoundHtmlWrapper::isPlaying() {
    if (!m_csoundEngine)
        return false;
    else
        return m_csoundEngine->isRunning();
}

int CsoundHtmlWrapper::isScorePending() {
    if (!csound) {
        return -1;
    }
    return csoundIsScorePending(csound);
}

void CsoundHtmlWrapper::message(const QString &text) {
    if (!csound) {
        return;
    }
    csoundMessage(csound, "%s", text.toLocal8Bit().constData());
}

int CsoundHtmlWrapper::perform() {
    int result = -1;
    if (checkCsound()) {
        return result;
	}
    stop();
    if (m_csoundEngine) {
        result = m_csoundEngine->startPerformanceThread();
    }
    return result;
}

int CsoundHtmlWrapper::perform_thread_routine() {
    qDebug() ;
    int result = 0;
    result = csoundStart(csound);
    message("Csound has started running...");
    for (csound_stop = false, csound_finished = false;
         ((csound_stop == false) && (csound_finished == false) && (csound != nullptr)); )
    {
        csound_finished = csoundPerformKsmps(csound);
    }
    message("Csound has stopped running.");
    // Although the thread has been started by the CsoundHtmlWrapper,
    // the cleanup should be done by the CsoundEngine.
    // result = csoundCleanup(csound);
    // if (result) {
    //     message("Failed to clean up Csound performance.");
	// }
    csoundReset(csound);
    return result;
}


int CsoundHtmlWrapper::readScore(const QString &text) {
    if (!csound) {
		return -1;
	}  
    qDebug();
    return csoundReadScore(csound, text.toLocal8Bit());
}

void CsoundHtmlWrapper::reset() {
    if (!m_csoundEngine) {
        return;
    }
    // maybe even better:
    //m_csoundEngine->stop(); // calls cleanup and everything needed ?
    m_csoundEngine->cleanupCsound();
    // in this point m_CsoundEngine can be already 0...
    //this seems to fail: csoundReset(m_csoundEngine->getCsound());
}

void CsoundHtmlWrapper::rewindScore() {
    if (!csound) {
        return;
    }
    csoundRewindScore(csound);
}

int CsoundHtmlWrapper::runUtility(const QString &command, int argc, char **argv) {
    if (!csound) {
        return -1;
    }
    return csoundRunUtility(csound, command.toLocal8Bit(), argc, argv); // probably does not work from JS due char **
}

int CsoundHtmlWrapper::scoreEvent(char type, const double *pFields, long numFields) {
    if (!csound) {
        return -1;
    }
    return csoundScoreEvent(csound,type, pFields, numFields);
}

void CsoundHtmlWrapper::setControlChannel(const QString &name, double value) {
    if (!csound) {
        return;
    }
    csoundSetControlChannel(csound,name.toLocal8Bit(), value);
}

int CsoundHtmlWrapper::setGlobalEnv(const QString &name, const QString &value) {
    if (!csound) {
        return -1;
    }
    return csoundSetGlobalEnv(name.toLocal8Bit(), value.toLocal8Bit());
}

void CsoundHtmlWrapper::setInput(const QString &name){
    if (!csound) {
        return;
    }
#if CS_APIVERSION>=4
    csoundSetInput(csound, name.toLocal8Bit());
#else
    csoundSetInput(csound, name.toLocal8Bit().data());
#endif
}

void CsoundHtmlWrapper::setMessageCallback(QObject *callback){
    qDebug();
    callback->dumpObjectInfo();
}

int CsoundHtmlWrapper::setOption(const QString &name){
    if (!csound) {
        return -1;
    }
#if CS_APIVERSION>=4
    return csoundSetOption(csound, name.toLocal8Bit());
#else
    return csoundSetOption(csound, name.toLocal8Bit().data());
#endif
}

void CsoundHtmlWrapper::setOutput(const QString &name, const QString &type, const QString &format){
    if (!csound) {
        return;
    }
#if CS_APIVERSION>=4
    csoundSetOutput(csound, name.toLocal8Bit(), type.toLocal8Bit(), format.toLocal8Bit());
#else
    csoundSetOutput(csound, name.toLocal8Bit().data(), type.toLocal8Bit().data(), format.toLocal8Bit().data());
#endif
}

void CsoundHtmlWrapper::setScoreOffsetSeconds(double value){
    if (!csound) {
        return;
    }
    csoundSetScoreOffsetSeconds(csound, value);
}

void CsoundHtmlWrapper::setScorePending(bool value){
    if (!csound) {
        return;
    }
    csoundSetScorePending(csound,(int) value);
}

void CsoundHtmlWrapper::setStringChannel(const QString &name, const QString &value){
    if (!csound) {
        return;
    }
    csoundSetStringChannel(csound,  name.toLocal8Bit(), value.toLocal8Bit().data());
}

int CsoundHtmlWrapper::start()
{
    ///if (checkCsound()) { // TODO: how to avoid "Csound already started" problem?
    ///    return -1;
    ///}
    if (!csound) {
        return -1;
    }

    return csoundStart(csound);
}

void CsoundHtmlWrapper::stop(){
    if (!m_csoundEngine) {
        return;
    }
    //qDebug();
    m_csoundEngine->stop();
}

double CsoundHtmlWrapper::tableGet(int table_number, int index){
    if (!csound) {
        return -1;
    }
    return csoundTableGet(csound, table_number, index);
}

int CsoundHtmlWrapper::tableLength(int table_number){
    if (!csound) {
        return -1;
    }
    return csoundTableLength(csound, table_number);
}

void CsoundHtmlWrapper::tableSet(int table_number, int index, double value){
    if (!csound) {
        return;
    }
	csoundTableSet(csound, table_number, index, value);
}

int CsoundHtmlWrapper::checkCsound() // checks if CsoundEngine and CsoundOptions are set and if csound is 0 in CsoundEngine, creates it (call prepareCsound to get it ready for performance).
// call this function from compileCsdText, compileOrc and other functions that can be called from html-only file before perform().
{
	if (!m_csoundEngine) {
		qDebug()<<"CsoundEngine not available for CsoundHtmlWrapper!";
		return -2;
	}
	if (!m_options) {
		qDebug()<<"CsoundOptions not set for CsoundHtmlWrapper!";
		return -3;
	}
	if (!m_csoundEngine->getCsound()) {
        qDebug()<<"Prepare Csound for compilation...";
        qDebug()<<"Current filename is: "<<m_options->fileName1;
		int ret =m_csoundEngine->prepareCsound(m_options); // coundCreate() is executed here
		csound = m_csoundEngine->getCsound();
		return ret;
	}
    return 0; // if it arrives here, report success
}



