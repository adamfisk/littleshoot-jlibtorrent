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


    public long getMaxByteForTorrent(final File torrentFile)
        {
        try
            {
            final String path = torrentFile.getCanonicalPath();
            return get_max_byte_for_torrent(path);
            }
        catch (IOException e)
            {
            // TODO Auto-generated catch block
            e.printStackTrace();
            }
        return -1;
        }
    
    public void startSession()
        {
        start();
        }
    
    public void stopSession()
        {
        stop();
        }
    
    native long get_max_byte_for_torrent(final String path);
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
