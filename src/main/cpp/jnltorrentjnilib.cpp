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
#include <libtorrent/extensions/ut_pex.hpp>
#include "libtorrent/bencode.hpp"

#include "org_lastbamboo_jni_JLibTorrent.h"

using namespace std;

typedef map<const string, const libtorrent::torrent_handle> TorrentPathToDownloadHandle;
typedef map<const libtorrent::sha1_hash, const unsigned int> InfoHashToIndexMap;


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
			
			m_session->start_dht();
            
			m_session->add_dht_router(std::make_pair(
                std::string("router.bittorrent.com"), 6881)
            );
            
			m_session->add_dht_router(std::make_pair(
                std::string("router.utorrent.com"), 6881)
            );
            
			m_session->add_dht_router(std::make_pair(
                std::string("router.bitcomet.com"), 6881)
            );

			libtorrent::session_settings settings;

			settings.user_agent = "LittleShoot/1.0 libtorrent/"
                LIBTORRENT_VERSION;
			settings.stop_tracker_timeout = 5;
			
            m_session->set_settings(settings);

            m_session->add_extension(&libtorrent::create_ut_pex_plugin);
			m_status = m_session->status();
			 
        }
        
        void stop()
        {
            m_session.reset();
        }
	
		const libtorrent::torrent_handle download_torrent(
			const char* incompleteDir, const char* torrentPath, int size)
		{
			cout << "Building buffer of size: " << size << endl;
			vector<char> buf(size);
			ifstream(torrentPath, ios_base::binary).read(&buf[0], size);
			
			libtorrent::lazy_entry e;
			int ret = libtorrent::lazy_bdecode(&buf[0], &buf[0] + buf.size(), e);
			if (ret != 0)
			{
				cerr << "Bad bencoding. Returning invalid handle." << ret << endl;
				return libtorrent::torrent_handle();
			}

			cerr << "bencoding is valid!!: " << ret << endl;
			libtorrent::add_torrent_params p;
			p.save_path = incompleteDir;
			try
			{
				p.ti = new libtorrent::torrent_info(e);
				libtorrent::torrent_handle handle = m_session->add_torrent(p);
				
				if (!handle.is_valid())
				{
					cerr << "Torrent handle not valid!!" << endl;
				}
				
				cout << "Adding torrent path to map: " << torrentPath << endl;
				
				const string stringPath = torrentPath;
				m_torrent_path_to_handle.insert(
				    TorrentPathToDownloadHandle::value_type(stringPath, handle));
				handle.set_sequential_download(true);
				return handle;
			}
			catch (exception& e)
			{
				cerr << "Error adding torrent" << e.what() << endl;
                return libtorrent::torrent_handle();
			}
		}
	
        libtorrent::torrent_handle get_torrent_for_path(const char* torrentPath)
        {
            using namespace libtorrent;
            const string stringPath = torrentPath;
            const TorrentPathToDownloadHandle::iterator iter = 
                m_torrent_path_to_handle.find(stringPath);
            if (iter != m_torrent_path_to_handle.end())
            {
                cout << "Found torrent" << endl;
                const torrent_handle th = iter->second;
                
                if (!th.has_metadata()) 
                {
                    cerr << "No metadata for torrent.  Returning invalid." << endl;
                    return torrent_handle();
                }
                if (!th.is_valid()) 
                {
                    cerr << "Torrent not valid.  Returning invalid." << endl; 
                    return torrent_handle();
                }
                return th;
            }
            else
            {
                cerr << "No handle for torrent! Returning invalid." << endl;
                return torrent_handle();
            }
        }
	
		const long get_index_for_torrent(const char* torrentPath)
		{
			using namespace libtorrent;
			const string stringPath = torrentPath;
			const TorrentPathToDownloadHandle::iterator iter = 
				m_torrent_path_to_handle.find(stringPath);
			if (iter != m_torrent_path_to_handle.end())
			{
				cout << "Found torrent" << endl;
				const torrent_handle th = iter->second;
				
				if (!th.has_metadata()) 
				{
					cerr << "No metadata for torrent" << endl;
					return -1;
				}
				if (!th.is_valid()) 
				{
					cerr << "Torrent not valid" << endl; 
					return -1;
				}
				const torrent_info ti = th.get_torrent_info();
				const torrent_status status = th.status();
				if (is_finished(th))
				{
					cout << "File is finished!!!" << endl;
					return ti.total_size();
				}
				
				cout << "Download rate: " << status.download_rate << endl;
				
				const sha1_hash sha1 = th.info_hash();
				
				int index = 0;
				const InfoHashToIndexMap::iterator iter = m_piece_to_index_map.find(sha1);
				if (iter != m_piece_to_index_map.end())
				{
					index = iter->second;
					std::cout << "Found existing index: " << index << std::endl;
					//return index;
				}
				else
				{
					cerr << "No existing torrent" << endl;
					m_piece_to_index_map.insert(InfoHashToIndexMap::value_type(sha1, 0));
					return -1;
				}
				const unsigned int numPieces = status.pieces.size();
				cout << "Num pieces is: " << numPieces << endl;
				for (unsigned int j = index; j < numPieces; j++)
				{
					if (status.pieces[j])
					{
						cout << "Found piece at index: " << j << endl;
						// We have this piece -- stream it.
					} else
					{
						// We do not have this piece -- set the index and
						// break.
						cout << "Setting index to: " << j << endl;
						//m_piece_to_index_map[sha1] = j;
						m_piece_to_index_map.insert(InfoHashToIndexMap::value_type(sha1, j));
						
						cout << "index: " << j << endl;
						cout << "piece length is: " << ti.piece_length() << endl;
						cout << "num pieces is: " << ti.num_pieces() << endl;
						
						const unsigned long maxByte = j * ti.piece_length();
						
						cout << "max byte is: " << maxByte << endl;

						return maxByte;
					}
				}
				return (index - 1)  * ti.piece_length();
			}
			else
			{
				// We don't yet know about the torrent.  
				cerr << "No torrent found at " << torrentPath << endl;
				return -1;
			}
		}
	
        const bool is_finished(const libtorrent::torrent_handle& th)
        {
            using namespace libtorrent;
            const torrent_status status = th.status();
            const torrent_status::state_t s = status.state;
            cout << "Found state: " << s << endl;
            cout << "Sequential download: " << th.is_sequential_download();
            if (s == torrent_status::finished)
            {
                cout << "Got finished state!!" << endl;
                return true;
            } else if (s == torrent_status::seeding)
            {
                cout << "Got seeding state!!" << endl;
                return true;
            } else
            {
                cout << "Got other state!!" << endl;
                return false;
            }
            
        }
	
		const boost::filesystem::path get_save_path_for_torrent(const char* torrentPath)
		{
			using namespace libtorrent;
			const torrent_handle th = get_torrent_for_path(torrentPath);
			const torrent_info ti = th.get_torrent_info();
			const file_entry fe = ti.file_at(0);
			return fe.path;
		}
	
		const int get_size_for_torrent(const char* torrentPath)
		{
			using namespace libtorrent;
			const torrent_handle th = get_torrent_for_path(torrentPath);
			const torrent_info ti = th.get_torrent_info();
			return ti.total_size();
		}
	
	
        const int get_state_for_torrent(const char* torrentPath)
        {
            using namespace libtorrent;
            const torrent_handle th = get_torrent_for_path(torrentPath);
            const torrent_status status = th.status();
            return status.state;
        }
        
		void remove_torrent(const char* torrentPath)
		{
			using namespace libtorrent;
			const torrent_handle th = get_torrent_for_path(torrentPath);
			m_session->remove_torrent(th);
		}
        
        const boost::shared_ptr<libtorrent::session> & get_session()
        {
            return m_session;
        }
    
    private:
    
        boost::shared_ptr<libtorrent::session> m_session;
		//boost::asio::deadline_timer m_timer;
		int m_count;
		libtorrent::session_status m_status;
		InfoHashToIndexMap m_piece_to_index_map;
		TorrentPathToDownloadHandle m_torrent_path_to_handle;
		
    
    protected:
    
        // ...
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

JNIEXPORT jint JNICALL Java_org_lastbamboo_jni_JLibTorrent_get_1torrent_1handle(
    JNIEnv * env, jobject obj, jstring arg
    )
{
  const char * argutf  = env->GetStringUTFChars(arg, JNI_FALSE);

  jint rc = 0;
  
  env->ReleaseStringUTFChars(arg, argutf);

  return rc;
}

JNIEXPORT void JNICALL Java_org_lastbamboo_jni_JLibTorrent_download_1torrent(
    JNIEnv * env, jobject obj, jstring arg
    )
{
	std::cout << "Got download torrent call from Java with data:" << arg << std::endl;
	//const char * argutf  = env->GetStringUTFChars(arg, JNI_FALSE);
	
	//jint rc = download_torrent(argutf);
    
	//std::cout << "Got download torrent call" << arg << std::endl;
	
}

JNIEXPORT void JNICALL Java_org_lastbamboo_jni_JLibTorrent_processEvents(JNIEnv * env, jobject obj)
{
    // Perfrom any event processing here.
    //session::instance().logAlerts();
}

JNIEXPORT jlong JNICALL Java_org_lastbamboo_jni_JLibTorrent_add_1torrent(
    JNIEnv * env, jobject obj, jstring jIncompleteDir, jstring arg, jint size
    )
{
    const char * incompleteDir = env->GetStringUTFChars(jIncompleteDir, JNI_FALSE);
    const char * torrentPath  = env->GetStringUTFChars(arg, JNI_FALSE);
	std::cout << "Got download call from Java for path:" << torrentPath << std::endl;
    if (!torrentPath)
    {
		cerr << "Out of memory!!" << endl;
		return -1; /* OutOfMemoryError already thrown */
	}
	
	const libtorrent::torrent_handle handle = session::instance().download_torrent(
        incompleteDir, torrentPath, size
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
	std::cout << "Got max byte call for path:" << torrentPath << std::endl;
    if (!torrentPath)
    {
		cerr << "Out of memory!!" << endl;
		return -1; /* OutOfMemoryError already thrown */
	}
	
	long index = session::instance().get_index_for_torrent(torrentPath);
	
	env->ReleaseStringUTFChars(arg, torrentPath);
	return index;	
}

JNIEXPORT jstring JNICALL Java_org_lastbamboo_jni_JLibTorrent_get_1save_1path_1for_1torrent(
    JNIEnv * env, jobject obj, jstring arg
)
{
    const char * torrentPath  = env->GetStringUTFChars(arg, JNI_FALSE);
	std::cout << "Got save path request for torrent:" << torrentPath << std::endl;
    if (!torrentPath)
    {
		cerr << "Out of memory!!" << endl;
		return NULL; /* OutOfMemoryError already thrown */
	}
	
	boost::filesystem::path path = 
		session::instance().get_save_path_for_torrent(torrentPath); 
	const char * savePath = path.string().c_str();
	
	env->ReleaseStringUTFChars(arg, torrentPath);
	return env->NewStringUTF(savePath);
}

JNIEXPORT jlong JNICALL Java_org_lastbamboo_jni_JLibTorrent_get_1size_1for_1torrent(
    JNIEnv * env, jobject obj, jstring arg
)
{
	const char * torrentPath  = env->GetStringUTFChars(arg, JNI_FALSE);
	std::cout << "Got size request for torrent:" << torrentPath << std::endl;
    if (!torrentPath)
    {
		cerr << "Out of memory!!" << endl;
		return NULL; /* OutOfMemoryError already thrown */
	}
	
	const int size = 
		session::instance().get_size_for_torrent(torrentPath); 
	
	env->ReleaseStringUTFChars(arg, torrentPath);
	return size;
}

JNIEXPORT void JNICALL Java_org_lastbamboo_jni_JLibTorrent_remove_1torrent(
    JNIEnv * env, jobject obj, jstring arg
)
{
	const char * torrentPath  = env->GetStringUTFChars(arg, JNI_FALSE);
	cout << "Got cancel request for torrent:" << torrentPath << endl;
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
	cout << "Got cancel request for torrent:" << torrentPath << endl;
    if (!torrentPath)
    {
		cerr << "Out of memory!!" << endl;
		return NULL; /* OutOfMemoryError already thrown */
	}
	
	const int state = session::instance().get_state_for_torrent(torrentPath); 
	
	env->ReleaseStringUTFChars(arg, torrentPath);
	return state;
}