# See the file LICENSE for redistribution information.
#
# Copyright (c) 2003-2005
#	Sleepycat Software.  All rights reserved.
#
# $Id: rep042.tcl,v 12.5 2005/10/04 20:34:34 sue Exp $
#
# TEST	rep042
# TEST	Concurrency with updates.
# TEST
# TEST 	Verify racing role changes and updates don't result in 
# TEST  pages with LSN 0,1.  Set up an environment that is master.
# TEST  Spawn child process that does a delete, but using the
# TEST  $env check so that it sleeps in the middle of the call.
# TEST  Master downgrades and then sleeps as a client so that
# TEST  child will run.  Verify child does not succeed (should
# TEST  get read-only error) due to role change in the middle of
# TEST  its call.
proc rep042 { method { niter 10 } { tnum "042" } args } {

	source ./include.tcl
	if { $is_windows9x_test == 1 } { 
		puts "Skipping replication test on Win 9x platform."
		return
	} 
	set args [convert_args $method $args]
	set logsets [create_logsets 2]

	# Run the body of the test with and without recovery.
	set recopts { "" "-recover" }
	foreach r $recopts {
		foreach l $logsets {
			set logindex [lsearch -exact $l "in-memory"]
			if { $r == "-recover" && $logindex != -1 } {
				puts "Rep$tnum: Skipping\
				    for in-memory logs with -recover."
				continue
			}

			puts "Rep$tnum ($method $r):\
			    Concurrency with updates."
			puts "Rep$tnum: Master logs are [lindex $l 0]"
			puts "Rep$tnum: Client logs are [lindex $l 1]"
			rep042_sub $method $niter $tnum $l $r $args
		}
	}
}

proc rep042_sub { method niter tnum logset recargs largs } {
	source ./include.tcl
	global perm_response_list

	env_cleanup $testdir
	set omethod [convert_method $method]

	replsetup $testdir/MSGQUEUEDIR

	set masterdir $testdir/MASTERDIR
	set clientdir $testdir/CLIENTDIR

	file mkdir $masterdir
	file mkdir $clientdir
	set m_logtype [lindex $logset 0]
	set c_logtype [lindex $logset 1]

	# In-memory logs require a large log buffer, and cannot
	# be used with -txn nosync.
	set m_logargs [adjust_logargs $m_logtype]
	set c_logargs [adjust_logargs $c_logtype]
	set m_txnargs [adjust_txnargs $m_logtype]
	set c_txnargs [adjust_txnargs $c_logtype]

	# Open a master.
	repladd 1
	set ma_cmd "berkdb_env_noerr -create -lock_max 2500 \
	    -log_max 1000000 $m_txnargs $m_logargs \
	    -home $masterdir -rep_master \
	    -rep_transport \[list 1 replsend\]"
#	set ma_cmd "berkdb_env_noerr -create -lock_max 2500 \
#	    -log_max 1000000 $m_txnargs $m_logargs \
#	    -verbose {rep on} -errfile /dev/stderr \
#	    -home $masterdir -rep_master -rep_transport \
#	    \[list 1 replsend\]"
	set masterenv [eval $ma_cmd $recargs]
	error_check_good master_env [is_valid_env $masterenv] TRUE

	# Open a client
	repladd 2
	set cl_cmd "berkdb_env_noerr -create -home $clientdir \
	    $c_txnargs $c_logargs -rep_client \
	    -rep_transport \[list 2 replsend\]"
#	set cl_cmd "berkdb_env_noerr -create -home $clientdir \
#	    -verbose {rep on} -errfile /dev/stderr \
#	    $c_txnargs $c_logargs -rep_client \
#	    -rep_transport \[list 2 replsend\]"
	set clientenv [eval $cl_cmd $recargs]
	error_check_good client_env [is_valid_env $clientenv] TRUE

	# Bring the client online.
	process_msgs "{$masterenv 1} {$clientenv 2}"

	puts "\tRep$tnum.a: Create and populate database."
	set dbname rep042.db
	set db [eval "berkdb_open -create $omethod -auto_commit \
	    -env $masterenv $largs $dbname"]
	for { set i 1 } { $i < $niter } { incr i } {
		set t [$masterenv txn]
		error_check_good db_put \
		    [eval $db put -txn $t $i [chop_data $method data$i]] 0
		error_check_good txn_commit [$t commit] 0
	}
	process_msgs "{$masterenv 1} {$clientenv 2}"

	set ops {del truncate}
	foreach op $ops {
		# Fork child process on client.  The child will do a delete.
		set sleepval 4
		set scrlog $testdir/repscript.log
		puts "\tRep$tnum.b: Fork child process on client ($op)."
		set pid [exec $tclsh_path $test_path/wrap.tcl \
		    rep042script.tcl $scrlog \
		    $masterdir $sleepval $dbname $op &]
	
		# Wait for child process to start up.
		while { 1 } {
			if { [file exists $masterdir/marker.db] == 0  } {
				tclsleep 1
			} else {
				tclsleep 1
				break
			}
		}
	
 		puts "\tRep$tnum.c: Downgrade during child $op."
		error_check_good downgrade [$masterenv rep_start -client] 0
	
		puts "\tRep$tnum.d: Waiting for child ..."
		# Watch until the child is done.
		watch_procs $pid 5
		puts "\tRep$tnum.e: Upgrade to master again ..."
		error_check_good upgrade [$masterenv rep_start -master] 0
		set end [expr $niter * 2]
		for { set i $niter } { $i <= $end } { incr i } {
			set t [$masterenv txn]
			error_check_good db_put \
			    [eval $db put -txn $t $i [chop_data $method data$i]] 0
			error_check_good txn_commit [$t commit] 0
		}
		process_msgs "{$masterenv 1} {$clientenv 2}" 

		error_check_good check [check_script $scrlog "read-only"] 1
		fileremove -f $masterdir/marker.db
	}

	# Clean up.
	error_check_good db_close [$db close] 0
	error_check_good masterenv_close [$masterenv close] 0
	error_check_good clientenv_close [$clientenv close] 0

	replclose $testdir/MSGQUEUEDIR
}

proc check_script { log str } {
	set ret 0
	set res [catch {open $log} id]
	if { $res != 0 } {
		puts "FAIL: open of $log failed: $id"
		# Return 0
		return $ret
	}
	while { [gets $id val] != -1 } {
#		puts "line: $val"
		if { [is_substr $val $str] } {
			set ret 1
			break
		}
	}
	close $id
	return $ret
}
