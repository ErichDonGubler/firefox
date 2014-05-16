const Cu = Components.utils;

function run_test() {
  // Existing module.
  do_check_true(!Cu.isModuleLoaded("resource://gre/modules/devtools/LayoutHelpers.jsm"),
                "isModuleLoaded returned correct value for non-loaded module");
  Cu.import("resource://gre/modules/devtools/LayoutHelpers.jsm");
  do_check_true(Cu.isModuleLoaded("resource://gre/modules/devtools/LayoutHelpers.jsm"),
                "isModuleLoaded returned true after loading that module");
  Cu.unload("resource://gre/modules/devtools/LayoutHelpers.jsm");
  do_check_true(!Cu.isModuleLoaded("resource://gre/modules/devtools/LayoutHelpers.jsm"),
                "isModuleLoaded returned false after unloading that module");

  // Non-existing module
  do_check_true(!Cu.isModuleLoaded("resource://gre/modules/devtools/LayoutHelpers1.jsm"),
                "isModuleLoaded returned correct value for non-loaded module");
  try {
    Cu.import("resource://gre/modules/devtools/LayoutHelpers1.jsm");
    do_check_true(false,
                  "Should have thrown while trying to load a non existing file");
  } catch (ex) {}
  do_check_true(!Cu.isModuleLoaded("resource://gre/modules/devtools/LayoutHelpers1.jsm"),
                "isModuleLoaded returned correct value for non-loaded module");

  // incorrect url
  try {
    Cu.isModuleLoaded("resource://modules/devtools/LayoutHelpers1.jsm");
    do_check_true(false,
                  "Should have thrown while trying to load a non existing file");
  } catch (ex) {
    do_check_true(true, "isModuleLoaded threw an exception while loading incorrect uri");
  }
}
