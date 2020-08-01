#include "vsTMM.h"

template<typename T>
void BuildMM::buildMask(PVideoFrame* cSrc, PVideoFrame* oSrc, PVideoFrame& dst, const int cCount, const int oCount, const int order, const int field, IScriptEnvironment* env) noexcept
{
    const uint16_t* tmmlut = tmmlut16.data() + order * static_cast<int64_t>(8) + field * static_cast<int64_t>(4);
    uint16_t tmmlutf[64];

    for (int i = 0; i < 64; i++)
        tmmlutf[i] = tmmlut[vlut[i]];

    T* __restrict plut[2];

    for (int i = 0; i < 2; i++)
        plut[i] = new T[2 * static_cast<int64_t>(length_) - 1];

    T* __restrict* __restrict ptlut[3];

    for (int i = 0; i < 3; i++)
        ptlut[i] = new T * [i & 1 ? cCount : oCount];

    const int offo = (length_ & 1) ? 0 : 1;
    const int offc = (length_ & 1) ? 1 : 0;
    const int ct = cCount / 2;

    for (int plane = 0; plane < planecount; plane++)
    {
        const int plane_ = planes[plane];

        if (process[plane] == 3)
        {
            const int stride = dst->GetPitch(plane_) / sizeof(T);
            const int width = dst->GetRowSize(plane_) / vi.ComponentSize();
            const int height = dst->GetHeight(plane_);

            for (int i = 0; i < cCount; i++)
                ptlut[1][i] = reinterpret_cast<T*>(cSrc[i]->GetWritePtr(plane_));

            for (int i = 0; i < oCount; i++)
            {
                if (field == 1)
                {
                    ptlut[0][i] = reinterpret_cast<T*>(oSrc[i]->GetWritePtr(plane_));
                    ptlut[2][i] = ptlut[0][i] + stride;
                }
                else
                    ptlut[0][i] = ptlut[2][i] = reinterpret_cast<T*>(oSrc[i]->GetWritePtr(plane_));
            }

            T* __restrict dstp = reinterpret_cast<T*>(dst->GetWritePtr(plane_));

            if (field == 1)
            {
                for (int j = 0; j < height; j += 2)
                    std::fill_n(dstp + stride * static_cast<int64_t>(j), width, static_cast<T>(ten));
                dstp += stride;
            }
            else
                for (int j = 1; j < height; j += 2)
                    std::fill_n(dstp + stride * static_cast<int64_t>(j), width, static_cast<T>(ten));

            for (int y = field; y < height; y += 2)
            {
                for (int x = 0; x < width; x++)
                {
                    if (!ptlut[1][ct - 2][x] && !ptlut[1][ct][x] && !ptlut[1][ct + 1][x])
                    {
                        dstp[x] = static_cast<T>(sixty);
                        continue;
                    }

                    for (int j = 0; j < cCount; j++)
                        plut[0][j * 2 + offc] = plut[1][j * 2 + offc] = ptlut[1][j][x];

                    for (int j = 0; j < oCount; j++)
                    {
                        plut[0][j * 2 + offo] = ptlut[0][j][x];
                        plut[1][j * 2 + offo] = ptlut[2][j][x];
                    }

                    int val = 0;

                    for (int i = 0; i < length_; i++)
                    {
                        for (int j = 0; j < length_ - 4; j++)
                        {
                            if (!plut[0][i + j])
                                goto j1;
                        }
                        val |= gvlut[i] * 8;
                    j1:
                        for (int j = 0; j < length_ - 4; j++)
                        {
                            if (!plut[1][i + j])
                                goto j2;
                        }
                        val |= gvlut[i];
                    j2:
                        if (vlut[val] == 2)
                            break;
                    }
                    dstp[x] = static_cast<T>(tmmlutf[val]);
                }

                for (int i = 0; i < cCount; i++)
                    ptlut[1][i] += stride;

                for (int i = 0; i < oCount; i++)
                {
                    if (y != 0)
                        ptlut[0][i] += stride;
                    if (y != height - 3)
                        ptlut[2][i] += stride;
                }

                dstp += stride * static_cast<int64_t>(2);
            }
        }
    }

    for (int i = 0; i < 2; i++)
        delete[] plut[i];
    for (int i = 0; i < 3; i++)
        delete[] ptlut[i];
}

BuildMM::BuildMM(PClip _child, PClip bf, int mode, int order, int field, int length, int mtype, int y, int u, int v, IScriptEnvironment* env)
    : Common(_child, y, u, v, env), mode_(mode), order_(order), field_(field), length_(length), mtype_(mtype), bf_(bf)
{
    viSaved = vi;
    vi.SetFieldBased(false);
    vi.height *= 2;

    if (order_ == -1)
        order_ = child->GetParity(0) ? 1 : 0;
    if (field_ == -1)
        field_ = order_;
    if (mode == 1)
    {
        vi.num_frames *= 2;
        vi.fps_numerator *= 2;
    }

    gvlut = new uint8_t[length_];

    for (int i = 0; i < length_; i++)
        gvlut[i] = (i == 0) ? 1 : (i == length_ - 1 ? 4 : 2);

    switch (mtype_)
    {
        case 0:
            vlut =
            {
                    0, 1, 2, 2, 3, 0, 2, 2,
                    1, 1, 2, 2, 0, 1, 2, 2,
                    2, 2, 2, 2, 2, 2, 2, 2,
                    2, 2, 2, 2, 2, 2, 2, 2,
                    3, 0, 2, 2, 3, 3, 2, 2,
                    0, 1, 2, 2, 3, 1, 2, 2,
                    2, 2, 2, 2, 2, 2, 2, 2,
                    2, 2, 2, 2, 2, 2, 2, 2
            };
            break;
        case 1:
            vlut =
            {
                    0, 0, 2, 2, 0, 0, 2, 2,
                    0, 1, 2, 2, 0, 1, 2, 2,
                    2, 2, 2, 2, 2, 2, 2, 2,
                    2, 2, 2, 2, 2, 2, 2, 2,
                    0, 0, 2, 2, 3, 3, 2, 2,
                    0, 1, 2, 2, 3, 1, 2, 2,
                    2, 2, 2, 2, 2, 2, 2, 2,
                    2, 2, 2, 2, 2, 2, 2, 2
            };
            break;
        default:
            vlut =
            {
                    0, 0, 0, 0, 0, 0, 0, 0,
                    0, 1, 0, 1, 0, 1, 0, 1,
                    0, 0, 2, 2, 0, 0, 2, 2,
                    0, 1, 2, 2, 0, 1, 2, 2,
                    0, 0, 0, 0, 3, 3, 3, 3,
                    0, 1, 0, 1, 3, 1, 3, 1,
                    0, 0, 2, 2, 3, 3, 2, 2,
                    0, 1, 2, 2, 3, 1, 2, 2
            };
            break;
    }

    tmmlut16 =
    {
        sixty, twenty, fifty, ten, sixty, ten, forty, thirty,
        sixty, ten, forty, thirty, sixty, twenty, fifty, ten
    };
}

PVideoFrame __stdcall BuildMM::GetFrame(int n, IScriptEnvironment* env)
{
    int field = field_;

    if (mode_ == 1)
    {
        field = (n & 1) ? 1 - order_ : order_;
        n /= 2;
    }

    PVideoFrame* srct = new PVideoFrame[length_ - 2];
    PVideoFrame* srcb = new PVideoFrame[length_ - 2];
    int tStart, tStop, bStart, bStop, cCount, oCount;
    PVideoFrame* cSrc, * oSrc;

    if (field == 1)
    {
        tStart = n - (length_ - 1) / 2;
        tStop = n + (length_ - 1) / 2 - 2;
        const int bn = (order_ == 1) ? n - 1 : n;
        bStart = bn - (length_ - 2) / 2;
        bStop = bn + 1 + (length_ - 2) / 2 - 2;
        oCount = tStop - tStart + 1;
        cCount = bStop - bStart + 1;
        oSrc = srct;
        cSrc = srcb;
    }
    else
    {
        const int tn = (order_ == 0) ? n - 1 : n;
        tStart = tn - (length_ - 2) / 2;
        tStop = tn + 1 + (length_ - 2) / 2 - 2;
        bStart = n - (length_ - 1) / 2;
        bStop = n + (length_ - 1) / 2 - 2;
        cCount = tStop - tStart + 1;
        oCount = bStop - bStart + 1;
        cSrc = srct;
        oSrc = srcb;
    }

    for (int i = tStart; i <= tStop; i++)
    {
        if (i < 0 || i >= viSaved.num_frames - 2)
        {
            srct[i - tStart] = env->NewVideoFrame(viSaved);                        

            for (int plane = 0; plane < planecount; plane++)
            {
                const int plane_ = planes[plane];

                memset(srct[i - tStart]->GetWritePtr(plane_), 0, static_cast<int64_t>(srct[i - tStart]->GetPitch(plane_)) * srct[i - tStart]->GetHeight(plane_));
            }
        }
        else
             srct[i - tStart] = child->GetFrame(i, env);
    }

    for (int i = bStart; i <= bStop; i++)
    {
        if (i < 0 || i >= viSaved.num_frames - 2)
        {
            srcb[i - bStart] = env->NewVideoFrame(viSaved);

            for (int plane = 0; plane < planecount; plane++)
            {
                const int plane_ = planes[plane];

                memset(srcb[i - bStart]->GetWritePtr(plane_), 0, static_cast<int64_t>(srcb[i - bStart]->GetPitch(plane_)) * srcb[i - bStart]->GetHeight(plane_));
            }
                
        }
        else
            srcb[i - bStart] = bf_->GetFrame(i, env);
    }

    PVideoFrame dst = has_at_least_v8 ? env->NewVideoFrameP(vi, srct) : env->NewVideoFrame(vi);

    if (vi.ComponentSize() == 1)
        buildMask<uint8_t>(cSrc, oSrc, dst, cCount, oCount, order_, field, env);
    else
        buildMask<uint16_t>(cSrc, oSrc, dst, cCount, oCount, order_, field, env);

    delete[] srct;
    delete[] srcb;

    return dst;
}
