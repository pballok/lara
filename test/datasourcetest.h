#ifndef DATASOURCETEST_H
#define DATASOURCETEST_H

#include "unittest.h"

class cDataSourceTest : public cUnitTest
{
public:
    cDataSourceTest()                 throw();
    virtual ~cDataSourceTest()        throw();

    virtual void run()                throw();

private:
};

#endif // DATASOURCETEST_H
