// only included in vid.c

#ifndef B173C_SHADERS_H
#define B173C_SHADERS_H

#define GLSL_VERSION "#version 430 core\n"
#define stringify(...) #__VA_ARGS__

extern const char *blocks_v_glsl;
extern const char *blocks_g_glsl;
extern const char *blocks_f_glsl;

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
			gl_Position.z += 0.001f;
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
