/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2005
 *	Sleepycat Software.  All rights reserved.
 *
 * $Id: CacheFilePriority.java,v 12.1 2005/06/16 20:22:58 bostic Exp $
 */

package com.sleepycat.db;

import com.sleepycat.db.internal.DbConstants;

public final class CacheFilePriority {
    public static final CacheFilePriority DEFAULT =
        new CacheFilePriority("DEFAULT", DbConstants.DB_PRIORITY_DEFAULT);
    public static final CacheFilePriority HIGH =
        new CacheFilePriority("HIGH", DbConstants.DB_PRIORITY_HIGH);
    public static final CacheFilePriority LOW =
        new CacheFilePriority("LOW", DbConstants.DB_PRIORITY_LOW);
    public static final CacheFilePriority VERY_HIGH =
        new CacheFilePriority("VERY_HIGH", DbConstants.DB_PRIORITY_VERY_HIGH);
    public static final CacheFilePriority VERY_LOW =
        new CacheFilePriority("VERY_LOW", DbConstants.DB_PRIORITY_VERY_LOW);

    /* package */
    static CacheFilePriority fromFlag(int flag) {
        switch (flag) {
        case DbConstants.DB_PRIORITY_DEFAULT:
            return DEFAULT;
        case DbConstants.DB_PRIORITY_HIGH:
            return HIGH;
        case DbConstants.DB_PRIORITY_LOW:
            return LOW;
        case DbConstants.DB_PRIORITY_VERY_HIGH:
            return VERY_HIGH;
        case DbConstants.DB_PRIORITY_VERY_LOW:
            return VERY_LOW;
        default:
            throw new IllegalArgumentException(
                "Unknown cache priority: " + flag);
        }
    }

    private final String priorityName;
    private final int flag;

    private CacheFilePriority(final String priorityName, final int flag) {
        this.priorityName = priorityName;
        this.flag = flag;
    }

    public String toString() {
        return "CacheFilePriority." + priorityName;
    }

    /* package */
    int getFlag() {
        return flag;
    }
}
