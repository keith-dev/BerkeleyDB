# See the file LICENSE for redistribution information.
#
# Copyright (c) 2003-2004
#	Sleepycat Software.  All rights reserved.
#
# $Id: rep018script.tcl,v 1.5 2004/09/22 18:01:06 bostic Exp $
#
# Rep018 script - concurrency with checkpoints.
#
# Test dbremove with replication.
#
# Usage: rep018script clientdir dbfile
# clientdir: client env directory
# niter: number of items in file
# dbfile: name of database file
#
source ./include.tcl
source $test_path/test.tcl
source $test_path/testutils.tcl
source $test_path/reputils.tcl

set usage "repscript clientdir niter dbfile method"

# Verify usage
if { $argc != 4 } {
	puts stderr "FAIL:[timestamp] Usage: $usage"
	exit
}

# Initialize arguments
set clientdir [ lindex $argv 0 ]
set niter [ lindex $argv 1 ]
set dbfile [ lindex $argv 2 ]
set method [ lindex $argv 3 ]

# Join the queue env.  We assume the rep test convention of
# placing the messages in $testdir/MSGQUEUEDIR.
set queueenv [eval berkdb_env -home $testdir/MSGQUEUEDIR]
error_check_good script_qenv_open [is_valid_env $queueenv] TRUE

#
# We need to set up our own machids.
# Add 1 for master env id, and 2 for the clientenv id.
#
repladd 1
repladd 2

# Join the client env.
set cl_cmd "berkdb_env_noerr -home $clientdir \
	-txn -rep_client -rep_transport \[list 2 replsend\]"
# set cl_cmd "berkdb_env_noerr -home $clientdir \
# 	-verbose {rep on} -errfile /dev/stderr \
# 	-txn -rep_client -rep_transport \[list 2 replsend\]"
set clientenv [eval $cl_cmd]
error_check_good script_cenv_open [is_valid_env $clientenv] TRUE

# Make sure we can read data on client.
set db [eval "berkdb_open -env $clientenv $dbfile"]
for { set i 1 } { $i <= $niter } { incr i } {
	set ret [lindex [$db get $i] 0]
	error_check_good db_get $ret [list $i [pad_data $method data$i]]
}

# Put a timestamp in a shared file.
set markerenv [berkdb_env -create -home $testdir -txn]
error_check_good markerenv_open [is_valid_env $markerenv] TRUE
set marker \
    [eval "berkdb_open -create -btree -auto_commit -env $markerenv marker.db"]
error_check_good timestamp_ready \
    [$marker put -auto_commit CHILDREADY [timestamp -r]] 0

# Give the parent a chance to process messages and hang.
tclsleep 30

# Clean up the child so the parent can go forward.
error_check_good timestamp_done \
    [$marker put -auto_commit CHILDDONE [timestamp -r]] 0
error_check_good client_db_close [$db close] 0

# Check that the master is done.
while { [llength [$marker get PARENTDONE]] == 0 } {
	tclsleep 1
}

# Verify that the newly recreated database is now empty.
set db [eval "berkdb_open -env $clientenv $dbfile"]
set cursor [$db cursor]
error_check_good db_empty [llength [$cursor get -first]] 0
error_check_good cursor_close [$cursor close] 0
error_check_good db_close [$db close] 0
error_check_good marker_db_close [$marker close] 0
error_check_good markerenv_close [$markerenv close] 0
error_check_good script_client_close [$clientenv close] 0

