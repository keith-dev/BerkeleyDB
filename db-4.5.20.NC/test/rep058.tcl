# See the file LICENSE for redistribution information.
#
# Copyright (c) 2005-2006
#	Oracle Corporation.  All rights reserved.
#
# $Id: rep058.tcl,v 12.8 2006/08/24 14:46:38 bostic Exp $
#
# TEST	rep058
# TEST
# TEST	Replication with early databases
# TEST
# TEST	Mimic an application where they create a database before
# TEST	calling rep_start, thus writing log records on a client
# TEST	before it is a client.  Verify we cannot join repl group.

proc rep058 { method { tnum "058" } args } {

	source ./include.tcl
	if { $is_windows9x_test == 1 } {
		puts "Skipping replication test on Win 9x platform."
		return
	}

	# There should be no difference with methods.  Just use btree.
	if { $checking_valid_methods } {
		set test_methods { btree }
		return $test_methods
	}
	if { [is_btree $method] == 0 } {
		puts "Rep058: Skipping for method $method."
		return
	}

	set args [convert_args $method $args]

	set logsets [create_logsets 2]
	foreach r $test_recopts {
		foreach l $logsets {
			set logindex [lsearch -exact $l "in-memory"]
			if { $r == "-recover" && $logindex != -1 } {
				puts "Skipping test with -recover for\
				    in-memory logs."
				continue
			}

			puts "Rep$tnum ($method $r): Replication with \
			    pre-created databases."
			puts "Rep$tnum: Master logs are [lindex $l 0]"
			puts "Rep$tnum: Client logs are [lindex $l 1]"
			rep058_sub $method $tnum $l $r $args
		}
	}
}

proc rep058_sub { method tnum logset recargs largs } {
	global testdir
	source ./include.tcl
	set orig_tdir $testdir

	set masterdir $testdir/MASTERDIR
	set clientdir $testdir/CLIENTDIR

	env_cleanup $testdir
	replsetup $testdir/MSGQUEUEDIR
	file mkdir $masterdir
	file mkdir $clientdir

	set m_logtype [lindex $logset 0]
	set c_logtype [lindex $logset 1]

	# In-memory logs require a large log buffer, and cannot
	# be used with -txn nosync.  Adjust the args for master
	# and client.
	set m_logargs [adjust_logargs $m_logtype]
	set m_txnargs [adjust_txnargs $m_logtype]
	set c_logargs [adjust_logargs $c_logtype]
	set c_txnargs [adjust_txnargs $c_logtype]

	set omethod [convert_method $method]

	# Open a master.
	repladd 1
	set envcmd(M) "berkdb_env_noerr -create $m_txnargs \
	    $m_logargs -lock_detect default \
	    -home $masterdir -rep_transport \[list 1 replsend\]"
#	set envcmd(M) "berkdb_env_noerr -create $m_txnargs \
#	    $m_logargs -lock_detect default \
#	    -errpfx ENV.M -verbose {rep on} -errfile /dev/stderr \
#	    -home $masterdir -rep_transport \[list 1 replsend\]"
	set menv [eval $envcmd(M) $recargs]
	error_check_good master_env0 [is_valid_env $menv] TRUE

	# Open a client
	repladd 2
	set envcmd(C) "berkdb_env_noerr -create $c_txnargs \
	    $c_logargs -lock_detect default \
	    -home $clientdir -rep_transport \[list 2 replsend\]"
#	set envcmd(C) "berkdb_env_noerr -create $c_txnargs \
# 	    $c_logargs -lock_detect default \
#	    -errpfx ENV.C -verbose {rep on} -errfile /dev/stderr \
#	    -home $clientdir -rep_transport \[list 2 replsend\]"
	set cenv [eval $envcmd(C) $recargs]
	error_check_good client_env [is_valid_env $cenv] TRUE

	puts "\tRep$tnum.a: Create same database in both envs."
	set dbname "test.db"
	set mdb [eval {berkdb_open_noerr -env $menv -create \
	    -auto_commit -mode 0644} -btree $dbname]
	error_check_good open [is_valid_db $mdb] TRUE
	set cdb [eval {berkdb_open_noerr -env $cenv -create \
	    -auto_commit -mode 0644} -btree $dbname]
	error_check_good open [is_valid_db $cdb] TRUE

	puts "\tRep$tnum.b: Start master and client now."
	error_check_good master [$menv rep_start -master] 0
	error_check_good client [$cenv rep_start -client] 0

	set envlist "{$menv 1} {$cenv 2}"
	process_msgs $envlist 0 NONE err
	error_check_good msg_err [is_substr $err "never part of"] 1

	puts "\tRep$tnum.c: Clean up."
	error_check_good cdb_close [$cdb close] 0
	error_check_good cdb_close [$mdb close] 0

	error_check_good menv_close [$menv close] 0
	error_check_good cenv_close [$cenv close] 0

	replclose $testdir/MSGQUEUEDIR
	set testdir $orig_tdir
	return
}

