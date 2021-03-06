# See the file LICENSE for redistribution information.
#
# Copyright (c) 2001-2005
#	Sleepycat Software.  All rights reserved.
#
# $Id: rep051.tcl,v 12.3 2005/10/18 19:05:54 carol Exp $
#
# TEST	rep051
# TEST	Test of compaction with replication.
# TEST
# TEST	Run rep_test in a replicated master environment.
# TEST	Delete a large number of entries and compact with -freespace.
# TEST	Propagate the changes to the client and make sure client and 
# TEST	master match.  

proc rep051 { method { niter 5000 } { tnum "051" } args } {
	source ./include.tcl
	if { $is_windows9x_test == 1 } {
		puts "Skipping replication test on Win9x platform."
		return
	}

	# Compaction is an option for btree and recno databases only.
	if { [is_hash $method] == 1 || [is_queue $method] == 1 } {
		puts "Skipping test$tnum for method $method."
		return
	}
							
	# Run tests with and without recovery.  If we're doing testing
	# of in-memory logging, skip the combination of recovery
	# and in-memory logging -- it doesn't make sense.
	set logsets [create_logsets 2]
	set saved_args $args

	foreach recopt { "" "-recover" } {
		foreach l $logsets {
			set logindex [lsearch -exact $l "in-memory"]
			if { $recopt == "-recover" && $logindex != -1 } {
				puts "Skipping test with -recover for in-memory logs."
				continue
			}
			set envargs ""
			set args $saved_args
			puts "Rep$tnum: Replication with compaction ($method $recopt)."
			puts "Rep$tnum: Master logs are [lindex $l 0]"
			puts "Rep$tnum: Client logs are [lindex $l 1]"
			rep051_sub $method \
			    $niter $tnum $envargs $l $recopt $args
		}
	}
}

proc rep051_sub { method niter tnum envargs logset recargs largs } {
	source ./include.tcl
	global testdir

	env_cleanup $testdir

	replsetup $testdir/MSGQUEUEDIR

	set masterdir $testdir/MASTERDIR
	set clientdir $testdir/CLIENTDIR

	file mkdir $masterdir
	file mkdir $clientdir

	set m_logtype [lindex $logset 0]
	set c_logtype [lindex $logset 1]

	# In-memory logs require a large log buffer, and cannot
	# be used with -txn nosync.  Adjust the args for master
	# and client.
	set m_logargs [adjust_logargs $m_logtype]
	set c_logargs [adjust_logargs $c_logtype]
	set m_txnargs [adjust_txnargs $m_logtype]
	set c_txnargs [adjust_txnargs $c_logtype]

	# Open a master.
	repladd 1
	set env_cmd(M) "berkdb_env_noerr -create -lock_max 2500 \
	    -log_max 1000000 $envargs $m_logargs $recargs \
	    -home $masterdir -errpfx MASTER $m_txnargs -rep_master \
	    -rep_transport \[list 1 replsend\]"
#	set env_cmd(M) "berkdb_env_noerr -create -lock_max 2500 \
#	    -log_max 1000000 $envargs $m_logargs $recargs \
#	    -home $masterdir \
#	    -verbose {rep on} -errfile /dev/stderr \
#	    -errpfx MASTER $m_txnargs -rep_master \
#	    -rep_transport \[list 1 replsend\]"
	set masterenv [eval $env_cmd(M)]
	error_check_good master_env [is_valid_env $masterenv] TRUE

	# Open a client
	repladd 2
	set env_cmd(C) "berkdb_env_noerr -create -lock_max 2500 \
	    -log_max 1000000 $envargs $c_logargs $recargs \
	    -home $clientdir -errpfx CLIENT $c_txnargs -rep_client \
	    -rep_transport \[list 2 replsend\]"
#	set env_cmd(C) "berkdb_env_noerr -create -lock_max 2500 \
#	    -log_max 1000000 $envargs $c_logargs $recargs \
#	    -home $clientdir \
#	    -verbose {rep on} -errfile /dev/stderr \
#	    -errpfx CLIENT $c_txnargs -rep_client \
#	    -rep_transport \[list 2 replsend\]"
	set clientenv [eval $env_cmd(C)]
	error_check_good client_env [is_valid_env $clientenv] TRUE

	# Bring the client online by processing the startup messages.
	set envlist "{$masterenv 1} {$clientenv 2}"
	process_msgs $envlist

	# Explicitly create the db handle so we can do deletes, 
	# and also to make the page size small.
	set testfile "test.db"
	set omethod [convert_method $method]
	set db [eval {berkdb_open_noerr -env $masterenv -auto_commit\
	    -pagesize 512 -create -mode 0644} $largs $omethod $testfile]
 	error_check_good db_open [is_valid_db $db] TRUE

	# Run rep_test in the master and update client.
	puts "\tRep$tnum.a:\
	    Running rep_test in replicated env ($envargs $recargs)."
	eval rep_test $method $masterenv $db $niter 0 0 0 0 $largs
	process_msgs $envlist

	# Verify that contents match.
	puts "\tRep$tnum.b: Verifying client database contents."
	rep_verify $masterdir $masterenv $clientdir $clientenv

	# Delete most entries.  Since some of our methods renumber,
	# delete starting at $niter and working down to 0.
	puts "\tRep$tnum.c: Remove most entries, by cursor."
	set count [expr $niter - 1]
	set n 20
	set t [$masterenv txn]
	error_check_good txn [is_valid_txn $t $masterenv] TRUE
	set txn "-txn $t"

	set dbc [eval {$db cursor} $txn]

	# Leave every nth item.
	set dbt [$dbc get -first]
	while { $count > 0 } {
		if { [expr $count % $n] != 0 } {
			error_check_good dbc_del [$dbc del] 0
		}
		set dbt [$dbc get -next] 
		incr count -1
	}

	error_check_good dbc_close [$dbc close] 0
	error_check_good t_commit [$t commit] 0

	# Compact database.
	puts "\tRep$tnum.d: Compact database."
	set free1 [stat_field $db stat "Pages on freelist"]
	set t [$masterenv txn]
	error_check_good txn [is_valid_txn $t $masterenv] TRUE
	set txn "-txn $t"
 
	set ret [eval {$db compact} $txn {-freespace}] 

	error_check_good t_commit [$t commit] 0
	error_check_good db_sync [$db sync] 0
	set free2 [stat_field $db stat "Pages on freelist"]
	error_check_good more_free_pages [expr $free2 > $free1] 1

	# Process messages.
	process_msgs $envlist

	# Reverify.
	puts "\tRep$tnum.b: Verifying client database contents."
	rep_verify $masterdir $masterenv $clientdir $clientenv
	
	# Clean up.
	error_check_good db_close [$db close] 0
	error_check_good masterenv_close [$masterenv close] 0
	error_check_good clientenv_close [$clientenv close] 0
	replclose $testdir/MSGQUEUEDIR
}
