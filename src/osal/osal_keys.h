#if !defined(OSAL_KEYS_H)
#define OSAL_KEYS_H

#include "osal_export.h"
#include "keycode/keycode.h"

#ifdef __cplusplus
extern "C" {
#endif

EXPORT unsigned int CALL osal_is_key_pressed(unsigned int _key, unsigned int _mask);

EXPORT unsigned int CALL osal_virtual_key_to_hid(unsigned int _key);

EXPORT const char * CALL osal_keycode_name(unsigned int _hidCode);

#ifdef __cplusplus
}
#endif

#endif /* #define OSAL_KEYS_H */

