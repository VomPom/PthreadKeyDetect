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

#include <dlfcn.h>
#include "Backtrace.h"
#include "Backtrace.h"
#include "FpUnwinder.h"
#include <MinimalRegs.h>
#include <cxxabi.h>
#include <PthreadExt.h>
#define FP_MINIMAL_REG_SIZE 4
namespace wechat_backtrace {

    QUT_EXTERN_C_BLOCK

#ifdef __aarch64__
    static const bool m_is_arm32 = false;
#else
    static const bool m_is_arm32 = true;
#endif

    BACKTRACE_EXPORT void
    BACKTRACE_FUNC_WRAPPER(restore_frame_detail)(const Frame *frames, const size_t frame_size,
                                                 const std::function<void(
                                                         FrameDetail)> &frame_callback) {
        if (frames == nullptr || frame_callback == nullptr) {
            return;
        }

        for (size_t i = 0; i < frame_size; i++) {
            auto &frame_data = frames[i];
            Dl_info stack_info{};
            int success = dladdr((void *) frame_data.pc,
                                 &stack_info); // 用修正后的 pc dladdr 会偶现 npe crash, 因此还是用 lr

#ifdef __aarch64__  // TODO Fix hardcode
            // fp_unwind 得到的 pc 除了第 0 帧实际都是 LR, arm64 指令长度都是定长 32bit, 所以 -4 以恢复 pc
            uptr real_pc = frame_data.pc - (i > 0 ? 4 : 0);
#else
            uptr real_pc = frame_data.pc;
#endif
            FrameDetail detail = {
                    .rel_pc = real_pc - (uptr) stack_info.dli_fbase,
                    .map_name = success == 0 || stack_info.dli_fname == nullptr ? "null"
                                                                                : stack_info.dli_fname,
                    .function_name = success == 0 || stack_info.dli_sname == nullptr ? "null"
                                                                                     : stack_info.dli_sname
            };
            frame_callback(detail);
        }

    }


    static inline void
    fp_based_unwind_inlined(Frame *frames, const size_t max_frames,
                            size_t &frame_size) {
        uptr regs[FP_MINIMAL_REG_SIZE];
        GetFramePointerMinimalRegs(regs);
        FpUnwind(regs, frames, max_frames, frame_size);
    }

    BACKTRACE_EXPORT void
    BACKTRACE_FUNC_WRAPPER(unwind_adapter)(Frame *frames, const size_t max_frames,
                                           size_t &frame_size) {

        fp_based_unwind_inlined(frames, max_frames, frame_size);
    }


    QUT_EXTERN_C_BLOCK_END
}
