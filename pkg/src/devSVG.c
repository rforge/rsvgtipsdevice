/*
 *  SVGTips device, (C) 2008 Tony Plate, based on
 *  SVG device, (C) 2002 T Jake Luciani, Based on PicTex device, for
 *  R : A Computer Language for Statistical Data Analysis
 *  Copyright (C) 1995, 1996  Robert Gentleman and Ross Ihaka
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "R.h"
#include "Rversion.h"

#include "Rinternals.h"
#include "R_ext/GraphicsEngine.h"

#if R_VERSION < R_Version(2,7,0)
# include "Rgraphics.h"
# include "Rdevices.h"
# include "R_ext/GraphicsDevice.h"
typedef GEDevDesc* pGEDevDesc;
typedef NewDevDesc* pDevDesc;
typedef R_GE_gcontext* pGEcontext;
#endif

#if R_VERSION < R_Version(2,8,0)
typedef NewDevDesc DevDesc;
#endif

#include  <ctype.h> /* for islower */

#ifndef BEGIN_SUSPEND_INTERRUPTS
# define BEGIN_SUSPEND_INTERRUPTS
# define END_SUSPEND_INTERRUPTS
#endif

#if R_VERSION < R_Version(2,6,0)
# define R_GE_str2col Rf_str2col
  unsigned int Rf_str2col(const char *);
#endif

/* device-specific information per SVG device */

#define DOTSperIN       72.27
#define in2dots(x)      (DOTSperIN * x)

#define SVGDescMagic 0x76B29A1E
typedef struct {
    /* 'magic' helps identify if we have a SVG device -- I can't find any better way :-(
     * Look for use of 'magic' in code to see where this is needed.
     */
    int magic;
    FILE *texfp;
    char *filename;
    int pageno;
    int landscape;
    double width;
    double height;
    double pagewidth;
    double pageheight;
    double xlast;
    double ylast;
    double clipleft, clipright, cliptop, clipbottom;
    double clippedx0, clippedy0, clippedx1, clippedy1;

    double cex;
    double srt;
    int lty;
    int lwd;
    int col;
    int fg;
    int bg;
    int fontsize;
    int fontface;
    int tipFontSize;
    double tipOpacity;
    Rboolean debug;
    Rboolean xmlHeader;
    Rboolean useStyleAttributes; /* if 1, use style="stroke: black" etc, if 0 use stroke="black" etc */
    Rboolean onefile;     /* drop headers etc*/
    int toolTipMode; /* 0 = no tooltips; 1 = title + desc; 2 = title + 2 line desc */
    char *shapeContents; /* a string to put inside shapes */
    int shapeContentsUsed;
    char *shapeURL; /* URL to use for a shape */
    int shapeURLUsed;
    char *shapeURLTarget; /* URL target to use for a shape */
    int shapeURLTargetUsed;
    char *title;
} SVGDesc;


/* Global device information */

/*
 * The charwidth table was constructed by eyeballing results until they
 * looked good using the plot svgplot11.svg in the examples in
 * RSVGTipsDevice.Rd.  Unfortunately, SVG seems to have different
 * string widths on different systems -- these here are tuned to
 * FireFox running under Ubuntu.  Even FireFox running under Windows
 * seems to render with different string widths :-(
 * I don't know why there are 4 vectors of charwidth here -- I made
 * them all the same.  While all the standard ascii characters seem to
 * use data from the first vector, some special characters seem to draw
 * on data from the others.
 */

/* This charwidth works for font-family="Arial" in FireFox under Ubuntu */

static double charwidth[4][128] = {
    {
0.7056, 0.7056, 0.7056, 0.7056, 0.7056, 0.7056, 0.7056, 0.7056,
0.7056, 0.7056, 0.7056, 0.7056, 0.7056, 0.7056, 0.7056, 0.7056,
0.7056, 0.7056, 0.7056, 0.7056, 0.7056, 0.7056, 0.7056, 0.7056,
0.7056, 0.7056, 0.7056, 0.7056, 0.7056, 0.7056, 0.7056, 0.7056,
0.42, 0.42, 0.504, 0.7056, 0.7056, 1.1928, 0.8904, 0.3024,
0.42, 0.42, 0.504, 0.7896, 0.42, 0.42, 0.42, 0.42,
0.7056, 0.7056, 0.7056, 0.7056, 0.7056, 0.7056, 0.7056, 0.7056,
0.7056, 0.7056, 0.42, 0.42, 0.7896, 0.7896, 0.7896, 0.7056,
1.3944, 0.8904, 0.8904, 0.9912, 0.9912, 0.8904, 0.7896, 0.9912,
0.9912, 0.42, 0.7056, 0.8904, 0.7056, 1.092, 0.9912, 0.9912,
0.8904, 0.9912, 0.9912, 0.8904, 0.7896, 0.9912, 0.8904, 1.2936,
0.8904, 0.8904, 0.7896, 0.42, 0.42, 0.42, 0.6048, 0.7056,
0.42, 0.7056, 0.7056, 0.7056, 0.7056, 0.7056, 0.42, 0.7056,
0.7056, 0.3024, 0.3024, 0.7056, 0.3024, 1.092, 0.7056, 0.7056,
0.7056, 0.7056, 0.42, 0.7056, 0.42, 0.7056, 0.7056, 0.9912,
0.7056, 0.7056, 0.7056, 0.42, 0.3024, 0.42, 0.7896, 1.2936
    },{
0.7056, 0.7056, 0.7056, 0.7056, 0.7056, 0.7056, 0.7056, 0.7056,
0.7056, 0.7056, 0.7056, 0.7056, 0.7056, 0.7056, 0.7056, 0.7056,
0.7056, 0.7056, 0.7056, 0.7056, 0.7056, 0.7056, 0.7056, 0.7056,
0.7056, 0.7056, 0.7056, 0.7056, 0.7056, 0.7056, 0.7056, 0.7056,
0.42, 0.42, 0.504, 0.7056, 0.7056, 1.1928, 0.8904, 0.3024,
0.42, 0.42, 0.504, 0.7896, 0.42, 0.42, 0.42, 0.42,
0.7056, 0.7056, 0.7056, 0.7056, 0.7056, 0.7056, 0.7056, 0.7056,
0.7056, 0.7056, 0.42, 0.42, 0.7896, 0.7896, 0.7896, 0.7056,
1.3944, 0.8904, 0.8904, 0.9912, 0.9912, 0.8904, 0.7896, 0.9912,
0.9912, 0.42, 0.7056, 0.8904, 0.7056, 1.092, 0.9912, 0.9912,
0.8904, 0.9912, 0.9912, 0.8904, 0.7896, 0.9912, 0.8904, 1.2936,
0.8904, 0.8904, 0.7896, 0.42, 0.42, 0.42, 0.6048, 0.7056,
0.42, 0.7056, 0.7056, 0.7056, 0.7056, 0.7056, 0.42, 0.7056,
0.7056, 0.3024, 0.3024, 0.7056, 0.3024, 1.092, 0.7056, 0.7056,
0.7056, 0.7056, 0.42, 0.7056, 0.42, 0.7056, 0.7056, 0.9912,
0.7056, 0.7056, 0.7056, 0.42, 0.3024, 0.42, 0.7896, 1.2936
    },{
0.7056, 0.7056, 0.7056, 0.7056, 0.7056, 0.7056, 0.7056, 0.7056,
0.7056, 0.7056, 0.7056, 0.7056, 0.7056, 0.7056, 0.7056, 0.7056,
0.7056, 0.7056, 0.7056, 0.7056, 0.7056, 0.7056, 0.7056, 0.7056,
0.7056, 0.7056, 0.7056, 0.7056, 0.7056, 0.7056, 0.7056, 0.7056,
0.42, 0.42, 0.504, 0.7056, 0.7056, 1.1928, 0.8904, 0.3024,
0.42, 0.42, 0.504, 0.7896, 0.42, 0.42, 0.42, 0.42,
0.7056, 0.7056, 0.7056, 0.7056, 0.7056, 0.7056, 0.7056, 0.7056,
0.7056, 0.7056, 0.42, 0.42, 0.7896, 0.7896, 0.7896, 0.7056,
1.3944, 0.8904, 0.8904, 0.9912, 0.9912, 0.8904, 0.7896, 0.9912,
0.9912, 0.42, 0.7056, 0.8904, 0.7056, 1.092, 0.9912, 0.9912,
0.8904, 0.9912, 0.9912, 0.8904, 0.7896, 0.9912, 0.8904, 1.2936,
0.8904, 0.8904, 0.7896, 0.42, 0.42, 0.42, 0.6048, 0.7056,
0.42, 0.7056, 0.7056, 0.7056, 0.7056, 0.7056, 0.42, 0.7056,
0.7056, 0.3024, 0.3024, 0.7056, 0.3024, 1.092, 0.7056, 0.7056,
0.7056, 0.7056, 0.42, 0.7056, 0.42, 0.7056, 0.7056, 0.9912,
0.7056, 0.7056, 0.7056, 0.42, 0.3024, 0.42, 0.7896, 1.2936
    },{
0.7056, 0.7056, 0.7056, 0.7056, 0.7056, 0.7056, 0.7056, 0.7056,
0.7056, 0.7056, 0.7056, 0.7056, 0.7056, 0.7056, 0.7056, 0.7056,
0.7056, 0.7056, 0.7056, 0.7056, 0.7056, 0.7056, 0.7056, 0.7056,
0.7056, 0.7056, 0.7056, 0.7056, 0.7056, 0.7056, 0.7056, 0.7056,
0.42, 0.42, 0.504, 0.7056, 0.7056, 1.1928, 0.8904, 0.3024,
0.42, 0.42, 0.504, 0.7896, 0.42, 0.42, 0.42, 0.42,
0.7056, 0.7056, 0.7056, 0.7056, 0.7056, 0.7056, 0.7056, 0.7056,
0.7056, 0.7056, 0.42, 0.42, 0.7896, 0.7896, 0.7896, 0.7056,
1.3944, 0.8904, 0.8904, 0.9912, 0.9912, 0.8904, 0.7896, 0.9912,
0.9912, 0.42, 0.7056, 0.8904, 0.7056, 1.092, 0.9912, 0.9912,
0.8904, 0.9912, 0.9912, 0.8904, 0.7896, 0.9912, 0.8904, 1.2936,
0.8904, 0.8904, 0.7896, 0.42, 0.42, 0.42, 0.6048, 0.7056,
0.42, 0.7056, 0.7056, 0.7056, 0.7056, 0.7056, 0.42, 0.7056,
0.7056, 0.3024, 0.3024, 0.7056, 0.3024, 1.092, 0.7056, 0.7056,
0.7056, 0.7056, 0.42, 0.7056, 0.42, 0.7056, 0.7056, 0.9912,
0.7056, 0.7056, 0.7056, 0.42, 0.3024, 0.42, 0.7896, 1.2936
    }
};

/* This charwidth works for font-family="sans-serif" in FireFox under Ubuntu */
/*
static double charwidth[4][128] = {
    {
0.7896000, 0.7896000, 0.7896000, 0.7896000, 0.7896000, 0.7896000, 0.7896000,
0.7896000, 0.7896000, 0.7896000, 0.7896000, 0.7896000, 0.7896000, 0.7896000,
0.7896000, 0.7896000, 0.7896000, 0.7896000, 0.7896000, 0.7896000, 0.7896000,
0.7896000, 0.7896000, 0.7896000, 0.7896000, 0.7896000, 0.7896000, 0.7896000,
0.7896000, 0.7896000, 0.7896000, 0.7896000, 0.8064000, 0.5040000, 0.5040000,
1.0920000, 0.8064000, 1.1928000, 1.0920000, 0.3024000, 0.5040000, 0.5040000,
0.7056000, 1.0920000, 0.4200000, 0.5040000, 0.4200000, 0.4200000, 0.7896000,
0.7896000, 0.7896000, 0.7896000, 0.7896000, 0.7896000, 0.7896000, 0.7896000,
0.7896000, 0.7896000, 0.4200000, 0.4200000, 1.0920000, 1.0920000, 1.0920000,
0.7056000, 1.2936000, 0.8904000, 0.8904000, 0.8904000, 0.9912000, 0.7896000,
0.7056000, 0.9912000, 0.9912000, 0.3024000, 0.3024000, 0.7896000, 0.7056000,
1.0920000, 0.9912000, 0.9912000, 0.7896000, 0.9912000, 0.7896000, 0.8904000,
0.7056000, 0.9912000, 0.8904000, 1.0920000, 0.7896000, 0.7056000, 0.9912000,
0.5040000, 0.4200000, 0.5040000, 1.0920000, 0.7056000, 0.7056000, 0.7896000,
0.7896000, 0.7056000, 0.7896000, 0.7896000, 0.4200000, 0.7896000, 0.7896000,
0.3024000, 0.3024000, 0.7056000, 0.3024000, 1.2936000, 0.7896000, 0.7896000,
0.7896000, 0.7896000, 0.5040000, 0.7056000, 0.5040000, 0.7896000, 0.7056000,
0.8904000, 0.7056000, 0.7056000, 0.7056000, 0.7896000, 0.4200000, 0.7896000,
1.0920000, 0.7896000
    },
    {
0.7896000, 0.7896000, 0.7896000, 0.7896000, 0.7896000, 0.7896000, 0.7896000,
0.7896000, 0.7896000, 0.7896000, 0.7896000, 0.7896000, 0.7896000, 0.7896000,
0.7896000, 0.7896000, 0.7896000, 0.7896000, 0.7896000, 0.7896000, 0.7896000,
0.7896000, 0.7896000, 0.7896000, 0.7896000, 0.7896000, 0.7896000, 0.7896000,
0.7896000, 0.7896000, 0.7896000, 0.7896000, 0.8064000, 0.5040000, 0.5040000,
1.0920000, 0.8064000, 1.1928000, 1.0920000, 0.3024000, 0.5040000, 0.5040000,
0.7056000, 1.0920000, 0.4200000, 0.5040000, 0.4200000, 0.4200000, 0.7896000,
0.7896000, 0.7896000, 0.7896000, 0.7896000, 0.7896000, 0.7896000, 0.7896000,
0.7896000, 0.7896000, 0.4200000, 0.4200000, 1.0920000, 1.0920000, 1.0920000,
0.7056000, 1.2936000, 0.8904000, 0.8904000, 0.8904000, 0.9912000, 0.7896000,
0.7056000, 0.9912000, 0.9912000, 0.3024000, 0.3024000, 0.7896000, 0.7056000,
1.0920000, 0.9912000, 0.9912000, 0.7896000, 0.9912000, 0.7896000, 0.8904000,
0.7056000, 0.9912000, 0.8904000, 1.0920000, 0.7896000, 0.7056000, 0.9912000,
0.5040000, 0.4200000, 0.5040000, 1.0920000, 0.7056000, 0.7056000, 0.7896000,
0.7896000, 0.7056000, 0.7896000, 0.7896000, 0.4200000, 0.7896000, 0.7896000,
0.3024000, 0.3024000, 0.7056000, 0.3024000, 1.2936000, 0.7896000, 0.7896000,
0.7896000, 0.7896000, 0.5040000, 0.7056000, 0.5040000, 0.7896000, 0.7056000,
0.8904000, 0.7056000, 0.7056000, 0.7056000, 0.7896000, 0.4200000, 0.7896000,
1.0920000, 0.7896000
    },
    {
0.7896000, 0.7896000, 0.7896000, 0.7896000, 0.7896000, 0.7896000, 0.7896000,
0.7896000, 0.7896000, 0.7896000, 0.7896000, 0.7896000, 0.7896000, 0.7896000,
0.7896000, 0.7896000, 0.7896000, 0.7896000, 0.7896000, 0.7896000, 0.7896000,
0.7896000, 0.7896000, 0.7896000, 0.7896000, 0.7896000, 0.7896000, 0.7896000,
0.7896000, 0.7896000, 0.7896000, 0.7896000, 0.8064000, 0.5040000, 0.5040000,
1.0920000, 0.8064000, 1.1928000, 1.0920000, 0.3024000, 0.5040000, 0.5040000,
0.7056000, 1.0920000, 0.4200000, 0.5040000, 0.4200000, 0.4200000, 0.7896000,
0.7896000, 0.7896000, 0.7896000, 0.7896000, 0.7896000, 0.7896000, 0.7896000,
0.7896000, 0.7896000, 0.4200000, 0.4200000, 1.0920000, 1.0920000, 1.0920000,
0.7056000, 1.2936000, 0.8904000, 0.8904000, 0.8904000, 0.9912000, 0.7896000,
0.7056000, 0.9912000, 0.9912000, 0.3024000, 0.3024000, 0.7896000, 0.7056000,
1.0920000, 0.9912000, 0.9912000, 0.7896000, 0.9912000, 0.7896000, 0.8904000,
0.7056000, 0.9912000, 0.8904000, 1.0920000, 0.7896000, 0.7056000, 0.9912000,
0.5040000, 0.4200000, 0.5040000, 1.0920000, 0.7056000, 0.7056000, 0.7896000,
0.7896000, 0.7056000, 0.7896000, 0.7896000, 0.4200000, 0.7896000, 0.7896000,
0.3024000, 0.3024000, 0.7056000, 0.3024000, 1.2936000, 0.7896000, 0.7896000,
0.7896000, 0.7896000, 0.5040000, 0.7056000, 0.5040000, 0.7896000, 0.7056000,
0.8904000, 0.7056000, 0.7056000, 0.7056000, 0.7896000, 0.4200000, 0.7896000,
1.0920000, 0.7896000
    },
    {
0.7896000, 0.7896000, 0.7896000, 0.7896000, 0.7896000, 0.7896000, 0.7896000,
0.7896000, 0.7896000, 0.7896000, 0.7896000, 0.7896000, 0.7896000, 0.7896000,
0.7896000, 0.7896000, 0.7896000, 0.7896000, 0.7896000, 0.7896000, 0.7896000,
0.7896000, 0.7896000, 0.7896000, 0.7896000, 0.7896000, 0.7896000, 0.7896000,
0.7896000, 0.7896000, 0.7896000, 0.7896000, 0.8064000, 0.5040000, 0.5040000,
1.0920000, 0.8064000, 1.1928000, 1.0920000, 0.3024000, 0.5040000, 0.5040000,
0.7056000, 1.0920000, 0.4200000, 0.5040000, 0.4200000, 0.4200000, 0.7896000,
0.7896000, 0.7896000, 0.7896000, 0.7896000, 0.7896000, 0.7896000, 0.7896000,
0.7896000, 0.7896000, 0.4200000, 0.4200000, 1.0920000, 1.0920000, 1.0920000,
0.7056000, 1.2936000, 0.8904000, 0.8904000, 0.8904000, 0.9912000, 0.7896000,
0.7056000, 0.9912000, 0.9912000, 0.3024000, 0.3024000, 0.7896000, 0.7056000,
1.0920000, 0.9912000, 0.9912000, 0.7896000, 0.9912000, 0.7896000, 0.8904000,
0.7056000, 0.9912000, 0.8904000, 1.0920000, 0.7896000, 0.7056000, 0.9912000,
0.5040000, 0.4200000, 0.5040000, 1.0920000, 0.7056000, 0.7056000, 0.7896000,
0.7896000, 0.7056000, 0.7896000, 0.7896000, 0.4200000, 0.7896000, 0.7896000,
0.3024000, 0.3024000, 0.7056000, 0.3024000, 1.2936000, 0.7896000, 0.7896000,
0.7896000, 0.7896000, 0.5040000, 0.7056000, 0.5040000, 0.7896000, 0.7056000,
0.8904000, 0.7056000, 0.7056000, 0.7056000, 0.7896000, 0.4200000, 0.7896000,
1.0920000, 0.7896000
    }
};
*/

/*************
 * charwidth table from RSvgDevice
static double charwidth[4][128] = {
    {
	0.5416690, 0.8333360, 0.7777810, 0.6111145, 0.6666690, 0.7083380, 0.7222240,
	0.7777810, 0.7222240, 0.7777810, 0.7222240, 0.5833360, 0.5361130, 0.5361130,
	0.8138910, 0.8138910, 0.2388900, 0.2666680, 0.5000020, 0.5000020, 0.5000020,
	0.5000020, 0.5000020, 0.6666700, 0.4444460, 0.4805580, 0.7222240, 0.7777810,
	0.5000020, 0.8611145, 0.9722260, 0.7777810, 0.2388900, 0.3194460, 0.5000020,
	0.8333360, 0.5000020, 0.8333360, 0.7583360, 0.2777790, 0.3888900, 0.3888900,
	0.5000020, 0.7777810, 0.2777790, 0.3333340, 0.2777790, 0.5000020, 0.5000020,
	0.5000020, 0.5000020, 0.5000020, 0.5000020, 0.5000020, 0.5000020, 0.5000020,
	0.5000020, 0.5000020, 0.2777790, 0.2777790, 0.3194460, 0.7777810, 0.4722240,
	0.4722240, 0.6666690, 0.6666700, 0.6666700, 0.6388910, 0.7222260, 0.5972240,
	0.5694475, 0.6666690, 0.7083380, 0.2777810, 0.4722240, 0.6944480, 0.5416690,
	0.8750050, 0.7083380, 0.7361130, 0.6388910, 0.7361130, 0.6458360, 0.5555570,
	0.6805570, 0.6875050, 0.6666700, 0.9444480, 0.6666700, 0.6666700, 0.6111130,
	0.2888900, 0.5000020, 0.2888900, 0.5000020, 0.2777790, 0.2777790, 0.4805570,
	0.5166680, 0.4444460, 0.5166680, 0.4444460, 0.3055570, 0.5000020, 0.5166680,
	0.2388900, 0.2666680, 0.4888920, 0.2388900, 0.7944470, 0.5166680, 0.5000020,
	0.5166680, 0.5166680, 0.3416690, 0.3833340, 0.3611120, 0.5166680, 0.4611130,
	0.6833360, 0.4611130, 0.4611130, 0.4347230, 0.5000020, 1.0000030, 0.5000020,
	0.5000020, 0.5000020
    },
    {
	0.5805590, 0.9166720, 0.8555600, 0.6722260, 0.7333370, 0.7944490, 0.7944490,
	0.8555600, 0.7944490, 0.8555600, 0.7944490, 0.6416700, 0.5861150, 0.5861150,
	0.8916720, 0.8916720, 0.2555570, 0.2861130, 0.5500030, 0.5500030, 0.5500030,
	0.5500030, 0.5500030, 0.7333370, 0.4888920, 0.5652800, 0.7944490, 0.8555600,
	0.5500030, 0.9472275, 1.0694500, 0.8555600, 0.2555570, 0.3666690, 0.5583360,
	0.9166720, 0.5500030, 1.0291190, 0.8305610, 0.3055570, 0.4277800, 0.4277800,
	0.5500030, 0.8555600, 0.3055570, 0.3666690, 0.3055570, 0.5500030, 0.5500030,
	0.5500030, 0.5500030, 0.5500030, 0.5500030, 0.5500030, 0.5500030, 0.5500030,
	0.5500030, 0.5500030, 0.3055570, 0.3055570, 0.3666690, 0.8555600, 0.5194470,
	0.5194470, 0.7333370, 0.7333370, 0.7333370, 0.7027820, 0.7944490, 0.6416700,
	0.6111145, 0.7333370, 0.7944490, 0.3305570, 0.5194470, 0.7638930, 0.5805590,
	0.9777830, 0.7944490, 0.7944490, 0.7027820, 0.7944490, 0.7027820, 0.6111145,
	0.7333370, 0.7638930, 0.7333370, 1.0388950, 0.7333370, 0.7333370, 0.6722260,
	0.3430580, 0.5583360, 0.3430580, 0.5500030, 0.3055570, 0.3055570, 0.5250030,
	0.5611140, 0.4888920, 0.5611140, 0.5111140, 0.3361130, 0.5500030, 0.5611140,
	0.2555570, 0.2861130, 0.5305590, 0.2555570, 0.8666720, 0.5611140, 0.5500030,
	0.5611140, 0.5611140, 0.3722250, 0.4216690, 0.4041690, 0.5611140, 0.5000030,
	0.7444490, 0.5000030, 0.5000030, 0.4763920, 0.5500030, 1.1000060, 0.5500030,
	0.5500030, 0.550003 },
    {
	0.5416690, 0.8333360, 0.7777810, 0.6111145, 0.6666690, 0.7083380, 0.7222240,
	0.7777810, 0.7222240, 0.7777810, 0.7222240, 0.5833360, 0.5361130, 0.5361130,
	0.8138910, 0.8138910, 0.2388900, 0.2666680, 0.5000020, 0.5000020, 0.5000020,
	0.5000020, 0.5000020, 0.7375210, 0.4444460, 0.4805580, 0.7222240, 0.7777810,
	0.5000020, 0.8611145, 0.9722260, 0.7777810, 0.2388900, 0.3194460, 0.5000020,
	0.8333360, 0.5000020, 0.8333360, 0.7583360, 0.2777790, 0.3888900, 0.3888900,
	0.5000020, 0.7777810, 0.2777790, 0.3333340, 0.2777790, 0.5000020, 0.5000020,
	0.5000020, 0.5000020, 0.5000020, 0.5000020, 0.5000020, 0.5000020, 0.5000020,
	0.5000020, 0.5000020, 0.2777790, 0.2777790, 0.3194460, 0.7777810, 0.4722240,
	0.4722240, 0.6666690, 0.6666700, 0.6666700, 0.6388910, 0.7222260, 0.5972240,
	0.5694475, 0.6666690, 0.7083380, 0.2777810, 0.4722240, 0.6944480, 0.5416690,
	0.8750050, 0.7083380, 0.7361130, 0.6388910, 0.7361130, 0.6458360, 0.5555570,
	0.6805570, 0.6875050, 0.6666700, 0.9444480, 0.6666700, 0.6666700, 0.6111130,
	0.2888900, 0.5000020, 0.2888900, 0.5000020, 0.2777790, 0.2777790, 0.4805570,
	0.5166680, 0.4444460, 0.5166680, 0.4444460, 0.3055570, 0.5000020, 0.5166680,
	0.2388900, 0.2666680, 0.4888920, 0.2388900, 0.7944470, 0.5166680, 0.5000020,
	0.5166680, 0.5166680, 0.3416690, 0.3833340, 0.3611120, 0.5166680, 0.4611130,
	0.6833360, 0.4611130, 0.4611130, 0.4347230, 0.5000020, 1.0000030, 0.5000020,
	0.5000020, 0.5000020 },
    {
	0.5805590, 0.9166720, 0.8555600, 0.6722260, 0.7333370, 0.7944490, 0.7944490,
	0.8555600, 0.7944490, 0.8555600, 0.7944490, 0.6416700, 0.5861150, 0.5861150,
	0.8916720, 0.8916720, 0.2555570, 0.2861130, 0.5500030, 0.5500030, 0.5500030,
	0.5500030, 0.5500030, 0.8002530, 0.4888920, 0.5652800, 0.7944490, 0.8555600,
	0.5500030, 0.9472275, 1.0694500, 0.8555600, 0.2555570, 0.3666690, 0.5583360,
	0.9166720, 0.5500030, 1.0291190, 0.8305610, 0.3055570, 0.4277800, 0.4277800,
	0.5500030, 0.8555600, 0.3055570, 0.3666690, 0.3055570, 0.5500030, 0.5500030,
	0.5500030, 0.5500030, 0.5500030, 0.5500030, 0.5500030, 0.5500030, 0.5500030,
	0.5500030, 0.5500030, 0.3055570, 0.3055570, 0.3666690, 0.8555600, 0.5194470,
	0.5194470, 0.7333370, 0.7333370, 0.7333370, 0.7027820, 0.7944490, 0.6416700,
	0.6111145, 0.7333370, 0.7944490, 0.3305570, 0.5194470, 0.7638930, 0.5805590,
	0.9777830, 0.7944490, 0.7944490, 0.7027820, 0.7944490, 0.7027820, 0.6111145,
	0.7333370, 0.7638930, 0.7333370, 1.0388950, 0.7333370, 0.7333370, 0.6722260,
	0.3430580, 0.5583360, 0.3430580, 0.5500030, 0.3055570, 0.3055570, 0.5250030,
	0.5611140, 0.4888920, 0.5611140, 0.5111140, 0.3361130, 0.5500030, 0.5611140,
	0.2555570, 0.2861130, 0.5305590, 0.2555570, 0.8666720, 0.5611140, 0.5500030,
	0.5611140, 0.5611140, 0.3722250, 0.4216690, 0.4041690, 0.5611140, 0.5000030,
	0.7444490, 0.5000030, 0.5000030, 0.4763920, 0.5500030, 1.1000060, 0.5500030,
	0.5500030, 0.550003
    }
};
****************/

static char* toolTip1Header[] = {
    "<script type=\"text/ecmascript\"><![CDATA[",
    "    var SVGDocument = null;",
    "    var SVGRoot = null;",
    "    var SVGViewBox = null;",
    "    var svgns = 'http://www.w3.org/2000/svg';",
    "    var xlinkns = 'http://www.w3.org/1999/xlink';",
    "    var map = null;",
    "    var toolTip = null;",
    "    var TrueCoords = null;",
    "    var tipBox = null;",
    "    var tipText = null;",
    "    var tipTitle = null;",
    "    var tipDesc1 = null;",
    "",
    "    var lastElement = null;",
    "    var titleText = '';",
    "    var titleDesc = '';",
    "",
    "",
    "    function Init(evt)",
    "    {",
    "       SVGDocument = evt.target.ownerDocument;",
    "       SVGRoot = SVGDocument.documentElement;",
    "       TrueCoords = SVGRoot.createSVGPoint();",
    "       map = document.getElementById('canvas')",
    "",
    "       toolTip = SVGDocument.getElementById('ToolTip');",
    "       tipBox = SVGDocument.getElementById('tipbox');",
    "       tipText = SVGDocument.getElementById('tipText');",
    "       tipText.setAttributeNS(null, 'font-family', 'Arial, sans-serif' );",
    "       tipTitle = SVGDocument.getElementById('tipTitle');",
    "       tipDesc1 = SVGDocument.getElementById('tipDesc1');",
    "       //window.status = (TrueCoords);",
    "",
    "       //create event for object",
    "       SVGRoot.addEventListener('mousemove', ShowTooltip, false);",
    "       SVGRoot.addEventListener('mouseout', HideTooltip, false);",
    "    };",
    "",
    "",
    "    function GetTrueCoords(evt)",
    "    {",
    "       // find the current zoom level and pan setting, and adjust the reported",
    "       //    mouse position accordingly",
    "       var newScale = SVGRoot.currentScale;",
    "       var translation = SVGRoot.currentTranslate;",
    "       TrueCoords.x = (evt.pageX - translation.x)/newScale;",
    "       TrueCoords.y = (evt.pageY - translation.y)/newScale;",
    "    };",
    "",
    "",
    "    function HideTooltip( evt )",
    "    {",
    "       toolTip.setAttributeNS(null, 'visibility', 'hidden');",
    "    };",
    "",
    "",
    "   function ShowTooltip( evt )",
    "   {",
    "      GetTrueCoords( evt );",
    "",
    "      var tipScale = 1/SVGRoot.currentScale;",
    "      var textWidth = 0;",
    "      var tspanWidth = 0;",
    "      var boxHeight = 20;",
    "",
    "      tipBox.setAttributeNS(null, 'transform', 'scale(' + tipScale + ',' + tipScale + ')' );",
    "      tipText.setAttributeNS(null, 'transform', 'scale(' + tipScale + ',' + tipScale + ')' );",
    "",
    "      var titleValue = '';",
    "      var desc1Value = '';",
    "      var targetElement = evt.target;",
    "      if ( lastElement != targetElement )",
    "      {",
    "         var targetTitle = targetElement.getElementsByTagName('tiptitle').item(0);",
    "         if ( targetTitle )",
    "         {",
    "            // if there is a 'tiptitle' element, use its contents for the tooltip tiptitle",
    "            titleValue = targetTitle.firstChild.nodeValue;",
    "         }",
    "",
    "         var targetDesc1 = targetElement.getElementsByTagName('desc1').item(0);",
    "         if ( targetDesc1 )",
    "         {",
    "            // if there is a 'desc1' element, use its contents for the tooltip desc1",
    "            desc1Value = targetDesc1.firstChild.nodeValue;",
    "",
    "            if ( '' == titleValue )",
    "            {",
    "               // if there is no 'tiptitle' element, use the contents of the 'desc1' element for the tooltip title instead",
    "               titleValue = desc1Value;",
    "               desc1Value = '';",
    "            }",
    "         } else {",
    "             // useful for debugging positioning...",
    "             // desc1Value = 'xPos=' + TrueCoords.x + ' yPos=' + TrueCoords.y;",
    "         }",
    "",
    "         // if there is still no 'tiptitle' element, use the contents of the 'id' attribute for the tooltip title",
    "         if ( '' == titleValue)",
    "         {",
    "             titleValue = targetElement.getAttributeNS(null, 'id');",
    "             if (titleValue = 'canvas')",
    "                 titleValue = ''",
    "         }",
    "",
    "         // selectively assign the tooltip title and desc the proper values,",
    "         //   and hide those which don't have text values",
    "         //",
    "         var titleDisplay = 'none';",
    "         if ( '' != titleValue )",
    "         {",
    "            tipTitle.firstChild.nodeValue = titleValue;",
    "            titleDisplay = 'inline';",
    "         }",
    "         tipTitle.setAttributeNS(null, 'display', titleDisplay );",
    "",
    "         var desc1Display = 'none';",
    "         if ( '' != desc1Value )",
    "         {",
    "            tipDesc1.firstChild.nodeValue = desc1Value;",
    "            desc1Display = 'inline';",
    "         }",
    "         tipDesc1.setAttributeNS(null, 'display', desc1Display );",
    "      }",
    "",
    "      // if there are tooltip contents to be displayed, adjust the size and position of the box",
    "      if ( '' != titleValue )",
    "      {",
    "         var xPos = TrueCoords.x;",
    "         var yPos = TrueCoords.y;",
    "         //return rectangle around text as SVGRect object",
    "         var outline = tipText.getBBox();",
    "         tipBox.setAttributeNS(null, 'width', Number(outline.width) + 10);",
    "         tipBox.setAttributeNS(null, 'height', Number(outline.height) + 10);",
    "",
    "         // work out which direction tooltip needs to hang in to be visible",
    "         var mapbb = map.getBBox();",
    "         if (xPos + (10 * tipScale) + Number(outline.width) < Number(mapbb.width))",
    "             xPos = xPos + 10 * tipScale;",
    "         else",
    "             xPos = xPos - 10 * tipScale - Number(outline.width);",
    "         if (yPos + (10 * tipScale) + Number(outline.height) < Number(mapbb.height))",
    "             yPos = yPos + 10 * tipScale;",
    "         else",
    "             yPos = yPos - 10 * tipScale - Number(outline.height);",
    "",
    "         // update position",
    "         toolTip.setAttributeNS(null, 'transform', 'translate(' + xPos + ',' + yPos + ')');",
    "         toolTip.setAttributeNS(null, 'visibility', 'visible');",
    "       }",
    "    };",
    "",
    "   ]]></script>",
    "!END!"
};

static char* toolTip2Header[] = {
    "<script type=\"text/ecmascript\"><![CDATA[",
    "   // This code for a title + two-line tooltip mode displays OK in",
    "   // the default SVG renderers in Firefox and Safari, but does not",
    "   // display in the Batik standalone SVG viewer. It probably needs",
    "   // some declarations to extend XML elements to make it fully",
    "   // compliant with the SVG specs, see man/RSVGTipsDevice.future.Rd.",
    "   var SVGDocument = null;",
    "   var SVGRoot = null;",
    "   var SVGViewBox = null;",
    "   var svgns = 'http://www.w3.org/2000/svg';",
    "   var xlinkns = 'http://www.w3.org/1999/xlink';",
    "   var map = null;",
    "   var toolTip = null;",
    "   var TrueCoords = null;",
    "   var tipBox = null;",
    "   var tipText = null;",
    "   var tipTitle = null;",
    "   var tipDesc1 = null;",
    "   var tipDesc2 = null;",
    "",
    "   var lastElement = null;",
    "   var titleText = '';",
    "   var titleDesc = '';",
    "",
    "",
    "   function Init(evt)",
    "   {",
    "      SVGDocument = evt.target.ownerDocument;",
    "      SVGRoot = SVGDocument.documentElement;",
    "      TrueCoords = SVGRoot.createSVGPoint();",
    "      map = document.getElementById('canvas')",
    "",
    "      toolTip = SVGDocument.getElementById('ToolTip');",
    "      tipBox = SVGDocument.getElementById('tipbox');",
    "      tipText = SVGDocument.getElementById('tipText');",
    "      tipText.setAttributeNS(null, 'font-family', 'Arial, sans-serif' );",
    "      tipTitle = SVGDocument.getElementById('tipTitle');",
    "      tipDesc1 = SVGDocument.getElementById('tipDesc1');",
    "      tipDesc2 = SVGDocument.getElementById('tipDesc2');",
    "      //window.status = (TrueCoords);",
    "",
    "      //create event for object",
    "      SVGRoot.addEventListener('mousemove', ShowTooltip, false);",
    "      SVGRoot.addEventListener('mouseout', HideTooltip, false);",
    "   };",
    "",
    "",
    "   function GetTrueCoords(evt)",
    "   {",
    "      // find the current zoom level and pan setting, and adjust the reported",
    "      //    mouse position accordingly",
    "      var newScale = SVGRoot.currentScale;",
    "      var translation = SVGRoot.currentTranslate;",
    "      TrueCoords.x = (evt.pageX - translation.x)/newScale;",
    "      TrueCoords.y = (evt.pageY - translation.y)/newScale;",
    "   };",
    "",
    "",
    "   function HideTooltip( evt )",
    "   {",
    "      toolTip.setAttributeNS(null, 'visibility', 'hidden');",
    "   };",
    "",
    "",
    "   function ShowTooltip( evt )",
    "   {",
    "      GetTrueCoords( evt );",
    "",
    "      var tipScale = 1/SVGRoot.currentScale;",
    "      var textWidth = 0;",
    "      var tspanWidth = 0;",
    "      var boxHeight = 20;",
    "",
    "      tipBox.setAttributeNS(null, 'transform', 'scale(' + tipScale + ',' + tipScale + ')' );",
    "      tipText.setAttributeNS(null, 'transform', 'scale(' + tipScale + ',' + tipScale + ')' );",
    "",
    "      var titleValue = '';",
    "      var desc1Value = '';",
    "      var desc2Value = '';",
    "      var targetElement = evt.target;",
    "      if ( lastElement != targetElement )",
    "      {",
    "         var targetTitle = targetElement.getElementsByTagName('tiptitle').item(0);",
    "         if ( targetTitle )",
    "         {",
    "            // if there is a 'tiptitle' element, use its contents for the tooltip tiptitle",
    "            titleValue = targetTitle.firstChild.nodeValue;",
    "         }",
    "",
    "         var targetDesc1 = targetElement.getElementsByTagName('desc1').item(0);",
    "         if ( targetDesc1 )",
    "         {",
    "            // if there is a 'desc1' element, use its contents for the tooltip desc1",
    "            desc1Value = targetDesc1.firstChild.nodeValue;",
    "",
    "            if ( '' == titleValue )",
    "            {",
    "               // if there is no 'tiptitle' element, use the contents of the 'desc1' element for the tooltip title instead",
    "               titleValue = desc1Value;",
    "               desc1Value = '';",
    "            }",
    "         } else {",
    "             // useful for debugging positioning...",
    "             // desc1Value = 'xPos=' + TrueCoords.x + ' yPos=' + TrueCoords.y;",
    "         }",
    "         var targetDesc2 = targetElement.getElementsByTagName('desc2').item(0);",
    "         if ( targetDesc2 )",
    "         {",
    "             // if there is a 'desc2' element, use its contents for the tooltip desc",
    "             desc2Value = targetDesc2.firstChild.nodeValue;",
    "         } else {",
    "             // useful for debugging positioning... here bounding box of root element",
    "             // desc2Value = 'bbox=' + map.getBBox().width + ' x ' + map.getBBox().height;",
    "         }",
    "",
    "         // if there is still no 'tiptitle' element, use the contents of the 'id' attribute for the tooltip title",
    "         if ( '' == titleValue)",
    "         {",
    "             titleValue = targetElement.getAttributeNS(null, 'id');",
    "             if (titleValue = 'canvas')",
    "                 titleValue = ''",
    "         }",
    "",
    "         // selectively assign the tooltip title and desc the proper values,",
    "         //   and hide those which don't have text values",
    "         //",
    "         var titleDisplay = 'none';",
    "         if ( '' != titleValue )",
    "         {",
    "            tipTitle.firstChild.nodeValue = titleValue;",
    "            titleDisplay = 'inline';",
    "         }",
    "         tipTitle.setAttributeNS(null, 'display', titleDisplay );",
    "",
    "         var desc1Display = 'none';",
    "         if ( '' != desc1Value )",
    "         {",
    "            tipDesc1.firstChild.nodeValue = desc1Value;",
    "            desc1Display = 'inline';",
    "         }",
    "         tipDesc1.setAttributeNS(null, 'display', desc1Display );",
    "",
    "         var desc2Display = 'none';",
    "         if ( '' != desc2Value )",
    "         {",
    "            tipDesc2.firstChild.nodeValue = desc2Value;",
    "            desc2Display = 'inline';",
    "         }",
    "         tipDesc2.setAttributeNS(null, 'display', desc2Display );",
    "      }",
    "",
    "      // if there are tooltip contents to be displayed, adjust the size and position of the box",
    "      if ( '' != titleValue )",
    "      {",
    "         var xPos = TrueCoords.x;",
    "         var yPos = TrueCoords.y;",
    "         //return rectangle around text as SVGRect object",
    "         var outline = tipText.getBBox();",
    "         tipBox.setAttributeNS(null, 'width', Number(outline.width) + 10);",
    "         tipBox.setAttributeNS(null, 'height', Number(outline.height) + 10);",
    "",
    "         // work out which direction tooltip needs to hang in to be visible",
    "         var mapbb = map.getBBox();",
    "         if (xPos + (10 * tipScale) + Number(outline.width) < Number(mapbb.width))",
    "             xPos = xPos + 10 * tipScale;",
    "         else",
    "             xPos = xPos - 10 * tipScale - Number(outline.width);",
    "         if (yPos + (10 * tipScale) + Number(outline.height) < Number(mapbb.height))",
    "             yPos = yPos + 10 * tipScale;",
    "         else",
    "             yPos = yPos - 10 * tipScale - Number(outline.height);",
    "",
    "         // update position",
    "         toolTip.setAttributeNS(null, 'transform', 'translate(' + xPos + ',' + yPos + ')');",
    "         toolTip.setAttributeNS(null, 'visibility', 'visible');",
    "       }",
    "    };",
    "",
    "]]></script>",
    "!END!"
};

/*
static char* toolTip3Header[] = {
    "<script type=\"text/ecmascript\"><![CDATA[",
    "   // This code for a title + two-line tooltip mode displays OK in",
    "   // the default SVG renderers in Firefox and Safari, but does not",
    "   // display in the Batik standalone SVG viewer. It probably needs",
    "   // some declarations to extend XML elements to make it fully",
    "   // compliant with the SVG specs, see man/RSVGTipsDevice.future.Rd.",
    "   var SVGDocument = null;",
    "   var SVGRoot = null;",
    "   var SVGViewBox = null;",
    "   var svgns = 'http://www.w3.org/2000/svg';",
    "   var xlinkns = 'http://www.w3.org/1999/xlink';",
    "   var map = null;",
    "   var toolTip = null;",
    "   var TrueCoords = null;",
    "   var tipBox = null;",
    "   var tipText = null;",
    "   var tipTitle = null;",
    "   var tipDesc1 = null;",
    "   var tipDesc2 = null;",
    "   var tipDesc3 = null;",
    "",
    "   var lastElement = null;",
    "   var titleText = '';",
    "   var titleDesc = '';",
    "",
    "",
    "   function Init(evt)",
    "   {",
    "      SVGDocument = evt.target.ownerDocument;",
    "      SVGRoot = SVGDocument.documentElement;",
    "      TrueCoords = SVGRoot.createSVGPoint();",
    "      map = document.getElementById('canvas')",
    "",
    "      toolTip = SVGDocument.getElementById('ToolTip');",
    "      tipBox = SVGDocument.getElementById('tipbox');",
    "      tipText = SVGDocument.getElementById('tipText');",
    "      tipText.setAttributeNS(null, 'font-family', 'Arial, sans-serif' );",
    "      tipTitle = SVGDocument.getElementById('tipTitle');",
    "      tipDesc1 = SVGDocument.getElementById('tipDesc1');",
    "      tipDesc2 = SVGDocument.getElementById('tipDesc2');",
    "      tipDesc3 = SVGDocument.getElementById('tipDesc3');",
    "      //window.status = (TrueCoords);",
    "",
    "      //create event for object",
    "      SVGRoot.addEventListener('mousemove', ShowTooltip, false);",
    "      SVGRoot.addEventListener('mouseout', HideTooltip, false);",
    "   };",
    "",
    "",
    "   function GetTrueCoords(evt)",
    "   {",
    "      // find the current zoom level and pan setting, and adjust the reported",
    "      //    mouse position accordingly",
    "      var newScale = SVGRoot.currentScale;",
    "      var translation = SVGRoot.currentTranslate;",
    "      TrueCoords.x = (evt.pageX - translation.x)/newScale;",
    "      TrueCoords.y = (evt.pageY - translation.y)/newScale;",
    "   };",
    "",
    "",
    "   function HideTooltip( evt )",
    "   {",
    "      toolTip.setAttributeNS(null, 'visibility', 'hidden');",
    "   };",
    "",
    "",
    "   function ShowTooltip( evt )",
    "   {",
    "      GetTrueCoords( evt );",
    "",
    "      var tipScale = 1/SVGRoot.currentScale;",
    "      var textWidth = 0;",
    "      var tspanWidth = 0;",
    "      var boxHeight = 20;",
    "",
    "      tipBox.setAttributeNS(null, 'transform', 'scale(' + tipScale + ',' + tipScale + ')' );",
    "      tipText.setAttributeNS(null, 'transform', 'scale(' + tipScale + ',' + tipScale + ')' );",
    "",
    "      var titleValue = '';",
    "      var desc1Value = '';",
    "      var desc2Value = '';",
    "      var desc3Value = '';",
    "      var targetElement = evt.target;",
    "      if ( lastElement != targetElement )",
    "      {",
    "         var targetTitle = targetElement.getElementsByTagName('tiptitle').item(0);",
    "         if ( targetTitle )",
    "         {",
    "            // if there is a 'tiptitle' element, use its contents for the tooltip tiptitle",
    "            titleValue = targetTitle.firstChild.nodeValue;",
    "         }",
    "",
    "         var targetDesc1 = targetElement.getElementsByTagName('desc1').item(0);",
    "         if ( targetDesc1 )",
    "         {",
    "            // if there is a 'desc1' element, use its contents for the tooltip desc1",
    "            desc1Value = targetDesc1.firstChild.nodeValue;",
    "",
    "            if ( '' == titleValue )",
    "            {",
    "               // if there is no 'tiptitle' element, use the contents of the 'desc1' element for the tooltip title instead",
    "               titleValue = desc1Value;",
    "               desc1Value = '';",
    "            }",
    "         } else {",
    "             // useful for debugging positioning...",
    "             // desc1Value = 'xPos=' + TrueCoords.x + ' yPos=' + TrueCoords.y;",
    "         }",
    "         var targetDesc2 = targetElement.getElementsByTagName('desc2').item(0);",
    "         if ( targetDesc2 )",
    "         {",
    "             // if there is a 'desc2' element, use its contents for the tooltip desc",
    "             desc2Value = targetDesc2.firstChild.nodeValue;",
    "         } else {",
    "             // useful for debugging positioning... here bounding box of root element",
    "             // desc2Value = 'bbox=' + map.getBBox().width + ' x ' + map.getBBox().height;",
    "         }",
    "",
    "         var targetDesc3 = targetElement.getElementsByTagName('desc3').item(0);",
    "         if ( targetDesc3 )",
    "         {",
    "             // if there is a 'desc3' element, use its contents for the tooltip desc",
    "             desc3Value = targetDesc3.firstChild.nodeValue;",
    "         } else {",
    "             // useful for debugging positioning... here bounding box of root element",
    "             // desc3Value = 'bbox=' + map.getBBox().width + ' x ' + map.getBBox().height;",
    "         }",
    "",
    "         // if there is still no 'tiptitle' element, use the contents of the 'id' attribute for the tooltip title",
    "         if ( '' == titleValue)",
    "         {",
    "             titleValue = targetElement.getAttributeNS(null, 'id');",
    "             if (titleValue = 'canvas')",
    "                 titleValue = ''",
    "         }",
    "",
    "         // selectively assign the tooltip title and desc the proper values,",
    "         //   and hide those which don't have text values",
    "         //",
    "         var titleDisplay = 'none';",
    "         if ( '' != titleValue )",
    "         {",
    "            tipTitle.firstChild.nodeValue = titleValue;",
    "            titleDisplay = 'inline';",
    "         }",
    "         tipTitle.setAttributeNS(null, 'display', titleDisplay );",
    "",
    "         var desc1Display = 'none';",
    "         if ( '' != desc1Value )",
    "         {",
    "            tipDesc1.firstChild.nodeValue = desc1Value;",
    "            desc1Display = 'inline';",
    "         }",
    "         tipDesc1.setAttributeNS(null, 'display', desc1Display );",
    "",
    "         var desc2Display = 'none';",
    "         if ( '' != desc2Value )",
    "         {",
    "            tipDesc2.firstChild.nodeValue = desc2Value;",
    "            desc2Display = 'inline';",
    "         }",
    "         tipDesc2.setAttributeNS(null, 'display', desc2Display );",
    "",
    "         var desc3Display = 'none';",
    "         if ( '' != desc3Value )",
    "         {",
    "            tipDesc3.firstChild.nodeValue = desc3Value;",
    "            desc3Display = 'inline';",
    "         }",
    "         tipDesc3.setAttributeNS(null, 'display', desc3Display );",
    "      }",
    "",
    "      // if there are tooltip contents to be displayed, adjust the size and position of the box",
    "      if ( '' != titleValue )",
    "      {",
    "         var xPos = TrueCoords.x;",
    "         var yPos = TrueCoords.y;",
    "         //return rectangle around text as SVGRect object",
    "         var outline = tipText.getBBox();",
    "         tipBox.setAttributeNS(null, 'width', Number(outline.width) + 10);",
    "         tipBox.setAttributeNS(null, 'height', Number(outline.height) + 10);",
    "",
    "         // work out which direction tooltip needs to hang in to be visible",
    "         var mapbb = map.getBBox();",
    "         if (xPos + (10 * tipScale) + Number(outline.width) < Number(mapbb.width))",
    "             xPos = xPos + 10 * tipScale;",
    "         else",
    "             xPos = xPos - 10 * tipScale - Number(outline.width);",
    "         if (yPos + (10 * tipScale) + Number(outline.height) < Number(mapbb.height))",
    "             yPos = yPos + 10 * tipScale;",
    "         else",
    "             yPos = yPos - 10 * tipScale - Number(outline.height);",
    "",
    "         // update position",
    "         toolTip.setAttributeNS(null, 'transform', 'translate(' + xPos + ',' + yPos + ')');",
    "         toolTip.setAttributeNS(null, 'visibility', 'visible');",
    "       }",
    "    };",
    "",
    "]]></script>",
    "!END!"
};
*/

/* Device driver actions */

static void   SVG_Activate(pDevDesc dd);
static void   SVG_Circle(double x, double y, double r,
                         const pGEcontext gc,
                         pDevDesc dd);
static void   SVG_Clip(double, double, double, double, pDevDesc);
static void   SVG_Close(pDevDesc);
static void   SVG_Deactivate(pDevDesc );
static void   SVG_Line(double x1, double y1, double x2, double y2,
                       const pGEcontext gc,
                       pDevDesc dd);
static Rboolean SVG_Locator(double*, double*, pDevDesc);
static void   SVG_Mode(int, pDevDesc);
static void   SVG_NewPage(const pGEcontext gc, pDevDesc dd);
static Rboolean SVG_Open(pDevDesc, SVGDesc*);
static void   SVG_Polygon(int n, double *x, double *y,
                          const pGEcontext gc,
                          pDevDesc dd);
static void   SVG_Polyline(int n, double *x, double *y,
                           const pGEcontext gc,
                           pDevDesc dd);
static void   SVG_Rect(double x0, double y0, double x1, double y1,
                       const pGEcontext gc,
                       pDevDesc dd);
static void   SVG_Size(double *left, double *right,
                       double *bottom, double *top,
                       pDevDesc dd);

/*
static void   SVG_Resize(double *left, double *right,
                         double *bottom, double *top,
                         pDevDesc dd);
*/
static double SVG_StrWidth(const char *str,
                           const pGEcontext gc,
                           pDevDesc dd);
static void   SVG_Text(double x, double y, const char *str,
                       double rot, double hadj,
                       const pGEcontext gc,
                       pDevDesc dd);
static void   SVG_MetricInfo(int c,
                             const pGEcontext gc,
                             double* ascent, double* descent,
                             double* width, pDevDesc dd);

/* Support routines */

static char MyColBuf[8];
static char HexDigits[] = "0123456789ABCDEF";

char *col2RGBname(unsigned int col)
{
    MyColBuf[0] = '#';
    MyColBuf[1] = HexDigits[(col >>  4) & 15];
    MyColBuf[2] = HexDigits[(col      ) & 15];
    MyColBuf[3] = HexDigits[(col >> 12) & 15];
    MyColBuf[4] = HexDigits[(col >>  8) & 15];
    MyColBuf[5] = HexDigits[(col >> 20) & 15];
    MyColBuf[6] = HexDigits[(col >> 16) & 15];
    MyColBuf[7] = '\0';
    return &MyColBuf[0];
}

/*Thanks Paul*/
static void SVG_Size(double *left, double *right,
                     double *bottom, double *top,
                     pDevDesc dd)
{
    *left = dd->left;
    *right = dd->right;
    *bottom = dd->bottom;
    *top = dd->top;
}

void SetSvgShapeContents(char **str)
{
    pGEDevDesc dd = GEcurrentDevice();
    SVGDesc *ptd;
    if (dd==0 || dd->dev==0)
	return;
    ptd = (SVGDesc *) dd->dev->deviceSpecific;
    /* this might get called when some other graphics device is being used, */
    /* in which case we want to do nothing. */
    if (ptd==0 || ptd->magic!=SVGDescMagic)
	return;
    if (ptd->shapeContents!=0)
	Free(ptd->shapeContents);
    if (!(ptd->shapeContents = Calloc(strlen(str[0])+1, char)))
        return;
    strcpy(ptd->shapeContents, str[0]);
    ptd->shapeContentsUsed = 0;
}

void SetSvgShapeURL(char **str)
{
    pGEDevDesc dd = GEcurrentDevice();
    SVGDesc *ptd;
    if (dd==0 || dd->dev==0)
	return;
    ptd = (SVGDesc *) dd->dev->deviceSpecific;
    /* this might get called when some other graphics device is being used, */
    /* in which case we want to do nothing. */
    if (ptd==0 || ptd->magic!=SVGDescMagic)
	return;
    if (ptd->shapeURL!=0)
	Free(ptd->shapeURL);
    if (!(ptd->shapeURL = Calloc(strlen(str[0])+1, char)))
        return;
    strcpy(ptd->shapeURL, str[0]);
    ptd->shapeURLUsed = 0;
}

void SetSvgShapeURLTarget(char **str)
{
    GEDevDesc *dd = GEcurrentDevice();
    SVGDesc *ptd;
    if (dd==0 || dd->dev==0)
	return;
    ptd = (SVGDesc *) dd->dev->deviceSpecific;
    /* this might get called when some other graphics device is being used, */
    /* in which case we want to do nothing. */
    if (ptd==0 || ptd->magic!=SVGDescMagic)
	return;
    if (ptd->shapeURLTarget!=0)
	Free(ptd->shapeURLTarget);
    if (!(ptd->shapeURLTarget = Calloc(strlen(str[0])+1, char)))
        return;
    strcpy(ptd->shapeURLTarget, str[0]);
    ptd->shapeURLTargetUsed = 0;
}

void GetSvgToolTipMode(int *mode)
{
    pGEDevDesc dd = GEcurrentDevice();
    SVGDesc *ptd;
    /* this might get called when some other graphics device is being used, */
    /* in which case we want to return mode = -1 */
    *mode = -1;
    if (dd==0 || dd->dev==0)
	return;
    ptd = (SVGDesc *) dd->dev->deviceSpecific;
    if (ptd==0 || ptd->magic!=SVGDescMagic)
	return;
    *mode = ptd->toolTipMode;
}

/*Get Device point from user point*/
/*void GetSvgDevicePoint(double x, double y)
  {
  pGEDevDesc dd = GEcurrentDevice();

  x = toDeviceX(x,
  GConvert(x, y, GE_NDC, GE_DEVICE, dd);
  }
*/
/*Get User Point from device point*/
/*void GetSvgUserPoint(double x, double y)
  {
  DevDesc *dd = GEcurrentDevice();
  x = fromDeviceX(x
  GConvert(x, y, GE_DEVICE, GE_NDC, dd);
  }
*/
/*Get Device points for an array of user points*/
/*void GetSvgDevicePoints(double *x, double *y, int *n)
  {
  int i;

  pGEDevDesc dd = GEcurrentDevice();

  for(i = 0; i < *n; i++){
  GConvert(x+i, y+i, GE_NDC, DEVICE, dd);
  }
  }
*/
/*Get Device Boundries i.e. Width & Height*/
/*void GetSvgDeviceBoundry(double *w, double *h)
  {
  DevDesc *dd = CurrentDevice();
  SVGDesc *ptd = (SVGDesc *) dd->deviceSpecific;

  *w = in2dots(ptd->width);
  *h = in2dots(ptd->height);
  }
*/
static void SetLinetype(int newlty, int newlwd, pDevDesc dd, int
                        fgcol, int col)
{
    SVGDesc *ptd = (SVGDesc *) dd->deviceSpecific;

    int code, i, dashleft;
    double fillop, strokeop;
    char *strokeCol, *fillCol;
    ptd->lty = newlty;
    ptd->lwd = newlwd;

    /*Set line size + color*/
    if (ptd->useStyleAttributes)
	fprintf(ptd->texfp, "style=\"stroke-width:%dpx", newlwd);
    else
	fprintf(ptd->texfp, "stroke-width=\"%dpx\"", newlwd);

    if (R_VERSION < R_Version(2,0,0)) {
	/* Old type alpha channels, 0 means opaque */

	/* code is set as follows */
	/* code == 0, nothing to draw */
	/* code == 1, outline only */
	/* code == 2, fill only */
	/* code == 3, outline and fill */

	code = 2 * (R_ALPHA(fgcol) == 0) + (R_ALPHA(col) == 0);

	if (code == 0){
	    strokeCol = "none";
	    fillCol = "none";
	} else if ( code == 1){
	    strokeCol = col2RGBname(col);
	    fillCol = "none";
	} else if ( code == 2){
	    strokeCol = col2RGBname(fgcol);
	    fillCol = col2RGBname(fgcol);
	} else if (code == 3){
	    strokeCol = col2RGBname(col);
	    fillCol = col2RGBname(fgcol);
	}
	if (ptd->useStyleAttributes)
	    fprintf(ptd->texfp, ";stroke:%s;fill:%s", strokeCol, fillCol);
	else
	    fprintf(ptd->texfp, " stroke=\"%s\" fill=\"%s\"", strokeCol, fillCol);
    } else {
	/* As of R 2.0.0, the coding of alpha channels changed,
	   R handles transparency properly. Well, almost. For some
	   misterious reasons the highest bit of the alpha channel is always
	   1, probably because the sign of int. (?) For now we simply mask
	   out this bit. 2004/11/25 Gabor Csardi. */


	strokeop=((double)((col>>24)&127))/127;
	fillop=((double)((fgcol>>24)&127))/127;
	if (ptd->useStyleAttributes) {
	    fprintf(ptd->texfp, ";stroke:%s", col2RGBname(col));
	    fprintf(ptd->texfp, ";fill:%s", col2RGBname(fgcol));
	    fprintf(ptd->texfp, ";stroke-opacity:%f", strokeop);
	    fprintf(ptd->texfp, ";fill-opacity:%f", fillop);
	} else {
	    fprintf(ptd->texfp, " stroke=\"%s\"", col2RGBname(col));
	    fprintf(ptd->texfp, " fill=\"%s\"", col2RGBname(fgcol));
	    fprintf(ptd->texfp, " stroke-opacity=\"%f\"", strokeop);
	    fprintf(ptd->texfp, " fill-opacity=\"%f\"", fillop);
	}

    } /* if R_VERSION < 2.0.0 */

    /*Set line pattern type*/
    if (ptd->lty) {
	/* R and SVG use the same convention for specifying dashed lines */
	if (ptd->useStyleAttributes)
	    fprintf(ptd->texfp, ";stroke-dasharray:");
	else
	    fprintf(ptd->texfp, " stroke-dasharray=\"");
	dashleft = ptd->lty;
	for(i=0 ; i<8 && dashleft & 15 ; i++) {
	    /* dashlen is dashleft & 15 */
	    if (i>0)
		fprintf(ptd->texfp, ", ");
	    fprintf(ptd->texfp, "%d", dashleft & 15);
	    dashleft = dashleft >> 4;
	}
	if (!ptd->useStyleAttributes)
	    fprintf(ptd->texfp, "\"");
    }

    if (ptd->useStyleAttributes)
	fprintf(ptd->texfp, "\"");
}

static void SetFont(int face, int size, SVGDesc *ptd)
{
    int lface=face, lsize= size;
    if (lface < 1 || lface > 4) lface = 1;
    if (lsize < 1 || lsize > 24) lsize = 10;

    ptd->fontsize = lsize;
    ptd->fontface = lface;
}

static void SVG_Activate(pDevDesc dd)
{
}

static void SVG_Deactivate(pDevDesc dd)
{
}

static void SVG_MetricInfo(int c,
                           const pGEcontext gc,
                           double* ascent, double* descent,
                           double* width, pDevDesc dd)
{
    /* metric information not available => return 0,0,0 */
    *ascent  = 0.0;
    *descent = 0.0;
    *width   = 0.0;
}


static void SVG_header(SVGDesc *ptd)
{
    int i;
    if (ptd->xmlHeader) {
	/* http://jwatt.org/svg/authoring/ recommends no DTD */
	fprintf(ptd->texfp, "<?xml version=\"1.0\"?>\n");
	fprintf(ptd->texfp, "<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\"\n        \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\">\n");
    }
    /* need to include the links to xlink and ev namespaces for <a xlink:href="URL"></a> to work
       see http://jwatt.org/svg/authoring/ */
    fprintf(ptd->texfp, "<svg version=\"1.1\"\n     baseProfile=\"full\"\n");
    fprintf(ptd->texfp, "     xmlns=\"http://www.w3.org/2000/svg\"\n");
    fprintf(ptd->texfp, "     xmlns:xlink=\"http://www.w3.org/1999/xlink\"\n");
    fprintf(ptd->texfp, "     xmlns:ev=\"http://www.w3.org/2001/xml-events\"\n");
    fprintf(ptd->texfp, "     width=\"%.2f\" height=\"%.2f\"\n",
	    in2dots(ptd->width), in2dots(ptd->height));
    fprintf(ptd->texfp, "     viewBox=\"0 0 %.2f %.2f\"\n",
	    in2dots(ptd->width), in2dots(ptd->height));
    fprintf(ptd->texfp, "     font-family=\"Arial, Helvetica, sans-serif\"\n");
    if (ptd->toolTipMode)
	fprintf(ptd->texfp, "     onload='Init(evt)'\n");

    fprintf(ptd->texfp, ">\n");
    if (ptd->title)
	fprintf(ptd->texfp, "<title>%s</title>\n", ptd->title);
    if (ptd->toolTipMode)
	fprintf(ptd->texfp, "<desc>R SVG Plot with tooltips! (mode=%d)</desc>\n", ptd->toolTipMode);
    else
	fprintf(ptd->texfp, "<desc>R SVG Plot!</desc>\n");
    if (ptd->toolTipMode==1)
	for (i=0; strcmp("!END!", toolTip1Header[i]); i++)
	    fprintf(ptd->texfp, "%s\n", toolTip1Header[i]);
    else if (ptd->toolTipMode==2)
	for (i=0; strcmp("!END!", toolTip2Header[i]); i++)
	    fprintf(ptd->texfp, "%s\n", toolTip2Header[i]);
    /* This element named 'canvas' is what allows map.getBBox() to work */
    fprintf(ptd->texfp,
	    "<rect width=\"100%%\" height=\"100%%\" style=\"fill:%s\" id='canvas'/>\n",
	    col2RGBname(ptd->bg));

    ptd->pageno++;
}

static void SVG_footer(SVGDesc *ptd)
{
    if (ptd->toolTipMode>0) {
	fprintf(ptd->texfp, "<g id='ToolTip' opacity='%g' visibility='hidden' pointer-events='none'>\n",
		ptd->tipOpacity);
	fprintf(ptd->texfp, "   <rect id='tipbox' x='0' y='5' width='88' height='%d' rx='2' ry='2' fill='white' stroke='black'/>\n",
		2 * ptd->tipFontSize);
	fprintf(ptd->texfp, "   <text id='tipText' x='5' y='%d' font-family='Arial' font-size='%dpt' fill='blue'>\n",
		2 * ptd->tipFontSize, ptd->tipFontSize);
	fprintf(ptd->texfp, "      <tspan id='tipTitle' x='5' font-weight='bold' fill='black'><![CDATA[]]></tspan>\n");
	if (ptd->toolTipMode==1) {
	    fprintf(ptd->texfp, "      <tspan id='tipDesc1' x='5' dy='%d'><![CDATA[]]></tspan>\n",
		    (int) ((1.3 * ptd->tipFontSize) + 0.5));
	} else if (ptd->toolTipMode==2) {
	    fprintf(ptd->texfp, "      <tspan id='tipDesc1' x='5' dy='%d'><![CDATA[]]></tspan>\n",
		    (int) ((1.3 * ptd->tipFontSize) + 0.5));
	    fprintf(ptd->texfp, "      <tspan id='tipDesc2' x='5' dy='%d'><![CDATA[]]></tspan>\n",
		    (int) ((1.3 * ptd->tipFontSize) + 0.5));
	} else if (ptd->toolTipMode==3) {
	    fprintf(ptd->texfp, "      <tspan id='tipDesc1' x='5' dy='%d'><![CDATA[]]></tspan>\n",
		    (int) ((1.3 * ptd->tipFontSize) + 0.5));
	    fprintf(ptd->texfp, "      <tspan id='tipDesc2' x='5' dy='%d'><![CDATA[]]></tspan>\n",
		    (int) ((1.3 * ptd->tipFontSize) + 0.5));
	    fprintf(ptd->texfp, "      <tspan id='tipDesc3' x='5' dy='%d'><![CDATA[]]></tspan>\n",
		    (int) ((1.3 * ptd->tipFontSize) + 0.5));
	}
	fprintf(ptd->texfp, "   </text>\n");
	fprintf(ptd->texfp, "</g>\n");
    }
    fprintf(ptd->texfp, "</svg>\n");
}

/* Initialize the device */

static Rboolean SVG_Open(pDevDesc dd, SVGDesc *ptd)
{
    ptd->fontsize = 0;
    ptd->fontface = 0;
    ptd->debug = FALSE;

    ptd->fg = dd->startcol;
    ptd->bg = dd->startfill;
    ptd->col = ptd->fg;

    if (!( ptd->texfp = (FILE *) fopen(R_ExpandFileName(ptd->filename), "w") ))
	return FALSE;

    SVG_header(ptd);

    /* ensure that line drawing is set up at the first */
    /* graphics call */
    ptd->lty = -1;
    ptd->lwd = -1;

    return TRUE;
}

/* Interactive Resize */

/*
static void SVG_Resize(double *left, double *right,
                       double *bottom, double *top,
                       pDevDesc dd)
{

}
*/

static void SVG_Clip(double x0, double x1, double y0, double y1,
                     pDevDesc dd)
{
    SVGDesc *ptd = (SVGDesc *) dd->deviceSpecific;

    ptd->clipleft = x0;
    ptd->clipright = x1;
    ptd->clipbottom = y0;
    ptd->cliptop = y1;
}

/* Start a new page */

static void SVG_NewPage(const pGEcontext gc, pDevDesc dd)
{
    SVGDesc *ptd = (SVGDesc *) dd->deviceSpecific;
    int size;

    if (ptd->onefile){
    } else if (ptd->pageno) {
	SVG_footer(ptd);
	SVG_header(ptd);
    } else ptd->pageno++;

    /* face = ptd->fontface; */
    size = ptd->fontsize;
    ptd->fontface = 0;
    ptd->fontsize = 0;
    SetFont(gc->fontface, size, ptd);
}

/* Close down the driver */

static void SVG_Close(pDevDesc dd)
{
    SVGDesc *ptd = (SVGDesc *) dd->deviceSpecific;

    SVG_footer(ptd);

    fclose(ptd->texfp);

    if (ptd->shapeContents != 0)
	Free(ptd->shapeContents);
    if (ptd->shapeURL != 0)
	Free(ptd->shapeURL);
    if (ptd->shapeURLTarget != 0)
	Free(ptd->shapeURLTarget);
    if (ptd->title != 0)
	Free(ptd->title);
    Free(ptd);
}


static void SVG_Line(double x1, double y1, double x2, double y2,
                     const pGEcontext gc,
                     pDevDesc dd)
{
    SVGDesc *ptd = (SVGDesc *) dd->deviceSpecific;

    fprintf(ptd->texfp, "<line x1=\"%.2f\" y1=\"%.2f\" x2=\"%.2f\" ",
	    x1, y1, x2);

    fprintf(ptd->texfp, "y2=\"%.2f\" ",
            y2);

    SetLinetype(gc->lty, gc->lwd, dd, NA_INTEGER, gc->col);
    fprintf(ptd->texfp, "/>\n");
}

static void SVG_Polyline(int n, double *x, double *y,
                         const pGEcontext gc,
                         pDevDesc dd)
{
    int i;
    SVGDesc *ptd = (SVGDesc *) dd->deviceSpecific;
    fprintf(ptd->texfp, "<polyline points=\"");

    for (i=0; i<n; i++) {
	fprintf(ptd->texfp, "%.2f , %.2f ", x[i], y[i]);
    }
    fprintf(ptd->texfp, "\" ");

    SetLinetype(gc->lty, gc->lwd, dd, NA_INTEGER, gc->col);

    fprintf(ptd->texfp, "/>\n");
}

/* String Width in Rasters */
/* For the current font in pointsize fontsize */

static double SVG_StrWidth(const char *str,
                           const pGEcontext gc,
                           pDevDesc dd)
{
    SVGDesc *ptd = (SVGDesc *) dd->deviceSpecific;

    const char *p;
    int size;
    double sum;
    size =  gc->cex * gc->ps + 0.5;
    SetFont(gc->fontface, size, ptd);
    sum = 0;
    for(p=str ; *p ; p++)
	sum += charwidth[ptd->fontface][(int)*p];

    return sum * size;
}


/* Possibly Filled Rectangle */
static void SVG_Rect(double x0, double y0, double x1, double y1,
                     const pGEcontext gc,
                     pDevDesc dd)
{
    double tmp;
    SVGDesc *ptd = (SVGDesc *) dd->deviceSpecific;

    /*Make sure width and height are positive*/
    if (x0 >= x1){
	tmp = x0;
	x0 = x1;
	x1 = tmp;
    }

    if (y0 >= y1){
	tmp = y0;
	y0 = y1;
	y1 = tmp;
    }

    if (ptd->shapeURL != 0 && !ptd->shapeURLUsed) {
	fprintf(ptd->texfp, "<a xlink:href=\"%s\"\n", ptd->shapeURL);
	if (ptd->shapeURLTarget != 0 && !ptd->shapeURLTargetUsed)
	    fprintf(ptd->texfp, " target=\"%s\"", ptd->shapeURLTarget);
	fprintf(ptd->texfp, ">\n");
    }
    fprintf(ptd->texfp,
	    "<rect x=\"%.2f\" y=\"%.2f\" width=\"%.2f\" height=\"%.2f\" ",
	    x0, y0, x1-x0, y1-y0);


    SetLinetype(gc->lty, gc->lwd, dd, gc->fill, gc->col);
    if (ptd->shapeContents == 0 || ptd->shapeContentsUsed) {
	fprintf(ptd->texfp, " />\n");
    } else {
	fprintf(ptd->texfp, ">\n%s\n", ptd->shapeContents);
	ptd->shapeContentsUsed = 1;
	fprintf(ptd->texfp, "</rect>\n");
    }
    if (ptd->shapeURL != 0 && !ptd->shapeURLUsed) {
	fprintf(ptd->texfp, "</a>\n");
	ptd->shapeURLUsed = 1;
	if (ptd->shapeURLTarget != 0 && !ptd->shapeURLTargetUsed)
	    ptd->shapeURLTargetUsed = 1;
    }
}

static void SVG_Circle(double x, double y, double r,
                       const pGEcontext gc,
                       pDevDesc dd)
{
    SVGDesc *ptd = (SVGDesc *) dd->deviceSpecific;


    if (ptd->shapeURL != 0 && !ptd->shapeURLUsed) {
	fprintf(ptd->texfp, "<a xlink:href=\"%s\"\n", ptd->shapeURL);
	if (ptd->shapeURLTarget != 0 && !ptd->shapeURLTargetUsed)
	    fprintf(ptd->texfp, " target=\"%s\"", ptd->shapeURLTarget);
	fprintf(ptd->texfp, ">\n");
    }
    fprintf(ptd->texfp,
	    "<circle cx=\"%.2f\" cy=\"%.2f\" r=\"%.2f\" ",
	    x, y, r*1.5);

    SetLinetype(gc->lty, gc->lwd, dd, gc->fill, gc->col);

    if (ptd->shapeContents == 0 || ptd->shapeContentsUsed) {
	fprintf(ptd->texfp, " />\n");
    } else {
	fprintf(ptd->texfp, ">\n%s\n", ptd->shapeContents);
	ptd->shapeContentsUsed = 1;
	fprintf(ptd->texfp, "</circle>\n");
    }
    if (ptd->shapeURL != 0 && !ptd->shapeURLUsed) {
	fprintf(ptd->texfp, "</a>\n");
	ptd->shapeURLUsed = 1;
	if (ptd->shapeURLTarget != 0 && !ptd->shapeURLTargetUsed)
	    ptd->shapeURLTargetUsed = 1;
    }

}

static void SVG_Polygon(int n, double *x, double *y,
                        const pGEcontext gc,
                        pDevDesc dd)
{
    int i;

    SVGDesc *ptd = (SVGDesc *) dd->deviceSpecific;


    if (ptd->shapeURL != 0 && !ptd->shapeURLUsed) {
	fprintf(ptd->texfp, "<a xlink:href=\"%s\"\n", ptd->shapeURL);
	if (ptd->shapeURLTarget != 0 && !ptd->shapeURLTargetUsed)
	    fprintf(ptd->texfp, " target=\"%s\"", ptd->shapeURLTarget);
	fprintf(ptd->texfp, ">\n");
    }

    fprintf(ptd->texfp, "<polygon points=\"");

    for (i=0; i<n; i++) {
	fprintf(ptd->texfp, "%.2f , %.2f ", x[i], y[i]);
    }

    fprintf(ptd->texfp, "\" ");

    SetLinetype(gc->lty, gc->lwd, dd, gc->fill, gc->col);

    if (ptd->shapeContents == 0 || ptd->shapeContentsUsed) {
	fprintf(ptd->texfp, " />\n");
    } else {
	fprintf(ptd->texfp, ">\n%s\n", ptd->shapeContents);
	ptd->shapeContentsUsed = 1;
	fprintf(ptd->texfp, "</polygon>\n");
    }
    if (ptd->shapeURL != 0 && !ptd->shapeURLUsed) {
	fprintf(ptd->texfp, "</a>\n");
	ptd->shapeURLUsed = 1;
	if (ptd->shapeURLTarget != 0 && !ptd->shapeURLTargetUsed)
	    ptd->shapeURLTargetUsed = 1;
    }
}
/*
 * Add SVG_Path soon!
 */

static void textext(const char *str, SVGDesc *ptd)
{
    const char *c;
    for( ; *str ; str++)
        switch(*str) {
	case '&':
	    /* Check if we have the pattern "&#?[A-Za-z0-9]+;" which would already be an XML entity
	     * XML entities are things like "&amp;" (ampersand) and "&#x3b1;" ('alpha' in Greek font)
	     * It might be possible to make this test a bit stricter (e.g., only allow numbers or
	     * upper case if there is a '#' directly after the '&' - look at doing this if it
	     * seems necessary.
	     */
	    c = str+1;
	    if (*c=='#')
		c++;
	    while (*c && isalnum(*c))
		c++;
	    /* Now, if *c is a ';' it means we found an XML entity, so in that case
	     * write the '&' and allow the enclosing for() loop to write the rest.
	     * If *c is not a ';', it means that *str was an ampersand that needs
	     * to be encoded as '&amp;', so write that out.
	     */
	    if (*c == ';')
		fputc(*str, ptd->texfp);
	    else
		fputs("&amp;", ptd->texfp);
	    break;
	case ' ': /* &nbsp; == &#160; */
	    fputs("&#160;", ptd->texfp);
	    break;
	case '<':
	    fputs("&lt;", ptd->texfp);
	    break;
	case '>':
	    fputs("&gt;", ptd->texfp);
	    break;
	case '\'':
	    fputs("&apos;", ptd->texfp);
	    break;
	case '"':
	    fputs("&quot;", ptd->texfp);
	    break;
        default:
            fputc(*str, ptd->texfp);
            break;
        }

}


/* Rotated Text */

static void SVG_Text(double x, double y, const char *str,
                     double rot, double hadj,
                     const pGEcontext gc,
                     pDevDesc dd)
{
    int size;

    SVGDesc *ptd = (SVGDesc *) dd->deviceSpecific;

    size = gc->cex * gc->ps + 0.5;

    if (ptd->shapeURL != 0 && !ptd->shapeURLUsed) {
	fprintf(ptd->texfp, "<a xlink:href=\"%s\"\n", ptd->shapeURL);
	if (ptd->shapeURLTarget != 0 && !ptd->shapeURLTargetUsed)
	    fprintf(ptd->texfp, " target=\"%s\"", ptd->shapeURLTarget);
	fprintf(ptd->texfp, ">\n");
    }

    fprintf(ptd->texfp, "<text transform=\"translate(%.2f,%.2f) ", x, y);
    if (rot != 0)
	fprintf(ptd->texfp, " rotate(%0.0f)\" ",-1.0*rot);
    else
	fprintf(ptd->texfp, "\" ");

    if (gc->fontfamily && strlen(gc->fontfamily))
	fprintf(ptd->texfp, "font-family=\"%s\" ", gc->fontfamily);
    fprintf(ptd->texfp, "fill=\"%s\" ", col2RGBname(gc->col));

    SetFont(gc->fontface, size, ptd);
    /* original code had no units on the font-size, but I found that
     * gave errors in the SVG renderer in FireFox under MS Windows
     */
    fprintf(ptd->texfp, " style=\"font-size:%dpt\" ", ptd->fontsize);

    fprintf(ptd->texfp, ">");

    textext(str, ptd);

    if (ptd->shapeContents != 0 && ! ptd->shapeContentsUsed) {
	fprintf(ptd->texfp, "%s\n", ptd->shapeContents);
	ptd->shapeContentsUsed = 1;
    }

    fprintf(ptd->texfp, "</text>\n");
    if (ptd->shapeURL != 0 && !ptd->shapeURLUsed) {
	fprintf(ptd->texfp, "</a>\n");
	ptd->shapeURLUsed = 1;
	if (ptd->shapeURLTarget != 0 && !ptd->shapeURLTargetUsed)
	    ptd->shapeURLTargetUsed = 1;
    }
}

/* Pick */
static Rboolean SVG_Locator(double *x, double *y, pDevDesc dd)
{
    return FALSE;
}

/* Set Graphics mode - not needed for PS */
static void SVG_Mode(int mode, pDevDesc dd)
{
}

static SEXP SVG_Cap(pDevDesc dd)
{
    SEXP raster = R_NilValue;
    return raster;
}

static void SVG_Raster(unsigned int *raster, int w, int h,
		       double x, double y,
		       double width, double height,
		       double rot,
		       Rboolean interpolate,
		       const pGEcontext gc, pDevDesc dd)
{
}

Rboolean SVGDeviceDriver(pDevDesc dd, char *filename, char *bg, char *fg,
                         double width, double height, Rboolean debug,
                         Rboolean xmlHeader, char *title, int toolTipMode,
			 int tipFontSize, double tipOpacity,
			 Rboolean onefile, Rboolean useStyleAttributes)
{
    SVGDesc *ptd;

    if (!(ptd = Calloc(1, SVGDesc)))
        return FALSE;
    ptd->magic = SVGDescMagic;
    ptd->shapeContents = 0;
    ptd->shapeContentsUsed = 0;
    ptd->shapeURL = 0;
    ptd->shapeURLTarget = 0;
    ptd->shapeURLUsed = 0;
    ptd->toolTipMode = ((toolTipMode>0 && toolTipMode<=2) ? toolTipMode : 0);
    ptd->title = 0;
    if (title && strlen(title))
	if ((ptd->title = Calloc(strlen(title)+1, char)))
	    strcpy(ptd->title, title);

    ptd->filename = Calloc(strlen(filename)+1, char);
    strcpy(ptd->filename, filename);

    dd->startfill = R_GE_str2col(bg);
    dd->startcol = R_GE_str2col(fg);
    dd->startps = 10;
    dd->startlty = 0;
    dd->startfont = 1;
    dd->startgamma = 1;

    dd->activate = SVG_Activate;
    dd->deactivate = SVG_Deactivate;
    dd->close = SVG_Close;
    dd->clip = SVG_Clip;
    dd->size = SVG_Size;
    dd->newPage = SVG_NewPage;
    dd->line = SVG_Line;
    dd->text = SVG_Text;
    dd->strWidth = SVG_StrWidth;
    dd->rect = SVG_Rect;
    dd->circle = SVG_Circle;
    dd->polygon = SVG_Polygon;
    dd->polyline = SVG_Polyline;
    dd->locator = SVG_Locator;
    dd->mode = SVG_Mode;
    dd->metricInfo = SVG_MetricInfo;
    dd->cap = SVG_Cap;
    dd->raster = SVG_Raster;

    /* Screen Dimensions in Pixels */

    dd->left = 0;               /* left */
    dd->right = in2dots(width);/* right */
    dd->bottom = in2dots(height);               /* bottom */
    dd->top = 0;  /* top */
    ptd->width = width;
    ptd->height = height;
    ptd->xmlHeader = xmlHeader;
    ptd->useStyleAttributes = useStyleAttributes;
    ptd->onefile   = onefile;
    ptd->tipFontSize = tipFontSize;
    ptd->tipOpacity = tipOpacity;

    if ( ! SVG_Open(dd, ptd) )
        return FALSE;

    /* Base Pointsize */
    /* Nominal Character Sizes in Pixels */
    /* cra[1] tuned to get margins OK in svgplot4 in RSVGTipsDevice/man/RSVGTipsDevice.Rd
     * after setting default font to Arial */
    dd->cra[0] =         (6.0/12.0) * 10.0;
    dd->cra[1] =        (13.0/12.0) * 10.0;

    /* Character Addressing Offsets */
    /* These offsets should center a single */
    /* plotting character over the plotting point. */
    /* Pure guesswork and eyeballing ... */

    dd->xCharOffset =  0; /*0.4900;*/
    dd->yCharOffset =  0.5; /*0.3333;*/
    dd->yLineBias = 0; /*0.1;*/

    /* Inches per Raster Unit */
    /* We use printer points, i.e. 72.27 dots per inch : */
    dd->ipr[0] = dd->ipr[1] = 1./DOTSperIN;

    dd->canClip = FALSE;
    dd->canHAdj = 0;
    dd->canChangeGamma = FALSE;

    ptd->lty = 1;
    ptd->pageno = 0;
    ptd->debug = debug;

    dd->deviceSpecific = (void *) ptd;
    dd->displayListOn = FALSE;
    return TRUE;
}

static  pGEDevDesc RSvgDevice(char **file, char **bg, char **fg,
                              double *width, double *height, int *debug,
                              int *xmlHeader, char **title, int *toolTipMode,
			      int *tipFontSize, double *tipOpacity,
			      int *onefile, int *useStyleAttributes)
{
    pGEDevDesc dd;
    pDevDesc dev;

    if (debug[0] == NA_LOGICAL) debug = FALSE;

    R_GE_checkVersionOrDie(R_GE_version);
    R_CheckDeviceAvailable();
    BEGIN_SUSPEND_INTERRUPTS {
        if (!(dev = Calloc(1, DevDesc)))
            error("unable to allocate memory for DevDesc (small)");
        if (!SVGDeviceDriver(dev, file[0], bg[0], fg[0], width[0], height[0], debug[0],
			     xmlHeader[0], title[0], toolTipMode[0], tipFontSize[0],
			     tipOpacity[0], onefile[0], useStyleAttributes[0])) {
            Free(dev);
            error("unable to start device SVG");
        }
        dd = GEcreateDevDesc(dev);

#if R_VERSION < R_Version(2,7,0)
        gsetVar(install(".Device"), mkString("devSVG"), R_NilValue);
	Rf_addDevice((DevDesc*) dd);
#else
        GEaddDevice2(dd, "devSVG");
#endif
        GEinitDisplayList(dd);
    } END_SUSPEND_INTERRUPTS;

    return(dd);
}

void do_SVG(char **file, char **bg, char **fg, double *width, double *height,
            int *debug, int *xmlHeader, char **title, int *toolTipMode,
	    int *tipFontSize, double *tipOpacity,
	    int *onefile, int *useStyleAttributes)
{
    char *vmax;

    vmax = vmaxget();

    RSvgDevice(file, bg, fg, width, height, debug, xmlHeader, title, toolTipMode,
	       tipFontSize, tipOpacity, onefile, useStyleAttributes);

    vmaxset(vmax);
}
