/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2006
 *      Oracle Corporation.  All rights reserved.
 *
 * $Id: MultipleKeyNIODataEntry.java,v 1.3 2006/09/08 20:32:14 bostic Exp $
 */

package com.sleepycat.db;

import com.sleepycat.db.internal.DbConstants;
import com.sleepycat.db.internal.DbUtil;

import java.nio.ByteBuffer;

public class MultipleKeyNIODataEntry extends MultipleEntry {
    public MultipleKeyNIODataEntry() {
        super(null);
    }

    public MultipleKeyNIODataEntry(final ByteBuffer data) {
        super(data);
    }

    /**
     * Return the bulk retrieval flag and reset the entry position so that the
     * next set of key/data can be returned.
     */
    /* package */
    int getMultiFlag() {
        pos = 0;
        return DbConstants.DB_MULTIPLE_KEY;
    }

    public boolean next(final DatabaseEntry key, final DatabaseEntry data) {
        byte[] intarr;
        int saveoffset;
        if (pos == 0)
            pos = ulen - INT32SZ;

        // pull the offsets out of the ByteBuffer.
        if(this.data_nio.capacity() < 16)
            return false;
        intarr = new byte[16];
        saveoffset = this.data_nio.position();
        this.data_nio.position(pos - INT32SZ*3);
        this.data_nio.get(intarr, 0, 16);
        this.data_nio.position(saveoffset);

        final int keyoff = DbUtil.array2int(intarr, 12);

        // crack out the key and data offsets and lengths.
        if (keyoff < 0)
            return false;

        final int keysz = DbUtil.array2int(intarr, 8);
        final int dataoff = DbUtil.array2int(intarr, 4);
        final int datasz = DbUtil.array2int(intarr, 0);

        // move the position to one before the last offset read.
        pos -= INT32SZ*4;

        key.setDataNIO(this.data_nio);
        key.setOffset(keyoff);
        key.setSize(keysz);

        data.setDataNIO(this.data_nio);
        data.setOffset(dataoff);
        data.setSize(datasz);

        return true;
    }
}
