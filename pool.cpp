#include "pool.h"
#include <glog/logging.h>
#include <thread>

void timer_cb(uv_timer_t* handle) {
    LOG(INFO) << "loop running...";
}

void async_cb(uv_async_t* handle) {
    auto func = (std::function<void()>*)(handle->data);
    (*func)();
    delete func;
    delete handle;
}

void run(void* arg) {
    LOG(INFO) << "loop running";
    uv_timer_t timer;
    uv_loop_t* loop = (uv_loop_t*)arg;
    uv_timer_init(loop, &timer);
    uv_timer_start(&timer, timer_cb, 1000, 60*1000);
    uv_run(loop, UV_RUN_DEFAULT);
}

Pool::Pool() {
    for (int i = 0; i < LOOP_NUM; i ++) {
        loops[i] = (uv_loop_t*)malloc(sizeof(uv_loop_t));
        uv_loop_init(loops[i]);
        uv_thread_create(&threads[i], run, loops[i]);
    }
}

void Pool::Async(uv_loop_t *loop, std::function<void()>&& func) {
    uv_async_t* handle = new uv_async_t;
    uv_async_init(loop, handle, async_cb);
    handle->data = new std::function<void()>(std::move(func));
    uv_async_send(handle);
}
