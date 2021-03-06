# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996-2001
#	Sleepycat Software.  All rights reserved.
#
# $Id: test009.tcl,v 11.5 2001/01/25 18:23:08 bostic Exp $
#
# DB Test 9 {access method}
# Check that we reuse overflow pages.  Create database with lots of
# big key/data pairs.  Go through and delete and add keys back
# randomly.  Then close the DB and make sure that we have everything
# we think we should.
proc test009 { method {nentries 10000} args} {
	eval {test008 $method $nentries 9 0} $args
}
