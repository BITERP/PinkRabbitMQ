#include <cstdio>
#include "src/RabbitMQClient.h"
#include "src/AddInNative.h"
#include "src/ConversionWchar.h"
#include "src/Utils.h"

int main()
{
	AddInNative native;
	tVariant* params = new tVariant[6];
   
	params[0].pstrVal = "devdevopsrmq.bit-erp.loc";
	params[1].uintVal = 5672;
	params[2].pstrVal = "rkudakov_devops";
	params[3].pstrVal = "rkudakov_devops";
	params[4].pstrVal = "rkudakov_devops";
	
    native.CallAsProc(AddInNative::Methods::eMethConnect, params, sizeof(params));

    return 0;
}