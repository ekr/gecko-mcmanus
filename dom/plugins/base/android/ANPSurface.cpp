/* -*- Mode: IDL; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Android NPAPI support code
 *
 * The Initial Developer of the Original Code is
 * the Mozilla Foundation.
 * Portions created by the Initial Developer are Copyright (C) 2011
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Doug Turner <dougt@mozilla.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include "assert.h"
#include "ANPBase.h"
#include <android/log.h>
#include "AndroidBridge.h"
#include "gfxImageSurface.h"
#include "gfxContext.h"
#include "nsNPAPIPluginInstance.h"

#define LOG(args...)  __android_log_print(ANDROID_LOG_INFO, "GeckoPlugins" , ## args)
#define ASSIGN(obj, name)   (obj)->name = anp_surface_##name


// used to cache JNI method and field IDs for Surface Objects
static struct ANPSurfaceInterfaceJavaGlue {
  bool        initialized;
  jclass geckoAppShellClass;
  jclass surfaceInfoCls;
  jmethodID getSurfaceInfo;
  jfieldID jFormat;
  jfieldID jWidth ;
  jfieldID jHeight;
} gSurfaceJavaGlue;

#define getClassGlobalRef(env, cname)                                    \
     (jClass = jclass(env->NewGlobalRef(env->FindClass(cname))))

static void init(JNIEnv* env) {
  if (gSurfaceJavaGlue.initialized)
    return;
  
  gSurfaceJavaGlue.geckoAppShellClass = mozilla::AndroidBridge::GetGeckoAppShellClass();
  
  jmethodID getClass = env->GetStaticMethodID(gSurfaceJavaGlue.geckoAppShellClass, 
                                              "getSurfaceInfoClass",
                                              "()Ljava/lang/Class;");

  gSurfaceJavaGlue.surfaceInfoCls = (jclass) env->NewGlobalRef(env->CallStaticObjectMethod(gSurfaceJavaGlue.geckoAppShellClass, getClass));

  gSurfaceJavaGlue.jFormat = env->GetFieldID(gSurfaceJavaGlue.surfaceInfoCls, "format", "I");
  gSurfaceJavaGlue.jWidth = env->GetFieldID(gSurfaceJavaGlue.surfaceInfoCls, "width", "I");
  gSurfaceJavaGlue.jHeight = env->GetFieldID(gSurfaceJavaGlue.surfaceInfoCls, "height", "I");

  gSurfaceJavaGlue.getSurfaceInfo = env->GetStaticMethodID(gSurfaceJavaGlue.geckoAppShellClass, "getSurfaceInfo", "(Landroid/view/SurfaceView;)Lorg/mozilla/gecko/SurfaceInfo;");
  gSurfaceJavaGlue.initialized = true;
}

static bool anp_lock(JNIEnv* env, jobject surfaceView, ANPBitmap* bitmap, ANPRectI* dirtyRect) {
  LOG("%s", __PRETTY_FUNCTION__);
  if (!bitmap || !surfaceView) {
    LOG("%s, null bitmap or surface, exiting", __PRETTY_FUNCTION__);
    return false;
  }

  init(env);

  jobject info = env->CallStaticObjectMethod(gSurfaceJavaGlue.geckoAppShellClass,
                                             gSurfaceJavaGlue.getSurfaceInfo, surfaceView);

  LOG("info: %p", info);
  if (!info)
    return false;

  bitmap->width  = env->GetIntField(info, gSurfaceJavaGlue.jWidth);
  bitmap->height = env->GetIntField(info, gSurfaceJavaGlue.jHeight);

  if (bitmap->width <= 0 || bitmap->height <= 0)
    return false;

  int format = env->GetIntField(info, gSurfaceJavaGlue.jFormat);
  gfxImageFormat targetFormat;

  // format is PixelFormat
  if (format & 0x00000001) {
    /*
    bitmap->format = kRGBA_8888_ANPBitmapFormat;
    bitmap->rowBytes = bitmap->width * 4;
    targetFormat = gfxASurface::ImageFormatARGB32;
    */
    
    // We actually can't handle this right now because gfxImageSurface
    // doesn't support RGBA32.
    LOG("Unable to handle 32bit pixel format");
    return false;
  } else if (format & 0x00000004) {
    bitmap->format = kRGB_565_ANPBitmapFormat;
    bitmap->rowBytes = bitmap->width * 2;
    targetFormat = gfxASurface::ImageFormatRGB16_565;
  } else {
    LOG("format from glue is unknown %d\n", format);
    return false;
  }

  nsNPAPIPluginInstance* pinst = nsNPAPIPluginInstance::FindByJavaSurface((void*)surfaceView);
  if (!pinst) {
    LOG("Failed to get plugin instance");
    return false;
  }

  NPRect lockRect;
  if (dirtyRect) {
    lockRect.top = dirtyRect->top;
    lockRect.left = dirtyRect->left;
    lockRect.right = dirtyRect->right;
    lockRect.bottom = dirtyRect->bottom;
  } else {
    // No dirty rect, use the whole bitmap
    lockRect.top = lockRect.left = 0;
    lockRect.right = bitmap->width;
    lockRect.bottom = bitmap->height;
  }
  
  gfxImageSurface* target = pinst->LockTargetSurface(bitmap->width, bitmap->height, targetFormat, &lockRect);
  bitmap->baseAddr = target->Data();

  env->DeleteLocalRef(info);

  return true;
}

static void anp_unlock(JNIEnv* env, jobject surfaceView) {
  LOG("%s", __PRETTY_FUNCTION__);

  if (!surfaceView) {
    LOG("null surface, exiting %s", __PRETTY_FUNCTION__);
    return;
  }

  nsNPAPIPluginInstance* pinst = nsNPAPIPluginInstance::FindByJavaSurface((void*)surfaceView);
  if (!pinst) {
    LOG("Could not find plugin instance!");
    return;
  }
  
  pinst->UnlockTargetSurface(true /* invalidate the locked area */);
}

///////////////////////////////////////////////////////////////////////////////

#define ASSIGN(obj, name)   (obj)->name = anp_##name

void InitSurfaceInterface(ANPSurfaceInterfaceV0 *i) {

  ASSIGN(i, lock);
  ASSIGN(i, unlock);

  // setup the java glue struct
  gSurfaceJavaGlue.initialized = false;
}
