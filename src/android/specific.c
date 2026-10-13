/*****************************************************************************
 * specific.c: stubs for Android OS-specific initialization
 *****************************************************************************
 * Copyright © 2016 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <assert.h>
#include <errno.h>

#include <heimdall_common.h>
#include <heimdall_network.h>
#include <heimdall_fs.h>
#include "../libheimdall.h"
#include "../config/configuration.h"

#include <string.h>
#include <jni.h>

static JavaVM *s_jvm = NULL;
#define GENERIC_DIR_COUNT (HEIMDALL_SNAPSHOTS_DIR - HEIMDALL_DESKTOP_DIR + 1)
static char *ppsz_generic_names[GENERIC_DIR_COUNT] = {};
static struct {
    struct {
        jclass clazz;
        jmethodID getExternalStoragePublicDirectory;
    } Environment;
    struct {
        jmethodID getAbsolutePath;
    } File;
    struct {
        jclass clazz;
        jmethodID getProperty;
    } System;
} fields = { .Environment.clazz = NULL };

static char *
get_java_string(JNIEnv *env, jclass clazz, const char *psz_name)
{
    jfieldID id = (*env)->GetStaticFieldID(env, clazz, psz_name,
                                           "Ljava/lang/String;");
    if ((*env)->ExceptionCheck(env))
    {
        (*env)->ExceptionClear(env);
        return NULL;
    }

    jstring jstr = (*env)->GetStaticObjectField(env, clazz, id);

    const char *psz_str = (*env)->GetStringUTFChars(env, jstr, 0);
    if (psz_str == NULL)
        return NULL;

    char *psz_strdup = strdup(psz_str);

    (*env)->ReleaseStringUTFChars(env, jstr, psz_str);
    (*env)->DeleteLocalRef(env, jstr);

    return psz_strdup;
}

void
JNI_OnUnload(JavaVM* vm, void* reserved)
{
    (void) reserved;

    for (size_t i = 0; i < GENERIC_DIR_COUNT; ++i)
        free(ppsz_generic_names[i]);

    JNIEnv* env = NULL;
    if ((*vm)->GetEnv(vm, (void**) &env, JNI_VERSION_1_2) != JNI_OK)
        return;

    if (fields.Environment.clazz)
        (*env)->DeleteGlobalRef(env, fields.Environment.clazz);

    if (fields.System.clazz)
        (*env)->DeleteGlobalRef(env, fields.System.clazz);
}

/* This function is called when the libheimdallore dynamic library is loaded via the
 * java.lang.System.loadLibrary method. Therefore, s_jvm will be already set
 * when libheimdall_InternalInit is called. */
jint
JNI_OnLoad(JavaVM *vm, void *reserved)
{
    s_jvm = vm;
    JNIEnv* env = NULL;

    if ((*vm)->GetEnv(vm, (void**) &env, JNI_VERSION_1_2) != JNI_OK)
        return -1;

    jclass clazz = (*env)->FindClass(env, "android/os/Environment");
    if ((*env)->ExceptionCheck(env))
        return -1;

    static const char *ppsz_env_names[GENERIC_DIR_COUNT] = {
        NULL,                   /* HEIMDALL_DESKTOP_DIR */
        "DIRECTORY_DOWNLOADS",  /* HEIMDALL_DOWNLOAD_DIR */
        NULL,                   /* HEIMDALL_TEMPLATES_DIR */
        NULL,                   /* HEIMDALL_PUBLICSHARE_DIR */
        "DIRECTORY_DOCUMENTS",  /* HEIMDALL_DOCUMENTS_DIR */
        "DIRECTORY_MUSIC",      /* HEIMDALL_MUSIC_DIR */
        "DIRECTORY_PICTURES",   /* HEIMDALL_PICTURES_DIR */
        "DIRECTORY_MOVIES",     /* HEIMDALL_VIDEOS_DIR */
        "DIRECTORY_SCREENSHOTS",/* HEIMDALL_SNAPSHOTS_DIR */
    };
    for (size_t i = 0; i < GENERIC_DIR_COUNT; ++i)
    {
        if (ppsz_env_names[i] != NULL)
            ppsz_generic_names[i] = get_java_string(env, clazz,
                                                   ppsz_env_names[i]);
    }

    fields.Environment.clazz = (*env)->NewGlobalRef(env, clazz);
    fields.Environment.getExternalStoragePublicDirectory =
        (*env)->GetStaticMethodID(env, clazz,
                                  "getExternalStoragePublicDirectory",
                                  "(Ljava/lang/String;)Ljava/io/File;");
    if ((*env)->ExceptionCheck(env))
        goto error;
    (*env)->DeleteLocalRef(env, clazz);

    clazz = (*env)->FindClass(env, "java/io/File");
    fields.File.getAbsolutePath =
        (*env)->GetMethodID(env, clazz, "getAbsolutePath",
                           "()Ljava/lang/String;");
    if ((*env)->ExceptionCheck(env))
        goto error;
    (*env)->DeleteLocalRef(env, clazz);

    clazz = (*env)->FindClass(env, "java/lang/System");
    if ((*env)->ExceptionCheck(env))
        goto error;
    fields.System.clazz = (*env)->NewGlobalRef(env, clazz);
    fields.System.getProperty =
        (*env)->GetStaticMethodID(env, clazz, "getProperty",
                                  "(Ljava/lang/String;)Ljava/lang/String;");
    (*env)->DeleteLocalRef(env, clazz);

    return JNI_VERSION_1_2;

error:
    if (clazz)
        (*env)->DeleteLocalRef(env, clazz);
    JNI_OnUnload(vm, reserved);
    return -1;
}

void
system_Init(void)
{
}

void
system_Configure(libheimdall_int_t *p_libheimdall, int i_argc, const char *const pp_argv[])
{
    (void)i_argc; (void)pp_argv;
    assert(s_jvm != NULL);
    var_Create(p_libheimdall, "android-jvm", HEIMDALL_VAR_ADDRESS);
    var_SetAddress(p_libheimdall, "android-jvm", s_jvm);
}

static char *config_GetHomeDir(const char *psz_dir, const char *psz_default_dir)
{
    char *psz_home = getenv("HOME");
    if (psz_home == NULL)
        goto fallback;

    if (psz_dir == NULL)
        return strdup(psz_home);

    char *psz_fullpath;
    if (asprintf(&psz_fullpath, "%s/%s", psz_home, psz_dir) == -1)
        goto fallback;
    if (heimdall_mkdir(psz_fullpath, 0700) == -1 && errno != EEXIST)
    {
        free(psz_fullpath);
        goto fallback;
    }
    return psz_fullpath;

fallback:
    return psz_default_dir != NULL ? strdup(psz_default_dir) : NULL;
}

static JNIEnv *get_env(bool *p_detach)
{
    JNIEnv *env;
    if ((*s_jvm)->GetEnv(s_jvm, (void **)&env, JNI_VERSION_1_2) != JNI_OK)
    {
        /* attach the thread to the Java VM */
        JavaVMAttachArgs args;

        args.version = JNI_VERSION_1_2;
        args.name = "config_GetGenericDir";
        args.group = NULL;

        if ((*s_jvm)->AttachCurrentThread(s_jvm, &env, &args) != JNI_OK)
            return NULL;
        *p_detach = true;
    }
    else
        *p_detach = false;
    return env;
}

static void release_env(bool b_detach)
{
    if (b_detach)
        (*s_jvm)->DetachCurrentThread(s_jvm);
}

static char *config_GetGenericDir(const char *psz_name)
{
    JNIEnv *env;
    bool b_detach;
    char *psz_ret = NULL;

    env = get_env(&b_detach);
    if (env == NULL)
        return NULL;

    jstring jname= (*env)->NewStringUTF(env, psz_name);
    if ((*env)->ExceptionCheck(env))
    {
        (*env)->ExceptionClear(env);
        jname = NULL;
    }
    if (jname == NULL)
        goto error;

    jobject jfile = (*env)->CallStaticObjectMethod(env,
                        fields.Environment.clazz,
                        fields.Environment.getExternalStoragePublicDirectory,
                        jname);
    (*env)->DeleteLocalRef(env, jname);
    if (jfile == NULL)
        goto error;

    jstring jpath = (*env)->CallObjectMethod(env, jfile,
                                             fields.File.getAbsolutePath);
    (*env)->DeleteLocalRef(env, jfile);

    const char *psz_path = (*env)->GetStringUTFChars(env, jpath, 0);
    if (psz_path == NULL)
        goto error;
    psz_ret = strdup(psz_path);
    (*env)->ReleaseStringUTFChars(env, jpath, psz_path);
    (*env)->DeleteLocalRef(env, jpath);

error:
    release_env(b_detach);
    return psz_ret;
}

char *platform_GetUserDir (heimdall_userdir_t type)
{
    switch (type)
    {
        case HEIMDALL_USERDATA_DIR:
            return config_GetHomeDir(".share",
                "/sdcard/Android/data/com.lycaria.heimdall");
        case HEIMDALL_CACHE_DIR:
            return config_GetHomeDir(".cache",
                "/sdcard/Android/data/com.lycaria.heimdall/cache");
        case HEIMDALL_HOME_DIR:
            return config_GetHomeDir(NULL, NULL);
        case HEIMDALL_CONFIG_DIR:
            return config_GetHomeDir(".config", NULL);

        case HEIMDALL_DESKTOP_DIR:
        case HEIMDALL_DOWNLOAD_DIR:
        case HEIMDALL_TEMPLATES_DIR:
        case HEIMDALL_PUBLICSHARE_DIR:
        case HEIMDALL_DOCUMENTS_DIR:
        case HEIMDALL_MUSIC_DIR:
        case HEIMDALL_SNAPSHOTS_DIR:
        case HEIMDALL_PICTURES_DIR:
        case HEIMDALL_VIDEOS_DIR:
        {
            assert(type >= HEIMDALL_DESKTOP_DIR && type <= HEIMDALL_VIDEOS_DIR);
            const char *psz_name = ppsz_generic_names[type - HEIMDALL_DESKTOP_DIR];
            if (psz_name != NULL)
                return config_GetGenericDir(psz_name);
        }
    }
    return NULL;
}

char *config_GetSysPath(heimdall_sysdir_t type, const char *filename)
{
    char *dir = NULL;

    switch (type)
    {
        case HEIMDALL_SYSDATA_DIR:
            dir = strdup("/system/usr/share");
            break;
        case HEIMDALL_LIB_DIR:
            dir = config_GetLibDir();
            break;
        default:
            break;
    }

    if (filename == NULL || dir == NULL)
        return dir;

    char *path;
    if (unlikely(asprintf(&path, "%s/%s", dir, filename) == -1))
        path = NULL;
    free(dir);
    return path;
}

char *heimdall_getProxyUrl(const char *url)
{
    HEIMDALL_UNUSED(url);

    /**
     * Determines the network proxy server to use (if any).
     *
     * This function fetch the Android proxy using the System.getProperty()
     * method with "http.proxyHost" and "http.proxyPort" keys. This is working
     * only for Android 4.0 and after.
     */

    JNIEnv *env;
    bool b_detach;
    char *psz_ret = NULL;
    const char *psz_host = NULL, *psz_port = NULL;
    jstring jhost = NULL, jport = NULL;

    env = get_env(&b_detach);
    if (env == NULL)
        return NULL;

    /* Fetch "http.proxyHost" property */
    jstring jkey = (*env)->NewStringUTF(env, "http.proxyHost");
    if ((*env)->ExceptionCheck(env))
    {
        (*env)->ExceptionClear(env);
        jkey = NULL;
    }
    if (jkey == NULL)
        goto end;

    jhost = (*env)->CallStaticObjectMethod(env, fields.System.clazz,
                                           fields.System.getProperty, jkey);
    (*env)->DeleteLocalRef(env, jkey);
    if (jhost == NULL)
        goto end;

    psz_host = (*env)->GetStringUTFChars(env, jhost, 0);
    /* Ensure the property is valid */
    if (psz_host == NULL || psz_host[0] == '\0')
        goto end;

    /* Fetch "http.proxyPort" property (only if "http.proxyHost" is valid) */
    jkey = (*env)->NewStringUTF(env, "http.proxyPort");
    if ((*env)->ExceptionCheck(env))
    {
        (*env)->ExceptionClear(env);
        jkey = NULL;
    }
    if (jkey == NULL)
        goto end;

    jport = (*env)->CallStaticObjectMethod(env, fields.System.clazz,
                                           fields.System.getProperty, jkey);
    (*env)->DeleteLocalRef(env, jkey);

    /* Ensure the property is valid */
    if (jport != NULL)
    {
        psz_port = (*env)->GetStringUTFChars(env, jport, 0);
        if (psz_port != NULL && (psz_port[0] == '\0' || psz_port[0] == '0'))
        {
            (*env)->ReleaseStringUTFChars(env, jport, psz_port);
            psz_port = NULL;
        }
    }

    /* Concat "http://" "http.proxyHost" and "http.proxyPort" */
    if (asprintf(&psz_ret, "http://%s%s%s",
                 psz_host,
                 psz_port != NULL ? ":" : "",
                 psz_port != NULL ? psz_port : "") == -1)
        psz_ret = NULL;

end:
    if (psz_host != NULL)
        (*env)->ReleaseStringUTFChars(env, jhost, psz_host);
    if (jhost != NULL)
        (*env)->DeleteLocalRef(env, jhost);
    if (psz_port != NULL)
        (*env)->ReleaseStringUTFChars(env, jport, psz_port);
    if (jport != NULL)
        (*env)->DeleteLocalRef(env, jport);
    release_env(b_detach);
    return psz_ret;
}
