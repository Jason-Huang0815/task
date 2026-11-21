#include "framework.h"

// 主机逻辑测试入口：runAll() 返回失败用例数，全部通过时打印总标记。
int main() {
    const int failed = test::runAll();
    if (failed == 0) {
        std::printf("\nALL LOGIC TESTS PASSED\n");
        return 0;
    }
    return 1;
}
