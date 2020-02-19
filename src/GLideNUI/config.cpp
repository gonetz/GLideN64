#pragma once
#include "wtl.h"
#include "config.h"

class GlideN64WtlModule :
    public CAppModule
{
public:
    GlideN64WtlModule(HINSTANCE hinst)
    {
        Init(NULL, hinst);
    }
    virtual ~GlideN64WtlModule(void)
    {
        Term();
    }
};

GlideN64WtlModule * WtlModule = NULL;

void ConfigInit(void * hinst)
{
    WtlModule = new GlideN64WtlModule((HINSTANCE)hinst);
}

void ConfigCleanup(void)
{
    if (WtlModule)
    {
        delete WtlModule;
        WtlModule = NULL;
    }
}
