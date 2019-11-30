//#include <cstring>

#include "variable_resolvers.h"

#include "../Marlin/src/module/temperature.h"
#include "../Common/marlin_vars.h"
#include "../Common/marlin_client.h"

static char tempBuffer[256] = { '\0' };

typedef float (*resolv_fn)();
//typedef const void* resolv_fn();
//typedef const void* (resolv_fn)();
typedef const void (*TFuncPtr)();

//FIXME volani resolveru zrejme probiha na jinem vlakne nez kde se inicializuje lwsapi, takze si uchovame marlinVars pro pozdejsi pouziti
marlin_vars_t* MarlinVarsForResolver;

enum EConversion
{
	EC_String,
	EC_Float,
	EC_Int,
};

struct VariableItem
{
	const char* variable;
	EConversion convType;
	TFuncPtr funcPtr;
};

/*******************/

const char *ConvertFloat(float (*fce)()) {
	float v = fce();
	snprintf(tempBuffer, sizeof(tempBuffer), "%f", v);
	return tempBuffer;
}

const char *ConvertInt(int (*fce)()) {
	int v = fce();
	snprintf(tempBuffer, sizeof(tempBuffer), "%i", v);
	return tempBuffer;
}

/*******************/

float test_float()
{
	return 123.45f;
}

int test_int()
{
	return 4321;
}

const char* test_string()
{
	return "testResult";
}

/********************/

float actual_nozzle()
{
	return thermalManager.degHotend(0);
}

float target_nozzle()
{
	return thermalManager.degTargetHotend(0);
}

/*
    float actual_nozzle = thermalManager.degHotend(0);
	float target_nozzle = thermalManager.degTargetHotend(0);
	float actual_heatbed = thermalManager.degBed();
	float target_heatbed = thermalManager.degTargetBed();
*/

VariableItem KnownVariables[] = {
		{ "test_float", EC_Float, (TFuncPtr)test_float },
		{ "test_int", EC_Int, (TFuncPtr)test_int },
		{ "test_string", EC_String, (TFuncPtr)test_string },

		{ "actual_nozzle", EC_Float, (TFuncPtr)actual_nozzle /*(TFuncPtr)([]()->float { return thermalManager.degHotend(0); })*/ },
		{ "target_nozzle", EC_Float, (TFuncPtr)target_nozzle },
		{ "actual_heatbed", EC_Float, (TFuncPtr)thermalManager.degBed },
		{ "target_heatbed", EC_Float, (TFuncPtr)thermalManager.degTargetBed },
};

const char *getVariableValue(const char *variableNameStart, int nameLen)
{
	if (nameLen <= 1)
		return "$";

	variableNameStart++;	//uvodni $
	nameLen--;

	strncpy(tempBuffer, variableNameStart, nameLen);
	tempBuffer[nameLen] = 0;

	int id = marlin_vars_get_id_by_name(tempBuffer);
	if(id >= 0)
	{
		//marlin_vars_value_to_str(marlin_vars(), id, tempBuffer);
		marlin_vars_value_to_str(MarlinVarsForResolver, id, tempBuffer);
		return tempBuffer;
	}

	int cnt = sizeof(KnownVariables) / sizeof(VariableItem);
	for (int i = 0; i < cnt; i++)
	{
		if (!strncmp(variableNameStart, KnownVariables[i].variable, nameLen))
		{
			switch (KnownVariables[i].convType)
			{
			case EC_String:
				return ((const char*(*)())KnownVariables[i].funcPtr)();
			case EC_Float:
				return ConvertFloat((float(*)())KnownVariables[i].funcPtr);
			case EC_Int:
				return ConvertInt((int(*)())KnownVariables[i].funcPtr);
			}
		}
	}

	return "\"unknown\"";
}
