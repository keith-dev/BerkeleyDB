# See the file LICENSE for redistribution information.
#
# Copyright (c) 2001-2006
#	Oracle Corporation.  All rights reserved.
#
# $Id: rep001.tcl,v 12.11 2006/08/24 14:46:37 bostic Exp $
#
# TEST  rep001
# TEST	Replication rename and forced-upgrade test.
# TEST
# TEST	Run rep_test in a replicated master environment.
# TEST	Verify that the database on the client is correct.
# TEST	Next, remove the database, close the master, upgrade the
# TEST	client, reopen the master, and make sure the new master can
# TEST	correctly run rep_test and propagate it in the other direction.

proc rep001 { method { niter 1000 } { tnum "001" } args } {
	global passwd
	global has_crypto

	source ./include.tcl
	if { $is_windows9x_test == 1 } {
		puts "Skipping replication test on Win9x platform."
		return
	}

	if { $checking_valid_methods } {
		return "ALL"
	}

	# Rep056 runs rep001 with in-memory named databases.
	set inmem 0
	set msg "using regular named databases"
	if { $tnum == "056" } {
		set inmem 1
		set msg "using in-memory named databases"
		if { [is_queueext $method] == 1 } {
			puts "Skipping rep$tnum for method $method"
			return
		}
	}

	# Run tests with and without recovery.  If we're doing testing
	# of in-memory logging, skip the combination of recovery
	# and in-memory logging -- it doesn't make sense.
	set logsets [create_logsets 2]
	set saved_args $args

	foreach recopt $test_recopts {
		foreach l $logsets {
			set logindex [lsearch -exact $l "in-memory"]
			if { $recopt == "-recover" && $logindex != -1 } {
				puts "Skipping test with -recover for in-memory logs."
				continue
			}
			set envargs ""
			set args $saved_args
			puts -nonewline "Rep$tnum: Replication sanity test "
			puts "($method $recopt), $msg."
			puts "Rep$tnum: Master logs are [lindex $l 0]"
			puts "Rep$tnum: Client logs are [lindex $l 1]"
			rep001_sub $method \
			    $niter $tnum $envargs $l $recopt $inmem $args

			# Skip encrypted tests if not supported.
			if { $has_crypto == 0 || $inmem } {
				continue
			}

			# Run the same tests with security.  In-memory
			# databases don't work with encryption.
			append envargs " -encryptaes $passwd "
			append args " -encrypt "
			puts "Rep$tnum: Replication and security sanity test\
			    ($method $recopt)."
			puts "Rep$tnum: Master logs are [lindex $l 0]"
			puts "Rep$tnum: Client logs are [lindex $l 1]"
			rep001_sub $method \
			    $niter $tnum $envargs $l $recopt $inmem $args
		}
	}
}

proc rep001_sub { method niter tnum envargs logset recargs inmem largs } {
	source ./include.tcl
	global testdir
	global encrypt

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
	set env_cmd(M) "berkdb_env_noerr -create \
	    -log_max 1000000 $envargs $m_logargs $recargs \
	    -home $masterdir -errpfx MASTER $m_txnargs -rep_master \
	    -rep_transport \[list 1 replsend\]"
#	set env_cmd(M) "berkdb_env_noerr -create \
#	    -log_max 1000000 $envargs $m_logargs $recargs \
#	    -home $masterdir \
#	    -verbose {rep on} -errfile /dev/stderr \
#	    -errpfx MASTER $m_txnargs -rep_master \
#	    -rep_transport \[list 1 replsend\]"
	set masterenv [eval $env_cmd(M)]
	error_check_good master_env [is_valid_env $masterenv] TRUE

	# Open a client
	repladd 2
	set env_cmd(C) "berkdb_env_noerr -create \
	    -log_max 1000000 $envargs $c_logargs $recargs \
	    -home $clientdir -errpfx CLIENT $c_txnargs -rep_client \
	    -rep_transport \[list 2 replsend\]"
#	set env_cmd(C) "berkdb_env_noerr -create \
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

	# Run rep_test in the master (and update client).
	puts "\tRep$tnum.a:\
	    Running rep_test in replicated env ($envargs $recargs)."
	eval rep_test $method $masterenv NULL $niter 0 0 0 $inmem $largs
	process_msgs $envlist

	puts "\tRep$tnum.b: Verifying client database contents."
	if { $inmem } {
		set dbname { "" "test.db" }
	} else {
		set dbname "test.db"
	}

	rep_verify $masterdir $masterenv $clientdir $clientenv 0 1 1 $dbname

	# Remove the file (and update client).
	puts "\tRep$tnum.c: Remove the file on the master and close master."
	error_check_good remove \
	    [eval {$masterenv dbremove} -auto_commit $dbname] 0
	error_check_good masterenv_close [$masterenv close] 0
	process_msgs $envlist

	puts "\tRep$tnum.d: Upgrade client."
	set newmasterenv $clientenv
	error_check_good upgrade_client [$newmasterenv rep_start -master] 0

	# Run rep_test in the new master
	puts "\tRep$tnum.e: Running rep_test in new master."
	eval rep_test $method $newmasterenv NULL $niter 0 0 0 $inmem $largs
	set envlist "{$newmasterenv 2}"
	process_msgs $envlist

	puts "\tRep$tnum.f: Reopen old master as client and catch up."
	# Throttle master so it can't send everything at once
	$newmasterenv rep_limit 0 [expr 64 * 1024]
	set newclientenv [eval {berkdb_env -create -recover} \
	    $envargs -txn nosync \
	    {-home $masterdir -rep_client -rep_transport [list 1 replsend]}]
	error_check_good newclient_env [is_valid_env $newclientenv] TRUE
	set envlist "{$newclientenv 1} {$newmasterenv 2}"
	process_msgs $envlist

	# If we're running with a low number of iterations, we might
	# not have had to throttle the data transmission; skip the check.
	if { $niter > 200 } {
		set nthrottles \
		    [stat_field $newmasterenv rep_stat "Transmission limited"]
		error_check_bad nthrottles $nthrottles -1
		error_check_bad nthrottles $nthrottles 0
	}

	# Run a modified rep_test in the new master (and update client).
	puts "\tRep$tnum.g: Running rep_test in new master."
	eval rep_test $method \
	    $newmasterenv NULL $niter $niter $niter 0 $inmem $largs
	process_msgs $envlist

	# Verify the database in the client dir.
	puts "\tRep$tnum.h: Verifying new client database contents."

	rep_verify $masterdir $newmasterenv $clientdir $newclientenv 0 1 1 $dbname

	error_check_good newmasterenv_close [$newmasterenv close] 0
	error_check_good newclientenv_close [$newclientenv close] 0

	if { [lsearch $envargs "-encrypta*"] !=-1 } {
		set encrypt 1
	}
	error_check_good verify \
	    [verify_dir $clientdir "\tRep$tnum.k: " 0 0 1] 0
	replclose $testdir/MSGQUEUEDIR
}
