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
    private long m_totalPayloadUploadBytes;
    private long m_totalPayloadDownloadBytes;
    private float m_uploadRate;
    private float m_downloadRate;
    private float m_payloadUploadRate;
    private float m_payloadDownloadRate;
    private int m_numPeers;
    private final boolean m_isPro;

    public JLibTorrent(final Collection<File> libFiles, final boolean isPro)
        {
        this.m_isPro = isPro;
        boolean libLoaded = false;
        for (final File lib : libFiles)
            {
            if (lib.isFile())
                {
                System.load(lib.getAbsolutePath());
                libLoaded = true;
                System.out.println("Loading: "+lib);
                break;
                }
            }
        if (!libLoaded)
            {
            System.loadLibrary("jnltorrent");
            }
        init();
        }
    
    public JLibTorrent(final boolean isPro)
        {
        this.m_isPro = isPro;
        System.loadLibrary("jnltorrent");
        init();
        }
    
    public JLibTorrent(final String libraryPath, final boolean isPro)
        {
        this.m_isPro = isPro;
        System.load(libraryPath);
        init();
        }
    
    private void init()
        {
        cacheMethodIds();
        start(this.m_isPro);
        }
    
    public void updateSessionStatus() 
        {
        update_session_status();
        }
    
    public void download(final File incompleteDir, final File torrentFile, 
        final boolean sequential, final int torrentState) throws IOException 
        {
        final long handle = add_torrent(incompleteDir.getCanonicalPath(), 
            torrentFile.getCanonicalPath(), (int) torrentFile.length(), 
            sequential, torrentState);
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

    /**
     * Performs a "hard" resume of a torrent.  This is necessary when starting
     * torrents from previous sessions that were paused in the previous 
     * session.  The usual resume scenario doesn't appear to work in this 
     * case because simply setting a torrent to auto_managed that was never
     * started out of the pause state in the first place appears to have no 
     * effect.
     * 
     * @param torrentFile The torrent file.
     */
    public void hardResumeTorrent(final File torrentFile)
        {
        System.out.println("Performing hard resume");
        final String path = normalizePath(torrentFile);
        hard_resume_torrent(path);
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
    
    public void setMaxUploadSpeed(final int bytesPerSecond)
        {
        set_max_upload_speed(bytesPerSecond);
        }
    
    private native void set_max_upload_speed(final int bytesPerSecond);
    
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
    
    private native void hard_resume_torrent(final String path);
    
    private native long get_max_byte_for_torrent(final String path);
    
    /**
     * Initialize the libtorrent core.
     */
    private native void start(final boolean isPro);
    
    /**
     * Shuts down the libtorrent core.
     */
    public native void stop();
    
    private native long add_torrent(String incompletePath, String torrentPath, 
        int size, boolean sequential, int torrentState);

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

    private void setPayloadUploadRate(float payloadUploadRate)
        {
        m_payloadUploadRate = payloadUploadRate;
        }

    public float getPayloadUploadRate()
        {
        return m_payloadUploadRate;
        }

    private void setPayloadDownloadRate(float payloadDownloadRate)
        {
        m_payloadDownloadRate = payloadDownloadRate;
        }

    public float getPayloadDownloadRate()
        {
        return m_payloadDownloadRate;
        }

    private void setTotalPayloadUploadBytes(long totalPayloadUploadBytes)
        {
        m_totalPayloadUploadBytes = totalPayloadUploadBytes;
        }

    public long getTotalPayloadUploadBytes()
        {
        return m_totalPayloadUploadBytes;
        }

    private void setTotalPayloadDownloadBytes(final long totalPayloadDownloadBytes)
        {
        m_totalPayloadDownloadBytes = totalPayloadDownloadBytes;
        }

    public long getTotalPayloadDownloadBytes()
        {
        return m_totalPayloadDownloadBytes;
        }

    }
