#version 120
/*
 * generic_fp.glsl
 * generic quake 3 shader rendering fragment program
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

/* uniform variables */
uniform int u_TexEnv;
uniform sampler2D u_Texture0;
uniform sampler2D u_Texture1;

uniform int u_TCGen0;
uniform int u_TCGen1;

uniform mat3 u_Lightinfo[32];
uniform int	u_NumLights;

uniform mat4 u_ModelViewMatrix;
uniform mat4 u_ModelViewProjectionMatrix;

varying vec3 vVertex;
varying vec3 n,t,b;
varying mat4 mvm;

uniform int u_etype;
uniform int u_thisStageLightmap;


varying vec4 vColor;
/* multi-texturing modes */
#define REPLACE		0
#define MODULATE	1
#define DECAL		2
#define BLEND		3
#define ADD			4
#define COMBINE		5

vec4 applyTexture(sampler2D textureUnit, int type, int index, vec4 color) {
	vec4 texture = texture2D(textureUnit, gl_TexCoord[index].st);

	if (type == REPLACE) {
		color		= texture;
	} else if (type == MODULATE) {
		color		*= texture;
	} else if (type == DECAL) {
		color		= vec4(mix(color.rgb, texture.rgb, texture.a), color.a);
	} else if (type == BLEND) {
		color		= vec4(mix(color.rgb, gl_TextureEnvColor[index].rgb, texture.rgb), color.a * texture.a);
	} else if (type == ADD) {
		color.rgb	+= texture.rgb;
		color.a		*= texture.a;

		color		= clamp(color, 0.0, 1.0);
	} else {
		color		= clamp(texture * color, 0.0, 1.0);
	}

	return color;
}



void main (void) {

	vec4 vDiffuse=vec4(0.0,0.0,0.0,0.0);
	vec3 texdetail=-vec3(u_ModelViewProjectionMatrix*vec4(normalize(texture2D(u_Texture0,gl_TexCoord[0].st).rgb),0.0))*3.1;
    int i;
    vec3 lVec;

    if ((u_thisStageLightmap>0) || (u_etype==0))
        {
        for (int i=1;i<u_NumLights;i++)
        {
	    mat3 m=u_Lightinfo[i];
	    vec3 lightpos=m[0].xyz;
	    vec4 lVec2 = mvm * vec4(lightpos,1) ;
	    float dist=length(lVec2.xyz- vVertex.xyz);
	    lVec=normalize(lVec2.xyz- vVertex.xyz);
        vec3 lightcolor=m[1].xyz;
        float strength=m[2].x;
//        vDiffuse += max(0.0,dot(-n*texdetail,lVec))*vec4(lightcolor,1) * (strength*10.0/(dist*dist));
        vDiffuse += max(0.0,dot(-n,lVec))*vec4(lightcolor,1) * (strength*10.0/(dist*dist));
        }
        }

    vec4 color=vColor;
    if (u_TCGen0>0)
    {
        if (u_TCGen0==2)
            {
            color=applyTexture(u_Texture0, MODULATE, 0, color);
            //color.rgb+=vDiffuse.rgb;
            }
        else if (u_TCGen0==4)
            {
            color=applyTexture(u_Texture0, MODULATE, 0, color);
            //color.rgb+=vDiffuse.rgb;
            }
        else
        color=applyTexture(u_Texture0, MODULATE, 0, color);
    }

    if (u_TCGen1>0)
    {
    if (u_TexEnv > -1)
    {
        color=applyTexture(u_Texture1, u_TexEnv, 1, color);
        if (u_TCGen1==2)
        color.rgb+=vDiffuse.rgb;
    }
    }

    if ((u_thisStageLightmap>0) && (u_etype==0)) color.rgb+=vDiffuse.rgb;

// 0 -> tcgen 4

if (u_TCGen0==2) color=vec4(1,0,1,1);

gl_FragColor=color;

//if (u_thisStageLightmap>0) gl_FragColor=vec4(1.0,0.0,1.0,1.0);
}
