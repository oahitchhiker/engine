/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// tr_bloom.c: General post-processing shader stuff including bloom, leifx, and everything else
//		that's postprocessed.  Maintained by leilei and Hitchiker 

#include "tr_local.h"

static cvar_t *r_bloom;
static cvar_t *r_bloom_sample_size;
static cvar_t *r_bloom_fast_sample;
static cvar_t *r_bloom_alpha;
static cvar_t *r_bloom_darken;
static cvar_t *r_bloom_intensity;
static cvar_t *r_bloom_diamond_size;
static cvar_t *r_bloom_cascade;
static cvar_t *r_bloom_cascade_blur;
static cvar_t *r_bloom_cascade_intensity;
static cvar_t *r_bloom_cascade_alpha;
static cvar_t *r_bloom_cascade_dry;
static cvar_t *r_bloom_dry;
extern cvar_t	*r_overBrightBits;
extern cvar_t	*r_gamma;
static cvar_t *r_bloom_reflection;		// LEILEI
static cvar_t *r_bloom_sky_only;		// LEILEI





extern float ScreenFrameCount;

static struct {
	struct {
		image_t	*texture;
		int		width, height;
		float	readW, readH;
	} effect;
	struct {
		image_t	*texture;
		int		width, height;
		float	readW, readH;
	} screen;
	struct {
		image_t	*texture;
		int		width, height;
		float	readW, readH;
	} depth;
	struct {
		int		width, height;
	} work;

		// leilei - motion blur
	struct {
		image_t	*texture;
		int		width, height;
		float	readW, readH;
	} motion1;
	struct {
		image_t	*texture;
		int		width, height;
		float	readW, readH;
	} motion2;
	struct {
		image_t	*texture;
		int		width, height;
		float	readW, readH;
	} motion3;
	struct {
		image_t	*texture;
		int		width, height;
		float	readW, readH;
	} motion4;
	struct {
		image_t	*texture;
		int		width, height;
		float	readW, readH;
	} motion5;
	struct {
		image_t	*texture;
		int		width, height;
		float	readW, readH;
	} mpass1;
	struct {
		image_t	*texture;
		int		width, height;
		float	readW, readH;
	} mpass2;
	struct {
		image_t	*texture;
		int		width, height;
		float	readW, readH;
	} mpass3;

	struct {
		image_t	*texture;
		int		width, height;
		float	readW, readH;
	} mpass4;
	struct {
		image_t	*texture;
		int		width, height;
		float	readW, readH;
	} tv;
	struct {
		int		width, height;
	} tvwork;
	struct {
		image_t	*texture;
		int		width, height;
		float	readW, readH;
	} tveffect;

	qboolean started;
} postproc;



cvar_t *r_film;
extern int	force32upload;		
int		leifxmode;
int		leifxpass;
int		fakeit = 0;

int 	tvinterlace = 1;
int 	tvinter= 1;
extern int tvWidth;
extern int tvHeight;
extern float tvAspectW;	// aspect correction
extern int vresWidth;
extern int vresHeight;
/* 
============================================================================== 
 
						LIGHT BLOOMS
 
============================================================================== 
*/ 

static float Diamond8x[8][8] =
{ 
	{ 0.0f, 0.0f, 0.0f, 0.1f, 0.1f, 0.0f, 0.0f, 0.0f, },
	{ 0.0f, 0.0f, 0.2f, 0.3f, 0.3f, 0.2f, 0.0f, 0.0f, },
	{ 0.0f, 0.2f, 0.4f, 0.6f, 0.6f, 0.4f, 0.2f, 0.0f, },
	{ 0.1f, 0.3f, 0.6f, 0.9f, 0.9f, 0.6f, 0.3f, 0.1f, },
	{ 0.1f, 0.3f, 0.6f, 0.9f, 0.9f, 0.6f, 0.3f, 0.1f, },
	{ 0.0f, 0.2f, 0.4f, 0.6f, 0.6f, 0.4f, 0.2f, 0.0f, },
	{ 0.0f, 0.0f, 0.2f, 0.3f, 0.3f, 0.2f, 0.0f, 0.0f, },
	{ 0.0f, 0.0f, 0.0f, 0.1f, 0.1f, 0.0f, 0.0f, 0.0f  }
};


static float Star8x[8][8] =
{ 
	{ 0.4f, 0.1f, 0.0f, 0.0f, 0.0f, 0.0f, 0.1f, 0.4f, },
	{ 0.1f, 0.6f, 0.2f, 0.0f, 0.0f, 0.2f, 0.6f, 0.1f, },
	{ 0.0f, 0.2f, 0.7f, 0.6f, 0.6f, 0.7f, 0.2f, 0.0f, },
	{ 0.0f, 0.0f, 0.6f, 0.9f, 0.9f, 0.6f, 0.0f, 0.0f, },
	{ 0.0f, 0.0f, 0.6f, 0.9f, 0.9f, 0.6f, 0.0f, 0.0f, },
	{ 0.0f, 0.2f, 0.7f, 0.6f, 0.6f, 0.7f, 0.2f, 0.0f, },
	{ 0.1f, 0.6f, 0.2f, 0.0f, 0.0f, 0.2f, 0.6f, 0.1f, },
	{ 0.4f, 0.1f, 0.0f, 0.0f, 0.0f, 0.0f, 0.1f, 0.4f, }
};


static float Diamond6x[6][6] =
{ 
	{ 0.0f, 0.0f, 0.1f, 0.1f, 0.0f, 0.0f, },
	{ 0.0f, 0.3f, 0.5f, 0.5f, 0.3f, 0.0f, }, 
	{ 0.1f, 0.5f, 0.9f, 0.9f, 0.5f, 0.1f, },
	{ 0.1f, 0.5f, 0.9f, 0.9f, 0.5f, 0.1f, },
	{ 0.0f, 0.3f, 0.5f, 0.5f, 0.3f, 0.0f, },
	{ 0.0f, 0.0f, 0.1f, 0.1f, 0.0f, 0.0f  }
};

static float Diamond4x[4][4] =
{  
	{ 0.3f, 0.4f, 0.4f, 0.3f, },
	{ 0.4f, 0.9f, 0.9f, 0.4f, },
	{ 0.4f, 0.9f, 0.9f, 0.4f, },
	{ 0.3f, 0.4f, 0.4f, 0.3f  }
};

static struct {
	struct {
		image_t	*texture;
		int		width, height;
		float	readW, readH;
	} effect;
	struct {
		image_t	*texture;
		int		width, height;
		float	readW, readH;
	} effect2;
	struct {
		image_t	*texture;
		int		width, height;
		float	readW, readH;
	} screen;
	struct {
		int		width, height;
	} work;
	qboolean started;
} bloom;



static void ID_INLINE R_Bloom_Quad( int width, int height, float texX, float texY, float texWidth, float texHeight ) {
	int x = 0;
	int y = 0;
	x = 0;
	y += glConfig.vidHeight - height;
	width += x;
	height += y;
	
	texWidth += texX;
	texHeight += texY;

	qglBegin( GL_QUADS );							
	qglTexCoord2f(	texX,						texHeight	);	
	qglVertex2f(	x,							y	);

	qglTexCoord2f(	texX,						texY	);				
	qglVertex2f(	x,							height	);	

	qglTexCoord2f(	texWidth,					texY	);				
	qglVertex2f(	width,						height	);	

	qglTexCoord2f(	texWidth,					texHeight	);	
	qglVertex2f(	width,						y	);				
	qglEnd ();
}


// LEILEI - Bloom Reflection


static void ID_INLINE R_Bloom_Quad_Lens(float offsert, int width, int height, float texX, float texY, float texWidth, float texHeight) {
	int x = 0;
	int y = 0;
	x = 0;
	y += glConfig.vidHeight - height;
	width += x;
	height += y;
	offsert = offsert * 9; // bah
	texWidth -= texX;
	texHeight -= texY;

	qglBegin( GL_QUADS );							
	qglTexCoord2f(	texX,						texHeight	);	
	qglVertex2f(	width + offsert,							height + offsert	);

	qglTexCoord2f(	texX,						texY	);				
	qglVertex2f(	width + offsert,							y	- offsert);	

	qglTexCoord2f(	texWidth,					texY	);				
	qglVertex2f(	x - offsert,						y	- offsert);	

	qglTexCoord2f(	texWidth,					texHeight	);	
	qglVertex2f(	x - offsert,						height	+ offsert);			
	qglEnd ();
}


// LEILEI - end

/*
=================
R_Bloom_InitTextures
=================
*/
static void R_Bloom_InitTextures( void )
{
	byte	*data;

	// find closer power of 2 to screen size 
	for (bloom.screen.width = 1;bloom.screen.width< glConfig.vidWidth;bloom.screen.width *= 2);
	for (bloom.screen.height = 1;bloom.screen.height < glConfig.vidHeight;bloom.screen.height *= 2);

	bloom.screen.readW = glConfig.vidWidth / (float)bloom.screen.width;
	bloom.screen.readH = glConfig.vidHeight / (float)bloom.screen.height;

	// find closer power of 2 to effect size 
	bloom.work.width = r_bloom_sample_size->integer;
	bloom.work.height = bloom.work.width * ( glConfig.vidWidth / glConfig.vidHeight );

	for (bloom.effect.width = 1;bloom.effect.width < bloom.work.width;bloom.effect.width *= 2);
	for (bloom.effect.height = 1;bloom.effect.height < bloom.work.height;bloom.effect.height *= 2);

	bloom.effect.readW = bloom.work.width / (float)bloom.effect.width;
	bloom.effect.readH = bloom.work.height / (float)bloom.effect.height;
	
	bloom.effect2.readW=bloom.effect.readW;
	bloom.effect2.readH=bloom.effect.readH;
	bloom.effect2.width=bloom.effect.width;
	bloom.effect2.height=bloom.effect.height;
	

	// disable blooms if we can't handle a texture of that size
	if( bloom.screen.width > glConfig.maxTextureSize ||
		bloom.screen.height > glConfig.maxTextureSize ||
		bloom.effect.width > glConfig.maxTextureSize ||
		bloom.effect.height > glConfig.maxTextureSize ||
		bloom.work.width > glConfig.vidWidth ||
		bloom.work.height > glConfig.vidHeight
	) {
		ri.Cvar_Set( "r_bloom", "0" );
		Com_Printf( S_COLOR_YELLOW"WARNING: 'R_InitBloomTextures' too high resolution for light bloom, effect disabled\n" );
		return;
	}

	// leilei - let's not do that bloom disabling anymore
	force32upload = 1;

	data = ri.Hunk_AllocateTempMemory( bloom.screen.width * bloom.screen.height * 4 );
	Com_Memset( data, 0, bloom.screen.width * bloom.screen.height * 4 );
	bloom.screen.texture = R_CreateImage( "***bloom screen texture***", data, bloom.screen.width, bloom.screen.height, qfalse, qfalse, GL_CLAMP_TO_EDGE  );
	ri.Hunk_FreeTempMemory( data );

	data = ri.Hunk_AllocateTempMemory( bloom.effect.width * bloom.effect.height * 4 );
	Com_Memset( data, 0, bloom.effect.width * bloom.effect.height * 4 );
	bloom.effect.texture = R_CreateImage( "***bloom effect texture***", data, bloom.effect.width, bloom.effect.height, qfalse, qfalse, GL_CLAMP_TO_EDGE  );
	bloom.effect2.texture = R_CreateImage( "***bloom effect texture 2***", data, bloom.effect.width, bloom.effect.height, qfalse, qfalse, GL_CLAMP_TO_EDGE  );
	ri.Hunk_FreeTempMemory( data );
	bloom.started = qtrue;
	force32upload = 0;
}



/*
=================
R_InitBloomTextures
=================
*/
void R_InitBloomTextures( void )
{
	if( !r_bloom->integer )
		return;
	memset( &bloom, 0, sizeof( bloom ));
	R_Bloom_InitTextures ();
}
/*
=================
R_Bloom_DrawEffect
=================
*/
static void R_Bloom_DrawEffect( void )
{
	float alpha=r_bloom_alpha->value;
	GL_Bind( bloom.effect.texture );
	GL_State( GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE );
	if(r_bloom_cascade->integer){
		alpha=r_bloom_cascade_alpha->value;
	}
	qglColor4f( alpha,alpha,alpha, 1.0f );
	R_Bloom_Quad( glConfig.vidWidth, glConfig.vidHeight, 0, 0, bloom.effect.readW, bloom.effect.readW );
}


/*
=================
R_Bloom_LensEffect


LEILEI's Silly hack
=================
*/
static void R_Bloom_LensEffect( void )
{
	GL_Bind( bloom.effect.texture );
	GL_State( GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE );

	qglColor4f( 0.78f, 0.23f, 0.34f, 0.07f );
	R_Bloom_Quad_Lens(16, glConfig.vidWidth, glConfig.vidHeight, 0, 0, bloom.effect.readW, bloom.effect.readW );
	qglColor4f( 0.78f, 0.39f, 0.21f, 0.07f );
	R_Bloom_Quad_Lens(32, glConfig.vidWidth, glConfig.vidHeight, 0, 0, bloom.effect.readW, bloom.effect.readW );
	qglColor4f( 0.78f, 0.59f, 0.21f, 0.07f );
	R_Bloom_Quad_Lens(48, glConfig.vidWidth, glConfig.vidHeight, 0, 0, bloom.effect.readW, bloom.effect.readW );
	qglColor4f( 0.71f, 0.75f, 0.21f, 0.07f );
	R_Bloom_Quad_Lens(64, glConfig.vidWidth, glConfig.vidHeight, 0, 0, bloom.effect.readW, bloom.effect.readW );
	qglColor4f( 0.52f, 0.78f, 0.21f, 0.07f );
	R_Bloom_Quad_Lens(80, glConfig.vidWidth, glConfig.vidHeight, 0, 0, bloom.effect.readW, bloom.effect.readW );
	qglColor4f( 0.32f, 0.78f, 0.21f, 0.07f );
	R_Bloom_Quad_Lens(96, glConfig.vidWidth, glConfig.vidHeight, 0, 0, bloom.effect.readW, bloom.effect.readW );
	qglColor4f( 0.21f, 0.78f, 0.28f, 0.07f );
	R_Bloom_Quad_Lens(112, glConfig.vidWidth, glConfig.vidHeight, 0, 0, bloom.effect.readW, bloom.effect.readW );
	qglColor4f( 0.21f, 0.78f, 0.47f, 0.07f );
	R_Bloom_Quad_Lens(128, glConfig.vidWidth, glConfig.vidHeight, 0, 0, bloom.effect.readW, bloom.effect.readW );
	qglColor4f( 0.21f, 0.77f, 0.66f, 0.07f );
	R_Bloom_Quad_Lens(144, glConfig.vidWidth, glConfig.vidHeight, 0, 0, bloom.effect.readW, bloom.effect.readW );
	qglColor4f( 0.21f, 0.67f, 0.78f, 0.07f );
	R_Bloom_Quad_Lens(160, glConfig.vidWidth, glConfig.vidHeight, 0, 0, bloom.effect.readW, bloom.effect.readW );
	qglColor4f( 0.21f, 0.47f, 0.78f, 0.07f );
	R_Bloom_Quad_Lens(176, glConfig.vidWidth, glConfig.vidHeight, 0, 0, bloom.effect.readW, bloom.effect.readW );
	qglColor4f( 0.21f, 0.28f, 0.78f, 0.07f );
	R_Bloom_Quad_Lens(192, glConfig.vidWidth, glConfig.vidHeight, 0, 0, bloom.effect.readW, bloom.effect.readW );
	qglColor4f( 0.35f, 0.21f, 0.78f, 0.07f );
	R_Bloom_Quad_Lens(208, glConfig.vidWidth, glConfig.vidHeight, 0, 0, bloom.effect.readW, bloom.effect.readW );
	qglColor4f( 0.53f, 0.21f, 0.78f, 0.07f );
	R_Bloom_Quad_Lens(224, glConfig.vidWidth, glConfig.vidHeight, 0, 0, bloom.effect.readW, bloom.effect.readW );
	qglColor4f( 0.72f, 0.21f, 0.75f, 0.07f );
	R_Bloom_Quad_Lens(240, glConfig.vidWidth, glConfig.vidHeight, 0, 0, bloom.effect.readW, bloom.effect.readW );
	qglColor4f( 0.78f, 0.21f, 0.59f, 0.07f );
	R_Bloom_Quad_Lens(256, glConfig.vidWidth, glConfig.vidHeight, 0, 0, bloom.effect.readW, bloom.effect.readW );

}



/*
 =================
 R_Bloom_Cascaded
 =================
 Tcpp: sorry for my poor English skill.
 */
static void R_Bloom_Cascaded( void ){
	int scale;
	int oldWorkW, oldWorkH;
	int newWorkW, newWorkH;
	float bloomShiftX=r_bloom_cascade_blur->value/(float)bloom.effect.width;
	float bloomShiftY=r_bloom_cascade_blur->value/(float)bloom.effect.height;
	float intensity=r_bloom_cascade_intensity->value;
	float intensity2;
	
	qglColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
	//Take the backup texture and downscale it
	GL_Bind( bloom.screen.texture );
	GL_State( GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO );
	R_Bloom_Quad( bloom.work.width, bloom.work.height, 0, 0, bloom.screen.readW, bloom.screen.readH );
	//Copy downscaled framebuffer into a texture
	GL_Bind( bloom.effect.texture );
	qglCopyTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, 0, 0, bloom.work.width, bloom.work.height );
	
	/* Copy the result to the effect texture */
	GL_Bind( bloom.effect2.texture );
	qglCopyTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, 0, 0, bloom.work.width, bloom.work.height );
	
	// do blurs..
	scale=32;
	while(bloom.work.width<scale)
		scale>>=1;
	while(bloom.work.height<scale)
		scale>>=1;
	
	// prepare the first level.
	newWorkW=bloom.work.width/scale;
	newWorkH=bloom.work.height/scale;
	
	GL_Bind( bloom.effect2.texture );
	GL_State( GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO );
	intensity2=intensity/(float)scale;
	qglColor4f( intensity2, intensity2, intensity2, 1.0 );
	R_Bloom_Quad( newWorkW, newWorkH, 
				 0, 0, 
				 bloom.effect2.readW, bloom.effect2.readH );
	
	// go through levels.
	while(scale>1){
		float oldScaleInv=1.f/(float)scale;
		scale>>=1;
		oldWorkH=newWorkH;
		oldWorkW=newWorkW;
		newWorkW=bloom.work.width/scale;
		newWorkH=bloom.work.height/scale;
		
		// get effect texture.
		GL_Bind( bloom.effect.texture );
		qglCopyTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, 0, 0, oldWorkW, oldWorkH);
		
		// maginfy the previous level.
		if(r_bloom_cascade_blur->value<.01f){
			// don't blur.
			qglColor4f( 1.f, 1.f, 1.f, 1.0 );
			GL_State( GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO );
			R_Bloom_Quad( newWorkW, newWorkH, 
						 0, 0, 
						 bloom.effect.readW*oldScaleInv, bloom.effect.readH*oldScaleInv );
		}else{
			// blur.
			qglColor4f( .25f, .25f, .25f, 1.0 );
			GL_State( GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO );
			R_Bloom_Quad( newWorkW, newWorkH, 
						 -bloomShiftX, -bloomShiftY, 
						 bloom.effect.readW*oldScaleInv, bloom.effect.readH*oldScaleInv );
			
			GL_State( GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE );
			R_Bloom_Quad( newWorkW, newWorkH, 
						 bloomShiftX, -bloomShiftY, 
						 bloom.effect.readW*oldScaleInv, bloom.effect.readH*oldScaleInv );
			R_Bloom_Quad( newWorkW, newWorkH, 
						 -bloomShiftX, bloomShiftY, 
						 bloom.effect.readW*oldScaleInv, bloom.effect.readH*oldScaleInv );
			R_Bloom_Quad( newWorkW, newWorkH, 
						 bloomShiftX, bloomShiftY, 
						 bloom.effect.readW*oldScaleInv, bloom.effect.readH*oldScaleInv );
		}
		
		// add the input.
		intensity2=intensity/(float)scale;
		qglColor4f( intensity2, intensity2, intensity2, 1.0 );
		
		GL_State( GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE );
		GL_Bind( bloom.effect2.texture );
		
		R_Bloom_Quad( newWorkW, newWorkH, 
					 0, 0, 
					 bloom.effect2.readW, bloom.effect2.readH );
		
		
	}
	
	GL_Bind( bloom.effect.texture );
	qglCopyTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, 0, 0, bloom.work.width, bloom.work.height );
	
}

/*
=================
R_Bloom_GeneratexDiamonds
=================
*/
static void R_Bloom_WarsowEffect( void )
{
	int		i, j, k;
	float	intensity, scale, *diamond;


	qglColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
	//Take the backup texture and downscale it
	GL_Bind( bloom.screen.texture );
	GL_State( GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO );
	R_Bloom_Quad( bloom.work.width, bloom.work.height, 0, 0, bloom.screen.readW, bloom.screen.readH );
	//Copy downscaled framebuffer into a texture
	GL_Bind( bloom.effect.texture );
	qglCopyTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, 0, 0, bloom.work.width, bloom.work.height );
	// darkening passes with repeated filter
	if( r_bloom_darken->integer ) {
		int i;
		GL_State( GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO );

		for( i = 0; i < r_bloom_darken->integer; i++ ) {
			R_Bloom_Quad( bloom.work.width, bloom.work.height, 
				0, 0, 
				bloom.effect.readW, bloom.effect.readH );
		}
		qglCopyTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, 0, 0, bloom.work.width, bloom.work.height );
	}
	/* Copy the result to the effect texture */
	GL_Bind( bloom.effect.texture );
	qglCopyTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, 0, 0, bloom.work.width, bloom.work.height );

	// bluring passes, warsow uses a repeated semi blend on a selectable diamond grid
	qglColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
	GL_State( GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE_MINUS_SRC_COLOR );
	if( r_bloom_diamond_size->integer > 7 || r_bloom_diamond_size->integer <= 3 ) {
		if( r_bloom_diamond_size->integer != 8 )
			ri.Cvar_Set( "r_bloom_diamond_size", "8" );
	} else if( r_bloom_diamond_size->integer > 5 ) {
		if( r_bloom_diamond_size->integer != 6 )
			ri.Cvar_Set( "r_bloom_diamond_size", "6" );
	} else if( r_bloom_diamond_size->integer > 3 ) {
		if( r_bloom_diamond_size->integer != 4 )
			ri.Cvar_Set( "r_bloom_diamond_size", "4" );
	}

	switch( r_bloom_diamond_size->integer ) {
		case 4:
			k = 2;
			diamond = &Diamond4x[0][0];
			scale = r_bloom_intensity->value * 0.8f;
			break;
		case 6:
			k = 3;
			diamond = &Diamond6x[0][0];
			scale = r_bloom_intensity->value * 0.5f;
			break;
		case 9:
			k = 4;
			diamond = &Star8x[0][0];
			scale = r_bloom_intensity->value * 0.3f;
			break;

		default:
		case 8:
			k = 4;
			diamond = &Diamond8x[0][0];
			scale = r_bloom_intensity->value * 0.3f;
			break;
	}

	for( i = 0; i < r_bloom_diamond_size->integer; i++ ) {
		for( j = 0; j < r_bloom_diamond_size->integer; j++, diamond++ ) {
			float x, y;
			intensity =  *diamond * scale;
			if( intensity < 0.01f )
				continue;
			qglColor4f( intensity, intensity, intensity, 1.0 );
			x = (i - k) * ( 2 / 640.0f ) * bloom.effect.readW;
			y = (j - k) * ( 2 / 480.0f ) * bloom.effect.readH;

			R_Bloom_Quad( bloom.work.width, bloom.work.height, x, y, bloom.effect.readW, bloom.effect.readH );
		}
	}
	qglCopyTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, 0, 0, bloom.work.width, bloom.work.height );
}											

/*
=================
R_Bloom_BackupScreen
Backup the full original screen to a texture for downscaling and later restoration
=================
*/
static void R_Bloom_BackupScreen( void ) {
	GL_Bind( bloom.screen.texture );
	qglCopyTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, 0, 0, glConfig.vidWidth, glConfig.vidHeight );
}

/*
=================
R_Bloom_RestoreScreen
Restore the temporary framebuffer section we used with the backup texture
=================
*/
static void R_Bloom_RestoreScreen( void ) {
	float dry=r_bloom_dry->value;
	if(r_bloom_cascade->integer)
		dry=r_bloom_cascade_dry->value;
	GL_State( GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO );
	GL_Bind( bloom.screen.texture );
	qglColor4f( dry,dry,dry, 1 );
	if(dry<.99f){
		R_Bloom_Quad( bloom.screen.width, bloom.screen.height, 0, 0,
					1.f,
					1.f );
	}else{
		R_Bloom_Quad( bloom.work.width, bloom.work.height, 0, 0,
			bloom.work.width / (float)bloom.screen.width,
			bloom.work.height / (float)bloom.screen.height );
	}
}
/*
=================
R_Bloom_RestoreScreen_Postprocessed
Restore the temporary framebuffer section we used with the backup texture
=================
*/
extern int mpasses;

static void ID_INLINE R_Bloom_QuadTV( int width, int height, float texX, float texY, float texWidth, float texHeight, int aa ) {
	int x = 0;
	int y = 0;
	float aspcenter = 0;
	//int aspoff = 0;
	float raa = r_retroAA->value;
	if (raa < 1) raa = 1;

	float xpix = 1.0f / width / (4 / raa);
	float ypix = 1.0f / height / (4 / raa);
	float xaa;
	float yaa;
	x = 0;
	y = 0;

	


	if (aa == 0){	xaa = 0; yaa = 0;   }
	if (aa == 1){	xaa = -xpix; yaa = ypix;   }
	if (aa == 2){	xaa = -xpix; yaa = -ypix;   }
	if (aa == 3){	xaa = xpix; yaa = -ypix;   }
	if (aa == 4){	xaa = xpix; yaa = ypix;   }



	//y += tvHeight - height;
	width += x;
	height += y;
	
	texWidth += texX;
	texHeight += texY;

	if (tvAspectW != 1.0){
		aspcenter = tvWidth * ((1.0f - tvAspectW) / 2);
		// leilei - also do a quad that is 100% black, hiding our actual rendered viewport

	//	qglViewport	(0, 0, 	tvWidth, tvHeight );
	//	qglScissor	(0, 0,	tvWidth, tvHeight );
		qglBegin( GL_QUADS );	
	if (r_tvFilter->integer)	// bilinear filter
	{
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	}
	else
	{
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	}
		qglColor4f( 0.0, 0.0, 0.0, 1 );		
		qglVertex2f(0,0	);
		qglVertex2f(0,height);	
		qglVertex2f(width,height);	
		qglVertex2f(width,0);	
		qglEnd ();
		qglColor4f( 1.0, 1.0, 1.0, 1 );	
	}


	//aspcenter = 0;
	//aspoff = tvWidth * tvAspectW;

	if (!aa){
	qglViewport(aspcenter, 0, 	(tvWidth * tvAspectW), tvHeight );
	qglScissor(aspcenter, 0,	(tvWidth * tvAspectW), tvHeight );
	}
	qglBegin( GL_QUADS );	
	//GL_State( GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE );
	if (aa)
	qglColor4f( 0.25, 0.25, 0.25, 1 );						
	qglTexCoord2f(	texX + xaa,						texHeight + yaa);	
	qglVertex2f(	x,							y	);
	qglTexCoord2f(	texX + xaa,						texY + yaa	);				
	qglVertex2f(	x,							height	);	
	qglTexCoord2f(	texWidth + xaa,					texY	+ yaa );				
	qglVertex2f(	width,						height	);	
	qglTexCoord2f(	texWidth + xaa,					texHeight + yaa	);	
	qglVertex2f(	width,						y	);				
	qglEnd ();
}


static void R_Bloom_RestoreScreen_Postprocessed( void ) {
	glslProgram_t	*program;

	if (!vertexShaders) return;
	// leilei, please check (eventually instead of return here do a goto jump to
	// non-glsl part below

	R_GLSL_UseProgram(tr.postprocessingProgram);
	program=tr.programs[tr.postprocessingProgram];
	

	if (leifxmode)
	{
	if (leifxmode == 1){ if (vertexShaders) R_GLSL_UseProgram(tr.leiFXDitherProgram); program=tr.programs[tr.leiFXDitherProgram];}
	if (leifxmode == 2){ if (vertexShaders) R_GLSL_UseProgram(tr.leiFXGammaProgram); program=tr.programs[tr.leiFXGammaProgram];}
	if (leifxmode == 3){ if (vertexShaders) R_GLSL_UseProgram(tr.leiFXFilterProgram); program=tr.programs[tr.leiFXFilterProgram];}
	if (leifxmode == 888){ if (vertexShaders) R_GLSL_UseProgram(tr.animeProgram); program=tr.programs[tr.animeProgram];}
	if (leifxmode == 999){ if (vertexShaders) R_GLSL_UseProgram(tr.animeFilmProgram); program=tr.programs[tr.animeFilmProgram];}
	if (leifxmode == 777){ if (vertexShaders) R_GLSL_UseProgram(tr.motionBlurProgram); program=tr.programs[tr.motionBlurProgram];}
	if (leifxmode == 778){ if (vertexShaders) R_GLSL_UseProgram(tr.motionBlurProgram); program=tr.programs[tr.motionBlurProgram];}
	if (leifxmode == 779){ if (vertexShaders) R_GLSL_UseProgram(tr.motionBlurPostProgram); program=tr.programs[tr.motionBlurPostProgram];}
	if (leifxmode == 632){ if (vertexShaders) R_GLSL_UseProgram(tr.NTSCEncodeProgram); program=tr.programs[tr.NTSCEncodeProgram];}
	if (leifxmode == 633){ if (vertexShaders) R_GLSL_UseProgram(tr.NTSCDecodeProgram); program=tr.programs[tr.NTSCDecodeProgram];}
	if (leifxmode == 634){ if (vertexShaders) R_GLSL_UseProgram(tr.NTSCBleedProgram); program=tr.programs[tr.NTSCBleedProgram];}
	if (leifxmode == 666){ if (vertexShaders) R_GLSL_UseProgram(tr.BrightnessProgram); program=tr.programs[tr.BrightnessProgram];}
	if (leifxmode == 1236){ if (vertexShaders) R_GLSL_UseProgram(tr.CRTProgram); program=tr.programs[tr.CRTProgram];}
	if (leifxmode == 1997){ if (vertexShaders) R_GLSL_UseProgram(tr.paletteProgram); program=tr.programs[tr.paletteProgram];}
	}

	// Feed GLSL postprocess program
	if (program->u_ScreenSizeX > -1) R_GLSL_SetUniform_u_ScreenSizeX(program, glConfig.vidWidth);
	if (program->u_ScreenSizeY > -1) R_GLSL_SetUniform_u_ScreenSizeY(program, glConfig.vidHeight);
	if (program->u_FBTexSizeX > -1) R_GLSL_SetUniform_u_FBTexSizeX(program, (float)glConfig.vidWidth/(float)postproc.screen.width);
	if (program->u_FBTexSizeY > -1) R_GLSL_SetUniform_u_FBTexSizeY(program, (float)glConfig.vidHeight/(float)postproc.screen.height);
     // in UV scale (0.0 .. 1.0)
	if (program->u_ScreenToNextPixelX > -1) R_GLSL_SetUniform_u_ScreenToNextPixelX(program, ((float)glConfig.vidWidth/(float)postproc.screen.width)/(float)glConfig.vidWidth);
    // in UV space
	if (program->u_ScreenToNextPixelY > -1) R_GLSL_SetUniform_u_ScreenToNextPixelY(program, ((float)glConfig.vidHeight/(float)postproc.screen.height)/(float)glConfig.vidHeight);
    // in UV space	
	// leilei - for TV shaders
	if (program->u_ActualScreenSizeX > -1) R_GLSL_SetUniform_u_ActualScreenSizeX(program, tvWidth);
	if (program->u_ActualScreenSizeY > -1) R_GLSL_SetUniform_u_ActualScreenSizeY(program, tvHeight);

	//if (program->u_Time > -1) R_GLSL_SetUniform_Time(program, backEnd.refdef.time);
	if (program->u_Time > -1) R_GLSL_SetUniform_Time(program, ScreenFrameCount);

	// Brightness stuff
	if (program->u_CC_Brightness > -1) R_GLSL_SetUniform_u_CC_Brightness(program, 1.0);
	if (program->u_CC_Gamma > -1) R_GLSL_SetUniform_u_CC_Gamma(program, r_gamma->value);
	if (program->u_CC_Overbright > -1) R_GLSL_SetUniform_u_CC_Overbright(program, r_overBrightBits->value);
	if (program->u_CC_Saturation > -1) R_GLSL_SetUniform_u_CC_Saturation(program, 1.0);
	if (program->u_CC_Contrast > -1) R_GLSL_SetUniform_u_CC_Contrast(program, 1.0);

	// 
	if (leifxmode == 3){ R_GLSL_SetUniform_u_CC_Brightness(program, leifxpass); }

	if (program->u_zFar > -1) R_GLSL_SetUniform_u_zFar(program, backEnd.viewParms.zFar);
	if (program->u_zNear > -1) R_GLSL_SetUniform_u_zNear(program, r_znear->value);

	/* model view projection matrix */
	if (program->u_ModelViewProjectionMatrix > -1)
		R_GLSL_SetUniform_ModelViewProjectionMatrix(program, MVPMatrixSunPos);

	// working
	oa_SunPos[0]*=((float)glConfig.vidWidth/(float)postproc.screen.width);
	oa_SunPos[1]*=((float)glConfig.vidHeight/(float)postproc.screen.height);
	if (program->u_SunPos > -1) R_GLSL_SetUniform_u_SunPos(program, oa_SunPos);

	// does not work yet
	glsl_rotationBlurPosition_old[0]*=((float)glConfig.vidWidth/(float)postproc.screen.width);
	glsl_rotationBlurPosition_old[1]*=((float)glConfig.vidHeight/(float)postproc.screen.height);		
	if (program->u_rotationBlurPosition > -1) R_GLSL_SetUniform_u_rotationBlurPosition(program, glsl_rotationBlurPosition);
	if (program->u_rotationBlurPositionOld > -1) R_GLSL_SetUniform_u_rotationBlurPositionOld(program, glsl_rotationBlurPosition_old);


	GL_SelectTexture(0);
	GL_State( GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO );
	GL_Bind( postproc.screen.texture );
	GL_SelectTexture(6);
	GL_State( GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO );
    GL_Bind(tonemap);
	GL_SelectTexture(7);
	GL_State( GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO );
	GL_Bind( postproc.depth.texture );	

	// motion blur crap
	if( r_motionblur->integer > 2){
	if (program->u_mpasses > -1) R_GLSL_SetUniform_u_mpasses(program, mpasses);
	GL_SelectTexture(2);
	GL_State( GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO );
	GL_Bind( postproc.motion1.texture );	
	GL_SelectTexture(3);
	GL_State( GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO );
	GL_Bind( postproc.motion2.texture );	
	GL_SelectTexture(4);
	GL_State( GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO );
	GL_Bind( postproc.motion3.texture );	
	GL_SelectTexture(5);
	GL_State( GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO );
	GL_Bind( postproc.motion4.texture );	
	GL_SelectTexture(6);
	GL_State( GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO );
	GL_Bind( postproc.motion5.texture );	
	GL_SelectTexture(11);
	GL_State( GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO );
	GL_Bind( postproc.mpass1.texture );	
	GL_SelectTexture(12);
	GL_State( GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO );
	GL_Bind( postproc.mpass1.texture );	
	GL_SelectTexture(13);
	GL_State( GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO );
	GL_Bind( postproc.mpass1.texture );	
	GL_SelectTexture(14);
	GL_State( GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO );
	GL_Bind( postproc.mpass1.texture );	
	}
	else
	{
	GL_SelectTexture(6);
	GL_State( GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO );
    GL_Bind(tonemap);
	}

	qglColor4f( 1, 1, 1, 1 );

//	if (leifxmode == 778)
//		return;
	if (leifxmode == 1234){
			{
			R_Bloom_QuadTV( glConfig.vidWidth, glConfig.vidHeight, 0, 0, postproc.screen.readW,postproc.screen.readH, 0 );
			}
		}
	else if (leifxmode == 1236){
			{
			R_Bloom_QuadTV( glConfig.vidWidth, glConfig.vidHeight, 0, 0, postproc.screen.readW,postproc.screen.readH, 0 );
			}
		}
	else if (leifxmode == 1233){
			

			R_Bloom_QuadTV( glConfig.vidWidth, glConfig.vidHeight, 0, 0, postproc.screen.readW,postproc.screen.readH, 1 );
			GL_SelectTexture(0);
			GL_State( GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE );
			GL_Bind( postproc.screen.texture );
			R_Bloom_QuadTV( glConfig.vidWidth, glConfig.vidHeight, 0, 0, postproc.screen.readW,postproc.screen.readH, 2 );
			R_Bloom_QuadTV( glConfig.vidWidth, glConfig.vidHeight, 0, 0, postproc.screen.readW,postproc.screen.readH, 3 );
			R_Bloom_QuadTV( glConfig.vidWidth, glConfig.vidHeight, 0, 0, postproc.screen.readW,postproc.screen.readH, 4 );

		}
	else
		{
	R_Bloom_Quad( glConfig.vidWidth, glConfig.vidHeight, 0, 0,
			postproc.screen.readW,postproc.screen.readH );
		}
	if (vertexShaders) R_GLSL_UseProgram(0);
	GL_SelectTexture(0);

}

/*
=================
R_Bloom_DownsampleView
Scale the copied screen back to the sample size used for subsequent passes
=================
*/
/*static void R_Bloom_DownsampleView( void )
{
	// TODO, Provide option to control the color strength here /
//	qglColor4f( r_bloom_darken->value, r_bloom_darken->value, r_bloom_darken->value, 1.0f );
	qglColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
	GL_Bind( bloom.screen.texture );
	GL_State( GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO );
	//Downscale it
	R_Bloom_Quad( bloom.work.width, bloom.work.height, 0, 0, bloom.screen.readW, bloom.screen.readH );
#if 1
	GL_Bind( bloom.effect.texture );
	qglCopyTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, 0, 0, bloom.work.width, bloom.work.height );
	// darkening passes
	if( r_bloom_darken->integer ) {
		int i;
		GL_State( GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO );

		for( i = 0; i < r_bloom_darken->integer; i++ ) {
			R_Bloom_Quad( bloom.work.width, bloom.work.height, 
				0, 0, 
				bloom.effect.readW, bloom.effect.readH );
		}
		qglCopyTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, 0, 0, bloom.work.width, bloom.work.height );
	}
#endif
	// Copy the result to the effect texture /
	GL_Bind( bloom.effect.texture );
	qglCopyTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, 0, 0, bloom.work.width, bloom.work.height );
}

static void R_Bloom_CreateEffect( void ) {
	int dir, x;
	int range;

	//First step will zero dst, rest will one add
	GL_State( GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO );
//	GL_Bind( bloom.screen.texture );
	GL_Bind( bloom.effect.texture );
	range = 4;
	for (dir = 0;dir < 2;dir++)
	{
		// blend on at multiple vertical offsets to achieve a vertical blur
		// TODO: do offset blends using GLSL
		for (x = -range;x <= range;x++)
		{
			float xoffset, yoffset, r;
			if (!dir){
				xoffset = 0;
				yoffset = x*1.5;
			} else {
				xoffset = x*1.5;
				yoffset = 0;
			}
			xoffset /= bloom.work.width;
			yoffset /= bloom.work.height;
			// this r value looks like a 'dot' particle, fading sharply to
			// black at the edges
			// (probably not realistic but looks good enough)
			//r = ((range*range+1)/((float)(x*x+1)))/(range*2+1);
			//r = (dir ? 1.0f : brighten)/(range*2+1);
			r = 2.0f /(range*2+1)*(1 - x*x/(float)(range*range));
//			r *= r_bloom_darken->value;
			qglColor4f(r, r, r, 1);
			R_Bloom_Quad( bloom.work.width, bloom.work.height, 
				xoffset, yoffset, 
				bloom.effect.readW, bloom.effect.readH );
//				bloom.screen.readW, bloom.screen.readH );
			GL_State( GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE );
		}
	}
	GL_Bind( bloom.effect.texture );
	qglCopyTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, 0, 0, bloom.work.width, bloom.work.height );
}*/

/*
=================
R_BloomScreen
=================
*/
void R_BloomScreen( void )
{
	if( !r_bloom->integer )
		return;
	if(r_bloom_sky_only->integer)
		return;
	if ( backEnd.doneBloom )
		return;
	if ( !backEnd.doneSurfaces )
		return;
	backEnd.doneBloom = qtrue;
	if( !bloom.started ) {
		R_Bloom_InitTextures();
		if( !bloom.started )
			return;
	}

	if ( !backEnd.projection2D )
		RB_SetGL2D();
#if 0
	// set up full screen workspace
	GL_TexEnv( GL_MODULATE );
	qglScissor( 0, 0, glConfig.vidWidth, glConfig.vidHeight );
	qglViewport( 0, 0, glConfig.vidWidth, glConfig.vidHeight );
	qglMatrixMode( GL_PROJECTION );
    qglLoadIdentity ();
	qglOrtho( 0, glConfig.vidWidth, glConfig.vidHeight, 0, 0, 1 );
	qglMatrixMode( GL_MODELVIEW );
    qglLoadIdentity ();

	GL_Cull( CT_TWO_SIDED );
#endif

	qglColor4f( 1, 1, 1, 1 );

	//Backup the old screen in a texture
	R_Bloom_BackupScreen();
	// create the bloom texture using one of a few methods
	if(r_bloom_cascade->integer)
		R_Bloom_Cascaded();
	else
		R_Bloom_WarsowEffect ();
//	R_Bloom_CreateEffect();
	// restore the screen-backup to the screen
	R_Bloom_RestoreScreen();
	// Do the final pass using the bloom texture for the final effect
	R_Bloom_DrawEffect ();

// 	LEILEI - Lens Bloom Hack
//		The concept of this is to inverse the coordinates on both X and Y,
//		then scale outward using the offset value applied, as well as the modulated color
//		applied to give a rainbow streak effect.  Most effective with high bloom darkness
//		values.
	if(r_bloom_reflection->integer)
	R_Bloom_LensEffect ();
}



void R_BloomInit( void ) {
	memset( &bloom, 0, sizeof( bloom ));

	r_bloom = ri.Cvar_Get( "r_bloom", "0", CVAR_ARCHIVE );
	r_bloom_alpha = ri.Cvar_Get( "r_bloom_alpha", "0.3", CVAR_ARCHIVE );
	r_bloom_diamond_size = ri.Cvar_Get( "r_bloom_diamond_size", "8", CVAR_ARCHIVE );
	r_bloom_intensity = ri.Cvar_Get( "r_bloom_intensity", "1.3", CVAR_ARCHIVE );
	r_bloom_darken = ri.Cvar_Get( "r_bloom_darken", "4", CVAR_ARCHIVE );
	r_bloom_sample_size = ri.Cvar_Get( "r_bloom_sample_size", "256", CVAR_ARCHIVE|CVAR_LATCH ); // Tcpp: I prefer 256 to 128
	r_bloom_fast_sample = ri.Cvar_Get( "r_bloom_fast_sample", "0", CVAR_ARCHIVE|CVAR_LATCH );
	r_bloom_cascade = ri.Cvar_Get( "r_bloom_cascade", "0", CVAR_ARCHIVE );
	r_bloom_cascade_blur = ri.Cvar_Get( "r_bloom_cascade_blur", ".4", CVAR_ARCHIVE );
	r_bloom_cascade_intensity= ri.Cvar_Get( "r_bloom_cascade_intensity", "20", CVAR_ARCHIVE );
	r_bloom_cascade_alpha = ri.Cvar_Get( "r_bloom_cascade_alpha", "0.15", CVAR_ARCHIVE );
	r_bloom_cascade_dry = ri.Cvar_Get( "r_bloom_cascade_dry", "0.8", CVAR_ARCHIVE );
	r_bloom_dry = ri.Cvar_Get( "r_bloom_dry", "1", CVAR_ARCHIVE );
	r_bloom_reflection = ri.Cvar_Get( "r_bloom_reflection", "0", CVAR_ARCHIVE );
	r_bloom_sky_only = ri.Cvar_Get( "r_bloom_sky_only", "0", CVAR_ARCHIVE );
}





// =================================================================
// GLSL POST PROCESSING
// =================================================================





/*
=================
R_Postprocess_InitTextures
=================
*/
static void R_Postprocess_InitTextures( void )
{
	byte	*data;
	//int vidinted = glConfig.vidHeight * 0.55f;
	int intdiv = 1;

	force32upload = 1;
	// find closer power of 2 to screen size 
	for (postproc.screen.width = 1;postproc.screen.width< glConfig.vidWidth;postproc.screen.width *= 2);

//	if (r_tvMode->integer > 1)	// interlaced

//	for (postproc.screen.height = 1;postproc.screen.height < vidinted;postproc.screen.height *= 2);
//else
	for (postproc.screen.height = 1;postproc.screen.height < glConfig.vidHeight;postproc.screen.height *= 2);

//	if (r_tvMode->integer > 1)	
//		intdiv = 2;

	postproc.screen.readW = glConfig.vidWidth / (float)postproc.screen.width;
	postproc.screen.readH = glConfig.vidHeight / (float)postproc.screen.height;

	postfx_width = glConfig.vidWidth ; 
	postfx_height = glConfig.vidHeight;

	



	// find closer power of 2 to effect size 
	postproc.work.width = r_bloom_sample_size->integer;  /// ue4 type lens flare texture here (marker for future development)
	
	postproc.work.height = postproc.work.width * ( glConfig.vidWidth / glConfig.vidHeight );

	for (postproc.effect.width = 1;postproc.effect.width < postproc.work.width;postproc.effect.width *= 2);
	for (postproc.effect.height = 1;postproc.effect.height < postproc.work.height;postproc.effect.height *= 2);


	postproc.effect.readW = postproc.work.width / (float)postproc.effect.width;
	postproc.effect.readH = postproc.work.height / (float)postproc.effect.height;

	
	
//	postproc.screen.readH /= intdiv; // interlacey


	// disable blooms if we can't handle a texture of that size
	if( 	postproc.screen.width > glConfig.maxTextureSize ||
			postproc.screen.height > glConfig.maxTextureSize 
	) {
		ri.Cvar_Set( "r_postprocess", "none" );
		ri.Cvar_Set( "r_postprocess_multipart", "none" );
		Com_Printf( S_COLOR_YELLOW"WARNING: 'R_InitPostprocessTextures' too high resolution for postprocessing, effect disabled\n" );
		postprocess=0;
		return;
	}

	force32upload = 1;

	data = ri.Hunk_AllocateTempMemory( postproc.screen.width * postproc.screen.height * 4 );
	Com_Memset( data, 0, postproc.screen.width * postproc.screen.height * 4 );
	postproc.screen.texture = R_CreateImage( "***postproc screen texture***", data, postproc.screen.width, postproc.screen.height, qfalse, qfalse, GL_CLAMP_TO_EDGE  );
	ri.Hunk_FreeTempMemory( data );
	
	// leilei - tv output texture

	if (r_tvMode->integer > -1){
		// find closer power of 2 to screen size 
		for (postproc.tv.width = 1;postproc.tv.width< tvWidth;postproc.tv.width *= 2);
		for (postproc.tv.height = 1;postproc.tv.height < tvHeight;postproc.tv.height *= 2);
	
		//postproc.tv.height /= intdiv; // interlacey
		

		postproc.tv.readW = tvWidth / (float)postproc.tv.width;
		postproc.tv.readH = tvHeight / (float)postproc.tv.height;
	
		// find closer power of 2 to effect size 
		postproc.tvwork.width = r_bloom_sample_size->integer;
		postproc.tvwork.height = postproc.tvwork.width * ( tvWidth / tvHeight );

	//	postproc.tvwork.height /= intdiv; // interlacey
	
		for (postproc.tveffect.width = 1;postproc.tveffect.width < postproc.tvwork.width;postproc.tveffect.width *= 2);
if (intdiv > 1)
		for (postproc.tveffect.height = 1;(postproc.tveffect.height/2) < postproc.tvwork.height;postproc.tveffect.height *= 2);
		else
		for (postproc.tveffect.height = 1;postproc.tveffect.height < postproc.tvwork.height;postproc.tveffect.height *= 2);

		postproc.tveffect.readW = postproc.tvwork.width / (float)postproc.tveffect.width;
		postproc.tveffect.readH = postproc.tvwork.height / (float)postproc.tveffect.height;
	

	
		data = ri.Hunk_AllocateTempMemory( tvWidth * tvHeight * 4 );
		Com_Memset( data, 0, tvWidth * tvHeight * 4 );
		postproc.tv.texture = R_CreateImage( "***tv output screen texture***", data, tvWidth, tvHeight, qfalse, qfalse, GL_CLAMP_TO_EDGE  );
		ri.Hunk_FreeTempMemory( data );
	}

	// leilei - motion blur textures!

	if (r_motionblur->integer > 2){
	data = ri.Hunk_AllocateTempMemory( postproc.screen.width * postproc.screen.height * 4 );
	Com_Memset( data, 0, postproc.screen.width * postproc.screen.height * 4 );
	postproc.motion1.texture = R_CreateImage( "***motionblur1 texture***", data, postproc.screen.width, postproc.screen.height, qfalse, qfalse, GL_CLAMP_TO_EDGE  );
	postproc.motion2.texture = R_CreateImage( "***motionblur2 texture***", data, postproc.screen.width, postproc.screen.height, qfalse, qfalse, GL_CLAMP_TO_EDGE  );
	postproc.motion3.texture = R_CreateImage( "***motionblur3 texture***", data, postproc.screen.width, postproc.screen.height, qfalse, qfalse, GL_CLAMP_TO_EDGE  );
	postproc.motion4.texture = R_CreateImage( "***motionblur4 texture***", data, postproc.screen.width, postproc.screen.height, qfalse, qfalse, GL_CLAMP_TO_EDGE  );
	postproc.motion5.texture = R_CreateImage( "***motionblur5 texture***", data, postproc.screen.width, postproc.screen.height, qfalse, qfalse, GL_CLAMP_TO_EDGE  );
	postproc.mpass1.texture = R_CreateImage( "***motionaccum1 texture***", data, postproc.screen.width, postproc.screen.height, qfalse, qfalse, GL_CLAMP_TO_EDGE  );
	postproc.mpass2.texture = R_CreateImage( "***motionaccum1 texture***", data, postproc.screen.width, postproc.screen.height, qfalse, qfalse, GL_CLAMP_TO_EDGE  );
	postproc.mpass3.texture = R_CreateImage( "***motionaccum1 texture***", data, postproc.screen.width, postproc.screen.height, qfalse, qfalse, GL_CLAMP_TO_EDGE  );
	postproc.mpass4.texture = R_CreateImage( "***motionaccum1 texture***", data, postproc.screen.width, postproc.screen.height, qfalse, qfalse, GL_CLAMP_TO_EDGE  );
	ri.Hunk_FreeTempMemory( data );
	}

	// GLSL Depth Buffer

	data = ri.Hunk_AllocateTempMemory( postproc.screen.width * postproc.screen.height *4);
	Com_Memset( data, 0, postproc.screen.width * postproc.screen.height * 4 );
	depthimage=1;
	postproc.depth.texture = R_CreateImage( "***depthbuffer texture***",  data, postproc.screen.width, postproc.screen.height, qfalse, qfalse, GL_CLAMP_TO_EDGE   );
	depthimage=0;
	ri.Hunk_FreeTempMemory( data );

	postproc.started = qtrue;
	force32upload = 0;
	
}

/*
=================
R_InitPostprocessTextures
=================
*/
void R_InitPostprocessTextures( void )
{
	if( !postprocess )
		return;
	memset( &postproc, 0, sizeof( postproc ));
	R_Postprocess_InitTextures ();
}


/*
=================
R_Postprocess_BackupScreen
Backup the full original screen to a texture for downscaling and later restoration
=================
*/
static void R_Postprocess_BackupScreen( void ) {

	GL_Bind( postproc.screen.texture );
	qglCopyTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, 0, 0, glConfig.vidWidth, glConfig.vidHeight );

	GL_Bind( postproc.depth.texture );
	qglCopyTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, 0, 0, glConfig.vidWidth, glConfig.vidHeight );

		
}


/*
=================
R_PostprocessScreen
=================
*/
void R_PostprocessScreen( void )
{
	if( !postprocess )
		return;
	if ( backEnd.donepostproc )
		return;
	if ( !backEnd.doneSurfaces )
		return;
	if ( !vertexShaders )
		return;		// leilei - cards without support for this should not ever activate this
	backEnd.donepostproc = qtrue;
	if( !postproc.started ) {
		R_Postprocess_InitTextures();
		if( !postproc.started )
			return;
	}

	if ( !backEnd.projection2D )
		RB_SetGL2D();
#if 0
	// set up full screen workspace
	GL_TexEnv( GL_MODULATE );
	qglScissor( 0, 0, glConfig.vidWidth, glConfig.vidHeight );
	qglViewport( 0, 0, glConfig.vidWidth, glConfig.vidHeight );
	qglMatrixMode( GL_PROJECTION );
    qglLoadIdentity ();
	qglOrtho( 0, glConfig.vidWidth, glConfig.vidHeight, 0, 0, 1 );
	qglMatrixMode( GL_MODELVIEW );
    qglLoadIdentity ();

	GL_Cull( CT_TWO_SIDED );
#endif

	qglColor4f( 1, 1, 1, 1 );

	//Backup the old screen in a texture
	R_Postprocess_BackupScreen();
	//Redraw texture using GLSL program
	R_Bloom_RestoreScreen_Postprocessed();
}



void R_PostprocessingInit(void) {
	memset( &postproc, 0, sizeof( postproc ));
}


// =================================================================
// LEIFX FILTER EFFECTS
// =================================================================



static void ID_INLINE R_LeiFX_Pointless_Quad( int width, int height, float texX, float texY, float texWidth, float texHeight ) {
	int x = 0;
	int y = 0;
	x = 0;
	y += glConfig.vidHeight - height;
	width += x;
	height += y;
	
	texWidth += texX;
	texHeight += texY;

	qglBegin( GL_QUADS );							

	qglTexCoord2f(	texX,						texHeight	);	
	qglVertex2f(	x,							y	);

	qglTexCoord2f(	texX,						texY	);				
	qglVertex2f(	x,							height	);	

	qglTexCoord2f(	texWidth,					texY	);				
	qglVertex2f(	width,						height	);	

	qglTexCoord2f(	texWidth,					texHeight	);	
	qglVertex2f(	width,						y	);				
	qglEnd ();
}

void R_LeiFX_Stupid_Hack (void)
{
	GL_State( GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_SRC_COLOR);
	GL_Bind( tr.whiteImage );
	GL_Cull( CT_TWO_SIDED );

	qglColor4f( 1, 1, 1, 1 );
	R_LeiFX_Pointless_Quad( 0, 0, 0, 0, 1, 1 );

}
void R_LeiFXPostprocessDitherScreen( void )
{
	if( !r_leifx->integer)
		return;
	if ( backEnd.doneleifx)
		return;
	if ( !vertexShaders )
		return;		// leilei - cards without support for this should not ever activate this
//	if ( !backEnd.doneSurfaces )
//		return;
//	backEnd.doneleifx = qtrue;
	if( !postproc.started ) {
		force32upload = 1;
		R_Postprocess_InitTextures();
		if( !postproc.started )
			return;
	}

	if ( !backEnd.projection2D )
		RB_SetGL2D();

//	postprocess = 1;

	leifxmode = 1;
	
	// The stupidest hack in america
	R_LeiFX_Stupid_Hack();


	if (r_leifx->integer > 1){
		leifxmode = 1;			// reduct and dither - 1 pass
		R_Postprocess_BackupScreen();
		R_Bloom_RestoreScreen_Postprocessed();
		}

		force32upload = 0;	
	
}


void R_LeiFXPostprocessFilterScreen( void )
{
	if( !r_leifx->integer)
		return;
	if ( backEnd.doneleifx)
		return;
	if ( !vertexShaders )
		return;		// leilei - cards without support for this should not ever activate this
//	if ( !backEnd.doneSurfaces )
//		return;
	if( !postproc.started ) {
		force32upload = 1;
		R_Postprocess_InitTextures();
		if( !postproc.started )
			return;
	}

	if ( !backEnd.projection2D )
		RB_SetGL2D();
		force32upload = 1;

//	postprocess = 1;

/*	if (r_leifx->integer == 3){
		leifxmode = 2;			// gamma - 1 pass
	// The stupidest hack in america
	R_LeiFX_Stupid_Hack();
		R_Postprocess_BackupScreen();
		R_Bloom_RestoreScreen_Postprocessed();
		}
*/						// Gamma disabled because r_alternateBrightness 2 makes it redundant now.
	if (r_leifx->integer > 3){
		leifxmode = 3;			// filter - 4 pass
	// The stupidest hack in america
	R_LeiFX_Stupid_Hack();
		leifxpass = 0;
		R_Postprocess_BackupScreen();
		R_Bloom_RestoreScreen_Postprocessed();
		leifxpass = 1;
		R_Postprocess_BackupScreen();
		R_Bloom_RestoreScreen_Postprocessed();
		leifxpass = 2;
		R_Postprocess_BackupScreen();
		R_Bloom_RestoreScreen_Postprocessed();
		leifxpass = 3;
		R_Postprocess_BackupScreen();
		R_Bloom_RestoreScreen_Postprocessed();
		}
	backEnd.doneleifx = qtrue;

			force32upload = 0;
	

}



// =================================================================
// NTSC EFFECTS
// =================================================================



void R_NTSCScreen( void )
{
	if( !r_ntsc->integer)
		return;
	if ( backEnd.donentsc)
		return;
	if ( !vertexShaders )
		return;		// leilei - cards without support for this should not ever activate this
//	if ( !backEnd.doneSurfaces )
//		return;
	if( !postproc.started ) {
		force32upload = 1;
		R_Postprocess_InitTextures();
		if( !postproc.started )
			return;
	}

	if ( !backEnd.projection2D )
		RB_SetGL2D();
		force32upload = 1;

	int ntsc_bleed	= 0;
	int ntsc_encode = 0;
	int ntsc_decode = 0;

	// TODO: Switch it up
	if (r_ntsc->integer == 1) ntsc_bleed = 1;
	else if (r_ntsc->integer == 2){ ntsc_bleed = 1; ntsc_encode = 1; ntsc_decode = 1; }
	else if (r_ntsc->integer == 3){ ntsc_bleed = 0; ntsc_encode = 1; ntsc_decode = 1; }
	else if (r_ntsc->integer == 4){ ntsc_bleed = 1; ntsc_encode = 1; ntsc_decode = 1; }
	else if (r_ntsc->integer > 6){ ntsc_bleed = 666; ntsc_encode = 0; ntsc_decode = 0; }
	else { ntsc_bleed = 0; ntsc_encode = 1; ntsc_decode = 0; }
	


//	qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
//	qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );

	R_LeiFX_Stupid_Hack();
		if (ntsc_encode){
		leifxmode = 632;		// Encode to composite
		R_Postprocess_BackupScreen();
		R_Bloom_RestoreScreen_Postprocessed();
		}


		if (ntsc_decode){
		leifxmode = 633;		// Decode to RGB
		R_Postprocess_BackupScreen();
		R_Bloom_RestoreScreen_Postprocessed();
		}

		if (ntsc_bleed){
		leifxmode = 634;		// Encode to YUV and decode to RGB
		R_Postprocess_BackupScreen();
		R_Bloom_RestoreScreen_Postprocessed();
		}



		if (ntsc_bleed == 666){
		leifxmode = 634;		// Encode to YUV and decode to RGB EXCESSIVELY
		int passasses = r_ntsc->integer;
		int j;
		for (j=0;j<passasses;j++)
			{
				R_Postprocess_BackupScreen();
				R_Bloom_RestoreScreen_Postprocessed();
			}
		

		}

	backEnd.donentsc = qtrue;

			force32upload = 0;
	

}

// =================================================================
// ALTERNATE SCREEN BRIGHTNESS
// =================================================================

extern cvar_t *r_alternateBrightness;
// shamelessly ripped off from tr_bloom.c
static void ID_INLINE R_Brighter_Quad( int width, int height, float texX, float texY, float texWidth, float texHeight ) {
	int x = 0;
	int y = 0;
	x = 0;
	y += glConfig.vidHeight - height;
	width += x;
	height += y;
	
	texWidth += texX;
	texHeight += texY;

	qglBegin( GL_QUADS );							
	qglTexCoord2f(	texX,						texHeight	);	
	qglVertex2f(	x,							y	);

	qglTexCoord2f(	texX,						texY	);				
	qglVertex2f(	x,							height	);	

	qglTexCoord2f(	texWidth,					texY	);				
	qglVertex2f(	width,						height	);	

	qglTexCoord2f(	texWidth,					texHeight	);	
	qglVertex2f(	width,						y	);				
	qglEnd ();
}



void R_BrightItUp (int dst, int src, float intensity)
{
	GL_State( GLS_DEPTHTEST_DISABLE | dst | src);
	GL_Bind( tr.whiteImage );
	GL_Cull( CT_TWO_SIDED );
	
	float alpha=intensity;	// why
	qglColor4f( alpha,alpha,alpha, 1.0f );
	if (!fakeit)
	R_Brighter_Quad( glConfig.vidWidth, glConfig.vidHeight, 0, 0, 1, 1 );

}


void R_BrightScreen( void )
{
	int mode;	// 0 = none; 1 = blend; 2 = shader
	if( !r_alternateBrightness->integer)
		return;
	if ( backEnd.doneAltBrightness)
		return;

	if (r_alternateBrightness->integer == 1)
		mode = 1;	// force use blend
	if (r_alternateBrightness->integer == 2)
	{
		// Automatically determine from capabilities
		if ( vertexShaders )
		mode = 2;
		else
		mode = 1;
	}

	// the modern pixel shader way
	if (mode == 2)
	{
		if ( !vertexShaders )
			return;		// leilei - cards without support for this should not ever activate this
		if( !postproc.started ) {
			force32upload = 1;
			R_Postprocess_InitTextures();
			if( !postproc.started )
				return;
		}
	
		if ( !backEnd.projection2D )
		RB_SetGL2D();
	
		force32upload = 1;
	
		leifxmode = 666;			// anime effect - outlines, desat, bloom and other crap to go with it
		R_LeiFX_Stupid_Hack();
		R_Postprocess_BackupScreen();
		R_Bloom_RestoreScreen_Postprocessed();
		backEnd.doneAltBrightness = qtrue;
	
		force32upload = 0;
	}	

	// the fixed function quad blending way
	else if (mode == 1)
	{
		int eh, ah;
		if ((r_overBrightBits->integer))
			{
	
			ah = r_overBrightBits->integer;
			if (ah < 1) ah = 1; if (ah > 2) ah = 2; // clamp so it never looks stupid


			// Blend method
			// do a loop for every overbright bit enabled

			for (eh=0; eh<ah; eh++)
			R_BrightItUp(GLS_SRCBLEND_DST_COLOR, GLS_DSTBLEND_ONE, 1.0f); 

			backEnd.doneAltBrightness = qtrue;
			}

	}
	
}


void R_AltBrightnessInit( void ) {

	r_alternateBrightness = ri.Cvar_Get( "r_alternateBrightness", "0", CVAR_ARCHIVE | CVAR_LATCH);
	r_film = ri.Cvar_Get( "r_film", "0", CVAR_ARCHIVE );
}



// =================================================================
// FILM POSTPROCESS
// =================================================================

void R_FilmScreen( void )
{
	vec3_t tone, toneinv, tonecont;

	if( !r_film->integer )
		return;
	if ( backEnd.doneFilm )
		return;
	if ( !backEnd.projection2D )
		RB_SetGL2D();
	backEnd.doneFilm = qtrue;

		// set up our colors, this is our default
		tone[0] = 0.8f;
		tone[1] = 0.9f;
		tone[2] = 1.0f;
	//VectorCopy( backEnd.currentEntity->ambientLight, tone );
		if (backEnd.currentEntity){
			if (backEnd.currentEntity->ambientLight[0] > 0.001f && backEnd.currentEntity->ambientLight[1] > 0.001f && backEnd.currentEntity->ambientLight[2] > 0.001f){
				tone[0] = backEnd.currentEntity->ambientLight[0];
				tone[1] = backEnd.currentEntity->ambientLight[1];
				tone[2] = backEnd.currentEntity->ambientLight[2];
			}
		}

	//	VectorNormalize(tone);

		tone[0] *= 0.3 + 0.7;
		tone[1] *= 0.3 + 0.7;
		tone[2] *= 0.3 + 0.7;

	//	tone[0] = 1.6f;
	//	tone[1] = 1.2f;
	//	tone[2] = 0.7f;


		// TODO: Get overexposure to flares raising this faking "HDR"
		tonecont[0] = 0.0f;
		tonecont[1] = 0.0f;
		tonecont[2] = 0.0f;
	

		// inverted

		toneinv[0] = tone[0] * -1 + 1 + tonecont[0];
		toneinv[1] = tone[1] * -1 + 1 + tonecont[1];
		toneinv[2] = tone[2] * -1 + 1 + tonecont[2];

		// darken vignette.
		GL_State( GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO);GL_Bind( tr.dlightImage );	GL_Cull( CT_TWO_SIDED );
		qglColor4f( 0.941177, 0.952941, 0.968628, 1.0f );
		R_Brighter_Quad( glConfig.vidWidth, glConfig.vidHeight, 0.35f, 0.35f, 0.2f, 0.2f );



		// brighten.
		GL_State( GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ONE);GL_Bind( tr.dlightImage );	GL_Cull( CT_TWO_SIDED );
		//qglColor4f( 0.941177, 0.952941, 0.968628, 1.0f );
		qglColor4f( (0.9f + (tone[0] * 0.5)), (0.9f + (tone[1] * 0.5)), (0.9f + (tone[2] * 0.5)), 1.0f );

		R_Brighter_Quad( glConfig.vidWidth, glConfig.vidHeight, 0.25f, 0.25f, 0.48f, 0.48f );

		// invert.
		GL_State( GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_ONE_MINUS_DST_COLOR | GLS_DSTBLEND_ONE_MINUS_SRC_COLOR);GL_Bind( tr.whiteImage );	GL_Cull( CT_TWO_SIDED );
	//	qglColor4f(0.85098, 0.85098, 0.815686, 1.0f );
		qglColor4f( (0.8f + (toneinv[0] * 0.5)), (0.8f + (toneinv[1] * 0.5)), (0.8f + (toneinv[2] * 0.5)), 1.0f );
		R_Brighter_Quad( glConfig.vidWidth, glConfig.vidHeight, 0, 0, 1, 1 );

		// brighten.
		GL_State( GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_SRC_COLOR);GL_Bind( tr.whiteImage );	GL_Cull( CT_TWO_SIDED );
		qglColor4f( 0.615686, 0.615686, 0.615686, 1.0f );
		R_Brighter_Quad( glConfig.vidWidth, glConfig.vidHeight, 0, 0, 1, 1 );


		// invoort.
		GL_State( GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_ONE_MINUS_DST_COLOR | GLS_DSTBLEND_ONE_MINUS_SRC_COLOR);GL_Bind( tr.whiteImage );	GL_Cull( CT_TWO_SIDED );
		qglColor4f(1.0f, 1.0f, 1.0f , 1.0f );
		R_Brighter_Quad( glConfig.vidWidth, glConfig.vidHeight, 0, 0, 1, 1 );

		// brighten.
		GL_State( GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_SRC_COLOR);GL_Bind( tr.whiteImage );	GL_Cull( CT_TWO_SIDED );
	//	qglColor4f(  0.866667, 0.847059, 0.776471, 1.0f );
		qglColor4f( (0.73f + (toneinv[0] * 0.4)), (0.73f + (toneinv[1] * 0.4)), (0.73f + (toneinv[2] * 0.4)), 1.0f );
		R_Brighter_Quad( glConfig.vidWidth, glConfig.vidHeight, 0, 0, 1, 1 );





}





// =================================================================
// OLD-STYLE ANTIALIASING
// =================================================================



// leilei - old 2000 console-style antialiasing/antiflickering
void R_RetroAAScreen( void )
{
	if( !r_retroAA->integer)
		return;
	if ( backEnd.doneraa)
		return;
	if ( !vertexShaders )
		return;		// leilei - cards without support for this should not ever activate this
	if( !postproc.started ) {
		force32upload = 1;
		R_Postprocess_InitTextures();
		if( !postproc.started )
			return;
	}

	if ( !backEnd.projection2D )
		RB_SetGL2D();
	force32upload = 1;

	leifxmode = 1233;		// just show it through to tvWidth/tvHeight
	//R_Postprocess_BackupScreen();
	R_Postprocess_BackupScreen();
	R_Bloom_RestoreScreen_Postprocessed();
	
	backEnd.doneraa = qtrue;

	force32upload = 0;
	

}







// =================================================================
// !GLSL!
// ANIME SHADERS
// =================================================================





void R_AnimeScreen( void )
{
	if( !r_anime->integer)
		return;
	if ( backEnd.doneanime)
		return;
	if ( !backEnd.doneSurfaces )
		return;
	if ( !vertexShaders )
		return;		// leilei - cards without support for this should not ever activate this
	if( !postproc.started ) {
		force32upload = 1;
		R_Postprocess_InitTextures();
		if( !postproc.started )
			return;
	}

	if ( !backEnd.projection2D )
	RB_SetGL2D();

	force32upload = 1;

	leifxmode = 888;			// anime effect - outlines, desat, bloom and other crap to go with it
	R_LeiFX_Stupid_Hack();
	R_Postprocess_BackupScreen();
	R_Bloom_RestoreScreen_Postprocessed();
	leifxmode = 999;			// film effect - to blur things slightly, and add some grain and chroma stuff
        R_LeiFX_Stupid_Hack();
	R_Postprocess_BackupScreen();
	R_Bloom_RestoreScreen_Postprocessed();
	backEnd.doneanime = qtrue;

			force32upload = 0;
	

}








// =================================================================
// !GLSL!
// MOTION BLUR EFFECT
// =================================================================


// leilei - motion blur hack
void R_MotionBlur_BackupScreen(int which) {
	if( r_motionblur->integer < 3)
		return;
	if (which == 1){ GL_Bind( postproc.motion1.texture ); qglCopyTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, 0, 0, glConfig.vidWidth, glConfig.vidHeight ); }  // gather thee samples
	if (which == 2){ GL_Bind( postproc.motion2.texture ); qglCopyTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, 0, 0, glConfig.vidWidth, glConfig.vidHeight ); }
	if (which == 3){ GL_Bind( postproc.motion3.texture ); qglCopyTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, 0, 0, glConfig.vidWidth, glConfig.vidHeight ); }
	if (which == 4){ GL_Bind( postproc.motion4.texture ); qglCopyTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, 0, 0, glConfig.vidWidth, glConfig.vidHeight ); } 
	if (which == 5){ GL_Bind( postproc.motion5.texture ); qglCopyTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, 0, 0, glConfig.vidWidth, glConfig.vidHeight ); }
	if (which == 11){ GL_Bind( postproc.mpass1.texture ); qglCopyTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, 0, 0, glConfig.vidWidth, glConfig.vidHeight ); }	// to accum
	if (which == 12){ GL_Bind( postproc.mpass1.texture ); qglCopyTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, 0, 0, glConfig.vidWidth, glConfig.vidHeight ); }	// to accum
	if (which == 13){ GL_Bind( postproc.mpass1.texture ); qglCopyTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, 0, 0, glConfig.vidWidth, glConfig.vidHeight ); }	// to accum
	if (which == 14){ GL_Bind( postproc.mpass1.texture ); qglCopyTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, 0, 0, glConfig.vidWidth, glConfig.vidHeight ); }	// to accum
	if (which == 18){ GL_Bind( postproc.screen.texture ); qglCopyTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, 0, 0, glConfig.vidWidth, glConfig.vidHeight ); }	// to accum
	
	
}


void R_MblurScreen( void )
{
	if( r_motionblur->integer < 3)
		return;
	if ( backEnd.donemblur)
		return;
	if ( !backEnd.doneSurfaces )
		return;
	if ( !vertexShaders )
		return;		// leilei - cards without support for this should not ever activate this
	if( !postproc.started ) {
		force32upload = 1;
		R_Postprocess_InitTextures();
		if( !postproc.started )
			return;
	}

	if ( !backEnd.projection2D )
	RB_SetGL2D();


		

	force32upload = 1;

	leifxmode = 777;			// accumulate frames for the motion blur buffer
	R_LeiFX_Stupid_Hack();
	R_Postprocess_BackupScreen();
	R_Bloom_RestoreScreen_Postprocessed();
	force32upload = 0;
}

void R_MblurScreenPost( void )
{
	if( r_motionblur->integer < 3)
		return;
	if ( !backEnd.doneSurfaces )
		return;
	if ( !vertexShaders )
		return;		// leilei - cards without support for this should not ever activate this
	if( !postproc.started ) {
		force32upload = 1;
		R_Postprocess_InitTextures();
		if( !postproc.started )
			return;
	}

	backEnd.donemblur = qtrue;

	if ( !backEnd.projection2D )
	RB_SetGL2D();

	force32upload = 1;

	leifxmode = 770;			// accumulate frames for the motion blur buffer
	R_LeiFX_Stupid_Hack();
//	R_Postprocess_BackupScreen();
	R_Bloom_RestoreScreen_Postprocessed();
	force32upload = 0;
}



// =================================================================
// TV MODE
// =================================================================

static void R_Postprocess_BackupScreenTV( void ) {

	//int intdiv;
	//intdiv = 1;


	GL_TexEnv( GL_MODULATE );
	qglScissor( 0, 0, glConfig.vidWidth, glConfig.vidHeight );
	qglViewport( 0, 0, glConfig.vidWidth, glConfig.vidHeight );
	qglMatrixMode( GL_PROJECTION );
    qglLoadIdentity ();
	qglOrtho( 0, glConfig.vidWidth, glConfig.vidHeight, 0, 0, 1 );
	qglMatrixMode( GL_MODELVIEW );
    qglLoadIdentity ();


	GL_Bind( postproc.screen.texture );
	qglCopyTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, 0, 0, glConfig.vidWidth, glConfig.vidHeight);

}

static void R_Postprocess_ScaleTV( void ) {


	GL_TexEnv( GL_MODULATE );
	qglScissor( 0, 0, glConfig.vidWidth, glConfig.vidHeight );
	qglViewport( 0, 0, glConfig.vidWidth, glConfig.vidHeight );
	qglMatrixMode( GL_PROJECTION );
    qglLoadIdentity ();

}



// tvmode doesn't do any actual postprocessing yet (ntsc, shadow mask etc)

float tvtime;

void R_TVScreen( void )
{
	if( r_tvMode->integer < 0)
		return;
	if ( backEnd.donetv)
		return;
	if ( !vertexShaders )
		return;		// leilei - cards without support for this should not ever activate this
	if( !postproc.started ) {
		force32upload = 1;
		R_Postprocess_InitTextures();
		if( !postproc.started )
			return;
	}

//	if ( !backEnd.projection2D )
//		RB_SetGL2D();



		force32upload = 1;

//	postprocess = 1;

	if (backEnd.refdef.time > tvtime){
		tvinterlace *= -1;
	tvtime = backEnd.refdef.time + (1000.0f / 60); // 60hz
	}
		
	tvinter = tvinterlace;
	if (tvinter < 0) tvinter = 0;
	if (r_tvMode->integer < 100) tvinter = 0;
	tvinter = 0;

	leifxmode = 1234;		// just show it through to tvWidth/tvHeight

	//if (r_tvMode->integer > 2)
	//	leifxmode = 1236;		// run it through a shader
	//R_Postprocess_BackupScreen();

	if (r_tvMode->integer > 100)
	{
	R_Postprocess_ScaleTV();
	}
	else
	{
	R_Postprocess_BackupScreenTV();

	R_Bloom_RestoreScreen_Postprocessed();
	}
	
	backEnd.donetv = qtrue;

	force32upload = 0;
	

}






// =================================================================
// PALLETIZING
// =================================================================




void R_PaletteScreen( void )
{
	if( !r_palletize->integer)
		return;
	if ( backEnd.donepalette)
		return;
	if ( !backEnd.doneSurfaces )
		return;
	if ( !vertexShaders )
		return;		// leilei - cards without support for this should not ever activate this
	if( !postproc.started ) {
		force32upload = 1;
		R_Postprocess_InitTextures();
		if( !postproc.started )
			return;
	}

	if ( !backEnd.projection2D )
	RB_SetGL2D();

	force32upload = 1;

	leifxmode = 1997;			// anime effect - outlines, desat, bloom and other crap to go with it
	R_LeiFX_Stupid_Hack();
	R_Postprocess_BackupScreen();
	R_Bloom_RestoreScreen_Postprocessed();
	backEnd.donepalette = qtrue;

	force32upload = 0;
	

}




// =================================================================
// WATER BUFFER TEST
// =================================================================
/*

static struct {
	// NO!
} water;
*/
// leilei - experimental water effect
static void R_Water_InitTextures( void )
{
	// NO!
}




void R_InitWaterTextures( void )
{
	// NO!
}



static void R_Water_BackupScreen( void ) 
{
	// NO!
}

static void R_WaterWorks( void )
{
	// NO!
}											


static void R_Water_RestoreScreen( void ) {
	// NO!	
}
 

void R_WaterInit( void ) {
	// NO!
}


void R_WaterScreen( void )
{
	// NO!
}


// Possibly a single water plane can be defined at 0 vertical position.. then render the scene using a new command called WATERREFLECTION; capture to texture and
// another time the same scene using WATERREFRACTION;capture to texture and use these textures for water reflections (should happen before the scene is rendered normally)
// note this will fail if water plane is above of below 0 on vertical axis so could be used with caution by mapping people





