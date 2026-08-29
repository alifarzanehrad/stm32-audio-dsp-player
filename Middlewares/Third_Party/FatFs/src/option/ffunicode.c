/*
 * Minimal FatFs Unicode helpers for ASCII/English long filenames.
 *
 * The audio player accepts English WAV filenames. FatFs requires these two
 * hooks whenever long filename support is enabled. Characters outside ASCII
 * are rejected during OEM conversion instead of being converted incorrectly.
 */

#include "ff.h"

WCHAR ff_convert(WCHAR character, UINT direction)
{
    (void)direction;

    if (character < 0x80U)
    {
        return character;
    }

    return 0U;
}

WCHAR ff_wtoupper(WCHAR character)
{
    if ((character >= (WCHAR)'a') &&
        (character <= (WCHAR)'z'))
    {
        character -= (WCHAR)('a' - 'A');
    }

    return character;
}
