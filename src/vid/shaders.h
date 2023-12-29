// only included in vid.c

#ifndef B173C_SHADERS_H
#define B173C_SHADERS_H

#define GLSL_VERSION "#version 430 core\n"
#define stringify(...) #__VA_ARGS__

const char *shader_vertex = GLSL_VERSION stringify(
	uniform mat4 MODEL;
	uniform mat4 VIEW;
	uniform mat4 PROJECTION;

	layout(location = 0) in vec3 VERTEX_IN;
	//layout(location = 1) in vec2 DATA_IN;

	out vec2 DATA;

	void main()
	{
		gl_Position = vec4(VERTEX_IN, 1.0f);
		DATA = vec2(0,0);
	}
);

const char *shader_geometry = GLSL_VERSION stringify(
	layout(points) in;
	layout(triangle_strip, max_vertices = 4) out;

	in vec2[] DATA;

	uniform mat4 MODEL;
	uniform mat4 VIEW;
	uniform mat4 PROJECTION;

	out float blockId;
	out float shade;

	void vert(float ox, float oy, float oz)
	{
		gl_Position = PROJECTION * VIEW * MODEL * (gl_in[0].gl_Position + vec4(ox, oy, oz, 0.0f));
		EmitVertex();
	}

	void main()
	{
		blockId = 2.0f;//DATA[0].x;
		shade = 255.0f;

		/* create a single face */
		vert(-0.5f, -0.5f, 0.0f);
		vert(+0.5f, -0.5f, 0.0f);
		vert(-0.5f, +0.5f, 0.0f);
		vert(+0.5f, +0.5f, 0.0f);

		EndPrimitive();

	}
);

const char *shader_fragment = GLSL_VERSION stringify(
	out vec4 COLOR;
	in float blockId;
	in float shade;

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
		gl_Position = vec4(org, 0.0f, 1.0f);
		UV = TEXCOORD;
		FONTCHAR = int(FONTDATA.z);
		FONTCOLOR = int(FONTDATA.w);
	}
);

#define COLORCODE(cod, color) "if(code == " #cod ") return vec4(" #color ", 1);"
const char *shader_fragment2d = GLSL_VERSION stringify(

	in vec2 UV;
	flat in int FONTCHAR;
	flat in int FONTCOLOR;

	out vec4 COLOR;

	uniform sampler2D TEXTURE0;

	vec4 get_color_from_code(int code)
	{
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
		return vec4(1);
	}

	void main()
	{
		vec2 newuv;

		newuv.x = float((FONTCHAR % 16 + UV.x) * 8) / 128.0f;
		newuv.y = float((FONTCHAR / 16 + UV.y) * 8) / 128.0f;

		COLOR = texture(TEXTURE0, newuv) * get_color_from_code(FONTCOLOR);
	}
);
#undef COLORCODE

#endif