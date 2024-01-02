// only included in vid.c

#ifndef B173C_SHADERS_H
#define B173C_SHADERS_H

#define GLSL_VERSION "#version 430 core\n"
#define stringify(...) #__VA_ARGS__

const char *shader_vertex = GLSL_VERSION stringify(
	layout(location=0) in vec4 VERTEX_IN;

	void main()
	{
		gl_Position = VERTEX_IN + vec4(0.5f, 0.5f, 0.5f, 0.0f);
	}
);

const char *shader_geometry = GLSL_VERSION stringify(
	layout(points) in;
	layout(triangle_strip, max_vertices = 24) out;

	uniform mat4 VIEW;
	uniform mat4 PROJECTION;

	out flat int blockId;
	out flat int metadata;
	out flat float shade;
	out vec2 UV;

	int modulo(int a, int b)
	{
		return a - (b * int(floor(a / b)));
	}

	void vert(float ox, float oy, float oz, float u, float v)
	{
		gl_Position = PROJECTION * VIEW * (vec4(gl_in[0].gl_Position.xyz, 1.0f) + vec4(ox, oy, oz, 0.0f));
		UV.x = u;
		UV.y = v;
		EmitVertex();
	}

	void main()
	{
		int data =
		int(gl_in[0].gl_Position.w);
		int sides = data & 63;
		blockId = (data >> 6) & 255;
		int renderType = (data >> 14) & 31;
		metadata = data >> 19;
		shade = 250.0f;

		/* reference image for the comments */
		/* persp: looking down on a cube    */
		/* that only has its back walls     */
		/*              __-__               */
		/*         ..```  |  ```..          */
		/*   <---  |      |      |  --->    */
		/*   -x    |    _.+._    |    -z    */
		/*         |..``     ``..|          */
		/*           ```.._..```            */
		/*                |                 */
		/*                | -y              */
		/*                V                 */

		if(renderType == 0 || renderType == 2) {
			float x1 = 0.5f;
			float x2 = ((renderType == 2) ? (0.5f - 1.0f / 16.0f) : (0.5f));
			float y = 0.5f;
			float z1 = 0.5f;
			float z2 = ((renderType == 2) ? (0.5f - 1.0f / 16.0f) : (0.5f));
			if((sides & 32) != 0) {
				/* z- */
				shade = 150.0f;
				vert(-x1, -y, -z2, 1.0f, 1.0f); /* bot left */
				vert(-x1, +y, -z2, 1.0f, 0.0f); /* top left */
				vert(+x1, -y, -z2, 0.0f, 1.0f); /* bot right */
				vert(+x1, +y, -z2, 0.0f, 0.0f); /* top right */
				EndPrimitive();
			}
			if((sides & 16) != 0) {
				/* z+ */
				shade = 150.0f;
				vert(-x1, -y, +z2, 0.0f, 1.0f); /* bot left */
				vert(+x1, -y, +z2, 1.0f, 1.0f); /* bot right */
				vert(-x1, +y, +z2, 0.0f, 0.0f); /* top left */
				vert(+x1, +y, +z2, 1.0f, 0.0f); /* top right */
				EndPrimitive();
			}
			if((sides & 8) != 0) {
				/* x- */
				shade = 200.0f;
				vert(-x2, -y, -z1, 0.0f, 1.0f); /* bot right */
				vert(-x2, -y, +z1, 1.0f, 1.0f); /* bot left */
				vert(-x2, +y, -z1, 0.0f, 0.0f); /* top right */
				vert(-x2, +y, +z1, 1.0f, 0.0f); /* top left */
				EndPrimitive();
			}
			if((sides & 4) != 0) {
				/* x+ */
				shade = 200.0f;
				vert(+x2, -y, -z1, 1.0f, 1.0f); /* bot right */
				vert(+x2, +y, -z1, 1.0f, 0.0f); /* top right */
				vert(+x2, -y, +z1, 0.0f, 1.0f); /* bot left */
				vert(+x2, +y, +z1, 0.0f, 0.0f); /* top left */
				EndPrimitive();
			}
			if((sides & 2) != 0) {
				/* y- */
				shade = 100.0f;
				vert(-x1, -y, -z1, 0.0f, 0.0f); /* top */
				vert(+x1, -y, -z1, 1.0f, 0.0f); /* right */
				vert(-x1, -y, +z1, 0.0f, 1.0f); /* left */
				vert(+x1, -y, +z1, 1.0f, 1.0f); /* bottom */
				EndPrimitive();
			}
			if((sides & 1) != 0) {
				/* y+ */
				shade = 255.0f;
				vert(-x1, +y, +z1, 0.0f, 1.0f); /* left */
				vert(+x1, +y, +z1, 1.0f, 1.0f); /* bottom */
				vert(-x1, +y, -z1, 0.0f, 0.0f); /* top */
				vert(+x1, +y, -z1, 1.0f, 0.0f); /* right */
				EndPrimitive();
			}
		} else if(renderType == 1) {
			float r = sqrt(2) * 0.0625f;
			shade = 255.0f;
			vert(-0.5f + r, -0.5f, -0.5f + r, 1.0f, 1.0f);
			vert(-0.5f + r, +0.5f, -0.5f + r, 1.0f, 0.0f);
			vert(+0.5f - r, -0.5f, +0.5f - r, 0.0f, 1.0f);
			vert(+0.5f - r, +0.5f, +0.5f - r, 0.0f, 0.0f);
			EndPrimitive();
			vert(+0.5f - r, -0.5f, -0.5f + r, 1.0f, 1.0f);
			vert(+0.5f - r, +0.5f, -0.5f + r, 1.0f, 0.0f);
			vert(-0.5f + r, -0.5f, +0.5f - r, 0.0f, 1.0f);
			vert(-0.5f + r, +0.5f, +0.5f - r, 0.0f, 0.0f);
			EndPrimitive();
			vert(-0.5f + r, -0.5f, -0.5f + r, 1.0f, 1.0f);
			vert(+0.5f - r, -0.5f, +0.5f - r, 0.0f, 1.0f);
			vert(-0.5f + r, +0.5f, -0.5f + r, 1.0f, 0.0f);
			vert(+0.5f - r, +0.5f, +0.5f - r, 0.0f, 0.0f);
			EndPrimitive();
			vert(+0.5f - r, -0.5f, -0.5f + r, 1.0f, 1.0f);
			vert(-0.5f + r, -0.5f, +0.5f - r, 0.0f, 1.0f);
			vert(+0.5f - r, +0.5f, -0.5f + r, 1.0f, 0.0f);
			vert(-0.5f + r, +0.5f, +0.5f - r, 0.0f, 0.0f);
			EndPrimitive();
		} else if(renderType == 3) {
			bool is_plate = blockId == 70 || blockId == 72 ;
			float h = is_plate ? (1.0f / 16.0f) : 0.5f;
			float x1 = 0.5f - (is_plate ? h : 0.0f);
			float z1 = 0.5f - (is_plate ? h : 0.0f);
			float y1 =  h-0.5f;
			float y2 =   -0.5f;
			shade = 200.0f;
			vert(-x1, y1, +z1,      h,  1.0f -  h);
			vert(+x1, y1, +z1, 1.0f-h,  1.0f -  h);
			vert(-x1, y1, -z1,      h,  h);
			vert(+x1, y1, -z1, 1.0f-h,  h);
			EndPrimitive();
			vert(-x1, y2, -z1, 1.0f - h, 1.0f    ); /* bot left */
			vert(-x1, y1, -z1, 1.0f - h, 1.0f - h); /* top left */
			vert(+x1, y2, -z1,        h, 1.0f    ); /* bot right */
			vert(+x1, y1, -z1,        h, 1.0f - h); /* top right */
			EndPrimitive();
			vert(-x1, y2, +z1,        h, 1.0f    ); /* bot left */
			vert(+x1, y2, +z1, 1.0f - h, 1.0f    ); /* bot right */
			vert(-x1, y1, +z1,        h, 1.0f - h); /* top left */
			vert(+x1, y1, +z1, 1.0f - h, 1.0f - h); /* top right */
			EndPrimitive();
			vert(-x1, y2, -z1,        h, 1.0f    ); /* bot right */
			vert(-x1, y2, +z1, 1.0f - h, 1.0f    ); /* bot left */
			vert(-x1, y1, -z1,        h, 1.0f - h); /* top right */
			vert(-x1, y1, +z1, 1.0f - h, 1.0f - h); /* top left */
			EndPrimitive();
			vert(+x1, y1, -z1, 1.0f - h, 1.0f    ); /* bot right */
			vert(+x1, y2, -z1, 1.0f - h, 1.0f - h); /* top right */
			vert(+x1, y1, +z1,        h, 1.0f    ); /* bot left */
			vert(+x1, y2, +z1,        h, 1.0f - h); /* top left */
			EndPrimitive();
		}
	}
);

const char *shader_fragment = GLSL_VERSION stringify(
	uniform int mode;

	uniform sampler2D TEXTURE0;

	out vec4 COLOR;
	in vec2 UV;
	in flat int blockId;
	in flat int metadata;
	in flat float shade;

	int modulo(int a, int b)
	{
		return a - (b * int(floor(a / b)));
	}

	void main()
	{
		/*
  		newuv.x = float((modulo(FONTCHAR, 16) + UV.x) * 8) / 128.0f;
		newuv.y = float((FONTCHAR / 16 + UV.y) * 8) / 128.0f;
		 */
		vec4 COLORMOD = vec4(1);

		int tex_y =       ((blockId - 1) / 16);
		int tex_x = (modulo(blockId - 1, 16));

		if(blockId == 2) {
			if(shade == 200.0f || shade == 150.0f) {
				tex_y = 15;
				tex_x = 15;
			} else if(shade == 100.0f) {
				tex_y =        (int(3 - 1) / 16);
				tex_x = (modulo(int(3) - 1, 16));
			} else if(shade == 255.0f) {
				COLORMOD.rgb = vec3(0.3,0.85,0.3);
			}
		} else if(blockId == 31 || blockId == 18) {
			COLORMOD.rgb = vec3(0.3,0.85,0.3);
		} else if(blockId == 17 && (shade == 255.0f || shade == 100.0f)) {
			tex_y = 15;
			tex_x = 14;
		} else if(blockId == 81) {
			if(shade == 255.0f) {
				tex_y = 15;
				tex_x = 12;
			} else if(shade == 100.0f) {
				tex_y = 15;
				tex_x = 13;
			}
		} else if(blockId == 58) {
			if(shade == 150.0f) {
				tex_y = 14;
				tex_x = 15;
			} else if(shade == 255.0f) {
				tex_y = 13;
				tex_x = 15;
 			} else if(shade == 100.0f) {
				tex_y = 0;
				tex_x = 4;
			}
		} else if(blockId == 72 || blockId == 44) {
			tex_y = 0;
			tex_x = 4;
		} else if(blockId == 70) {
			tex_y = 0;
			tex_x = 0;
		}

		vec2 newuv;
		newuv.x = float(tex_x) / 16.0f + UV.x / 16.0f;
		newuv.y = float(tex_y) / 16.0f + UV.y / 16.0f;
		COLOR = texture(TEXTURE0, newuv) * COLORMOD * vec4(vec3(shade / 256.0f), 1.0f);

		if(mode == 1 && COLOR.a == 0) {
			discard;
		} else if(mode == 0 && COLOR.a < 1) {
			discard;
		}

	}

);

const char *shader_vertex2d = GLSL_VERSION stringify(
	layout(location = 0) in vec2 VERTEX;
	layout(location = 1) in vec2 TEXCOORD;
	layout(location = 2) in vec4 FONTDATA;

	out vec2 UV;
	flat out int FONTCHAR;
	flat out int FONTCOLOR;

	void main()
	{
		/* move to top left */
		vec2 org = VERTEX + vec2(-1.0f, 1.0f);
		/* add instance offset */
		/* x2 because opengl is -1 to 1, offset is 0 to 1 */
		org += vec2(FONTDATA.x, -FONTDATA.y) * 2;
		gl_Position = vec4(org, -1.0f, 1.0f);
		UV = TEXCOORD;

		FONTCHAR = int(FONTDATA.z);
		FONTCOLOR = int(FONTDATA.w);

		if(FONTCOLOR < 0) {
			/* shadow - draw behind */
			gl_Position.z += 1.0f;
		}
	}
);

#define COLORCODE(cod, color) "if(code == " #cod ") return vec4(mul,mul,mul,1) * vec4(" #color ", 1);"
const char *shader_fragment2d = GLSL_VERSION stringify(

	in vec2 UV;
	flat in int FONTCHAR;
	flat in int FONTCOLOR;

	out vec4 COLOR;

	uniform sampler2D TEXTURE0;

	vec4 get_color_from_code(int code)
	{
		float mul = 1.0f;
		if(code < 0) {
			code = -code;
			mul = 0.25f;
		}
		float a = 170.0f / 256.0f;
		float b = 85.0f / 256.0f;
	)
		COLORCODE(0x0, vec3(0))
		COLORCODE(0x1, vec3(0,0,a))
		COLORCODE(0x2, vec3(0,a,0))
		COLORCODE(0x3, vec3(0,a,a))
		COLORCODE(0x4, vec3(a,0,0))
		COLORCODE(0x5, vec3(a,0,a))
		COLORCODE(0x6, vec3(1,a,0))
		COLORCODE(0x7, vec3(a,a,a))
		COLORCODE(0x8, vec3(b,b,b))
		COLORCODE(0x9, vec3(b,b,1))
		COLORCODE(0xa, vec3(b,1,b))
		COLORCODE(0xb, vec3(b,1,1))
		COLORCODE(0xc, vec3(1,b,b))
		COLORCODE(0xd, vec3(1,b,1))
		COLORCODE(0xe, vec3(1,1,b))
		COLORCODE(0xf, vec3(1))
	stringify(
		return vec4(mul,mul,mul,1);
	}

	int modulo(int a, int b)
	{
		return a - (b * int(floor(a / b)));
	}

	void main()
	{
		vec2 newuv;

		newuv.x = float((modulo(FONTCHAR, 16) + UV.x) * 8) / 128.0f;
		newuv.y = float((FONTCHAR / 16 + UV.y) * 8) / 128.0f;

		COLOR = texture(TEXTURE0, newuv) * get_color_from_code(FONTCOLOR);
		if(COLOR.a < 0.5) {
			discard;
		}
	}
);
#undef COLORCODE

#endif
