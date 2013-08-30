/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "js/Id.h"

#ifdef JS_USE_JSID_STRUCT_TYPES
const jsid JSID_VOID  = { size_t(JSID_TYPE_VOID) };
const jsid JSID_EMPTY = { size_t(JSID_TYPE_OBJECT) };
#endif

