/* android_native_unity.c -- the 27 NDK symbols libunity.so imports, for the
 * ZOOKEEPER DX Switch port. Unity is NOT a NativeActivity, so unlike cr3_nx's
 * android_native.c there is no ANativeActivity glue / android_main / AInputQueue
 * here: the engine is driven by the JNI-registered natives (see main.c). We only
 * provide the raw NDK functions libunity calls directly:
 *
 *   ANativeWindow_acquire/_release/_fromSurface/_setBuffersGeometry/
 *                _getWidth/_getHeight/_getFormat      -> libnx NWindow
 *   ALooper_prepare/_acquire/_release/_pollOnce/_wake/_forThread
 *                                                     -> condvar wait/wake
 *   ASensorManager_ , ASensorEventQueue_ , ASensor_   -> "no sensors"
 *
 * IMPORTANT context-ownership note: the engine creates its OWN EGL context from
 * the ANativeWindow (cr3_nx's main.c creates none). The host must NOT create an
 * SDL_GL / EGL context. Use SDL for audio + HID only. Delete the
 * SDL_GL_SetAttribute/SDL_GL_CreateContext/SDL_GL_SwapWindow calls from the
 * earlier main_skeleton.c; the engine calls eglSwapBuffers itself.
 *
 * Needs devkitA64 + libnx (switch.h) + switch-mesa. Not host-compilable.
 */

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <switch.h>
#include <GLES3/gl3.h> /* docked cursor overlay */
#include "util.h"   /* debugPrintf */
#include "config.h" /* config.portrait */

#ifndef AWINDOW_FORMAT_RGBA_8888
#define AWINDOW_FORMAT_RGBA_8888 1
#endif

/* opaque NDK types -> concrete libnx instances */
typedef struct ANativeWindow ANativeWindow;     /* == NWindow* at runtime */
typedef struct ALooper       ALooper;

/* ==========================================================================
 * dock-aware screen state (also read by unity_jni.c's Display getters)
 * ========================================================================== */
static u32 g_w = 720, g_h = 1280;   /* fbstub45 PORTRAIT (stable) */
/* The resolution Unity BELIEVES it has = the last size it requested via
 * setBuffersGeometry (its SetResolution 640x1137). We reject the real NWindow
 * resize (kept at g_w x g_h) but Unity maps ALL input into this believed space,
 * so injected touch/cursor coords must be SCALED from framebuffer space
 * (g_w x g_h) into it -- otherwise the right/bottom ~11% lands beyond Unity's
 * screen and is dead (the "can't tap the far right" bug, on touch AND cursor).
 * 0 => not set yet, use g_w/g_h. */
static int g_input_w = 0, g_input_h = 0;
static inline float nxp_scale_x(void){ return (float)(g_input_w > 0 ? g_input_w : (int)g_w) / (float)g_w; }
static inline float nxp_scale_y(void){ return (float)(g_input_h > 0 ? g_input_h : (int)g_h) / (float)g_h; }

void android_native_update_mode(void){
  if (appletGetOperationMode() == AppletOperationMode_Console) { g_w = 1080; g_h = 1920; }
  else                                                         { g_w = 720;  g_h = 1280; }
}
u32 android_native_width(void)  { return g_w; }
u32 android_native_height(void) { return g_h; }

/* ==========================================================================
 * ANativeWindow  ->  libnx NWindow
 * ========================================================================== */
/* fbstub45: pin the displayed region to exactly the dimensions Unity renders
 * into. nwindowSetDimensions may allocate a width-aligned (e.g. 720 -> 768)
 * swapchain buffer; without a matching crop the compositor can scan the extra
 * uninitialized columns, which shows up as the image being "cut off" / garbage
 * on the right edge. Cropping to (0,0,bw,bh) guarantees only the rendered
 * content is presented (stretched to the panel, no cutoff). */
/* TATE-mode rotation, ported from the VLN reference port: the portrait render
 * is rotated by the COMPOSITOR onto the landscape panel via the BufferQueue
 * per-buffer transform flag (nwindowSetTransform -> bqinput.transform on every
 * queue). Producer-side only -- needs none of the vi:m layer permissions that
 * blocked the pillarbox route (rc=0xdc01, handoff §6.2). Correct aspect, zero
 * GPU cost; the player holds the console rotated. config.portrait: 1 = ROT_90
 * CW (default, right Joy-Con up), 2 = ROT_270 CCW (left Joy-Con up),
 * 0 = none (previous stretched-16:9 behavior). Touch + stick cursor are
 * remapped through the same rotation in android_native_feed_hid. */
#ifndef NATIVE_WINDOW_TRANSFORM_ROT_90
#define NATIVE_WINDOW_TRANSFORM_ROT_90  0x4   /* HAL_TRANSFORM_ROT_90  */
#endif
#ifndef NATIVE_WINDOW_TRANSFORM_ROT_270
#define NATIVE_WINDOW_TRANSFORM_ROT_270 0x7   /* ROT_90 | ROT_180      */
#endif
static u32 nx_portrait_transform(void) {
  if (config.portrait == 2) return (u32)NATIVE_WINDOW_TRANSFORM_ROT_270;
  if (config.portrait == 1) return (u32)NATIVE_WINDOW_TRANSFORM_ROT_90;
  return 0;
}

static void nx_window_set_geom(NWindow *w, u32 bw, u32 bh) {
  Result rc = nwindowSetDimensions(w, bw, bh);
  nwindowSetCrop(w, 0, 0, bw, bh);
  nwindowSetTransform(w, nx_portrait_transform());
  u32 aw = 0, ah = 0;
  nwindowGetDimensions(w, &aw, &ah);
  debugPrintf("[gfx] window geom: requested %ux%u (rc=0x%x), nwindow reports %ux%u, crop 0,0,%u,%u, portrait=%d (1=ROT90 2=ROT270 0=none)\n",
              bw, bh, rc, aw, ah, bw, bh, config.portrait);
}

ANativeWindow *android_native_window(void){
  NWindow *w = nwindowGetDefault();
  nx_window_set_geom(w, g_w, g_h);
  return (ANativeWindow *)w;
}
void     ANativeWindow_acquire(ANativeWindow *w){ (void)w; }                 /* singleton: refcount no-op */
void     ANativeWindow_release(ANativeWindow *w){ (void)w; }
ANativeWindow *ANativeWindow_fromSurface(void *env, void *surface){
  (void)env; (void)surface; return android_native_window();               /* one surface == our window */
}
int32_t  ANativeWindow_getWidth (ANativeWindow *w){ (void)w; return (int32_t)g_w; }
int32_t  ANativeWindow_getHeight(ANativeWindow *w){ (void)w; return (int32_t)g_h; }
int32_t  ANativeWindow_getFormat(ANativeWindow *w){ (void)w; return AWINDOW_FORMAT_RGBA_8888; }
int32_t  ANativeWindow_setBuffersGeometry(ANativeWindow *w, int32_t width, int32_t height, int32_t format){
  (void)format;
  /* The NX window is a FIXED-SIZE display. Resizing the real window to a
   * non-native size (e.g. the game's saved 640x1137 low-res, applied EARLY at
   * startup when Unity knows it from PlayerPrefs) makes mesa build a 640x1137
   * swapchain whose buffers the NX display path never consumes -> the first
   * eglSwapBuffers blocks forever (the boot-2 hang). Android devices without
   * hardware resolution scaling behave exactly like this fix: the resize
   * "succeeds" (returns 0) but readback (getWidth/getHeight, eglQuerySurface)
   * still shows the native size, which is precisely how Unity detects
   * "Hardware resolution scaling not supported" and falls back to its software
   * blit -- the same path that already works when SetResolution happens late
   * at frame 2. So: accept only the native geometry; report success for the
   * rest so Unity's own fallback engages. */
  if (width > 0 && height > 0) {
    g_input_w = width; g_input_h = height;   /* Unity's believed screen == its input space */
    if ((u32)width != g_w || (u32)height != g_h) {
      debugPrintf("[gfx] setBuffersGeometry %dx%d REJECTED (fixed-size window stays %ux%u; engine will blit-scale)\n",
                  width, height, g_w, g_h);
      return 0;
    }
    nx_window_set_geom((NWindow *)w, (u32)width, (u32)height);
  }
  return 0;
}

/* ==========================================================================
 * ALooper -- Unity uses it as a per-thread wait/wake primitive (not real fd
 * polling), so a condvar-backed looper is sufficient. If the engine turns out
 * to register real fds, port cr3_nx's fake-fd PollItem layer in here.
 * ========================================================================== */
#define ALOOPER_POLL_WAKE     (-1)
#define ALOOPER_POLL_TIMEOUT  (-3)
#define MAX_LOOPERS 16

struct ALooper { Mutex m; CondVar cv; int signaled; int refs; u32 owner; int used; };
static struct ALooper g_loopers[MAX_LOOPERS];
static Mutex g_loopers_lock;
static int   g_loopers_init = 0;

static void loopers_once(void){ if(!g_loopers_init){ mutexInit(&g_loopers_lock); g_loopers_init=1; } }

static struct ALooper *looper_for(u32 tid, int create){
  loopers_once();
  mutexLock(&g_loopers_lock);
  for (int i=0;i<MAX_LOOPERS;i++) if (g_loopers[i].used && g_loopers[i].owner==tid){
    struct ALooper *l=&g_loopers[i]; mutexUnlock(&g_loopers_lock); return l; }
  if (create) for (int i=0;i<MAX_LOOPERS;i++) if (!g_loopers[i].used){
    struct ALooper *l=&g_loopers[i];
    l->used=1; l->owner=tid; l->signaled=0; l->refs=1;
    mutexInit(&l->m); condvarInit(&l->cv);
    mutexUnlock(&g_loopers_lock); return l; }
  mutexUnlock(&g_loopers_lock);
  return NULL;
}
static u32 cur_tid(void){ return (u32)(uintptr_t)threadGetCurHandle(); }

ALooper *ALooper_prepare(int opts){ (void)opts; return (ALooper *)looper_for(cur_tid(), 1); }
ALooper *ALooper_forThread(void){  return (ALooper *)looper_for(cur_tid(), 0); }
void     ALooper_acquire(ALooper *l){ struct ALooper *L=(void*)l; if(L){ mutexLock(&L->m); L->refs++; mutexUnlock(&L->m);} }
void     ALooper_release(ALooper *l){ struct ALooper *L=(void*)l; if(L){ mutexLock(&L->m); if(--L->refs<=0) L->used=0; mutexUnlock(&L->m);} }

void ALooper_wake(ALooper *l){
  struct ALooper *L=(void*)l; if(!L) return;
  mutexLock(&L->m); L->signaled=1; condvarWakeAll(&L->cv); mutexUnlock(&L->m);
}
int ALooper_pollOnce(int timeoutMillis, int *outFd, int *outEvents, void **outData){
  struct ALooper *L = (void*)looper_for(cur_tid(), 1);
  if (outFd) *outFd=0;
  if (outEvents) *outEvents=0;
  if (outData) *outData=NULL;
  mutexLock(&L->m);
  if (!L->signaled){
    if (timeoutMillis==0){ mutexUnlock(&L->m); return ALOOPER_POLL_TIMEOUT; }
    if (timeoutMillis<0)  condvarWait(&L->cv,&L->m);
    else condvarWaitTimeout(&L->cv,&L->m,(u64)timeoutMillis*1000000ull);
  }
  int was = L->signaled; L->signaled=0;
  mutexUnlock(&L->m);
  return was ? ALOOPER_POLL_WAKE : ALOOPER_POLL_TIMEOUT;
}
/* Unity rarely uses these two, but provide them for completeness. */
int ALooper_addFd(ALooper *l,int fd,int ident,int events,void *cb,void *data){
  (void)l;(void)fd;(void)ident;(void)events;(void)cb;(void)data; return 1; }
int ALooper_removeFd(ALooper *l,int fd){ (void)l;(void)fd; return 1; }

/* ==========================================================================
 * Sensors -- report none. (CR3 imported no ASensorManager; Unity does, so these
 * must exist and return a clean empty state rather than be missing symbols.)
 * ========================================================================== */
void *ASensorManager_getInstance(void){ static int x; return &x; }
void *ASensorManager_getInstanceForPackage(const char *p){ (void)p; return ASensorManager_getInstance(); }
int   ASensorManager_getSensorList(void *m, void **list){ (void)m; if(list)*list=NULL; return 0; }
void *ASensorManager_getDefaultSensor(void *m, int type){ (void)m;(void)type; return NULL; }
void *ASensorManager_createEventQueue(void *m, void *looper, int ident, void *cb, void *data){
  (void)m;(void)looper;(void)ident;(void)cb;(void)data; static int q; return &q; }
int   ASensorManager_destroyEventQueue(void *m, void *q){ (void)m;(void)q; return 0; }

int   ASensorEventQueue_enableSensor (void *q, const void *s){ (void)q;(void)s; return -1; }
int   ASensorEventQueue_disableSensor(void *q, const void *s){ (void)q;(void)s; return 0; }
int   ASensorEventQueue_setEventRate (void *q, const void *s, int32_t us){ (void)q;(void)s;(void)us; return 0; }
int   ASensorEventQueue_getEvents    (void *q, void *ev, size_t n){ (void)q;(void)ev;(void)n; return 0; }
int   ASensorEventQueue_hasEvents    (void *q){ (void)q; return 0; }

const char *ASensor_getName      (const void *s){ (void)s; return ""; }
const char *ASensor_getVendor    (const void *s){ (void)s; return ""; }
int         ASensor_getType      (const void *s){ (void)s; return 0; }
float       ASensor_getResolution(const void *s){ (void)s; return 0.0f; }
int         ASensor_getMinDelay  (const void *s){ (void)s; return 0; }

/* cr3 dead-handler stub: no orientation sensor -> report level. */
void android_get_orientation(float *x, float *y, float *z){
  if (x) *x = 0.0f;
  if (y) *y = 0.0f;
  if (z) *z = 0.0f;
}

/* ==========================================================================
 * HID polling -> Unity input.
 * Unity ingests input through the Java UnityPlayer (touch -> nativeInjectEvent /
 * key path). The exact native event struct is engine-internal: recover the
 * registered "injectEvent"/"nativePointer*" method from libunity's JNI_OnLoad
 * (PORTING_PLAN.md S4) and fill in feed_one_touch(). Until then this reads HID
 * but doesn't yet hand it to the engine.
 * ========================================================================== */
/* ==========================================================================
 * HID -> Unity input via nativeInjectEvent.
 * Handheld: touchscreen passes through. Docked (no touch): left stick drives a
 * virtual cursor, A = tap. Builds a fake MotionEvent (unity_input.c) and calls
 * the recovered nativeInjectEvent(env, thiz, event, deviceId).
 * ========================================================================== */
#include "unity_input.h"
#include "nx_pointer.h"
#include "config.h"

extern FILE *fopen_fake(const char *path, const char *mode);
extern int   fclose_fake(FILE *f);

static PadState g_pad;
static HidTouchScreenState g_touch;
static int   g_prev_touch = 0;        /* pointers down last frame */
static float g_last_tx = 360, g_last_ty = 640;  /* last handheld touch (game space) for UP */

void android_native_input_init(void){
  padConfigureInput(1, HidNpadStyleSet_NpadStandard);
  padInitializeDefault(&g_pad);
  hidInitializeTouchScreen();
  /* input_log_fn intentionally left unset: the MotionEvent-getter trace is a
   * debug aid (set it to debugPrintf to re-enable). Leaving it off keeps the
   * touch path log-silent so taps don't stutter on slow SD writes. */
}

/* inject signature == recovered nativeInjectEvent: (env,thiz,InputEvent,int)->Z */
typedef uint8_t (*inject_fn)(void*,void*,void*,int);

static void nxp_ensure_init(void){
  static int done = 0;
  if (done) return;
  done = 1;
  NxpConfig c = {0};
  c.screen_w = (int)g_w; c.screen_h = (int)g_h;   /* render (portrait) space   */
  c.panel_w  = 1280;     c.panel_h  = 720;         /* Switch touch panel        */
  c.data_dir = "sdmc:/switch/zookeeper";           /* cursor.png / pointer.cfg  */
  c.rotation = config.portrait;                    /* 1 CW / 2 CCW / 0 none     */
  c.handle_touch = 0;                              /* host keeps its own touch  */
  c.cursor_id = 0; c.max_touch_slots = UI_MAX_POINTERS;
  c.fopen_fn = fopen_fake; c.fclose_fn = fclose_fake;
  debugPrintf("[nxp] init: cursor screen=%dx%d (g_w=%u g_h=%u) panel=%dx%d rot=%d\n",
              c.screen_w, c.screen_h, g_w, g_h, c.panel_w, c.panel_h, c.rotation);
  nxp_init(&c);
}

void android_native_feed_hid(inject_fn inject, void *env, void *thiz){
  nxp_ensure_init();
  padUpdate(&g_pad);

  /* ---- handheld touchscreen ---- */
  int n = hidGetTouchScreenStates(&g_touch, 1);
  if (n > 0 && g_touch.count > 0){
    int   ids[UI_MAX_POINTERS]; float xs[UI_MAX_POINTERS]; float ys[UI_MAX_POINTERS];
    int c = g_touch.count > UI_MAX_POINTERS ? UI_MAX_POINTERS : g_touch.count;
    /* The Switch panel reports touches in its native 1280x720 landscape space;
     * the render is rotated onto it per config.portrait (see nx_window_set_geom).
     * Undo that same rotation so a tap lands where the player sees it. */
    const float PANEL_W = 1280.0f, PANEL_H = 720.0f;
    for (int i=0;i<c;i++){ ids[i]=(int)g_touch.touches[i].finger_id;
      float px=(float)g_touch.touches[i].x, py=(float)g_touch.touches[i].y;
      if (config.portrait == 2) {          /* ROT_270 (CCW): game(PANEL_H-py, px)  */
        xs[i]= (PANEL_H-py) * ((float)g_w / PANEL_H);
        ys[i]=  px          * ((float)g_h / PANEL_W);
      } else if (config.portrait == 1) {   /* ROT_90 (CW): game(py, PANEL_W-px)    */
        xs[i]=  py          * ((float)g_w / PANEL_H);
        ys[i]= (PANEL_W-px) * ((float)g_h / PANEL_W);
      } else {                             /* no rotation: stretched full-panel map */
        xs[i]=  px * ((float)g_w / PANEL_W);
        ys[i]=  py * ((float)g_h / PANEL_H);
      } }
    g_last_tx = xs[0]; g_last_ty = ys[0];               /* remember for the UP */
    int action = g_prev_touch ? AMOTION_ACTION_MOVE : AMOTION_ACTION_DOWN;
    inject(env, thiz, unity_motionevent(action, c, ids, xs, ys), 0);
    g_prev_touch = c;
    return;
  }
  if (g_prev_touch){                                  /* released -> UP at last pos */
    int   ids[1]={0}; float xs[1]={g_last_tx}, ys[1]={g_last_ty};
    inject(env, thiz, unity_motionevent(AMOTION_ACTION_UP, 1, ids, xs, ys), 0);
    g_prev_touch = 0;
    return;
  }

  /* ---- cursor: stick / USB mouse / gyro, via nx_pointer (rotation-aware) ----
   * nx_pointer reads its own pad + mouse + sixaxis, handles +/- toggles, L/R
   * recenter, D-pad sensitivity, cursor.png and pointer.cfg. It emits pointer
   * events already in render space; forward each as a single-pointer motion
   * event. (Touch above is handled by the host and returns early, so this runs
   * only when not touching.) */
  nxp_update();
  NxpEvent pev[8];
  int pn = nxp_poll(pev, 8);
  for (int i = 0; i < pn; i++){
    int   ids[1] = { 0 };                               /* same touch slot as finger 0 -> taps register */
    float xs[1]  = { pev[i].x }, ys[1] = { pev[i].y };
    int action = pev[i].phase == NXP_DOWN ? AMOTION_ACTION_DOWN
               : pev[i].phase == NXP_UP   ? AMOTION_ACTION_UP
                                          : AMOTION_ACTION_MOVE;
    if (pev[i].phase == NXP_DOWN)
      debugPrintf("[nxp] cursor tap at (%.0f,%.0f)\n", pev[i].x, pev[i].y);
    inject(env, thiz, unity_motionevent(action, 1, ids, xs, ys), 0);
  }

  /* B -> Android Back key (menu-back), edge-triggered */
  static int prev_b = 0;
  int b = (padGetButtons(&g_pad) & HidNpadButton_B) ? 1 : 0;
  if (b && !prev_b) inject(env, thiz, unity_keyevent(AKEY_ACTION_DOWN, AKEYCODE_BACK), 0);
  if (!b && prev_b) inject(env, thiz, unity_keyevent(AKEY_ACTION_UP,   AKEYCODE_BACK), 0);
  prev_b = b;
}

/* Docked/desktop cursor overlay is now provided by nx_pointer (nxp_draw),
 * called from the eglSwapBuffers wrapper in imports.c. */
