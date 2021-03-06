/*-
 * DO NOT EDIT: automatically built by dist/s_java_stat.
 *
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2005
 *	Sleepycat Software.  All rights reserved.
 */

package com.sleepycat.db;

import com.sleepycat.db.internal.DbUtil;

public class CompactStats
{
    // no public constructor
    protected CompactStats() {}

    /* package */
    CompactStats(int fillpercent, int timeout, int pages) {
        this.compact_fillpercent = fillpercent;
        this.compact_timeout = timeout;
        this.compact_pages = pages;
    }

    private int compact_fillpercent;
    public int getFillPercent() {
        return compact_fillpercent;
    }

    private int compact_timeout;
    public int getTimeout() {
        return compact_timeout;
    }

    private int compact_pages;
    public int getPages() {
        return compact_pages;
    }

    private int compact_pages_free;
    public int getPagesFree() {
        return compact_pages_free;
    }

    private int compact_pages_examine;
    public int getPagesExamine() {
        return compact_pages_examine;
    }

    private int compact_levels;
    public int getLevels() {
        return compact_levels;
    }

    private int compact_deadlock;
    public int getDeadlock() {
        return compact_deadlock;
    }

    private int compact_pages_truncated;
    public int getPagesTruncated() {
        return compact_pages_truncated;
    }

    private int compact_truncate;
    public int getTruncate() {
        return compact_truncate;
    }

    public String toString() {
        return "CompactStats:"
            + "\n  compact_fillpercent=" + compact_fillpercent
            + "\n  compact_timeout=" + compact_timeout
            + "\n  compact_pages=" + compact_pages
            + "\n  compact_pages_free=" + compact_pages_free
            + "\n  compact_pages_examine=" + compact_pages_examine
            + "\n  compact_levels=" + compact_levels
            + "\n  compact_deadlock=" + compact_deadlock
            + "\n  compact_pages_truncated=" + compact_pages_truncated
            + "\n  compact_truncate=" + compact_truncate
            ;
    }
}
// end of TransactionStats.java
