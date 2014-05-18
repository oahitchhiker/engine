// LeiFX shader - Pixel filtering process
// 
// 	Copyright (C) 2013-2014 leilei
// 
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or (at your option)
// any later version.



uniform sampler2D u_Texture0; 
varying vec2 texture_coordinate;
uniform float u_ScreenToNextPixelX;
uniform float u_ScreenToNextPixelY;
uniform float u_ScreenSizeX;
uniform float u_ScreenSizeY;

#define 	PIXELWIDTH 0.66f
#define		BLURAMOUNT 0.5f

void main()
{
	vec2 px;	// shut



	px.x = u_ScreenToNextPixelX;
	px.y = u_ScreenToNextPixelY; 
    	// Sampling The Texture And Passing It To The Frame Buffer 
    	gl_FragColor = texture2D(u_Texture0, texture_coordinate); 


	// A more direct port from the HLSL code i've written.  Which rocks.  Yeah.
	float blendy;	// to blend unblended with blend... trying to smooth the jag :(
	float blenda;
	float blendfactor;

	vec4 pixel1 = texture2D(u_Texture0, texture_coordinate + vec2(px.x * PIXELWIDTH / 4, 0)); 
	vec4 pixel2 = texture2D(u_Texture0, texture_coordinate + vec2(-px.x * PIXELWIDTH, 0)); 
	vec4 pixel0 = texture2D(u_Texture0, texture_coordinate + vec2(0, 0)); 

	vec4 pixelblend;


	vec4	onee = 1;	// because I can't just stick 1 in the GLSL dot function
	float gary1 = dot(pixel1,onee);
	float gary2 = dot(pixel2,onee);

	float mean = 1.0;
	mean = gary1 - gary2;

	if (mean < 0)	mean *= -1;
	//if (mean > 1) 	mean = 1;	
	mean = pow(mean, BLURAMOUNT);	// Adjust this value if you want to control the blur...

	if (mean > 1) mean = 1;	

	{
		// variably BLEND IT ALL TO H*CK!!!!
		blendy = 1 - mean;
		blenda = 1 - blendy;
		pixel0 /= 3;
		pixel1 /= 3;
		pixel2 /= 3;
   		pixelblend.rgb = pixel0 + pixel1 + pixel2;
		gl_FragColor.rgb = (pixelblend.rgb * blendy) + (gl_FragColor.rgb * blenda);
	}
}	