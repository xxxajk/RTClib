/*
 * TO-DO: add intel, esp8266, and others....
 */


#ifndef _ALL_ATOMIC_

#define _ALL_ATOMIC_
#if defined(__PIC32__)
#include <wiring.h>
#include "PIC32_ATOMIC.h"
#elif !defined(CORE_TEENSY) && defined(__arm__)
/* arduino ARM */
#include "ARM_ATOMIC.h"
#else
/* avr, and teensy arm */
#include <util/atomic.h>
#endif

#endif
