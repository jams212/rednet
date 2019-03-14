
#include "uframework.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

using namespace rednet;
int main(int argc, char const *argv[])
{
    const char *filename = NULL;
    if (argc >= 2)
        filename = argv[1];
    INST(uframework, startup, filename);
    return 0;
}
