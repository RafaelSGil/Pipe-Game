#pragma once
#ifndef  REGISTRY
#include "Names.h"
typedef struct _Registry {
	HKEY key;
	TCHAR keyCompletePath[BUFFER];
	DWORD dposition;
	TCHAR name[BUFFER];
}Registry;

#endif // ! REGISTRY

