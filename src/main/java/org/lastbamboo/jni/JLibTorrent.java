package org.lastbamboo.jni;

import java.io.File;
import java.io.IOException;
import java.util.Collection;

/**
 * Java wrapper class for calls to native lib torrent.
 */
public class JLibTorrent
    {
    
    private long m_totalUploadBytes;
    private long m_totalDownloadBytes;
    private float m_uploadRate;
    private float m_downloadRate;
    private int m_numPeers;

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
        
        cacheMethodIds();
        start();
        }
    
    public void updateSessionStatus() 
        {
        update_session_status();
        }
    
    public void download(final File incompleteDir, final File torrentFile, 
        final boolean sequential) throws IOException 
        {
        final long handle = add_torrent(incompleteDir.getCanonicalPath(), 
            torrentFile.getCanonicalPath(), (int) torrentFile.length(), 
            sequential);
        }
    
    public String moveToDownloadsDir(final File torrentFile)
        {
        final String path = normalizePath(torrentFile);
        return move_to_downloads_dir(path);
        }

    public long getMaxByteForTorrent(final File torrentFile)
        {
        final String path = normalizePath(torrentFile);
        return get_max_byte_for_torrent(path);
        }
    
    public void startSession()
        {
        start();
        }
    
    public void stopSession()
        {
        stop();
        }
    
    public void pauseTorrent(final File torrentFile)
        {
        final String path = normalizePath(torrentFile);
        pause_torrent(path);
        }
    
    public void resumeTorrent(final File torrentFile)
        {
        final String path = normalizePath(torrentFile);
        resume_torrent(path);
        }
    
    public long getSizeForTorrent(final File torrentFile)
        {
        final String path = normalizePath(torrentFile);
        return get_size_for_torrent(path);
        }
    

    public void removeTorrent(final File torrentFile)
        {
        final String path = normalizePath(torrentFile);
        remove_torrent(path);
        }
    
    public int getStateForTorrent(final File torrentFile)
        {
        final String path = normalizePath(torrentFile);
        return get_state_for_torrent(path);
        }
    
    public String getName(final File torrentFile)
        {
        final String path = normalizePath(torrentFile);
        return get_name_for_torrent(path);
        }
    
    public int getNumFiles(final File torrentFile)
        {
        final String path = normalizePath(torrentFile);
        return get_num_files_for_torrent(path);
        }

    public double getDownloadSpeed(final File torrentFile)
        {
        final String path = normalizePath(torrentFile);
        return get_speed_for_torrent(path);
        }
    
    public int getNumHosts(final File torrentFile)
        {
        final String path = normalizePath(torrentFile);
        return get_num_peers_for_torrent(path);
        }
    
    public long getBytesRead(final File torrentFile)
        {
        final String path = normalizePath(torrentFile);
        return get_bytes_read_for_torrent(path);
        }
    
    /*
    public float getUploadRate()
        {
        return get_upload_rate();
        }
    
    public float getDownloadRate()
        {
        return get_download_rate();
        }
        
    public long getTotalDownloadBytes()
        {
        return get_total_download_bytes();
        }
    
    public long getTotalUploadBytes()
        {
        return get_total_upload_bytes();
        }
        */
    
    private final String normalizePath(final File torrentFile)
        {
        try
            {
            return torrentFile.getCanonicalPath();
            }
        catch (final IOException e)
            {
            return torrentFile.getAbsolutePath();
            }
        }
    
    private native void cacheMethodIds();
    
    private native void update_session_status();
    
    private native float get_upload_rate();
    private native float get_download_rate();
    private native long get_total_download_bytes();
    private native long get_total_upload_bytes();
    
    private native String move_to_downloads_dir(final String path);
    
    private native long get_bytes_read_for_torrent(final String path);
    
    private native int get_num_peers_for_torrent(final String path);
    
    private native double get_speed_for_torrent(final String path);

    private native int get_num_files_for_torrent(final String path);
    
    private native int get_state_for_torrent(final String path);
    
    private native long get_size_for_torrent(final String path);

    private native String get_name_for_torrent(final String path);
    
    private native void remove_torrent(final String path);
    
    private native void pause_torrent(final String path);
    
    private native void resume_torrent(final String path);
    
    private native long get_max_byte_for_torrent(final String path);
    
    /**
     * Initialize the libtorrent core.
     */
    private native void start();
    
    /**
     * Shuts down the libtorrent core.
     */
    private native void stop();
    
    private native long add_torrent(String incompletePath, String torrentPath, 
        int size, boolean sequential);

    private void setTotalUploadBytes(final long totalUploadBytes)
        {
        m_totalUploadBytes = totalUploadBytes;
        }

    public long getTotalUploadBytes()
        {
        return m_totalUploadBytes;
        }

    private void setTotalDownloadBytes(final long totalDownloadBytes)
        {
        m_totalDownloadBytes = totalDownloadBytes;
        }

    public long getTotalDownloadBytes()
        {
        return m_totalDownloadBytes;
        }

    private void setDownloadRate(final float downloadRate)
        {
        m_downloadRate = downloadRate;
        }

    public float getDownloadRate()
        {
        return m_downloadRate;
        }

    private void setUploadRate(float uploadRate)
        {
        m_uploadRate = uploadRate;
        }

    public float getUploadRate()
        {
        return m_uploadRate;
        }

    private void setNumPeers(int numPeers)
        {
        m_numPeers = numPeers;
        }

    public int getNumPeers()
        {
        return m_numPeers;
        }

    
    }
