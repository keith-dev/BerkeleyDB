# See the file LICENSE for redistribution information.
#
# Copyright (c) 2004
#	Sleepycat Software.  All rights reserved.
#
# $Id: rep054.tcl,v 1.3 2005/10/18 19:05:54 carol Exp $
#
# TEST	rep054
# TEST	Test of internal initialization where a far-behind
# TEST	client takes over as master.
# TEST
# TEST	One master, two clients.
# TEST	Run rep_test and process.
# TEST	Close client 1.
# TEST	Run rep_test, opening new databases, and processing 
# TEST	messages.  Archive as we go so that log files get removed.
# TEST	Close master and reopen client 1 as master.  Process messages.
# TEST	Verify that new master and client are in sync.
# TEST	Run rep_test again, adding data to one of the new 
# TEST	named databases.

proc rep054 { method { nentries 200 } { tnum "054" } args } {
	set args [convert_args $method $args]

	# This test needs to set its own pagesize.
	set pgindex [lsearch -exact $args "-pagesize"]
	if { $pgindex != -1 } {
		puts "Rep$tnum: skipping for specific pagesizes"
		return
	}

	# Run the body of the test with and without recovery.
	set recopts { "" " -recover " }
	foreach r $recopts {
		puts "Rep$tnum ($method $r $args):  Internal\
		    initialization test: far-behind client\
		    becomes master."
		rep054_sub $method $nentries $tnum \
		    $r $args
	}
}

proc rep054_sub { method nentries tnum recargs largs } {
	global testdir
	global util_path

	env_cleanup $testdir
	set omethod [convert_method $method]

	replsetup $testdir/MSGQUEUEDIR

	set masterdir $testdir/MASTERDIR
	set clientdir $testdir/CLIENTDIR
	set clientdir2 $testdir/CLIENTDIR2

	file mkdir $masterdir
	file mkdir $clientdir
	file mkdir $clientdir2

	# Log size is small so we quickly create more than one.
	# The documentation says that the log file must be at least
	# four times the size of the in-memory log buffer.
	set pagesize 4096
	append largs " -pagesize $pagesize "
	set log_buf [expr $pagesize * 2]
	set log_max [expr $log_buf * 4]

	# Open a master.
	repladd 1
	set ma_envcmd "berkdb_env_noerr -create -txn nosync \
	    -log_buffer $log_buf -log_max $log_max \
	    -home $masterdir -rep_transport \[list 1 replsend\]"
#	set ma_envcmd "berkdb_env_noerr -create -txn nosync \
#	    -log_buffer $log_buf -log_max $log_max \
#	    -verbose {rep on} -errpfx MASTER -errfile /dev/stderr \
#	    -home $masterdir -rep_transport \[list 1 replsend\]"
	set masterenv [eval $ma_envcmd $recargs -rep_master]
	error_check_good master_env [is_valid_env $masterenv] TRUE

	# Open a client
	repladd 2
	set cl_envcmd "berkdb_env_noerr -create -txn nosync \
	    -log_buffer $log_buf -log_max $log_max \
	    -home $clientdir -rep_transport \[list 2 replsend\]"
#	set cl_envcmd "berkdb_env_noerr -create -txn nosync \
#	    -log_buffer $log_buf -log_max $log_max \
#	    -verbose {rep on} -errpfx CLIENT \
#	    -home $clientdir -rep_transport \[list 2 replsend\]"
	set clientenv [eval $cl_envcmd $recargs -rep_client]
	error_check_good client_env [is_valid_env $clientenv] TRUE

	# Open 2nd client
	repladd 3
	set cl2_envcmd "berkdb_env_noerr -create -txn nosync \
	    -log_buffer $log_buf -log_max $log_max \
	    -home $clientdir2 -rep_transport \[list 3 replsend\]"
#	set cl2_envcmd "berkdb_env_noerr -create -txn nosync \
#	    -log_buffer $log_buf -log_max $log_max \
#	    -verbose {rep on} -errpfx CLIENT2 \
#	    -home $clientdir2 -rep_transport \[list 3 replsend\]"
	set clientenv2 [eval $cl2_envcmd $recargs -rep_client]
	error_check_good client2_env [is_valid_env $clientenv2] TRUE

	# Bring the clients online by processing the startup messages.
	set envlist "{$masterenv 1} {$clientenv 2} {$clientenv2 3}"
	process_msgs $envlist

	# Run rep_test in the master and in each client.
	puts "\tRep$tnum.a: Running rep_test in master & clients."
	set start 0
	eval rep_test $method $masterenv NULL $nentries $start $start 0 0 $largs
	set start [expr $nentries + $start]
	process_msgs $envlist

	# Master is in sync with both clients.
	rep_verify $masterdir $masterenv $clientdir $clientenv
	# Process messages again in case we are running with debug_rop.
	process_msgs $envlist
	rep_verify $masterdir $masterenv $clientdir2 $clientenv2

	puts "\tRep$tnum.b: Close client 1."
	error_check_good client_close [$clientenv close] 0
	set envlist "{$masterenv 1} {$clientenv2 3}"

	# Find out what exists on the client.  Loop until the first 
	# master log file is greater than the last client log file.
	set res [eval exec $util_path/db_archive -l -h $clientdir]
	set last_client_log [lindex [lsort $res] end]

	set stop 0
	while { $stop == 0 } {
		# Run rep_test in the master (don't update client).
		puts "\tRep$tnum.c: Running rep_test in replicated env."
		eval rep_test $method $masterenv NULL $nentries \
		    $start $start 0 0 $largs
		set start [expr $nentries + $start]
		replclear 2

		puts "\tRep$tnum.d: Run db_archive on master."
		set res [eval exec $util_path/db_archive -d -h $masterdir]
		set res [eval exec $util_path/db_archive -l -h $masterdir]
		if { [lsearch -exact $res $last_client_log] == -1 } {
			set stop 1
		}
	}

	# Create a database that does not even exist on client 1.
	set newfile "newtest.db"
	set newdb [eval {berkdb_open_noerr -env $masterenv -create \
	    -auto_commit -mode 0644} $largs $omethod $newfile]
	error_check_good newdb_open [is_valid_db $newdb] TRUE
	eval rep_test $method $masterenv $newdb $nentries $start $start 0 0 $largs
	set start [expr $start + $nentries]
	process_msgs $envlist

	# Identify last master log file.
	set res [eval exec $util_path/db_archive -l -h $masterdir]
	set last_master_log [lindex [lsort $res] end]
	set stop 0

	# Send the master and client2 far ahead of client 1.  Archive
	# so there will be a gap between the log files of the closed
	# client and the active master and client and we've
	# archived away the creation of the new database.
	puts "\tRep$tnum.e: Running rep_test in master & remaining client."
	while { $stop == 0 } {

		eval rep_test \
		    $method $masterenv NULL $nentries $start $start 0 0 $largs
		set start [expr $start + $nentries]

		process_msgs $envlist

		puts "\tRep$tnum.f: Archiving ..."
		set res [eval exec $util_path/db_archive -d -h $masterdir]
		set res [eval exec $util_path/db_archive -d -h $clientdir2]
		if { [lsearch -exact $res $last_master_log] == -1 } {
			set stop 1 
		}
	}
	process_msgs $envlist

	# Master is in sync with client 2.
	rep_verify $masterdir $masterenv $clientdir2 $clientenv2

	# Close master.
	puts "\tRep$tnum.g: Close master."
	error_check_good newdb_close [$newdb close] 0
	error_check_good close_master [$masterenv close] 0

	# The new database is still there. 
	error_check_good newfile_exists [file exists $masterdir/$newfile] 1

	puts "\tRep$tnum.h: Reopen client1 as master."
	replclear 2
	set newmasterenv [eval $cl_envcmd $recargs -rep_master]
	error_check_good newmasterenv [is_valid_env $newmasterenv] TRUE
	# Archive the first log file away.
	set res [$newmasterenv log_archive -arch_remove]

	puts "\tRep$tnum.i: Reopen master as client."
	set oldmasterenv [eval $ma_envcmd $recargs -rep_client]
	error_check_good oldmasterenv [is_valid_env $oldmasterenv] TRUE
	set envlist "{$oldmasterenv 1} {$newmasterenv 2} {$clientenv2 3}"

	puts "\tRep$tnum.j: Verify error."
	process_msgs $envlist 0 NONE err
	error_check_bad err $err 0
	error_check_good errchk [is_substr $err "Client too far ahead"] 1

	error_check_good newmasterenv_close [$newmasterenv close] 0
	error_check_good oldmasterenv_close [$oldmasterenv close] 0
	error_check_good clientenv2_close [$clientenv2 close] 0
	replclose $testdir/MSGQUEUEDIR
}
