#version 330 core
in vec2 TexCoords;
out vec4 color;

uniform sampler2D image;
uniform float pixelSize;
uniform vec2 resolution;

void main()
{
    vec2 texelSize = vec2(pixelSize) / resolution;
    vec2 coord = TexCoords - mod(TexCoords, texelSize) + texelSize * 0.5;
    color = texture(image, coord);
}