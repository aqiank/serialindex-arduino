#ifndef IO_HPP
#define IO_HPP

#include "util.h"

#define BAUDRATE       (9600)
#define CAPACITY       (6)
#define BUFFERSIZE     (128)
#define MAX_KEY_LENGTH (16)

typedef void (*Function)(void);

union Tolerance {
	int   i;
	float f;
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
	IntValue,
	FloatValue,
	StringValue,
	ArrayValue,
	IntArrayValue,
	FloatArrayValue,
	SliceArrayValue,
	IntSliceArrayValue,
	FloatSliceArrayValue,
	Skip,
};

enum Type {
	Unknown = 0,
	Int,
	Float,
	String,
	IntArray,
	FloatArray,
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

	// int
	IO&            add(const char *k, int &v, int theTolerance);
	IO&            add(const char *k, int &v);

	// float
	IO&            add(const char *k, float &v, float theTolerance);
	IO&            add(const char *k, float &v);

	// int-array
	template<int N>
	IO&            add(const char *k, int (&v)[N]);

	template<int N>
	IO&            add(const char *k, int (&v)[N], int tolerance);

	// float-array
	template<int N>
	IO&            add(const char *k, float (&v)[N]);

	template<int N>
	IO&            add(const char *k, float (&v)[N], float tolerance);

	// string
	template<int N>
	IO&            add(const char *k, char (&v)[N]);
	IO&            add(const char *k, char *&v);

	// function
	IO&            listen(const char *k, void (*v)(void));

	bool           check_value_updates();
	size_t         available();
	void           reset_context(void);

	char           read();
	bool           read_int();
	bool           read_float();
	bool           read_int_array();
	bool           read_float_array();
	bool           read_string();

	void           write(char);
	void           write_key(char c);
	void           write_value(char c);
	void           write_int(char c);
	void           write_float(char c);
	void           write_string(char c);
	void           write_array(char c);
	void           write_int_array(char c);
	void           write_float_array(char c);
	void           write_slice_array(char c);
	void           write_int_slice_array(char c);
	void           write_float_slice_array(char c);
	void           write_skip(char c);

	ValidateResult validate_int(char *s, char *e);
	ValidateResult validate_float(char *s, char *e);
	ValidateResult validate_string(char *s, char *e);
	ValidateResult validate_int_array(char *s, char *e);
	ValidateResult validate_float_array(char *s, char *e);
	ValidateResult validate_int_slice_array(char *s, char *e);
	ValidateResult validate_float_slice_array(char *s, char *e);
	ValidateResult validate_int_slice(char *s, char *e);
	ValidateResult validate_float_slice(char *s, char *e);

	void           eval(char *s, char *e);
	void           eval_int(char *s, char *e);
	void           eval_float(char *s, char *e);
	void           eval_string(char *s, char *e);
	void           eval_int_array(char *s, char *e);
	void           eval_int_array_nth(char *s, char *e, size_t i);
	void           eval_float_array(char *s, char *e);
	void           eval_float_array_nth(char *s, char *e, size_t i);
	void           eval_int_slice_array(char *s, char *e);
	void           eval_float_slice_array(char *s, char *e);
	void           eval_int_slice(char *s, char *e);
	void           eval_float_slice(char *s, char *e);

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

// string
template<int N>
IO& IO::add(const char *k, char (&v)[N]) 
{
	return add(k, v, Type::String);
}

#endif
