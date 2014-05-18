// Color control shaders
// 
// 	Copyright (C) 2013-2014 leilei
// 
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or (at your option)
// any later version.


uniform sampler2D u_Texture0; 
varying vec2 texture_coordinate;

uniform float u_CC_Overbright; 
uniform float u_CC_Brightness; 
uniform float u_CC_Gamma; 
uniform float u_CC_Contrast; 
uniform float u_CC_Saturation; 


float erroredtable[16] = {
	14,4,15,1,   
	8,12,5,10,
	15,2,14,3,
	6,12,7,11		
};

void main()
{
	
    	gl_FragColor = texture2D(u_Texture0, texture_coordinate); 

	// Dither setup...
	vec4 OldColor; // compare differences for amplifying a correctional dither...
	float OldCol = 2.0f / 255;
	int ditdex = 	int(mod(gl_FragCoord.x, 4.0)) * 4 + int(mod(gl_FragCoord.y, 4.0)); // 4x4!
	// looping through a lookup table matrix
	vec3 color;
	vec3 colord;
	int coloredr;
	int coloredg;
	int coloredb;
	color.r = 1;
	color.g = 1;
	color.b = 1;
	int yeh = 0;
	float ohyes;

	for (yeh=ditdex; yeh<(ditdex+16); yeh++) ohyes = erroredtable[yeh-15];
	ohyes = floor(ohyes) / 24;
	OldColor = OldCol;	// reset oldcolor

	// Overbrights
    	gl_FragColor *= (u_CC_Overbright + 1);

	OldColor *= (u_CC_Overbright + 1);
	gl_FragColor.rgb += (ohyes * OldColor.rgb);	// dither this stage
	OldColor = OldCol; 	// reset oldcolor

	// Gamma Correction
	float gamma = u_CC_Gamma;

	gl_FragColor.r = pow(gl_FragColor.r, 1.0 / gamma);
	gl_FragColor.g = pow(gl_FragColor.g, 1.0 / gamma);
	gl_FragColor.b = pow(gl_FragColor.b, 1.0 / gamma);

	OldColor.r = pow(OldColor.r, 1.0 / gamma);
	OldColor.g = pow(OldColor.g, 1.0 / gamma);
	OldColor.b = pow(OldColor.b, 1.0 / gamma);
	gl_FragColor.rgb += (ohyes * OldColor.rgb);	// dither this stage

}	