#ifndef SERIAL_INDEX_H
#define SERIAL_INDEX_H

#include "util.h"

#define BAUDRATE       (9600)
#define CAPACITY       (6)
#define BUFFERSIZE     (128)
#define MAX_KEY_LENGTH (16)

// comment out one or more of the following flags to disable certain types
#define SERIALINDEX_INT
#define SERIALINDEX_FLOAT
#define SERIALINDEX_STRING
#define SERIALINDEX_INT_ARRAY
#define SERIALINDEX_FLOAT_ARRAY

#if defined(SERIALINDEX_INT_ARRAY) || defined(SERIALINDEX_FLOAT_ARRAY)
#define SERIALINDEX_ARRAY
#endif

// comment out one of the following flags to disable read or write capability
#define SERIALINDEX_READ
#define SERIALINDEX_WRITE

enum Mode {
	Read = 1,
	Write = 2,
};

union Tolerance {
	int   i;

#ifdef SERIALINDEX_FLOAT
	float f;
#endif
};

struct Val {
	void *    now;
	void *    before;
	Tolerance tolerance;
	size_t    length;
};

enum Context {
	Key,
	Value,

#ifdef SERIALINDEX_INT
	IntValue,
#endif

#ifdef SERIALINDEX_FLOAT
	FloatValue,
#endif

#ifdef SERIALINDEX_STRING
	StringValue,
#endif

#ifdef SERIALINDEX_ARRAY
	ArrayValue,
#endif

#ifdef SERIALINDEX_INT_ARRAY
	IntArrayValue,
#endif

#ifdef SERIALINDEX_FLOAT_ARRAY
	FloatArrayValue,
#endif

#ifdef SERIALINDEX_ARRAY
	SliceArrayValue,
#endif

#ifdef SERIALINDEX_INT_ARRAY
	IntSliceArrayValue,
#endif

#ifdef SERIALINDEX_FLOAT_ARRAY
	FloatSliceArrayValue,
#endif

	Skip,
};

enum Type {
	Unknown = 0,

#ifdef SERIALINDEX_INT
	Int,
#endif

#ifdef SERIALINDEX_FLOAT
	Float,
#endif

#ifdef SERIALINDEX_STRING
	String,
#endif

#ifdef SERIALINDEX_INT_ARRAY
	IntArray,
#endif

#ifdef SERIALINDEX_FLOAT_ARRAY
	FloatArray,
#endif

};

enum ValidateResult {
	Ok,
	Continue,
	Invalid,
};

typedef void (*Function)(void);
typedef bool (*InputFunc)(char **data, int *len);
typedef int (*OutputFunc)(const char *key, char delimiter, const void *value, const char *eol, Type type, size_t length);

class SerialIndex {
public:
	SerialIndex();
	~SerialIndex();

	SerialIndex&      setSerial(Stream &stream);
	SerialIndex&      setInputFunc(InputFunc fn);
	SerialIndex&      setOutputFunc(OutputFunc fn);
	SerialIndex&      ping(char *k);
	void              update(void);

#ifdef SERIALINDEX_READ
	SerialIndex&      in();
	SerialIndex&      read(bool);
#endif

#ifdef SERIALINDEX_WRITE
	SerialIndex&      out();
	SerialIndex&      write(bool);
#endif

#ifdef SERIALINDEX_INT
	// int
	SerialIndex&      add(const char *k, int &v, int theTolerance);
	SerialIndex&      add(const char *k, int &v);
#endif

#ifdef SERIALINDEX_FLOAT
	// float
	SerialIndex&      add(const char *k, float &v, float theTolerance);
	SerialIndex&      add(const char *k, float &v);
#endif

#ifdef SERIALINDEX_INT_ARRAY
	// int-array
	template<int N>
	SerialIndex&      add(const char *k, int (&v)[N]);

	template<int N>
	SerialIndex&      add(const char *k, int (&v)[N], int tolerance);
#endif

#ifdef SERIALINDEX_FLOAT_ARRAY
	// float-array
	template<int N>
	SerialIndex&      add(const char *k, float (&v)[N]);

	template<int N>
	SerialIndex&      add(const char *k, float (&v)[N], float tolerance);
#endif

#ifdef SERIALINDEX_STRING
	// string
	template<int N>
	SerialIndex&      add(const char *k, char (&v)[N]);
	SerialIndex&      add(const char *k, char *&v);
#endif

	// function
	SerialIndex&      listen(const char *k, void (*v)(void));

	void              reset_context(void);

#ifdef SERIALINDEX_READ
	void              read_input(char c);
#endif

#ifdef SERIALINDEX_WRITE
	bool              write_output();
#endif

#ifdef SERIALINDEX_INT
#ifdef SERIALINDEX_WRITE
	bool              write_int();
#endif
#ifdef SERIALINDEX_READ
	void              read_int(char c);
	ValidateResult    validate_int(char *s, char *e);
	bool              eval_int(char *s, char *e);
#endif
#endif

#ifdef SERIALINDEX_FLOAT
#ifdef SERIALINDEX_WRITE
	bool              write_float();
#endif
#ifdef SERIALINDEX_READ
	void              read_float(char c);
	ValidateResult    validate_float(char *s, char *e);
	bool              eval_float(char *s, char *e);
#endif
#endif

#ifdef SERIALINDEX_STRING
#ifdef SERIALINDEX_WRITE
	bool              write_string();
#endif
#ifdef SERIALINDEX_READ
	void              read_string(char c);
	ValidateResult    validate_string(char *s, char *e);
	bool              eval_string(char *s, char *e);
#endif
#endif

#ifdef SERIALINDEX_INT_ARRAY
#ifdef SERIALINDEX_WRITE
	bool              write_int_array();
#endif
#ifdef SERIALINDEX_READ
	void              read_int_array(char c);
	void              read_int_slice_array(char c);
	ValidateResult    validate_int_array(char *s, char *e);
	ValidateResult    validate_int_slice_array(char *s, char *e);
	ValidateResult    validate_int_slice(char *s, char *e);
	bool              eval_int_array(char *s, char *e);
	bool              eval_int_array_nth(char *s, char *e, size_t i);
	bool              eval_int_slice_array(char *s, char *e);
	bool              eval_int_slice(char *s, char *e);
#endif
#endif

#ifdef SERIALINDEX_FLOAT_ARRAY
#ifdef SERIALINDEX_WRITE
	bool              write_float_array();
#endif
#ifdef SERIALINDEX_READ
	void              read_float_array(char c);
	void              read_float_slice_array(char c);
	ValidateResult    validate_float_array(char *s, char *e);
	ValidateResult    validate_float_slice_array(char *s, char *e);
	ValidateResult    validate_float_slice(char *s, char *e);
	bool              eval_float_array(char *s, char *e);
	bool              eval_float_array_nth(char *s, char *e, size_t i);
	bool              eval_float_slice_array(char *s, char *e);
	bool              eval_float_slice(char *s, char *e);
#endif
#endif

#ifdef SERIALINDEX_READ
#ifdef SERIALINDEX_ARRAY
	void              read_array(char c);
	void              read_slice_array(char c);
#endif

	void              read_key(char c);
	void              read_value(char c);
	void              read_skip(char c);

	void              eval(char *s, char *e);
#endif

private:
	Stream *          serial;
	int               mode;
	const char **     keys;
	Type *            types;
	Val *             values;
	Function *        functions;
	char *            buffer;
	Context           context;
	size_t            ibuffer;
	size_t            ikey;
	size_t            nkeys;
	size_t            capacity;

	InputFunc         inputFunc;
	OutputFunc        outputFunc;

	size_t            find_key(const char *s);
	bool              is_eol();

	template<typename T>
	SerialIndex&      add(const char *k, T &v, Type t);

	template<typename T>
	SerialIndex&      add(const char *k, T &v, Type t, Tolerance tolerance);

	template<typename T, int N>
	SerialIndex&      add(const char *k, T (&v)[N], Type t, Tolerance tolerance);
};

// generic
template<typename T>
SerialIndex& SerialIndex::add(const char *k, T &v, Type t)
{
	if (!k || nkeys >= capacity)
		goto out;

	if (find_key(k) < SIZE_MAX)
		goto out;

	types[nkeys] = t;
	keys[nkeys] = k;
	values[nkeys] = Val { now: &v, before: malloc(sizeof(v)), tolerance: { 0 } };
	functions[nkeys] = 0;
	nkeys++;

out:
	return *this;
}

// generic
template<typename T>
SerialIndex& SerialIndex::add(const char *k, T &v, Type t, Tolerance tolerance)
{
	if (!k || nkeys >= capacity)
		goto out;

	if (find_key(k) < SIZE_MAX)
		goto out;

	types[nkeys] = t;
	keys[nkeys] = k;
	values[nkeys] = Val { now: &v, before: malloc(sizeof(v)), tolerance: tolerance };
	functions[nkeys] = 0;
	nkeys++;

out:
	return *this; 
}

// generic
template<typename T, int N>
SerialIndex& SerialIndex::add(const char *k, T (&v)[N], Type t, Tolerance tolerance)
{
	if (!k || nkeys >= capacity)
		goto out;

	if (find_key(k) < SIZE_MAX)
		goto out;

	types[nkeys] = t;
	keys[nkeys] = k;
	values[nkeys] = Val { now: &v, before: malloc(sizeof(v)), tolerance, length: N };
	functions[nkeys] = 0;
	nkeys++;

out:
	return *this;
}

#ifdef SERIALINDEX_STRING
// string
template<int N>
SerialIndex& SerialIndex::add(const char *k, char (&v)[N]) 
{
	return add(k, v, Type::String);
}

#endif

#ifdef SERIALINDEX_INT_ARRAY

// int-array
template<int N>
SerialIndex& SerialIndex::add(const char *k, int (&v)[N])
{
	Tolerance tolerance = { i: 0 };
	return add(k, v, Type::IntArray, tolerance);
}

template<int N>
SerialIndex& SerialIndex::add(const char *k, int (&v)[N], int theTolerance)
{
	Tolerance tolerance = { i: theTolerance };
	return add(k, v, Type::IntArray, tolerance);
}

#endif

#ifdef SERIALINDEX_FLOAT_ARRAY

// float-array
template<int N>
SerialIndex& SerialIndex::add(const char *k, float (&v)[N]) 
{
	Tolerance tolerance = { f: 0 };
	return add(k, v, Type::FloatArray, tolerance);
}

template<int N>
SerialIndex& SerialIndex::add(const char *k, float (&v)[N], float theTolerance)
{
	Tolerance tolerance = { f: theTolerance };
	return add(k, v, Type::FloatArray, tolerance);
}

#endif

#endif // SERIAL_INDEX_H
