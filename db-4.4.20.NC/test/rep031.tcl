# See the file LICENSE for redistribution information.
#
# Copyright (c) 2004-2005
#	Sleepycat Software.  All rights reserved.
#
# $Id: rep031.tcl,v 12.9 2005/10/20 15:48:12 carol Exp $
#
# TEST	rep031
# TEST	Test of internal initialization and blocked operations.
# TEST
# TEST	One master, one client.
# TEST	Put one more record to the master.
# TEST  Test that internal initialization blocks:
# TEST  log_archive, rename, remove, fileid_reset, lsn_reset.
# TEST	Sleep 30+ seconds.
# TEST  Test that blocked operations are now unblocked.
#
proc rep031 { method { niter 200 } { tnum "031" } args } {

	source ./include.tcl
	if { $is_windows9x_test == 1 } { 
		puts "Skipping replication test on Win 9x platform."
		return
	} 

	# There is nothing method-sensitive in this test, so 
	# skip for all except btree.
	if { [is_btree $method] != 1 } {
		puts "Skipping rep031 for method $method."
		return
	}
	
	set args [convert_args $method $args]

	# This test needs to set its own pagesize.
	set pgindex [lsearch -exact $args "-pagesize"]
        if { $pgindex != -1 } {
                puts "Rep$tnum: skipping for specific pagesizes"
                return
        }

	# Run the body of the test with and without recovery,
	# and with and without cleaning.
	set recopts { "" " -recover " }
	set cleanopts { clean noclean }
	foreach r $recopts {
		foreach c $cleanopts {
			puts "Rep$tnum ($method $r $c $args): Test of\
			    internal initialization and blocked operations."
			rep031_sub $method $niter $tnum $r $c $args
		}
	}
}

proc rep031_sub { method niter tnum recargs clean largs } {
	source ./include.tcl

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

	# Open a master.
	repladd 1
	set ma_envcmd "berkdb_env_noerr -create -txn nosync \
	    -log_buffer $log_buf -log_max $log_max \
	    -home $masterdir -rep_transport \[list 1 replsend\]"
#	set ma_envcmd "berkdb_env_noerr -create -txn nosync \
#	    -log_buffer $log_buf -log_max $log_max \
#	    -verbose {rep on} -errpfx MASTER \
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

	# Bring the clients online by processing the startup messages.
	set envlist "{$masterenv 1} {$clientenv 2}"
	process_msgs $envlist

	# Run rep_test in the master (and update client).
	puts "\tRep$tnum.a: Running rep_test in replicated env."
	eval rep_test $method $masterenv NULL $niter 0 0 0 0 $largs
	process_msgs $envlist

	puts "\tRep$tnum.b: Close client."
	error_check_good client_close [$clientenv close] 0

	# Find out what exists on the client.  We need to loop until
	# the first master log file > last client log file. 
	set res [eval exec $util_path/db_archive -l -h $clientdir]

	set last_client_log [lindex [lsort $res] end]

	set stop 0
	while { $stop == 0 } {
		# Run rep_test in the master (don't update client).
		puts "\tRep$tnum.c: Running rep_test in replicated env."
		eval rep_test $method $masterenv NULL $niter 0 0 0 0 $largs
		replclear 2

		puts "\tRep$tnum.d: Run db_archive on master."
		set res [eval exec $util_path/db_archive -d -h $masterdir]
		set res [eval exec $util_path/db_archive -l -h $masterdir]
		if { [lsearch -exact $res $last_client_log] == -1 } {
			set stop 1
		}
	}

	puts "\tRep$tnum.e: Reopen client ($clean)."
	if { $clean == "clean" } {
		env_cleanup $clientdir
	}
	set clientenv [eval $cl_envcmd $recargs -rep_client]
	error_check_good client_env [is_valid_env $clientenv] TRUE
	set envlist "{$masterenv 1} {$clientenv 2}"
	process_msgs $envlist 0 NONE err
	if { $clean == "noclean" } {
		puts "\tRep$tnum.e.1: Trigger log request"
		#
		# When we don't clean, starting the client doesn't
		# trigger any events.  We need to generate some log
		# records so that the client requests the missing
		# logs and that will trigger it.
		#
		set entries 10
		eval rep_test $method $masterenv NULL $entries $niter 0 0 0 $largs
		process_msgs $envlist 0 NONE err
	}

	#
	# We have now forced an internal initialization.  Verify it is correct.
	#
	puts "\tRep$tnum.f: Verify logs and databases"
	rep_verify $masterdir $masterenv $clientdir $clientenv 1

	#
	# Internal initializations disable certain operations on the master for
	# 30 seconds after the last init-related message is received
	# by the master.  Those operations are dbremove, dbrename and
	# log_archive (with removal).
	#
	puts "\tRep$tnum.g: Try to remove and rename the database."

	set dbname "test.db"
	set old $dbname
	set new $dbname.new
	set stat [catch {$masterenv dbrename -auto_commit $old $new} ret]
	error_check_good rename_fail $stat 1
	error_check_good rename_err [is_substr $ret "invalid"] 1
	set stat [catch {$masterenv dbremove -auto_commit $old} ret]
	error_check_good remove_fail $stat 1
	error_check_good remove_err [is_substr $ret "invalid"] 1

	puts "\tRep$tnum.h: Try to reset LSNs and fileid on the database."
	set stat [catch {$masterenv id_reset $old} ret]
	error_check_good id_reset $stat 1
	error_check_good id_err [is_substr $ret "invalid"] 1
	set stat [catch {$masterenv lsn_reset $old} ret]
	error_check_good lsn_reset $stat 1
	error_check_good lsn_err [is_substr $ret "invalid"] 1

	#
	# Need entries big enough to generate additional log files.
	# However, db_archive will not return an error, it will
	# just retain the log file.
	#
	puts "\tRep$tnum.i: Run rep_test to generate more logs."
	set entries 200
	eval rep_test $method $masterenv NULL $entries $niter 0 0 0 $largs
	process_msgs $envlist 0 NONE err

	puts "\tRep$tnum.j: Try to db_archive."
	set res [eval exec $util_path/db_archive -l -h $masterdir]
	set first [lindex $res 0]
	set res [eval exec $util_path/db_archive -d -h $masterdir]
	set res [eval exec $util_path/db_archive -l -h $masterdir]
	error_check_bad log.gone [lsearch -exact $res $first] -1

	puts "\tRep$tnum.j.0: Try to log_archive in master env."
	set res [$masterenv log_archive -arch_remove]
	set res [eval exec $util_path/db_archive -l -h $masterdir]
	error_check_bad log.gone0 [lsearch -exact $res $first] -1

	# We can't open a second handle on the env in HP-UX.
	if { $is_hp_test != 1 } {
		puts "\tRep$tnum.j.1: Try to log_archive in new non-rep env."
		set newenv [berkdb_env_noerr -txn nosync \
		    -log_buffer $log_buf -log_max $log_max \
		    -home $masterdir]
		error_check_good newenv [is_valid_env $newenv] TRUE
		set res [$newenv log_archive -arch_remove]
		set res [eval exec $util_path/db_archive -l -h $masterdir]
		error_check_bad log.gone1 [lsearch -exact $res $first] -1
	}

	#
	# Sleep 32 seconds - The timeout is 30 seconds, but we need
	# to sleep a bit longer to make sure we cross the timeout.
	#
	set to 32
	puts "\tRep$tnum.k: Wait $to seconds to timeout"
	tclsleep $to
	puts "\tRep$tnum.l: Retry blocked operations after wait"
	set stat [catch {$masterenv id_reset $old} ret]
	error_check_good id_reset_work $stat 0
	set stat [catch {$masterenv lsn_reset $old} ret]
	error_check_good lsn_reset_work $stat 0
	set stat [catch {$masterenv dbrename -auto_commit $old $new} ret]
	error_check_good rename_work $stat 0
	set stat [catch {$masterenv dbremove -auto_commit $new} ret]
	error_check_good remove_work $stat 0
	process_msgs $envlist 0 NONE err

	# Remove the files via the 2nd non-rep env, check via db_archive.
	if { $is_hp_test != 1 } {
		set res [$newenv log_archive -arch_remove]
		set res [eval exec $util_path/db_archive -l -h $masterdir]
		error_check_good log.gone [lsearch -exact $res $first] -1
		error_check_good newenv_close [$newenv close] 0
	} else {
		set res [$masterenv log_archive -arch_remove]
		set res [eval exec $util_path/db_archive -l -h $masterdir]
		error_check_good log.gone [lsearch -exact $res $first] -1
	}
	error_check_good masterenv_close [$masterenv close] 0
	error_check_good clientenv_close [$clientenv close] 0
	replclose $testdir/MSGQUEUEDIR
}
