// Copyright (c) 2012 Ecma International.  All rights reserved.
// This code is governed by the BSD license found in the LICENSE file.

/*---
es5id: 15.2.3.7-6-a-295
description: >
    Object.defineProperties - 'O' is an Arguments object, 'P' is an
    array index named data property of 'O' but not defined in
    [[ParameterMap]] of 'O', test TypeError is thrown when updating
    the [[Writable]] attribute value of 'P' which is not configurable
    (10.6 [[DefineOwnProperty]] step 4)
includes: [propertyHelper.js]
---*/


var arg;

(function fun() {
  arg = arguments;
}());

Object.defineProperty(arg, "0", {
  value: 0,
  writable: false,
  enumerable: false,
  configurable: false
});

try {
  Object.defineProperties(arg, {
    "0": {
      writable: true
    }
  });

  throw new Test262Error("Expected an exception.");
} catch (e) {
  if (!(e instanceof TypeError)) {
    throw new Test262Error("Expected TypeError, got " + e);
  }
}

verifyProperty(arg, "0", {
  value: 0,
  writable: false,
  enumerable: false,
  configurable: false,
});

reportCompare(0, 0);
