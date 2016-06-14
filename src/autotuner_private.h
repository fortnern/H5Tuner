/*
* Copyright by The HDF Group.
* All rights reserved.
*
* This file is part of h5tuner. The full h5tuner copyright notice,
* including terms governing use, modification, and redistribution, is
* contained in the file COPYING, which can be found at the root of the
* source code distribution tree.  If you do not have access to this file,
* you may request a copy from help@hdfgroup.org.
*/

#ifndef _autotuner_private_H
#define _autotuner_private_H

/* Global includes */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pwd.h>
#include <assert.h>
#include "mpi.h"
#include "hdf5.h"
#include "autotuner.h"
#include "mxml.h"

/* Error macros */
#define SUCCEED 0
#define FAIL -1
#define ERROR(MSG) \
do { \
    fprintf(stderr, "FAILED in " __FILE__ " at line %d:\n  " MSG "\n", __LINE__); \
    ret_value = FAIL; \
    goto done; \
} while(0)

#endif /* _autotuner_private_H */

