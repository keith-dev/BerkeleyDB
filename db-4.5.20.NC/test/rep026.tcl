# See the file LICENSE for redistribution information.
#
# Copyright (c) 2004-2006
#	Oracle Corporation.  All rights reserved.
#
# $Id: rep026.tcl,v 12.12 2006/09/13 21:51:23 carol Exp $
#
# TEST	rep026
# TEST	Replication elections - simulate a crash after sending
# TEST	a vote.

proc rep026 { method args } {
	source ./include.tcl

	global mixed_mode_logging
	set tnum "026"
	if { $is_windows9x_test == 1 } {
		puts "Skipping replication test on Win 9x platform."
		return
	}

	# Run for btree only.
	if { $checking_valid_methods } {
		set test_methods { btree }
		return $test_methods
	}
	if { [is_btree $method] == 0 } {
		puts "Rep$tnum: Skipping for method $method."
		return
	}

	# This test uses recovery, so mixed-mode testing isn't
	# appropriate.
	if { $mixed_mode_logging > 0 } {
		puts "Rep$tnum: Skipping for mixed-mode logging."
		return
	}

	global rand_init
	error_check_good set_random_seed [berkdb srand $rand_init] 0

	set nclients 5
	set logsets [create_logsets [expr $nclients + 1]]
	foreach l $logsets {
		puts "Rep$tnum ($method): Election generations -\
		    simulate crash after sending a vote."
		puts "Rep$tnum: Master logs are [lindex $l 0]"
		for { set i 0 } { $i < $nclients } { incr i } {
			puts "Rep$tnum: Client $i logs are\
			    [lindex $l [expr $i + 1]]"
		}
		rep026_sub $method $nclients $tnum $l $args
	}
}

proc rep026_sub { method nclients tnum logset largs } {
	source ./include.tcl
	global errorInfo
	global machids queuedbs
	env_cleanup $testdir

	set qdir $testdir/MSGQUEUEDIR
	replsetup $qdir

	set masterdir $testdir/MASTERDIR
	file mkdir $masterdir
	set m_logtype [lindex $logset 0]
	set m_logargs [adjust_logargs $m_logtype]
	set m_txnargs [adjust_txnargs $m_logtype]

	for { set i 0 } { $i < $nclients } { incr i } {
		set clientdir($i) $testdir/CLIENTDIR.$i
		file mkdir $clientdir($i)
		set c_logtype($i) [lindex $logset [expr $i + 1]]
		set c_logargs($i) [adjust_logargs $c_logtype($i)]
		set c_txnargs($i) [adjust_txnargs $c_logtype($i)]
	}

# To debug elections, the lines to uncomment are below the
# error checking portion of this test.  This is needed in order
# for the error messages to come back in errorInfo and for
# that portion of the test to pass.
	# Open a master.
	set envlist {}
	repladd 1
	set env_cmd(M) "berkdb_env -create -log_max 1000000 \
	    -home $masterdir $m_txnargs $m_logargs -rep_master \
	    -errpfx MASTER -rep_transport \[list 1 replsend\]"
	set masterenv [eval $env_cmd(M)]
	error_check_good master_env [is_valid_env $masterenv] TRUE
	lappend envlist "$masterenv 1"

	# Open the clients.
	for { set i 0 } { $i < $nclients } { incr i } {
		set envid [expr $i + 2]
		repladd $envid
		set env_cmd($i) "berkdb_env_noerr -create \
		    -home $clientdir($i) $c_txnargs($i) $c_logargs($i) \
		    -rep_client -rep_transport \[list $envid replsend\]"
		set clientenv($i) [eval $env_cmd($i)]
		error_check_good \
		    client_env($i) [is_valid_env $clientenv($i)] TRUE
		lappend envlist "$clientenv($i) $envid"
	}
	# Bring the clients online by processing the startup messages.
	process_msgs $envlist

	# Run a modified test001 in the master.
	puts "\tRep$tnum.a: Running rep_test in replicated env."
	set niter 10
	eval rep_test $method $masterenv NULL $niter 0 0 0 0 $largs
	process_msgs $envlist
	error_check_good masterenv_close [$masterenv close] 0
	set envlist [lreplace $envlist 0 0]

# To debug elections, uncomment the lines below to turn on verbose
# and set the errfile.  Also edit reputils.tcl
# in proc start_election and swap the 2 commented lines with
# their counterpart.
	foreach pair $envlist {
		set i [expr [lindex $pair 1] - 2]
		replclear [expr $i + 2]
		set err_cmd($i) "none"
		set crash($i) 0
		set pri($i) 10
#		error_check_good pfx [$clientenv($i) errpfx CLIENT$i] 0
#		error_check_good verb [$clientenv($i) verbose rep on] 0
#		$clientenv($i) errfile /dev/stderr
#		set env_cmd($i) [concat $env_cmd($i) \
#		    "-errpfx CLIENT$i -verbose {rep on} -errfile /dev/stderr"]
	}

	# In each case we simulate a crash in client C, recover, and
	# call a second election.  We vary the caller of the second
	# election (C or some other client) and when the election
	# messages from before the crash are processed - before or
	# after the second election.
	#
	foreach option { "1 b before" "2 c before" "1 d after" "2 e after"} {
		# Elector 1 calls the first election, elector 2
		# calls the second election.
		set elector1 1
		set elector2 [lindex $option 0]
		set let [lindex $option 1]
		set restore [lindex $option 2]

		if { $elector1 == $elector2 } {
			puts "\tRep$tnum.$let: Simulated crash and recovery\
			    (crashing client calls second election)."
		} else {
			puts "\tRep$tnum.$let: Simulated crash and recovery\
			    (non-crashing client calls second election)."
		}

		puts "\tRep$tnum.$let: Process messages from crasher\
		    $restore 2nd election."

		puts "\t\tRep$tnum.$let.1: Note egens for all clients."
		# Note egens for all the clients.
		foreach pair $envlist {
			set i [expr [lindex $pair 1] - 2]
			set clientenv($i) [lindex $pair 0]
			set egen($i) [stat_field \
			    $clientenv($i) rep_stat "Election generation number"]
		}

		# Call an election which simulates a crash after sending
		# its VOTE1.
		set msg "\tRep$tnum.$let.2"
		puts "\t$msg: Start election, simulate a crash."
		set nsites $nclients
		set nvotes $nclients
		# Make the winner the crashing client, since the
		# crashing client will have the biggest LSN.
		set elector 1
		set winner $elector
		set crash($elector) 1
		setpriority pri $nclients $winner
		set err_cmd($elector) "electvote1"
		run_election env_cmd envlist err_cmd pri crash \
		    $qdir $msg $elector $nsites $nvotes $nclients $winner 0

		set msg "\tRep$tnum.$let.3"
		puts "\t$msg: Close and reopen elector with recovery."
		error_check_good \
		    clientenv_close($elector) [$clientenv($elector) close] 0

		# Have other clients SKIP the election messages and process
		# only C's startup messages.  We'll do it by copying the files
		# and emptying the originals.
		set cwd [pwd]
		foreach machid $machids {
			replready $machid to
			cd $qdir
			set msgdbs [glob -nocomplain ready.$machid.*]
			foreach msgdb $msgdbs {
				set savename [string replace $msgdb 0 5 save.]
				file copy -force $msgdb $savename
			}
			cd $cwd
			replclear $machid
		}

		# Reopen C and process messages.  Only the startup messages
		# will be available.
		set clientenv($elector) [eval $env_cmd($elector) -recover]
		set envlist [lreplace $envlist \
		    $elector $elector "$clientenv($elector) [expr $elector + 2]"]
		process_msgs $envlist

		# Verify egens (should be +1 in C, and unchanged in other clients).
		foreach pair $envlist {
			set i [expr [lindex $pair 1] - 2]
			set clientenv($i) [lindex $pair 0]
			set newegen($i) [stat_field \
			    $clientenv($i) rep_stat "Election generation number"]
			if { $i == $elector } {
				error_check_good egen+1 $newegen($i) [expr $egen($i) + 1]
			} else {
				error_check_good egen_unchanged $newegen($i) $egen($i)
			}
		}

		# First chance to restore messages.
		if { $restore == "before" } {
			restore_messages $qdir
		}

		# Have C call an election (no crash simulation) and process
		# all the messages.
		set msg "\tRep$tnum.$let.4"
		puts "\t$msg: Call second election."
		set err_cmd($elector) "none"
		set crash($elector) 0
		run_election env_cmd envlist err_cmd pri crash \
		    $qdir $msg $elector2 $nsites $nvotes $nclients $winner 1

		# Second chance to restore messages.
		if { $restore == "after" } {
			restore_messages $qdir
		}
		process_msgs $envlist

		# Verify egens (should be +2 or more in all clients).
		puts "\t\tRep$tnum.$let.5: Check egens."
		foreach pair $envlist {
			set i [expr [lindex $pair 1] - 2]
			set clientenv($i) [lindex $pair 0]
			set newegen($i) [stat_field \
			    $clientenv($i) rep_stat "Election generation number"]
			set mingen [expr $egen($i) + 2]
			error_check_good egen+more($i) \
			    [expr $newegen($i) >= $mingen] 1
		}
	}

	# Clean up.
	foreach pair $envlist {
		set cenv [lindex $pair 0]
		error_check_good cenv_close [$cenv close] 0
	}
	replclose $testdir/MSGQUEUEDIR
}

proc restore_messages { qdir } {
	global machids
	set cwd [pwd]
	foreach machid $machids {
		replready $machid to
		cd $qdir
		set msgdbs [glob -nocomplain save.$machid.*]
		foreach msgdb $msgdbs {
			set rdyname [string replace $msgdb 0 4 ready.]
			file copy -force $msgdb $rdyname
		}
		cd $cwd
	}
}

