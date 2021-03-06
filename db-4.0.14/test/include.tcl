set tclsh_path @TCL_TCLSH@
set tcllib .libs/libdb_tcl-@DB_VERSION_MAJOR@.@DB_VERSION_MINOR@.@SOSUFFIX@
set rpc_server localhost
set rpc_path .
set src_root @srcdir@/..
set test_path @srcdir@/../test
global testdir
set testdir ./TESTDIR

set KILL "@db_cv_path_kill@"

# DO NOT EDIT BELOW THIS LINE: automatically built by dist/s_test.

global dict
global util_path
set rpc_testdir $rpc_path/TESTDIR

global is_hp_test
global is_qnx_test
global is_windows_test
