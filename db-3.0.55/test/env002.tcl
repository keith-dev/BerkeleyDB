# See the file LICENSE for redistribution information.
#
# Copyright (c) 1999
#	Sleepycat Software.  All rights reserved.
#
#	@(#)env002.tcl	11.2 (Sleepycat) 9/7/99
#
# Env Test 002
# Test DB_LOG_DIR and env name resolution
# With an environment path specified using -home, and then again
# with it specified by the environment variable DB_HOME:
# 	1) Make sure that the DB_LOG_DIR config file option is respected
#		a) as a relative pathname.
#		b) as an absolute pathname.
#	2) Make sure that the DB_LOG_DIR db_config argument is respected,
#		again as relative and absolute pathnames.
#	3) Make sure that if -both- db_config and a file are present,
#		only the file is respected (see doc/env/naming.html).
proc env002 { } {
	#   env002 is essentially just a small driver that runs
	# env002_body--formerly the entire test--twice;  once, it
	# supplies a "home" argument to use with environment opens,
	# and the second time it sets DB_HOME instead.
	#   Note that env002_body itself calls env002_run_test to run
	# the body of the actual test and check for the presence
	# of logs.  The nesting, I hope, makes this test's structure simpler.

	global env
	source ./include.tcl

	puts "Env002: DB_LOG_DIR test."

	puts "\tEnv002: Running with -home argument to berkdb env."
	env002_body "-home $testdir"

	puts "\tEnv002: Running with environment variable DB_HOME set."
	set env(DB_HOME) $testdir
	env002_body "-use_environ"

	unset env(DB_HOME)

	puts "\tEnv002: Running with both DB_HOME and -home set."
	# Should respect -only- -home, so we give it a bogus
	# environment variable setting.
	set env(DB_HOME) $testdir/bogus_home
	env002_body "-use_environ -home $testdir"
	unset env(DB_HOME)

}

proc env002_body { home_arg } {
	source ./include.tcl


	cleanup $testdir
	set logdir "logs_in_here"

	exec $MKDIR $testdir/$logdir

	# Set up full path to $logdir for when we test absolute paths.
	set curdir [pwd]
	cd $testdir/$logdir
	set fulllogdir [pwd]
	cd $curdir

	env002_make_config $logdir

	# Run the meat of the test.
	env002_run_test a 1 "relative path, config file" $home_arg \
		$testdir/$logdir

	cleanup $testdir

	exec $MKDIR $fulllogdir
	env002_make_config $fulllogdir

	# Run the test again
	env002_run_test a 2 "absolute path, config file" $home_arg \
		$fulllogdir

	cleanup $testdir

	# Now we try without a config file, but instead with db_config
	# relative paths
	exec $MKDIR $testdir/$logdir
	env002_run_test b 1 "relative path, db_config" "$home_arg \
		-config {{DB_LOG_DIR $logdir} {DB_DATA_DIR .}}" \
		$testdir/$logdir

	cleanup $testdir

	# absolute
	exec $MKDIR $fulllogdir
	env002_run_test b 2 "absolute path, db_config" "$home_arg \
		-config {{DB_LOG_DIR $fulllogdir} {DB_DATA_DIR .}}" \
		$fulllogdir

	cleanup $testdir

	# Now, set db_config -and- have a # DB_CONFIG file, and make
	# sure only the latter is honored.

	exec $MKDIR $testdir/$logdir
	env002_make_config $logdir

	# note that we supply a -nonexistent- log dir to db_config
	env002_run_test c 1 "relative path, both db_config and file" \
		"$home_arg -config {{DB_LOG_DIR $testdir/bogus} \
		{DB_DATA_DIR .}}" $testdir/$logdir
	cleanup $testdir


	exec $MKDIR $fulllogdir
	env002_make_config $fulllogdir

	# note that we supply a -nonexistent- log dir to db_config
	env002_run_test c 2 "relative path, both db_config and file" \
		"$home_arg -config {{DB_LOG_DIR $fulllogdir/bogus} \
		{DB_DATA_DIR .}}" $fulllogdir


}

proc env002_run_test { major minor msg env_args log_path} {
	global testdir
	set testfile "env002.db"

	puts "\t\tEnv002.$major.$minor: $msg"

	# Create an environment, with logging, and scribble some
	# stuff in a [btree] database in it.
	# puts [concat {berkdb env -create -log -mpool -private} $env_args]
	set dbenv [eval {berkdb env -create -log -mpool -private} $env_args]
	error_check_good env_open [is_valid_env $dbenv] TRUE
	set db [berkdb open -env $dbenv -create -btree -mode 0644 $testfile]
	error_check_good db_open [is_valid_db $db] TRUE

	set key "some_key"
	set data "some_data"

	error_check_good db_put \
		[$db put $key [chop_data btree $data]] 0

	error_check_good db_close [$db close] 0
	error_check_good env_close [$dbenv close] 0

	# Now make sure the log file is where we want it to be.
	error_check_good db_exists [file exists $testdir/$testfile] 1
	error_check_good log_exists \
		[file exists $log_path/log.0000000001] 1
}

proc env002_make_config { logdir } {
	global testdir

	set cid [open $testdir/DB_CONFIG w]
	puts $cid "DB_DATA_DIR ."
	puts $cid "DB_LOG_DIR $logdir"
	close $cid
}
