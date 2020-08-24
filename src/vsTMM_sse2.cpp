#include <limits>

#include "vsTMM.h"

template<typename T>
static inline T abs_dif(const T& a, const T& b) noexcept
{
    return sub_saturated(a, b) | sub_saturated(b, a);
}

template<typename T1, typename T2, int step>
void CreateMM::threshMask_sse2(PVideoFrame& src, PVideoFrame& dst, int plane, IScriptEnvironment* env) noexcept
{ 
    switch (plane)
    {
        case PLANAR_Y: plane = 0; break;
        case PLANAR_U: plane = 1; break;
        default: plane = 2; break;
    }
        
    const int height = vi.height >> (plane ? vi.GetPlaneHeightSubsampling(PLANAR_U) : 0);
    const int dst_stride = dst->GetPitch(PLANAR_Y) / sizeof(T1);
    T1* dstp0 = reinterpret_cast<T1*>(dst->GetWritePtr(PLANAR_Y)) + widthPad;
    T1* dstp1 = dstp0 + dst_stride * static_cast<int64_t>(height);

    if (plane == 0 && mtqL_ > -1 && mthL_ > -1)
    {
        std::fill_n(dstp0 - widthPad, dst_stride * height, static_cast<T1>(mtqL_));
        std::fill_n(dstp1 - widthPad, dst_stride * height, static_cast<T1>(mthL_));
        return;
    }
    else if (plane > 0 && mtqC_ > -1 && mthC_ > -1)
    {
        std::fill_n(dstp0 - widthPad, dst_stride * height, static_cast<T1>(mtqC_));
        std::fill_n(dstp1 - widthPad, dst_stride * height, static_cast<T1>(mthC_));
        return;
    }

    const int stride = src->GetPitch(PLANAR_Y) / sizeof(T1);
    const T1* srcp = reinterpret_cast<const T1*>(src->GetReadPtr(PLANAR_Y)) + widthPad;
    const T1* srcpp = srcp + stride;
    const T1* srcpn = srcpp;
    const int width = vi.width >> (plane ? vi.GetPlaneWidthSubsampling(PLANAR_U) : 0);

    constexpr T1 peak = std::numeric_limits<T1>::max();

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x += step)
        {
            const T2 topLeft = T2().load(srcpp + x - 1);
            const T2 top = T2().load_a(srcpp + x);
            const T2 topRight = T2().load(srcpp + x + 1);
            const T2 left = T2().load(srcp + x - 1);
            const T2 center = T2().load_a(srcp + x);
            const T2 right = T2().load(srcp + x + 1);
            const T2 bottomLeft = T2().load(srcpn + x - 1);
            const T2 bottom = T2().load_a(srcpn + x);
            const T2 bottomRight = T2().load(srcpn + x + 1);

            T2 min0 = peak, max0 = _mm_setzero_si128();
            T2 min1 = peak, max1 = _mm_setzero_si128();

            switch (ttype_)
            {
                case 0: // 4 neighbors - compensated
                {
                    min0 = min(min0, top);
                    max0 = max(max0, top);
                    min1 = min(min1, left);
                    max1 = max(max1, left);
                    min1 = min(min1, right);
                    max1 = max(max1, right);
                    min0 = min(min0, bottom);
                    max0 = max(max0, bottom);

                    const T2 atv = max((abs_dif<T2>(center, min0) + vHalf[plane]) >> vShift[plane], (abs_dif<T2>(center, max0) + vHalf[plane]) >> vShift[plane]);
                    const T2 ath = max((abs_dif<T2>(center, min1) + hHalf[plane]) >> hShift[plane], (abs_dif<T2>(center, max1) + hHalf[plane]) >> hShift[plane]);
                    const T2 atmax = max(atv, ath);
                    ((atmax + 2) >> 2).store_nt(dstp0 + x);
                    ((atmax + 1) >> 1).store_nt(dstp1 + x);
                }
                break;
                case 1: // 8 neighbors - compensated
                {
                    min0 = min(min0, topLeft);
                    max0 = max(max0, topLeft);
                    min0 = min(min0, top);
                    max0 = max(max0, top);
                    min0 = min(min0, topRight);
                    max0 = max(max0, topRight);
                    min1 = min(min1, left);
                    max1 = max(max1, left);
                    min1 = min(min1, right);
                    max1 = max(max1, right);
                    min0 = min(min0, bottomLeft);
                    max0 = max(max0, bottomLeft);
                    min0 = min(min0, bottom);
                    max0 = max(max0, bottom);
                    min0 = min(min0, bottomRight);
                    max0 = max(max0, bottomRight);

                    const T2 atv = max((abs_dif<T2>(center, min0) + vHalf[plane]) >> vShift[plane], (abs_dif<T2>(center, max0) + vHalf[plane]) >> vShift[plane]);
                    const T2 ath = max((abs_dif<T2>(center, min1) + hHalf[plane]) >> hShift[plane], (abs_dif<T2>(center, max1) + hHalf[plane]) >> hShift[plane]);
                    const T2 atmax = max(atv, ath);
                    ((atmax + 2) >> 2).store_nt(dstp0 + x);
                    ((atmax + 1) >> 1).store_nt(dstp1 + x);
                }
                break;
                case 2: // 4 neighbors - not compensated
                {
                    min0 = min(min0, top);
                    max0 = max(max0, top);
                    min0 = min(min0, left);
                    max0 = max(max0, left);
                    min0 = min(min0, right);
                    max0 = max(max0, right);
                    min0 = min(min0, bottom);
                    max0 = max(max0, bottom);

                    const T2 at = max(abs_dif<T2>(center, min0), abs_dif<T2>(center, max0));
                    ((at + 2) >> 2).store_nt(dstp0 + x);
                    ((at + 1) >> 1).store_nt(dstp1 + x);
                }
                break;
                case 3: // 8 neighbors - not compensated
                {
                    min0 = min(min0, topLeft);
                    max0 = max(max0, topLeft);
                    min0 = min(min0, top);
                    max0 = max(max0, top);
                    min0 = min(min0, topRight);
                    max0 = max(max0, topRight);
                    min0 = min(min0, left);
                    max0 = max(max0, left);
                    min0 = min(min0, right);
                    max0 = max(max0, right);
                    min0 = min(min0, bottomLeft);
                    max0 = max(max0, bottomLeft);
                    min0 = min(min0, bottom);
                    max0 = max(max0, bottom);
                    min0 = min(min0, bottomRight);
                    max0 = max(max0, bottomRight);

                    const T2 at = max(abs_dif<T2>(center, min0), abs_dif<T2>(center, max0));
                    ((at + 2) >> 2).store_nt(dstp0 + x);
                    ((at + 1) >> 1).store_nt(dstp1 + x);
                }
                break;
                case 4:
                { // 4 neighbors - not compensated (range)
                    min0 = min(min0, top);
                    max0 = max(max0, top);
                    min0 = min(min0, left);
                    max0 = max(max0, left);
                    min0 = min(min0, center);
                    max0 = max(max0, center);
                    min0 = min(min0, right);
                    max0 = max(max0, right);
                    min0 = min(min0, bottom);
                    max0 = max(max0, bottom);

                    const T2 at = max0 - min0;
                    ((at + 2) >> 2).store_nt(dstp0 + x);
                    ((at + 1) >> 1).store_nt(dstp1 + x);
                }
                break;
                default: // 8 neighbors - not compensated (range)
                {
                    min0 = min(min0, topLeft);
                    max0 = max(max0, topLeft);
                    min0 = min(min0, top);
                    max0 = max(max0, top);
                    min0 = min(min0, topRight);
                    max0 = max(max0, topRight);
                    min0 = min(min0, left);
                    max0 = max(max0, left);
                    min0 = min(min0, center);
                    max0 = max(max0, center);
                    min0 = min(min0, right);
                    max0 = max(max0, right);
                    min0 = min(min0, bottomLeft);
                    max0 = max(max0, bottomLeft);
                    min0 = min(min0, bottom);
                    max0 = max(max0, bottom);
                    min0 = min(min0, bottomRight);
                    max0 = max(max0, bottomRight);

                    const T2 at = max0 - min0;
                    ((at + 2) >> 2).store_nt(dstp0 + x);
                    ((at + 1) >> 1).store_nt(dstp1 + x);
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

    T1* dstp = reinterpret_cast<T1*>(dst->GetWritePtr(PLANAR_Y));

    if (plane == 0 && mtqL_ > -1)
        std::fill_n(dstp, dst_stride * height, static_cast<T1>(mtqL_));
    else if (plane == 0 && mthL_ > -1)
        std::fill_n(dstp + dst_stride * static_cast<int64_t>(height), dst_stride * height, static_cast<T1>(mthL_));
    else if (plane > 0 && mtqC_ > -1)
        std::fill_n(dstp, dst_stride * height, static_cast<T1>(mtqC_));
    else if (plane > 0 && mthC_ > -1)
        std::fill_n(dstp + dst_stride * static_cast<int64_t>(height), dst_stride * height, static_cast<T1>(mthC_));
}

template void CreateMM::threshMask_sse2<uint8_t, Vec16uc, 16>(PVideoFrame& src, PVideoFrame& dst, int plane, IScriptEnvironment* env) noexcept;
template void CreateMM::threshMask_sse2<uint16_t, Vec8us, 8>(PVideoFrame& src, PVideoFrame& dst, int plane, IScriptEnvironment* env) noexcept;

template<typename T1, typename T2, int step>
void CreateMM::motionMask_sse2(PVideoFrame& src1, PVideoFrame& msk1, PVideoFrame& src2, PVideoFrame& msk2, PVideoFrame& dst, const int plane, IScriptEnvironment* env) noexcept
{    
    const int height = vi.height >> (plane != PLANAR_Y ? vi.GetPlaneHeightSubsampling(PLANAR_U) : 0);
    const int msk1_stride = msk1->GetPitch(PLANAR_Y) / sizeof(T1);
    const int msk2_stride = msk2->GetPitch(PLANAR_Y) / sizeof(T1);
    const int dst_stride = dst->GetPitch(PLANAR_Y) / sizeof(T1);
    const T1* mskp1q = reinterpret_cast<const T1*>(msk1->GetReadPtr(PLANAR_Y)) + widthPad;
    const T1* mskp2q = reinterpret_cast<const T1*>(msk2->GetReadPtr(PLANAR_Y)) + widthPad;
    T1* dstpq = reinterpret_cast<T1*>(dst->GetWritePtr(PLANAR_Y)) + widthPad;
    const T1* mskp1h = mskp1q + msk1_stride * static_cast<int64_t>(height);
    const T1* mskp2h = mskp2q + msk2_stride * static_cast<int64_t>(height);
    T1* dstph = dstpq + dst_stride * static_cast<int64_t>(height);
    const int stride = src1->GetPitch(PLANAR_Y) / sizeof(T1);
    const int src2_stride = src2->GetPitch(PLANAR_Y) / sizeof(T1);
    const int width = vi.width >> (plane != PLANAR_Y ? vi.GetPlaneWidthSubsampling(PLANAR_U) : 0);
    const T1* srcp1 = reinterpret_cast<const T1*>(src1->GetReadPtr(PLANAR_Y)) + widthPad;
    const T1* srcp2 = reinterpret_cast<const T1*>(src2->GetReadPtr(PLANAR_Y)) + widthPad;

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x += step)
        {
            const T2 diff = abs_dif<T2>(T2().load_a(srcp1 + x), T2().load_a(srcp2 + x));
            const T2 minq = min(T2().load_a(mskp1q + x), T2().load_a(mskp2q + x));
            const T2 minh = min(T2().load_a(mskp1h + x), T2().load_a(mskp2h + x));
            const T2 threshq = min(max(add_saturated(minq, nt_), minthresh_), maxthresh_);
            const T2 threshh = min(max(add_saturated(minh, nt_), minthresh_), maxthresh_);
            select(diff <= threshq, T2(1), _mm_setzero_si128()).store_nt(dstpq + x);
            select(diff <= threshh, T2(1), _mm_setzero_si128()).store_nt(dstph + x);
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

template void CreateMM::motionMask_sse2<uint8_t, Vec16uc, 16>(PVideoFrame& src1, PVideoFrame& msk1, PVideoFrame& src2, PVideoFrame& msk2, PVideoFrame& dst, const int plane, IScriptEnvironment* env) noexcept;
template void CreateMM::motionMask_sse2<uint16_t, Vec8us, 8>(PVideoFrame& src1, PVideoFrame& msk1, PVideoFrame& src2, PVideoFrame& msk2, PVideoFrame& dst, const int plane, IScriptEnvironment* env) noexcept;

template<typename T1, typename T2, int step>
void CreateMM::andMasks_sse2(PVideoFrame& src1, PVideoFrame& src2, PVideoFrame& dst, const int plane, IScriptEnvironment* env) noexcept
{
    const int width = vi.width >> (plane != PLANAR_Y ? vi.GetPlaneWidthSubsampling(PLANAR_U) : 0);
    const int height = (vi.height * 2) >> (plane != PLANAR_Y ? vi.GetPlaneHeightSubsampling(PLANAR_U) : 0);
    const int stride = src1->GetPitch(PLANAR_Y) / sizeof(T1);
    const int src2_stride = src2->GetPitch(PLANAR_Y) / sizeof(T1);
    const int dst_stride = dst->GetPitch(PLANAR_Y) / sizeof(T1);
    const T1* srcp1 = reinterpret_cast<const T1*>(src1->GetReadPtr(PLANAR_Y)) + widthPad;
    const T1* srcp2 = reinterpret_cast<const T1*>(src2->GetReadPtr(PLANAR_Y)) + widthPad;
    T1* dstp = reinterpret_cast<T1*>(dst->GetWritePtr(PLANAR_Y)) + widthPad;

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x += step)
            (T2().load_a(srcp1 + x) & T2().load_a(srcp2 + x) & T2().load_a(dstp + x)).store_nt(dstp + x);

        dstp[-1] = dstp[1];
        dstp[width] = dstp[width - 2];

        srcp1 += stride;
        srcp2 += src2_stride;
        dstp += dst_stride;
    }
}

template void CreateMM::andMasks_sse2<uint8_t, Vec16uc, 16>(PVideoFrame& src1, PVideoFrame& src2, PVideoFrame& dst, const int plane, IScriptEnvironment* env) noexcept;
template void CreateMM::andMasks_sse2<uint16_t, Vec8us, 8>(PVideoFrame& src1, PVideoFrame& src2, PVideoFrame& dst, const int plane, IScriptEnvironment* env) noexcept;

template<typename T1, typename T2, int step>
void CreateMM::combineMasks_sse2(PVideoFrame& src, PVideoFrame& dst, const int plane, IScriptEnvironment* env) noexcept
{
    const int srcStride = src->GetPitch(PLANAR_Y) / sizeof(T1);
    const int height = dst->GetHeight(plane);
    const T1* srcp0 = reinterpret_cast<const T1*>(src->GetReadPtr(PLANAR_Y)) + widthPad;
    const T1* srcpp0 = srcp0 + srcStride;
    const T1* srcpn0 = srcpp0;
    const T1* srcp1 = srcp0 + srcStride * static_cast<int64_t>(height);
    const int width = dst->GetRowSize(plane) / sizeof(T1);    
    const int dstStride = dst->GetPitch(plane) / sizeof(T1);
    T1* dstp = reinterpret_cast<T1*>(dst->GetWritePtr(plane));

    constexpr T1 peak = std::numeric_limits<T1>::max();

    env->BitBlt(dst->GetWritePtr(plane), dst->GetPitch(plane), src->GetReadPtr(PLANAR_Y) + widthPad * sizeof(T1), src->GetPitch(PLANAR_Y), dst->GetRowSize(plane), height);

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x += step)
        {
            const T2 count = T2().load(srcpp0 + x - 1) + T2().load_a(srcpp0 + x) + T2().load(srcpp0 + x + 1) +
                T2().load(srcp0 + x - 1) + T2().load(srcp0 + x + 1) +
                T2().load(srcpn0 + x - 1) + T2().load_a(srcpn0 + x) + T2().load(srcpn0 + x + 1);
            select(T2().load_a(srcp0 + x) == T2(_mm_setzero_si128()) && T2().load_a(srcp1 + x) != T2(_mm_setzero_si128()) && count >= cstr_, peak, T2().load_a(dstp + x)).store_nt(dstp + x);
        }

        srcpp0 = srcp0;
        srcp0 = srcpn0;
        srcpn0 += (y < height - 2) ? srcStride : -srcStride;
        srcp1 += srcStride;
        dstp += dstStride;
    }
}

template void CreateMM::combineMasks_sse2<uint8_t, Vec16uc, 16>(PVideoFrame& src, PVideoFrame& dst, const int plane, IScriptEnvironment* env) noexcept;
template void CreateMM::combineMasks_sse2<uint16_t, Vec8us, 8>(PVideoFrame& src, PVideoFrame& dst, const int plane, IScriptEnvironment* env) noexcept;
