#include <iostream>
#include <chrono>

#include "py_utils/make_func.h"


/*
 * Python::float is C::double
 * Python::int   is C::long
 * Python::long  is other python-type for BigInt
*/



//PURE C-API
PyObject* floatToInt1(PyObject*, PyObject *args) {
	if (Py_SIZE(args) != 1) {
		std::string err = std::string() +
			__FILE__ + "::" + __func__ +
			"() takes exactly 1 argument (" + std::to_string(Py_SIZE(args)) + " given)";
		PyErr_SetString(PyExc_TypeError, err.c_str());
		return nullptr;
	}

	PyObject *obj = PyTuple_GET_ITEM(args, 0);
	if (PyFloat_CheckExact(obj)) {
		return PyInt_FromLong(long(PyFloat_AS_DOUBLE(obj)));
	}

	std::string err = "expected float, got ";
	err += PyType_CheckExact(obj) ? "type" : obj->ob_type->tp_name;
	PyErr_SetString(PyExc_TypeError, err.c_str());
	return nullptr;
}

//C-API with expansion python-args to function-args
PyObject* floatToInt2(PyObject *obj) {
	if (PyFloat_CheckExact(obj))
		return PyInt_FromLong(long(PyFloat_AS_DOUBLE(obj)));

	std::string err = "expected float, got ";
	err += PyType_CheckExact(obj) ? "type" : obj->ob_type->tp_name;
	PyErr_SetString(PyExc_TypeError, err.c_str());
	return nullptr;
}

//usual C function
long floatToInt3(double d) {
	return long(d);
}

//template function
template <typename To, typename From>
To convFromTo(From obj) {
	return To(obj);
}


long getTimer() {
	static auto startTime = std::chrono::system_clock::now();

	auto now = std::chrono::system_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime);
	return duration.count();
}


void testPyCode(const std::string &testName, const std::string &code, PyObject *global) {
	std::cout << "Start test <" << testName << ">\n";
	PyObject *t = Py_CompileString(code.c_str(), "<string>", Py_file_input);
	PyCodeObject *co = reinterpret_cast<PyCodeObject*>(t);

	if (!co) {
		std::cout << "Compile Error\n";
		PyErr_Print();
		std::cout << "\n";
		return;
	}

	long t1 = getTimer();

	PyEval_EvalCode(co, global, nullptr);
	if (PyErr_Occurred()) {
		PyErr_Print();
		Py_DECREF(co);
		std::cout << "\n";
		return;
	}

	long t2 = getTimer();
	std::cout << "Ok, time: " << (t2 - t1) << " ms\n\n";

	Py_DECREF(co);
}

void tests(PyObject *global) {
	std::string code;

	code =
		"print 'ftoi_1', ftoi_1(5.2), ftoi_1(-5.2)\n"
		"print 'ftoi_2', ftoi_2(5.2), ftoi_2(-5.2)\n"
		"print 'ftoi_3', ftoi_3(5.2), ftoi_3(-5.2)\n"
		"print 'ftoi_4', ftoi_4(5.2), ftoi_4(-5.2)\n"
		"print 'ftoi_5', ftoi_5(5.2), ftoi_5(-5.2)\n"
		"print 'int   ',    int(5.2),    int(-5.2)"
	;
	testPyCode("Is Correct?", code, global);

	code =
		"for i in xrange(1000000):\n"
		"    a = ftoi_1(5.2)"
	;
	testPyCode("Speed-1", code, global);//time: 306

	code =
		"for i in xrange(1000000):\n"
		"    a = ftoi_2(5.2)"
	;
	testPyCode("Speed-2", code, global);//time: 318

	code =
		"for i in xrange(1000000):\n"
		"    a = ftoi_3(5.2)"
	;
	testPyCode("Speed-3", code, global);//time: 315

	code =
		"for i in xrange(1000000):\n"
		"    a = ftoi_4(5.2)"
	;
	testPyCode("Speed-4", code, global);//time: 345

	code =
		"for i in xrange(1000000):\n"
		"    a = ftoi_5(5.2)"
	;
	testPyCode("Speed-5", code, global);//time: 318

	code =
		"for i in xrange(1000000):\n"
		"    a = int(5.2)"
	;
	testPyCode("Speed-int", code, global);//time: 530
}


std::string& getStaticStrValue() {
	static std::string str = "start value";
	return str;
}
void setStaticStrValue(std::string s1, std::string s2) {
	std::string &str = getStaticStrValue();
	str = s1 + ' ' + s2;
}

const char* getStaticStrValue2() {
	std::string &str = getStaticStrValue();
	return str.c_str();
}
char getFirstByte(const char *s) {
	return s[0];
}

int iSum3d(double a, double b, double c) {
	return int(a + b + c);
}
uint64_t ullSqrUs(unsigned short i) {
	return uint64_t(i) * uint64_t(i);
}

void examples(PyObject *global) {
	PyObject *pyFunc;

	pyFunc = makePyFunc(getStaticStrValue);
	PyDict_SetItemString(global, "get_str", pyFunc);
	Py_DECREF(pyFunc);

	//print "start value"
	testPyCode("Example-1", "print get_str()", global);

	pyFunc = makePyFunc(setStaticStrValue);
	PyDict_SetItemString(global, "set_str", pyFunc);
	Py_DECREF(pyFunc);

	//TypeError:
	//setStaticStrValue() takes exactly 2 arguments (0 given)
	testPyCode("Exception-1", "set_str()", global);

	//TypeError:
	//setStaticStrValue() takes exactly 2 arguments (3 given)
	testPyCode("Exception-2", "set_str('q', 'w', 'e')", global);

	//TypeError:
	//setStaticStrValue(), argument #2: expected str, got int
	testPyCode("Exception-3", "set_str('qwe', 234)", global);

	//TypeError:
	//setStaticStrValue(), argument #2: expected str, got int
	// (exactly #2, not #1, because convert-order is not defined)
	testPyCode("Exception-4", "set_str(123, 234)", global);

	//TypeError:
	//setStaticStrValue(), argument #1: expected str, got type
	testPyCode("Exception-5", "set_str(str, 'qwe')", global);

	//ok
	testPyCode("Example-2", "set_str('next', 'value')", global);

	//print "next value"
	testPyCode("Example-3", "print get_str()", global);


	pyFunc = makePyFunc(getStaticStrValue2);
	PyDict_SetItemString(global, "get_str_2", pyFunc);
	Py_DECREF(pyFunc);

	//print "next value" again
	testPyCode("Example-4", "print get_str_2()", global);


	pyFunc = makePyFunc(getFirstByte);
	PyDict_SetItemString(global, "get_first_byte", pyFunc);
	Py_DECREF(pyFunc);

	//print 97 (ord('abc'[0]))
	testPyCode("Example-5", "print get_first_byte('abc')", global);

	//print 0 (c-string ends with '\0')
	testPyCode("Example-6", "print get_first_byte('')", global);


	pyFunc = makePyFunc(iSum3d);
	PyDict_SetItemString(global, "i_sum_3d", pyFunc);
	Py_DECREF(pyFunc);

	// [1 -> 1.0]   [2 -> 2.0]
	// int(1.0 + 2.0 + 3.0)
	//print 6
	testPyCode("Example-7", "print i_sum_3d(1, 2, 3.0)", global);


	pyFunc = makePyFunc(ullSqrUs);
	PyDict_SetItemString(global, "ull_sqr_us", pyFunc);
	Py_DECREF(pyFunc);

	//OverflowError:
	//ullSqrUs(), argument #1: int too small to convert to uint16_t
	testPyCode("Exception-6", "print ull_sqr_us(-1)", global);

	//OverflowError:
	//ullSqrUs(), argument #1: float too large to convert to uint16_t
	testPyCode("Exception-7", "print ull_sqr_us(100000.567)", global);

	//print 1000000
	testPyCode("Example-8", "print ull_sqr_us(1000.0)", global);
}

int main() {
	Py_Initialize();

	PyObject *main = PyImport_AddModule("__main__");
	PyObject *global = PyModule_GetDict(main);

	PyObject *pyFunc;


	//PURE C-API
	static PyMethodDef tmp{"ftoi_1", floatToInt1, METH_VARARGS, nullptr};
	pyFunc = PyCFunction_NewEx(&tmp, nullptr, nullptr);
	PyDict_SetItemString(global, "ftoi_1", pyFunc);
	Py_DECREF(pyFunc);

	//C-API with expansion python-args to function-args
	pyFunc = makePyFunc(floatToInt2);
	PyDict_SetItemString(global, "ftoi_2", pyFunc);
	Py_DECREF(pyFunc);

	//usual C function
	pyFunc = makePyFunc(floatToInt3);
	PyDict_SetItemString(global, "ftoi_3", pyFunc);
	Py_DECREF(pyFunc);

	//template function
	pyFunc = makePyFunc(convFromTo<long, double>);
	PyDict_SetItemString(global, "ftoi_4", pyFunc);
	Py_DECREF(pyFunc);


	//lambda

	long (*lambda)(double) = [](double d) -> long {
		return long(d);
	};
	pyFunc = makePyFunc(lambda);

	//OR:
	/*
	auto lambda = [](double d) -> long {
		return long(d);
	};
	pyFunc = makePyFunc(static_cast<long(*)(double)>(lambda));
	*/

	PyDict_SetItemString(global, "ftoi_5", pyFunc);
	Py_DECREF(pyFunc);


	tests(global);
	examples(global);

	//Don't forget!!!
	//Especially important if Py_Initialize & makePyFunc called after Py_Finalize again
	clearPyWrappers();

	Py_Finalize();

	return 0;
}
