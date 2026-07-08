/* unity_entrypoints.h -- UnityPlayer native methods recovered from
 * libunity.so's JNI_OnLoad (ZOOKEEPER DX, Unity 2022.3.62f2, arm64).
 *
 * JNI_OnLoad @ 0x5ddf00 calls 10 RegisterNatives helpers. Helper #1 registers
 * the 26 com/unity3d/player/UnityPlayer natives below; the other 9 register
 * Swappy frame-pacing (ignored -- we drive our own loop) and the stub classes
 * (ARCore/Camera2/HFP/ads/orientation/softinput) we never invoke.
 *
 * These offsets are LINK-TIME addresses for THIS exact libunity.so. If the game
 * is updated, re-run extract_entrypoints.py against the new binary.
 *
 * Runtime address = unity_mod.load_virtbase + offset (the .so links at base 0).
 */
#ifndef UNITY_ENTRYPOINTS_H
#define UNITY_ENTRYPOINTS_H

#include <stdint.h>
#include "so_util.h"

/* ---- UnityPlayer native method offsets (link-time vaddr) ---------------- */
/* drive-critical */
#define OFF_JNI_OnLoad                    0x5ddf00 /* (JavaVM*,reserved)->jint  caches VM via jni::Initialize, registers natives */
#define OFF_initJni                       0x5dd0fc /* (env,thiz,Context)                 */
#define OFF_nativeRecreateGfxState        0x5dd330 /* (env,thiz,int,Surface)  set surface*/
#define OFF_nativeSendSurfaceChangedEvent 0x5dd398 /* (env,thiz)                         */
#define OFF_nativeRender                  0x5dd3f0 /* (env,thiz)->Z   per-frame; false=stop */
#define OFF_nativeInjectEvent             0x5dd450 /* (env,thiz,InputEvent,int)->Z input */
#define OFF_nativePause                   0x5dd198 /* (env,thiz)->Z                      */
#define OFF_nativeResume                  0x5dd1fc /* (env,thiz)                         */
#define OFF_nativeFocusChanged            0x5dd2dc /* (env,thiz,Z)                       */
#define OFF_nativeDone                    0x5dd108 /* (env,thiz)->Z   shutdown           */
#define OFF_nativeApplicationUnload       0x5dd28c /* (env,thiz)                         */
#define OFF_nativeLowMemory               0x5dd244 /* (env,thiz)                         */
#define OFF_nativeOrientationChanged      0x5dde48 /* (env,thiz,int,int)                 */
/* secondary / usually unused for a port */
#define OFF_nativeUnitySendMessage        0x5dda5c /* (env,thiz,String,String,byte[])    */
#define OFF_nativeMuteMasterAudio         0x5ddc6c /* (env,thiz,Z)                       */
#define OFF_nativeGetNoWindowMode         0x5ddea8 /* (env,thiz)->Z                      */
#define OFF_nativeIsAutorotationOn        0x5ddc0c /* (env,thiz)->Z                      */
/* soft keyboard (route via SoftInputProvider stub; not needed for first boot) */
#define OFF_nativeSetInputArea            0x5dd744
#define OFF_nativeSetKeyboardIsVisible    0x5dd7c4
#define OFF_nativeSetInputString          0x5dd81c
#define OFF_nativeSetInputSelection       0x5dd8bc
#define OFF_nativeSoftInputClosed         0x5dda0c
#define OFF_nativeSoftInputCanceled       0x5dd924
#define OFF_nativeSoftInputLostFocus      0x5dd974
#define OFF_nativeReportKeyboardConfigChanged 0x5dd9c4
#define OFF_nativeSendSurfaceChanged      OFF_nativeSendSurfaceChangedEvent
#define OFF_nativeSetLaunchURL            0x5ddcc8
#define OFF_nativeHidePreservedContent    0x5dde00

/* ---- JNI native signatures: ret (*)(JNIEnv*, jobject thiz, args...) ----- */
typedef void     (*fn_initJni)(void*,void*,void*);
typedef void     (*fn_gfxstate)(void*,void*,int32_t,void*);
typedef void     (*fn_v)(void*,void*);
typedef uint8_t  (*fn_z)(void*,void*);
typedef void     (*fn_vz)(void*,void*,int32_t);
typedef uint8_t  (*fn_inject)(void*,void*,void*,int32_t);
typedef void     (*fn_orient)(void*,void*,int32_t,int32_t);

#define UNITY_RESOLVE(mod, off) ((void*)((uintptr_t)(mod).load_virtbase + (off)))

/* ===========================================================================
 * Drive sequence (what the Java UnityPlayer does; you do it in main.c):
 *
 *   initJni(env, thiz, fake_context);                 // early init
 *   nativeRecreateGfxState(env, thiz, 0, fake_surface);// give it the surface
 *   nativeSendSurfaceChangedEvent(env, thiz);          // engine builds GL state
 *   for (;;) {
 *       // input: nativeInjectEvent(env,thiz, motionEvent, deviceId);  // see NOTE
 *       if (!nativeRender(env, thiz)) break;            // false == engine wants out
 *   }
 *   nativeApplicationUnload(env, thiz);  nativeDone(env, thiz);
 *
 * NOTE on input: nativeInjectEvent takes a Java InputEvent/MotionEvent jobject,
 * which the engine then queries back via JNI (getActionMasked/getX/getY/
 * getPointerId/getPointerCount...). So feeding touch needs a fake MotionEvent in
 * jni_fake.c (a stateful handle whose getters return the values you stashed) --
 * the analogous stateful-object trick used for InputStream in unity_jni.c. That
 * MotionEvent shim is the one remaining input piece.
 * =========================================================================== */

#endif /* UNITY_ENTRYPOINTS_H */
