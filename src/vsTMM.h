#pragma once

#include <array>

#include "avisynth.h"
#include "vectorclass/vectorclass.h"

class Common : public GenericVideoFilter
{
protected:
    int y_, u_, v_;
    int process[3] = { 0 };
    bool has_at_least_v8;
    const int planes[4] = { PLANAR_Y, PLANAR_U, PLANAR_V, PLANAR_A };
    const int planecount = std::min(vi.NumComponents(), 3);
    uint16_t ten, twenty, thirty, forty, fifty, sixty, seventy;

public:
    Common(PClip clip, int y, int u, int v, IScriptEnvironment* env)
        : GenericVideoFilter(clip), y_(y), u_(u), v_(v)
    {
        for (int i = 0; i < planecount; i++)
        {
            switch (i)
            {
                case 0:
                    switch (y_)
                    {
                        case 3: process[i] = 3; break;
                        case 2: process[i] = 2; break;
                        default: process[i] = 1; break;
                    }
                    break;
                case 1:
                    switch (u_)
                    {
                        case 3: process[i] = 3; break;
                        case 2: process[i] = 2; break;
                        default: process[i] = 1; break;
                    }
                    break;
                default:
                    switch (v_)
                    {
                        case 3: process[i] = 3; break;
                        case 2: process[i] = 2; break;
                        default: process[i] = 1; break;
                    }
                    break;
            }
        }

        has_at_least_v8 = true;
        try { env->CheckVersion(8); }
        catch (const AvisynthError&) { has_at_least_v8 = false; }

        const int peak = (1 << vi.BitsPerComponent()) - 1;
        ten = 10 * peak / 255;
        twenty = 20 * peak / 255;
        thirty = 30 * peak / 255;
        forty = 40 * peak / 255;
        fifty = 50 * peak / 255;
        sixty = 60 * peak / 255;
        seventy = 70 * peak / 255;
    }

    int __stdcall SetCacheHints(int cachehints, int frame_range)
    {
        return cachehints == CACHE_GET_MTMODE ? MT_MULTI_INSTANCE : 0;
    }
};

class CreateMM : public Common
{
    int ttype_, mtqL_, mthL_, mtqC_, mthC_, nt_, minthresh_, maxthresh_, cstr_, opt_;
    int widthPad, hShift[3], vShift[3], hHalf[3], vHalf[3];
    VideoInfo vipad, vimsk;

    template<typename T>
    void threshMask_c(PVideoFrame& src, PVideoFrame& dst, int plane, IScriptEnvironment* env) noexcept;
    template<typename T>
    void motionMask_c(PVideoFrame& src1, PVideoFrame& msk1, PVideoFrame& src2, PVideoFrame& msk2, PVideoFrame& dst, const int plane, IScriptEnvironment* env) noexcept;
    template<typename T>
    void andMasks_c(PVideoFrame& src1, PVideoFrame& src2, PVideoFrame& dst, const int plane, IScriptEnvironment* env) noexcept;
    template<typename T>
    void combineMasks_c(PVideoFrame& src, PVideoFrame& dst, const int plane, IScriptEnvironment* env) noexcept;

    template<typename T1, typename T2, int step>
    void threshMask_sse2(PVideoFrame& src, PVideoFrame& dst, int plane, IScriptEnvironment* env) noexcept;
    template<typename T1, typename T2, int step>
    void motionMask_sse2(PVideoFrame& src1, PVideoFrame& msk1, PVideoFrame& src2, PVideoFrame& msk2, PVideoFrame& dst, const int plane, IScriptEnvironment* env) noexcept;
    template<typename T1, typename T2, int step>
    void andMasks_sse2(PVideoFrame& src1, PVideoFrame& src2, PVideoFrame& dst, const int plane, IScriptEnvironment* env) noexcept;
    template<typename T1, typename T2, int step>
    void combineMasks_sse2(PVideoFrame& src, PVideoFrame& dst, const int plane, IScriptEnvironment* env) noexcept;

    template<typename T1, typename T2, int step>
    void threshMask_avx2(PVideoFrame& src, PVideoFrame& dst, int plane, IScriptEnvironment* env) noexcept;
    template<typename T1, typename T2, int step>
    void motionMask_avx2(PVideoFrame& src1, PVideoFrame& msk1, PVideoFrame& src2, PVideoFrame& msk2, PVideoFrame& dst, const int plane, IScriptEnvironment* env) noexcept;
    template<typename T1, typename T2, int step>
    void andMasks_avx2(PVideoFrame& src1, PVideoFrame& src2, PVideoFrame& dst, const int plane, IScriptEnvironment* env) noexcept;
    template<typename T1, typename T2, int step>
    void combineMasks_avx2(PVideoFrame& src, PVideoFrame& dst, const int plane, IScriptEnvironment* env) noexcept;

    template<typename T1, typename T2, int step>
    void threshMask_avx512(PVideoFrame& src, PVideoFrame& dst, int plane, IScriptEnvironment* env) noexcept;
    template<typename T1, typename T2, int step>
    void motionMask_avx512(PVideoFrame& src1, PVideoFrame& msk1, PVideoFrame& src2, PVideoFrame& msk2, PVideoFrame& dst, const int plane, IScriptEnvironment* env) noexcept;
    template<typename T1, typename T2, int step>
    void andMasks_avx512(PVideoFrame& src1, PVideoFrame& src2, PVideoFrame& dst, const int plane, IScriptEnvironment* env) noexcept;
    template<typename T1, typename T2, int step>
    void combineMasks_avx512(PVideoFrame& src, PVideoFrame& dst, const int plane, IScriptEnvironment* env) noexcept;

public:
    CreateMM(PClip _child, int ttype, int mtqL, int mthL, int mtqC, int mthC, int nt, int minthresh, int maxthresh, int cstr, int y, int u, int v, int opt, IScriptEnvironment* env);
    PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
};

class BuildMM : public Common
{
    int mode_, order_, field_, length_, mtype_;
    PClip bf_;
    uint8_t* gvlut;
    std::array<uint8_t, 64> vlut;
    std::array<uint16_t, 16> tmmlut16;
    VideoInfo viSaved;

    template<typename T>
    void buildMask(PVideoFrame* cSrc, PVideoFrame* oSrc, PVideoFrame& dst, const int cCount, const int oCount, const int order, const int field, IScriptEnvironment* env) noexcept;

public:
    BuildMM(PClip _child, PClip bf, int mode, int order, int field, int length, int mtype, int y, int u, int v, IScriptEnvironment* env);
    PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
};

class BinaryMM : public Common
{
    int athresh_, metric_, expand_;
    bool binary_, link_;
    int athresh6, athreshsq, peak;
    int mode_, order_, field_;
    PClip clip_;

    template<typename T>
    void checkSpatial(PVideoFrame& src, PVideoFrame& dst, IScriptEnvironment* env) noexcept;
    template<typename T>
    void expandMask(PVideoFrame& mask, const int field, IScriptEnvironment* env) noexcept;
    template<typename T>
    void linkMask(PVideoFrame& mask, const int field, IScriptEnvironment* env) noexcept;
    template<typename T>
    void binaryMask(PVideoFrame& src, PVideoFrame& dst, IScriptEnvironment* env) noexcept;

public:
    BinaryMM(PClip _child, int athresh, int metric, int expand, bool binary, bool link, PClip clip, int mode, int order, int field, int y, int u, int v, IScriptEnvironment* env);
    PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
};
