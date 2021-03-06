# See the file LICENSE for redistribution information.
#
# Copyright (c) 2005
#	Sleepycat Software.  All rights reserved.
#
# $Id: rep044.tcl,v 12.4 2005/10/18 19:05:54 carol Exp $
#
# TEST	rep044
# TEST	
# TEST	Test rollbacks with open file ids. 
# TEST
# TEST	We have one master with two handles and one client. 
# TEST	Each time through the main loop, we open a db, write
# TEST	to the db, and close the db.  Each one of these actions
# TEST	is propagated to the client, or a roll back is forced 
# TEST	by swapping masters.  

proc rep044 { method { tnum "044" } args } {

	source ./include.tcl
	if { $is_windows9x_test == 1 } { 
		puts "Skipping replication test on Win 9x platform."
		return
	} 
	set args [convert_args $method $args]
	set logsets [create_logsets 2]

	# HP-UX can't open two handles on the same env, so it
	# can't run this test.
	if { $is_hp_test == 1 } {
		puts "Skipping rep$tnum for HP-UX."
		return
	}

	foreach l $logsets {
		set logindex [lsearch -exact $l "in-memory"]
		puts "Rep$tnum ($method): Replication with rollbacks\
		    and open file ids."
		puts "Rep$tnum: Master logs are [lindex $l 0]"
		puts "Rep$tnum: Client 0 logs are [lindex $l 1]"
		rep044_sub $method $tnum $l $args
	}
}

proc rep044_sub { method tnum logset largs } {
	global testdir
	source ./include.tcl
	set orig_tdir $testdir

	set masterdir $testdir/ENV0
	set clientdir $testdir/ENV1

	set m_logtype [lindex $logset 0]
	set c_logtype [lindex $logset 1]

	# In-memory logs require a large log buffer, and cannot
	# be used with -txn nosync.
	set m_logargs [adjust_logargs $m_logtype]
	set c_logargs [adjust_logargs $c_logtype]
	set m_txnargs [adjust_txnargs $m_logtype]
	set c_txnargs [adjust_txnargs $c_logtype]

	set niter 20
	set omethod [convert_method $method]

	# The main loop runs all the permutations of processing/not
	# processing the database open to the clients; processing/not 
	# processing the database writes to the clients; and processing/
	# not processing the database close to the clients.  Set up the
	# options in advance so the loop is not heavily indented. 
	#
	# Each entry displays { open write close }. 
	# For example { 1 1 0 } means we process messages after the 
	# db open and the db writes but not after the db close. 

	set optionsets {
		{1 1 1} 
		{1 1 0}
		{1 0 1}
		{1 0 0}
		{0 1 1}
		{0 1 0}
		{0 0 1}
		{0 0 0}
	}

	# Main loop.
	foreach set $optionsets {

		env_cleanup $testdir
		replsetup $testdir/MSGQUEUEDIR
		file mkdir $masterdir
		file mkdir $clientdir

		set processopens [lindex $set 0]
		set processwrites [lindex $set 1]
		set processcloses [lindex $set 2]

		set notdoing {}
		if { $processopens == 0 } { 
			append notdoing " OPENS" 
		}
		if { $processwrites == 0 } { 
			append notdoing " WRITES"
		}
		if { $processcloses == 0 } {
			append notdoing " CLOSES"
		}
		if { $notdoing != {} } {
			puts "Rep$tnum:\
			    Loop with $notdoing not processed to client."
		}

		# Open a master.
		repladd 1
		set envcmd(M0) "berkdb_env_noerr -create $m_txnargs \
		    $m_logargs -lock_max 2500 -lock_detect default \
		    -home $masterdir -rep_transport \[list 1 replsend\]"
#		set envcmd(M0) "berkdb_env_noerr -create $m_txnargs \
#		    $m_logargs -lock_max 2500 -lock_detect default \
#		    -errpfx ENV.M0 -verbose {rep on} -errfile /dev/stderr \
#		    -home $masterdir -rep_transport \[list 1 replsend\]"
		set menv0 [eval $envcmd(M0) -rep_master]
		error_check_good master_env0 [is_valid_env $menv0] TRUE

		# Open second handle on master env. 
		set envcmd(M1) "berkdb_env_noerr $m_txnargs \
		    $m_logargs -lock_max 2500 -lock_detect default \
		    -home $masterdir -rep_transport \[list 1 replsend\]"
#		set envcmd(M1) "berkdb_env_noerr -create $m_txnargs \
#		    $m_logargs -lock_max 2500 -lock_detect default \
#		    -errpfx ENV.M1 -verbose {rep on} -errfile /dev/stderr \
#		    -home $masterdir -rep_transport \[list 1 replsend\]"
		set menv1 [eval $envcmd(M1)]
		error_check_good master_env [is_valid_env $menv1] TRUE
		error_check_good rep_start [$menv1 rep_start -master] 0

		# Open a client
		repladd 2
		set envcmd(C) "berkdb_env_noerr -create $c_txnargs \
		    $c_logargs -lock_max 2500 -errpfx ENV.C \
		    -errfile /dev/stderr -lock_detect default \
		    -home $clientdir -rep_transport \[list 2 replsend\]"
#		set envcmd(C) "berkdb_env_noerr -create $c_txnargs \
#	  	  $c_logargs -lock_max 2500 -lock_detect default \
#		    -errpfx ENV.C -verbose {rep on} -errfile /dev/stderr \
#		    -home $clientdir -rep_transport \[list 2 replsend\]"
		set cenv [eval $envcmd(C) -rep_client]
		error_check_good client_env [is_valid_env $cenv] TRUE

		# Bring the client online by processing the startup messages.
		set envlist "{$menv0 1} {$cenv 2}"
		process_msgs $envlist

		puts "\tRep$tnum.a: Run rep_test in 1st master env."
		set start 0
		eval rep_test $method $menv0 NULL $niter $start $start 0 0 $largs
		incr start $niter
		process_msgs $envlist

		puts "\tRep$tnum.b: Open db in 2nd master env."
		# Open the db here; we want it to remain open after rep_test.
		set db1 [eval {berkdb_open_noerr -env $menv1 -auto_commit \
		     -mode 0644} $largs $omethod test.db]
		error_check_good dbopen [is_valid_db $db1] TRUE

		if { $processopens == 1 } {
			puts "\tRep$tnum.b1:\
			    Process db open messages to client."
			process_msgs $envlist
		} else {
			set start [do_switch $method $niter $start $menv0 $cenv $largs]
		}

		puts "\tRep$tnum.c: Write to database in 2nd master."
		# We don't use rep_test here, because sometimes we abort.
		for { set i 1 } { $i <= $niter } { incr i } {
			set t [$menv1 txn]
			set key $i
			set str STRING.$i
			if [catch {eval {$db1 put}\
			    -txn $t {$key [chop_data $method $str]}} result] {
				# If handle is dead, abort txn, then
				# close and reopen db.
				error_check_good handle_dead \
				    [is_substr $result HANDLE_DEAD] 1
				error_check_good txn_abort [$t abort] 0
				error_check_good close_handle [$db1 close] 0
				set db1 [eval {berkdb_open_noerr \
				    -env $menv1 -auto_commit -mode 0644}\
				    $largs $omethod test.db]
			} else {
				error_check_good txn_commit [$t commit] 0
			}
		}

		if { $processwrites == 1 } {
			puts "\tRep$tnum.c1:\
			    Process db put messages to client."
			process_msgs $envlist
		} else {
			set start [do_switch $method $niter $start $menv0 $cenv $largs]
		}	

		puts "\tRep$tnum.d: Close database using 2nd master env handle."
		error_check_good db_close [$db1 close] 0

		if { $processcloses == 1 } {
			puts "\tRep$tnum.d1:\
			    Process db close messages to client."
			process_msgs $envlist
		} else {
			set start [do_switch $method $niter $start $menv0 $cenv $largs]
		}	

		puts "\tRep$tnum.e: Clean up."	
		error_check_good menv0_close [$menv0 close] 0
		error_check_good menv1_close [$menv1 close] 0
		error_check_good cenv_close [$cenv close] 0

		replclose $testdir/MSGQUEUEDIR
	}
	set testdir $orig_tdir
	return
}

proc do_switch { method niter start masterenv clientenv largs } {
	set envlist "{$masterenv 1} {$clientenv 2}"

	# Downgrade master, upgrade client.
	error_check_good master_downgrade [$masterenv rep_start -client] 0
	error_check_good client_upgrade [$clientenv rep_start -master] 0
	process_msgs $envlist

	# Run rep_test in the new master.
	eval rep_test $method $clientenv NULL $niter $start $start 0 0 $largs
	incr start $niter
	process_msgs $envlist
	
	# Downgrade newmaster, upgrade original master.
	error_check_good client_downgrade [$clientenv rep_start -client] 0
	error_check_good master_upgrade [$masterenv rep_start -master] 0

	# Run rep_test in the restored master.
	eval rep_test $method $masterenv NULL $niter $start $start 0 0 $largs
	incr start $niter
	process_msgs $envlist

	return $start
}
