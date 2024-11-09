/*
 * Tencent is pleased to support the open source community by making wechat-matrix available.
 * Copyright (C) 2021 THL A29 Limited, a Tencent company. All rights reserved.
 * Licensed under the BSD 3-Clause License (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      https://opensource.org/licenses/BSD-3-Clause
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef LIBWECHATBACKTRACE_BACKTRACE_H
#define LIBWECHATBACKTRACE_BACKTRACE_H

#include <android/log.h>
#include <functional>
#include "BacktraceDefine.h"

namespace wechat_backtrace {

    QUT_EXTERN_C_BLOCK

    void BACKTRACE_FUNC_WRAPPER(unwind_adapter)(
            Frame *frames, const size_t max_frames, size_t &frame_size);

    void BACKTRACE_FUNC_WRAPPER(restore_frame_detail)(
            const Frame *frames, const size_t frame_size,
            const std::function<void(FrameDetail)> &frame_callback);

    void BACKTRACE_FUNC_WRAPPER(notify_maps_changed)();

    QUT_EXTERN_C_BLOCK_END
}

#endif //LIBWECHATBACKTRACE_BACKTRACE_H
