/* synthtoys DSSI software synthesizer framework
 *
 * Copyright (C) 2011 Sean Bolton.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef _DEBUG_H
#define _DEBUG_H

/* DSSP_DEBUG bits */
#define DB_DSSI    1   /* DSSI interface */
#define DB_AUDIO   2   /* audio output */
#define DB_NOTE    4   /* note on/off, voice allocation */
#define DB_DATA    8   /* plugin patchbank handling */
#define DB_MAIN   16   /* GUI main program flow */
#define DB_OSC    32   /* GUI OSC handling */
#define DB_IO     64   /* GUI patch file input/output */
#define DB_GUI   128   /* GUI GUI callbacks, updating, etc. */

/* If you want debug information, define DSSP_DEBUG to the DB_* bits you're
 * interested in getting debug information about, bitwise-ORed together.
 * Otherwise, leave it undefined. */
// #define DSSP_DEBUG (1+8+16+32+64)
// #define DSSP_DEBUG (1+16+32+64+128) /* !FIX! */

#ifdef DSSP_DEBUG

#include <stdio.h>
#define DSSP_DEBUG_INIT(x)
#define DEBUG_MESSAGE(type, fmt...) { if (DSSP_DEBUG & type) { fprintf(stderr, "%s", plugin_name); fprintf(stderr, fmt); } }
#define GUIDB_MESSAGE(type, fmt...) { if (DSSP_DEBUG & type) { fprintf(stderr, "%s", plugin_gui_name); fprintf(stderr, fmt); } }
// -FIX-:
// #include "message_buffer.h"
// #define DSSP_DEBUG_INIT(x)  mb_init(x)
// #define DEBUG_MESSAGE(type, fmt...) { \-
//     if (DSSP_DEBUG & type) { \-
//         char _m[256]; \-
//         snprintf(_m, 255, fmt); \-
//         add_message(_m); \-
//     } \-
// }

#else  /* !DSSP_DEBUG */

#define DSSP_DEBUG_INIT(x)
#define DEBUG_MESSAGE(type, fmt...)
#define GUIDB_MESSAGE(type, fmt...)

#endif  /* DSSP_DEBUG */

#endif /* _DEBUG_H */

