package org.lastbamboo.jni;

import java.io.File;
import java.io.IOException;


public class JLibTorrent
    {
    
    public JLibTorrent()
        {
        System.out.println("Instance created");
        System.loadLibrary("jnltorrent");
        }
    
    public JLibTorrent(final String libraryPath)
        {
        System.load(libraryPath);
        }
    
    public void download(final File torrentFile) throws IOException
        {
        add_torrent(torrentFile.getCanonicalPath(), (int) torrentFile.length());
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
    
    native int add_torrent(String torrentData, int size);
    
    native void processEvents();

    }
