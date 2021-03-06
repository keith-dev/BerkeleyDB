# See the file LICENSE for redistribution information.
#
# Copyright (c) 2004
#	Sleepycat Software.  All rights reserved.
#
# $Id: rep020.tcl,v 1.10 2004/09/22 18:01:06 bostic Exp $
#
# TEST  rep020
# TEST	Replication elections - test election generation numbers.
# TEST

proc rep020 { method args } {
	global rand_init

	set tnum "020"
	if { [is_btree $method] == 0 } {
		puts "Rep$tnum: Skipping for method $method."
		return
	}

	error_check_good set_random_seed [berkdb srand $rand_init] 0
	set nclients 5
	set logsets [create_logsets [expr $nclients + 1]]
	foreach l $logsets {
		puts "Rep$tnum ($method): Election generation test."
		puts "Rep$tnum: Master logs are [lindex $l 0]"
		for { set i 0 } { $i < $nclients } { incr i } {
			puts "Rep$tnum: Client $i logs are\
			    [lindex $l [expr $i + 1]]"
		}
		rep020_sub $method $nclients $tnum $l $args
	}
}

proc rep020_sub { method nclients tnum logset args } {
	source ./include.tcl
	global errorInfo
	global mixed_mode_logging
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

	# Run a modified test001 in the master.
	process_msgs $envlist
	puts "\tRep$tnum.a: Running rep_test in replicated env."
	set niter 10
	eval rep_test $method $masterenv NULL $niter 0 0
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
		set pri($i) 10
		set crash($i) 0
#		error_check_good pfx [$clientenv($i) errpfx CLIENT$i] 0
#		error_check_good verb [$clientenv($i) verbose rep on] 0
#		$clientenv($i) errfile /dev/stderr
#		set env_cmd($i) [concat $env_cmd($i) \
#		    "-errpfx CLIENT$i -verbose {rep on} -errfile /dev/stderr"]
	}

	set msg "Rep$tnum.b"
	puts "\t$msg: Run elections to increment egen."

	set nelect 2
	set nsites $nclients
	set nvotes $nclients
	for { set j 0 } { $j < $nelect } { incr j } {
		# Pick winner and elector randomly.
		set winner [berkdb random_int 0 [expr $nclients - 1]]
		setpriority pri $nclients $winner
		set elector [berkdb random_int 0 [expr $nclients - 1]]
		run_election env_cmd envlist err_cmd pri crash\
		    $qdir $msg $elector $nsites $nvotes $nclients $winner 1
	}
	process_msgs $envlist

	set msg "Rep$tnum.c"
	puts "\t$msg: Updating egen when getting an old vote."

	#
	# Find the last client and save the election generation number.
	# Close the last client and adjust the list of envs to process.
	#
	set i [expr $nclients - 1]
	set last [lindex $envlist end]
	set clientenv($i) [lindex $last 0]
	set egen($i) \
	    [stat_field $clientenv($i) rep_stat "Election generation number"]
	error_check_good clientenv_close($i) [$clientenv($i) close] 0
	set envlist [lreplace $envlist end end]

	# Run a few more elections while the last client is closed.
	# Make sure we don't pick the closed client as the winner,
	# and require votes from one fewer site.
	#
	set orig_nvotes $nvotes
	set orig_nclients $nclients
	set nvotes [expr $orig_nvotes - 1]
	set nclients [expr $orig_nclients - 1]
	for { set j 0 } { $j < $nelect } { incr j } {
		set winner [berkdb random_int 0 [expr $nclients - 1]]
		setpriority pri $nclients $winner
		set elector [berkdb random_int 0 [expr $nclients - 1]]
		run_election env_cmd envlist err_cmd pri crash\
		    $qdir $msg $elector $nsites $nvotes $nclients $winner 1
	}
	process_msgs $envlist
	#
	# Verify that the last client's election generation number has
	# changed, and that it matches the other clients.
	#
	set pair [lindex $envlist 0]
	set clenv [lindex $pair 0]
	set clegen [stat_field \
	    $clenv rep_stat "Election generation number"]

	# Reopen last client's env.  Do not run recovery, but do
	# process messages to get the egen updated.
	replclear $envid
	set clientenv($i) [eval $env_cmd($i)]
	lappend envlist "$clientenv($i) $envid"
	error_check_good client_reopen [is_valid_env $clientenv($i)] TRUE
	process_msgs $envlist

	set newegen($i) \
	    [stat_field $clientenv($i) rep_stat "Election generation number"]
	error_check_bad egen_changed $newegen($i) $egen($i)
	error_check_good egen_changed1 $newegen($i) $clegen

	set msg "Rep$tnum.d"
	puts "\t$msg: New client starts election."
	#
	# Run another election, this time called by the last client.
	# This should succeed because the last client has already
	# caught up to the others for egen.
	#
	set winner 2
	set nvotes $orig_nvotes
	set nclients $orig_nclients
	set elector [expr $nclients - 1]
	setpriority pri $nclients $winner
	run_election env_cmd envlist err_cmd pri crash\
	    $qdir $msg $elector $nsites $nvotes $nclients $winner 0

	set newegen($i) \
	    [stat_field $clientenv($i) rep_stat "Election generation number"]
	foreach pair $envlist {
		set i [expr [lindex $pair 1] - 2]
		set clientenv($i) [lindex $pair 0]
		set egen($i) [stat_field \
		    $clientenv($i) rep_stat "Election generation number"]
	}
	error_check_good egen_catchup $egen(4) $egen(3)

	# Skip this part of the test for mixed-mode logging,
	# since we can't recover with in-memory logs.
	if { $mixed_mode_logging == 0 } {
		set msg "Rep$tnum.e"
		puts "\t$msg: Election generation is not changed in recovery."
		# Note all client egens.  Close, recover, process messages,
		# and check that egens are unchanged.
		foreach pair $envlist {
			set i [expr [lindex $pair 1] - 2]
			set clientenv($i) [lindex $pair 0]
			set egen($i) [stat_field $clientenv($i) \
			    rep_stat "Election generation number"]
			error_check_good \
			    clientenv_close($i) [$clientenv($i) close] 0
			set clientenv($i) [eval $env_cmd($i) -recover]
			set envlist [lreplace \
			    $envlist $i $i "$clientenv($i) [expr $i + 2]"]
		}
		process_msgs $envlist
		foreach pair $envlist {
			set newegen($i) [stat_field $clientenv($i) \
			    rep_stat "Election generation number"]
			error_check_good egen_recovery $egen($i) $newegen($i)
		}

		# Run an election.  Now the egens should go forward.
		set winner [berkdb random_int 0 [expr $nclients - 1]]
		setpriority pri $nclients $winner
		set elector [berkdb random_int 0 [expr $nclients - 1]]
		run_election env_cmd envlist err_cmd pri crash \
		    $qdir $msg $elector $nsites $nvotes $nclients $winner 1

		foreach pair $envlist {
			set i [expr [lindex $pair 1] - 2]
			set clientenv($i) [lindex $pair 0]
			set newegen($i) [stat_field $clientenv($i) \
			    rep_stat "Election generation number"]
			error_check_good \
			    egen_forward [expr $newegen($i) > $egen($i)] 1
		}
	}

	foreach pair $envlist {
		set cenv [lindex $pair 0]
                error_check_good cenv_close [$cenv close] 0
        }

	replclose $testdir/MSGQUEUEDIR
}

