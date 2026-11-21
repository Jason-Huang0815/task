#include "ui/nav_model.h"

// 页面导航：环形推进 + 弹窗锁定。纯整数运算，主机测试覆盖环绕与锁定。

namespace hb::ui {

void NavModel::next() {
    if (m_locked) {
        return;
    }
    m_index = (m_index + 1) % kPageCount;
}

void NavModel::prev() {
    if (m_locked) {
        return;
    }
    m_index = (m_index + kPageCount - 1) % kPageCount;
}

void NavModel::home() {
    if (m_locked) {
        return;
    }
    m_index = 0;
}

}  // namespace hb::ui
