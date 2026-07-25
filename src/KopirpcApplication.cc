#include "KopirpcApplication.h"

/*
* 
*/
void KopirpcApplication::Init(int argc, char **argv)
{

}

KopirpcApplication& KopirpcApplication::GetInstance()
{
    static KopirpcApplication app; 
    return app; 
}