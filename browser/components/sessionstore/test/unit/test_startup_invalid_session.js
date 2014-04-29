/* Any copyright is dedicated to the Public Domain.
   http://creativecommons.org/publicdomain/zero/1.0/ */

function run_test() {
  let profd = do_get_profile();

  let sourceSession = do_get_file("data/sessionstore_invalid.js");
  sourceSession.copyTo(profd, "sessionstore.js");

  let sourceCheckpoints = do_get_file("data/sessionCheckpoints_all.json");
  sourceCheckpoints.copyTo(profd, "sessionCheckpoints.json");

  do_test_pending();
  let startup = Cc["@mozilla.org/browser/sessionstartup;1"].
    getService(Ci.nsISessionStartup);

  afterSessionStartupInitialization(function cb() {
    do_check_eq(startup.sessionType, Ci.nsISessionStartup.NO_SESSION);
    do_test_finished();
  });
}
