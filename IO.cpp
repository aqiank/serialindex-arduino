#include "IO.hpp"

static const char   KV_DELIMITER                = '=';
static const char   EOL[]                       = "\r\n";
static const char   SLICE_DELIMITER             = '=';
static const size_t EOL_LEN                     = LEN(EOL) - 1;

IO::IO()
{
	context    = Context::Key;
	capacity   = CAPACITY;
	keys       = new const char *[capacity];
	types      = new Type[capacity];
	functions  = new Function[capacity];
	buffer     = new char[BUFFERSIZE];
	ibuffer    = 0;
	nbuffer    = 0;
	ikey       = SIZE_MAX;
	nkeys      = 0;

	values     = (Val *) malloc(sizeof(*values) * capacity);
	memset(values, 0, sizeof(*values) * capacity);
}

IO::~IO()
{
	size_t i;

	for (i = 0; i < nkeys; i++) {
		if (values[i].before)
			free(values[i].before); // used free() here because delete requires typed variable
	}
	free(values);

	delete buffer;
	delete types;
	delete keys;
	delete functions;
}

// int
IO& IO::add(const char *k, int &v, int theTolerance) 
{
	return add(k, v, Type::Int);
}

IO& IO::add(const char *k, int &v) 
{
	return add(k, v, Type::Int);
}

// float
IO& IO::add(const char *k, float &v, float theTolerance)
{
	return add(k, v, Type::Float);
}

IO& IO::add(const char *k, float &v) 
{
	return add(k, v, Type::Float);
}

// string
IO& IO::add(const char *k, char *&v) 
{
	return add(k, v, Type::String);
}

// function
IO& IO::listen(const char *k, void (*v)(void)) 
{
	size_t i;

	if (!k)
		goto out;

	i = find_key(k);
	if (i == SIZE_MAX)
		goto out;

	functions[i] = v;

out:
	return *this; 
}

size_t IO::available()
{
	while (ikey < nkeys && ikey < SIZE_MAX) {
		if (!values[ikey].tolerance.i && !values[ikey].before)
			continue;

		ibuffer = 0;

		switch (types[ikey]) {
		case Int:
			if (read_int())
				goto out;
			break;

		case Float:
			if (read_float())
				goto out;
			break;

		case IntArray:
			if (read_int_array())
				goto out;
			break;

		case FloatArray:
			if (read_float_array())
				goto out;
			break;

		case String:
			if (read_string())
				goto out;
			break;

		default:
			break;
		}

		ikey++;
	}

out:
	return nbuffer - ibuffer;
}

char IO::read()
{
	if (ibuffer >= nbuffer) {
		ikey = 0;
		return 0;
	}

	return buffer[ibuffer++];
}

bool IO::read_int()
{
	const int tolerance = values[ikey].tolerance.i;
	const int *now      = (int *) values[ikey].now;
	int *before         = (int *) values[ikey].before;

	if (*now - *before > tolerance) {
		snprintf(buffer, BUFFERSIZE, "%s%c%d%s", keys[ikey], KV_DELIMITER, *now, EOL);
		nbuffer = strlen(buffer);

		if (!before)
			before = (int *) malloc(sizeof(*before));

		*before = *now;

		return true;
	}

	return false;
}

bool IO::read_float()
{
	const float tolerance = values[ikey].tolerance.f;
	const float *now      = (float *) values[ikey].now;
	float *before         = (float *) values[ikey].before;

	if (*now - *before > tolerance) {
		snprintf(buffer, BUFFERSIZE, "%s%c%f%s", keys[ikey], KV_DELIMITER, *now, EOL);
		nbuffer = strlen(buffer);

		if (!before)
			before = (float *) malloc(sizeof(*before));

		*before = *now;

		return true;
	}

	return false;
}

bool IO::read_int_array()
{
	const size_t length = values[ikey].length;
	const int tolerance = values[ikey].tolerance.f;
	const int *now      = (int *) values[ikey].now;
	int *before         = (int *) values[ikey].before;
	bool changed        = false;
	char *p =  buffer, *q = buffer + BUFFERSIZE;
	size_t i;

	for (i = 0; i < length; i++) {
		if (*(now + i) - *(before + i) > tolerance) {
			changed = true;
			break;
		}
	}

	if (changed) {
		p += snprintf(p, q - p, "%s%c[", keys[ikey], KV_DELIMITER);

		for (i = 0; i < length; i++) {
			p += snprintf(p, q - p, "%d,", now[i]);
			before[i] = now[i];
		}

		p += snprintf(p, q - p, "]%s", EOL);
	}

	return changed;
}

bool IO::read_float_array()
{
	const size_t length   = values[ikey].length;
	const float tolerance = values[ikey].tolerance.f;
	const float *now      = (float *) values[ikey].now;
	float *before         = (float *) values[ikey].before;
	bool changed          = false;
	char *p =  buffer, *q = buffer + BUFFERSIZE;
	size_t i;

	for (i = 0; i < length; i++) {
		if (*(now + i) - *(before + i) > tolerance) {
			changed = true;
			break;
		}
	}

	if (changed) {
		p += snprintf(p, q - p, "%s%c[", keys[ikey], KV_DELIMITER);

		for (i = 0; i < length; i++) {
			p += snprintf(p, q - p, "%f,", now[i]);
			before[i] = now[i];
		}

		p += snprintf(p, q - p, "]%s", EOL);
	}

	return changed;
}

bool IO::read_string()
{
	const char *now      = (char *) values[ikey].now;
	char *before         = (char *) values[ikey].before;

	if (strcmp(now, before) != 0) {
		snprintf(buffer, BUFFERSIZE, "%s%c%d%s", keys[ikey], KV_DELIMITER, *now, EOL);
		nbuffer = strlen(buffer);

		if (before)
			before = strcpy(before, now);
		else
			before = strdup(now);

		return true;
	}

	return false;
}

void IO::write(char c)
{
	buffer[ibuffer++] = c;

	if (ibuffer >= BUFFERSIZE) {
		ibuffer = 0;
		context = Context::Skip;
	}

	switch (context) {
	case Context::Key:
		write_key(c);
		break;

	case Context::Value:
		write_value(c);
		break;

	case Context::IntValue:
		write_int(c);
		break;

	case Context::FloatValue:
		write_float(c);
		break;

	case Context::StringValue:
		write_string(c);
		break;

	case Context::ArrayValue:
		write_array(c);
		break;

	case Context::IntArrayValue:
		write_int_array(c);
		break;

	case Context::FloatArrayValue:
		write_float_array(c);
		break;

	case Context::SliceArrayValue:
		write_slice_array(c);
		break;

	case Context::IntSliceArrayValue:
		write_int_slice_array(c);
		break;

	case Context::FloatSliceArrayValue:
		write_float_slice_array(c);
		break;

	case Context::Skip:
		write_skip(c);
		break;
	}
}

void IO::write_key(char c)
{
	if (c == KV_DELIMITER) {
		buffer[ibuffer - 1] = 0;

		ikey = find_key(buffer);
		if (ikey == SIZE_MAX)
			goto skip;

		context = Context::Value;
		ibuffer = 0;
	} else if (!isalpha(c)) {
		goto skip;
	}

	return;

skip:
	context = Context::Skip;
}

void IO::write_value(char c)
{
	const Type type = types[ikey];

	switch (c) {
	case '0'...'9':
		if (type == Type::Int)
			context = Context::IntValue;
		else if (type == Type::Float)
			context = Context::FloatValue;
		else
			goto skip;
		return;

	case '[':
		if (type != Type::IntArray && type != Type::FloatArray)
			goto skip;

		context = Context::ArrayValue;
		return;

	case '{':
		if (type != Type::IntArray && type != Type::FloatArray)
			goto skip;

		context = Context::SliceArrayValue;
		return;

	default:
		if (type != Type::String)
			goto skip;

		context = Context::StringValue;
		return;
	}

skip:
	context = Context::Skip;
	return;
}

void IO::write_int(char c)
{
	if (is_eol()) {
		if (validate_int(&buffer[0], &buffer[ibuffer - EOL_LEN]) == ValidateResult::Ok) {
			eval(&buffer[0], &buffer[ibuffer - EOL_LEN]);
			return;
		}

		reset_context();
	}
}

void IO::write_float(char c)
{
	if (is_eol()) {
		if (validate_float(&buffer[0], &buffer[ibuffer - EOL_LEN]) == ValidateResult::Ok) {
			eval(&buffer[0], &buffer[ibuffer - EOL_LEN]);
			return;
		}

		reset_context();
	}
}

void IO::write_string(char c)
{
	if (is_eol()) {
		switch (validate_string(&buffer[0], &buffer[ibuffer - EOL_LEN])) {
		case ValidateResult::Ok:
			eval(&buffer[0], &buffer[ibuffer - EOL_LEN]);

		case ValidateResult::Continue:
			return;

		default:
			reset_context();
		}
	}
}

void IO::write_array(char c)
{
	const Type type = types[ikey];

	switch (c) {
	case '0'...'9':
		if (type == Type::IntArray)
			context = Context::IntArrayValue;
		else if (type == Type::FloatArray)
			context = Context::FloatArrayValue;
		else
			goto skip;
		break;

	default:

skip:
		context = Context::Skip;
		return;
	}
}

void IO::write_int_array(char c)
{
	if (c == ']') {
		if (validate_int_array(&buffer[1], &buffer[ibuffer + 1]) == ValidateResult::Ok) {
			eval(&buffer[1], &buffer[ibuffer + 1]);
			return;
		}

		context = Context::Skip;
	}
}

void IO::write_float_array(char c)
{
	if (c == ']') {
		if (validate_float_array(&buffer[1], &buffer[ibuffer + 1]) == ValidateResult::Ok) {
			eval(&buffer[1], &buffer[ibuffer + 1]);
			return;
		}

		context = Context::Skip;
	}
}

void IO::write_slice_array(char c)
{
	const Type type = types[ikey];

	switch (c) {
	case '0'...'9':
		if (type == Type::IntArray)
			context = Context::IntSliceArrayValue;
		else if (type == Type::FloatArray)
			context = Context::FloatSliceArrayValue;
		else
			goto skip;
		break;

	default:

skip:
		context = Context::Skip;
		return;
	}
}

void IO::write_int_slice_array(char c)
{
	if (c == '}') {
		if (validate_int_slice_array(&buffer[1], &buffer[ibuffer + 1]) == ValidateResult::Ok)
			eval(&buffer[1], &buffer[ibuffer + 1]);

		context = Context::Skip;
	}
}

void IO::write_float_slice_array(char c)
{
	if (c == '}') {
		if (validate_float_slice_array(&buffer[1], &buffer[ibuffer + 1]) == ValidateResult::Ok)
			eval(&buffer[1], &buffer[ibuffer + 1]);

		context = Context::Skip;
	}
}

void IO::write_skip(char c)
{
	if (!is_eol())
		return;

	reset_context();
}

ValidateResult IO::validate_int(char *s, char *e)
{
	const char *p;

	for (p = s; p < e; p++) {
		if (!isdigit(*p))
			return ValidateResult::Invalid;
	}

	return ValidateResult::Ok;
}

ValidateResult IO::validate_float(char *s, char *e)
{
	const char *p;
	int ndots = 0;

	for (p = s; p < e; p++) {
		if (isdigit(*p))
			continue;
		else if (*p == '.' && ndots == 0)
			ndots++;
		else
			return ValidateResult::Invalid;
	}

	return ValidateResult::Ok;
}

ValidateResult IO::validate_string(char *s, char *e)
{
	const char fc = *s;
	const bool has_start_quote = fc == '\'' || fc == '"';
	const bool has_end_quote = e - s > 1 && *(e - 1) == fc;

	if (has_start_quote && !has_end_quote)
		return ValidateResult::Continue;

	return ValidateResult::Ok;
}

ValidateResult IO::validate_int_array(char *s, char *e)
{
	char *pp, *q;
	size_t i = 0;

	for (pp = q = s; q < e; q++) {
		if (*q == ',' || *q == ']') {
			if (validate_int(pp, q) != ValidateResult::Ok)
				return ValidateResult::Invalid;

			pp = q + 1;
			i++;
		}
	}

	return ValidateResult::Ok;
}

ValidateResult IO::validate_float_array(char *s, char *e)
{
	char *pp, *q;
	size_t i = 0;

	for (pp = q = s; q < e; q++) {
		if (*q == ',' || *q == ']') {
			if (validate_float(pp, q) != ValidateResult::Ok)
				return ValidateResult::Invalid;

			pp = q + 1;
			i++;
		}
	}

	return ValidateResult::Ok;
}

ValidateResult IO::validate_int_slice_array(char *s, char *e)
{
	char *pp, *q;
	size_t i = 0;

	for (pp = q = s; q < e; q++) {
		if (*q == ',' || *q == '}') {
			if (validate_int_slice(pp, q) != ValidateResult::Ok)
				return ValidateResult::Invalid;

			pp = q + 1;
			i++;
		}
	}

	return ValidateResult::Ok;
}

ValidateResult IO::validate_float_slice_array(char *s, char *e)
{
	char *pp, *q;
	size_t i = 0;

	for (pp = q = s; q < e; q++) {
		if (*q == ',' || *q == '}') {
			if (validate_float_slice(pp, q) != ValidateResult::Ok)
				return ValidateResult::Invalid;

			pp = q + 1;
			i++;
		}
	}

	return ValidateResult::Ok;
}

ValidateResult IO::validate_int_slice(char *s, char *e)
{
	char *p;
	char *dp = 0;
	char *rdp = 0;

	for (p = s; p < e; p++) {
		if (dp)
			return validate_int(p, e);

		if (*p == SLICE_DELIMITER) {
			dp = p;
		} else if (is_slice_range_delimiter(p)) {
			if (rdp)
				goto invalid;
			rdp = p;
			p += SLICE_RANGE_DELIMITER_LEN - 1;
		} else if (!isdigit(*p)) {
			goto invalid;
		}
	}

invalid:
	return ValidateResult::Invalid;
}

ValidateResult IO::validate_float_slice(char *s, char *e)
{
	char *p;
	char *dp = 0;
	char *rdp = 0;

	for (p = s; p < e; p++) {
		if (dp)
			return validate_float(p, e);

		if (*p == SLICE_DELIMITER) {
			dp = p;
		} else if (is_slice_range_delimiter(p)) {
			if (rdp)
				goto invalid;
			rdp = p;
			p += SLICE_RANGE_DELIMITER_LEN - 1;
		} else if (!isdigit(*p)) {
			goto invalid;
		}
	}

invalid:
	return ValidateResult::Invalid;
}

void IO::eval(char *s, char *e)
{
	switch (context) {
	case Context::IntValue:
		eval_int(s, e);
		break;

	case Context::FloatValue:
		eval_float(s, e);
		break;

	case Context::StringValue:
		eval_string(s, e);
		break;

	case Context::IntArrayValue:
		eval_int_array(s, e);
		break;

	case Context::FloatArrayValue:
		eval_float_array(s, e);
		break;

	case Context::IntSliceArrayValue:
		eval_int_slice_array(s, e);
		break;

	case Context::FloatSliceArrayValue:
		eval_float_slice_array(s, e);
		break;

	default:
		return;
	}

	if (functions[ikey])
		functions[ikey]();

	reset_context();
}

void IO::eval_int(char *s, char *e)
{
	int *value = (int *) values[ikey].now;

	*value = atois(s, e);
}

void IO::eval_float(char *s, char *e)
{
	float *value = (float *) values[ikey].now;
	
	*value = strtods(s, e, NULL);
}

void IO::eval_string(char *s, char *e)
{
	char *value = (char *) values[ikey].now;

	if (*s == '\'' || *s == '"') {
		s += 1;
		e -= 1;
	}

	*e = 0;
	strcpy(value, s);
}

void IO::eval_int_array(char *s, char *e)
{
	char *p, *q;
	size_t i = 0;

	for (p = q = s; q < e; q++) {
		if (*q == ',' || *q == ']') {
			eval_int_array_nth(p, q, i);
			p = q + 1;
			i++;
		}
	}
}

void IO::eval_int_array_nth(char *s, char *e, size_t i)
{
	int *array = (int *) values[ikey].now;

	if (*s == ']' && *e == ']')
		return;

	array[i] = atois(s, e);
}

void IO::eval_float_array(char *s, char *e)
{
	char *p, *q;
	size_t i = 0;

	for (p = q = s; q < e; q++) {
		if (*q == ',' || *q == ']') {
			eval_float_array_nth(p, q, i);
			p = q + 1;
			i++;
		}
	}
}

void IO::eval_float_array_nth(char *s, char *e, size_t i)
{
	float *array = (float *) values[ikey].now;

	if (*s == ']' && *e == ']')
		return;

	array[i] = strtods(s, e, NULL);
}

void IO::eval_int_slice_array(char *s, char *e)
{
	char *p, *q;
	size_t i = 0;

	for (p = q = s; q < e; q++) {
		if (*q == ',' || *q == '}') {
			eval_int_slice(p, q);
			p = q + 1;
			i++;
		}
	}
}

void IO::eval_float_slice_array(char *s, char *e)
{
	char *p, *q;
	size_t i = 0;

	for (p = q = s; q < e; q++) {
		if (*q == ',' || *q == '}') {
			eval_float_slice(p, q);
			p = q + 1;
			i++;
		}
	}
}

void IO::eval_int_slice(char *s, char *e)
{
	char *p, *dp = 0, *rdp = 0;
	int *array = (int *) values[ikey].now;
	int value = 0;
	size_t start = 0, end = 0;
	size_t i;

	for (p = s; p < e; p++) {
		if (*p == SLICE_DELIMITER)
			dp = p;
		else if (is_slice_range_delimiter(p))
			rdp = p;
	}

	if (rdp) {
		if (rdp > s)
			start = atois(s, rdp);

		if (rdp < dp - SLICE_RANGE_DELIMITER_LEN)
			end = atois(rdp + SLICE_RANGE_DELIMITER_LEN, dp);
	} else {
		start = atois(s, dp);
		end = start + 1;
	}

	value = atois(dp + 1, e);

	for (i = start; i < end; i++)
		array[i] = value;
}

void IO::eval_float_slice(char *s, char *e)
{
	char *p, *dp = 0, *rdp = 0;
	float *array = (float *) values[ikey].now;
	float value = 0;
	size_t start = 0, end = 0;
	size_t i;

	for (p = s; p < e; p++) {
		if (*p == SLICE_DELIMITER)
			dp = p;
		else if (is_slice_range_delimiter(p))
			rdp = p;
	}

	if (rdp) {
		if (rdp > s)
			start = atois(s, rdp);

		if (rdp < dp - SLICE_RANGE_DELIMITER_LEN)
			end = atois(rdp + SLICE_RANGE_DELIMITER_LEN, dp);
	} else {
		start = atois(s, dp);
		end = start + 1;
	}

	value = strtods(dp + 1, e, NULL);

	for (i = start; i < end; i++)
		array[i] = value;
}

bool IO::is_eol()
{
	size_t i;

	if (ibuffer < EOL_LEN)
		return false;

	for (i = 0; i < EOL_LEN; i++) {
		if (buffer[ibuffer - EOL_LEN + i] != EOL[i] )
			return false;
	}

	return true;
}

void IO::reset_context(void)
{
	context = Context::Key;
	ibuffer = 0;
	ikey = SIZE_MAX;
}

size_t IO::find_key(const char *s) 
{
	size_t i;

	for (i = 0; i < nkeys; i++) {
		if (strcmp(keys[i], s) == 0)
			return i;
	}

	return SIZE_MAX;
}
