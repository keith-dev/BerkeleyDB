# See the file LICENSE for redistribution information.
#
# Copyright (c) 2006
#	Oracle Corporation.  All rights reserved.
#
# $Id: rep062.tcl,v 1.5 2006/09/11 18:54:25 carol Exp $
#
# TEST	rep062
# TEST	Test of internal initialization where client has a different
# TEST  kind of database than the master.
# TEST
# TEST	Create a master of one type, and let the client catch up.
# TEST	Close the client.
# TEST	Remove the database on the master, and create a new
# TEST	database of the same name but a different type.
# TEST	Run the master ahead far enough that internal initialization
# TEST	will be required on the reopen of the client.
# TEST	Reopen the client and verify.

proc rep062 { method {tnum "062"} args } {

	source ./include.tcl
	if { $is_windows9x_test == 1 } {
		puts "Skipping replication test on Win 9x platform."
		return
	}

	# This test uses different access methods internally.
	# Called from outside, accept only btree.
	if { $checking_valid_methods } {
		set test_methods { btree }
		return $test_methods
	}
	if { [is_btree $method] != 1 } {
		puts "Skipping rep$tnum for method $method."
		return
	}

	# This test needs to set its own pagesize.
	set pgindex [lsearch -exact $args "-pagesize"]
	if { $pgindex != -1 } {
		puts "Rep$tnum: skipping for specific pagesizes"
		return
	}

	set logsets [create_logsets 2]

	# Run the body of the test with and without recovery,
	# and with and without cleaning.
	foreach r $test_recopts {
		foreach l $logsets {
			set logindex [lsearch -exact $l "in-memory"]
			if { $r == "-recover" && $logindex != -1 } {
				puts "Skipping rep$tnum for -recover\
				    with in-memory logs."
				continue
			}
			puts "Rep$tnum ($method $r):\
			    Internal initialization with change in\
			    access method of database."
			puts "Rep$tnum: Master logs are [lindex $l 0]"
			puts "Rep$tnum: Client logs are [lindex $l 1]"
			rep062_sub $method $tnum $l $r $args
		}
	}
}

proc rep062_sub { method tnum logset recargs largs } {
	global testdir
	global util_path
	global passwd
	global encrypt

	set masterdir $testdir/MASTERDIR
	set clientdir $testdir/CLIENTDIR

	# Log size is small so we quickly create more than one.
	# The documentation says that the log file must be at least
	# four times the size of the in-memory log buffer.
	set maxpg 16384
	set log_buf [expr $maxpg * 2]
	set log_max [expr $log_buf * 4]
	set cache [expr $maxpg * 32 ]

	set m_logargs " -log_buffer $log_buf"
	set c_logargs " -log_buffer $log_buf"

	set m_logtype [lindex $logset 0]
	set c_logtype [lindex $logset 1]

	# In-memory logs cannot be used with -txn nosync.
	set m_logargs [adjust_logargs $m_logtype]
	set c_logargs [adjust_logargs $c_logtype]
	set m_txnargs [adjust_txnargs $m_logtype]
	set c_txnargs [adjust_txnargs $c_logtype]

	# Set up pairs of databases to test.  The first element is whether
	# to open an encrypted env, the second is the original database
	# method and flags, the third is the replacement database and flags.
	set pairlist {
	    { 0 {btree ""} {hash ""} }
	    { 0 {queueext "-pagesize 2048"} {queue ""} }
	    { 0 {queueext ""} {btree ""} }
	    { 0 {queue ""} {recno ""} }
	    { 0 {hash ""} {queue ""} }
	    { 0 {recno ""} {btree ""} }
	    { 0 {hash ""} {queueext "-pagesize 16384"} }
	    { 0 {queueext "-pagesize 2048"} {queueext "-pagesize 16384"} }
	    { 0 {queueext "-pagesize 16384"} {queueext "-pagesize 2048"} }
	    { 0 {queue ""} {queueext "-pagesize 16384"} }
	    { 1 {btree ""} {btree "-encrypt"} }
	    { 1 {btree "-encrypt"} {btree ""} }
	    { 1 {queue ""} {queue "-encrypt"} }
	    { 1 {queue "-encrypt"} {queue ""} }
	}

	foreach p $pairlist {
		env_cleanup $testdir
		replsetup $testdir/MSGQUEUEDIR

		file mkdir $masterdir
		file mkdir $clientdir

		# Extract values from the list.
		set encryptenv [lindex [lindex $p 0] 0]
		set encryptmsg "clear"
		if { $encryptenv == 1 } {
			set encryptmsg "encrypted"
		}
		set method1 [lindex [lindex $p 1] 0]
		set method2 [lindex [lindex $p 2] 0]
		set flags1 [lindex [lindex $p 1] 1]
		set flags2 [lindex [lindex $p 2] 1]

		puts "Rep$tnum: Testing with $encryptmsg env."
		puts -nonewline "Rep$tnum: Replace [lindex $p 1] "
		puts "database with [lindex $p 2] database."

		# Set up flags for encryption if necessary.
		set envflags ""
		set enc ""
		if { $encryptenv == 1 } {
			set envflags "-encryptaes $passwd"
			set enc " -P $passwd"
		}

		# Derive args for specified methods.
		set args1 [convert_args $method1 ""]
		set args2 [convert_args $method2 ""]

		# Open a master.
		repladd 1
		set ma_envcmd "berkdb_env_noerr -create $m_txnargs \
		    $m_logargs -log_max $log_max \
		    -cachesize { 0 $cache 1 } $envflags \
		    -home $masterdir -rep_transport \[list 1 replsend\]"
#		set ma_envcmd "berkdb_env_noerr -create $m_txnargs \
#		    $m_logargs -log_max $log_max \
#		    -cachesize { 0 $cache 1 } $envflags \
#		    -verbose {rep on} -errpfx MASTER -errfile /dev/stderr \
#		    -home $masterdir -rep_transport \[list 1 replsend\]"
		set masterenv [eval $ma_envcmd $recargs -rep_master]
		error_check_good master_env [is_valid_env $masterenv] TRUE

		# Open a client.
		repladd 2
		set cl_envcmd "berkdb_env_noerr -create $c_txnargs \
		    $c_logargs -log_max $log_max \
		    -cachesize { 0 $cache 1 } $envflags \
		    -home $clientdir -rep_transport \[list 2 replsend\]"
#		set cl_envcmd "berkdb_env_noerr -create $c_txnargs \
#		    $c_logargs -log_max $log_max \
#		    -cachesize { 0 $cache 1 } $envflags \
#		    -verbose {rep on} -errpfx CLIENT -errfile /dev/stderr \
#		    -home $clientdir -rep_transport \[list 2 replsend\]"
		set clientenv [eval $cl_envcmd $recargs -rep_client]
		error_check_good client_env [is_valid_env $clientenv] TRUE

		# Bring the client online by processing the startup messages.
		set envlist "{$masterenv 1} {$clientenv 2}"
		process_msgs $envlist

		# Open two databases on the master - one to test different
		# methods, one to advance the log, forcing internal
		# initialization.

		puts "\tRep$tnum.a: Open test database (it will change methods)."
		set testfile "test.db"
		set omethod [convert_method $method1]
		set db1 [eval {berkdb_open} -env $masterenv -auto_commit \
		    -create $omethod $flags1 $args1 -mode 0644 $testfile]
		error_check_good db1open [is_valid_db $db1] TRUE

		puts "\tRep$tnum.b: Open log-advance database."
		set testfile2 "test2.db"
		set db2 [eval {berkdb_open} -env $masterenv -auto_commit \
		    -create $omethod $args1 -mode 0644 $flags1 $testfile2]
		error_check_good db2open [is_valid_db $db2] TRUE

		puts "\tRep$tnum.c: Add a few records to test db."
		set nentries 10
		set start 0
		eval rep_test $method1 \
		    $masterenv $db1 $nentries $start $start 0 0 $args1
		incr start $nentries
		process_msgs $envlist

		puts "\tRep$tnum.d: Close client."

		# First save the log number of the latest client log.
		set last_client_log [get_logfile $clientenv last]
		error_check_good client_close [$clientenv close] 0

		# Remove the database on the master, create new database
		# of different type.
		puts "\tRep$tnum.e: Remove test database."
		error_check_good db1_close [$db1 close] 0
		error_check_good db1_remove [$masterenv dbremove $testfile] 0

		puts "\tRep$tnum.f: \
		    Create new test database; same name, different method."
		set omethod [convert_method $method2]
		set db1 [eval {berkdb_open} -env $masterenv -auto_commit \
		    -create $omethod $args2 -mode 0644 $flags2 $testfile]
		error_check_good db1open [is_valid_db $db1] TRUE

		# Run rep_test in the master enough to require internal
		# initialization upon client reopen.  Use the extra db.
		set stop 0
		set niter 100
		while { $stop == 0 } {
			# Run rep_test in the master (don't update client).
			puts "\tRep$tnum.g: \
			    Run rep_test until internal init is required."
		 	eval rep_test $method1 $masterenv \
			    $db2 $niter $start $start 0 0 $largs
			incr start $niter
			replclear 2

			puts "\tRep$tnum.h: Run db_archive on master."
			if { $m_logtype != "in-memory"} {
				set res [eval exec \
				    $util_path/db_archive $enc -d -h $masterdir]
				set res [eval exec \
				    $util_path/db_archive $enc -l -h $masterdir]
			}
			set first_master_log [get_logfile $masterenv first]
			if { $first_master_log > $last_client_log } {
				set stop 1
			}
		}

		puts "\tRep$tnum.i: Reopen client."
		set clientenv [eval $cl_envcmd $recargs -rep_client]
		error_check_good client_env [is_valid_env $clientenv] TRUE

		set envlist "{$masterenv 1} {$clientenv 2}"
		process_msgs $envlist 0 NONE err

		puts "\tRep$tnum.j: Add a few records to cause initialization."
		set entries 20
		eval rep_test $method2 \
		    $masterenv $db1 $entries $start $start 0 0 $largs
		incr start $entries
		process_msgs $envlist 0 NONE err

		puts "\tRep$tnum.k: Verify logs and databases"
		# Make sure encryption value is correct.
		if { $encryptenv == 1 } {
			set encrypt 1
		}
		rep_verify $masterdir $masterenv $clientdir $clientenv 1

		check_log_location $masterenv
		check_log_location $clientenv

		error_check_good db1_close [$db1 close] 0
		error_check_good db2_close [$db2 close] 0
		error_check_good masterenv_close [$masterenv close] 0
		error_check_good clientenv_close [$clientenv close] 0
		replclose $testdir/MSGQUEUEDIR
	}
}

