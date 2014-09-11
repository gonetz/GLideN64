#include <assert.h>
#include "../GBI.h"

int MicrocodeDialog(unsigned int /*_crc*/, const char * /*_str*/)
{
	assert(last_good_ucode != (unsigned int)-1 && "Unknown microcode!");
	return last_good_ucode;
}
