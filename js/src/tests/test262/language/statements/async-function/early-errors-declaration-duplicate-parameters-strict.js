// |reftest| error:SyntaxError
'use strict';
// Copyright 2016 Microsoft, Inc. All rights reserved.
// This code is governed by the BSD license found in the LICENSE file.

/*---
author: Brian Terlson <brian.terlson@microsoft.com>
esid: pending
description: >
  If strict mode, early error rules for StrictFormalParameters are applied
negative:
  phase: early
  type: SyntaxError
flags: [onlyStrict]
---*/

async function foo(a, a) { }
