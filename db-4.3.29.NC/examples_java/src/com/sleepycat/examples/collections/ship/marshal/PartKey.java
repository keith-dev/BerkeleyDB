/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2004
 *	Sleepycat Software.  All rights reserved.
 *
 * $Id: PartKey.java,v 1.3 2004/09/22 16:17:13 mark Exp $
 */

package com.sleepycat.examples.collections.ship.marshal;

import com.sleepycat.bind.tuple.TupleInput;
import com.sleepycat.bind.tuple.TupleOutput;

/**
 * A PartKey serves as the key in the key/data pair for a part entity.
 *
 * <p> In this sample, PartKey is bound to the stored key tuple entry by
 * implementing the MarshalledKey interface, which is called by {@link
 * SampleViews.MarshalledKeyBinding}. </p>
 *
 * @author Mark Hayes
 */
public class PartKey implements MarshalledKey {

    private String number;

    public PartKey(String number) {

        this.number = number;
    }

    public final String getNumber() {

        return number;
    }

    public String toString() {

        return "[PartKey: number=" + number + ']';
    }

    // --- MarshalledKey implementation ---

    PartKey() {

        // A no-argument constructor is necessary only to allow the binding to
        // instantiate objects of this class.
    }

    public void unmarshalKey(TupleInput keyInput) {

        this.number = keyInput.readString();
    }

    public void marshalKey(TupleOutput keyOutput) {

        keyOutput.writeString(this.number);
    }
}
