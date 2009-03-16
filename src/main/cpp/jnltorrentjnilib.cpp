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

#define ENABLE_MESSAGE_QUEUE 0

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
    
#if defined(ENABLE_MESSAGE_QUEUE) && (ENABLE_MESSAGE_QUEUE)
        session()
        {
            // ...
        }
#endif // ENABLE_MESSAGE_QUEUE

        static session & instance() 
        {
			return boost::details::pool::singleton_default<session>::instance();
        }

        void start()
        {
            std::cout << "Starting session..." << std::endl;
            
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

			settings.user_agent = "LittleShoot/0.84 libtorrent/"
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
            
            m_session->start_upnp();

            m_session->start_natpmp();

            m_session->add_extension(&libtorrent::create_metadata_plugin);
            m_session->add_extension(&libtorrent::create_ut_pex_plugin);
            m_session->add_extension(&libtorrent::create_ut_metadata_plugin);
            m_session->add_extension(&libtorrent::create_smart_ban_plugin);

            m_session->set_max_uploads(4);
            m_session->set_max_half_open_connections(8);
            m_session->set_download_rate_limit(-1);
            
            // Common 768k dsl - factor (8 active s, 5 active s).
            m_session->set_upload_rate_limit(1024 * 96);
            m_session->set_settings(settings);
            
#if defined(ENABLE_MESSAGE_QUEUE) && (ENABLE_MESSAGE_QUEUE)
            session::instance().tick();
#endif // ENABLE_MESSAGE_QUEUE
        }
        
        void stop()
        {
#if defined(ENABLE_MESSAGE_QUEUE) && (ENABLE_MESSAGE_QUEUE)
            session::instance().tick();
            save_state(true);
#endif // ENABLE_MESSAGE_QUEUE
            m_session.reset();
        }
	
		const libtorrent::torrent_handle download_torrent(
            const char * incompleteDir, 
            const char * torrentPath, 
            std::size_t size,
            bool sequential
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
            
            std::cout << "Using incomplete dir" << incompleteDir << endl;
			libtorrent::add_torrent_params p;
			p.save_path = incompleteDir;
            
            cout << "Save path for torrent is: " << incompleteDir << endl;
            cout << "Sequential: " << sequential << endl;
            
            p.ti = new libtorrent::torrent_info(e);
            p.auto_managed = true;
            
            // Compact mode for single file torrents.
            p.storage_mode = 
                sequential ? 
                libtorrent::storage_mode_compact : 
                libtorrent::storage_mode_sparse
            ;
            p.paused = false;
            p.duplicate_is_error = false;
            
            libtorrent::torrent_handle handle = m_session->add_torrent(p);
            
            // Sequential mode for single file torrents.
            
            handle.set_sequential_download(sequential);
            handle.set_max_connections(50);
            //handle.set_max_uploads(-1);
            handle.set_upload_limit(1024 * 32);
            handle.set_download_limit(-1);
            
            if (!handle.is_valid())
            {
                cerr << "Torrent handle not valid!!" << endl;
            }
            else
            {

            }
            
            cout << "Adding torrent path to map: " << torrentPath << endl;
            
            const string stringPath = torrentPath;
            m_torrent_path_to_handle.insert(
                TorrentPathToDownloadHandle::value_type(stringPath, handle));
            
            cout << "Torrent name: " << handle.name() << endl;

            return handle;
		}
        
        void remove_torrent(const char * torrent_path)
        {
            if (torrent_path)
            {
                const libtorrent::torrent_handle th = session::instance(
                    ).handle(torrent_path)
                ;
                
                if (th.is_valid())
                {
                    session::instance().get_session()->remove_torrent(th);
                
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
                    log_debug("piece length is: " << ti.piece_length());
                    log_debug("num pieces is: " << ti.num_pieces());
                    
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
                cerr << "Invalid torrent" << endl;
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
                return 201;
            }
            if (th.is_paused())
            {
                return 200;
            } 
            else
            {
                const libtorrent::torrent_status stat = th.status();
                if (!stat.error.empty())
                {
                    return 201;
                }
                else
                {
                    return stat.state;
                }
            }
        }
    
        const boost::filesystem::path move_to_downloads_dir(const char* torrentPath)
        {
            const libtorrent::torrent_handle th = handle(torrentPath);
            
            if (!th.is_valid())
            {
                cerr << "Invalid torrent!!" << endl;
                return boost::filesystem::path(".");
            }
            const boost::filesystem::path savePath = th.save_path();
            cout << "Save path is: " << savePath.string() << endl;
            const boost::filesystem::path tempDir = savePath.parent_path();
            const boost::filesystem::path incompleteDir = tempDir.parent_path();
            //const boost::filesystem::path sharedDir = incompleteDir.parent_path();
            const boost::filesystem::path downloadsDir(incompleteDir / "downloads");
            th.move_storage(downloadsDir);
            return downloadsDir;
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
        
        void load_state()
        {
#if 0
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
        
        void save_state(bool flag = false)
        {
            if (flag)
            {
                m_session->pause();
            }
        
            TorrentPathToDownloadHandle::iterator it = 
                m_torrent_path_to_handle.begin();
                
			for (; it != m_torrent_path_to_handle.end(); ++it)
			{
                libtorrent::torrent_handle h = it->second;
                
                if (!h.is_valid())
                {
                    continue;
                }
                
                if (h.is_paused())
                {
                    continue;
                }
                
                if (!h.has_metadata())
                {
                    continue;
                }
                
                h.save_resume_data();
                
                std::cout << "torrent: \n" << h.get_torrent_info().name() << "--torrent\n" << std::endl;
            }
#if 0
            boost::filesystem::ofstream out(
                "/session_state.dat", std::ios_base::binary
            );
            out.unsetf(std::ios_base::skipws);
            libtorrent::bencode(
                std::ostream_iterator<char>(out), m_session->state()
            );
#endif
        }
        
#if defined(ENABLE_MESSAGE_QUEUE) && (ENABLE_MESSAGE_QUEUE)
        void tick()
        {
            std::cout << "##########: tick" << std::endl;

                std::auto_ptr<libtorrent::alert> a;
                
                a = m_session->pop_alert();
                
                while (a.get())
                {
                    std::cout << "##########:" << a->message() << std::endl;
                    
                     if (libtorrent::tracker_announce_alert * p = dynamic_cast<
                        libtorrent::tracker_announce_alert *>(a.get())
                        )
                     { 
                        if (p->handle.is_valid())
                        {
                            std::cout << 
                                BOOST_CURRENT_FUNCTION << 
                                ": tracker_announce_alert(" << p->message() << 
                                ")." << 
                            std::endl;
                        }
                    }
                    else if (libtorrent::tracker_reply_alert * p = dynamic_cast<
                        libtorrent::tracker_reply_alert *>(a.get())
                        )
                    {
                        if (p->handle.is_valid())
                        {
                            std::cout << 
                                BOOST_CURRENT_FUNCTION << 
                                ": tracker_announce_alert(" << p->message() << 
                                ")." << 
                            std::endl;
                        }
                    }
                    else if (libtorrent::portmap_alert * p = dynamic_cast<
                        libtorrent::portmap_alert *>(a.get())
                        )
                    {
                        std::cout << 
                            BOOST_CURRENT_FUNCTION << ": portmap_alert(" << 
                            p->message() << ")." << 
                        std::endl;
                    }
                    else if (
                        libtorrent::save_resume_data_failed_alert * p = 
                        dynamic_cast<libtorrent::save_resume_data_failed_alert *
                        >(a.get())
                        )
                    {
                        std::cout << 
                            BOOST_CURRENT_FUNCTION << 
                            ": save_resume_data_failed_alert(" << 
                            p->message() << ")." << 
                        std::endl;
                    }
                    else if (
                        libtorrent::save_resume_data_alert * p = dynamic_cast<
                            libtorrent::save_resume_data_alert *
                            >(a.get())
                        )
                    {
                        std::cout << 
                            BOOST_CURRENT_FUNCTION << 
                            ": save_resume_data_alert(" << 
                            p->message() << ")." << 
                        std::endl;
                        
                        libtorrent::torrent_handle h = p->handle;
						boost::filesystem::ofstream out(
                            h.save_path() / (
                                h.get_torrent_info().name() + ".fastresume"
                            ), std::ios_base::binary
                        );
						out.unsetf(std::ios_base::skipws);
						libtorrent::bencode(
                            std::ostream_iterator<char>(out), *p->resume_data
                        );
                    }
                    else
                    {
                        std::cout << 
                            BOOST_CURRENT_FUNCTION << 
                            ": Alert(" << 
                            p->message() << ")." << 
                        std::endl;
                    }
		
                    a = m_session->pop_alert();
                }
            
//            deadline_timer_.expires_from_now(boost::posix_time::seconds(1));
//            deadline_timer_.async_wait(boost::bind(
//                &session::tick, boost::ref(instance()), boost::asio::placeholders::error)
//            );
        }

#endif // ENABLE_MESSAGE_QUEUE

    private:
    
        boost::shared_ptr<libtorrent::session> m_session;
		InfoHashToIndexMap m_piece_to_index_map;
		TorrentPathToDownloadHandle m_torrent_path_to_handle;
		
    protected:
    
#if defined(ENABLE_MESSAGE_QUEUE) && (ENABLE_MESSAGE_QUEUE)
     //   boost::asio::io_service io_service_;
       // boost::asio::deadline_timer deadline_timer_;
     //   boost::shared_ptr<boost::thread> thread_;
#endif // ENABLE_MESSAGE_QUEUE
};

void voidCall(const char* torrentPath, void (*pt2Func)(const char *))
{
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

JNIEXPORT void JNICALL Java_org_lastbamboo_jni_JLibTorrent_start(JNIEnv * env , jobject obj)
{
    std::cout << "jnltorrent start" << std::endl;
    session::instance().start();
}

JNIEXPORT void JNICALL Java_org_lastbamboo_jni_JLibTorrent_stop(JNIEnv * env , jobject obj)
{
    std::cout << "jnltorrent stop" << std::endl;
    session::instance().stop();
}

JNIEXPORT jlong JNICALL Java_org_lastbamboo_jni_JLibTorrent_add_1torrent(
    JNIEnv * env, jobject obj, jstring jIncompleteDir, jstring arg, jint size,
    jboolean sequential
    )
{
    const char * incompleteDir = env->GetStringUTFChars(jIncompleteDir, JNI_FALSE);
    const char * torrentPath  = env->GetStringUTFChars(arg, JNI_FALSE);
    cout << "Downloading to dir:" << incompleteDir << endl;
	cout << "Got download call from Java for path:" << torrentPath << endl;
    if (!incompleteDir)
    {
		cerr << "Out of memory!!" << endl;
		return -1; /* OutOfMemoryError already thrown */
	}
    if (!torrentPath)
    {
		cerr << "Out of memory!!" << endl;
		return -1; /* OutOfMemoryError already thrown */
	}
    
    try
    {
        const libtorrent::torrent_handle handle = 
            session::instance().download_torrent(incompleteDir, torrentPath, size, sequential);  
    }
    catch (exception & e)
    {
#ifndef NDEBUG
        cerr << BOOST_CURRENT_FUNCTION << ": caught(" << e.what() << ")" << endl;
#endif
    }
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


void removeFunc(const char * torrentPath)
{    
    if (torrentPath)
    {
        session::instance().remove_torrent(torrentPath);
    }
}    
JNIEXPORT void JNICALL Java_org_lastbamboo_jni_JLibTorrent_remove_1torrent(
    JNIEnv * env, jobject obj, jstring arg
) {return voidCall(env, arg, &removeFunc);}


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


string const moveToDownloadsDirFunc(const char* torrentPath)
{
    const boost::filesystem::path newPath = 
        session::instance().move_to_downloads_dir(torrentPath);
    return newPath.string();
}
JNIEXPORT jstring JNICALL Java_org_lastbamboo_jni_JLibTorrent_move_1to_1downloads_1dir(
    JNIEnv * env, jobject obj, jstring arg
){return stringCall(env, arg, &moveToDownloadsDirFunc);}
    

void pause(const char* torrentPath)
{
    const libtorrent::torrent_handle th = session::instance().handle(torrentPath);
    if (th.is_valid())
    {
        th.auto_managed(false);
        th.pause();
    }
}
JNIEXPORT void JNICALL Java_org_lastbamboo_jni_JLibTorrent_pause_1torrent(
    JNIEnv * env, jobject obj, jstring arg
){return voidCall(env, arg, &pause);}


void resume(const char* torrentPath)
{
    session::instance().handle(torrentPath).auto_managed(true);
}
JNIEXPORT void JNICALL Java_org_lastbamboo_jni_JLibTorrent_resume_1torrent(
    JNIEnv * env, jobject obj, jstring arg
){return voidCall(env, arg, &resume);}


void checkMethodId(const jmethodID field)
{
    if (field == NULL) 
    {
        cerr << "Missing method ID" << endl;
        return; /* method not found */
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

jmethodID m_sessionStatusTotalUpload;
jmethodID m_sessionStatusTotalDownload;
jmethodID m_sessionStatusTotalPayloadUpload;
jmethodID m_sessionStatusTotalPayloadDownload;
jmethodID m_sessionStatusUploadRate;
jmethodID m_sessionStatusDownloadRate;
jmethodID m_sessionStatusPayloadUploadRate;
jmethodID m_sessionStatusPayloadDownloadRate;
jmethodID m_sessionStatusNumPeers;


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




