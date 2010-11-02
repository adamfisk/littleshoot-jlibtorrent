package org.lastbamboo.jni;

import java.io.File;
import java.io.IOException;
import java.util.Collection;
import java.util.Collections;
import java.util.LinkedHashMap;
import java.util.Map;

import org.lastbamboo.common.util.CommonUtils;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;


/**
 * Java wrapper class for calls to native lib torrent.
 */
public class JLibTorrent
    {

    private final Logger m_log = LoggerFactory.getLogger(getClass());
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
    private final File m_dataDir = CommonUtils.getLittleShootDir();

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
        start(this.m_isPro, normalizePath(this.m_dataDir));
        checkAlerts();
        }
    
    /**
     * Shuts down the libtorrent core.
     */
    public void stopLibTorrent() 
        {
        m_stopped = true;
        stop();
        }
    
    public void updateSessionStatus() 
        {
        if (this.m_stopped) return;
        update_session_status();
        }
    
    public void download(final File incompleteDir, final File torrentFile, 
        final boolean sequential, final int torrentState) throws IOException 
        {
        add_torrent(incompleteDir.getCanonicalPath(), 
            torrentFile.getCanonicalPath(), (int) torrentFile.length(), 
            sequential, torrentState);
        }

    public void rename(final File torrentFile, final String newName)
        {
        final String path = normalizePath(torrentFile);
        rename(path, newName);
        }
    
    public void moveToDownloadsDir(final File torrentFile, 
        final File downloadsDir)
        {
        final String path = normalizePath(torrentFile);
        final String downloadsDirPath = normalizePath(downloadsDir);
        move_to_downloads_dir(path, downloadsDirPath);
        }

    public long getMaxByteForTorrent(final File torrentFile)
        {
        if (this.m_stopped) return -1L;
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
        m_log.info("Performing hard resume");
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
    
    public void removeTorrentAndFiles(final File torrentFile)
        {
        final String path = normalizePath(torrentFile);
        remove_torrent_and_files(path);
        }
    
    public int getStateForTorrent(final File torrentFile)
        {
        if (this.m_stopped) return -1;
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
    
    private final Map<Integer, PortMappingListener> m_mappingIdsToListeners =
        Collections.synchronizedMap(new LinkedHashMap<Integer, PortMappingListener>()
            {
            private static final long serialVersionUID = 748372975L;
            @Override
            protected boolean removeEldestEntry(
                final Map.Entry<Integer, PortMappingListener> eldest)
                {
                // This makes the map automatically purge the least used
                // entry.  
                final boolean remove = size() > 200;
                return remove;
                }
            });
    
    private volatile boolean m_stopped;

        
    public void addTcpUpnpPortMapping(final PortMappingListener listener,
        final int internalPort, final int externalPort)
        {
        final int mappingId = add_tcp_upnp_mapping(internalPort, externalPort);
        m_mappingIdsToListeners.put(mappingId, listener);
        }
        
    public void addUdpUpnpPortMapping(final PortMappingListener listener,
        final int internalPort, final int externalPort)
        {
        final int mappingId = add_udp_upnp_mapping(internalPort, externalPort);
        m_mappingIdsToListeners.put(mappingId, listener);
        }
        
    public void addTcpNapPmpPortMapping(final PortMappingListener listener,
        final int internalPort, final int externalPort)
        {
        final int mappingId = 
            add_tcp_natpmp_mapping(internalPort, externalPort);
        m_mappingIdsToListeners.put(mappingId, listener);
        }
        
    public void addUdpNatPmpPortMapping(final PortMappingListener listener,
        final int internalPort, final int externalPort)
        {
        final int mappingId = 
            add_udp_natpmp_mapping(internalPort, externalPort);
        m_mappingIdsToListeners.put(mappingId, listener);
        }
    
    
    private native int add_tcp_upnp_mapping(final int internalPort, final int externalPort);
    private native int add_udp_upnp_mapping(final int internalPort, final int externalPort);
    private native int add_tcp_natpmp_mapping(final int internalPort, final int externalPort);
    private native int add_udp_natpmp_mapping(final int internalPort, final int externalPort);
    
    public void checkAlerts() 
        {
        final Runnable runner = new Runnable()
            {

            public void run() {
                while (!m_stopped) {
                    // We sleep first so we don't immediately start checking
                    // before LibTorrent's initialized.
                    try {
                        Thread.sleep(2000);
                    } catch (final InterruptedException e) {
                        m_log.error("Interrupted?", e);
                    }
                    check_alerts();
                }
            }
            };
            final Thread t = new Thread(runner);
            t.setDaemon(true);
            t.start();
        }
    
    private native void check_alerts();
    
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
    
    private native void move_to_downloads_dir(final String path, 
        final String downloadsDir);
    
    private native void rename(final String path, final String newName);
    
    private native long get_bytes_read_for_torrent(final String path);
    
    private native int get_num_peers_for_torrent(final String path);
    
    private native double get_speed_for_torrent(final String path);

    private native int get_num_files_for_torrent(final String path);
    
    private native int get_state_for_torrent(final String path);
    
    private native long get_size_for_torrent(final String path);

    private native String get_name_for_torrent(final String path);
    
    private native void remove_torrent(final String path);
    
    private native void remove_torrent_and_files(final String path);
    
    private native void pause_torrent(final String path);
    
    private native void resume_torrent(final String path);
    
    private native void hard_resume_torrent(final String path);
    
    private native long get_max_byte_for_torrent(final String path);
    
    /**
     * Initialize the libtorrent core.
     * 
     * @param dataDir The path to the data directory for storing certain kinds
     * of files.
     */
    private native void start(final boolean isPro, final String dataDir);
    
    private native void stop();
    
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
    
    public void portMapAlert(final int mappingId, final int externalPort, 
        final int type) 
        {
        m_log.info("GOT PORT MAPPED!! ID: "+mappingId+
            " EXTERNAL PORT: "+externalPort);
        
        final PortMappingListener listener = 
            this.m_mappingIdsToListeners.get(mappingId);
        
        if (listener == null)
            {
            m_log.error("No listener for ID!! "+mappingId);
            return;
            }
        listener.externalPortMapped(externalPort);
        }
    
    public void portMapLogAlert(final int type, final String message) 
        {
        m_log.info("Port map log for type {}: "+message, type);
        }

    public void log(final String msg) 
        {
        m_log.info("From native code: {}", msg);
        }
    
    public void logError(final String msg) 
        {
        m_log.error("From native code: {}", msg);
        }
    }
