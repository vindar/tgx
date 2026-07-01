#include "tgx_bench.h"

#include <math.h>
#include <stdarg.h>
#include <stdio.h>

namespace tgxbench
{

namespace
{

uint32_t defaultMinFrames(const BenchTest& test, const BenchCaps& caps)
{
    return test.min_frames ? test.min_frames : caps.min_frames;
}

uint32_t defaultMaxFrames(const BenchTest& test, const BenchCaps& caps)
{
    return test.max_frames ? test.max_frames : caps.max_frames;
}

uint32_t defaultTargetUs(const BenchTest& test, const BenchCaps& caps)
{
    return test.target_us ? test.target_us : caps.target_us;
}

void printChecksum(uint64_t checksum)
{
    const uint32_t hi = (uint32_t)(checksum >> 32);
    const uint32_t lo = (uint32_t)(checksum & 0xffffffffUL);
    benchPrintf("0x%08lx%08lx", (unsigned long)hi, (unsigned long)lo);
}

} // namespace

const char* benchMemoryKindName(BenchMemoryKind kind)
{
    switch (kind)
        {
        case BenchMemoryKind::Framebuffer: return "framebuffer";
        case BenchMemoryKind::ZBuffer: return "zbuffer";
        case BenchMemoryKind::ScratchFast: return "scratch_fast";
        case BenchMemoryKind::AssetCache: return "asset_cache";
        case BenchMemoryKind::Bulk: return "bulk";
        }
    return "unknown";
}

bool benchInitContext(BenchContext& ctx)
{
    ctx.caps = benchGetCaps();
    ctx.width = ctx.caps.default_width;
    ctx.height = ctx.caps.default_height;
    ctx.framebuffer = nullptr;
    ctx.zbuffer = nullptr;

    const size_t pixel_count = (size_t)ctx.width * (size_t)ctx.height;
    ctx.framebuffer = static_cast<tgx::RGB565*>(benchAlloc(BenchMemoryKind::Framebuffer, pixel_count * sizeof(tgx::RGB565), 4));
    ctx.zbuffer = static_cast<uint16_t*>(benchAlloc(BenchMemoryKind::ZBuffer, pixel_count * sizeof(uint16_t), 4));

    if ((!ctx.framebuffer) || (!ctx.zbuffer))
        {
        benchPrintf("TGXNB3D_ERROR,stage=alloc,width=%d,height=%d,fb=%p,zbuf=%p\n",
                    ctx.width, ctx.height, (void*)ctx.framebuffer, (void*)ctx.zbuffer);
        return false;
        }

    ctx.image.set(ctx.framebuffer, ctx.width, ctx.height);
    ctx.renderer.setViewportSize(ctx.width, ctx.height);
    ctx.renderer.setOffset(0, 0);
    ctx.renderer.setImage(&ctx.image);
    ctx.renderer.setZbuffer(ctx.zbuffer);
    ctx.renderer.setPerspective(45.0f, ((float)ctx.width) / ((float)ctx.height), 1.0f, 80.0f);
    ctx.renderer.setLookAt(tgx::fVec3{0.0f, 0.0f, 0.0f}, tgx::fVec3{0.0f, 0.0f, -1.0f}, tgx::fVec3{0.0f, 1.0f, 0.0f});
    ctx.renderer.setCulling(0);
    ctx.renderer.setLight(tgx::fVec3{0.25f, -0.45f, -1.0f},
                          tgx::RGBf{0.18f, 0.18f, 0.18f},
                          tgx::RGBf{0.85f, 0.82f, 0.76f},
                          tgx::RGBf{0.0f, 0.0f, 0.0f});
    ctx.renderer.setMaterial(tgx::RGBf{0.92f, 0.55f, 0.28f}, 0.25f, 0.85f, 0.0f, 32);
    benchClearTarget(ctx);
    return true;
}

void benchClearTarget(BenchContext& ctx)
{
    ctx.image.clear(tgx::RGB565_Black);
    ctx.renderer.clearZbuffer();
}

uint64_t benchFramebufferChecksum(const BenchContext& ctx)
{
    const size_t pixel_count = (size_t)ctx.width * (size_t)ctx.height;
    uint64_t h = 1469598103934665603ULL;
    h ^= (uint32_t)ctx.width;
    h *= 1099511628211ULL;
    h ^= (uint32_t)ctx.height;
    h *= 1099511628211ULL;
    for (size_t i = 0; i < pixel_count; ++i)
        {
        h ^= (uint16_t)ctx.framebuffer[i].val;
        h *= 1099511628211ULL;
        }
    return h;
}

int benchRunModule(const BenchModule& module)
{
    if (!benchBackendSetup())
        {
        return 1;
        }

    benchResetAllocators();

    BenchContext ctx;
    if (!benchInitContext(ctx))
        {
        return 1;
        }

    const BenchCaps& caps = ctx.caps;

    benchPrintf("TGXNB3D_BEGIN,version=1,board=%s,profile=%s,module=%s,module_name=%s,width=%d,height=%d,tests=%lu\n",
                caps.board_name, caps.profile_name, module.id, module.name,
                ctx.width, ctx.height, (unsigned long)module.test_count);
    benchPrintf("TGXNB3D_INFO,board=%s,power_class=%d,fast_arena=%lu,bulk_arena=%lu,memory_backend=%s,loaded_shaders=%ld\n",
                caps.board_name, caps.power_class,
                (unsigned long)caps.fast_arena_bytes,
                (unsigned long)caps.bulk_arena_bytes,
                benchMemoryBackendName(),
                (long)TGX_BENCH_LOADED_SHADERS);

    uint32_t ok_count = 0;
    uint32_t fail_count = 0;
    uint32_t skip_count = 0;

    for (uint32_t i = 0; i < module.test_count; ++i)
        {
        const BenchTest& test = module.tests[i];
        const uint32_t min_frames = defaultMinFrames(test, caps);
        const uint32_t max_frames = defaultMaxFrames(test, caps);
        const uint32_t target_us = defaultTargetUs(test, caps);

        benchPrintf("TGXNB3D_TEST_BEGIN,id=%s,name=%s,shaders=%ld,min_frames=%lu,max_frames=%lu,target_us=%lu\n",
                    test.id, test.name, (long)test.shaders,
                    (unsigned long)min_frames, (unsigned long)max_frames, (unsigned long)target_us);
        benchYield();

        if ((test.setup) && (!test.setup(ctx)))
            {
            ++fail_count;
            benchPrintf("TGXNB3D_RESULT,id=%s,status=FAIL,reason=setup\n", test.id);
            benchYield();
            continue;
            }

        const uint32_t warmup_frames = 1;
        for (uint32_t f = 0; f < warmup_frames; ++f)
            {
            benchClearTarget(ctx);
            if (test.render) test.render(ctx, 0);
            }
        benchYield();

        uint32_t measured_frames = 0;
        uint64_t total_us = 0;
        double sum = 0.0;
        double sum_sq = 0.0;
        uint32_t min_us = 0xffffffffUL;
        uint32_t max_us = 0;
        uint32_t last_running_ms = benchMillis();

        do
            {
            benchClearTarget(ctx);
            const uint32_t t0 = benchMicros();
            if (test.render) test.render(ctx, 0);
            const uint32_t dt = benchMicros() - t0;

            total_us += dt;
            sum += (double)dt;
            sum_sq += ((double)dt) * ((double)dt);
            if (dt < min_us) min_us = dt;
            if (dt > max_us) max_us = dt;
            ++measured_frames;

            const uint32_t now_ms = benchMillis();
            if ((now_ms - last_running_ms) >= 500U)
                {
                benchPrintf("TGXNB3D_RUNNING,module=%s,test=%s,frames=%lu,total_us=%lu\n",
                            module.id, test.id, (unsigned long)measured_frames, (unsigned long)total_us);
                last_running_ms = now_ms;
                }
            }
        while ((measured_frames < min_frames) ||
               ((measured_frames < max_frames) && (total_us < target_us)));

        const uint64_t checksum = benchFramebufferChecksum(ctx);
        const double mean = measured_frames ? (sum / (double)measured_frames) : 0.0;
        double variance = 0.0;
        if (measured_frames > 1)
            {
            variance = (sum_sq / (double)measured_frames) - (mean * mean);
            if (variance < 0.0) variance = 0.0;
            }
        const double stddev = sqrt(variance);
        const double fps = total_us ? ((double)measured_frames * 1000000.0 / (double)total_us) : 0.0;

        benchPrintf("TGXNB3D_RESULT,id=%s,status=OK,frames=%lu,total_us=%lu,mean_us_x100=%lu,min_us=%lu,max_us=%lu,stddev_us_x100=%lu,fps_x100=%lu,checksum=",
                    test.id,
                    (unsigned long)measured_frames,
                    (unsigned long)total_us,
                    (unsigned long)(mean * 100.0 + 0.5),
                    (unsigned long)min_us,
                    (unsigned long)max_us,
                    (unsigned long)(stddev * 100.0 + 0.5),
                    (unsigned long)(fps * 100.0 + 0.5));
        printChecksum(checksum);
        benchPrintln("");

        if (test.teardown) test.teardown(ctx);
        ++ok_count;
        benchYield();
        }

    benchPrintf("TGXNB3D_END,module=%s,status=%s,tests=%lu,ok=%lu,skip=%lu,fail=%lu\n",
                module.id, fail_count ? "FAIL" : "OK",
                (unsigned long)module.test_count,
                (unsigned long)ok_count,
                (unsigned long)skip_count,
                (unsigned long)fail_count);

    for (int repeat = 0; repeat < 2; ++repeat)
        {
        benchDelayMs(250);
        benchPrintf("TGXNB3D_REPEAT,module=%s,status=%s,ok=%lu,skip=%lu,fail=%lu\n",
                    module.id, fail_count ? "FAIL" : "OK",
                    (unsigned long)ok_count,
                    (unsigned long)skip_count,
                    (unsigned long)fail_count);
        }

    return fail_count ? 1 : 0;
}

} // namespace tgxbench
