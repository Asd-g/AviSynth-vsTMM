#include "vsTMM.h"

AVSValue __cdecl Create_vsTMM(AVSValue args, void* user_data, IScriptEnvironment* env)
{
	if (!args[0].IsClip())
		env->ThrowError("vsTMM:  first argument must be a clip!");

	PClip oclip = args[0].AsClip();
	const VideoInfo& vi = oclip->GetVideoInfo();
	int compnum = vi.NumComponents();

	if (vi.IsFieldBased())
		env->ThrowError("vsTMM:  input clip cannot be field based!");
	if (vi.IsRGB() || vi.BitsPerComponent() == 32)
		env->ThrowError("vsTMM: clip must be 8..16 bit in Y/YUV format.");
	if (vi.height < 4)
		env->ThrowError("vsTMM: height must be greater than or equal to 4");
	if (vi.width & 1 || vi.height & 1)
		env->ThrowError("vsTMM: width and height must be multiples of 2");
	if (compnum > 1 && vi.GetPlaneWidthSubsampling(PLANAR_U) > 1)
		env->ThrowError("vsTMM: only horizontal chroma subsampling 1x-2x supported");
	if (compnum > 1 && vi.GetPlaneHeightSubsampling(PLANAR_U) > 1)
		env->ThrowError("vsTMM: only vertical chroma subsampling 1x-2x supported");

	int mode = args[1].AsInt(0);

	if (mode < 0 || mode > 1)
		env->ThrowError("vsTMM: mode must be 0 or 1");

	int order = args[2].AsInt(-1);

	if (order == -1)
		order = oclip->GetParity(0) ? 1 : 0;

	int field = args[3].AsInt(-1);

	if (field < -1 || field > 1)
		env->ThrowError("vsTMM: field must be -1, 0 or 1");
	if (field == -1)
		field = order;

	int length = args[4].AsInt(10);	
	
	if (length < 6)
		env->ThrowError("vsTMM: length must be greater than or equal to 6");

	int mtype = args[5].AsInt(1);

	if (mtype < 0 || mtype > 2)
		env->ThrowError("vsTMM: mtype must be 0, 1 or 2");

	int ttype = args[6].AsInt(1);

	if (ttype < 0 || ttype > 5)
		env->ThrowError("vsTMM: ttype must be 0, 1, 2, 3, 4 or 5");

	int mtqL = args[7].AsInt(-1);
	int mthL = args[8].AsInt(-1);
	int mtqC = args[9].AsInt(-1);
	int mthC = args[10].AsInt(-1);

	if (mtqL < -2 || mtqL > 255)
		env->ThrowError("vsTMM: mtqL must be between -2 and 255 (inclusive)");
	if (mthL < -2 || mthL > 255)
		env->ThrowError("vsTMM: mthL must be between -2 and 255 (inclusive)");
	if (mtqC < -2 || mtqC > 255)
		env->ThrowError("vsTMM: mtqC must be between -2 and 255 (inclusive)");
	if (mthC < -2 || mthC > 255)
		env->ThrowError("vsTMM: mthC must be between -2 and 255 (inclusive)");

	int nt = args[11].AsInt(2);

	if (nt < 0 || nt > 255)
		env->ThrowError("vsTMM: nt must be between 0 and 255 (inclusive)");

	int minthresh = args[12].AsInt(4);
	int maxthresh = args[13].AsInt(75);

	if (minthresh < 0 || minthresh > 255)
		env->ThrowError("vsTMM: minthresh must be between 0 and 255 (inclusive)");
	if (maxthresh < 0 || maxthresh > 255)
		env->ThrowError("vsTMM: maxthresh must be between 0 and 255 (inclusive)");

	int athresh = args[15].AsInt(-1);

	if (athresh < -1 || athresh > 255)
		env->ThrowError("vsTMM: athresh must be between -1 and 255 (inclusive)");

	int metric = args[16].AsInt(0);

	if (metric < 0 || metric > 1)
		env->ThrowError("vsTMM: metric must be 0 or 1");

	int expand = args[17].AsInt(0);

	if (expand < 0)
		env->ThrowError("vsTMM: expand must be greater than or equal to 0");

	int y = args[19].AsInt(3);
	int u = args[20].AsInt(3);
	int v = args[21].AsInt(3);

	if (y < 1 || y > 3)
		env->ThrowError("vsTMM: y must be between 1..3");
	if (u < 1 || u > 3)
		env->ThrowError("vsTMM: y must be between 1..3");
	if (v < 1 || v > 3)
		env->ThrowError("vsTMM: y must be between 1..3");

	int opt = args[22].AsInt(-1);

	if (opt < -1 || opt > 3)
		env->ThrowError("vsTMM: opt must be between -1..3");
	if (!(env->GetCPUFlags() & CPUF_AVX512F) && opt == 3)
		env->ThrowError("vsTMM: opt=3 requires AVX512F.");
	if (!(env->GetCPUFlags() & CPUF_AVX2) && opt == 2)
		env->ThrowError("vsTMM: opt=2 requires AVX2.");
	if (!(env->GetCPUFlags() & CPUF_SSE2) && opt == 1)
		env->ThrowError("vsTMM: opt=1 requires SSE2.");

	try
	{
		oclip = env->Invoke("SeparateFields", oclip).AsClip();
		const char* filter[] = { "SelectEven", "SelectOdd" };
		int parity = oclip->GetParity(0) ? 0 : 1;
		PClip topf = env->Invoke(filter[parity], oclip).AsClip();
		PClip botf = env->Invoke(filter[!parity], oclip).AsClip();

		topf = new CreateMM(topf, ttype, mtqL, mthL, mtqC, mthC, nt, minthresh, maxthresh, args[14].AsInt(4), y, u, v, opt, env);
		botf = new CreateMM(botf, ttype, mtqL, mthL, mtqC, mthC, nt, minthresh, maxthresh, args[14].AsInt(4), y, u, v, opt, env);

		PClip mask = new BuildMM(topf, botf, mode, order, field, length, mtype, y, u, v, env);

		return new BinaryMM(mask, athresh, metric, expand, args[18].AsBool(true), args[0].AsClip(), mode, order, field, y, u, v, env);
	}

	catch (IScriptEnvironment::NotFound)
	{
		env->ThrowError("vsTMM:  IScriptEnvironment::NotFound error while" \
			" invoking filters!");
	}
	catch (AvisynthError e)
	{
		env->ThrowError("vsTMM:  avisynth error while invoking filters (%s)!",
			e.msg);
	}

	return 0;
}

const AVS_Linkage* AVS_linkage = nullptr;

extern "C" __declspec(dllexport) const char* __stdcall
AvisynthPluginInit3(IScriptEnvironment * env, const AVS_Linkage* const vectors)
{
	AVS_linkage = vectors;

	env->AddFunction("vsTMM", "c[mode]i[order]i[field]i[length]i[mtype]i[ttype]i[mtqL]i[mthL]i[mtqC]i[mthC]i[nt]i[minthresh]i[maxthresh]i[cstr]i[athresh]i[metric]i[expand]i[link]b[y]i[u]i[v]i[opt]i", Create_vsTMM, 0);

	return "vsTMM";
}
