#ifndef ACTION_H
#define ACTION_H

#include <QString>
#include <map>

#include "actiondef.h"

typedef std::map<QString,QString>        tmActionAttribs;
typedef tmActionAttribs::const_iterator  tiActionAttribs;

class cAction
{
public:

    typedef struct
    {
        unsigned int uiYear;
        unsigned int uiMonth;
        unsigned int uiDay;
        unsigned int uiHour;
        unsigned int uiMinute;
        unsigned int uiSecond;
        unsigned int uiMSecond;
    } tsTimeStamp;

    cAction( const QString &p_qsName = "", const QString &p_qsTimeStamp = "",
             const tsTimeStamp* p_poTimeStamp = NULL,
             const unsigned int p_uiFileId = 0, const unsigned long p_ulLineNum = 0,
             const cActionResult::teResult p_enResult = cActionResult::MIN,
             const cActionUpload::teUpload p_enUpload = cActionUpload::MIN );
    ~cAction();

    QString                  name() const throw();
    QString                  timeStamp() const throw();
    tsTimeStamp              timeStampStruct() const throw();
    unsigned int             fileId() const throw();
    unsigned long            lineNum() const throw();
    cActionResult::teResult  result() const throw();
    cActionUpload::teUpload  upload() const throw();
    void                     addAttribute( const QString &p_qsTextName, const QString &p_qsTextValue ) throw();
    tiActionAttribs          attributesBegin() const throw();
    tiActionAttribs          attributesEnd() const throw();
    QString                  attribute( const QString &p_qsAttribName ) const throw();

private:
    QString                  m_qsName;
    QString                  m_qsTimeStamp;
    tsTimeStamp              m_suTimeStamp;
    unsigned int             m_uiFileId;
    unsigned long            m_ulLineNum;
    cActionResult::teResult  m_enResult;
    cActionUpload::teUpload  m_enUpload;
    tmActionAttribs          m_maAttribs;
};

typedef std::multimap<QString, cAction>  tmActionList;
typedef tmActionList::const_iterator     tiActionList;

#endif // ACTION_H
