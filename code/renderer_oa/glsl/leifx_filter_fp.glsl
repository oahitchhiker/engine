// "LeiFX" shader - Pixel filtering process
// 
// 	Copyright (C) 2013-2015 leilei
// 
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or (at your option)
// any later version.

// New undither version

uniform sampler2D u_Texture0; 
varying vec2 texture_coordinate;
uniform float u_ScreenToNextPixelX;
uniform float u_ScreenToNextPixelY;
uniform float u_ScreenSizeX;
uniform float u_ScreenSizeY;

#define 	PIXELWIDTH 	0.66f

#define		FILTCAP		0.075	// filtered pixel should not exceed this 
#define		FILTCAPG	(FILTCAP / 2)


void main()
{
	vec2 px;	// shut
	float PIXELWIDTHH = u_ScreenToNextPixelX;


	px.x = u_ScreenToNextPixelX;
	px.y = u_ScreenToNextPixelY; 
    	// Sampling The Texture And Passing It To The Frame Buffer 
    	gl_FragColor = texture2D(u_Texture0, texture_coordinate); 

	vec4 pixel1 = texture2D(u_Texture0, texture_coordinate + vec2(px.x * PIXELWIDTH, 0)); 
	vec4 pixel2 = texture2D(u_Texture0, texture_coordinate + vec2(-px.x * PIXELWIDTH, 0)); 

	vec4 pixeldiff;			// Pixel difference for the dither check
	vec4 pixelmake;			// Pixel to make for adding to the buffer
	vec4 pixeldiffleft;		// Pixel to the left
	vec4 pixelblend;		

	{

		pixelmake.rgb = 0;
		pixeldiff.rgb = pixel2.rgb- gl_FragColor.rgb;

		pixeldiffleft.rgb = pixel1.rgb - gl_FragColor.rgb;

		if (pixeldiff.r > FILTCAP) 		pixeldiff.r = FILTCAP;
		if (pixeldiff.g > FILTCAPG) 		pixeldiff.g = FILTCAPG;
		if (pixeldiff.b > FILTCAP) 		pixeldiff.b = FILTCAP;

		if (pixeldiff.r < -FILTCAP) 		pixeldiff.r = -FILTCAP;
		if (pixeldiff.g < -FILTCAPG) 		pixeldiff.g = -FILTCAPG;
		if (pixeldiff.b < -FILTCAP) 		pixeldiff.b = -FILTCAP;

		if (pixeldiffleft.r > FILTCAP) 		pixeldiffleft.r = FILTCAP;
		if (pixeldiffleft.g > FILTCAPG) 	pixeldiffleft.g = FILTCAPG;
		if (pixeldiffleft.b > FILTCAP) 		pixeldiffleft.b = FILTCAP;

		if (pixeldiffleft.r < -FILTCAP) 	pixeldiffleft.r = -FILTCAP;
		if (pixeldiffleft.g < -FILTCAPG) 	pixeldiffleft.g = -FILTCAPG;
		if (pixeldiffleft.b < -FILTCAP) 	pixeldiffleft.b = -FILTCAP;

		pixelmake.rgb = (pixeldiff.rgb / 4) + (pixeldiffleft.rgb / 16);
		gl_FragColor.rgb= (gl_FragColor.rgb + pixelmake.rgb);
	}


}	