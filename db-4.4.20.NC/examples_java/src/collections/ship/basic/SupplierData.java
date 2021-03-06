/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2005
 *	Sleepycat Software.  All rights reserved.
 *
 * $Id: SupplierData.java,v 12.2 2005/06/16 20:22:23 bostic Exp $
 */

package collections.ship.basic;

import java.io.Serializable;

/**
 * A SupplierData serves as the data in the key/data pair for a supplier
 * entity.
 *
 * <p> In this sample, SupplierData is used both as the storage entry for the
 * data as well as the object binding to the data.  Because it is used
 * directly as storage data using serial format, it must be Serializable. </p>
 *
 * @author Mark Hayes
 */
public class SupplierData implements Serializable {

    private String name;
    private int status;
    private String city;

    public SupplierData(String name, int status, String city) {

        this.name = name;
        this.status = status;
        this.city = city;
    }

    public final String getName() {

        return name;
    }

    public final int getStatus() {

        return status;
    }

    public final String getCity() {

        return city;
    }

    public String toString() {

        return "[SupplierData: name=" + name +
	    " status=" + status +
	    " city=" + city + ']';
    }
}
