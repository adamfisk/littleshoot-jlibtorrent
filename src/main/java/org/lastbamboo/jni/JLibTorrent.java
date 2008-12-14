package org.lastbamboo.jni;

import java.io.File;
import java.io.IOException;

import org.apache.commons.lang.SystemUtils;


public class JLibTorrent
    {
    
    public JLibTorrent()
        {
        System.out.println("Instance created");
        
        final String libName = System.mapLibraryName("jnltorrent");
        final File lib1 = new File (libName);
        final File lsDir = new File(SystemUtils.USER_HOME, ".littleshoot");
        final File lib2 = new File (lsDir, libName);
        final File lib3 = new File (new File("../../lib"), libName);
        
        if (lib1.isFile())
            {
            System.load(lib1.getAbsolutePath());
            }
        else if (lib2.isFile())
            {
            System.load(lib2.getAbsolutePath());
            }
        else if (lib3.isFile())
            {
            System.load(lib3.getAbsolutePath());
            }
        else
            {
            System.loadLibrary("jnltorrent");
            }
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
    
    public void udpateFileData()
        {
        processEvents();
        }
    
    public String getSavePath(File torrentFile)
        {
        try
            {
            final String path = torrentFile.getCanonicalPath();
            return get_save_path_for_torrent(path);
            }
        catch (IOException e)
            {
            // TODO Auto-generated catch block
            e.printStackTrace();
            }
        return "";
        }
    
    private native String get_save_path_for_torrent(final String path);
    
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
