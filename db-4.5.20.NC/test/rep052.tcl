# See the file LICENSE for redistribution information.
#
# Copyright (c) 2004-2006
#	Oracle Corporation.  All rights reserved.
#
# $Id: rep052.tcl,v 12.9 2006/08/24 14:46:38 bostic Exp $
#
# TEST	rep052
# TEST	Test of replication with NOWAIT.
# TEST
# TEST	One master, one client.  After initializing
# TEST	everything normally, close client and let the
# TEST	master get ahead -- far enough that the master
# TEST 	no longer has the client's last log file.
# TEST	Reopen the client and turn on NOWAIT.
# TEST	Process a few messages to get the client into
# TEST	recovery mode, and verify that lockout occurs
# TEST 	on a txn API call (txn_begin) and an env API call.
# TEST	Process all the messages and verify that lockout
# TEST 	is over.

proc rep052 { method { niter 200 } { tnum "052" } args } {

	source ./include.tcl
	if { $is_windows9x_test == 1 } {
		puts "Skipping replication test on Win 9x platform."
		return
	}

	# Valid for all access methods.
	if { $checking_valid_methods } {
		return "ALL"
	}

	set args [convert_args $method $args]
	set saved_args $args

	# This test needs to set its own pagesize.
	set pgindex [lsearch -exact $args "-pagesize"]
	if { $pgindex != -1 } {
		puts "Rep$tnum: skipping for specific pagesizes"
		return
	}

	set logsets [create_logsets 2]
	set saved_args $args

	# Run the body of the test with and without recovery.  Skip
	# recovery with in-memory logging - it doesn't make sense.
	foreach r $test_recopts {
		foreach l $logsets {
			set logindex [lsearch -exact $l "in-memory"]
			if { $r == "-recover" && $logindex != -1 } {
				puts "Skipping rep$tnum for -recover\
				    with in-memory logs."
				continue
			}
			set envargs ""
			set args $saved_args
			puts "Rep$tnum ($method $envargs $r $args):\
			    Test lockouts with REP_NOWAIT."
			puts "Rep$tnum: Master logs are [lindex $l 0]"
			puts "Rep$tnum: Client logs are [lindex $l 1]"
			rep052_sub $method $niter $tnum $envargs \
			    $l $r $args
		}
	}
}

proc rep052_sub { method niter tnum envargs logset recargs largs } {
	global testdir
	global util_path

	env_cleanup $testdir

	replsetup $testdir/MSGQUEUEDIR

	set masterdir $testdir/MASTERDIR
	set clientdir $testdir/CLIENTDIR

	file mkdir $masterdir
	file mkdir $clientdir

	# Log size is small so we quickly create more than one.
	# The documentation says that the log file must be at least
	# four times the size of the in-memory log buffer.
	set pagesize 4096
	append largs " -pagesize $pagesize "
	set log_buf [expr $pagesize * 2]
	set log_max [expr $log_buf * 4]
	set m_logargs " -log_buffer $log_buf"
	set c_logargs " -log_buffer $log_buf"

	set m_logtype [lindex $logset 0]
	set c_logtype [lindex $logset 1]

	# In-memory logs cannot be used with -txn nosync.
	set m_txnargs [adjust_txnargs $m_logtype]
	set c_txnargs [adjust_txnargs $c_logtype]
	set m_logargs [adjust_logargs $m_logtype]
	set c_logargs [adjust_logargs $c_logtype]

	# Open a master.
	repladd 1
	set ma_envcmd "berkdb_env_noerr -create $m_txnargs \
	    $m_logargs -log_max $log_max $envargs \
	    -home $masterdir -rep_transport \[list 1 replsend\]"
#	set ma_envcmd "berkdb_env_noerr -create $m_txnargs \
#	    $m_logargs -log_max $log_max $envargs \
#	    -verbose {rep on} -errpfx MASTER \
#	    -home $masterdir -rep_transport \[list 1 replsend\]"
	set masterenv [eval $ma_envcmd $recargs -rep_master]
	error_check_good master_env [is_valid_env $masterenv] TRUE

	# Open a client
	repladd 2
	set cl_envcmd "berkdb_env_noerr -create $c_txnargs \
	    $c_logargs -log_max $log_max $envargs \
	    -home $clientdir -rep_transport \[list 2 replsend\]"
#	set cl_envcmd "berkdb_env_noerr -create $c_txnargs \
#	    $c_logargs -log_max $log_max $envargs \
#	    -verbose {rep on} -errpfx CLIENT \
#	    -home $clientdir -rep_transport \[list 2 replsend\]"
	set clientenv [eval $cl_envcmd $recargs -rep_client]
	error_check_good client_env [is_valid_env $clientenv] TRUE

	# Bring the clients online by processing the startup messages.
	set envlist "{$masterenv 1} {$clientenv 2}"
	process_msgs $envlist

	# Run rep_test in the master (and update client).
	puts "\tRep$tnum.a: Running rep_test in replicated env."
	set start 0
	eval rep_test $method $masterenv NULL $niter $start $start 0 0 $largs
	incr start $niter
	process_msgs $envlist

	# Find out what exists on the client before closing.  We'll need
	# to loop until the first master log file > last client log file.
	set last_client_log [get_logfile $clientenv last]

	puts "\tRep$tnum.b: Close client."
	error_check_good client_close [$clientenv close] 0

	# Find out what exists on the client.  We need to loop until
	# the first master log file > last client log file.

	set stop 0
	while { $stop == 0 } {
		# Run rep_test in the master (don't update client).
		puts "\tRep$tnum.c: Running rep_test in replicated env."
		eval rep_test \
		    $method $masterenv NULL $niter $start $start 0 0 $largs
		incr start $niter
		replclear 2

		puts "\tRep$tnum.d: Run db_archive on master."
		if { $m_logtype != "in-memory" } {
			set res \
			    [eval exec $util_path/db_archive -d -h $masterdir]
		}
		# Make sure we have a gap between the last client log and
		# the first master log.  This is easy with on-disk logs, since
		# we archive, but will take longer with in-memory logging.
		set first_master_log [get_logfile $masterenv first]
		if { $first_master_log > $last_client_log } {
			set stop 1
		}
	}

	puts "\tRep$tnum.e: Reopen client."
	env_cleanup $clientdir
	set clientenv [eval $cl_envcmd $recargs -rep_client]
	error_check_good client_env [is_valid_env $clientenv] TRUE
	set envlist "{$masterenv 1} {$clientenv 2}"

	# Turn on nowait.
	$clientenv rep_config {nowait on}

	# Process messages a few times, just enough to get client
	# into lockout/recovery mode, but not enough to complete recovery.
	set iter 4
	for { set i 0 } { $i < $iter } { incr i } {
		set nproced [proc_msgs_once $envlist NONE err]
	}

	puts "\tRep$tnum.f: Verify we are locked out of txn API calls."
	if { [catch { set txn [$clientenv txn] } res] } {
		error_check_good txn_lockout [is_substr $res "locked out"] 1
	} else {
		error_check_good txn_no_lockout [$txn commit] 0
		puts "FAIL: Not locked out of txn API calls: $res"
	}

	puts "\tRep$tnum.g: Verify we are locked out of env API calls."
	if { [catch { set stat [$clientenv lock_stat] } res] } {
		error_check_good env_lockout [is_substr $res "locked out"] 1
	} else {
		puts "FAIL: Not locked out of env API calls: $res"
	}

	# Now catch up and make sure we're not locked out anymore.
	process_msgs $envlist

	puts "\tRep$tnum.h: No longer locked out of txn API calls."
	if { [catch { set txn [$clientenv txn] } res] } {
		puts "FAIL: unable to start txn: $res"
	} else {
		error_check_good txn_no_lockout [$txn commit] 0
	}

	puts "\tRep$tnum.i: No longer locked out of env API calls."
	if { [catch { set stat [$clientenv rep_stat] } res] } {
		puts "FAIL: unable to make env call: $res"
	}

	puts "\tRep$tnum.h: Verify logs and databases"
	rep_verify $masterdir $masterenv $clientdir $clientenv 1

	error_check_good masterenv_close [$masterenv close] 0
	error_check_good clientenv_close [$clientenv close] 0
	replclose $testdir/MSGQUEUEDIR
}
