#include "pool.h"
#include <glog/logging.h>
#include <thread>

void timer_cb(uv_timer_t* handle) {
    LOG(INFO) << "loop running...:" << *(int*)handle->loop->data;
}

void async_cb(uv_async_t* handle) {
    auto func = (std::function<void()>*)(handle->data);
    (*func)();
    uv_close((uv_handle_t*)handle, [](uv_handle_t* handle) {
        delete handle;
    });
    delete func;
    handle->data = nullptr;
}

void run(void* arg) {
    uv_timer_t timer;
    uv_loop_t* loop = (uv_loop_t*)arg;
    uv_timer_init(loop, &timer);
    uv_timer_start(&timer, timer_cb, 1000, 60*1000);
    LOG(INFO) << "loop running:" << *(int*)loop->data;
    uv_run(loop, UV_RUN_DEFAULT);
}

Pool::Pool() {
    for (int i = 0; i < LOOP_NUM; i ++) {
        loops[i] = (uv_loop_t*)malloc(sizeof(uv_loop_t));
        loops[i]->data = new int(i);
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
