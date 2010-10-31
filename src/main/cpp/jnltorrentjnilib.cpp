/*
 *  jnltorrentjnilib.cpp
 *  jnltorrent
 *
 *  Created by Julian Cain on 12/6/08.
 *  Copyright (c) 2008, __MyCompanyName__. All rights reserved.
 *
 */

#include <iostream>
#include <time.h>

#include <boost/cstdint.hpp>
#include <boost/current_function.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <boost/pool/detail/singleton.hpp>
#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/progress.hpp>
#include <boost/function.hpp>

#include <libtorrent/session.hpp>
#include <libtorrent/upnp.hpp>
#include <libtorrent/natpmp.hpp>
#include <libtorrent/alert.hpp>
#include <libtorrent/alert_types.hpp>
#include <libtorrent/session_settings.hpp>
#include <libtorrent/extensions/metadata_transfer.hpp>
#include <libtorrent/extensions/ut_metadata.hpp>
#include <libtorrent/extensions/ut_pex.hpp>
#include <libtorrent/extensions/smart_ban.hpp>
#include <libtorrent/bencode.hpp>

#include "org_lastbamboo_jni_JLibTorrent.h"

using namespace std;

#define LS_TRY_BEGIN try {

#define LS_TRY_END \
} catch (std::exception &e) \
{ \
    const std::string msg = std::string(BOOST_CURRENT_FUNCTION) + "Caught exception: " + std::string(e.what()); \
    log_error(env, obj, msg.c_str()); \
} catch (...) \
{    \
} 

#define SAVE_SESSION_STATE 1

#ifndef NDEBUG
#define log_debug(s)                       \
    do {                                                        \
        if (1) \
        {                      \
            std::ostringstream oss; \
            oss << s; \
            std::cout << oss.str() << std::endl;                \
        } \
    } while (0)
#else
    #define log_debug(s) /* */
#endif

#define jlong_to_ptr(a) ((void *)(uintptr_t)(a))
#define ptr_to_jlong(a) ((jlong)(uintptr_t)(a))


typedef std::map<
    const std::string, const libtorrent::torrent_handle
> TorrentPathToDownloadHandle;

typedef std::map<
    const libtorrent::sha1_hash, unsigned int
> InfoHashToIndexMap;

/**
 * This implements a libtorrent session interface via a thread safe, 
 * non-copyable singleton instance.
 */
class session : private boost::noncopyable
{
    public:

        static session & instance() 
        {
			return boost::details::pool::singleton_default<session>::instance();
        }

        void start(bool isPro)
        {
            std::cout << "Starting LittleShoot 1.0 session..." << std::endl;
            m_is_pro = isPro;
            const boost::uint8_t version_major = 0;
            const boost::uint8_t version_minor = 1;
            const boost::uint8_t version_micro = 1;
        
			const boost::uint16_t port = std::rand() % (65535 - 16383);

			m_session.reset(new libtorrent::session(
                libtorrent::fingerprint(
                "LS", version_major, version_minor, version_micro / 10, 
                version_micro % 10), std::make_pair(port, port + 100)
                )
            );

			libtorrent::session_settings settings;

			settings.user_agent = "LittleShoot/1.0 LibTorrent/"
                LIBTORRENT_VERSION;
			settings.stop_tracker_timeout = 5;
            settings.ignore_limits_on_local_network = true;
            settings.share_ratio_limit = 2.0;
            
            m_session->set_alert_mask(
                libtorrent::alert::port_mapping_notification | 
                libtorrent::alert::storage_notification | 
                libtorrent::alert::status_notification | 
                libtorrent::alert::tracker_notification
            );
            
            m_upnp.reset(m_session->start_upnp());

            m_natpmp.reset(m_session->start_natpmp());

            m_session->add_extension(&libtorrent::create_metadata_plugin);
            m_session->add_extension(&libtorrent::create_ut_pex_plugin);
            m_session->add_extension(&libtorrent::create_ut_metadata_plugin);
            m_session->add_extension(&libtorrent::create_smart_ban_plugin);

            m_session->set_max_uploads(4);
            m_session->set_max_half_open_connections(8);
            m_session->set_download_rate_limit(-1);
            
            // Common 768k dsl - factor (8 active s, 5 active s).
            m_session->set_upload_rate_limit(1024 * 100);
            m_session->set_settings(settings);
            
            m_upload_rate_limit = 1024 * 100;
        }
        
        void stop()
        {
            m_session.reset();
        }
	
		const libtorrent::torrent_handle download_torrent(
            const char * incompleteDir, 
            const char * torrentPath, 
            std::size_t size,
            bool sequential,
            int torrent_state
            ) 
		{
			cout << "Building buffer of size: " << size << endl;
			vector<char> buf(size);
			
			boost::filesystem::path full_path = 
                boost::filesystem::system_complete(
                    boost::filesystem::path(torrentPath, boost::filesystem::native));
                
            if (!boost::filesystem::exists(full_path))
            {
                log_debug("Data dir not found, creating: "
                    << full_path.native_file_string()
                );
                              
                //boost::filesystem::create_directory(full_path);
            }
            
			boost::filesystem::ifstream(torrentPath, ios_base::binary).read(&buf[0], size);
            
            //libtorrent::entry e;
            libtorrent::lazy_entry e;
            try 
            {
                //e = libtorrent::bdecode(
                //    buf.begin(), buf.end()
                //);
                int ret = lazy_bdecode(&buf[0], &buf[0] + buf.size(), e);

                if (ret != 0)
                {
                    std::cerr << "invalid bencoding: " << ret << std::endl;
                    return libtorrent::torrent_handle();
                }
            }
            catch (std::exception & e)
            {
                std::cout << 
                    BOOST_CURRENT_FUNCTION << ": caught(" << 
                    e.what() << ")returning invalid handle." << 
                std::endl;
                
                return libtorrent::torrent_handle();
            }
            
            log_debug("Using incomplete dir" << incompleteDir);
			libtorrent::add_torrent_params p;
            
            boost::filesystem::path system_incomplete_path(incompleteDir);
             
            bool is_resume = false;
            if (!boost::filesystem::exists(system_incomplete_path))
            {
                log_debug("Incomplete path not found.");
            }
            else
            {
                /*
                cout << "Found incomplete path" << endl;
                //std::string filename = (system_incomplete_path / ("resume.fastresume")).string();
                const boost::filesystem::path resume_file_path(system_incomplete_path / "resume.fastresume");
                
                std::vector<char> resume_buf;
                if (libtorrent::load_file(resume_file_path, resume_buf) == 0)
                {
                    //cout << "Setting resume data using path" << endl;
                    //cout << "Not really setting resume data..." << endl;
                    //p.resume_data = &resume_buf;
                    //is_resume = true;
                }
                 */
            }
            p.ti = new libtorrent::torrent_info(e);
            p.save_path = incompleteDir;
            if (!is_resume)
            {
                p.auto_managed = true;
                
                // Compact mode for single file torrents.
                p.storage_mode = 
                    sequential ? 
                    libtorrent::storage_mode_compact : 
                    libtorrent::storage_mode_sparse
                    ;
                
                // The LibTorrent docs say torrents should be added in the
                // paused state, but we've seen them just stay paused 
                // forever in that case.
                p.paused = false;
                p.duplicate_is_error = false;
            }
            
            //cout << "About to add torrent..." << endl;
            libtorrent::torrent_handle handle = m_session->add_torrent(p);
            log_debug("Added torrent...");

            if (!is_resume)
            {
                //cout << "Configuring fresh torrent" << endl;
                // Sequential mode for single file torrents.
                handle.set_sequential_download(sequential);
                if (m_is_pro)
                {
                    log_debug("Setting max connections to 60 for pro");
                    handle.set_max_connections(60);
                }
                else
                {
                    log_debug("Setting max connections for free");
                    handle.set_max_connections(50);
                }
                handle.set_max_uploads(-1);
                //handle.set_upload_limit(1024 * 32);
                handle.set_download_limit(-1);
            }
            
            log_debug("Adding torrent path to map: " << torrentPath);
            
            const string stringPath = torrentPath;
            m_torrent_path_to_handle.insert(
                TorrentPathToDownloadHandle::value_type(stringPath, handle));
            
            log_debug("Torrent name: " << handle.name());

            return handle;
		}
    
        void set_max_upload_speed(int speed)
        {
            log_debug("Setting max upload speed to: " << speed);
            
            // Interpret all negative speeds as unlimited.
            if (speed < 0)
            {
                speed = -1;
            }
            m_session->set_upload_rate_limit(speed);
        }
        
        void remove_torrent(const char * torrent_path)
        {
            if (torrent_path)
            {
                const libtorrent::torrent_handle th = 
                    session::instance().handle(torrent_path);
                
                if (th.is_valid())
                {
                    session::instance().get_session()->remove_torrent(th);
                    m_torrent_path_to_handle.erase(torrent_path);
                }
            }
        }
    
        void remove_torrent_and_files(const char * torrent_path)
        {
            if (torrent_path)
            {
                const libtorrent::torrent_handle th = 
                    session::instance().handle(torrent_path);
                
                if (th.is_valid())
                {
                    session::instance().get_session()->remove_torrent(th, 
                        libtorrent::session::delete_files);
                    m_torrent_path_to_handle.erase(torrent_path);
                }
            }
        }
	
        const libtorrent::torrent_handle handle(const char* torrentPath)
        {
            using namespace libtorrent;
            const string stringPath = torrentPath;
            const TorrentPathToDownloadHandle::iterator iter = 
                m_torrent_path_to_handle.find(stringPath);
            
            if (iter != m_torrent_path_to_handle.end())
            {
                const torrent_handle th = iter->second;
                
                try
                {
                    if (!th.has_metadata()) 
                    {
                        return torrent_handle();
                    }
                    
                    if (!th.is_valid()) 
                    {
                        return torrent_handle();
                    }
                    return th;
                }
                catch (std::exception & e)
                {
#ifndef NDEBUG
                    std::cerr << 
                        BOOST_CURRENT_FUNCTION << ": caught(" << 
                        e.what() << ")" << 
                    std::endl;
#endif
                }
            }

            return torrent_handle();
        }
	
		const long get_index_for_torrent(const char* torrentPath)
		{
			using namespace libtorrent;
            const torrent_handle th = handle(torrentPath);
            if (!th.has_metadata()) 
            {
                log_debug("No metadata for torrent");
                return -1;
            }
            if (!th.is_valid()) 
            {
                log_debug("Torrent not valid"); 
                return -1;
            }
            
            const torrent_status status = th.status();
            log_debug("Download rate: " << status.download_rate);
            const torrent_info ti = th.get_torrent_info();
            if (is_finished(status))
            {
                log_debug("File is finished!!!");
                return ti.total_size();
            }
            
            const sha1_hash sha1 = th.info_hash();
            
            unsigned int index = 0;
            const InfoHashToIndexMap::iterator iter = m_piece_to_index_map.find(sha1);
            if (iter != m_piece_to_index_map.end())
            {
                index = iter->second;
                log_debug("Found existing index: " << index);
                //return index;
            }
            else
            {
                log_debug("No existing torrent");
                m_piece_to_index_map.insert(InfoHashToIndexMap::value_type(sha1, 0));
                return -1;
            }
            const unsigned int numPieces = status.pieces.size();
            //cout << "Num pieces is: " << numPieces << endl;
            for (unsigned int j = index; j < numPieces; j++)
            {
                if (status.pieces[j])
                {
                    log_debug("Found piece at index: " << j);
                    // We have this piece -- stream it.
                } else
                {
                    // We do not have this piece -- set the index and
                    // break.
                    log_debug("Setting index to: " << j);
                    m_piece_to_index_map[sha1] = j;
                    
                    log_debug("index: " << j);
                    //log_debug("piece length is: " << ti.piece_length());
                    //log_debug("num pieces is: " << ti.num_pieces());
                    
                    const unsigned long maxByte = j * ti.piece_length();
                    
                    log_debug("max byte is: " << maxByte);

                    return maxByte;
                }
            }
            return index * ti.piece_length();
		}
	
        const bool is_finished(const libtorrent::torrent_status& status)
        {
            using namespace libtorrent;
            const torrent_status::state_t s = status.state;
           // cout << "Found state: " << s << endl;
            if (s == torrent_status::finished)
            {
               // cout << "Got finished state!!" << endl;
                return true;
            } else if (s == torrent_status::seeding)
            {
               // cout << "Got seeding state!!" << endl;
                return true;
            } else
            {
               // cout << "Got other state!!" << endl;
                return false;
            }
            
        }
    
        string const get_name_for_torrent(const char* torrentPath) 
        {
            using namespace libtorrent;
            const torrent_handle th = handle(torrentPath);
            if (!th.is_valid() || !th.has_metadata())
            {
                log_debug("Invalid torrent");
                return "";
            }
            const torrent_info ti = th.get_torrent_info();
            string name;
            if (ti.num_files() == 1)
            {
               // cout << "get_full_save_path_for_torrent::returning path for a single file..." << endl;
                const file_entry fe = ti.file_at(0);
                name = fe.path.file_string();
            } else
            {
                name = ti.name();
            }
            
            return name;
        }
    
        int get_state_for_torrent(const char* torrentPath) 
        {
            const libtorrent::torrent_handle th = handle(torrentPath);
            if (!th.is_valid() || !th.has_metadata())
            {
                // This will happen when the torrent is canceled as well as
                // failed for some reason.
                //cerr << "Torrent handle is not valid or has no metadata" << endl;
                return 201;
            }
            if (th.is_paused())
            {
                //cout << "Torrent handle is paused" << endl;
                return 200;
            } 
            else
            {
                const libtorrent::torrent_status stat = th.status();
                if (!stat.error.empty())
                {
                    //cerr << "Status has an error: " << stat.error << endl;
                    return 201;
                }
                else
                {
                    //cout << "Returning raw state..." << endl;
                    return stat.state;
                }
            }
        }
    
        void move_to_downloads_dir(
            const char* torrentPath, const char* downloadsDirString)
        {
            const libtorrent::torrent_handle th = handle(torrentPath);
            cout << "Moving to: " << downloadsDirString << endl;
            
            if (!th.is_valid())
            {
                log_debug("Invalid torrent");
                return;// boost::filesystem::path(".");
            }
            //const boost::filesystem::path savePath = th.save_path();
            //cout << "Save path is: " << savePath.string() << endl;
            //const boost::filesystem::path tempDir = savePath.parent_path();
            //const boost::filesystem::path sharedDir = tempDir.parent_path();
            //const boost::filesystem::path sharedDir = incompleteDir.parent_path();
            //const boost::filesystem::path downloadsDir(sharedDir / "downloads");
            boost::filesystem::path downloadsDir = 
                boost::filesystem::system_complete(
                    boost::filesystem::path(downloadsDirString, boost::filesystem::native));
            
            if (!boost::filesystem::exists(downloadsDir))
            {
                cerr << "Downloads dir does not exist at: " << downloadsDir << endl;
                return;
                //boost::filesystem::create_directory(full_path);
            }
            //const boost::filesystem::path downloadsDir(downloadsDirString);
            th.move_storage(downloadsDir);
            //return downloadsDir;
        }
    
    
        void rename(const char* torrentPath, const char* newName)
        {
            const libtorrent::torrent_handle th = handle(torrentPath);
            cout << "Renaming to: " << newName << endl;
            
            if (!th.is_valid())
            {
                log_debug("Invalid torrent");
                return;
            }
            th.rename_file(0, newName);
        }
            
        const libtorrent::torrent_status status(const char* torrentPath) 
        {
            using namespace libtorrent;
            const torrent_handle th = handle(torrentPath);
            if (th.is_valid() && th.has_metadata())
            {
                return th.status();
            } else
            {
                return torrent_status();
            }
        }
    
        const libtorrent::session_status session_status()
        {
            return m_session->status();
        }
    
        const boost::shared_ptr<libtorrent::session> & get_session()
        {
            return m_session;
        }
	
		const boost::shared_ptr<libtorrent::upnp> & get_upnp()
		{
			return m_upnp;
		}
	
		const boost::shared_ptr<libtorrent::natpmp> & get_natpmp()
		{
			return m_natpmp;
		}
        
	/*
        void load_state()
        {
#if defined(SAVE_SESSION_STATE) && (SAVE_SESSION_STATE)
            boost::filesystem::ifstream state_file(
                "/session_state.dat", std::ios_base::binary
            );
            state_file.unsetf(std::ios_base::skipws);
            m_session->load_state(libtorrent::bdecode(
                std::istream_iterator<char>(state_file), 
                std::istream_iterator<char>())
            );
#endif
        }
	 */
        
        void save_torrents()
        {
            int num_resume_data = 0;
            std::vector<libtorrent::torrent_handle> handles = m_session->get_torrents();
            m_session->pause();
            for (std::vector<libtorrent::torrent_handle>::iterator i = handles.begin();
                 i != handles.end(); ++i)
            {
                libtorrent::torrent_handle& h = *i;
                if (!h.has_metadata()) continue;
                if (!h.is_valid()) continue;
                
                h.save_resume_data();
                ++num_resume_data;
            }
            
            std::cout << "About to loop through alerts... " << std::endl;
            while (num_resume_data > 0)
            {
                cout << "Waiting for alert" << endl;
                libtorrent::alert const* a = 
                    m_session->wait_for_alert(libtorrent::seconds(10));
                cout << "Got alert" << endl;
                
                // if we don't get an alert within 10 seconds, abort
                if (a == 0) 
                {
                    cout << "Alert is null...aborting." << endl;
                    break;
                }
                
                std::auto_ptr<libtorrent::alert> holder = m_session->pop_alert();
                
                if (dynamic_cast<libtorrent::save_resume_data_failed_alert const*>(a))
                {
                    //process_alert(a);
                    cout << "Got failure...decrementing resumes..." << endl;
                    --num_resume_data;
                    continue;
                }
                
                libtorrent::save_resume_data_alert const* rd = 
                    dynamic_cast<libtorrent::save_resume_data_alert const*>(a);
                if (rd == 0)
                {
                    //process_alert(a);
                    cout << "Not a resume data alert...continuing" << endl;
                    continue;
                }
                
                libtorrent::torrent_handle h = rd->handle;
                boost::filesystem::ofstream out(h.save_path()
                                                / ("resume.fastresume"), std::ios_base::binary);
                out.unsetf(std::ios_base::skipws);
                bencode(std::ostream_iterator<char>(out), *rd->resume_data);
                
                cout << "Got a resume alert...decrementing count..." << endl;
                --num_resume_data;
            }
            m_session->resume();
        }
    
	/*
        void save_state(bool flag = false)
        {
            save_torrents();
#if defined(SAVE_SESSION_STATE) && (SAVE_SESSION_STATE)
            boost::filesystem::ofstream out(
                "/session_state.dat", std::ios_base::binary
            );
            out.unsetf(std::ios_base::skipws);
            libtorrent::bencode(
                std::ostream_iterator<char>(out), m_session->state()
            );
#endif
        }
	 */

    private:
    
        boost::shared_ptr<libtorrent::session> m_session;
	    boost::shared_ptr<libtorrent::upnp> m_upnp;
	    boost::shared_ptr<libtorrent::natpmp> m_natpmp;
        bool m_is_pro;
		InfoHashToIndexMap m_piece_to_index_map;
		TorrentPathToDownloadHandle m_torrent_path_to_handle;
        static const int PausedState = 200;
        int m_upload_rate_limit;
};

jmethodID m_sessionStatusTotalUpload;
jmethodID m_sessionStatusTotalDownload;
jmethodID m_sessionStatusTotalPayloadUpload;
jmethodID m_sessionStatusTotalPayloadDownload;
jmethodID m_sessionStatusUploadRate;
jmethodID m_sessionStatusDownloadRate;
jmethodID m_sessionStatusPayloadUploadRate;
jmethodID m_sessionStatusPayloadDownloadRate;
jmethodID m_sessionStatusNumPeers;
jmethodID m_portMapAlert;
jmethodID m_portMapLogAlert;
jmethodID m_log;
jmethodID m_logError;

void voidCall(const char* torrentPath, void (*pt2Func)(const char *)) {
    try { 
        pt2Func(torrentPath);
    }
    catch (exception & e) {
#ifndef NDEBUG
        cerr << BOOST_CURRENT_FUNCTION << ": caught(" << e.what() << ")" << endl;
#endif
    }
}

void voidCall(JNIEnv * env, const jstring& arg, void (*pt2Func)(const char*))
{
    const char * torrentPath  = env->GetStringUTFChars(arg, JNI_FALSE);
    if (!torrentPath)
    {
		cerr << "Out of memory!!" << endl;
		return; // OutOfMemoryError already thrown 
	}
    try
    { 
        pt2Func(torrentPath);
    }
    catch (exception & e)
    {
#ifndef NDEBUG
        cerr << BOOST_CURRENT_FUNCTION << ": caught(" << e.what() << ")" << endl;
#endif
    }
    env->ReleaseStringUTFChars(arg, torrentPath);
}

void voidCall(JNIEnv * env, const jstring& arg1, const jstring& arg2, void (*pt2Func)(const char*, const char*))
{
    const char * torrentPath  = env->GetStringUTFChars(arg1, JNI_FALSE);
    const char * str2  = env->GetStringUTFChars(arg2, JNI_FALSE);
    if (!torrentPath)
    {
		cerr << "Out of memory!!" << endl;
		return; // OutOfMemoryError already thrown 
	}
    
    if (!str2)
    {
		cerr << "Out of memory!!" << endl;
		return;
	}
    
    try
    { 
        pt2Func(torrentPath, str2);
    }
    catch (exception & e)
    {
#ifndef NDEBUG
        cerr << BOOST_CURRENT_FUNCTION << ": caught(" << e.what() << ")" << endl;
#endif
    }
    
    env->ReleaseStringUTFChars(arg1, torrentPath);
    env->ReleaseStringUTFChars(arg2, str2);
}

void voidCall(JNIEnv * env, const jboolean& arg, void (*pt2Func)(bool))
{
    try
    { 
        pt2Func(arg);
    }
    catch (exception & e)
    {
#ifndef NDEBUG
        cerr << BOOST_CURRENT_FUNCTION << ": caught(" << e.what() << ")" << endl;
#endif
    }
}

void voidCall(JNIEnv * env, const jint& arg, void (*pt2Func)(int))
{
    try
    { 
        pt2Func(arg);
    }
    catch (exception & e)
    {
#ifndef NDEBUG
        cerr << BOOST_CURRENT_FUNCTION << ": caught(" << e.what() << ")" << endl;
#endif
    }
}

boost::int64_t longCall(JNIEnv * env, const jstring& arg, boost::int64_t (*pt2Func)(const char*))
{
    const char * torrentPath  = env->GetStringUTFChars(arg, JNI_FALSE);
    if (!torrentPath)
    {
		cerr << "Out of memory!!" << endl;
		return -1; // OutOfMemoryError already thrown 
	}
    
    boost::int64_t toReturn = -1;
    
    try
    { 
        toReturn = pt2Func(torrentPath);
    }
    catch (exception & e)
    {
#ifndef NDEBUG
        cerr << BOOST_CURRENT_FUNCTION << ": caught(" << e.what() << ")" << endl;
#endif
    }
    
    env->ReleaseStringUTFChars(arg, torrentPath);
    return toReturn;
}

int intCall(JNIEnv * env, int (*pt2Func)())
{
    int toReturn = -1;
    try
    { 
        toReturn = pt2Func();
    }
    catch (exception & e)
    {
#ifndef NDEBUG
        cerr << BOOST_CURRENT_FUNCTION << ": caught(" << e.what() << ")" << endl;
#endif
    }
    return toReturn;
}

int intCall(JNIEnv * env, const jstring& arg, int (*pt2Func)(const char*))
{
    const char * torrentPath  = env->GetStringUTFChars(arg, JNI_FALSE);
    if (!torrentPath)
    {
		cerr << "Out of memory!!" << endl;
		return -1; // OutOfMemoryError already thrown 
	}
    
    int toReturn = -1;
    try
    { 
        toReturn = pt2Func(torrentPath);
    }
    catch (exception & e)
    {
#ifndef NDEBUG
        cerr << BOOST_CURRENT_FUNCTION << ": caught(" << e.what() << ")" << endl;
#endif
    }
    
    env->ReleaseStringUTFChars(arg, torrentPath);
    return toReturn;
}

float floatCall(JNIEnv * env, const jstring& arg, float (*pt2Func)(const char*))
{
    const char * torrentPath  = env->GetStringUTFChars(arg, JNI_FALSE);
    if (!torrentPath)
    {
		cerr << "Out of memory!!" << endl;
		return -1; // OutOfMemoryError already thrown 
	}
    
    float toReturn = -1;
    try
    { 
        toReturn = pt2Func(torrentPath);
    }
    catch (exception & e)
    {
#ifndef NDEBUG
        cerr << BOOST_CURRENT_FUNCTION << ": caught(" << e.what() << ")" << endl;
#endif
    }
    
    env->ReleaseStringUTFChars(arg, torrentPath);
    return toReturn;
}

jstring const stringCall(JNIEnv * env, const jstring& arg, string const (*pt2Func)(const char*))
{
    const char * torrentPath  = env->GetStringUTFChars(arg, JNI_FALSE);
    if (!torrentPath)
    {
		cerr << "Out of memory!!" << endl;
		return env->NewStringUTF(""); // OutOfMemoryError already thrown 
	}
    
    string toReturn = "";
    try
    { 
        toReturn = pt2Func(torrentPath);
    }
    catch (exception & e)
    {
#ifndef NDEBUG
        cerr << BOOST_CURRENT_FUNCTION << ": caught(" << e.what() << ")" << endl;
#endif
    }
    
    env->ReleaseStringUTFChars(arg, torrentPath);
    return env->NewStringUTF(toReturn.c_str());
}

// Java function to C interfaces.

JNIEXPORT void JNICALL Java_org_lastbamboo_jni_JLibTorrent_start(
    JNIEnv * env , jobject obj, jboolean isPro)
{
    std::cout << "jnltorrent start" << std::endl;
    session::instance().start(isPro);
}

JNIEXPORT void JNICALL Java_org_lastbamboo_jni_JLibTorrent_stop(JNIEnv * env , jobject obj)
{
    std::cout << "jnltorrent stop" << std::endl;
    session::instance().stop();
}

JNIEXPORT jlong JNICALL Java_org_lastbamboo_jni_JLibTorrent_add_1torrent(
    JNIEnv * env, jobject obj, jstring jIncompleteDir, jstring arg, jint size,
    jboolean sequential, jint torrent_state
    )
{
    const char * incompleteDir = env->GetStringUTFChars(jIncompleteDir, JNI_FALSE);
    const char * torrentPath  = env->GetStringUTFChars(arg, JNI_FALSE);
    cout << "Downloading to dir:" << incompleteDir << endl;
	cout << "Got download call from Java for path:" << torrentPath << endl;
    if (!incompleteDir)
    {
		cerr << "Out of memory!!" << endl;
		return -1; // OutOfMemoryError already thrown
	}
    if (!torrentPath)
    {
		cerr << "Out of memory!!" << endl;
		return -1; // OutOfMemoryError already thrown 
	}
    
    try
    {
        const libtorrent::torrent_handle handle = 
            session::instance().download_torrent(incompleteDir, torrentPath, size, sequential, torrent_state);  
    }
    catch (exception & e)
    {
#ifndef NDEBUG
        cerr << BOOST_CURRENT_FUNCTION << ": caught(" << e.what() << ")" << endl;
#endif
    }
    cout << "Finished download_torrent call" << endl;
	env->ReleaseStringUTFChars(arg, torrentPath);
	env->ReleaseStringUTFChars(arg, incompleteDir);
	return 0;
}

boost::int64_t indexFunc(const char* torrentPath)
{    
    return session::instance().get_index_for_torrent(torrentPath);
}
    
JNIEXPORT jlong JNICALL Java_org_lastbamboo_jni_JLibTorrent_get_1max_1byte_1for_1torrent(
	JNIEnv * env, jobject obj, jstring arg
) {return longCall(env, arg, &indexFunc);}
 

string const nameFunc(const char* torrentPath)
{
    return session::instance().get_name_for_torrent(torrentPath);
}
JNIEXPORT jstring JNICALL Java_org_lastbamboo_jni_JLibTorrent_get_1name_1for_1torrent(
    JNIEnv * env, jobject obj, jstring arg
){return stringCall(env, arg, &nameFunc);}


boost::int64_t sizeFunc(const char* torrentPath)
{    
    return session::instance().status(torrentPath).total_wanted;
}
JNIEXPORT jlong JNICALL Java_org_lastbamboo_jni_JLibTorrent_get_1size_1for_1torrent(
    JNIEnv * env, jobject obj, jstring arg
) {return longCall(env, arg, &sizeFunc);}


void removeTorrentFunc(const char * torrentPath)
{    
    if (torrentPath)
    {
        session::instance().remove_torrent(torrentPath);
    }
}    
JNIEXPORT void JNICALL Java_org_lastbamboo_jni_JLibTorrent_remove_1torrent(
    JNIEnv * env, jobject obj, jstring arg
) {return voidCall(env, arg, &removeTorrentFunc);}

void removeTorrentAndFilesFunc(const char * torrentPath)
{    
    if (torrentPath)
    {
        session::instance().remove_torrent_and_files(torrentPath);
    }
} 
JNIEXPORT void JNICALL Java_org_lastbamboo_jni_JLibTorrent_remove_1torrent_1and_1files(
    JNIEnv * env, jobject obj, jstring arg
) {return voidCall(env, arg, &removeTorrentAndFilesFunc);}                        

int stateFunc(const char* torrentPath)
{    
    return session::instance().get_state_for_torrent(torrentPath); 
}
JNIEXPORT jint JNICALL Java_org_lastbamboo_jni_JLibTorrent_get_1state_1for_1torrent(
	JNIEnv * env, jobject obj, jstring arg
) {return intCall(env, arg, &stateFunc);}


int numFilesFunc(const char* torrentPath)
{
    const libtorrent::torrent_handle th = 
        session::instance().handle(torrentPath);
    
    if (th.is_valid() && th.has_metadata())
    {
        return th.get_torrent_info().num_files();
    } else
    { 
        return 0;
    }
}
JNIEXPORT jint JNICALL Java_org_lastbamboo_jni_JLibTorrent_get_1num_1files_1for_1torrent(
    JNIEnv * env, jobject obj, jstring arg
) {return intCall(env, arg, &numFilesFunc);}


boost::int64_t bytesReadFunc(const char* torrentPath)
{    
    return session::instance().status(torrentPath).total_wanted_done; 
}
JNIEXPORT jlong JNICALL Java_org_lastbamboo_jni_JLibTorrent_get_1bytes_1read_1for_1torrent(
    JNIEnv * env, jobject obj, jstring arg
) {return longCall(env, arg, &bytesReadFunc);}


int numPeersFunc(const char* torrentPath)
{    
    return session::instance().status(torrentPath).num_peers;
}
JNIEXPORT jint JNICALL Java_org_lastbamboo_jni_JLibTorrent_get_1num_1peers_1for_1torrent(
    JNIEnv * env, jobject obj, jstring arg
) {return intCall(env, arg, &numPeersFunc);}


float speedFunc(const char* torrentPath)
{    
    return session::instance().status(torrentPath).download_payload_rate;
}
JNIEXPORT jdouble JNICALL Java_org_lastbamboo_jni_JLibTorrent_get_1speed_1for_1torrent(
    JNIEnv * env, jobject obj, jstring arg
) {return floatCall(env, arg, &speedFunc);}


void moveToDownloadsDirFunc(const char* torrentPath, const char* downloadsDir)
{
    session::instance().move_to_downloads_dir(torrentPath, downloadsDir);
    //session::instance().save_torrents();
    //return newPath.string();
}
JNIEXPORT void JNICALL Java_org_lastbamboo_jni_JLibTorrent_move_1to_1downloads_1dir(
    JNIEnv * env, jobject obj, jstring arg, jstring downloadsDirString
){return voidCall(env, arg, downloadsDirString, &moveToDownloadsDirFunc);}

void renameFunc(const char* torrentPath, const char* newName)
{
    session::instance().rename(torrentPath, newName);
}
JNIEXPORT void JNICALL Java_org_lastbamboo_jni_JLibTorrent_rename(
    JNIEnv * env, jobject obj, jstring torrentPath, jstring newName)
{return voidCall(env, torrentPath, newName, &renameFunc);}

    

void pause(const char* torrentPath)
{
    const libtorrent::torrent_handle th = session::instance().handle(torrentPath);
    if (th.is_valid())
    {
        th.auto_managed(false);
        th.pause();
        //session::instance().save_torrents();
    }
    
}
JNIEXPORT void JNICALL Java_org_lastbamboo_jni_JLibTorrent_pause_1torrent(
    JNIEnv * env, jobject obj, jstring arg
){return voidCall(env, arg, &pause);}


void resume(const char* torrentPath)
{
    const libtorrent::torrent_handle th = session::instance().handle(torrentPath);
    if (th.is_valid())
    {
        th.auto_managed(true);
        cout << "Resumed...set auto_managed to true" << endl;
    }
    //session::instance().save_torrents();
}
JNIEXPORT void JNICALL Java_org_lastbamboo_jni_JLibTorrent_resume_1torrent(
    JNIEnv * env, jobject obj, jstring arg
){return voidCall(env, arg, &resume);}

void hard_resume(const char* torrentPath)
{
    const libtorrent::torrent_handle th = session::instance().handle(torrentPath);
    if (th.is_valid())
    {
        th.auto_managed(true);
        th.resume();
        cout << "Hard resumed...set auto_managed to true" << endl;
    }
}
JNIEXPORT void JNICALL Java_org_lastbamboo_jni_JLibTorrent_hard_1resume_1torrent(
    JNIEnv * env, jobject obj, jstring arg
) {return voidCall(env, arg, &hard_resume);}
            

void set_max_upload_speed(int speed)
{
    session::instance().set_max_upload_speed(speed);    
}
JNIEXPORT void JNICALL Java_org_lastbamboo_jni_JLibTorrent_set_1max_1upload_1speed(
    JNIEnv * env, jobject obj, jint arg
){return voidCall(env, arg, &set_max_upload_speed);}


int map_upnp_port(libtorrent::upnp::protocol_type p, int localPort, int externalPort)
{
    try
    { 
		if (session::instance().get_upnp().get())
		{
			return session::instance().get_upnp()->add_mapping(libtorrent::upnp::tcp, localPort, externalPort);
		}
	}
	catch (exception & e)
	{
#ifndef NDEBUG
		cerr << BOOST_CURRENT_FUNCTION << ": caught(" << e.what() << ")" << endl;
#endif
	}
	return -1; 												
}

int map_natpmp_port(libtorrent::natpmp::protocol_type p, int localPort, int externalPort)
{
    try
    { 
		if (session::instance().get_natpmp().get())
		{
			return session::instance().get_natpmp()->add_mapping(p, localPort, externalPort);
		}
    }
    catch (exception & e)
    {
#ifndef NDEBUG
        cerr << BOOST_CURRENT_FUNCTION << ": caught(" << e.what() << ")" << endl;
#endif
    }
	return -1; 												
}
												   
JNIEXPORT jint JNICALL Java_org_lastbamboo_jni_JLibTorrent_add_1tcp_1upnp_1mapping
(JNIEnv * env, jobject obj, jint internalPort, jint externalPort)
{return map_upnp_port(libtorrent::upnp::tcp, internalPort, externalPort);}


JNIEXPORT jint JNICALL Java_org_lastbamboo_jni_JLibTorrent_add_1udp_1upnp_1mapping
(JNIEnv * env, jobject obj, jint internalPort, jint externalPort)
{return map_upnp_port(libtorrent::upnp::udp, internalPort, externalPort);}

JNIEXPORT jint JNICALL Java_org_lastbamboo_jni_JLibTorrent_add_1tcp_1natpmp_1mapping
(JNIEnv * env, jobject obj, jint internalPort, jint externalPort) 
{return map_natpmp_port(libtorrent::natpmp::tcp, internalPort, externalPort);}

JNIEXPORT jint JNICALL Java_org_lastbamboo_jni_JLibTorrent_add_1udp_1natpmp_1mapping
(JNIEnv * env, jobject obj, jint internalPort, jint externalPort)
{return map_natpmp_port(libtorrent::natpmp::udp, internalPort, externalPort);}

void log_error(JNIEnv * env, jobject obj, const char* error) {
	jstring msg = env->NewStringUTF(error);
	env->CallVoidMethod(obj, m_logError, msg);
	env->DeleteLocalRef(msg);
}

void log(JNIEnv * env, jobject obj, const char* info) {
	jstring msg = env->NewStringUTF(info);
	env->CallVoidMethod(obj, m_log, msg);
	env->DeleteLocalRef(msg);
}

void checkMethodId(const jmethodID field)
{
    if (field == NULL) 
    {
        cerr << "Missing method ID" << endl;
        return; // method not found 
    }
}

const jmethodID floatMethodId(JNIEnv * env, const jclass cls, const char * methodName)
{
    const jmethodID id = env->GetMethodID(cls, methodName, "(F)V");
    checkMethodId(id);
    return id;
}

const jmethodID longMethodId(JNIEnv * env, const jclass cls, const char * methodName)
{
    const jmethodID id = env->GetMethodID(cls, methodName, "(J)V");
    checkMethodId(id);
    return id;
}

const jmethodID intMethodId(JNIEnv * env, const jclass cls, const char * methodName)
{
    const jmethodID id = env->GetMethodID(cls, methodName, "(I)V");
    checkMethodId(id);
    return id;
}

const jmethodID intIntMethodId(JNIEnv * env, const jclass cls, const char * methodName)
{
    const jmethodID id = env->GetMethodID(cls, methodName, "(II)V");
    checkMethodId(id);
    return id;
}

const jmethodID methodId(JNIEnv * env, const jclass cls, const char * methodName, const char * signature)
{
    const jmethodID id = env->GetMethodID(cls, methodName, signature);
    checkMethodId(id);
    return id;
}


JNIEXPORT void JNICALL Java_org_lastbamboo_jni_JLibTorrent_cacheMethodIds
(JNIEnv * env, jobject obj)
{
    cout << "Caching method IDs" << endl;
    const jclass cls = env->GetObjectClass(obj);
    m_sessionStatusTotalUpload = longMethodId(env, cls,"setTotalUploadBytes");
    m_sessionStatusTotalDownload = longMethodId(env, cls, "setTotalDownloadBytes");
    m_sessionStatusTotalPayloadUpload = longMethodId(env, cls,"setTotalPayloadUploadBytes");
    m_sessionStatusTotalPayloadDownload = longMethodId(env, cls, "setTotalPayloadDownloadBytes");
    m_sessionStatusUploadRate = floatMethodId(env, cls, "setUploadRate");
    m_sessionStatusDownloadRate = floatMethodId(env, cls, "setDownloadRate");
    m_sessionStatusPayloadUploadRate = floatMethodId(env, cls, "setPayloadUploadRate");
    m_sessionStatusPayloadDownloadRate = floatMethodId(env, cls, "setPayloadDownloadRate");
    m_sessionStatusNumPeers = intMethodId(env, cls, "setNumPeers");
	m_portMapAlert = methodId(env, cls, "portMapAlert", "(III)V");
	m_portMapLogAlert = methodId(env, cls, "portMapLogAlert", "(ILjava/lang/String;)V");
	m_log = methodId(env, cls, "log", "(Ljava/lang/String;)V");
	m_logError = methodId(env, cls, "log", "(Ljava/lang/String;)V");
}

JNIEXPORT void JNICALL Java_org_lastbamboo_jni_JLibTorrent_update_1session_1status
(JNIEnv * env, jobject obj)
{
    const libtorrent::session_status stat = session::instance().session_status();
    env->CallVoidMethod(obj, m_sessionStatusTotalUpload, stat.total_upload);
    env->CallVoidMethod(obj, m_sessionStatusTotalDownload, stat.total_download);
    env->CallVoidMethod(obj, m_sessionStatusTotalPayloadUpload, stat.total_payload_upload);
    env->CallVoidMethod(obj, m_sessionStatusTotalPayloadDownload, stat.total_payload_download);
    env->CallVoidMethod(obj, m_sessionStatusUploadRate, stat.upload_rate);
    env->CallVoidMethod(obj, m_sessionStatusDownloadRate, stat.download_rate);
    env->CallVoidMethod(obj, m_sessionStatusPayloadUploadRate, stat.payload_upload_rate);
    env->CallVoidMethod(obj, m_sessionStatusPayloadDownloadRate, stat.payload_download_rate);
    env->CallVoidMethod(obj, m_sessionStatusNumPeers, stat.num_peers);
}

JNIEXPORT void JNICALL Java_org_lastbamboo_jni_JLibTorrent_check_1alerts
(JNIEnv * env, jobject obj) { 
	LS_TRY_BEGIN;

	const boost::shared_ptr<libtorrent::session> ses = session::instance().get_session();
	
	if (!ses.get()) {
		return;
	}
	
	std::auto_ptr<libtorrent::alert> a = ses.get()->pop_alert();
	
	while (a.get()) {
		if (libtorrent::tracker_announce_alert * p = dynamic_cast<
			libtorrent::tracker_announce_alert *>(a.get())) {
			std::cout << "Tracker announce:" << a->message() << std::endl;
			if (p->handle.is_valid()) {
				std::cout << "Tracker_announce_alert: " << p->message() << std::endl;
			}
		}
		
		else if (libtorrent::tracker_reply_alert * p = dynamic_cast<
				 libtorrent::tracker_reply_alert *>(a.get())) {
			if (p->handle.is_valid()){
				std::cout << ": tracker_reply_alert: " << p->message() << std::endl;
			}
		}
		else if (libtorrent::portmap_alert * p = dynamic_cast<
				 libtorrent::portmap_alert *>(a.get())){
			std::cout << "PORT_MAP_ALERT: " << p->message() << std::endl;
			env->CallVoidMethod(obj, m_portMapAlert, p->mapping, p->external_port, p->type);
		}
		else if (libtorrent::portmap_log_alert * p = dynamic_cast<
				 libtorrent::portmap_log_alert *>(a.get())){
			std::cout << "PORT_MAP_LOG_ALERT: " << p->message() << std::endl;
			jstring msg = env->NewStringUTF(p->msg.c_str());
			env->CallVoidMethod(obj, m_portMapLogAlert, p->type, msg);
			env->DeleteLocalRef(msg);
		}
		
		else if (libtorrent::portmap_error_alert * p = dynamic_cast<
				 libtorrent::portmap_error_alert *>(a.get())) {
			std::cout << "PORT_MAP_ERROR_ALERT: " << p->message() << std::endl;
			const std::string msg = "PORT_MAP_ERROR_ALERT: " + std::string(p->message()); 
			log(env, obj, msg.c_str());
		}
		else if (libtorrent::save_resume_data_failed_alert * p = 
				 dynamic_cast<libtorrent::save_resume_data_failed_alert *>(a.get())) {
			std::cout << "Save_resume_data_failed_alert: " << p->message() << std::endl;
		}
		else if (libtorrent::save_resume_data_alert * p = dynamic_cast<
				 libtorrent::save_resume_data_alert * >(a.get())) {
			std::cout << "Save_resume_data_alert:" << p->message() << std::endl;
			/*
			libtorrent::torrent_handle h = p->handle;
			boost::filesystem::ofstream out(h.save_path() / (h.get_torrent_info().name() + ".fastresume"), std::ios_base::binary);
			out.unsetf(std::ios_base::skipws);
			libtorrent::bencode(std::ostream_iterator<char>(out), *p->resume_data);
			 */
		}
		else {
			std::cout << "ALERT WE DON'T HANDLE..." << endl;
			std::cout << BOOST_CURRENT_FUNCTION << ": Alert(" << a->message() << ")." << std::endl;
		}
		a = ses->pop_alert();
	}
	LS_TRY_END;
	/*
	}
    catch (exception & e) {
		//const char* msg = 
		const std::string msg = "Exception processing alerts: " + std::string(e.what());
		log_error(env, obj, msg.c_str());
#ifndef NDEBUG
        cerr << BOOST_CURRENT_FUNCTION << ": caught(" << e.what() << ")" << endl;
#endif
    }
	 */
}


