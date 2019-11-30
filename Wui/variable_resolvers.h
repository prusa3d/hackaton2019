#pragma once

#include "../Common/marlin_vars.h"

#ifdef __cplusplus
extern "C" {
#endif

extern marlin_vars_t* MarlinVarsForResolver;

const char *getVariableValue(const char *variableNameStart, int nameLen);

#ifdef __cplusplus
}
#endif
