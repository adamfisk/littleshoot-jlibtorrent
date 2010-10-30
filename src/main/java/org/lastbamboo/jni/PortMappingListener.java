package org.lastbamboo.jni;

/**
 * Listener for port mapping events.
 */
public interface PortMappingListener {

    /**
     * Called when an external port has been mapped.
     * 
     * @param port The port.
     */
    void externalPortMapped(int port);
}
