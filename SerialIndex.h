#ifndef SERIAL_INDEX_H
#define SERIAL_INDEX_H

// uncomment one of the following flags to disable read or write capability
#define SERIALINDEX_READ
#define SERIALINDEX_WRITE

#include "IO.hpp"

enum Mode {
	Read = 1,
	Write = 2,
};

class SerialIndex : public IO
{
public:
	SerialIndex(Stream &s);
	~SerialIndex();

	SerialIndex&   ping(char *k);

	SerialIndex&   begin(void);
	SerialIndex&   begin(long);
	SerialIndex&   begin(long, int);
	SerialIndex&   begin(long, int, int);

	void           update(void);

#ifdef SERIALINDEX_READ
	SerialIndex&   in();
	SerialIndex&   read(bool);
#endif

#ifdef SERIALINDEX_WRITE
	SerialIndex&   out();
	SerialIndex&   write(bool);
#endif

private:
	Stream&        serial;
	int            mode;
};

extern SerialIndex Index;

#endif // SERIAL_INDEX_H
