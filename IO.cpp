#include "IO.hpp"

static const char   KV_DELIMITER                = '=';
static const char   EOL[]                       = "\r\n";
static const char   SLICE_DELIMITER             = '=';
static const size_t EOL_LEN                     = LEN(EOL) - 1;

IO::IO()
{
	context    = Context::Key;
	capacity   = CAPACITY == SIZE_MAX ? SIZE_MAX - 1 : CAPACITY;
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

	for (i = 0; i < nkeys; i++)
		free(values[i].before); // used free() here because delete requires typed variable

	free(values);

	delete buffer;
	delete types;
	delete keys;
	delete functions;
}

// int
IO& IO::add(const char *k, int &v, int theTolerance) 
{
	Tolerance tolerance = { i: theTolerance };
	return add(k, v, Type::Int, tolerance);
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

bool IO::check_value_updates()
{
	if (ikey == SIZE_MAX)
		ikey = 0;

	if (nkeys == 0 || ikey >= nkeys)
		return false;

	ibuffer = 0;

	switch (types[ikey]) {
	case Type::Int:
		read_int();
		break;

	case Type::Float:
		read_float();
		break;

	case Type::IntArray:
		read_int_array();
		break;

	case Type::FloatArray:
		read_float_array();
		break;

	case Type::String:
		read_string();
		break;

	default:
		break;
	}

	ikey++;
	return true;
}

size_t IO::available()
{
	size_t d = nbuffer - ibuffer;

	if (d == 0) {
		nbuffer = 0;
		ibuffer = 0;
	}

	return d;
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

	if (abs(*now - *before) >= tolerance) {
		snprintf(buffer, BUFFERSIZE, "%s%c%d%s", keys[ikey], KV_DELIMITER, *now, EOL);
		nbuffer = strlen(buffer);
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

	if (fabs(*now - *before) >= tolerance) {

#if defined(__AVR_ATmega8__)    || \
    defined(__AVR_ATmega8U2__)  || \
    defined(__AVR_ATmega168__)  || \
    defined(__AVR_ATmega32U4__) || \
    defined(__AVR_ATmega328__)  || \
    defined(__AVR_ATmega328P__) || \
    defined(__AVR_ATmega1280__) || \
    defined(__AVR_ATmega2560__)

		// Some AVR processors don't support float standard C formatting, so we have to use AVR-specific stuff

		char now_str[9];
		dtostrf(*now, 8, 6, now_str);
		snprintf(buffer, BUFFERSIZE, "%s%c%s%s", keys[ikey], KV_DELIMITER, now_str, EOL);

#else
		snprintf(buffer, BUFFERSIZE, "%s%c%f%s", keys[ikey], KV_DELIMITER, *now, EOL);
#endif

		nbuffer = strlen(buffer);

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
		if (abs(*(now + i) - *(before + i)) >= tolerance) {
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

		nbuffer = strlen(buffer);
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
		if (fabs(*(now + i) - *(before + i)) >= tolerance) {
			changed = true;
			break;
		}
	}

	if (changed) {
		p += snprintf(p, q - p, "%s%c[", keys[ikey], KV_DELIMITER);

		for (i = 0; i < length; i++) {

#if defined(__AVR_ATmega8__)    || \
    defined(__AVR_ATmega8U2__)  || \
    defined(__AVR_ATmega168__)  || \
    defined(__AVR_ATmega32U4__) || \
    defined(__AVR_ATmega328__)  || \
    defined(__AVR_ATmega328P__) || \
    defined(__AVR_ATmega1280__) || \
    defined(__AVR_ATmega2560__)

			// Some AVR processors don't support float standard C formatting, so we have to use AVR-specific stuff

			char now_str[9];
			dtostrf(now[i], 8, 6, now_str);
			p += snprintf(p, q - p, "%s,", now_str);

#else
			p += snprintf(p, q - p, "%f,", now[i]);
#endif

			before[i] = now[i];
		}

		p += snprintf(p, q - p, "]%s", EOL);

		nbuffer = strlen(buffer);
	}

	return changed;
}

bool IO::read_string()
{
	const char *now     = (char *) values[ikey].now;
	char *before        = (char *) values[ikey].before;

	if (strcmp(now, before) != 0) {
		snprintf(buffer, BUFFERSIZE, "%s%c%s%s", keys[ikey], KV_DELIMITER, now, EOL);
		strcpy(before, now);
		nbuffer = strlen(buffer);
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
	int *now = (int *) values[ikey].now;
	int *before = (int *) values[ikey].before;

	*before = *now = atois(s, e);
}

void IO::eval_float(char *s, char *e)
{
	float *now = (float *) values[ikey].now;
	float *before = (float *) values[ikey].before;
	
	*before = *now = strtods(s, e, NULL);
}

void IO::eval_string(char *s, char *e)
{
	char *now = (char *) values[ikey].now;
	char *before = (char *) values[ikey].before;

	if (*s == '\'' || *s == '"') {
		s += 1;
		e -= 1;
	}

	*e = 0;
	strcpy(now, s);
	strcpy(before, s);
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
	int *now = (int *) values[ikey].now;
	int *before = (int *) values[ikey].before;

	if (*s == ']' && *e == ']')
		return;

	before[i] = now[i] = atois(s, e);
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
	float *now = (float *) values[ikey].now;
	float *before = (float *) values[ikey].before;

	if (*s == ']' && *e == ']')
		return;

	before[i] = now[i] = strtods(s, e, NULL);
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
	int *now = (int *) values[ikey].now;
	int *before = (int *) values[ikey].before;
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
		before[i] = now[i] = value;
}

void IO::eval_float_slice(char *s, char *e)
{
	char *p, *dp = 0, *rdp = 0;
	float *now = (float *) values[ikey].now;
	float *before = (float *) values[ikey].before;
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
		before[i] = now[i] = value;
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
