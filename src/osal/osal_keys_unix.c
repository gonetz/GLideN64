#include "osal_keys.h"

#ifdef __cplusplus
extern "C" {
#endif

EXPORT unsigned int CALL osal_is_key_pressed(unsigned int _key, unsigned int _mask)
{
	return 0;
}

EXPORT unsigned int CALL osal_virtual_key_to_hid(unsigned int _key)
{
	return 0;
}

EXPORT const char * CALL osal_keycode_name(unsigned int _hidCode)
{
	return NULL;
}

#ifdef __cplusplus
}
#endif
