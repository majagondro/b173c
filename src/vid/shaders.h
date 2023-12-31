// only included in vid.c

#ifndef B173C_SHADERS_H
#define B173C_SHADERS_H

#define GLSL_VERSION "#version 430 core\n"
#define stringify(...) #__VA_ARGS__

const char *shader_vertex = GLSL_VERSION stringify(
	layout(location = 0) in vec4 VERTEX_IN;
	//layout(location = 1) in vec2 DATA_IN;

	out vec2 DATA;

	void main()
	{
		gl_Position = VERTEX_IN;
		DATA = vec2(0,0);
	}
);

const char *shader_geometry = GLSL_VERSION stringify(
	layout(points) in;
	layout(triangle_strip, max_vertices = 4) out;

	in vec2[] DATA;

	uniform mat4 VIEW;
	uniform mat4 PROJECTION;

	out flat float blockId;
	out flat float shade;

	int modulo(int a, int b)
	{
		return a - (b * int(floor(a / b)));
	}

	void vert(float ox, float oy, float oz)
	{
		gl_Position = PROJECTION * VIEW * (vec4(gl_in[0].gl_Position.xyz, 1.0f) + vec4(ox, oy, oz, 0.0f));
		EmitVertex();
	}

	void main()
	{
		int side = modulo(int(gl_in[0].gl_Position.w), 10);
		blockId = float(int(gl_in[0].gl_Position.w) / 10);
		shade = 250.0f;
		/*vert(-0.5f, -0.5f, -0.5f);
		EndPrimitive();
		return;*/

		/* reference image for the comments */
		/* persp: looking down on a cube    */
		/* that only has its back walls     */
		/*              __-__               */
		/*         ..```  |  ```..          */
		/*   <---  |      |      |  --->    */
		/*   -x    |    ..+._    |    -z    */
		/*         |..``     ``..|          */
		/*           ```.._..```            */
		/*                |                 */
		/*                | -y              */
		/*                V                 */

		if(side == 0) {
			/* z- */
			shade = 150.0f;
			vert(-0.5f, -0.5f, -0.5f); /* bot left */
			vert(-0.5f, +0.5f, -0.5f); /* top left */
			vert(+0.5f, -0.5f, -0.5f); /* bot right */
			vert(+0.5f, +0.5f, -0.5f); /* top right */
		} else if(side == 1) {
			/* z+ */
			shade = 150.0f;
			vert(-0.5f, -0.5f, +0.5f); /* bot left */
			vert(+0.5f, -0.5f, +0.5f); /* bot right */
			vert(-0.5f, +0.5f, +0.5f); /* top left */
			vert(+0.5f, +0.5f, +0.5f); /* top right */
		} else if(side == 2) {
			/* x- */
			shade = 200.0f;
			vert(-0.5f, -0.5f, -0.5f); /* bot right */
			vert(-0.5f, -0.5f, +0.5f); /* bot left */
			vert(-0.5f, +0.5f, -0.5f); /* top right */
			vert(-0.5f, +0.5f, +0.5f); /* top left */
		} else if(side == 3) {
			/* x+ */
			shade = 200.0f;
			vert(+0.5f, -0.5f, -0.5f); /* bot right */
			vert(+0.5f, +0.5f, -0.5f); /* top right */
			vert(+0.5f, -0.5f, +0.5f); /* bot left */
			vert(+0.5f, +0.5f, +0.5f); /* top left */
		} else if(side == 4) {
			/* y- */
			shade = 250.0f;
			vert(-0.5f, -0.5f, -0.5f); /* top */
			vert(+0.5f, -0.5f, -0.5f); /* right */
			vert(-0.5f, -0.5f, +0.5f); /* left */
			vert(+0.5f, -0.5f, +0.5f); /* bottom */
		} else if(side == 5) {
			/* y+ */
			shade = 250.0f;
			vert(-0.5f, +0.5f, +0.5f); /* left */
			vert(+0.5f, +0.5f, +0.5f); /* bottom */
			vert(-0.5f, +0.5f, -0.5f); /* top */
			vert(+0.5f, +0.5f, -0.5f); /* right */
		}

		EndPrimitive();
	}
);

const char *shader_fragment = GLSL_VERSION stringify(
	out vec4 COLOR;
	in flat float blockId;
	in flat float shade;

	void main()
	{
		COLOR.a = 1;
		if(blockId == 1) {
			COLOR.rgb = vec3(0.2f);
		} else if(blockId == 3 || blockId == 17) {
			COLOR.rgb = vec3(0.6f, 0.4f, 0.2f);
		} else if(blockId == 2 || blockId == 18) {
			COLOR.rgb = vec3(0.2f, 0.5f, 0.2f);
		} else if(blockId == 9) {
			COLOR.rgb = vec3(0.15f, 0.15f, 0.8f);
			COLOR.a = 0.5f;
		} else if(blockId == 12) {
			COLOR.rgb = vec3(0.9f, 0.9f, 0.6f);
		} else if(blockId == 81) {
			COLOR.rgb = vec3(0.1f, 0.3f, 0.1f);
		} else {
			COLOR.rgb = vec3(blockId / 256.0f);
		}
		COLOR.rgb *= shade / 256.0f;
		//COLOR = vec4(1);
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
