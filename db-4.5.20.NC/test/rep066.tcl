# See the file LICENSE for redistribution information.
#
# Copyright (c) 2001-2006
#	Oracle Corporation.  All rights reserved.
#
# $Id: rep066.tcl,v 12.4 2006/09/08 20:32:18 bostic Exp $
#
# TEST	rep066
# TEST	Replication and dead log handles.
# TEST
# TEST	Run rep_test on master and a client.
# TEST	Simulate client crashes (master continues) until log 2.
# TEST	Open 2nd master env handle and put something in log and flush.
# TEST	Downgrade master, restart client as master.
# TEST	Run rep_test on newmaster until log 2.
# TEST	New master writes log records, newclient processes records
# TEST	and 2nd newclient env handle calls log_flush.
# TEST	New master commits, newclient processes and should succeed.
# TEST	Make sure 2nd handle detects the old log handle and doesn't
# TEST	write to a stale handle (if it does, the processing of the
# TEST	commit will fail).
#
proc rep066 { method { niter 10 } { tnum "066" } args } {

	source ./include.tcl
	if { $is_windows9x_test == 1 } {
		puts "Skipping replication test on Win 9x platform."
		return
	}

	# Run for all access methods.
	if { $checking_valid_methods } {
		return "ALL"
	}

	set args [convert_args $method $args]
	set logsets [create_logsets 2]

	# Run the body of the test with and without recovery.
	foreach r $test_recopts {
		foreach l $logsets {
			set logindex [lsearch -exact $l "in-memory"]
			if { $r == "-recover" && $logindex != -1 } {
				puts "Rep$tnum: Skipping\
				    for in-memory logs with -recover."
				continue
			}
			puts "Rep$tnum ($method $r):\
			    Replication and dead log handles."
			puts "Rep$tnum: Master logs are [lindex $l 0]"
			puts "Rep$tnum: Client logs are [lindex $l 1]"
			rep066_sub $method $niter $tnum $l $r $args
		}
	}
}

proc rep066_sub { method niter tnum logset recargs largs } {
	global testdir

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
	set m_logargs " -log_buffer $log_buf "
	set c_logargs " -log_buffer $log_buf "

	set m_logtype [lindex $logset 0]
	set c_logtype [lindex $logset 1]

	# In-memory logs require a large log buffer, and cannot
	# be used with -txn nosync.
	set m_logargs [adjust_logargs $m_logtype]
	set c_logargs [adjust_logargs $c_logtype]
	set m_txnargs [adjust_txnargs $m_logtype]
	set c_txnargs [adjust_txnargs $c_logtype]

	# Open a master.
	# Later we'll open a 2nd handle to this env.
	repladd 1
	set ma_envcmd "berkdb_env_noerr -create $m_txnargs \
	    $m_logargs -errpfx ENV0 -log_max $log_max \
	    -home $masterdir -rep_transport \[list 1 replsend\]"
#	set ma_envcmd "berkdb_env_noerr -create $m_txnargs \
#	    $m_logargs -log_max $log_max \
#	    -errpfx ENV0 -verbose {rep on} -errfile /dev/stderr \
#	    -home $masterdir -rep_transport \[list 1 replsend\]"
	set env0 [eval $ma_envcmd $recargs -rep_master]
	set masterenv $env0
	error_check_good master_env [is_valid_env $env0] TRUE

	# Open a client.
	repladd 2
	set cl_envcmd "berkdb_env_noerr -create $c_txnargs \
	    $c_logargs -errpfx ENV1 -log_max $log_max \
	    -home $clientdir -rep_transport \[list 2 replsend\]"
#	set cl_envcmd "berkdb_env_noerr -create $c_txnargs \
#	    $c_logargs -log_max $log_max \
#	    -errpfx ENV1 -verbose {rep on} -errfile /dev/stderr \
#	    -home $clientdir -rep_transport \[list 2 replsend\]"
	set env1 [eval $cl_envcmd $recargs -rep_client]
	set clientenv $env1
	error_check_good client_env [is_valid_env $env1] TRUE


	# Bring the clients online by processing the startup messages.
	set envlist "{$env0 1} {$env1 2}"
	process_msgs $envlist

	# Run a modified test001 in the master (and update clients).
	puts "\tRep$tnum.a.0: Running rep_test in replicated env."
	eval rep_test $method $masterenv NULL $niter 0 0 0 0 $largs
	process_msgs $envlist

	set nstart $niter
	set last_client_log [get_logfile $env1 last]
	set stop 0
	while { $stop == 0 } {
		puts "\tRep$tnum.b: Run test on master until log file changes."
		eval rep_test\
		    $method $masterenv NULL $niter $nstart $nstart 0 0 $largs
		incr nstart $niter
		replclear 2
		set last_master_log [get_logfile $masterenv last]
		if { $last_master_log > $last_client_log } {
			set stop 1
		}
	}

	# Open a 2nd env handle on the master.
	# We want to have some operations happen on the normal
	# handle and then flush them with this handle.
	puts "\tRep$tnum.c: Open 2nd master env and flush log."
	set 2ndenv [eval $ma_envcmd -rep_master -errpfx 2NDENV]
	error_check_good master_env [is_valid_env $2ndenv] TRUE

	set testfile2 "test$tnum.2.db"
	set omethod [convert_method $method]
	set txn [$masterenv txn]
	error_check_good txn [is_valid_txn $txn $masterenv] TRUE
	set db [eval {berkdb_open_noerr -env $masterenv -txn $txn \
	    -create -mode 0644} $largs $omethod $testfile2]
	error_check_good dbopen [is_valid_db $db] TRUE

	# Flush on the 2nd handle
	set lf [stat_field $2ndenv log_stat "Times log flushed to disk"]
	error_check_good flush [$2ndenv log_flush] 0
	set lf2 [stat_field $2ndenv log_stat "Times log flushed to disk"]
	error_check_bad log_flush $lf $lf2

	# Resolve the txn and close the database
	error_check_good commit [$txn commit] 0
	error_check_good close [$db close] 0

	# Nuke those messages for client about to become master.
	replclear 2

	puts "\tRep$tnum.d: Swap envs"
	set masterenv $env1
	set clientenv $env0
	error_check_good downgrade [$clientenv rep_start -client] 0
	error_check_good upgrade [$masterenv rep_start -master] 0
	set envlist "{$env0 1} {$env1 2}"
	process_msgs $envlist

	#
	# At this point, env0 should have rolled back across the log file.
	# We need to do some operations on the master, process them on
	# the client (but not a commit because that flushes).  We want
	# the message processing client env (env0) to put records in
	# the log buffer and the 2nd env handle to flush the log.
	#
	puts "\tRep$tnum.e: Run test until create new log file."
	#
	# Set this to the last log file the old master had.
	#
	set last_client_log $last_master_log
	set last_master_log [get_logfile $masterenv last]
	set stop 0
	while { $stop == 0 } {
		puts "\tRep$tnum.b: Run test on master until log file changes."
		eval rep_test\
		    $method $masterenv NULL $niter $nstart $nstart 0 0 $largs
		process_msgs $envlist
		incr nstart $niter
		set last_master_log [get_logfile $masterenv last]
		if { $last_master_log == $last_client_log } {
			set stop 1
		}
	}
	puts "\tRep$tnum.f: Create some log records."
	set txn [$masterenv txn]
	error_check_good txn [is_valid_txn $txn $masterenv] TRUE
	set db [eval {berkdb_open_noerr -env $masterenv -txn $txn \
	    -create -mode 0644} $largs $omethod $testfile2]
	error_check_good dbopen [is_valid_db $db] TRUE

	process_msgs $envlist
	# Flush on the 2nd handle
	puts "\tRep$tnum.g: Flush on 2nd env handle."
	set lf [stat_field $2ndenv log_stat "Times log flushed to disk"]
	error_check_good flush [$2ndenv log_flush] 0
	set lf2 [stat_field $2ndenv log_stat "Times log flushed to disk"]
	error_check_bad log_flush2 $lf $lf2

	# Resolve the txn and close the database
	puts "\tRep$tnum.h: Process commit on client env handle."
	error_check_good commit [$txn commit] 0
	error_check_good close [$db close] 0
	process_msgs $envlist

	error_check_good cl2_close [$2ndenv close] 0
	error_check_good env0_close [$env0 close] 0
	error_check_good env1_close [$env1 close] 0
	replclose $testdir/MSGQUEUEDIR
	return
}

