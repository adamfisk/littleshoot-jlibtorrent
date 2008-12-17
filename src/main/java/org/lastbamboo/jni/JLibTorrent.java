package org.lastbamboo.jni;

import java.io.File;
import java.io.IOException;
import java.util.Collection;

/**
 * Java wrapper class for calls to native lib torrent.
 */
public class JLibTorrent
    {
    
    public JLibTorrent(final Collection<File> libFiles)
        {
        boolean libLoaded = false;
        for (final File lib : libFiles)
            {
            if (lib.isFile())
                {
                System.load(lib.getAbsolutePath());
                libLoaded = true;
                break;
                }
            }
        if (!libLoaded)
            {
            System.loadLibrary("jnltorrent");
            }
        init();
        }
    
    public JLibTorrent()
        {
        System.loadLibrary("jnltorrent");
        init();
        }
    
    public JLibTorrent(final String libraryPath)
        {
        System.load(libraryPath);
        init();
        }
    
    private void init()
        {
        final Runnable hookRunner = new Runnable()
            {
            public void run()
                {
                System.out.println("Stopping LibTorrent...");
                stop();
                }
            };
        
        final Thread hook = new Thread(hookRunner, "LibTorrent-Shutdown-Thread");
        Runtime.getRuntime().addShutdownHook(hook);
        start();
        }
    
    public void download(final File incompleteDir, File torrentFile) throws IOException
        {
        final long handle = add_torrent(incompleteDir.getCanonicalPath(), 
            torrentFile.getCanonicalPath(), (int) torrentFile.length());
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
    
    public void udpateFileData()
        {
        processEvents();
        }
    
    public String getSavePath(final File torrentFile)
        {
        try
            {
            final String path = torrentFile.getCanonicalPath();
            return get_save_path_for_torrent(path);
            }
        catch (final IOException e)
            {
            // TODO Auto-generated catch block
            e.printStackTrace();
            }
        return "";
        }
    
    public long getSizeForTorrent(final File torrentFile)
        {
        try
            {
            final String path = torrentFile.getCanonicalPath();
            return get_size_for_torrent(path);
            }
        catch (final IOException e)
            {
            e.printStackTrace();
            }
        return -1L;
        }
    

    public void removeTorrent(final File torrentFile)
        {
        try
            {
            final String path = torrentFile.getCanonicalPath();
            remove_torrent(path);
            }
        catch (final IOException e)
            {
            e.printStackTrace();
            }
        }
    
    public int getStateForTorrent(final File torrentFile)
        {
        try
            {
            final String path = torrentFile.getCanonicalPath();
            return get_state_for_torrent(path);
            }
        catch (final IOException e)
            {
            e.printStackTrace();
            }
        return -1;
        }
    
    private native int get_state_for_torrent(final String path);
    
    private native long get_size_for_torrent(final String path);

    private native String get_save_path_for_torrent(final String path);
    
    private native void remove_torrent(final String path);
    
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
    
    native long add_torrent(String incompletePath, String torrentPath, int size);
    
    native void processEvents();

    }
