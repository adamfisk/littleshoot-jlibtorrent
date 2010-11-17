/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class org_lastbamboo_jni_JLibTorrent */

#ifndef _Included_org_lastbamboo_jni_JLibTorrent
#define _Included_org_lastbamboo_jni_JLibTorrent
#ifdef __cplusplus
extern "C" {
#endif
/*
 * Class:     org_lastbamboo_jni_JLibTorrent
 * Method:    set_max_upload_speed
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_org_lastbamboo_jni_JLibTorrent_set_1max_1upload_1speed
  (JNIEnv *, jobject, jint);

/*
 * Class:     org_lastbamboo_jni_JLibTorrent
 * Method:    add_tcp_upnp_mapping
 * Signature: (II)I
 */
JNIEXPORT jint JNICALL Java_org_lastbamboo_jni_JLibTorrent_add_1tcp_1upnp_1mapping
  (JNIEnv *, jobject, jint, jint);

/*
 * Class:     org_lastbamboo_jni_JLibTorrent
 * Method:    add_udp_upnp_mapping
 * Signature: (II)I
 */
JNIEXPORT jint JNICALL Java_org_lastbamboo_jni_JLibTorrent_add_1udp_1upnp_1mapping
  (JNIEnv *, jobject, jint, jint);

/*
 * Class:     org_lastbamboo_jni_JLibTorrent
 * Method:    add_tcp_natpmp_mapping
 * Signature: (II)I
 */
JNIEXPORT jint JNICALL Java_org_lastbamboo_jni_JLibTorrent_add_1tcp_1natpmp_1mapping
  (JNIEnv *, jobject, jint, jint);

/*
 * Class:     org_lastbamboo_jni_JLibTorrent
 * Method:    add_udp_natpmp_mapping
 * Signature: (II)I
 */
JNIEXPORT jint JNICALL Java_org_lastbamboo_jni_JLibTorrent_add_1udp_1natpmp_1mapping
  (JNIEnv *, jobject, jint, jint);

/*
 * Class:     org_lastbamboo_jni_JLibTorrent
 * Method:    check_alerts
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_lastbamboo_jni_JLibTorrent_check_1alerts
  (JNIEnv *, jobject);

/*
 * Class:     org_lastbamboo_jni_JLibTorrent
 * Method:    cacheMethodIds
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_lastbamboo_jni_JLibTorrent_cacheMethodIds
  (JNIEnv *, jobject);

/*
 * Class:     org_lastbamboo_jni_JLibTorrent
 * Method:    update_session_status
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_lastbamboo_jni_JLibTorrent_update_1session_1status
  (JNIEnv *, jobject);

/*
 * Class:     org_lastbamboo_jni_JLibTorrent
 * Method:    move_to_downloads_dir
 * Signature: (Ljava/lang/String;Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_org_lastbamboo_jni_JLibTorrent_move_1to_1downloads_1dir
  (JNIEnv *, jobject, jstring, jstring);

/*
 * Class:     org_lastbamboo_jni_JLibTorrent
 * Method:    rename
 * Signature: (Ljava/lang/String;Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_org_lastbamboo_jni_JLibTorrent_rename
  (JNIEnv *, jobject, jstring, jstring);

/*
 * Class:     org_lastbamboo_jni_JLibTorrent
 * Method:    get_bytes_read_for_torrent
 * Signature: (Ljava/lang/String;)J
 */
JNIEXPORT jlong JNICALL Java_org_lastbamboo_jni_JLibTorrent_get_1bytes_1read_1for_1torrent
  (JNIEnv *, jobject, jstring);

/*
 * Class:     org_lastbamboo_jni_JLibTorrent
 * Method:    get_num_peers_for_torrent
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_org_lastbamboo_jni_JLibTorrent_get_1num_1peers_1for_1torrent
  (JNIEnv *, jobject, jstring);

/*
 * Class:     org_lastbamboo_jni_JLibTorrent
 * Method:    get_speed_for_torrent
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_org_lastbamboo_jni_JLibTorrent_get_1speed_1for_1torrent
  (JNIEnv *, jobject, jstring);

/*
 * Class:     org_lastbamboo_jni_JLibTorrent
 * Method:    get_num_files_for_torrent
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_org_lastbamboo_jni_JLibTorrent_get_1num_1files_1for_1torrent
  (JNIEnv *, jobject, jstring);

/*
 * Class:     org_lastbamboo_jni_JLibTorrent
 * Method:    get_state_for_torrent
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_org_lastbamboo_jni_JLibTorrent_get_1state_1for_1torrent
  (JNIEnv *, jobject, jstring);

/*
 * Class:     org_lastbamboo_jni_JLibTorrent
 * Method:    get_size_for_torrent
 * Signature: (Ljava/lang/String;)J
 */
JNIEXPORT jlong JNICALL Java_org_lastbamboo_jni_JLibTorrent_get_1size_1for_1torrent
  (JNIEnv *, jobject, jstring);

/*
 * Class:     org_lastbamboo_jni_JLibTorrent
 * Method:    get_name_for_torrent
 * Signature: (Ljava/lang/String;)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_org_lastbamboo_jni_JLibTorrent_get_1name_1for_1torrent
  (JNIEnv *, jobject, jstring);

/*
 * Class:     org_lastbamboo_jni_JLibTorrent
 * Method:    remove_torrent
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_org_lastbamboo_jni_JLibTorrent_remove_1torrent
  (JNIEnv *, jobject, jstring);

/*
 * Class:     org_lastbamboo_jni_JLibTorrent
 * Method:    remove_torrent_and_files
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_org_lastbamboo_jni_JLibTorrent_remove_1torrent_1and_1files
  (JNIEnv *, jobject, jstring);

/*
 * Class:     org_lastbamboo_jni_JLibTorrent
 * Method:    pause_torrent
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_org_lastbamboo_jni_JLibTorrent_pause_1torrent
  (JNIEnv *, jobject, jstring);

/*
 * Class:     org_lastbamboo_jni_JLibTorrent
 * Method:    resume_torrent
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_org_lastbamboo_jni_JLibTorrent_resume_1torrent
  (JNIEnv *, jobject, jstring);

/*
 * Class:     org_lastbamboo_jni_JLibTorrent
 * Method:    hard_resume_torrent
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_org_lastbamboo_jni_JLibTorrent_hard_1resume_1torrent
  (JNIEnv *, jobject, jstring);

/*
 * Class:     org_lastbamboo_jni_JLibTorrent
 * Method:    get_max_byte_for_torrent
 * Signature: (Ljava/lang/String;)J
 */
JNIEXPORT jlong JNICALL Java_org_lastbamboo_jni_JLibTorrent_get_1max_1byte_1for_1torrent
  (JNIEnv *, jobject, jstring);

/*
 * Class:     org_lastbamboo_jni_JLibTorrent
 * Method:    start
 * Signature: (ZLjava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_org_lastbamboo_jni_JLibTorrent_start
  (JNIEnv *, jobject, jboolean, jstring);

/*
 * Class:     org_lastbamboo_jni_JLibTorrent
 * Method:    stop
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_lastbamboo_jni_JLibTorrent_stop
  (JNIEnv *, jobject);

/*
 * Class:     org_lastbamboo_jni_JLibTorrent
 * Method:    add_torrent
 * Signature: (Ljava/lang/String;Ljava/lang/String;IZI)J
 */
JNIEXPORT jlong JNICALL Java_org_lastbamboo_jni_JLibTorrent_add_1torrent
  (JNIEnv *, jobject, jstring, jstring, jint, jboolean, jint);

#ifdef __cplusplus
}
#endif
#endif
