# See the file LICENSE for redistribution information.
#
# Copyright (c) 2004-2005
#	Sleepycat Software.  All rights reserved.
#
# $Id: rep028.tcl,v 12.4 2005/10/18 19:04:17 carol Exp $
#
# TEST  	rep028
# TEST	Replication and non-rep env handles. (Also see rep006.)
# TEST
# TEST	Open second non-rep env on client, and create a db
# TEST	through this handle.  Open the db on master and put
# TEST	some data.  Check whether the non-rep handle keeps
# TEST	working.  Also check if opening the client database
# TEST	in the non-rep env writes log records.
#
proc rep028 { method { niter 100 } { tnum "028" } args } {

	source ./include.tcl
	if { $is_windows9x_test == 1 } { 
		puts "Skipping replication test on Win 9x platform."
		return
	} 
	# Skip test for HP-UX because we can't open an env twice.
	if { $is_hp_test == 1 } {
		puts "\tRep$tnum: Skipping for HP-UX."
		return
	}
	if { [is_btree $method] == 0 } {
		puts "\tRep$tnum: Skipping for method $method."
		return
	}

	set args [convert_args $method $args]
	set logsets [create_logsets 2]

	# Run the body of the test with and without recovery.
	set recopts { "" "-recover" }
	set clopts { "create" "open" }
	foreach r $recopts {
		foreach l $logsets {
			set logindex [lsearch -exact $l "in-memory"]
			if { $r == "-recover" && $logindex != -1 } {
				puts "Rep$tnum: Skipping\
				    for in-memory logs with -recover."
				continue
			}
			foreach c $clopts {
				puts "Rep$tnum ($method $r $c):\
				    Replication and non-rep env handles"
				puts "Rep$tnum: Master logs are [lindex $l 0]"
				puts "Rep$tnum: Client logs are [lindex $l 1]"
				rep028_sub $method $niter $tnum $l $r $c $args
			}
		}
	}
}

proc rep028_sub { method niter tnum logset recargs clargs largs } {
	source ./include.tcl
	global testdir
	global is_hp_test

	set omethod [convert_method $method]
	env_cleanup $testdir

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
	puts "\tRep$tnum.a: Open replicated envs and non-replicated client env."
	repladd 1
	set env_cmd(M) "berkdb_env_noerr -create -lock_max 2500 \
	    -log_max 1000000 -home $masterdir \
	    $m_txnargs $m_logargs -rep_master \
	    -rep_transport \[list 1 replsend\]"
#	set env_cmd(M) "berkdb_env_noerr -create -lock_max 2500 \
#	    -log_max 1000000 -home $masterdir \
#	    $m_txnargs $m_logargs -rep_master \
#	     -verbose {rep on} -errpfx MASTER \
#	    -rep_transport \[list 1 replsend\]"
	set masterenv [eval $env_cmd(M) $recargs]
	error_check_good master_env [is_valid_env $masterenv] TRUE

	# Open a client
	repladd 2
	set env_cmd(C) "berkdb_env_noerr -create $c_txnargs \
	    $c_logargs -lock_max 2500 -home $clientdir \
	    -rep_transport \[list 2 replsend\]"
#	set env_cmd(C) "berkdb_env_noerr -create $c_txnargs \
#	    $c_logargs -lock_max 2500 -home $clientdir \
#	     -verbose {rep on} -errpfx CLIENT \
#	    -rep_transport \[list 2 replsend\]"
	set clientenv [eval $env_cmd(C) $recargs]
	error_check_good client_env [is_valid_env $clientenv] TRUE

	# Open 2nd non-replication handle on client env, and create
	# a db.  Note, by not specifying any subsystem args, we
	# do a DB_JOINENV, which is what we want.
	set nonrepenv [eval {berkdb_env_noerr -home $clientdir}]
	error_check_good nonrepenv [is_valid_env $nonrepenv] TRUE

	set dbname "test$tnum.db"
	#
	# If we're testing create, verify that if a non-rep client
	# creates a database before the master does, then when that
	# client goes to use it, it gets DB_DEAD_HANDLE.
	#

	if { $clargs == "create" } {
		puts "\tRep$tnum.b: Create database non-replicated."
		set let c
		set nextlet d
		set nonrepdb [eval berkdb_open_noerr -auto_commit \
		    -create $omethod -env $nonrepenv $dbname]
		error_check_good nonrepdb_open [is_valid_db $nonrepdb] TRUE
		tclsleep 2
	} else {
		set let b
		set nextlet c
	}

	#
	# Now declare the clientenv a client.
	#
	puts "\tRep$tnum.$let: Declare env as rep client"
	error_check_good client [$clientenv rep_start -client] 0

	# Bring the client online by processing the startup messages.
	set envlist "{$masterenv 1} {$clientenv 2}"
	process_msgs $envlist 0 NONE err
	#
	# In the create case, we'll detect the non-rep log records and
	# determine this client was never part of the replication group.
	#
	if { $clargs == "create" } {
		error_check_good dead [is_substr $err \
		    "was never part"] 1
		error_check_good close [$nonrepdb close] 0
	}

	# Open the same db through the master handle.  Put data
	# and process messages.
	set db [eval berkdb_open \
	    -create $omethod -env $masterenv -auto_commit $dbname]
	error_check_good db_open [is_valid_db $db] TRUE
	eval rep_test $method $masterenv $db $niter 0 0 0 0 $largs
	process_msgs $envlist

	#
	# If we're the open case, we want to just read the existing
	# database through a non-rep readonly handle.  Doing so
	# should not create log records on the client (but has
	# in the past).
	#
	if { $clargs == "open" } {
		puts "\tRep$tnum.$nextlet: Open and read database"
		set nonrepdb [eval berkdb_open \
		    -rdonly -env $nonrepenv $dbname]
		error_check_good nonrepdb_open [is_valid_db $nonrepdb] TRUE
		#
		# If opening wrote log records, we need to process
		# some more on the client to notice the end of log
		# is now in an unexpected place.
		#
		eval rep_test $method $masterenv $db $niter 0 0 0 0 $largs
		process_msgs $envlist
		error_check_good close [$nonrepdb close] 0

		#
		# Verify master and client logs are identical
		#
		set stat [catch {eval exec $util_path/db_printlog \
		    -h $masterdir > $masterdir/prlog} result]
		error_check_good stat_mprlog $stat 0
		set stat [catch {eval exec $util_path/db_printlog \
		    -h $clientdir > $clientdir/prlog} result]
		error_check_good stat_cprlog $stat 0
		error_check_good log_cmp \
		    [filecmp $masterdir/prlog $clientdir/prlog] 0
	}
	# Clean up.
	error_check_good db_close [$db close] 0

	error_check_good nonrepenv_close [$nonrepenv close] 0
	error_check_good masterenv_close [$masterenv close] 0
	error_check_good clientenv_close [$clientenv close] 0

	replclose $testdir/MSGQUEUEDIR
}
