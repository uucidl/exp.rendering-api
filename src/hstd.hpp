#pragma once

// things I hope to see in the standard one day

/// designated field like in c99 (right now just a marker for documentation)
#define HSTD_DFIELD(name, value) value

/**
 * documented thruth value
 *
 * used to document parameters which are boolean
 */
#define HSTD_NTRUE(name) true
