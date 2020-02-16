#version 450
// #extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec3 fragColor;

vec2 positions[9] = vec2[](
    vec2(-1.0, -1.0), vec2( 0.0, -1.0), vec2( 1.0, -1.0),
    vec2(-1.0,  0.0), vec2( 0.0,  0.0), vec2( 1.0,  0.0),
    vec2(-1.0,  1.0), vec2( 0.0,  1.0), vec2( 1.0,  1.0)
);

#define miR 0.0
#define maR 1.0
#define miB 0.0
#define maB 1.0

vec3 colors[9] = vec3[](
    vec3(maR, 0.0, 0.0), vec3(1.0, 1.0, 1.0), vec3(0.0, 0.0, maB),
    vec3(maR, 0.0, 0.0), vec3(0.0, 1.0, 0.0), vec3(0.0, 0.0, maB),
    vec3(miR, 0.0, 0.0), vec3(0.0, 0.0, 0.0), vec3(0.0, 0.0, miB)
);

// 0 1 2
// 3 4 5
// 6 7 8

int indices[12] = int[](
	// 0,1,3, 3,1,4, 1,2,5, 1,5,4,

	// 3,4,7, 3,7,6, 4,5,7, 7,5,8

	0,1,6, 6,1,7, 1,2,7, 7,2,8
);

void main()
{
    gl_Position = vec4(positions[indices[gl_VertexIndex]], 0.0, 1.0);
    fragColor = colors[indices[gl_VertexIndex]];
}
