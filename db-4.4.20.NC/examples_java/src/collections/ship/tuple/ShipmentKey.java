/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2005
 *	Sleepycat Software.  All rights reserved.
 *
 * $Id: ShipmentKey.java,v 12.2 2005/06/16 20:22:46 bostic Exp $
 */

package collections.ship.tuple;

/**
 * A ShipmentKey serves as the key in the key/data pair for a shipment entity.
 *
 * <p> In this sample, ShipmentKey is bound to the key's tuple storage entry
 * using a TupleBinding.  Because it is not used directly as storage data, it
 * does not need to be Serializable. </p>
 *
 * @author Mark Hayes
 */
public class ShipmentKey {

    private String partNumber;
    private String supplierNumber;

    public ShipmentKey(String partNumber, String supplierNumber) {

        this.partNumber = partNumber;
        this.supplierNumber = supplierNumber;
    }

    public final String getPartNumber() {

        return partNumber;
    }

    public final String getSupplierNumber() {

        return supplierNumber;
    }

    public String toString() {

        return "[ShipmentKey: supplier=" + supplierNumber +
	    " part=" + partNumber + ']';
    }
}
