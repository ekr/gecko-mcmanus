/* -*- indent-tabs-mode: nil; js-indent-level: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

/**
 * Test with sessionCheckpoints.json containing valid data
 */
add_task(async function test_valid_file() {
  // Write valid data to checkpoint file
  let data = JSON.stringify({"final-ui-startup": true});
  await OS.File.writeAtomic(sessionCheckpointsPath, data,
                            {tmpPath: sessionCheckpointsPath + ".tmp"});

  CrashMonitor.init();
  let checkpoints = await CrashMonitor.previousCheckpoints;

  do_check_true(checkpoints["final-ui-startup"]);
  do_check_eq(Object.keys(checkpoints).length, 1);
});
