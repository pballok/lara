#include "action.h"
#include "qtframework.h"

cAction::cAction( const QString &p_qsName, const QString &p_qsTimeStamp,
                  const unsigned int p_uiFileId, const unsigned long p_ulLineNum,
                  const cActionResult::teResult p_enResult, const cActionUpload::teUpload p_enUpload )
{
    cTracer  obTracer( "cLogAnalyser::identifySingleLinerActions", p_qsName.toStdString() );

    m_qsName      = p_qsName;
    m_qsTimeStamp = p_qsTimeStamp;
    m_uiFileId    = p_uiFileId;
    m_ulLineNum   = p_ulLineNum;
    m_enResult    = p_enResult;
    m_enUpload    = p_enUpload;
}

cAction::~cAction()
{
}

QString cAction::name() const throw()
{
    return m_qsName;
}

QString cAction::timeStamp() const throw()
{
    return m_qsTimeStamp;
}

unsigned int cAction::fileId() const throw()
{
    return m_uiFileId;
}

unsigned long cAction::lineNum() const throw()
{
    return m_ulLineNum;
}

cActionResult::teResult cAction::result() const throw()
{
    return m_enResult;
}

cActionUpload::teUpload cAction::upload() const throw()
{
    return m_enUpload;
}