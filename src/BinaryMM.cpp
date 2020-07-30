#include <assert.h>

#include "vsTMM.h"

template<typename T>
void BinaryMM::checkSpatial(PVideoFrame& src, PVideoFrame& dst, IScriptEnvironment* env) noexcept
{
    for (int plane = 0; plane < planecount; plane++)
    {
        const int plane_ = planes[plane];

        if (process[plane] == 3)
        {
            const int stride = src->GetPitch(plane_) / sizeof(T);
            const T* srcp = reinterpret_cast<const T*>(src->GetReadPtr(plane_));
            const T* srcppp = srcp - stride * static_cast<int64_t>(2);
            const T* srcpp = srcp - stride;
            const T* srcpn = srcp + stride;
            const T* srcpnn = srcp + stride * static_cast<int64_t>(2);
            const int dst_stride = dst->GetPitch(plane_) / sizeof(T);
            const int width = src->GetRowSize(plane_) / vi.ComponentSize();
            const int height = src->GetHeight(plane_);
            T* __restrict dstp = reinterpret_cast<T*>(dst->GetWritePtr(plane_));

            if (metric_ == 0)
            {
                for (int x = 0; x < width; x++)
                {
                    const int sFirst = srcp[x] - srcpn[x];

                    if (dstp[x] == 60 && !((sFirst > athresh_ || sFirst < -athresh_) && std::abs(srcpnn[x] + srcp[x] * 4 + srcpnn[x] - 3 * (srcpn[x] + srcpn[x])) > athresh6))
                        dstp[x] = 10;
                }

                srcppp += stride;
                srcpp += stride;
                srcp += stride;
                srcpn += stride;
                srcpnn += stride;
                dstp += dst_stride;

                for (int x = 0; x < width; x++)
                {
                    const int sFirst = srcp[x] - srcpp[x];
                    const int sSecond = srcp[x] - srcpn[x];

                    if (dstp[x] == 60 && !(((sFirst > athresh_ && sSecond > athresh_) || (sFirst < -athresh_ && sSecond < -athresh_)) && std::abs(srcpnn[x] + srcp[x] * 4 + srcpnn[x] - 3 * (srcpp[x] + srcpn[x])) > athresh6))
                        dstp[x] = 10;
                }

                srcppp += stride;
                srcpp += stride;
                srcp += stride;
                srcpn += stride;
                srcpnn += stride;
                dstp += dst_stride;

                for (int y = 2; y < height - 2; y++)
                {
                    for (int x = 0; x < width; x++)
                    {
                        const int sFirst = srcp[x] - srcpp[x];
                        const int sSecond = srcp[x] - srcpn[x];

                        if (dstp[x] == 60 && !(((sFirst > athresh_ && sSecond > athresh_) || (sFirst < -athresh_ && sSecond < -athresh_)) && std::abs(srcppp[x] + srcp[x] * 4 + srcpnn[x] - 3 * (srcpp[x] + srcpn[x])) > athresh6))
                            dstp[x] = 10;
                    }

                    srcppp += stride;
                    srcpp += stride;
                    srcp += stride;
                    srcpn += stride;
                    srcpnn += stride;
                    dstp += dst_stride;
                }

                for (int x = 0; x < width; x++)
                {
                    const int sFirst = srcp[x] - srcpp[x];
                    const int sSecond = srcp[x] - srcpn[x];

                    if (dstp[x] == 60 && !(((sFirst > athresh_ && sSecond > athresh_) || (sFirst < -athresh_ && sSecond < -athresh_)) && std::abs(srcppp[x] + srcp[x] * 4 + srcppp[x] - 3 * (srcpp[x] + srcpn[x])) > athresh6))
                        dstp[x] = 10;
                }

                srcppp += stride;
                srcpp += stride;
                srcp += stride;
                srcpn += stride;
                srcpnn += stride;
                dstp += dst_stride;

                for (int x = 0; x < width; x++)
                {
                    const int sFirst = srcp[x] - srcpp[x];

                    if (dstp[x] == 60 && !((sFirst > athresh_ || sFirst < -athresh_) && std::abs(srcppp[x] + srcp[x] * 4 + srcppp[x] - 3 * (srcpp[x] + srcpp[x])) > athresh6))
                        dstp[x] = 10;
                }
            }
            else
            {
                for (int x = 0; x < width; x++)
                {
                    if (dstp[x] == 60 && !((srcp[x] - srcpn[x]) * (srcp[x] - srcpn[x]) > athreshsq))
                        dstp[x] = 10;
                }

                srcpp += stride;
                srcp += stride;
                srcpn += stride;
                dstp += dst_stride;

                for (int y = 1; y < height - 1; y++)
                {
                    for (int x = 0; x < width; x++)
                    {
                        if (dstp[x] == 60 && !((srcp[x] - srcpp[x]) * (srcp[x] - srcpn[x]) > athreshsq))
                            dstp[x] = 10;
                    }

                    srcpp += stride;
                    srcp += stride;
                    srcpn += stride;
                    dstp += dst_stride;
                }

                for (int x = 0; x < width; x++)
                {
                    if (dstp[x] == 60 && !((srcp[x] - srcpp[x]) * (srcp[x] - srcpp[x]) > athreshsq))
                        dstp[x] = 10;
                }
            }
        }
    }
}

template<typename T>
void BinaryMM::expandMask(PVideoFrame& mask, const int field, IScriptEnvironment* env) noexcept
{
    for (int plane = 0; plane < planecount; plane++)
    {
        const int plane_ = planes[plane];

        if (process[plane] == 3)
        {
            const int stride = mask->GetPitch(plane_) / sizeof(T) * 2;
            const int width = mask->GetRowSize(plane_) / vi.ComponentSize();
            const int height = mask->GetHeight(plane_);            
            T* __restrict maskp = reinterpret_cast<T*>(mask->GetWritePtr(plane_)) + stride / static_cast<int64_t>(2) * field;

            const int dis = expand_ >> (plane ? vi.GetPlaneWidthSubsampling(PLANAR_U) : 0);

            for (int y = field; y < height; y += 2)
            {
                for (int x = 0; x < width; x++)
                {
                    if (maskp[x] == 60)
                    {
                        int xt = x - 1;

                        while (xt >= 0 && xt >= x - dis)
                            maskp[xt--] = 60;

                        xt = x + 1;

                        int nc = x + dis + 1;

                        while (xt < width && xt <= x + dis)
                        {
                            if (maskp[xt] == 60)
                            {
                                nc = xt;
                                break;
                            }
                            else
                                maskp[xt++] = 60;
                        }

                        x = nc - 1;
                    }
                }

                maskp += stride;
            }
        }
    }
}

template<typename T>
void BinaryMM::linkMask(PVideoFrame& mask, const int field, IScriptEnvironment* env) noexcept
{
    const int strideY = mask->GetPitch(PLANAR_Y) / sizeof(T);
    const int strideUV = mask->GetPitch(PLANAR_U) / sizeof(T);
    const int strideY2 = strideY * (2 << vi.GetPlaneHeightSubsampling(PLANAR_U));
    const int strideUV2 = strideUV * 2;
    const int width = mask->GetRowSize(PLANAR_U) / vi.ComponentSize();
    const int height = mask->GetHeight(PLANAR_U);
    const T* maskpY = reinterpret_cast<const T*>(mask->GetReadPtr(PLANAR_Y)) + static_cast<int64_t>(strideY) * field;
    const T* maskpnY = maskpY + strideY * static_cast<int64_t>(2);
    T* __restrict maskpU = reinterpret_cast<T*>(mask->GetWritePtr(PLANAR_U)) + static_cast<int64_t>(strideUV) * field;
    T* __restrict maskpV = reinterpret_cast<T*>(mask->GetWritePtr(PLANAR_V)) + static_cast<int64_t>(strideUV) * field;

    for (int y = field; y < height; y += 2)
    {
        for (int x = 0; x < width; x++)
        {
            if (vi.GetPlaneWidthSubsampling(PLANAR_U) == 0)
            {
                if (vi.GetPlaneHeightSubsampling(PLANAR_U) == 0)
                {
                    if (maskpY[x] == 0x3C)
                        maskpU[x] = maskpV[x] = 0x3C;
                }
                else
                {
                    if (maskpY[x] == 0x3C && maskpnY[x] == 0x3C)
                        maskpU[x] = maskpV[x] = 0x3C;
                }
            }
            else
            {
                if (std::is_same<T, uint8_t>::value)
                {
                    if (vi.GetPlaneHeightSubsampling(PLANAR_U) == 0)
                    {
                        if (reinterpret_cast<const uint16_t*>(maskpY)[x] == 0x3C3C)
                            maskpU[x] = maskpV[x] = 0x3C;
                    }
                    else
                    {
                        if (reinterpret_cast<const uint16_t*>(maskpY)[x] == 0x3C3C && reinterpret_cast<const uint16_t*>(maskpnY)[x] == 0x3C3C)
                            maskpU[x] = maskpV[x] = 0x3C;
                    }
                }
                else
                {
                    if (vi.GetPlaneHeightSubsampling(PLANAR_U) == 0)
                    {
                        if (reinterpret_cast<const uint32_t*>(maskpY)[x] == 0x3C003C)
                            maskpU[x] = maskpV[x] = 0x3C;
                    }
                    else
                    {
                        if (reinterpret_cast<const uint32_t*>(maskpY)[x] == 0x3C003C && reinterpret_cast<const uint32_t*>(maskpnY)[x] == 0x3C003C)
                            maskpU[x] = maskpV[x] = 0x3C;
                    }
                }
            }
        }

        maskpY += strideY2;
        maskpnY += strideY2;
        maskpU += strideUV2;
        maskpV += strideUV2;
    }
}

template<typename T>
void BinaryMM::binaryMask(PVideoFrame& src, PVideoFrame& dst, IScriptEnvironment* env) noexcept
{
    for (int plane = 0; plane < planecount; plane++)
    {
        const int plane_ = planes[plane];

        if (process[plane] == 3)
        {
            const int stride = src->GetPitch(plane_) / sizeof(T);
            const int dst_stride = dst->GetPitch(plane_) / sizeof(T);
            const int width = src->GetRowSize(plane_) / vi.ComponentSize();
            const int height = src->GetHeight(plane_);
            const T* srcp = reinterpret_cast<const T*>(src->GetReadPtr(plane_));
            T* __restrict dstp = reinterpret_cast<T*>(dst->GetWritePtr(plane_));

            for (int y = 0; y < height; y++)
            {
                for (int x = 0; x < width; x++)
                    dstp[x] = (srcp[x] == 60) ? peak : 0;

                srcp += stride;
                dstp += dst_stride;
            }
        }
    }
}

BinaryMM::BinaryMM(PClip _child, int athresh, int metric, int expand, bool link, PClip clip, int mode, int order, int field, int y, int u, int v, IScriptEnvironment* env)
    : Common(_child, y, u, v, env), athresh_(athresh), metric_(metric), expand_(expand), link_(link), mode_(mode), order_(order), field_(field), clip_(clip)
{
    peak = (1 << vi.BitsPerComponent()) - 1;
    athresh_ = athresh_ > -1 ? athresh_ * peak / 255 : -1;
    athresh6 = athresh_ > -1 ? athresh_ * 6 : -1;
    athreshsq = athresh_ > -1 ? athresh_ * athresh_ : -1;

    if (planecount == 1 || y_ != 3)
        link_ = false;
}

PVideoFrame __stdcall BinaryMM::GetFrame(int n, IScriptEnvironment* env)
{
    int field = field_;

    if (mode_ == 1)
        field = (n & 1) ? 1 - order_ : order_;

    PVideoFrame src = clip_->GetFrame(n, env);
    PVideoFrame dst = has_at_least_v8 ? env->NewVideoFrameP(vi, &src) : env->NewVideoFrame(vi);

    for (int i = 0; i < planecount; ++i)
    {
        const int plane = planes[i];

        if (process[i] == 2)
            env->BitBlt(dst->GetWritePtr(plane), dst->GetPitch(plane), src->GetReadPtr(plane), src->GetPitch(plane), src->GetRowSize(plane), src->GetHeight(plane));
    }

    PVideoFrame mask = child->GetFrame(n, env);

    if (vi.ComponentSize() == 1)
    {
        if (athresh_ > -1)
            checkSpatial<uint8_t>(src, mask, env);
        if (expand_)
            expandMask<uint8_t>(mask, field, env);
        if (link_)
            linkMask<uint8_t>(mask, field, env);

        binaryMask<uint8_t>(mask, dst, env);
    }
    else
    {
        if (athresh_ > -1)
            checkSpatial<uint16_t>(src, mask, env);
        if (expand_)
            expandMask<uint16_t>(mask, field, env);
        if (link_)
            linkMask<uint16_t>(mask, field, env);

        binaryMask<uint16_t>(mask, dst, env);
    }

    return dst;
}
