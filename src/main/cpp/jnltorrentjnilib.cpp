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

typedef std::map<string, libtorrent::torrent_handle> TorrentPathToDownloadHandle;
typedef std::map<libtorrent::sha1_hash, int> InfoHashToIndexMap;


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
        
			const boost::uint16_t port = 43542;//(std::rand() % 65535);

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
            
			
			std::cout << "About to log alerts..." << std::endl;
			logAlerts();
            sleep(10);
			
			m_status = m_session->status();
			 
        }
        
        void stop()
        {
            m_session.reset();
        }
	
		libtorrent::torrent_handle download_torrent(const char* torrentPath, int size)
		{
			std::cout << "Building buffer of size: " << size << std::endl;
			std::vector<char> buf(size);
			std::ifstream(torrentPath, std::ios_base::binary).read(&buf[0], size);
			
			libtorrent::lazy_entry e;
			int ret = libtorrent::lazy_bdecode(&buf[0], &buf[0] + buf.size(), e);
			if (ret != 0)
			{
				std::cerr << "invalid bencoding: " << ret << std::endl;
				return libtorrent::torrent_handle();
			}

			std::cerr << "bencoding is valid!!: " << ret << std::endl;
			libtorrent::add_torrent_params p;
			p.save_path = "./";
			p.ti = new libtorrent::torrent_info(e);
			libtorrent::torrent_handle handle = m_session->add_torrent(p);
			
			std::cout << "Adding torrent path: " << torrentPath << std::endl;
			
			string stringPath = torrentPath;
			m_torrent_path_to_handle.insert(
				TorrentPathToDownloadHandle::value_type(stringPath, handle));
			return handle;
		}
	
		long get_index_for_torrent(const char* torrentPath)
		{
			using namespace libtorrent;
			string stringPath = torrentPath;
			TorrentPathToDownloadHandle::const_iterator iter = 
				m_torrent_path_to_handle.find(stringPath);
			if (iter != m_torrent_path_to_handle.end())
			{
				std::cout << "Found torrent" << std::endl;
				torrent_handle th = iter->second;
				
				if (!th.has_metadata()) return -1;
				if (!th.is_valid()) return -1;
				torrent_status status = th.status();
				
				sha1_hash sha1 = th.info_hash();
				
				int index = 0;
				InfoHashToIndexMap::const_iterator iter = m_piece_to_index_map.find(sha1);
				if (iter != m_piece_to_index_map.end())
				{
					index = iter->second;
					std::cout << "Found existing index: " << index << std::endl;
					return index;
				}
				return -1;
			}
			else
			{
				// We don't yet know about the torrent.  
				std::cerr << "No torrent found at " << torrentPath << std::endl;
				return -1;
			}
		}
        
        boost::shared_ptr<libtorrent::session> & get_session()
        {
            return m_session;
        }
	
	
	std::string to_string(int v, int width)
	{
        std::stringstream s;
        s.flags(std::ios_base::right);
        s.width(width);
        s.fill(' ');
        s << v;
        return s.str();
	}
	
	std::string& to_string(float v, int width, int precision = 3)
	{
        // this is a silly optimization
        // to avoid copying of strings
        enum { num_strings = 20 };
        static std::string buf[num_strings];
        static int round_robin = 0;
        std::string& ret = buf[round_robin];
        ++round_robin;
        if (round_robin >= num_strings) round_robin = 0;
        ret.resize(20);
        int size = std::sprintf(&ret[0], "%*.*f", width, precision, v);
        ret.resize((std::min)(size, width));
        return ret;
	}
	
	std::string const& add_suffix(float val)
	{
        const char* prefix[] = {"kB", "MB", "GB", "TB"};
        const int num_prefix = sizeof(prefix) / sizeof(const char*);
        for (int i = 0; i < num_prefix; ++i)
        {
			val /= 1000.f;
			if (fabs(val) < 1000.f)
			{
				std::string& ret = to_string(val, 4);
				ret += prefix[i];
				return ret;
			}
        }
        std::string& ret = to_string(val, 4);
        ret += "PB";
        return ret;
	}
	
	
	char const* esc(char const* code)
	{
#ifdef ANSI_TERMINAL_COLORS
        // this is a silly optimization
        // to avoid copying of strings
        enum { num_strings = 200 };
        static char buf[num_strings][20];
        static int round_robin = 0;
        char* ret = buf[round_robin];
        ++round_robin;
        if (round_robin >= num_strings) round_robin = 0;
        ret[0] = '\033';
        ret[1] = '[';
        int i = 2;
        int j = 0;
        while (code[j]) ret[i++] = code[j++];
        ret[i++] = 'm';
        ret[i++] = 0;
        return ret;
#else
        return "";
#endif
	}

	std::string const& progress_bar(float progress, int width, char const* code = "33")
	{
        static std::string bar;
        bar.clear();
        bar.reserve(width + 10);
		
        int progress_chars = static_cast<int>(progress * width + .5f);
        bar = esc(code);
        std::fill_n(std::back_inserter(bar), progress_chars, '#');
        bar += esc("0");
        std::fill_n(std::back_inserter(bar), width - progress_chars, '-');
        return bar;
	}
	
	void logAlerts()
	{
		using namespace libtorrent;
		updateFiles();
		std::vector<libtorrent::torrent_handle> handles = m_session->get_torrents();
		
		for (std::vector<libtorrent::torrent_handle>::iterator i = handles.begin();
			 i != handles.end(); ++i)
		{
			libtorrent::torrent_handle& h = *i;
			if (!h.has_metadata()) continue;
			if (!h.is_valid()) continue;
			torrent_status status = h.status();

			std::vector<size_type> file_progress;
			h.file_progress(file_progress);
			libtorrent::torrent_info const& info = h.get_torrent_info();
			std::cout << "SHA-1 for info: " << info.info_hash();
			for (int i = 0; i < info.num_files(); ++i)
			{
				
				float progress = info.file_at(i).size > 0
					?float(file_progress[i]) / info.file_at(i).size:1;
				if (file_progress[i] == info.file_at(i).size)
					std::cout << progress_bar(1.f, 100, "32");
				else
					std::cout << progress_bar(progress, 100, "33");
				std::cout << " " << to_string(progress * 100.f, 5) << "% "
					<< add_suffix(file_progress[i]) << " "
					<< info.file_at(i).path.leaf() << "\n";
			}	
		}
		//float downloadRate = m_status.download_rate;
		//std::cout << "Download rate: " << downloadRate << std::endl;		
		
		std::auto_ptr<libtorrent::alert> a = m_session->pop_alert();
		
		std::cout << "Popped alert!" << std::endl;
		while (a.get()) {
			std::cout << "Got some alert!" << std::endl;
			if (libtorrent::torrent_finished_alert* p = dynamic_cast<libtorrent::torrent_finished_alert*>(a.get())) {
				if (p->handle.is_valid()) {
					std::cout << "torrent finished" << std::endl;
				}
				
			} else if (libtorrent::peer_error_alert* p = dynamic_cast<libtorrent::peer_error_alert*>(a.get())) {
				std::cout << "peer error" << std::endl;
				
			} else if (libtorrent::invalid_request_alert* p = dynamic_cast<libtorrent::invalid_request_alert*>(a.get())) {
				std::cout << "invalid request!" << std::endl;
			} else if (libtorrent::tracker_error_alert * p = dynamic_cast<libtorrent::tracker_error_alert *>(a.get())) {
				std::cout << "tracker error!" << std::endl;
				if (p->handle.is_valid()) {
					switch (p->status_code) { 
						case 404:
						case 401: {
						}
							break;
						default:
							break;
					}
				}
			} else if (libtorrent::tracker_warning_alert * p = dynamic_cast<libtorrent::tracker_warning_alert *>(a.get())) { 
				
				if (p->handle.is_valid()) {
					std::cout << "tracker warning" << std::endl;
				}
				
			} else if (libtorrent::tracker_announce_alert * p = dynamic_cast<libtorrent::tracker_announce_alert *>(a.get())) { 
				
				if (p->handle.is_valid()) {
					std::cout << "tracker announce" << std::endl;
				}
				
			} else if (libtorrent::tracker_reply_alert * p = dynamic_cast<libtorrent::tracker_reply_alert *>(a.get())) {
				if (p->handle.is_valid()) {
					std::cout << "tracker reply" << std::endl;
				}
			} else if (libtorrent::portmap_alert * p = dynamic_cast<libtorrent::portmap_alert *>(a.get())) {
				std::cout << "portmap alert" << std::endl;
			} else {
				std::cout << "unknown alert" << std::endl;
			}
			
			a = m_session->pop_alert();
			
		}
	}
	
	void updateFiles() 
	{
		using namespace libtorrent;
		std::vector<torrent_handle> handles = m_session->get_torrents();
		
		for (std::vector<torrent_handle>::iterator i = handles.begin();
			 i != handles.end(); ++i)
		{
			torrent_handle& h = *i;
			if (!h.has_metadata()) continue;
			if (!h.is_valid()) continue;
			torrent_status status = h.status();
			
			sha1_hash sha1 = h.info_hash();
			
			int index = 0;
			InfoHashToIndexMap::const_iterator iter = m_piece_to_index_map.find(sha1);
			if (iter != m_piece_to_index_map.end())
			{
				index = iter->second;
				std::cout << "Found existing index: " << index << std::endl;
			}
			else
			{
				// We don't yet know about the torrent.  Set its start index
				// to 0.
				std::cout << "We didn't find the index. Setting start index to 0. " << std::endl;
				m_piece_to_index_map.insert(InfoHashToIndexMap::value_type(sha1, 0));
			}
			unsigned int numPieces = status.pieces.size();
			std::cout << "Num pieces is: " << numPieces << std::endl;
			for (unsigned int j = index; j < numPieces; j++)
			{
				if (status.pieces[j])
				{
					std::cout << "Found piece at index: " << j << std::endl;
					// We have this piece -- stream it.
				} else
				{
					// We do not have this piece -- set the index and
					// break.
					std::cout << "Setting index to: " << j << std::endl;
					m_piece_to_index_map[sha1] = j;
					break;
				}
			}
		}
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
  
  // Don't use objective-c!!!

  jint rc = 0;
  /* = get_torrent_handle(argutf);
  
    const std::string s = boost::lexical_cast<std::string>(handle.get_torrent_info().info_hash());
    NSString * handleHash = [NSString stringWithUTF8String:(s.c_str())];
	
	NSArray * torrents = [[TorrentController sharedInstance] torrents];
	
	NSEnumerator * enumerator = [torrents objectEnumerator];
	
	Torrent * torrent;
	
	while (torrent = [enumerator nextObject]) {
		if ([[torrent hash] isEqualToString:handleHash])
			return (torrent);
	}
	
	return (nil);
	 */
  
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
    session::instance().logAlerts();
}

JNIEXPORT jint JNICALL Java_org_lastbamboo_jni_JLibTorrent_add_1torrent(
    JNIEnv * env, jobject obj, jstring arg, jint size
    )
{
	std::cout << "Got download call from Java with data:" << arg << std::endl;
    const char * torrentPath  = env->GetStringUTFChars(arg, JNI_FALSE);
    if (!torrentPath)
    {
		return -1; /* OutOfMemoryError already thrown */
	}
	
	libtorrent::torrent_handle  handle = session::instance().download_torrent(
        torrentPath, size
    );
	
	env->ReleaseStringUTFChars(arg, torrentPath);
	return 0;
}

JNIEXPORT jlong JNICALL Java_org_lastbamboo_jni_JLibTorrent_get_1max_1byte_1for_1torrent(
	JNIEnv * env, jobject obj, jstring arg
	)
{
	std::cout << "Got download call from Java with data:" << arg << std::endl;
    const char * torrentPath  = env->GetStringUTFChars(arg, JNI_FALSE);
    if (!torrentPath)
    {
		return -1; /* OutOfMemoryError already thrown */
	}
	
	long index = session::instance().get_index_for_torrent(torrentPath);
	
	env->ReleaseStringUTFChars(arg, torrentPath);
	return index;	
}


