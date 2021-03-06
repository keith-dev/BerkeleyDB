# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996, 1997, 1998, 1999, 2000
#	Sleepycat Software.  All rights reserved.
#
#	$Id: dead004.tcl,v 11.2 2001/05/17 20:37:04 bostic Exp $
#
# Deadlock Test 4.
# This test is designed to make sure that we handle youngest and oldest
# deadlock detection even when the youngest and oldest transactions in the
# system are not involved in the deadlock (that is, we want to abort the
# youngest/oldest which is actually involved in the deadlock, not simply
# the youngest/oldest in the system).
# Since this is used for transaction systems, the locker ID is what we
# use to identify age (smaller number is older).
#
# The set up is that we have a total of 6 processes.  The oldest (locker 0)
# and the youngest (locker 5) simply acquire a lock, hold it for a long time
# and then release it.  The rest form a ring, obtaining lock N and requesting
# a lock on (N+1) mod 4.  The deadlock detector ought to pick locker 1 or 4
# to abort  and not 0 or 5.

proc dead004 { } {
	source ./include.tcl

	foreach a { o y } {
		puts "Dead004: Deadlock detector test -a $a"
		env_cleanup $testdir

		# Create the environment.
		puts "\tDead004.a: creating environment"
		set env [berkdb env -create -mode 0644 -lock -home $testdir]
		error_check_good lock_env:open [is_valid_env $env] TRUE
		error_check_good lock_env:close [$env close] 0

		set dpid [exec $util_path/db_deadlock -v -t 5 -a $a \
		    -h $testdir >& $testdir/dd.out &]

		set pidlist ""
		set procs 6

		foreach n $procs {

			sentinel_init

			# Fire off the tests
			puts "\tDead004: $n procs"
			for { set i 0 } { $i < $n } { incr i } {
				puts "$tclsh_path $test_path/wrap.tcl \
				    $testdir/dead004.log.$i \
				    ddoyscript.tcl $testdir $i $n $a"
				set p [exec $tclsh_path \
					$test_path/wrap.tcl \
					ddoyscript.tcl $testdir/dead004.log.$i \
					$testdir $i $n $a &]
				lappend pidlist $p
			}
			watch_procs 5

		}
		# Now check output
		set dead 0
		set clean 0
		set other 0
		for { set i 0 } { $i < $n } { incr i } {
			set did [open $testdir/dead004.log.$i]
			while { [gets $did val] != -1 } {
				switch $val {
					DEADLOCK { incr dead }
					1 { incr clean }
					default { incr other }
				}
			}
			close $did
		}
		puts "dead check..."
		dead_check oldyoung $n $dead $clean $other

		# Now verify that neither the oldest nor the
		# youngest were the deadlock.
		set did [open $testdir/dead004.log.0]
		error_check_bad file:young [gets $did val] -1
		error_check_good read:young $val 1
		close $did

		set did [open $testdir/dead004.log.[expr $procs - 1]]
		error_check_bad file:old [gets $did val] -1
		error_check_good read:old $val 1
		close $did

		exec $KILL $dpid

		# Windows needs files closed before deleting files,
		# so pause a little
		tclsleep 2
		fileremove -f $testdir/dd.out

		# Remove log files
		for { set i 0 } { $i < $n } { incr i } {
			fileremove -f $testdir/dead004.log.$i
		}
	}
}
