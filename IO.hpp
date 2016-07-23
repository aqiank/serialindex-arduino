#ifndef IO_HPP
#define IO_HPP

#include "util.h"

#define SERIALINDEX_INT          (1)
#define SERIALINDEX_FLOAT        (2)
#define SERIALINDEX_STRING       (4)
#define SERIALINDEX_INT_ARRAY    (8)
#define SERIALINDEX_FLOAT_ARRAY  (16)
#define SERIALINDEX_ARRAY        (24)
#define SERIALINDEX_ALL          (31)

#define SERIALINDEX_MODE         SERIALINDEX_INT

#define BAUDRATE       (9600)
#define CAPACITY       (6)
#define BUFFERSIZE     (128)
#define MAX_KEY_LENGTH (16)

typedef void (*Function)(void);

union Tolerance {
	int   i;

#if (SERIALINDEX_MODE & SERIALINDEX_FLOAT) != 0
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

#if (SERIALINDEX_MODE & SERIALINDEX_INT) != 0
	IntValue,
#endif

#if (SERIALINDEX_MODE & SERIALINDEX_FLOAT) != 0
	FloatValue,
#endif

#if (SERIALINDEX_MODE & SERIALINDEX_STRING) != 0
	StringValue,
#endif

#if (SERIALINDEX_MODE & SERIALINDEX_ARRAY) != 0
	ArrayValue,
#endif

#if (SERIALINDEX_MODE & SERIALINDEX_INT_ARRAY) != 0
	IntArrayValue,
#endif

#if (SERIALINDEX_MODE & SERIALINDEX_FLOAT_ARRAY) != 0
	FloatArrayValue,
#endif

#if (SERIALINDEX_MODE & SERIALINDEX_ARRAY) != 0
	SliceArrayValue,
#endif

#if (SERIALINDEX_MODE & SERIALINDEX_INT_ARRAY) != 0
	IntSliceArrayValue,
#endif

#if (SERIALINDEX_MODE & SERIALINDEX_FLOAT_ARRAY) != 0
	FloatSliceArrayValue,
#endif

	Skip,
};

enum Type {
	Unknown = 0,

#if (SERIALINDEX_MODE & SERIALINDEX_INT) != 0
	Int,
#endif

#if (SERIALINDEX_MODE & SERIALINDEX_FLOAT) != 0
	Float,
#endif

#if (SERIALINDEX_MODE & SERIALINDEX_STRING) != 0
	String,
#endif

#if (SERIALINDEX_MODE & SERIALINDEX_INT_ARRAY) != 0
	IntArray,
#endif

#if (SERIALINDEX_MODE & SERIALINDEX_FLOAT_ARRAY) != 0
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

#if (SERIALINDEX_MODE & SERIALINDEX_INT) != 0
	// int
	IO&            add(const char *k, int &v, int theTolerance);
	IO&            add(const char *k, int &v);
#endif

#if (SERIALINDEX_MODE & SERIALINDEX_FLOAT) != 0
	// float
	IO&            add(const char *k, float &v, float theTolerance);
	IO&            add(const char *k, float &v);
#endif

#if (SERIALINDEX_MODE & SERIALINDEX_INT_ARRAY) != 0
	// int-array
	template<int N>
	IO&            add(const char *k, int (&v)[N]);

	template<int N>
	IO&            add(const char *k, int (&v)[N], int tolerance);
#endif

#if (SERIALINDEX_MODE & SERIALINDEX_FLOAT_ARRAY) != 0
	// float-array
	template<int N>
	IO&            add(const char *k, float (&v)[N]);

	template<int N>
	IO&            add(const char *k, float (&v)[N], float tolerance);
#endif

#if (SERIALINDEX_MODE & SERIALINDEX_STRING) != 0
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

#if (SERIALINDEX_MODE & SERIALINDEX_INT) != 0
	bool           read_int();
	void           write_int(char c);
	ValidateResult validate_int(char *s, char *e);
	void           eval_int(char *s, char *e);
#endif

#if (SERIALINDEX_MODE & SERIALINDEX_FLOAT) != 0
	bool           read_float();
	void           write_float(char c);
	ValidateResult validate_float(char *s, char *e);
	void           eval_float(char *s, char *e);
#endif

#if (SERIALINDEX_MODE & SERIALINDEX_STRING) != 0
	bool           read_string();
	void           write_string(char c);
	ValidateResult validate_string(char *s, char *e);
	void           eval_string(char *s, char *e);
#endif

#if (SERIALINDEX_MODE & SERIALINDEX_INT_ARRAY) != 0
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

#if (SERIALINDEX_MODE & SERIALINDEX_FLOAT_ARRAY) != 0
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

#if (SERIALINDEX_MODE & SERIALINDEX_ARRAY) != 0
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

#if (SERIALINDEX_MODE & SERIALINDEX_STRING) != 0
// string
template<int N>
IO& IO::add(const char *k, char (&v)[N]) 
{
	return add(k, v, Type::String);
}

#endif

#if (SERIALINDEX_MODE & SERIALINDEX_INT_ARRAY) != 0

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

#if (SERIALINDEX_MODE & SERIALINDEX_FLOAT_ARRAY) != 0

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
