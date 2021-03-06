# See the file LICENSE for redistribution information.
#
# Copyright (c) 2004-2005
#	Sleepycat Software.  All rights reserved.
#
# $Id: rep029.tcl,v 12.10 2005/11/04 20:57:04 carol Exp $
#
# TEST	rep029
# TEST	Test of internal initialization.
# TEST
# TEST	One master, one client.
# TEST	Generate several log files.
# TEST	Remove old master log files.
# TEST	Delete client files and restart client.
# TEST	Put one more record to the master.
#
proc rep029 { method { niter 200 } { tnum "029" } args } {

	source ./include.tcl
	if { $is_windows9x_test == 1 } { 
		puts "Skipping replication test on Win 9x platform."
		return
	} 
	global passwd
	global has_crypto

	set args [convert_args $method $args]
	set saved_args $args

	# Rep057 runs rep029 with in-memory named databases.
	set inmem 0
	set msg "using regular named databases"
	if { $tnum == "057" } {
		set inmem 1
		set msg "using in-memory named databases"
		if { [is_queueext $method] == 1 } {
			puts "Skipping rep$tnum for method $method"
			return
		}
	}
		
	# This test needs to set its own pagesize.
	set pgindex [lsearch -exact $args "-pagesize"]
	if { $pgindex != -1 } {
		puts "Rep$tnum: skipping for specific pagesizes"
		return
	}

	set logsets [create_logsets 2]

	# Run the body of the test with and without recovery,
	# and with and without cleaning.  Skip recovery with in-memory
	# logging - it doesn't make sense.
	set recopts { "" "-recover" }
	set opts { bulk clean noclean }
	foreach r $recopts {
		foreach c $opts {
			foreach l $logsets {
				set logindex [lsearch -exact $l "in-memory"]
				if { $r == "-recover" && $logindex != -1 } {
					puts "Skipping rep$tnum for -recover\
					    with in-memory logs."
					continue
				}
				set envargs ""
				set args $saved_args
				puts "Rep$tnum ($method $envargs $r $c $args):\
				    Test of internal initialization with $msg."
				puts "Rep$tnum: Master logs are [lindex $l 0]"
				puts "Rep$tnum: Client logs are [lindex $l 1]"
				rep029_sub $method $niter $tnum $envargs \
				    $l $r $c $inmem $args	

				# Skip encrypted tests if not supported.
				if { $has_crypto == 0 || $inmem } {
					continue
				}	

				# Run same set of tests with security.
				#
				append envargs " -encryptaes $passwd "
				append args " -encrypt "
				puts "Rep$tnum ($method $envargs $r $c $args):\
				    Test of internal initialization with $msg."
				puts "Rep$tnum: Master logs are [lindex $l 0]"
				puts "Rep$tnum: Client logs are [lindex $l 1]"
				rep029_sub $method $niter $tnum $envargs \
				    $l $r $c $inmem $args
			}
		}
	}
}

proc rep029_sub { method niter tnum envargs logset recargs opts inmem largs } {
	global testdir
	global passwd
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

	set m_logtype [lindex $logset 0]
	set c_logtype [lindex $logset 1]

	# In-memory logs cannot be used with -txn nosync.  
	set m_txnargs [adjust_txnargs $m_logtype]
	set c_txnargs [adjust_txnargs $c_logtype]
	
	# Open a master.
	repladd 1
	set ma_envcmd "berkdb_env_noerr -create $m_txnargs \
	    -log_buffer $log_buf -log_max $log_max $envargs \
	    -home $masterdir -rep_transport \[list 1 replsend\]"
#	set ma_envcmd "berkdb_env_noerr -create $m_txnargs \
#	    -log_buffer $log_buf -log_max $log_max $envargs \
#	    -verbose {rep on} -errpfx MASTER -errfile /dev/stderr \
#	    -home $masterdir -rep_transport \[list 1 replsend\]"
	set masterenv [eval $ma_envcmd $recargs -rep_master]
	error_check_good master_env [is_valid_env $masterenv] TRUE

	# Open a client
	repladd 2
	set cl_envcmd "berkdb_env_noerr -create $c_txnargs \
	    -log_buffer $log_buf -log_max $log_max $envargs \
	    -home $clientdir -rep_transport \[list 2 replsend\]"
#	set cl_envcmd "berkdb_env_noerr -create $c_txnargs \
#	    -log_buffer $log_buf -log_max $log_max $envargs \
#	    -verbose {rep on} -errpfx CLIENT -errfile /dev/stderr \
#	    -home $clientdir -rep_transport \[list 2 replsend\]"
	set clientenv [eval $cl_envcmd $recargs -rep_client]
	error_check_good client_env [is_valid_env $clientenv] TRUE

	# Bring the clients online by processing the startup messages.
	set envlist "{$masterenv 1} {$clientenv 2}"
	process_msgs $envlist

	# Run rep_test in the master (and update client).
	puts "\tRep$tnum.a: Running rep_test in replicated env."
	eval rep_test $method $masterenv NULL $niter 0 0 0 $inmem $largs
	process_msgs $envlist 0 NONE err
	error_check_good process_msgs $err 0

	puts "\tRep$tnum.b: Close client."
	error_check_good client_close [$clientenv close] 0

	if { [lsearch $envargs "-encrypta*"] !=-1 } {
		set enc "-P $passwd"
	} else {
		set enc ""
	}

	# Find out what exists on the client.  We need to loop until
	# the first master log file > last client log file.
	set res [eval exec $util_path/db_archive $enc -l -h $clientdir]

	set last_client_log [lindex [lsort $res] end]

	set stop 0
	while { $stop == 0 } {
		# Run rep_test in the master (don't update client).
		puts "\tRep$tnum.c: Running rep_test in replicated env."
		eval rep_test $method $masterenv NULL $niter 0 0 0 $inmem $largs
		replclear 2

		puts "\tRep$tnum.d: Run db_archive on master."
		set res [eval exec $util_path/db_archive $enc -d -h $masterdir]
		set res [eval exec $util_path/db_archive $enc -l -h $masterdir]
		if { [lsearch -exact $res $last_client_log] == -1 } {
			set stop 1
		}
	}

	puts "\tRep$tnum.e: Reopen client ($opts)."
	if { $opts == "clean" } {
		env_cleanup $clientdir
	}
	if { $opts == "bulk" } {
		error_check_good bulk [$masterenv rep_config {bulk on}] 0
	}
	set clientenv [eval $cl_envcmd $recargs -rep_client]
	error_check_good client_env [is_valid_env $clientenv] TRUE
	set envlist "{$masterenv 1} {$clientenv 2}"
	process_msgs $envlist 0 NONE err
	error_check_good process_msgs $err 0
	if { $opts != "clean" } {
		puts "\tRep$tnum.e.1: Trigger log request"
		#
		# When we don't clean, starting the client doesn't
		# trigger any events.  We need to generate some log
		# records so that the client requests the missing
		# logs and that will trigger it.
		#
		set entries 10
		eval rep_test\
		    $method $masterenv NULL $entries $niter 0 0 $inmem $largs
		process_msgs $envlist 0 NONE err
		error_check_good process_msgs $err 0
	}

	puts "\tRep$tnum.f: Verify databases"
	#
	# If doing bulk testing, turn it off now so that it forces us
	# to flush anything currently in the bulk buffer.  We need to
	# do this because rep_test might have aborted a transaction on
	# its last iteration and those log records would still be in
	# the bulk buffer causing the log comparison to fail.
	#
	if { $opts == "bulk" } {
		puts "\tRep$tnum.f.1: Turn off bulk transfers."
		error_check_good bulk [$masterenv rep_config {bulk off}] 0
		process_msgs $envlist 0 NONE err
		error_check_good process_msgs $err 0
	}

	#
	# !!! This test CANNOT use rep_verify due to encryption.
	# Just compare databases.  We either have to copy in
	# all the code in rep_verify to adjust the beginning LSN
	# or skip the log check for just this one test.
	if { $inmem == 1 } {
		set dbname { "" "test.db" }
	} else {
		set dbname "test.db"
	}
	set db1 [eval {berkdb_open -env $masterenv} $largs -rdonly $dbname]
	set db2 [eval {berkdb_open -env $clientenv} $largs -rdonly $dbname]

	error_check_good comparedbs [db_compare \
	    $db1 $db2 $masterdir/$dbname $clientdir/$dbname] 0
	error_check_good db1_close [$db1 close] 0
	error_check_good db2_close [$db2 close] 0

	# Add records to the master and update client.
	puts "\tRep$tnum.g: Add more records and check again."
	set entries 10
	eval rep_test $method $masterenv NULL $entries $niter 0 0 $inmem $largs
	process_msgs $envlist 0 NONE err
	error_check_good process_msgs $err 0

	# Check again that master and client logs and dbs are identical.
	set db1 [eval {berkdb_open -env $masterenv} $largs -rdonly $dbname]
	set db2 [eval {berkdb_open -env $clientenv} $largs -rdonly $dbname]

	error_check_good comparedbs [db_compare \
	    $db1 $db2 $masterdir/$dbname $clientdir/$dbname] 0
	error_check_good db1_close [$db1 close] 0
	error_check_good db2_close [$db2 close] 0
        set bulkxfer [stat_field $masterenv rep_stat "Bulk buffer transfers"]
	if { $opts == "bulk" } {
		error_check_bad bulkxferon $bulkxfer 0
	} else {
		error_check_good bulkxferoff $bulkxfer 0
	}

	error_check_good masterenv_close [$masterenv close] 0
	error_check_good clientenv_close [$clientenv close] 0
	replclose $testdir/MSGQUEUEDIR
}
