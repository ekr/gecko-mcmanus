// Copyright (c) 2012 Ecma International.  All rights reserved.
// This code is governed by the BSD license found in the LICENSE file.

/*---
es5id: 11.8.3-5
description: >
    11.8.3 Less-than-or-equal Operator - Partial left to right order
    enforced when using Less-than-or-equal operator: valueOf <= valueOf
---*/

        var accessed = false;
        var obj1 = {
            valueOf: function () {
                accessed = true;
                return 3;
            }
        };
        var obj2 = {
            valueOf: function () {
                if (accessed === true) {
                    return 3;
                } else {
                    return 2;
                }
            }
        };

assert((obj1 <= obj2), '(obj1 <= obj2) !== true');

reportCompare(0, 0);
