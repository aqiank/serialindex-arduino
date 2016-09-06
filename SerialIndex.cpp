#include "SerialIndex.h"

static const char   KV_DELIMITER                = '=';
static const char   EOL[]                       = "\r\n";
static const char   SLICE_DELIMITER             = '=';
static const size_t EOL_LEN                     = LEN(EOL) - 1;

SerialIndex::SerialIndex()
{
	mode       = 0;
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

	inputFunc  = NULL;
	outputFunc = NULL;

	values     = (Val *) malloc(sizeof(*values) * capacity);
	memset(values, 0, sizeof(*values) * capacity);
}

SerialIndex::~SerialIndex() 
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

SerialIndex& SerialIndex::setSerial(Stream &stream)
{
	serial = &stream;
}

SerialIndex& SerialIndex::setInputFunc(InputFunc fn)
{
	inputFunc = fn;
}

SerialIndex& SerialIndex::setOutputFunc(OutputFunc fn)
{
	outputFunc = fn;
}

SerialIndex& SerialIndex::ping(char *k)
{
	// TODO
	return *this; 
}

void SerialIndex::update(void)
{
#ifdef SERIALINDEX_READ
	if ((mode & Mode::Read) != 0)
		in();
#endif

#ifdef SERIALINDEX_WRITE
	if ((mode & Mode::Write) != 0)
		out();
#endif
}

#ifdef SERIALINDEX_READ
SerialIndex& SerialIndex::in()
{
	if (inputFunc) {
		char *buf = NULL;
		bool shouldDelete;
		int len;
		int i;

		shouldDelete = inputFunc(&buf, &len);

		for (i = 0; i < len; i++)
			SerialIndex::readInput(buf[i]);

		if (shouldDelete)
			delete buf; 
	} else {
		while (serial->available())
			SerialIndex::readInput(serial->read());
	}

	return *this;
}

SerialIndex& SerialIndex::read(bool b)
{
	if (b)
		mode |= Mode::Read;
	else
		mode &= ~Mode::Read;

	return *this;
}
#endif

#ifdef SERIALINDEX_WRITE
SerialIndex& SerialIndex::out()
{
	while (SerialIndex::writeOutput())
		;

	SerialIndex::resetContext();

	return *this;
}

SerialIndex& SerialIndex::write(bool b)
{
	if (b)
		mode |= Mode::Write;
	else
		mode &= ~Mode::Write;

	return *this;
}
#endif

#ifdef SERIALINDEX_INT

// int
SerialIndex& SerialIndex::add(const char *k, int &v, int theTolerance) 
{
	Tolerance tolerance = { i: theTolerance };
	return add(k, v, Type::Int, tolerance);
}

SerialIndex& SerialIndex::add(const char *k, int &v) 
{
	return add(k, v, Type::Int);
}

#endif

#ifdef SERIALINDEX_FLOAT

// float
SerialIndex& SerialIndex::add(const char *k, float &v, float theTolerance)
{
	Tolerance tolerance = { f: theTolerance };
	return add(k, v, Type::Float, tolerance);
}

SerialIndex& SerialIndex::add(const char *k, float &v) 
{
	return add(k, v, Type::Float);
}

#endif

#ifdef SERIALINDEX_STRING

// string
SerialIndex& SerialIndex::add(const char *k, char *&v) 
{
	return add(k, v, Type::String);
}

#endif

// function
SerialIndex& SerialIndex::listen(const char *k, void (*v)(void)) 
{
	size_t i;

	if (!k)
		goto out;

	i = findKey(k);
	if (i == SIZE_MAX)
		goto out;

	functions[i] = v;

out:
	return *this; 
}

#ifdef SERIALINDEX_WRITE
bool SerialIndex::writeOutput()
{
	if (ikey == SIZE_MAX)
		ikey = 0;

	if (nkeys == 0 || ikey >= nkeys)
		return false;

	ibuffer = 0;

	switch (types[ikey]) {

#ifdef SERIALINDEX_INT
	case Type::Int:
		writeInt();
		break;
#endif

#ifdef SERIALINDEX_FLOAT
	case Type::Float:
		writeFloat();
		break;
#endif

#ifdef SERIALINDEX_STRING
	case Type::String:
		writeString();
		break;
#endif

#ifdef SERIALINDEX_INT_ARRAY
	case Type::IntArray:
		writeIntArray();
		break;
#endif

#ifdef SERIALINDEX_FLOAT_ARRAY
	case Type::FloatArray:
		writeFloatArray();
		break;
#endif

	default:
		break;
	}

	ikey++;
	return true;
}
#endif

#ifdef SERIALINDEX_INT
#ifdef SERIALINDEX_WRITE
bool SerialIndex::writeInt()
{
	const int tolerance = values[ikey].tolerance.i;
	const int *now      = (int *) values[ikey].now;
	int *before         = (int *) values[ikey].before;

	if (abs(*now - *before) > tolerance) {
		if (outputFunc) {
			outputFunc(keys[ikey], KV_DELIMITER, now, EOL, Type::Int, 0);
		} else {
			serial->print(keys[ikey]);
			serial->print(KV_DELIMITER);
			serial->print(*now);
			serial->print(EOL);
		}
		return true;
	}

	return false;
}
#endif

#ifdef SERIALINDEX_READ
void SerialIndex::readInt(char c)
{
	if (isEOL()) {
		if (validateInt(&buffer[0], &buffer[ibuffer - EOL_LEN]) == ValidateResult::Ok) {
			eval(&buffer[0], &buffer[ibuffer - EOL_LEN]);
			return;
		}

		resetContext();
	}
}

ValidateResult SerialIndex::validateInt(char *s, char *e)
{
	const char *p;

	for (p = s; p < e; p++) {
		if (!isdigit(*p) && !(p == s && *p == '-'))
			return ValidateResult::Invalid;
	}

	return ValidateResult::Ok;
}

bool SerialIndex::evalInt(char *s, char *e)
{
	const int tolerance = values[ikey].tolerance.i;
	int *now = (int *) values[ikey].now;
	int *before = (int *) values[ikey].before;

	*now = atois(s, e);
	if (abs(*now - *before) >= tolerance) {
		*before = *now;
		return true;
	}

	return false;
}
#endif
#endif

#ifdef SERIALINDEX_FLOAT
#ifdef SERIALINDEX_WRITE
bool SerialIndex::writeFloat()
{
	const float tolerance = values[ikey].tolerance.f;
	const float *now      = (float *) values[ikey].now;
	float *before         = (float *) values[ikey].before;

	if (fabs(*now - *before) >= tolerance) {
		if (outputFunc) {
			outputFunc(keys[ikey], KV_DELIMITER, now, EOL, Type::Float, 0);
		} else {
			serial->print(keys[ikey]);
			serial->print(KV_DELIMITER);
			serial->print(*now);
			serial->print(EOL);
		}
		*before = *now;
		return true;
	}

	return false;
}
#endif

#ifdef SERIALINDEX_READ
void SerialIndex::readFloat(char c)
{
	if (isEOL()) {
		if (validateFloat(&buffer[0], &buffer[ibuffer - EOL_LEN]) == ValidateResult::Ok) {
			eval(&buffer[0], &buffer[ibuffer - EOL_LEN]);
			return;
		}

		resetContext();
	}
}

ValidateResult SerialIndex::validateFloat(char *s, char *e)
{
	const char *p;
	int ndots = 0;

	for (p = s; p < e; p++) {
		if (isdigit(*p) || (p == s && *p == '-'))
			continue;
		else if (*p == '.' && ndots == 0)
			ndots++;
		else
			return ValidateResult::Invalid;
	}

	return ValidateResult::Ok;
}

bool SerialIndex::evalFloat(char *s, char *e)
{
	const float tolerance = values[ikey].tolerance.f;
	float *now = (float *) values[ikey].now;
	float *before = (float *) values[ikey].before;

	*now = strtods(s, e, NULL);
	if (fabs(*now - *before) >= tolerance) {
		*before = *now;
		return true;
	}

	return false;
}
#endif
#endif

#ifdef SERIALINDEX_STRING
#ifdef SERIALINDEX_WRITE
bool SerialIndex::writeString()
{
	const char *now     = (char *) values[ikey].now;
	char *before        = (char *) values[ikey].before;

	if (strcmp(now, before) != 0) {
		if (outputFunc) {
			outputFunc(keys[ikey], KV_DELIMITER, now, EOL, Type::String, 0);
		} else {
			serial->print(keys[ikey]);
			serial->print(KV_DELIMITER);
			serial->print(now);
			serial->print(EOL);
		}
		strcpy(before, now);
		return true;
	}

	return false;
}
#endif

#ifdef SERIALINDEX_READ
void SerialIndex::readString(char c)
{
	if (isEOL()) {
		switch (validateString(&buffer[0], &buffer[ibuffer - EOL_LEN])) {
		case ValidateResult::Ok:
			eval(&buffer[0], &buffer[ibuffer - EOL_LEN]);

		case ValidateResult::Continue:
			return;

		default:
			resetContext();
		}
	}
}

ValidateResult SerialIndex::validateString(char *s, char *e)
{
	const char fc = *s;
	const bool hasStartQuote = fc == '\'' || fc == '"';
	const bool hasEndQuote = e - s > 1 && *(e - 1) == fc;

	if (hasStartQuote && !hasEndQuote)
		return ValidateResult::Continue;

	return ValidateResult::Ok;
}

bool SerialIndex::evalString(char *s, char *e)
{
	char *now = (char *) values[ikey].now;
	char *before = (char *) values[ikey].before;

	if (*s == '\'' || *s == '"') {
		s += 1;
		e -= 1;
	}

	*e = 0;
	strcpy(now, s);

	if (strcmp(now, before) != 0) {
		strcpy(before, s);
		return true;
	}

	return false;
}
#endif
#endif

#ifdef SERIALINDEX_INT_ARRAY
#ifdef SERIALINDEX_WRITE
bool SerialIndex::writeIntArray()
{
	const size_t length = values[ikey].length;
	const int tolerance = values[ikey].tolerance.i;
	const int *now      = (int *) values[ikey].now;
	int *before         = (int *) values[ikey].before;
	bool changed        = false;
	char *p =  buffer, *q = buffer + BUFFERSIZE;
	size_t i;

	for (i = 0; i < length; i++) {
		if (abs(*(now + i) - *(before + i)) >= tolerance) {
			Serial.println("changed");
			changed = true;
			break;
		}
	}

	if (changed) {
		if (outputFunc) {
			outputFunc(keys[ikey], KV_DELIMITER, now, EOL, Type::IntArray, length);
		} else {
			serial->print(keys[ikey]);
			serial->print(KV_DELIMITER);
			serial->print("[");

			for (i = 0; i < length; i++) {
				serial->print(now[i]);
				if (i < length - 1)
					serial->print(",");
				before[i] = now[i];
			}
		}

		serial->print("]");
		serial->print(EOL);
	}

	return changed;
}
#endif

#ifdef SERIALINDEX_READ
void SerialIndex::readIntArray(char c)
{
	if (c == ']') {
		if (validateIntArray(&buffer[1], &buffer[ibuffer + 1]) == ValidateResult::Ok) {
			eval(&buffer[1], &buffer[ibuffer + 1]);
			return;
		}

		context = Context::Skip;
	}
}

void SerialIndex::readIntSliceArray(char c)
{
	if (c == '}') {
		if (validateIntSliceArray(&buffer[1], &buffer[ibuffer + 1]) == ValidateResult::Ok)
			eval(&buffer[1], &buffer[ibuffer + 1]);

		context = Context::Skip;
	}
}

ValidateResult SerialIndex::validateIntArray(char *s, char *e)
{
	char *pp, *q;
	size_t i = 0;

	for (pp = q = s; q < e; q++) {
		if (*q == ',' || *q == ']') {
			if (validateInt(pp, q) != ValidateResult::Ok)
				return ValidateResult::Invalid;

			pp = q + 1;
			i++;
		}
	}

	return ValidateResult::Ok;
}

ValidateResult SerialIndex::validateIntSliceArray(char *s, char *e)
{
	char *pp, *q;
	size_t i = 0;

	for (pp = q = s; q < e; q++) {
		if (*q == ',' || *q == '}') {
			if (validateIntSlice(pp, q) != ValidateResult::Ok)
				return ValidateResult::Invalid;

			pp = q + 1;
			i++;
		}
	}

	return ValidateResult::Ok;
}

ValidateResult SerialIndex::validateIntSlice(char *s, char *e)
{
	char *p;
	char *dp = 0;
	char *rdp = 0;

	for (p = s; p < e; p++) {
		if (dp)
			return validateInt(p, e);

		if (*p == SLICE_DELIMITER) {
			dp = p;
		} else if (isSliceRangeDelimiter(p)) {
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

bool SerialIndex::evalIntArray(char *s, char *e)
{
	char *p, *q;
	size_t i = 0;
	bool valueChanged = false;

	for (p = q = s; q < e; q++) {
		if (*q == ',' || *q == ']') {
			if (evalIntArrayNth(p, q, i))
				valueChanged = true;
			p = q + 1;
			i++;
		}
	}

	return valueChanged;
}

bool SerialIndex::evalIntArrayNth(char *s, char *e, size_t i)
{
	const int tolerance = values[ikey].tolerance.i;
	int *now = (int *) values[ikey].now;
	int *before = (int *) values[ikey].before;

	if (*s == ']' && *e == ']')
		return false;

	now[i] = atois(s, e);
	if (abs(now[i] - before[i]) >= tolerance) {
		before[i] = now[i];
		return true;
	}

	return false;
}

bool SerialIndex::evalIntSliceArray(char *s, char *e)
{
	char *p, *q;
	size_t i = 0;
	bool valueChanged = false;

	for (p = q = s; q < e; q++) {
		if (*q == ',' || *q == '}') {
			if (evalIntSlice(p, q))
				valueChanged = true;
			p = q + 1;
			i++;
		}
	}

	return valueChanged;
}

bool SerialIndex::evalIntSlice(char *s, char *e)
{
	const int tolerance = values[ikey].tolerance.i;
	char *p, *dp = 0, *rdp = 0;
	int *now = (int *) values[ikey].now;
	int *before = (int *) values[ikey].before;
	int value = 0;
	size_t start = 0, end = 0;
	size_t i;
	bool valueChanged = false;

	for (p = s; p < e; p++) {
		if (*p == SLICE_DELIMITER)
			dp = p;
		else if (isSliceRangeDelimiter(p))
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

	for (i = start; i < end; i++) {
		now[i] = value;
		if (abs(now[i] - before[i]) >= tolerance) {
			before[i] = now[i];
			valueChanged = true;
		}
	}

	return valueChanged;
}
#endif
#endif

#ifdef SERIALINDEX_FLOAT_ARRAY
#ifdef SERIALINDEX_WRITE
bool SerialIndex::writeFloatArray()
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
		if (outputFunc) {
			outputFunc(keys[ikey], KV_DELIMITER, now, EOL, Type::FloatArray, length);
		} else {
			serial->print(keys[ikey]);
			serial->print(KV_DELIMITER);
			serial->print("[");

			for (i = 0; i < length; i++) {
				serial->print(now[i]);
				if (i < length - 1)
					serial->print(",");
				before[i] = now[i];
			}

			serial->print("]");
			serial->print(EOL);
		}
	}

	return changed;
}
#endif

#ifdef SERIALINDEX_READ
void SerialIndex::readFloatArray(char c)
{
	if (c == ']') {
		if (validateFloatArray(&buffer[1], &buffer[ibuffer + 1]) == ValidateResult::Ok) {
			eval(&buffer[1], &buffer[ibuffer + 1]);
			return;
		}

		context = Context::Skip;
	}
}

void SerialIndex::readFloatSliceArray(char c)
{
	if (c == '}') {
		if (validateFloatSliceArray(&buffer[1], &buffer[ibuffer + 1]) == ValidateResult::Ok)
			eval(&buffer[1], &buffer[ibuffer + 1]);

		context = Context::Skip;
	}
}

ValidateResult SerialIndex::validateFloatArray(char *s, char *e)
{
	char *pp, *q;
	size_t i = 0;

	for (pp = q = s; q < e; q++) {
		if (*q == ',' || *q == ']') {
			if (validateFloat(pp, q) != ValidateResult::Ok)
				return ValidateResult::Invalid;
			pp = q + 1;
			i++;
		}
	}

	return ValidateResult::Ok;
}

ValidateResult SerialIndex::validateFloatSliceArray(char *s, char *e)
{
	char *pp, *q;
	size_t i = 0;

	for (pp = q = s; q < e; q++) {
		if (*q == ',' || *q == '}') {
			if (validateFloatSlice(pp, q) != ValidateResult::Ok)
				return ValidateResult::Invalid;
			pp = q + 1;
			i++;
		}
	}

	return ValidateResult::Ok;
}

ValidateResult SerialIndex::validateFloatSlice(char *s, char *e)
{
	char *p;
	char *dp = 0;
	char *rdp = 0;

	for (p = s; p < e; p++) {
		if (dp)
			return validateFloat(p, e);

		if (*p == SLICE_DELIMITER) {
			dp = p;
		} else if (isSliceRangeDelimiter(p)) {
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

bool SerialIndex::evalFloatArray(char *s, char *e)
{
	char *p, *q;
	size_t i = 0;
	bool valueChanged = false;

	for (p = q = s; q < e; q++) {
		if (*q == ',' || *q == ']') {
			if (evalFloatArrayNth(p, q, i))
				valueChanged = true;
			p = q + 1;
			i++;
		}
	}

	return valueChanged;
}

bool SerialIndex::evalFloatArrayNth(char *s, char *e, size_t i)
{
	const float tolerance = values[ikey].tolerance.f;
	float *now = (float *) values[ikey].now;
	float *before = (float *) values[ikey].before;

	if (*s == ']' && *e == ']')
		return false;

	now[i] = strtods(s, e, NULL);
	if (fabs(now[i] - before[i]) >= tolerance) {
		before[i] = now[i];
		return true;
	}

	return false;
}

bool SerialIndex::evalFloatSliceArray(char *s, char *e)
{
	char *p, *q;
	size_t i = 0;
	bool valueChanged = false;

	for (p = q = s; q < e; q++) {
		if (*q == ',' || *q == '}') {
			if (evalFloatSlice(p, q))
				valueChanged = true;
			p = q + 1;
			i++;
		}
	}

	return valueChanged;
}

bool SerialIndex::evalFloatSlice(char *s, char *e)
{
	const float tolerance = values[ikey].tolerance.f;
	char *p, *dp = 0, *rdp = 0;
	float *now = (float *) values[ikey].now;
	float *before = (float *) values[ikey].before;
	float value = 0;
	size_t start = 0, end = 0;
	size_t i;
	bool valueChanged = true;

	for (p = s; p < e; p++) {
		if (*p == SLICE_DELIMITER)
			dp = p;
		else if (isSliceRangeDelimiter(p))
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

	for (i = start; i < end; i++) {
		now[i] = value;
		if (fabs(now[i] - before[i]) >= tolerance) {
			before[i] = now[i];
			valueChanged = true;
		}
	}

	return valueChanged;
}
#endif
#endif

#ifdef SERIALINDEX_READ
void SerialIndex::readInput(char c)
{
	buffer[ibuffer++] = c;
	if (ibuffer >= BUFFERSIZE) {
		ibuffer = 0;
		context = Context::Skip;
	}

	switch (context) {
	case Context::Key:
		readKey(c);
		break;

	case Context::Value:
		readValue(c);
		break;

#ifdef SERIALINDEX_INT
	case Context::IntValue:
		readInt(c);
		break;
#endif

#ifdef SERIALINDEX_FLOAT
	case Context::FloatValue:
		readFloat(c);
		break;
#endif

#ifdef SERIALINDEX_STRING
	case Context::StringValue:
		readString(c);
		break;
#endif

#ifdef SERIALINDEX_ARRAY
	case Context::ArrayValue:
		readArray(c);
		break;

	case Context::SliceArrayValue:
		readSliceArray(c);
		break;
#endif

#ifdef SERIALINDEX_INT_ARRAY
	case Context::IntArrayValue:
		readIntArray(c);
		break;

	case Context::IntSliceArrayValue:
		readIntSliceArray(c);
		break;
#endif

#ifdef SERIALINDEX_FLOAT_ARRAY
	case Context::FloatArrayValue:
		readFloatArray(c);
		break;

	case Context::FloatSliceArrayValue:
		readFloatSliceArray(c);
		break;
#endif

	case Context::Skip:
		readSkip(c);
		break;
	}
}

void SerialIndex::readKey(char c)
{
	if (c == KV_DELIMITER) {
		buffer[ibuffer - 1] = 0;

		ikey = findKey(buffer);
		if (ikey == SIZE_MAX)
			goto skip;

		context = Context::Value;
		ibuffer = 0;
	} else if (!(isalpha(c) || isdigit(c))) {
		goto skip;
	}

	return;

skip:
	context = Context::Skip;
}

void SerialIndex::readValue(char c)
{
	const Type type = types[ikey];

	switch (c) {
	case '0'...'9':
	case '-':
		if (type == Type::Int)
			context = Context::IntValue;
#ifdef SERIALINDEX_FLOAT
		else if (type == Type::Float)
			context = Context::FloatValue;
#endif
		else
			goto skip;
		return;

#ifdef SERIALINDEX_ARRAY
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
#endif

#ifdef SERIALINDEX_STRING
	default:
		if (type != Type::String)
			goto skip;

		context = Context::StringValue;
		return;
#endif

	}

skip:
	context = Context::Skip;
	return;
}

#ifdef SERIALINDEX_ARRAY
void SerialIndex::readArray(char c)
{
	const Type type = types[ikey];

	switch (c) {
	case '0'...'9':
	case '-':
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

void SerialIndex::readSliceArray(char c)
{
	const Type type = types[ikey];

	switch (c) {
	case '0'...'9':
	case '-':
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
#endif

void SerialIndex::readSkip(char c)
{
	if (!isEOL())
		return;

	resetContext();
}

void SerialIndex::eval(char *s, char *e)
{
	bool valueChanged = false;

	switch (context) {

#ifdef SERIALINDEX_INT
	case Context::IntValue:
		valueChanged = evalInt(s, e);
		break;
#endif

#ifdef SERIALINDEX_FLOAT
	case Context::FloatValue:
		valueChanged = evalFloat(s, e);
		break;
#endif

#ifdef SERIALINDEX_STRING
	case Context::StringValue:
		valueChanged = evalString(s, e);
		break;
#endif

#ifdef SERIALINDEX_INT_ARRAY
	case Context::IntArrayValue:
		valueChanged = evalIntArray(s, e);
		break;

	case Context::IntSliceArrayValue:
		valueChanged = evalIntSliceArray(s, e);
		break;
#endif

#ifdef SERIALINDEX_FLOAT_ARRAY
	case Context::FloatArrayValue:
		valueChanged = evalFloatArray(s, e);
		break;

	case Context::FloatSliceArrayValue:
		valueChanged = evalFloatSliceArray(s, e);
		break;
#endif

	default:
		return;
	}

	if (valueChanged && functions[ikey])
		functions[ikey]();

	resetContext();
}

bool SerialIndex::isEOL()
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

void SerialIndex::resetContext(void)
{
	context = Context::Key;
	ibuffer = 0;
	ikey = SIZE_MAX;
}

size_t SerialIndex::findKey(const char *s) 
{
	size_t i;

	for (i = 0; i < nkeys; i++) {
		if (strcmp(keys[i], s) == 0)
			return i;
	}

	return SIZE_MAX;
}
#endif
