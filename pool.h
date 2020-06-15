#ifndef TEST_POOL_H
#define TEST_POOL_H
extern "C" {
#include <uv.h>
}
#include <vector>
#include <atomic>
#include <glog/logging.h>
#include <functional>
#define LOOP_NUM 4

class Pool {
public:
    Pool();
    static Pool& GetInstance() {
        static Pool pool;
        return pool;
    }
    uv_loop_t* GetLoop() {
        index ++;
        int i = index % LOOP_NUM;
        LOG(INFO) << "GetLoop:" << i;
        return loops[i];
    }

    static void Async(uv_loop_t *loop, std::function<void()>&& func);

private:
    uv_thread_t threads[LOOP_NUM];
    uv_loop_t* loops[LOOP_NUM];
    std::atomic_uint8_t index;
};


#endif //TEST_POOL_H
