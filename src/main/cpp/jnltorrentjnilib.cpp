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

			settings.user_agent = "LittleShoot/1.0 libtorrent/"
                LIBTORRENT_VERSION;
			settings.stop_tracker_timeout = 5;
            
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
			ifstream(torrentPath, ios_base::binary).read(&buf[0], size);
            
            libtorrent::entry e;
            
            try 
            {
                e = libtorrent::bdecode(
                    buf.begin(), buf.end()
                );
            
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
            handle.set_max_uploads(-1);
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
			const string stringPath = torrentPath;
			const TorrentPathToDownloadHandle::iterator iter = 
				m_torrent_path_to_handle.find(stringPath);
			if (iter != m_torrent_path_to_handle.end())
			{
				log_debug("Found torrent");
                
				const torrent_handle th = iter->second;
				
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
				const torrent_info ti = th.get_torrent_info();
				const torrent_status status = th.status();
				if (is_finished(th))
				{
					log_debug("File is finished!!!");
					return ti.total_size();
				}
				
				log_debug("Download rate: " << status.download_rate);
				
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
			else
			{
				// We don't yet know about the torrent.  
				log_debug("No torrent found at " << torrentPath);
				return -1;
			}
		}
	
        const bool is_finished(const libtorrent::torrent_handle& th)
        {
            using namespace libtorrent;
            const torrent_status status = th.status();
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
            const torrent_info ti = info(torrentPath);
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
    
        const libtorrent::size_type get_size_for_torrent(const char* torrentPath) 
		{
			return info(torrentPath).total_size();
		}
	
        const long get_bytes_read_for_torrent(const char* torrentPath) 
        {
            return status(torrentPath).total_wanted_done;
        }
    
        const int get_num_peers_for_torrent(const char* torrentPath) 
        {
            return status(torrentPath).num_peers;
        }
     
        const float get_download_rate_for_torrent(const char* torrentPath)
        {
            return status(torrentPath).download_rate;
        }
    
        int get_state_for_torrent(const char* torrentPath) 
        {
            const libtorrent::torrent_handle th = handle(torrentPath);
            //const libtorrent::torrent_status stat = status(torrentPath);
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
    
        const int get_num_files_for_torrent(const char* torrentPath) 
        {
            return info(torrentPath).num_files();
        }
    
        const boost::filesystem::path move_to_downloads_dir(const char* torrentPath)
        {
            const libtorrent::torrent_handle th = 
                handle(torrentPath);
            const boost::filesystem::path savePath = th.save_path();
            cout << "Save path is: " << savePath.string() << endl;
            const boost::filesystem::path tempDir = savePath.parent_path();
            const boost::filesystem::path incompleteDir = tempDir.parent_path();
            //const boost::filesystem::path sharedDir = incompleteDir.parent_path();
            const boost::filesystem::path downloadsDir(incompleteDir / "downloads");
            th.move_storage(downloadsDir);
            return downloadsDir;
        }
    
		void remove_torrent(const char* torrentPath) 
		{
			using namespace libtorrent;
			const torrent_handle th = handle(torrentPath);
            if (th.is_valid())
            {
                m_session->remove_torrent(th);
            }
		}
    
        const libtorrent::torrent_info info(const char* torrentPath) 
        {
            using namespace libtorrent;
            const torrent_handle th = handle(torrentPath);
            return th.get_torrent_info();
        }
            
        const libtorrent::torrent_status status(const char* torrentPath) 
        {
            using namespace libtorrent;
            const torrent_handle th = handle(torrentPath);
            return th.status();
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
	
	const libtorrent::torrent_handle handle = session::instance().download_torrent(
        incompleteDir, torrentPath, size, sequential
    );
	
	env->ReleaseStringUTFChars(arg, torrentPath);
	env->ReleaseStringUTFChars(arg, incompleteDir);
	return 0;
}

JNIEXPORT jlong JNICALL Java_org_lastbamboo_jni_JLibTorrent_get_1max_1byte_1for_1torrent(
	JNIEnv * env, jobject obj, jstring arg
	)
{
    const char * torrentPath  = env->GetStringUTFChars(arg, JNI_FALSE);
	//std::cout << "Got max byte call for path:" << torrentPath << std::endl;
    if (!torrentPath)
    {
		cerr << "Out of memory!!" << endl;
		return -1; /* OutOfMemoryError already thrown */
	}
	
	const long index = session::instance().get_index_for_torrent(torrentPath);
	
	env->ReleaseStringUTFChars(arg, torrentPath);
	return index;	
}

JNIEXPORT jstring JNICALL Java_org_lastbamboo_jni_JLibTorrent_get_1name_1for_1torrent(
    JNIEnv * env, jobject obj, jstring arg
)
{
    const char * torrentPath  = env->GetStringUTFChars(arg, JNI_FALSE);
 //   cout << "Got name request for torrent:" << torrentPath << endl;
    if (!torrentPath)
    {
		cerr << "Out of memory!!" << endl;
		return NULL; /* OutOfMemoryError already thrown */
	}
	
	const string name  = 
        session::instance().get_name_for_torrent(torrentPath); 
	
	env->ReleaseStringUTFChars(arg, torrentPath);
    cout << "Returning name..." << endl;
	return env->NewStringUTF(name.c_str());
}

JNIEXPORT jlong JNICALL Java_org_lastbamboo_jni_JLibTorrent_get_1size_1for_1torrent(
    JNIEnv * env, jobject obj, jstring arg
)
{
	const char * torrentPath  = env->GetStringUTFChars(arg, JNI_FALSE);
	//std::cout << "Got size request for torrent:" << torrentPath << std::endl;
    if (!torrentPath)
    {
		cerr << "Out of memory!!" << endl;
		return NULL; /* OutOfMemoryError already thrown */
	}
	
	const libtorrent::size_type size = 
		session::instance().get_size_for_torrent(torrentPath); 
	
	env->ReleaseStringUTFChars(arg, torrentPath);
  //  cout << "Returning size..." << endl;
	return size;
}

JNIEXPORT void JNICALL Java_org_lastbamboo_jni_JLibTorrent_remove_1torrent(
    JNIEnv * env, jobject obj, jstring arg
)
{
	const char * torrentPath  = env->GetStringUTFChars(arg, JNI_FALSE);
	log_debug("Got cancel request for torrent:" << torrentPath);
    if (!torrentPath)
    {
		cerr << "Out of memory!!" << endl;
		return; /* OutOfMemoryError already thrown */
	}
	
	session::instance().remove_torrent(torrentPath); 
	
	env->ReleaseStringUTFChars(arg, torrentPath);
}

JNIEXPORT jint JNICALL Java_org_lastbamboo_jni_JLibTorrent_get_1state_1for_1torrent(
	JNIEnv * env, jobject obj, jstring arg
)
{
	const char * torrentPath  = env->GetStringUTFChars(arg, JNI_FALSE);
	log_debug("Got state request for torrent:" << torrentPath);
    if (!torrentPath)
    {
		cerr << "Out of memory!!" << endl;
		return NULL; /* OutOfMemoryError already thrown */
	}
	
	const int state = session::instance().get_state_for_torrent(torrentPath); 

    log_debug("Returning state: " << state);
	env->ReleaseStringUTFChars(arg, torrentPath);
	return state;
}

JNIEXPORT jint JNICALL Java_org_lastbamboo_jni_JLibTorrent_get_1num_1files_1for_1torrent(
    JNIEnv * env, jobject obj, jstring arg
)
{
	const char * torrentPath  = env->GetStringUTFChars(arg, JNI_FALSE);
	log_debug("Got num files request for torrent:" << torrentPath);
    if (!torrentPath)
    {
		cerr << "Out of memory!!" << endl;
		return NULL; /* OutOfMemoryError already thrown */
	}
	
	const int numFiles = session::instance().get_num_files_for_torrent(torrentPath); 
	
	env->ReleaseStringUTFChars(arg, torrentPath);
	return numFiles;
}


JNIEXPORT jlong JNICALL Java_org_lastbamboo_jni_JLibTorrent_get_1bytes_1read_1for_1torrent(
    JNIEnv * env, jobject obj, jstring arg
)
{
	const char * torrentPath  = env->GetStringUTFChars(arg, JNI_FALSE);
	log_debug("Got bytes read request for torrent:" << torrentPath);
    if (!torrentPath)
    {
		cerr << "Out of memory!!" << endl;
		return NULL; /* OutOfMemoryError already thrown */
	}
	 
    const long bytesRead = 
        session::instance().get_bytes_read_for_torrent(torrentPath);
	
   // cout << "Bytes read: " << bytesRead << endl;
	env->ReleaseStringUTFChars(arg, torrentPath);
	return bytesRead;
}

/*
 * Class:     org_lastbamboo_jni_JLibTorrent
 * Method:    get_num_peers_for_torrent
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_org_lastbamboo_jni_JLibTorrent_get_1num_1peers_1for_1torrent(
    JNIEnv * env, jobject obj, jstring arg
)
{
    const char * torrentPath  = env->GetStringUTFChars(arg, JNI_FALSE);
	log_debug("Got num peers request for torrent:" << torrentPath);
    if (!torrentPath)
    {
		log_debug("Out of memory!!");
		return NULL; /* OutOfMemoryError already thrown */
	}
    
    const int numPeers = 
        session::instance().get_num_peers_for_torrent(torrentPath);
	
    log_debug("Num peers: " << numPeers);
	env->ReleaseStringUTFChars(arg, torrentPath);
	return numPeers;
}

/*
 * Class:     org_lastbamboo_jni_JLibTorrent
 * Method:    get_speed_for_torrent
 * Signature: (Ljava/lang/String;)D
 */
JNIEXPORT jdouble JNICALL Java_org_lastbamboo_jni_JLibTorrent_get_1speed_1for_1torrent(
    JNIEnv * env, jobject obj, jstring arg
)
{
    const char * torrentPath  = env->GetStringUTFChars(arg, JNI_FALSE);
	log_debug("Got speed request for torrent:" << torrentPath);
    if (!torrentPath)
    {
		cerr << "Out of memory!!" << endl;
		return NULL; /* OutOfMemoryError already thrown */
	}
    
    const float downloadRate = 
        session::instance().get_download_rate_for_torrent(torrentPath);
	
  //  cout << "Speed: " << downloadRate << endl;
	env->ReleaseStringUTFChars(arg, torrentPath);
	return downloadRate;
}

JNIEXPORT jstring JNICALL Java_org_lastbamboo_jni_JLibTorrent_move_1to_1downloads_1dir(
    JNIEnv * env, jobject obj, jstring arg
)
{
    const char * torrentPath  = env->GetStringUTFChars(arg, JNI_FALSE);
	cout << "Moving torrent to downloads dir:" << torrentPath << endl;
    if (!torrentPath)
    {
		cerr << "Out of memory!!" << endl;
		return NULL; /* OutOfMemoryError already thrown */
	}
    
    const boost::filesystem::path newPath = 
        session::instance().move_to_downloads_dir(torrentPath);
    
    const char * savePath = newPath.string().c_str();
	
	env->ReleaseStringUTFChars(arg, torrentPath);
    cout << "Returning path..." << savePath << endl;
	const jstring finalPath = env->NewStringUTF(savePath);

	return finalPath;
}

JNIEXPORT void JNICALL Java_org_lastbamboo_jni_JLibTorrent_pause_1torrent(
    JNIEnv * env, jobject obj, jstring arg
)
{
    const char * torrentPath  = env->GetStringUTFChars(arg, JNI_FALSE);
    if (!torrentPath)
    {
		cerr << "Out of memory!!" << endl;
	}
    const libtorrent::torrent_handle th = session::instance().handle(torrentPath);
    th.auto_managed(false);
    th.pause();
    env->ReleaseStringUTFChars(arg, torrentPath);
}


JNIEXPORT void JNICALL Java_org_lastbamboo_jni_JLibTorrent_resume_1torrent(
    JNIEnv * env, jobject obj, jstring arg
)
{
    const char * torrentPath  = env->GetStringUTFChars(arg, JNI_FALSE);
    cout << "Resuming torrent at:" << torrentPath << endl;
    if (!torrentPath)
    {
		cerr << "Out of memory!!" << endl;
	}
    
    session::instance().handle(torrentPath).auto_managed(true);
    env->ReleaseStringUTFChars(arg, torrentPath);
}

