#include "SerialIndex.h"
#include "IO.hpp"

SerialIndex::SerialIndex(Stream &s) :
	serial(s)
{
	begin();

	mode = 0;
}

SerialIndex::~SerialIndex() 
{
}

SerialIndex& SerialIndex::ping(char *k)
{
	// TODO
	return *this; 
}

SerialIndex& SerialIndex::begin(void)
{
	return begin(BAUDRATE, CAPACITY, BUFFERSIZE);
}

SerialIndex& SerialIndex::begin(long theBaudrate)
{
	return begin(theBaudrate, CAPACITY, BUFFERSIZE);
}

SerialIndex& SerialIndex::begin(long theBaudrate, int theCapacity)
{
	return begin(theBaudrate, theCapacity, BUFFERSIZE);
}

SerialIndex& SerialIndex::begin(long theBaudrate, int theCapacity, int theBufferSize)
{
	Serial.begin(theBaudrate);
	serial = Serial;

	// TODO

	return *this;
}

SerialIndex& SerialIndex::io(const char *k, bool theIn, bool theOut) 
{
	// TODO
	return *this;
}

SerialIndex& SerialIndex::in()
{
	while (serial.available())
		IO::write(serial.read());

	return *this;
}

SerialIndex& SerialIndex::out()
{
	while (IO::check_value_updates()) {
		while (IO::available())
			Serial.write(IO::read());
	}

	IO::reset_context();

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

SerialIndex& SerialIndex::write(bool b)
{
	if (b)
		mode |= Mode::Write;
	else
		mode &= ~Mode::Write;

	return *this;
}

void SerialIndex::update(void)
{
	if ((mode & Mode::Read) != 0)
		in();

	if ((mode & Mode::Write) != 0)
		out();
}

SerialIndex Index(Serial);
