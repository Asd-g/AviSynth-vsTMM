#include <limits>

#include "vsTMM.h"

template<typename T>
static void copyPad(PVideoFrame& src, PVideoFrame& dst, const int plane, const int widthPad, IScriptEnvironment* env) noexcept
{
    const int height = src->GetHeight(plane);    

    env->BitBlt(dst->GetWritePtr(PLANAR_Y) + widthPad * sizeof(T), dst->GetPitch(PLANAR_Y), src->GetReadPtr(plane), src->GetPitch(plane), src->GetRowSize(plane), height);

    const int stride = dst->GetPitch(PLANAR_Y) / sizeof(T);
    const int width = src->GetRowSize(plane) / sizeof(T);
    T* __restrict dstp = reinterpret_cast<T*>(dst->GetWritePtr(PLANAR_Y)) + widthPad;

    for (int y = 0; y < height; y++)
    {
        dstp[-1] = dstp[1];
        dstp[width] = dstp[width - 2];

        dstp += stride;
    }
}

template<typename T>
void CreateMM::threshMask_c(PVideoFrame& src, PVideoFrame& dst, int plane, IScriptEnvironment* env) noexcept
{
    switch (plane)
    {
        case 1: plane = 0; break;
        case 2: plane = 1; break;
        default: plane = 2; break;
    }

    const int height = vi.height >> (plane ? vi.GetPlaneHeightSubsampling(PLANAR_U) : 0);    
    const int dst_stride = dst->GetPitch(PLANAR_Y) / sizeof(T);    
    T* __restrict dstp0 = reinterpret_cast<T*>(dst->GetWritePtr(PLANAR_Y)) + widthPad;
    T* __restrict dstp1 = dstp0 + dst_stride * static_cast<int64_t>(height);

    if (plane == 0 && mtqL_ > -1 && mthL_ > -1)
    {
        std::fill_n(dstp0 - widthPad, dst_stride * height, static_cast<T>(mtqL_));
        std::fill_n(dstp1 - widthPad, dst_stride * height, static_cast<T>(mthL_));
        return;
    }
    else if (plane > 0 && mtqC_ > -1 && mthC_ > -1)
    {
        std::fill_n(dstp0 - widthPad, dst_stride * height, static_cast<T>(mtqC_));
        std::fill_n(dstp1 - widthPad, dst_stride * height, static_cast<T>(mthC_));
        return;
    }

    const int stride = src->GetPitch(PLANAR_Y) / sizeof(T);
    const T* srcp = reinterpret_cast<const T*>(src->GetReadPtr(PLANAR_Y)) + widthPad;
    const T* srcpp = srcp + stride;
    const T* srcpn = srcpp;
    const int width = vi.width >> (plane ? vi.GetPlaneWidthSubsampling(PLANAR_U) : 0);

    constexpr T peak = std::numeric_limits<T>::max();

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            int min0 = peak, max0 = 0;
            int min1 = peak, max1 = 0;

            switch (ttype_)
            {
                case 0: // 4 neighbors - compensated
                {
                    if (srcpp[x] < min0)
                        min0 = srcpp[x];
                    if (srcpp[x] > max0)
                        max0 = srcpp[x];
                    if (srcp[x - 1] < min1)
                        min1 = srcp[x - 1];
                    if (srcp[x - 1] > max1)
                        max1 = srcp[x - 1];
                    if (srcp[x + 1] < min1)
                        min1 = srcp[x + 1];
                    if (srcp[x + 1] > max1)
                        max1 = srcp[x + 1];
                    if (srcpn[x] < min0)
                        min0 = srcpn[x];
                    if (srcpn[x] > max0)
                        max0 = srcpn[x];

                    const int atv = std::max((std::abs(srcp[x] - min0) + vHalf[plane]) >> vShift[plane], (std::abs(srcp[x] - max0) + vHalf[plane]) >> vShift[plane]);
                    const int ath = std::max((std::abs(srcp[x] - min1) + hHalf[plane]) >> hShift[plane], (std::abs(srcp[x] - max1) + hHalf[plane]) >> hShift[plane]);
                    const int atmax = std::max(atv, ath);
                    dstp0[x] = (atmax + 2) >> 2;
                    dstp1[x] = (atmax + 1) >> 1;
                }
                break;
                case 1: // 8 neighbors - compensated
                {
                    if (srcpp[x - 1] < min0)
                        min0 = srcpp[x - 1];
                    if (srcpp[x - 1] > max0)
                        max0 = srcpp[x - 1];
                    if (srcpp[x] < min0)
                        min0 = srcpp[x];
                    if (srcpp[x] > max0)
                        max0 = srcpp[x];
                    if (srcpp[x + 1] < min0)
                        min0 = srcpp[x + 1];
                    if (srcpp[x + 1] > max0)
                        max0 = srcpp[x + 1];
                    if (srcp[x - 1] < min1)
                        min1 = srcp[x - 1];
                    if (srcp[x - 1] > max1)
                        max1 = srcp[x - 1];
                    if (srcp[x + 1] < min1)
                        min1 = srcp[x + 1];
                    if (srcp[x + 1] > max1)
                        max1 = srcp[x + 1];
                    if (srcpn[x - 1] < min0)
                        min0 = srcpn[x - 1];
                    if (srcpn[x - 1] > max0)
                        max0 = srcpn[x - 1];
                    if (srcpn[x] < min0)
                        min0 = srcpn[x];
                    if (srcpn[x] > max0)
                        max0 = srcpn[x];
                    if (srcpn[x + 1] < min0)
                        min0 = srcpn[x + 1];
                    if (srcpn[x + 1] > max0)
                        max0 = srcpn[x + 1];

                    const int atv = std::max((std::abs(srcp[x] - min0) + vHalf[plane]) >> vShift[plane], (std::abs(srcp[x] - max0) + vHalf[plane]) >> vShift[plane]);
                    const int ath = std::max((std::abs(srcp[x] - min1) + hHalf[plane]) >> hShift[plane], (std::abs(srcp[x] - max1) + hHalf[plane]) >> hShift[plane]);
                    const int atmax = std::max(atv, ath);
                    dstp0[x] = (atmax + 2) >> 2;
                    dstp1[x] = (atmax + 1) >> 1;
                }
                break;
                case 2: // 4 neighbors - not compensated
                {
                    if (srcpp[x] < min0)
                        min0 = srcpp[x];
                    if (srcpp[x] > max0)
                        max0 = srcpp[x];
                    if (srcp[x - 1] < min0)
                        min0 = srcp[x - 1];
                    if (srcp[x - 1] > max0)
                        max0 = srcp[x - 1];
                    if (srcp[x + 1] < min0)
                        min0 = srcp[x + 1];
                    if (srcp[x + 1] > max0)
                        max0 = srcp[x + 1];
                    if (srcpn[x] < min0)
                        min0 = srcpn[x];
                    if (srcpn[x] > max0)
                        max0 = srcpn[x];

                    const int at = std::max(std::abs(srcp[x] - min0), std::abs(srcp[x] - max0));
                    dstp0[x] = (at + 2) >> 2;
                    dstp1[x] = (at + 1) >> 1;
                }
                break;
                case 3: // 8 neighbors - not compensated
                {
                    if (srcpp[x - 1] < min0)
                        min0 = srcpp[x - 1];
                    if (srcpp[x - 1] > max0)
                        max0 = srcpp[x - 1];
                    if (srcpp[x] < min0)
                        min0 = srcpp[x];
                    if (srcpp[x] > max0)
                        max0 = srcpp[x];
                    if (srcpp[x + 1] < min0)
                        min0 = srcpp[x + 1];
                    if (srcpp[x + 1] > max0)
                        max0 = srcpp[x + 1];
                    if (srcp[x - 1] < min0)
                        min0 = srcp[x - 1];
                    if (srcp[x - 1] > max0)
                        max0 = srcp[x - 1];
                    if (srcp[x + 1] < min0)
                        min0 = srcp[x + 1];
                    if (srcp[x + 1] > max0)
                        max0 = srcp[x + 1];
                    if (srcpn[x - 1] < min0)
                        min0 = srcpn[x - 1];
                    if (srcpn[x - 1] > max0)
                        max0 = srcpn[x - 1];
                    if (srcpn[x] < min0)
                        min0 = srcpn[x];
                    if (srcpn[x] > max0)
                        max0 = srcpn[x];
                    if (srcpn[x + 1] < min0)
                        min0 = srcpn[x + 1];
                    if (srcpn[x + 1] > max0)
                        max0 = srcpn[x + 1];

                    const int at = std::max(std::abs(srcp[x] - min0), std::abs(srcp[x] - max0));
                    dstp0[x] = (at + 2) >> 2;
                    dstp1[x] = (at + 1) >> 1;
                }
                break;
                case 4: // 4 neighbors - not compensated (range)
                {
                    if (srcpp[x] < min0)
                        min0 = srcpp[x];
                    if (srcpp[x] > max0)
                        max0 = srcpp[x];
                    if (srcp[x - 1] < min0)
                        min0 = srcp[x - 1];
                    if (srcp[x - 1] > max0)
                        max0 = srcp[x - 1];
                    if (srcp[x] < min0)
                        min0 = srcp[x];
                    if (srcp[x] > max0)
                        max0 = srcp[x];
                    if (srcp[x + 1] < min0)
                        min0 = srcp[x + 1];
                    if (srcp[x + 1] > max0)
                        max0 = srcp[x + 1];
                    if (srcpn[x] < min0)
                        min0 = srcpn[x];
                    if (srcpn[x] > max0)
                        max0 = srcpn[x];

                    const int at = max0 - min0;
                    dstp0[x] = (at + 2) >> 2;
                    dstp1[x] = (at + 1) >> 1;
                }
                break;
                default: // 8 neighbors - not compensated (range)
                {
                    if (srcpp[x - 1] < min0)
                        min0 = srcpp[x - 1];
                    if (srcpp[x - 1] > max0)
                        max0 = srcpp[x - 1];
                    if (srcpp[x] < min0)
                        min0 = srcpp[x];
                    if (srcpp[x] > max0)
                        max0 = srcpp[x];
                    if (srcpp[x + 1] < min0)
                        min0 = srcpp[x + 1];
                    if (srcpp[x + 1] > max0)
                        max0 = srcpp[x + 1];
                    if (srcp[x - 1] < min0)
                        min0 = srcp[x - 1];
                    if (srcp[x - 1] > max0)
                        max0 = srcp[x - 1];
                    if (srcp[x] < min0)
                        min0 = srcp[x];
                    if (srcp[x] > max0)
                        max0 = srcp[x];
                    if (srcp[x + 1] < min0)
                        min0 = srcp[x + 1];
                    if (srcp[x + 1] > max0)
                        max0 = srcp[x + 1];
                    if (srcpn[x - 1] < min0)
                        min0 = srcpn[x - 1];
                    if (srcpn[x - 1] > max0)
                        max0 = srcpn[x - 1];
                    if (srcpn[x] < min0)
                        min0 = srcpn[x];
                    if (srcpn[x] > max0)
                        max0 = srcpn[x];
                    if (srcpn[x + 1] < min0)
                        min0 = srcpn[x + 1];
                    if (srcpn[x + 1] > max0)
                        max0 = srcpn[x + 1];

                    const int at = max0 - min0;
                    dstp0[x] = (at + 2) >> 2;
                    dstp1[x] = (at + 1) >> 1;
                }
                break;
            }
        }

        srcpp = srcp;
        srcp = srcpn;
        srcpn += (y < height - 2) ? stride : -stride;
        dstp0 += dst_stride;
        dstp1 += dst_stride;
    }

    T* dstp = reinterpret_cast<T*>(dst->GetWritePtr(PLANAR_Y));

    if (plane == 0 && mtqL_ > -1)
        std::fill_n(dstp, dst_stride * height, static_cast<T>(mtqL_));
    else if (plane == 0 && mthL_ > -1)
        std::fill_n(dstp + dst_stride * static_cast<int64_t>(height), dst_stride * height, static_cast<T>(mthL_));
    else if (plane > 0 && mtqC_ > -1)
        std::fill_n(dstp, dst_stride * height, static_cast<T>(mtqC_));
    else if (plane > 0 && mthC_ > -1)
        std::fill_n(dstp + dst_stride * static_cast<int64_t>(height), dst_stride * height, static_cast<T>(mthC_));
}

template<typename T>
void CreateMM::motionMask_c(PVideoFrame& src1, PVideoFrame& msk1, PVideoFrame& src2, PVideoFrame& msk2, PVideoFrame& dst, const int plane, IScriptEnvironment* env) noexcept
{  
    const int height = vi.height >> (plane != PLANAR_Y ? vi.GetPlaneHeightSubsampling(PLANAR_U) : 0);
    const int msk1_stride = msk1->GetPitch(PLANAR_Y) / sizeof(T);
    const int msk2_stride = msk2->GetPitch(PLANAR_Y) / sizeof(T);
    const int dst_stride = dst->GetPitch(PLANAR_Y) / sizeof(T);
    const T* mskp1q = reinterpret_cast<const T*>(msk1->GetReadPtr(PLANAR_Y)) + widthPad;
    const T* mskp2q = reinterpret_cast<const T*>(msk2->GetReadPtr(PLANAR_Y)) + widthPad;
    T* __restrict dstpq = reinterpret_cast<T*>(dst->GetWritePtr(PLANAR_Y)) + widthPad;
    const T* mskp1h = mskp1q + msk1_stride * static_cast<int64_t>(height);
    const T* mskp2h = mskp2q + msk2_stride * static_cast<int64_t>(height);
    T* __restrict dstph = dstpq + dst_stride * static_cast<int64_t>(height);
    const int stride = src1->GetPitch(PLANAR_Y) / sizeof(T);
    const int src2_stride = src2->GetPitch(PLANAR_Y) / sizeof(T);
    int width = vi.width >> (plane != PLANAR_Y ? vi.GetPlaneWidthSubsampling(PLANAR_U) : 0);
    const T* srcp1 = reinterpret_cast<const T*>(src1->GetReadPtr(PLANAR_Y)) + widthPad;
    const T* srcp2 = reinterpret_cast<const T*>(src2->GetReadPtr(PLANAR_Y)) + widthPad;

    constexpr T peak = std::numeric_limits<T>::max();

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            const int diff = std::abs(srcp1[x] - srcp2[x]);
            dstpq[x] = (diff <= std::min(std::max(std::min(mskp1q[x], mskp2q[x]) + nt_, minthresh_), maxthresh_)) ? peak : 0;
            dstph[x] = (diff <= std::min(std::max(std::min(mskp1h[x], mskp2h[x]) + nt_, minthresh_), maxthresh_)) ? peak : 0;
        }

        srcp1 += stride;
        srcp2 += src2_stride;
        mskp1q += msk1_stride;
        mskp1h += msk1_stride;
        mskp2q += msk2_stride;
        mskp2h += msk2_stride;
        dstpq += dst_stride;
        dstph += dst_stride;
    }
}

template<typename T>
void CreateMM::andMasks_c(PVideoFrame& src1, PVideoFrame& src2, PVideoFrame& dst, const int plane, IScriptEnvironment* env) noexcept
{
    const int stride = src1->GetPitch(PLANAR_Y) / sizeof(T);
    const int src2_stride = src2->GetPitch(PLANAR_Y) / sizeof(T);
    const int dst_stride = dst->GetPitch(PLANAR_Y) / sizeof(T);
    int width = vi.width >> (plane != PLANAR_Y ? vi.GetPlaneWidthSubsampling(PLANAR_U) : 0);
    const int height = (vi.height * 2) >> (plane != PLANAR_Y ? vi.GetPlaneHeightSubsampling(PLANAR_U) : 0);
    const T* srcp1 = reinterpret_cast<const T*>(src1->GetReadPtr(PLANAR_Y)) + widthPad;
    const T* srcp2 = reinterpret_cast<const T*>(src2->GetReadPtr(PLANAR_Y)) + widthPad;
    T* __restrict dstp = reinterpret_cast<T*>(dst->GetWritePtr(PLANAR_Y)) + widthPad;

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
            dstp[x] &= srcp1[x] & srcp2[x];

        dstp[-1] = dstp[1];
        dstp[width] = dstp[width - 2];

        srcp1 += stride;
        srcp2 += src2_stride;
        dstp += dst_stride;
    }
}

template<typename T>
void CreateMM::combineMasks_c(PVideoFrame& src, PVideoFrame& dst, const int plane, IScriptEnvironment* env) noexcept
{
    const int height = dst->GetHeight(plane);

    env->BitBlt(dst->GetWritePtr(plane), dst->GetPitch(plane), src->GetReadPtr(PLANAR_Y) + widthPad * sizeof(T), src->GetPitch(PLANAR_Y), dst->GetRowSize(plane), height);

    const int srcStride = src->GetPitch(PLANAR_Y) / sizeof(T);
    const T* srcp0 = reinterpret_cast<const T*>(src->GetReadPtr(PLANAR_Y)) + widthPad;
    const T* srcpp0 = srcp0 + srcStride;
    const T* srcpn0 = srcpp0;
    const T* srcp1 = srcp0 + srcStride * static_cast<int64_t>(height);
    const int width = dst->GetRowSize(plane) / sizeof(T);
    const int dstStride = dst->GetPitch(plane) / sizeof(T);
    T* __restrict dstp = reinterpret_cast<T*>(dst->GetWritePtr(plane));

    constexpr T peak = std::numeric_limits<T>::max();

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            if (srcp0[x] || !srcp1[x])
                continue;

            int count = 0;

            if (srcpp0[x - 1])
                count++;
            if (srcpp0[x])
                count++;
            if (srcpp0[x + 1])
                count++;
            if (srcp0[x - 1])
                count++;
            if (srcp0[x + 1])
                count++;
            if (srcpn0[x - 1])
                count++;
            if (srcpn0[x])
                count++;
            if (srcpn0[x + 1])
                count++;

            if (count >= cstr_)
                dstp[x] = peak;
        }

        srcpp0 = srcp0;
        srcp0 = srcpn0;
        srcpn0 += (y < height - 2) ? srcStride : -srcStride;
        srcp1 += srcStride;
        dstp += dstStride;
    }
}

CreateMM::CreateMM(PClip _child, int ttype, int mtqL, int mthL, int mtqC, int mthC, int nt, int minthresh, int maxthresh, int cstr, int y, int u, int v, int opt, IScriptEnvironment* env)
    : Common(_child, y, u, v, env), ttype_(ttype), mtqL_(mtqL), mthL_(mthL), mtqC_(mtqC), mthC_(mthC), nt_(nt), minthresh_(minthresh), maxthresh_(maxthresh), cstr_(cstr), opt_(opt)
{
    const int peak = (1 << vi.BitsPerComponent()) - 1;

    if (mtqL_ > -2 || mthL_ > -2 || mtqC_ > -2 || mthC_ > -2)
    {
        if (mtqL_ > -1)
            mtqL_ = mtqL_ * peak / 255;
        if (mthL_ > -1)
            mthL_ = mthL_ * peak / 255;
        if (mtqC_ > -1)
            mtqC_ = mtqC_ * peak / 255;
        if (mthC_ > -1)
            mthC_ = mthC_ * peak / 255;
    }

    nt_ = nt_ * peak / 255;
    minthresh_ = minthresh_ * peak / 255;
    maxthresh_ = maxthresh_ * peak / 255;

    *hShift = *vShift = *hHalf = *vHalf = 0;

    const int planecount = std::min(vi.NumComponents(), 3);
    for (int plane = 0; plane < planecount; plane++)
    {
        hShift[plane] = (plane ? vi.GetPlaneWidthSubsampling(PLANAR_U) : 0);
        vShift[plane] = (plane ? 1 << vi.GetPlaneHeightSubsampling(PLANAR_U) : 1);
        hHalf[plane] = hShift[plane] ? 1 << (hShift[plane] - 1) : hShift[plane];
        vHalf[plane] = 1 << (vShift[plane] - 1);
    }

    vipad = vimsk = vi;

    switch (vi.BitsPerComponent())
    {
        case 8: vipad.pixel_type = vimsk.pixel_type = VideoInfo::CS_Y8; break;
        case 10: vipad.pixel_type = vimsk.pixel_type = VideoInfo::CS_Y10; break;
        case 12: vipad.pixel_type = vimsk.pixel_type = VideoInfo::CS_Y12; break;
        case 14: vipad.pixel_type = vimsk.pixel_type = VideoInfo::CS_Y14; break;
        default: vipad.pixel_type = vimsk.pixel_type = VideoInfo::CS_Y16; break;
    }

    widthPad = 64 / vi.ComponentSize();
    vipad.width = vimsk.width = vi.width + widthPad * 2;
    vimsk.height = vi.height * 2;
}

PVideoFrame __stdcall CreateMM::GetFrame(int n, IScriptEnvironment* env)
{
    PVideoFrame src[3];
    PVideoFrame pad[3], msk[3][2];    
    
    for (int i = 0; i < 3; i++)
    {
        src[i] = child->GetFrame(std::min(n + i, vi.num_frames - 1), env);
        pad[i] = env->NewVideoFrame(vipad);
        msk[i][0] = env->NewVideoFrame(vimsk);
        msk[i][1] = env->NewVideoFrame(vimsk);
    }

    PVideoFrame dst[] = { env->NewVideoFrame(vimsk), has_at_least_v8 ? env->NewVideoFrameP(vi, &src[0]) : env->NewVideoFrame(vi) };

    const int comp_size = vi.ComponentSize();
    for (int plane = 0; plane < planecount; plane++)
    {
        const int plane_ = planes[plane];

        if (process[plane] == 3)
        {
            if (comp_size == 1)
            {
                if (opt_ == 3)
                {
                    for (int i = 0; i < 3; i++)
                    {
                        copyPad<uint8_t>(src[i], pad[i], plane_, widthPad, env);
                        threshMask_avx512<uint8_t, Vec64uc, 64>(pad[i], msk[i][0], plane_, env);
                    }

                    for (int i = 0; i < 2; i++)
                        motionMask_avx512<uint8_t, Vec64uc, 64>(pad[i], msk[i][0], pad[i + 1], msk[i + 1][0], msk[i][1], plane_, env);

                    motionMask_avx512<uint8_t, Vec64uc, 64>(pad[0], msk[0][0], pad[2], msk[2][0], dst[0], plane_, env);
                    andMasks_avx512<uint8_t, Vec64uc, 64>(msk[0][1], msk[1][1], dst[0], plane_, env);
                    combineMasks_avx512<uint8_t, Vec64uc, 64>(dst[0], dst[1], plane_, env);
                }
                else if ((!!(env->GetCPUFlags() & CPUF_AVX2) && opt_ < 0) || opt_ == 2)
                {
                    for (int i = 0; i < 3; i++)
                    {
                        copyPad<uint8_t>(src[i], pad[i], plane_, widthPad, env);
                        threshMask_avx2<uint8_t, Vec32uc, 32>(pad[i], msk[i][0], plane_, env);
                    }

                    for (int i = 0; i < 2; i++)
                        motionMask_avx2<uint8_t, Vec32uc, 32>(pad[i], msk[i][0], pad[i + 1], msk[i + 1][0], msk[i][1], plane_, env);

                    motionMask_avx2<uint8_t, Vec32uc, 32>(pad[0], msk[0][0], pad[2], msk[2][0], dst[0], plane_, env);
                    andMasks_avx2<uint8_t, Vec32uc, 32>(msk[0][1], msk[1][1], dst[0], plane_, env);
                    combineMasks_avx2<uint8_t, Vec32uc, 32>(dst[0], dst[1], plane_, env);
                }
                else if ((!!(env->GetCPUFlags() & CPUF_SSE2) && opt_ < 0) || opt_ == 1)
                {
                    for (int i = 0; i < 3; i++)
                    {
                        copyPad<uint8_t>(src[i], pad[i], plane_, widthPad, env);
                        threshMask_sse2<uint8_t, Vec16uc, 16>(pad[i], msk[i][0], plane_, env);
                    }

                    for (int i = 0; i < 2; i++)
                        motionMask_sse2<uint8_t, Vec16uc, 16>(pad[i], msk[i][0], pad[i + 1], msk[i + 1][0], msk[i][1], plane_, env);

                    motionMask_sse2<uint8_t, Vec16uc, 16>(pad[0], msk[0][0], pad[2], msk[2][0], dst[0], plane_, env);
                    andMasks_sse2<uint8_t, Vec16uc, 16>(msk[0][1], msk[1][1], dst[0], plane_, env);
                    combineMasks_sse2<uint8_t, Vec16uc, 16>(dst[0], dst[1], plane_, env);
                }
                else
                {
                    for (int i = 0; i < 3; i++)
                    {
                        copyPad<uint8_t>(src[i], pad[i], plane_, widthPad, env);
                        threshMask_c<uint8_t>(pad[i], msk[i][0], plane_, env);
                    }

                    for (int i = 0; i < 2; i++)
                        motionMask_c<uint8_t>(pad[i], msk[i][0], pad[i + 1], msk[i + 1][0], msk[i][1], plane_, env);

                    motionMask_c<uint8_t>(pad[0], msk[0][0], pad[2], msk[2][0], dst[0], plane_, env);
                    andMasks_c<uint8_t>(msk[0][1], msk[1][1], dst[0], plane_, env);
                    combineMasks_c<uint8_t>(dst[0], dst[1], plane_, env);
                }
            }
            else
            {
                if (opt_ == 3)
                {
                    for (int i = 0; i < 3; i++)
                    {
                        copyPad<uint16_t>(src[i], pad[i], plane_, widthPad, env);
                        threshMask_avx512<uint16_t, Vec32us, 32>(pad[i], msk[i][0], plane_, env);
                    }

                    for (int i = 0; i < 2; i++)
                        motionMask_avx512<uint16_t, Vec32us, 32>(pad[i], msk[i][0], pad[i + 1], msk[i + 1][0], msk[i][1], plane_, env);

                    motionMask_avx512<uint16_t, Vec32us, 32>(pad[0], msk[0][0], pad[2], msk[2][0], dst[0], plane_, env);
                    andMasks_avx512<uint16_t, Vec32us, 32>(msk[0][1], msk[1][1], dst[0], plane_, env);
                    combineMasks_avx512<uint16_t, Vec32us, 32>(dst[0], dst[1], plane_, env);
                }
                else if ((!!(env->GetCPUFlags() & CPUF_AVX2) && opt_ < 0) || opt_ == 2)
                {
                    for (int i = 0; i < 3; i++)
                    {
                        copyPad<uint16_t>(src[i], pad[i], plane_, widthPad, env);
                        threshMask_avx2<uint16_t, Vec16us, 16>(pad[i], msk[i][0], plane_, env);
                    }

                    for (int i = 0; i < 2; i++)
                        motionMask_avx2<uint16_t, Vec16us, 16>(pad[i], msk[i][0], pad[i + 1], msk[i + 1][0], msk[i][1], plane_, env);

                    motionMask_avx2<uint16_t, Vec16us, 16>(pad[0], msk[0][0], pad[2], msk[2][0], dst[0], plane_, env);
                    andMasks_avx2<uint16_t, Vec16us, 16>(msk[0][1], msk[1][1], dst[0], plane_, env);
                    combineMasks_avx2<uint16_t, Vec16us, 16>(dst[0], dst[1], plane_, env);
                }
                else if ((!!(env->GetCPUFlags() & CPUF_SSE2) && opt_ < 0) || opt_ == 1)
                {
                    for (int i = 0; i < 3; i++)
                    {
                        copyPad<uint16_t>(src[i], pad[i], plane_, widthPad, env);
                        threshMask_sse2<uint16_t, Vec8us, 8>(pad[i], msk[i][0], plane_, env);
                    }

                    for (int i = 0; i < 2; i++)
                        motionMask_sse2<uint16_t, Vec8us, 8>(pad[i], msk[i][0], pad[i + 1], msk[i + 1][0], msk[i][1], plane_, env);

                    motionMask_sse2<uint16_t, Vec8us, 8>(pad[0], msk[0][0], pad[2], msk[2][0], dst[0], plane_, env);
                    andMasks_sse2<uint16_t, Vec8us, 8>(msk[0][1], msk[1][1], dst[0], plane_, env);
                    combineMasks_sse2<uint16_t, Vec8us, 8>(dst[0], dst[1], plane_, env);
                }
                else
                {
                    for (int i = 0; i < 3; i++)
                    {
                        copyPad<uint16_t>(src[i], pad[i], plane_, widthPad, env);
                        threshMask_c<uint16_t>(pad[i], msk[i][0], plane_, env);
                    }

                    for (int i = 0; i < 2; i++)
                        motionMask_c<uint16_t>(pad[i], msk[i][0], pad[i + 1], msk[i + 1][0], msk[i][1], plane_, env);

                    motionMask_c<uint16_t>(pad[0], msk[0][0], pad[2], msk[2][0], dst[0], plane_, env);
                    andMasks_c<uint16_t>(msk[0][1], msk[1][1], dst[0], plane_, env);
                    combineMasks_c<uint16_t>(dst[0], dst[1], plane_, env);
                }
            }
        }
    }

    return dst[1];
}
