#ifndef IO_HPP
#define IO_HPP

#include "util.h"

// uncomment one or more of the following flags to disable certain types
#define IO_INT
#define IO_FLOAT
#define IO_STRING
#define IO_INT_ARRAY
#define IO_FLOAT_ARRAY

#if defined(IO_INT_ARRAY) || defined(IO_FLOAT_ARRAY)
#define IO_ARRAY
#endif

#define BAUDRATE       (9600)
#define CAPACITY       (6)
#define BUFFERSIZE     (128)
#define MAX_KEY_LENGTH (16)

typedef void (*Function)(void);

union Tolerance {
	int   i;

#ifdef IO_FLOAT
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

#ifdef IO_INT
	IntValue,
#endif

#ifdef IO_FLOAT
	FloatValue,
#endif

#ifdef IO_STRING
	StringValue,
#endif

#ifdef IO_ARRAY
	ArrayValue,
#endif

#ifdef IO_INT_ARRAY
	IntArrayValue,
#endif

#ifdef IO_FLOAT_ARRAY
	FloatArrayValue,
#endif

#ifdef IO_ARRAY
	SliceArrayValue,
#endif

#ifdef IO_INT_ARRAY
	IntSliceArrayValue,
#endif

#ifdef IO_FLOAT_ARRAY
	FloatSliceArrayValue,
#endif

	Skip,
};

enum Type {
	Unknown = 0,

#ifdef IO_INT
	Int,
#endif

#ifdef IO_FLOAT
	Float,
#endif

#ifdef IO_STRING
	String,
#endif

#ifdef IO_INT_ARRAY
	IntArray,
#endif

#ifdef IO_FLOAT_ARRAY
	FloatArray,
#endif

};

enum ValidateResult {
	Ok,
	Continue,
	Invalid,
};

class IO {
public:
	IO();
	~IO();

#ifdef IO_INT
	// int
	IO&            add(const char *k, int &v, int theTolerance);
	IO&            add(const char *k, int &v);
#endif

#ifdef IO_FLOAT
	// float
	IO&            add(const char *k, float &v, float theTolerance);
	IO&            add(const char *k, float &v);
#endif

#ifdef IO_INT_ARRAY
	// int-array
	template<int N>
	IO&            add(const char *k, int (&v)[N]);

	template<int N>
	IO&            add(const char *k, int (&v)[N], int tolerance);
#endif

#ifdef IO_FLOAT_ARRAY
	// float-array
	template<int N>
	IO&            add(const char *k, float (&v)[N]);

	template<int N>
	IO&            add(const char *k, float (&v)[N], float tolerance);
#endif

#ifdef IO_STRING
	// string
	template<int N>
	IO&            add(const char *k, char (&v)[N]);
	IO&            add(const char *k, char *&v);
#endif

	// function
	IO&            listen(const char *k, void (*v)(void));

	bool           check_value_updates();
	size_t         available();
	void           reset_context(void);

	char           read();

#ifdef IO_INT
	bool           read_int();
	void           write_int(char c);
	ValidateResult validate_int(char *s, char *e);
	void           eval_int(char *s, char *e);
#endif

#ifdef IO_FLOAT
	bool           read_float();
	void           write_float(char c);
	ValidateResult validate_float(char *s, char *e);
	void           eval_float(char *s, char *e);
#endif

#ifdef IO_STRING
	bool           read_string();
	void           write_string(char c);
	ValidateResult validate_string(char *s, char *e);
	void           eval_string(char *s, char *e);
#endif

#ifdef IO_INT_ARRAY
	bool           read_int_array();
	void           write_int_array(char c);
	void           write_int_slice_array(char c);
	ValidateResult validate_int_array(char *s, char *e);
	ValidateResult validate_int_slice_array(char *s, char *e);
	ValidateResult validate_int_slice(char *s, char *e);
	void           eval_int_array(char *s, char *e);
	void           eval_int_array_nth(char *s, char *e, size_t i);
	void           eval_int_slice_array(char *s, char *e);
	void           eval_int_slice(char *s, char *e);
#endif

#ifdef IO_FLOAT_ARRAY
	bool           read_float_array();
	void           write_float_array(char c);
	void           write_float_slice_array(char c);
	ValidateResult validate_float_array(char *s, char *e);
	ValidateResult validate_float_slice_array(char *s, char *e);
	ValidateResult validate_float_slice(char *s, char *e);
	void           eval_float_array(char *s, char *e);
	void           eval_float_array_nth(char *s, char *e, size_t i);
	void           eval_float_slice_array(char *s, char *e);
	void           eval_float_slice(char *s, char *e);
#endif

#ifdef IO_ARRAY
	void           write_array(char c);
	void           write_slice_array(char c);
#endif

	void           write(char);
	void           write_key(char c);
	void           write_value(char c);
	void           write_skip(char c);

	void           eval(char *s, char *e);

private:
	const char **  keys;
	Type *         types;
	Val *          values;
	Function *     functions;
	char *         buffer;
	Context        context;
	size_t         ibuffer;
	size_t         nbuffer;
	size_t         ikey;
	size_t         nkeys;
	size_t         capacity;

	size_t         find_key(const char *s);
	bool           is_eol();

	template<typename T>
	IO&            add(const char *k, T &v, Type t);

	template<typename T>
	IO&            add(const char *k, T &v, Type t, Tolerance tolerance);

	template<typename T, int N>
	IO&            add(const char *k, T (&v)[N], Type t, Tolerance tolerance);
};

// generic
template<typename T>
IO& IO::add(const char *k, T &v, Type t)
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
IO& IO::add(const char *k, T &v, Type t, Tolerance tolerance)
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
IO& IO::add(const char *k, T (&v)[N], Type t, Tolerance tolerance)
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

#ifdef IO_STRING
// string
template<int N>
IO& IO::add(const char *k, char (&v)[N]) 
{
	return add(k, v, Type::String);
}

#endif

#ifdef IO_INT_ARRAY

// int-array
template<int N>
IO& IO::add(const char *k, int (&v)[N])
{
	return add(k, v, Type::IntArray);
}

template<int N>
IO& IO::add(const char *k, int (&v)[N], int theTolerance)
{
	Tolerance tolerance = { i: theTolerance };
	return add(k, v, Type::IntArray, tolerance);
}

#endif

#ifdef IO_FLOAT_ARRAY

// float-array
template<int N>
IO& IO::add(const char *k, float (&v)[N]) 
{
	return add(k, v, Type::FloatArray);
}

template<int N>
IO& IO::add(const char *k, float (&v)[N], float theTolerance)
{
	Tolerance tolerance = { f: theTolerance };
	return add(k, v, Type::FloatArray, tolerance);
}

#endif

#endif
