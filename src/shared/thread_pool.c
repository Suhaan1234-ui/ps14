#include "ps14/thread.h"
#include <stdlib.h>
#include <string.h>

#ifdef PS14_PLATFORM_WINDOWS
#include <windows.h>
#else
#include <pthread.h>
#include <unistd.h>
#include <sched.h>
#endif

struct Ps14ThreadPool {
    u32 tc, qs; Ps14ThreadHandle* th; Ps14ThreadTask* q;
    Ps14Mutex qm; Ps14Semaphore qsema; Ps14Event se, pe;
    bool r, p; Ps14ThreadPriority pri; Ps14ThreadPoolStats st;
};

#ifdef PS14_PLATFORM_WINDOWS
DWORD WINAPI tpw(LPVOID a) {
    Ps14ThreadPool* p = (Ps14ThreadPool*)a;
    while (1) {
        if (!ps14_semaphore_wait(&p->qsema, 100)) {
            if (ps14_event_wait(&p->se, 0)) break; continue;
        }
        if (p->p) { ps14_semaphore_signal(&p->qsema); ps14_thread_sleep(10); continue; }
        Ps14ThreadTask* t = NULL;
        ps14_mutex_lock(&p->qm);
        if (p->q) { t = p->q; p->q = t->next; p->st.queue_size--; }
        ps14_mutex_unlock(&p->qm);
        if (t) { p->st.active_threads++; t->function(t->arg); p->st.active_threads--;
            p->st.completed_tasks++; free(t); }
    }
    return 0;
}
#else
void* tpw(void* a) {
    Ps14ThreadPool* p = (Ps14ThreadPool*)a;
    while (1) {
        if (!ps14_semaphore_wait(&p->qsema, 100)) {
            if (ps14_event_wait(&p->se, 0)) break; continue;
        }
        if (p->p) { ps14_semaphore_signal(&p->qsema); ps14_thread_sleep(10); continue; }
        Ps14ThreadTask* t = NULL;
        ps14_mutex_lock(&p->qm);
        if (p->q) { t = p->q; p->q = t->next; p->st.queue_size--; }
        ps14_mutex_unlock(&p->qm);
        if (t) { p->st.active_threads++; t->function(t->arg); p->st.active_threads--;
            p->st.completed_tasks++; free(t); }
    }
    return NULL;
}
#endif

i32 ps14_thread_pool_init(Ps14ThreadPool** po, u32 tc, u32 qs) {
    if (!po || tc == 0) return PS14_ERROR_INVALID_ARGUMENT;
    Ps14ThreadPool* p = malloc(sizeof(Ps14ThreadPool));
    if (!p) return PS14_ERROR_OUT_OF_MEMORY;
    memset(p, 0, sizeof(Ps14ThreadPool));
    p->tc = tc; p->qs = qs; p->r = true; p->p = false; p->pri = PS14_THREAD_PRIORITY_NORMAL;
    ps14_mutex_init(&p->qm); ps14_semaphore_init(&p->qsema, 0, qs);
    ps14_event_init(&p->se, false, false); ps14_event_init(&p->pe, false, false);
    p->th = malloc(tc * sizeof(Ps14ThreadHandle));
    if (!p->th) { free(p); return PS14_ERROR_OUT_OF_MEMORY; }
    for (u32 i = 0; i < tc; i++) {
        #ifdef PS14_PLATFORM_WINDOWS
        p->th[i] = CreateThread(NULL, 0, tpw, p, 0, NULL);
        if (!p->th[i]) { for (u32 j = 0; j < i; j++) CloseHandle(p->th[j]); free(p->th); free(p); return PS14_ERROR_UNKNOWN; }
        #else
        if (pthread_create(&p->th[i], NULL, tpw, p) != 0) { for (u32 j = 0; j < i; j++) pthread_join(p->th[j], NULL); free(p->th); free(p); return PS14_ERROR_UNKNOWN; }
        #endif
    }
    *po = p; return PS14_SUCCESS;
}

void ps14_thread_pool_shutdown(Ps14ThreadPool* p, bool w) {
    if (!p) return;
    ps14_event_set(&p->se);
    for (u32 i = 0; i < p->tc; i++) ps14_semaphore_signal(&p->qsema);
    if (w) {
        #ifdef PS14_PLATFORM_WINDOWS
        WaitForMultipleObjects(p->tc, p->th, TRUE, INFINITE);
        for (u32 i = 0; i < p->tc; i++) CloseHandle(p->th[i]);
        #else
        for (u32 i = 0; i < p->tc; i++) pthread_join(p->th[i], NULL);
        #endif
    }
    ps14_event_destroy(&p->se); ps14_event_destroy(&p->pe);
    ps14_semaphore_destroy(&p->qsema); ps14_mutex_destroy(&p->qm);
    free(p->th); free(p);
}

i32 ps14_thread_pool_submit(Ps14ThreadPool* p, void (*f)(void*), void* a, const char* n) {
    if (!p || !f) return PS14_ERROR_INVALID_ARGUMENT;
    if (p->p) return PS14_ERROR_UNKNOWN;
    Ps14ThreadTask* t = malloc(sizeof(Ps14ThreadTask));
    if (!t) return PS14_ERROR_OUT_OF_MEMORY;
    t->function = f; t->arg = a; t->name = n; t->next = NULL;
    ps14_mutex_lock(&p->qm);
    if (p->st.queue_size >= p->qs) { ps14_mutex_unlock(&p->qm); free(t); return PS14_ERROR_RESOURCE_EXHAUSTION; }
    Ps14ThreadTask** c = &p->q; while (*c) c = &(*c)->next; *c = t; p->st.queue_size++;
    ps14_mutex_unlock(&p->qm); ps14_semaphore_signal(&p->qsema); return PS14_SUCCESS;
}

void ps14_thread_pool_get_stats(Ps14ThreadPool* p, Ps14ThreadPoolStats* s) {
    if (!p || !s) return;
    ps14_mutex_lock(&p->qm); memcpy(s, &p->st, sizeof(Ps14ThreadPoolStats));
    s->idle_threads = p->tc - s->active_threads; s->total_threads = p->tc;
    ps14_mutex_unlock(&p->qm);
}

void ps14_thread_pool_set_priority(Ps14ThreadPool* p, Ps14ThreadPriority pr) { if (p) p->pri = pr; }
bool ps14_thread_pool_wait_for_completion(Ps14ThreadPool* p, u32 to) { return false; }
void ps14_thread_pool_pause(Ps14ThreadPool* p) { if (p) p->p = true; }
void ps14_thread_pool_resume(Ps14ThreadPool* p) { if (p) p->p = false; }
bool ps14_thread_pool_is_paused(Ps14ThreadPool* p) { return p ? p->p : false; }

#ifdef PS14_PLATFORM_WINDOWS
int ptc(Ps14ThreadHandle* h, Ps14ThreadFunc f, void* a, u32 pr) {
    DWORD wp = THREAD_PRIORITY_NORMAL;
    switch(pr) { case 0: wp=THREAD_PRIORITY_LOWEST; break; case 1: wp=THREAD_PRIORITY_BELOW_NORMAL; break;
        case 2: wp=THREAD_PRIORITY_NORMAL; break; case 3: wp=THREAD_PRIORITY_ABOVE_NORMAL; break;
        case 4: wp=THREAD_PRIORITY_HIGHEST; break; case 5: wp=THREAD_PRIORITY_TIME_CRITICAL; break; }
    *h = CreateThread(NULL, 0, f, a, 0, NULL); if (!*h) return 1;
    SetThreadPriority(*h, wp); return 0;
}
void pthj(Ps14ThreadHandle h) { WaitForSingleObject(h, INFINITE); CloseHandle(h); }
void pthd(Ps14ThreadHandle h) { CloseHandle(h); }
Ps14ThreadId pthi(void) { return GetCurrentThreadId(); }
void pths(u32 ms) { Sleep(ms); }
void pthy(void) { SwitchToThread(); }
void pmi(Ps14Mutex* m) { InitializeCriticalSection(m); }
void pmd(Ps14Mutex* m) { DeleteCriticalSection(m); }
void pml(Ps14Mutex* m) { EnterCriticalSection(m); }
void pmu(Ps14Mutex* m) { LeaveCriticalSection(m); }
bool pmt(Ps14Mutex* m) { return TryEnterCriticalSection(m) != 0; }
#else
int ptc(Ps14ThreadHandle* h, Ps14ThreadFunc f, void* a, u32 pr) {
    pthread_attr_t at; pthread_attr_init(&at); pthread_create(h, &at, f, a);
    pthread_attr_destroy(&at); return 0;
}
void pthj(Ps14ThreadHandle h) { pthread_join(h, NULL); }
void pthd(Ps14ThreadHandle h) { pthread_detach(h); }
Ps14ThreadId pthi(void) { return (Ps14ThreadId)pthread_self(); }
void pths(u32 ms) { usleep(ms * 1000); }
void pthy(void) { sched_yield(); }
void pmi(Ps14Mutex* m) { pthread_mutex_init(m, NULL); }
void pmd(Ps14Mutex* m) { pthread_mutex_destroy(m); }
void pml(Ps14Mutex* m) { pthread_mutex_lock(m); }
void pmu(Ps14Mutex* m) { pthread_mutex_unlock(m); }
bool pmt(Ps14Mutex* m) { return pthread_mutex_trylock(m) == 0; }
#endif

i32 ps14_thread_create(Ps14ThreadHandle* h, Ps14ThreadFunc f, void* a, Ps14ThreadPriority p) {
    return ptc(h, f, a, p);
}
void ps14_thread_join(Ps14ThreadHandle h) { pthj(h); }
void ps14_thread_detach(Ps14ThreadHandle h) { pthd(h); }
Ps14ThreadId ps14_thread_get_current_id(void) { return pthi(); }
const char* ps14_thread_get_name(void) { return NULL; }
void ps14_thread_set_name(const char* n) {}
void ps14_thread_sleep(u32 ms) { pths(ms); }
void ps14_thread_yield(void) { pthy(); }
void ps14_mutex_init(Ps14Mutex* m) { pmi(m); }
void ps14_mutex_destroy(Ps14Mutex* m) { pmd(m); }
void ps14_mutex_lock(Ps14Mutex* m) { pml(m); }
void ps14_mutex_unlock(Ps14Mutex* m) { pmu(m); }
bool ps14_mutex_try_lock(Ps14Mutex* m) { return pmt(m); }

#ifdef PS14_PLATFORM_WINDOWS
typedef HANDLE psem; #define INVS NULL
i32 psi(psem* s, u32 ic, u32 mc) { *s = CreateSemaphore(NULL, ic, mc, NULL); return *s ? 0 : 1; }
void psd(psem* s) { if (*s) { CloseHandle(*s); *s = NULL; } }
bool psw(psem* s, u32 to) { DWORD r = WaitForSingleObject(*s, to == 0 ? INFINITE : to); return r == WAIT_OBJECT_0; }
void pss(psem* s) { ReleaseSemaphore(*s, 1, NULL); }
u32 psc(psem* s) { LONG c = 0; ReleaseSemaphore(*s, 0, &c); return (u32)c; }
#else
typedef sem_t psem; #define INVS 0
i32 psi(psem* s, u32 ic, u32 mc) { return sem_init(s, 0, ic) == 0 ? 0 : 1; }
void psd(psem* s) { sem_destroy(s); }
bool psw(psem* s, u32 to) { if (to == 0) return sem_wait(s) == 0; struct timespec ts; clock_gettime(CLOCK_REALTIME, &ts); ts.tv_sec += to/
