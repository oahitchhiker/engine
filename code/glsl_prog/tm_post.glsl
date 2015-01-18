//tonemap example - multipart postprocess
vec4 tempColor = color;
float influence=0.9;
vec4 tonemap = color*(1.0-influence)+influence*texture2D(u_Texture6, vec2(floor(tempColor.b/16.0)*16.0+tempColor.r/16.0,tempColor.g));
color=tonemap;


