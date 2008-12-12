package org.lastbamboo.jni;

public class JLibTorrent
    {

    static
        {
        System.loadLibrary("jnltorrent");
        }
    
    public JLibTorrent()
        {
        System.out.println("Instance created");
        }

    /**
     * Initialize the libtorrent core.
     */
    native void start();
    
    /**
     * Shuts down the libtorrent core.
     */
    native void stop();

    native int get_torrent_handle(String srg);
    
    native void download_torrent(String url);
    
    native int add_torrent(String torrentData, int size);//byte[] torrent);
    
    native void processEvents();
    }
