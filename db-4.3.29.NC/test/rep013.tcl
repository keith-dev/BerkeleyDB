# See the file LICENSE for redistribution information.
#
# Copyright (c) 2001-2004
#	Sleepycat Software.  All rights reserved.
#
# $Id: rep013.tcl,v 11.11 2004/09/22 18:01:06 bostic Exp $
#
# TEST	rep013
# TEST	Replication and swapping master/clients with open dbs.
# TEST
# TEST	Run a modified version of test001 in a replicated master env.
# TEST	Make additional changes to master, but not to the client.
# TEST	Swap master and client.
# TEST	Verify that the roll back on clients gives dead db handles.
# TEST	Swap and verify several times.
proc rep013 { method { niter 10 } { tnum "013" } args } {
	set args [convert_args $method $args]
	set logsets [create_logsets 3]

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
			puts "Rep$tnum ($r):\
			    Replication and ($method) master/client swapping."
			puts "Rep$tnum: Master logs are [lindex $l 0]"
			puts "Rep$tnum: Client 0 logs are [lindex $l 1]"
			puts "Rep$tnum: Client 1 logs are [lindex $l 2]"
			rep013_sub $method $niter $tnum $l $r $args
		}
	}
}

proc rep013_sub { method niter tnum logset recargs largs } {
	global testdir
	env_cleanup $testdir
	set orig_tdir $testdir

	replsetup $testdir/MSGQUEUEDIR

	set masterdir $testdir/MASTERDIR
	set clientdir $testdir/CLIENTDIR
	set clientdir2 $testdir/CLIENTDIR.2
	file mkdir $masterdir
	file mkdir $clientdir
	file mkdir $clientdir2

	set m_logtype [lindex $logset 0]
	set c_logtype [lindex $logset 1]
	set c2_logtype [lindex $logset 2]

	# In-memory logs require a large log buffer, and cannot
	# be used with -txn nosync.
	set m_logargs [adjust_logargs $m_logtype]
	set c_logargs [adjust_logargs $c_logtype]
	set c2_logargs [adjust_logargs $c2_logtype]
	set m_txnargs [adjust_txnargs $m_logtype]
	set c_txnargs [adjust_txnargs $c_logtype]
	set c2_txnargs [adjust_txnargs $c2_logtype]

	# Set number of swaps between master and client.
	set nswap 6

	# Open a master.
	repladd 1
	set ma_envcmd "berkdb_env_noerr -create $m_txnargs \
	    $m_logargs -lock_max 2500 -errpfx ENV1 \
	    -cachesize {0 4194304 3} \
	    -home $masterdir -rep_transport \[list 1 replsend\]"
#	set ma_envcmd "berkdb_env_noerr -create $m_txnargs \
#	    $m_logargs -lock_max 2500 \
#	    -cachesize {0 4194304 3} \
#	    -errpfx ENV1 -verbose {recovery on} -errfile /dev/stderr \
#	    -home $masterdir -rep_transport \[list 1 replsend\]"
	set env1 [eval $ma_envcmd $recargs -rep_master]
	error_check_good master_env [is_valid_env $env1] TRUE

	# Open two clients
	repladd 2
	set cl_envcmd "berkdb_env_noerr -create $c_txnargs \
	    $c_logargs -lock_max 2500 -errpfx ENV2 \
	    -cachesize {0 2097152 2} \
	    -home $clientdir -rep_transport \[list 2 replsend\]"
#	set cl_envcmd "berkdb_env_noerr -create $c_txnargs \
#	    $c_logargs -lock_max 2500 \
#	    -cachesize {0 2097152 2} \
#	    -errpfx ENV2 -verbose {recovery on} -errfile /dev/stderr \
#	    -home $clientdir -rep_transport \[list 2 replsend\]"
	set env2 [eval $cl_envcmd $recargs -rep_client]
	error_check_good client_env [is_valid_env $env2] TRUE

	repladd 3
	set cl2_envcmd "berkdb_env_noerr -create $c2_txnargs \
	    $c2_logargs -lock_max 2500 -errpfx ENV3 \
	    -cachesize {0 1048576 1} \
	    -home $clientdir2 -rep_transport \[list 3 replsend\]"
#	set cl2_envcmd "berkdb_env_noerr -create $c2_txnargs \
#	    $c2_logargs -lock_max 2500 \
#	    -cachesize {0 1048576 1} \
#	    -errpfx ENV3 -verbose {recovery on} -errfile /dev/stderr \
#	    -home $clientdir2 -rep_transport \[list 3 replsend\]"
	set cl2env [eval $cl2_envcmd $recargs -rep_client]
	error_check_good client2_env [is_valid_env $cl2env] TRUE

	set testfile "test$tnum.db"

	set omethod [convert_method $method]

	set env1db_cmd "berkdb_open_noerr -env $env1 -auto_commit \
	     -create -mode 0644 $largs $omethod $testfile"
	set env1db [eval $env1db_cmd]
	error_check_good dbopen [is_valid_db $env1db] TRUE

	#
	# Verify that a client creating a database gets an error.
	#
	set stat [catch {berkdb_open_noerr -env $env2 -auto_commit \
	     -create -mode 0644 $largs $omethod $testfile} ret]
	error_check_good create_cl $stat 1
	error_check_good cr_str [is_substr $ret "invalid"] 1

	# Bring the clients online by processing the startup messages.
	set envlist "{$env1 1} {$env2 2} {$cl2env 3}"
	process_msgs $envlist

	set env2db_cmd "berkdb_open_noerr -env $env2 -auto_commit \
	     -mode 0644 $largs $omethod $testfile"
	set env2db [eval $env2db_cmd]
	error_check_good dbopen [is_valid_db $env2db] TRUE
	set env3db_cmd "berkdb_open_noerr -env $cl2env -auto_commit \
	     -mode 0644 $largs $omethod $testfile"
	set env3db [eval $env3db_cmd]
	error_check_good dbopen [is_valid_db $env3db] TRUE

	#
	# Set up all the master/client data we're going to need
	# to keep track of and swap.
	#
	set masterenv $env1
	set masterdb $env1db
	set mid 1
	set clientenv $env2
	set clientdb $env2db
	set cid 2
	set mdb_cmd "berkdb_open_noerr -env $masterenv -auto_commit \
	     -mode 0644 $largs $omethod $testfile"
	set cdb_cmd "berkdb_open_noerr -env $clientenv -auto_commit \
	     -mode 0644 $largs $omethod $testfile"

	# Run a modified test001 in the master (and update clients).
	puts "\tRep$tnum.a: Running test001 in replicated env."
	eval rep_test $method $masterenv $masterdb $niter 0 0
	set envlist "{$env1 1} {$env2 2} {$cl2env 3}"
	process_msgs $envlist

	set nstart 0
	for { set i 0 } { $i < $nswap } { incr i } {
		puts "\tRep$tnum.b.$i: Check for bad db handles"
		set dbl {masterdb clientdb env3db}
		set dbcmd {$mdb_cmd $cdb_cmd $env3db_cmd}

		set stat [catch {$masterdb stat} ret]
		if { $stat == 1 } {
			error_check_good dead [is_substr $ret \
			    DB_REP_HANDLE_DEAD] 1
			error_check_good close [$masterdb close] 0
			set masterdb [eval $mdb_cmd]
			error_check_good dbopen [is_valid_db $masterdb] TRUE
		}

		set stat [catch {$clientdb stat} ret]
		if { $stat == 1 } {
			error_check_good dead [is_substr $ret \
			    DB_REP_HANDLE_DEAD] 1
			error_check_good close [$clientdb close] 0
			set clientdb [eval $cdb_cmd]
			error_check_good dbopen [is_valid_db $clientdb] TRUE
		}

		set stat [catch {$env3db stat} ret]
		if { $stat == 1 } {
			error_check_good dead [is_substr $ret \
			    DB_REP_HANDLE_DEAD] 1
			error_check_good close [$env3db close] 0
			set env3db [eval $env3db_cmd]
			error_check_good dbopen [is_valid_db $env3db] TRUE
		}

		set nstart [expr $nstart + $niter]
		puts "\tRep$tnum.c.$i: Run test in master and client2 only"
		eval rep_test \
		    $method $masterenv $masterdb $niter $nstart $nstart
		set envlist "{$masterenv $mid} {$cl2env 3}"
		process_msgs $envlist

		# Nuke those for client about to become master.
		replclear $cid

		# Swap all the info we need.
		set tmp $masterenv
		set masterenv $clientenv
		set clientenv $tmp

		set tmp $masterdb
		set masterdb $clientdb
		set clientdb $tmp

		set tmp $mid
		set mid $cid
		set cid $tmp

		set tmp $mdb_cmd
		set mdb_cmd $cdb_cmd
		set cdb_cmd $tmp

		puts "\tRep$tnum.d.$i: Swap: master $mid, client $cid"
		error_check_good downgrade [$clientenv rep_start -client] 0
		error_check_good upgrade [$masterenv rep_start -master] 0
		set envlist "{$env1 1} {$env2 2} {$cl2env 3}"
		process_msgs $envlist
	}
	puts "\tRep$tnum.e: Closing"
	error_check_good masterdb [$masterdb close] 0
	error_check_good clientdb [$clientdb close] 0
	error_check_good cl2db [$env3db close] 0
	error_check_good env1_close [$env1 close] 0
	error_check_good env2_close [$env2 close] 0
	error_check_good cl2_close [$cl2env close] 0
	replclose $testdir/MSGQUEUEDIR
	set testdir $orig_tdir
	return
}
