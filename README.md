# Description

TMM builds a motion-mask for TDeint. 

This is [a port of the VapourSynth TMM](https://github.com/HomeOfVapourSynthEvolution/VapourSynth-TDeintMod).

# Usage

```
vsTMM (clip, int "mode", int "order", int "field", int "length", int "mtype", int "ttype", int "mtqL", int "mthL", int "mtqC", int "mthC", int "nt", int "minthresh", int "maxthresh", int "cstr", int "athresh", int "metric", int "expand", bool "binary", bool "link", int "y", int "u", int "v", int "opt")
```

## Parameters:

- clip\
    A clip to process. It must be in 8..16-bit YUV format.
    
- mode\
    Sets the mode of operation.\
    0: Same rate output.\
    1: Double rate output.\
    Default: 0.
    
- order\
    Sets the field order of the video.\
    -1: Use AviSynth internal order.\
    0: Bottom field first (bff).\
    1: Top field first (tff).\
    Default: -1.
    
- field\
    When in mode 0, this sets the field to be interpolated.\
    When in mode 1, this setting does nothing.\
    -1: Set field equal to order.\
    0: Interpolate top field (keep bottom field).\
    1: Interpolate bottom field (keep top field).\
    Default: -1.
    
- length\
    Sets the number of fields required for declaring pixels as stationary. length=6 means six fields (3 top/3 bottom), length=8 means 8 fields (4 top/4 bottom), etc... A larger value for length will prevent more motion-adaptive related artifacts, but will result in fewer pixels being weaved.\
    Must be greater than or equal to 6.\
    Default: 10.
    
- mtype\
    Sets whether or not both vertical neighboring lines in the current field of the line in the opposite parity field attempting to be weaved have to agree on both stationarity and direction.\
    0: No.\
    1: No for across, but yes for backwards/forwards.\
    2: Yes.\
    0 will result in the most pixels being weaved, while 2 will have the least artifacts.\
    Default: 1.
    
- ttype\
    Sets how to determine the per-pixel adaptive (quarter pel/half pel) motion thresholds.\
    0: 4 neighbors, diff to center pixel, compensated.\
    1: 8 neighbors, diff to center pixel, compensated.\
    2: 4 neighbors, diff to center pixel, uncompensated.\
    3: 8 neighbors, diff to center pixel, uncompensated.\
    4: 4 neighbors, range (max-min) of neighborhood.\
    5: 8 neighbors, range (max-min) of neighborhood.\
    Compensated means adjusted for distance differences due to field vs frames and chroma downsampling. The compensated versions will always result in thresholds <= to the uncompensated versions.\
    Default: 1.
    
- mtqL, mthL, mtqC, mthC\
    These parameters allow the specification of hard thresholds instead of using per-pixel adaptive motion thresholds.\
    mtqL sets the quarter pel threshold for luma, mthL sets the half pel threshold for luma, mtqC/mthC are the same but for chroma.\
    Setting all four parameters to -2 will disable motion adaptation (i.e. every pixel will be declared moving) allowing for a dumb bob. If these parameters are set to -1 then an adaptive threshold is used. Otherwise, if they are between 0 and 255 (inclusive) then the value of the parameter is used as the threshold for every pixel.\
    Must be between -2 and 255.\
    Default: mtqL = mthL = mtqC = mthC = -1.
    
- nt, minthresh, maxthresh\
    nt sets the noise threshold, which will be added to the value of each per-pixel threshold when determining if a pixel is stationary or not. After the addition of 'nt', any threshold less than minthresh will be increased to minthresh and any threshold greater than maxthresh will be decreased to maxthresh.\
    Must be between 0 and 255.\
    Default: nt = 2; minthresh = 4; maxthresh = 75.
    
- cstr\
    Sets the number of required neighbor pixels (3x3 neighborhood) in the quarter pel mask, of a pixel marked as moving in the quarter pel mask, but stationary in the half pel mask, marked as stationary for the pixel to be marked as stationary in the combined mask.\
    Default: 4.
    
- athresh\
    Area combing threshold used for spatial adaptation. Setting to -1 will disable spatial adaptation. Lower value will detect more combing, but will also result in more false positives. If your source is pure interlaced video you may want to simply disable spatial adaptation so that any moving pixels are counted as combed.\
    Must be between -1 and 255.\
    Default: -1.

- metric\
    Sets which spatial combing metric is used to detect combed pixels.\
    It has effect when athresh is greater than -1.    
    ```
    Assume 5 neighboring pixels (a,b,c,d,e) positioned vertically.
        a
        b
        c
        d
        e
    
    0:  d1 = c - b;
        d2 = c - d;
        if ((d1 > cthresh && d2 > cthresh) || (d1 < -cthresh && d2 < -cthresh))
        {
        if (abs(a+4*c+e-3*(b+d)) > cthresh*6) it's combed;
        }
    
    1:  val = (b - c) * (d - c);
        if (val > cthresh*cthresh) it's combed;
    ```
    Metric 0 is what TDeint always used previous to v1.0 RC7. Metric 1 is the combing metric used in Donald Graft's FieldDeinterlace()/IsCombed() funtions in decomb.dll.\
    Default: 0.

- expand\
    Sets the number of pixels to expand the comb mask horizontally on each side of combed pixels. Basically, if expand is greater than 0 then TMM will consider all pixels within 'expand' distance horizontally of a detected combed pixel to be combed as well. Can be useful when some combed pixels got missed in spatial adaptation.\
    Must be greater than or equal to 0.\
    Default: 0.
    
- binary\
    True: The output is binary comb mask.\
    False: The output is motion mask.\
    Default: False.

- link\
    Controls whether the luma plane is linked to chroma plane during comb mask creation.\
    Default: True. When the clip is Y or when luma plane is not filtered link = false. It does not have effect when binary = false.
    
- y, u, v\
    Planes to process.\
    1: Return garbage.\
    2: Copy plane.\
    3: Process plane.\
    Default: y = u = v = 3.

- opt\
    Sets which cpu optimizations to use.\
    -1: Auto-detect without AVX512.\
    0: Use C++ code.\
    1: Use SSE2 code.\
    2: Use AVX2 code.\
    3: Use AVX512 code.\
    Default: -1.
    