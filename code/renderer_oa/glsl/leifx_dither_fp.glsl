// LeiFX shader - Pixel dithering and reduction process
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


// This table came from the wikipedia article about Ordered Dithering. NOT MAME.  Just to clarify.
float erroredtable[16] = {
	16,4,13,1,   
	8,12,5,9,
	14,2,15,3,
	6,10,7,11		
};
void main()
{
	
    	// Sampling The Texture And Passing It To The Frame Buffer 
	gl_FragColor = texture2D(u_Texture0, texture_coordinate); 

	vec4 what;

	what.x = 1;	// shut
	what.y = 1;	// up
	what.z = 1;	// warnings
	vec4 huh;

	// *****************
	// STAGE 0
	// Grab our sampled pixels for processing......
	// *****************
	vec2 px;	// shut
	float egh = 1.0f;
	px.x = u_ScreenToNextPixelX;
	px.y =  u_ScreenToNextPixelY;

	vec4	pe1 = texture2D(u_Texture0, texture_coordinate);			// first pixel...
	
	// *****************
	// STAGE 1
	// Ordered dither with lookup table
	// *****************
	int ditdex = 	int(mod(gl_FragCoord.x, 4.0)) * 4 + int(mod(gl_FragCoord.y, 4.0)); // 4x4!
	vec3 color;
	vec3 colord;
	int coloredr;
	int coloredg;
	int coloredb;
	color.r = gl_FragColor.r * 255;
	color.g = gl_FragColor.g * 255;
	color.b = gl_FragColor.b * 255;
	int yeh = 0;
	float ohyes;

	// looping through a lookup table matrix
	for (yeh=ditdex; yeh<(ditdex+16); yeh++) ohyes = erroredtable[yeh-15];

	//ohyes = pow(ohyes, 0.92f) - 3;
	ohyes = floor(ohyes) - 6;

	colord.r = color.r + ohyes;
	colord.g = color.g + (ohyes / 2);
	colord.b = color.b + ohyes;

	if (color.r == 0) colord.r = 0;
	if (color.g == 0) colord.g = 0;
	if (color.b == 0) colord.b = 0;

	//if (color.r < colord.r) 	pe1.r = colord.r / 255; 
	//if (color.g < colord.g) 	pe1.g = colord.g / 255; 
	//if (color.b < colord.b) 	pe1.b = colord.b / 255; 

	pe1.rgb = colord.rgb / 255;

	// *****************
	// STAGE 2
	// Reduce color depth of sampled pixels....
	// *****************

	vec4 reduct;		// 16 bits per pixel (5-6-5)
	reduct.r = 64;
	reduct.g = 128;	// gotta be 16bpp for that green!
	reduct.b = 64;

  	pe1 = pow(pe1, what);  	pe1 = pe1 * reduct;  	pe1 = floor(pe1);  	pe1 = pe1 / reduct;  	pe1 = pow(pe1, what);

	gl_FragColor = pe1;

}	