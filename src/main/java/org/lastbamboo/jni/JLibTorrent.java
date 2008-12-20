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
    
    public String getFullSavePath(final File torrentFile) throws IOException
        {
        final String path = torrentFile.getCanonicalPath();
        return get_save_path_for_torrent(path);
        }
    
    public long getSizeForTorrent(final File torrentFile) throws IOException
        {
        final String path = torrentFile.getCanonicalPath();
        return get_size_for_torrent(path);
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
    
    public String getName(final File torrentFile) throws IOException
        {
        final String path = torrentFile.getCanonicalPath();
        return get_name_for_torrent(path);
        }
    
    public int getNumFiles(final File torrentFile) throws IOException
        {
        final String path = torrentFile.getCanonicalPath();
        return get_num_files_for_torrent(path);
        }

    public double getDownloadSpeed(final File torrentFile) throws IOException
        {
        final String path = torrentFile.getCanonicalPath();
        return get_speed_for_torrent(path);
        }
    
    public int getNumHosts(final File torrentFile) throws IOException
        {
        final String path = torrentFile.getCanonicalPath();
        return get_num_peers_for_torrent(path);
        }
    
    public long getBytesRead(final File torrentFile) throws IOException
        {
        final String path = torrentFile.getCanonicalPath();
        return get_bytes_read_for_torrent(path);
        }
    
    private native long get_bytes_read_for_torrent(final String path);
    
    private native int get_num_peers_for_torrent(final String path);
    
    private native double get_speed_for_torrent(final String path);

    private native int get_num_files_for_torrent(final String path);
    
    private native int get_state_for_torrent(final String path);
    
    private native long get_size_for_torrent(final String path);

    private native String get_name_for_torrent(final String path);
    
    private native String get_save_path_for_torrent(final String path);
    
    private native void remove_torrent(final String path);
    
    private native long get_max_byte_for_torrent(final String path);
    
    /**
     * Initialize the libtorrent core.
     */
    private native void start();
    
    /**
     * Shuts down the libtorrent core.
     */
    private native void stop();
    
    private native long add_torrent(String incompletePath, String torrentPath, int size);

    
    }
