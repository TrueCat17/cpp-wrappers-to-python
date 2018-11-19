# cpp-wrappers-to-python
Simple C++17 wrappers (makeFunc) to Python2 with param-checks

Usage:

```C++
#include "py_utils/make_func.h"

long ftoi(double d) {
	return d;
}

void initPython() {
	PyObject *pyFunc;
	
	Py_Initialize();
	
	
	PyObject *main = PyImport_AddModule("__main__");
	PyObject *global = PyModule_GetDict(main);
	
	
	pyFunc = makePyFunc(ftoi);//using
	PyDict_SetItemString(global, "ftoi", pyFunc);
	Py_DECREF(pyFunc);
	
	//...
}
```

See extra tests and examples at src/main.cpp

