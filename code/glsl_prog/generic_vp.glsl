#version 120
/*
 * generic_vp.glsl
 * generic quake 3 shader rendering vertex program
 * Copyright (C) 2010  Jens Loehr <jens.loehr@gmx.de>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

attribute vec3 u_Normal;
attribute vec3 u_Tangent;
attribute vec3 u_Binormal;

/* pi */
#define M_PI 3.14159265358979323846

/* texCoordGen_t */
#define TCGEN_IDENTITY 1
#define TCGEN_LIGHTMAP 2
#define TCGEN_TEXTURE 3

/* uniform variables */
uniform float u_IdentityLight;
uniform int u_AlphaGen;
uniform int u_ColorGen;
uniform int u_Greyscale;
uniform int u_TCGen0;
uniform int u_TCGen1;
uniform int u_TexEnv;
uniform vec3 u_AmbientLight;
uniform vec3 u_DirectedLight;
uniform vec3 u_DynamicLight;
uniform float u_LightDistance;
uniform vec3 u_LightDirection;
uniform vec4 u_ConstantColor;
uniform vec4 u_FogColor;
uniform vec4 u_EntityColor;

varying mat4 mvm;

/*
 * DeformGeometry
 * Deform geometry (deformVertexes)
 */
vec4 DeformGeometry(vec4 position) {
	// STUB
	return position;
}

/*
 * ComputeTexCoords
 * Compute texture coordinates
 */
vec4 ComputeTexCoords(vec4 texCoord0, vec4 texCoord1, int tcGen) {
	vec4 texCoord;

	if (tcGen == TCGEN_LIGHTMAP) {
		texCoord = texCoord1;
	} else {
		texCoord = texCoord0;
	}

	return texCoord;
}

/*
 * main
 * Entry point for generic vertex program
 */
 uniform mat4 u_ModelViewMatrix;

varying vec3 vVertex;
varying vec3 t,b,n;
varying vec4 vColor;


void main(void) {
	vec4 color;


    mvm=u_ModelViewMatrix;
	vVertex=vec3(mvm*gl_Vertex);

	n=normalize(gl_NormalMatrix*u_Normal);//*gl_NormalMatrix);
	t=normalize(gl_NormalMatrix*u_Tangent);//*gl_NormalMatrix);
	b=cross(normalize(t),normalize(n));

	/* vertex position */
	gl_Position = DeformGeometry(ftransform());

	/* vertex color */
	//color = ComputeColor(gl_Color);
	color = gl_Color;
	gl_FrontColor = color;
	gl_BackColor = color;

    vColor=color;

	/* texture coordinates */
	if (u_TCGen0>0)
	gl_TexCoord[0] = ComputeTexCoords(gl_MultiTexCoord0, gl_MultiTexCoord1, u_TCGen0);

	/* multi-texturing */
	if (u_TexEnv > -1) {
        if (u_TCGen1>0)
		gl_TexCoord[1] = ComputeTexCoords(gl_MultiTexCoord0, gl_MultiTexCoord1, u_TCGen1);
	}


	if (u_TCGen0==4)
	{
	vec3 u = normalize( vec3(gl_ModelViewMatrix * gl_Vertex) );
	vec3 n2 = normalize( gl_NormalMatrix * u_Normal);
	vec3 r = reflect( u, n2 );
	float m = 2.0 * sqrt( r.x*r.x + r.y*r.y + (r.z+1.0)*(r.z+1.0) );
	gl_TexCoord[0].s = r.x/m + 0.5;
	gl_TexCoord[0].t = r.y/m + 0.5;
    }
	if (u_TCGen1==4)
	{
	vec3 xu = normalize( vec3(gl_ModelViewMatrix * gl_Vertex) );
	vec3 xn2 = normalize( gl_NormalMatrix * u_Normal);
	vec3 xr = reflect( xu, xn2 );
	float xm = 2.0 * sqrt( xr.x*xr.x + xr.y*xr.y + (xr.z+1.0)*(xr.z+1.0) );
	gl_TexCoord[1].s = xr.x/xm + 0.5;
	gl_TexCoord[1].t = xr.y/xm + 0.5;
    }
}
