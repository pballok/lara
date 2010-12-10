#include <QDir>
#include <QFile>
#include <QDateTime>
#include <set>

#include "lara.h"
#include "outputcreator.h"

using namespace std;

cOutputCreator::cOutputCreator( const QString &p_qsDirPrefix )
{
    cTracer  obTracer( &g_obLogger, "cOutputCreator::cOutputCreator" );

    m_poDB = new cQTMySQLConnection( &g_obLogger );
    m_poDB->setHostName( g_poPrefs->dbHost() );
    m_poDB->setDatabaseName( g_poPrefs->dbSchema() );
    m_poDB->setUserName( g_poPrefs->dbUser() );
    m_poDB->setPassword( g_poPrefs->dbPassword() );
    try
    {
        m_poDB->open();
    } catch( cSevException &e )
    {
        g_obLogger << e;
        g_obLogger << cSeverity::WARNING << "Could not open DataBase. Result upload is disabled." << cLogMessage::EOM;
    }

    m_qsOutDir = QDir::cleanPath( g_poPrefs->outputDir() + "/" + p_qsDirPrefix );
}

cOutputCreator::~cOutputCreator()
{
    cTracer  obTracer( &g_obLogger, "cOutputCreator::~cOutputCreator" );

    for( tiActionCountList itActionCount = m_maActionCounts.begin(); itActionCount != m_maActionCounts.end(); itActionCount++ )
    {
        delete itActionCount->second;
    }

    delete m_poDB;
}

unsigned int cOutputCreator::fileId( const QString & p_qsFileName ) throw( cSevException )
{
    cTracer  obTracer( &g_obLogger, "cOutputCreator::fileId( " + p_qsFileName.toStdString() + " )" );

    int inIndex = m_slInputFiles.indexOf( p_qsFileName );
    if( inIndex == -1 )
    {
        m_slInputFiles.append( p_qsFileName );
        inIndex = m_slInputFiles.size() - 1;
    }

    obTracer << inIndex;

    return inIndex;
}

void cOutputCreator::addAction( const cAction *m_poAction ) throw( cSevException )
{
    m_mmActionList.insert( pair<QString, cAction>(m_poAction->name(), *m_poAction ) );
}

void cOutputCreator::addCountAction( const QString &p_qsCountName,
                                     const unsigned long p_ulOk,
                                     const unsigned long p_ulFailed ) throw()
{
    cTracer  obTracer( &g_obLogger, "cOutputCreator::addCountAction",
                       QString( "CountName: \"%1\" Ok: %2 Failed: %3" ).arg( p_qsCountName ).arg( p_ulOk ).arg( p_ulFailed ).toStdString() );

    tsActionResCount *poResCount = NULL;

    tiActionCountList  itActionCount = m_maActionCounts.find( p_qsCountName );
    if( itActionCount == m_maActionCounts.end() )
    {
        poResCount = new tsActionResCount;
        poResCount->ulFailed = 0;
        poResCount->ulOk = 0;
        m_maActionCounts.insert( pair<QString, tsActionResCount*>( p_qsCountName, poResCount ) );
    }
    else
    {
        poResCount = itActionCount->second;
    }

    poResCount->ulOk += p_ulOk;
    poResCount->ulFailed += p_ulFailed;

    obTracer << QString( "Total Ok: %1 Total Failed: %2" ).arg( poResCount->ulOk ).arg( poResCount->ulFailed ).toStdString();
}

void cOutputCreator::addAttribute( const QString &p_qsName, const QString &p_qsValue ) throw()
{
    cTracer  obTracer( &g_obLogger, "cOutputCreator::addAttribute",
                       QString( "Name: \"%1\" Value: \"%2\"" ).arg( p_qsName ).arg( p_qsValue ).toStdString() );

    m_maAttributes.insert( pair<QString, QString>( p_qsName, p_qsValue ) );
}

void cOutputCreator::generateActionSummary() const throw( cSevException )
{
    cTracer  obTracer( &g_obLogger, "cOutputCreator::generateActionSummary" );

    QDir obDir;
    if( !obDir.exists( m_qsOutDir ) ) obDir.mkpath( m_qsOutDir );

    QString qsFileName = m_qsOutDir + "/actionsummary.txt";
    QFile   obActionSummaryFile( qsFileName );
    if( !obActionSummaryFile.open( QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text ) )
        throw cSevException( cSeverity::ERROR, QString( "%1: %2" ).arg( qsFileName ).arg( obActionSummaryFile.errorString() ).toStdString() );

    obActionSummaryFile.write( "This is an Action Summary file generated by " + g_poPrefs->appName().toAscii() );
    obActionSummaryFile.write( " version " + g_poPrefs->version().toAscii() + "\n" );

    obActionSummaryFile.write( "Generation time: " );
    obActionSummaryFile.write( QDateTime::currentDateTime().toString( "dd-MMM-yyyy hh:mm:ss" ).toAscii() + "\n\n" );

    obActionSummaryFile.write( "Input files:\n" );
    for( int i = 0; i < m_slInputFiles.size(); i++ )
    {
        obActionSummaryFile.write( m_slInputFiles.at( i ).toAscii() + "\n" );
    }
    obActionSummaryFile.write( "\n" );

    for( tiAttributes itAttrib = m_maAttributes.begin();
         itAttrib != m_maAttributes.end();
         itAttrib++ )
    {
        obActionSummaryFile.write( QString( "%1: %2\n" ).arg( itAttrib->first ).arg( itAttrib->second ).toAscii() );
    }
    obActionSummaryFile.write( "\n" );

    obActionSummaryFile.write( "Action Counts:\n" );
    for( tiActionCountList itAction = m_maActionCounts.begin(); itAction != m_maActionCounts.end(); itAction++ )
    {
        obActionSummaryFile.write( itAction->first.toAscii() );
        obActionSummaryFile.write( " OK: " + QString::number( itAction->second->ulOk ).toAscii() );
        obActionSummaryFile.write( " FAILED: " + QString::number( itAction->second->ulFailed ).toAscii() );
        obActionSummaryFile.write( " TOTAL: " + QString::number( itAction->second->ulOk + itAction->second->ulFailed ).toAscii() );
        obActionSummaryFile.write( "\n" );
    }

    obActionSummaryFile.flush();
    obActionSummaryFile.close();
}

void cOutputCreator::uploadActionSummary() throw( cSevException )
{
    cTracer  obTracer( &g_obLogger, "cOutputCreator::uploadActionSummary" );

    if( !m_poDB->isOpen() ) return;

    QStringList slColumns = m_poDB->columnList( "cyclerconfigs" );
    if( slColumns.empty() ) throw cSevException( cSeverity::ERROR, "DataBase: \"cyclerconfigs\" table does not exist" );

    tiAttributes itAttrib = m_maAttributes.find( "cellName" );
    if( itAttrib == m_maAttributes.end() ) throw cSevException( cSeverity::ERROR, "Could not find \"cellName\" attribute" );
    QString qsCellName = itAttrib->second;

    itAttrib = m_maAttributes.find( "startDate" );
    if( itAttrib == m_maAttributes.end() ) throw cSevException( cSeverity::ERROR, "Could not find \"startDate\" attribute" );
    QString qsStartDate = itAttrib->second;

    itAttrib = m_maAttributes.find( "endDate" );
    if( itAttrib == m_maAttributes.end() ) throw cSevException( cSeverity::ERROR, "Could not find \"endDate\" attribute" );
    QString qsEndDate = itAttrib->second;

    itAttrib = m_maAttributes.find( "examName" );
    if( itAttrib == m_maAttributes.end() ) throw cSevException( cSeverity::ERROR, "Could not find \"examName\" attribute" );

    QString qsQuery = "SELECT cyclerconfigId FROM cyclerconfigs WHERE cellName =\"";
    qsQuery += qsCellName;
    qsQuery += "\" AND startDate=\"";
    qsQuery += qsStartDate;
    qsQuery += "\" AND endDate=\"";
    qsQuery += qsEndDate;
    qsQuery += "\"";
    QSqlQuery *poQueryRes = m_poDB->executeQTQuery( qsQuery );
    if( poQueryRes->first() )
    {
        QString qsError = QString( "Analysis result already exists as cyclerconfigId=%1" ).arg( poQueryRes->value( 0 ).toInt() );
        delete poQueryRes;
        throw cSevException( cSeverity::ERROR, qsError.toStdString() );
    }
    delete poQueryRes;

    qsQuery = "INSERT INTO cyclerconfigs SET ";
    bool boFieldPresent = false;
    for( itAttrib = m_maAttributes.begin();
         itAttrib != m_maAttributes.end();
         itAttrib++ )
    {
        if( !slColumns.contains( itAttrib->first ) ) continue;
        if( boFieldPresent ) qsQuery += ", ";
        else boFieldPresent = true;
        qsQuery += itAttrib->first;
        qsQuery += "=\"";
        qsQuery += itAttrib->second;
        qsQuery += "\"";
    }
    for( tiActionCountList itAction = m_maActionCounts.begin(); itAction != m_maActionCounts.end(); itAction++ )
    {
        if( !slColumns.contains( itAction->first ) ) continue;
        if( boFieldPresent ) qsQuery += ", ";
        else boFieldPresent = true;
        qsQuery += QString( "%1=\"%2\"" ).arg( itAction->first ).arg( itAction->second->ulOk + itAction->second->ulFailed );
    }

    poQueryRes = m_poDB->executeQTQuery( qsQuery );
    m_ulBatchId = poQueryRes->lastInsertId().toULongLong();
    delete poQueryRes;
}

void cOutputCreator::generateActionList() const throw( cSevException )
{
    cTracer  obTracer( &g_obLogger, "cOutputCreator::generateActionList" );

    QDir obDir;
    if( !obDir.exists( m_qsOutDir ) ) obDir.mkpath( m_qsOutDir );

    QString qsFileName = m_qsOutDir + "/actionlist.txt";
    QFile   obActionListFile( qsFileName );
    if( !obActionListFile.open( QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text ) )
        throw cSevException( cSeverity::ERROR, QString( "%1: %2" ).arg( qsFileName ).arg( obActionListFile.errorString() ).toStdString() );

    obActionListFile.write( "This is an Action List file generated by " + g_poPrefs->appName().toAscii() );
    obActionListFile.write( " version " + g_poPrefs->version().toAscii() + "\n" );

    obActionListFile.write( "Generation time: " );
    obActionListFile.write( QDateTime::currentDateTime().toString( "dd-MMM-yyyy hh:mm:ss" ).toAscii() + "\n\n" );

    obActionListFile.write( "Input files:\n" );
    for( int i = 0; i < m_slInputFiles.size(); i++ )
    {
        obActionListFile.write( m_slInputFiles.at( i ).toAscii() + "\n" );
    }
    obActionListFile.write( "\n" );

    obActionListFile.write( "Identified Actions:\n" );
    for( tiActionList itAction = m_mmActionList.begin(); itAction != m_mmActionList.end(); itAction++ )
    {
        obActionListFile.write( itAction->second.timeStamp().toAscii() + " " );
        obActionListFile.write( itAction->second.name().toAscii() + " " );
        obActionListFile.write( cActionResult::toStr( itAction->second.result() ) );
        for( tiActionAttribs itAttrib = itAction->second.attributesBegin();
             itAttrib != itAction->second.attributesEnd();
             itAttrib++ )
        {
            obActionListFile.write( " " + itAttrib->first.toAscii() + "=\"" + itAttrib->second.toAscii() + "\"" );
        }
        obActionListFile.write( " " + m_slInputFiles.at( itAction->second.fileId() ).toAscii() + ":" );
        obActionListFile.write( QString::number( itAction->second.lineNum() ).toAscii() + "\n" );
    }

    obActionListFile.flush();
    obActionListFile.close();
}

void cOutputCreator::uploadActionList() const throw( cSevException )
{
    cTracer  obTracer( &g_obLogger, "cOutputCreator::uploadActionList" );

    if( !m_poDB->isOpen() ) return;

    QStringList slColumns = m_poDB->columnList( "occurrences" );
    if( slColumns.empty() ) throw cSevException( cSeverity::ERROR, "DataBase: \"occurrences\" table does not exist" );

    for( tiActionList itAction = m_mmActionList.begin(); itAction != m_mmActionList.end(); itAction++ )
    {
        if( itAction->second.upload() == cActionUpload::NEVER ) continue;
        if( itAction->second.upload() == cActionUpload::FAILED && itAction->second.result() != cActionResult::FAILED ) continue;
        if( itAction->second.upload() == cActionUpload::OK && itAction->second.result() != cActionResult::OK ) continue;

        QString qsQuery = QString( "INSERT INTO occurrences SET cyclerconfigId=%1, " ).arg( m_ulBatchId );
        cAction::tsTimeStamp  suTimeStamp = itAction->second.timeStampStruct();
        qsQuery += QString( "created=\"%1-%2-%3 %4:%5:%6\"" ).arg( suTimeStamp.uiYear ).arg( suTimeStamp.uiMonth ).arg( suTimeStamp.uiDay ).arg( suTimeStamp.uiHour ).arg( suTimeStamp.uiMinute ).arg( suTimeStamp.uiSecond );
        for( tiActionAttribs itAttrib = itAction->second.attributesBegin();
             itAttrib != itAction->second.attributesBegin();
             itAttrib++ )
        {
            if( !slColumns.contains( itAttrib->first ) ) continue;
            qsQuery += ", ";
            qsQuery += itAttrib->first;
            qsQuery += "=\"";
            qsQuery += itAttrib->second;
            qsQuery += "\"";
        }

        m_poDB->executeQuery( qsQuery );
    }
}
